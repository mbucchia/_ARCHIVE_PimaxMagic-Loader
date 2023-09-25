#include "pch.h"

namespace logging {
    //
    // Log file helpers.
    //

    namespace {

        std::ofstream logStream;

        void InternalLog(const char* fmt, va_list va) {
            const std::time_t now = std::time(nullptr);

            char buf[1024];
            size_t offset = std::strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S %z: ", std::localtime(&now));
            vsnprintf_s(buf + offset, sizeof(buf) - offset, _TRUNCATE, fmt, va);
            OutputDebugStringA(buf);
            if (logStream.is_open()) {
                logStream << buf;
                logStream.flush();
            }
        }

    } // namespace

    void Log(const char* fmt, ...) {
        va_list va;
        va_start(va, fmt);
        InternalLog(fmt, va);
        va_end(va);
    }

    void StartLogging() {
        const auto localAppData = std::filesystem::path(getenv("LOCALAPPDATA")) / "PimaxMagic-Loader";
        CreateDirectoryA(localAppData.string().c_str(), nullptr);

        char path[_MAX_PATH];
        GetModuleFileNameA(nullptr, path, sizeof(path));
        std::filesystem::path executable(path);
        executable = executable.filename();

        // Start logging to file.
        if (!logStream.is_open()) {
            std::string logFile = (localAppData / ("PimaxMagic-Loader-" + executable.string() + ".log")).string();
            logStream.open(logFile, std::ios_base::ate);
        }

        Log("Hello World from '%s'!\n", path);
    }

} // namespace logging

namespace utils {
    //
    // Utility functions.
    //

    std::wstring RegGetString(HKEY hKey, const std::wstring& subKey, const std::wstring& value) {
        DWORD dataSize = 0;
        LONG retCode = ::RegGetValue(
            hKey, subKey.c_str(), value.c_str(), RRF_SUBKEY_WOW6464KEY | RRF_RT_REG_SZ, nullptr, nullptr, &dataSize);
        if (retCode != ERROR_SUCCESS || !dataSize) {
            return {};
        }

        std::wstring data(dataSize / sizeof(wchar_t), 0);
        retCode = ::RegGetValue(hKey,
                                subKey.c_str(),
                                value.c_str(),
                                RRF_SUBKEY_WOW6464KEY | RRF_RT_REG_SZ,
                                nullptr,
                                (PVOID)data.data(),
                                &dataSize);
        if (retCode != ERROR_SUCCESS) {
            return L"";
        }

        return data.substr(0, dataSize / sizeof(wchar_t) - 1);
    }

} // namespace utils

namespace {

    void LoadLibMagic() {
        static std::future<void> asyncLoad;
        static std::mutex asyncLoadMutex;

        // Bypass LibMagic for system services.
        char path[_MAX_PATH];
        GetModuleFileNameA(nullptr, path, sizeof(path));
        std::filesystem::path executable(path);
        executable = executable.filename();
        if (executable == "vrcompositor.exe" || executable == "vrdashboard.exe" || executable == "vrmonitor.exe" ||
            executable == "vrwebhelper.exe" || executable == "vrstartup.exe" || executable == "VarjoTracking.exe" ||
            executable == "VarjoHome.exe") {
            logging::Log("Bypassing LibMagic for system service: %s\n", executable.string().c_str());
            return;
        }

        std::unique_lock lock(asyncLoadMutex);

        if (!asyncLoad.valid()) {
            // Defer loading by 5s to make sure openvr_api finished loading.
            asyncLoad = std::async(std::launch::async, []() {
                using namespace std::chrono_literals;

                std::this_thread::sleep_for(5s);

                logging::Log("Loading LibMagic...\n");

                HMODULE ourModule = nullptr;
                GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS |
                                       GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
                                   (LPCWSTR)&LoadLibMagic,
                                   &ourModule);
                wchar_t path[_MAX_PATH];
                GetModuleFileNameW(ourModule, path, _MAX_PATH);
                std::filesystem::path root(path);
                root = root.parent_path();

                HMODULE libMagic = LoadLibraryW((root / L"LibMagicD3D1164.dll").c_str());
                logging::Log("LibMagic handle = %p\n", libMagic);
            });
        }
    }

    // Chain-load the real vrclient_x64.dll.
    HMODULE realVrClient = nullptr;
    void LoadRealVrClient(bool loadLibMagic = true) {
        static std::mutex realVrClientLoadMutex;

        std::unique_lock lock(realVrClientLoadMutex);

        if (realVrClient) {
            return;
        }

        // Try to use the local folder...
        {
            HMODULE ourModule = nullptr;
            GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
                               (LPCWSTR)&LoadRealVrClient,
                               &ourModule);
            wchar_t path[_MAX_PATH];
            GetModuleFileNameW(ourModule, path, _MAX_PATH);
            std::filesystem::path root(path);
            root = root.parent_path();
            realVrClient = LoadLibraryW((root / L"real_vrclient_x64.dll").c_str());
            if (realVrClient) {
                logging::Log("Redirecting to local copy of vrclient_x64...\n");
            }
        }

        // Otherwise fallback to using the SteamVR install folder.
        if (!realVrClient) {
            std::wstring steamVrPath =
                utils::RegGetString(HKEY_LOCAL_MACHINE, L"SOFTWARE\\WOW6432Node\\Valve\\Steam", L"InstallPath");
            if (!steamVrPath.empty()) {
                steamVrPath += L"\\steamapps\\common\\SteamVR\\";
                logging::Log("Using SteamVR at: %ls\n", steamVrPath.c_str());
            }
            realVrClient = LoadLibraryW((steamVrPath + L"bin\\vrclient_x64.dll").c_str());
        }

        logging::Log("vrclient_x64 handle = %p\n", realVrClient);

        if (loadLibMagic) {
            // Initiate loading of Pimax Magic.
            LoadLibMagic();
        }
    }

} // namespace

//
// DLL entry point.
//

BOOL APIENTRY DllMain(HMODULE hModule, DWORD ul_reason_for_call, LPVOID lpReserved) {
    switch (ul_reason_for_call) {
    case DLL_PROCESS_ATTACH:
        logging::StartLogging();
        break;

    case DLL_THREAD_ATTACH:
    case DLL_THREAD_DETACH:
    case DLL_PROCESS_DETACH:
        break;
    }
    return TRUE;
}

//
// Bridged OpenVR symbols.
//

#define VR_CALLTYPE __cdecl

#define BRIDGE_FUNC(name, ...)                                                                                         \
    static decltype(&name) pfnRealFunc = nullptr;                                                                      \
    if (!pfnRealFunc) {                                                                                                \
        LoadRealVrClient();                                                                                            \
        pfnRealFunc = (decltype(&name))GetProcAddress(realVrClient, #name);                                            \
    }                                                                                                                  \
    return pfnRealFunc(__VA_ARGS__);

#define BRIDGE_FUNC_NO_LOAD(name, ...)                                                                                 \
    static decltype(&name) pfnRealFunc = nullptr;                                                                      \
    if (!pfnRealFunc) {                                                                                                \
        LoadRealVrClient(false);                                                                                       \
        pfnRealFunc = (decltype(&name))GetProcAddress(realVrClient, #name);                                            \
    }                                                                                                                  \
    return pfnRealFunc(__VA_ARGS__);

uint32_t VR_CALLTYPE HmdSystemFactory() {
    BRIDGE_FUNC(HmdSystemFactory);
}

vr::IVRApplications* VR_CALLTYPE VRApplications() {
    BRIDGE_FUNC(VRApplications);
}

vr::IVRChaperone* VR_CALLTYPE VRChaperone() {
    BRIDGE_FUNC(VRChaperone);
}

vr::IVRChaperoneSetup* VR_CALLTYPE VRChaperoneSetup() {
    BRIDGE_FUNC(VRChaperoneSetup);
}

void* VR_CALLTYPE VRClientCoreFactory(const char* pInterfaceName, int* pReturnCode) {
    BRIDGE_FUNC(VRClientCoreFactory, pInterfaceName, pReturnCode);
}

vr::IVRCompositor* VR_CALLTYPE VRCompositor() {
    BRIDGE_FUNC(VRCompositor);
}

vr::IVRExtendedDisplay* VR_CALLTYPE VRExtendedDisplay() {
    BRIDGE_FUNC(VRExtendedDisplay);
}

vr::IVRNotifications* VR_CALLTYPE VRNotifications() {
    BRIDGE_FUNC(VRNotifications);
}

vr::IVROverlay* VR_CALLTYPE VROverlay() {
    BRIDGE_FUNC(VROverlay);
}

vr::IVRRenderModels* VR_CALLTYPE VRRenderModels() {
    BRIDGE_FUNC(VRRenderModels);
}

vr::IVRSettings* VR_CALLTYPE VRSettings() {
    BRIDGE_FUNC(VRSettings);
}

vr::IVRSystem* VR_CALLTYPE VRSystem() {
    BRIDGE_FUNC(VRSystem);
}

vr::IVRTrackedCamera* VR_CALLTYPE VRTrackedCamera() {
    BRIDGE_FUNC(VRTrackedCamera);
}

void* VR_CALLTYPE VR_GetGenericInterface(const char* interfaceVersion, vr::EVRInitError* error) {
    BRIDGE_FUNC(VR_GetGenericInterface, interfaceVersion, error);
}

uint32_t VR_CALLTYPE VR_GetInitToken() {
    BRIDGE_FUNC(VR_GetInitToken);
}

char* VR_CALLTYPE VR_GetStringForHmdError(int err) {
    BRIDGE_FUNC(VR_GetStringForHmdError, err);
}

const char* VR_CALLTYPE VR_GetVRInitErrorAsEnglishDescription(vr::EVRInitError error) {
    BRIDGE_FUNC(VR_GetVRInitErrorAsEnglishDescription, error);
}

const char* VR_CALLTYPE VR_GetVRInitErrorAsSymbol(vr::EVRInitError error) {
    BRIDGE_FUNC(VR_GetVRInitErrorAsSymbol, error);
}

vr::IVRSystem* VR_CALLTYPE VR_Init(vr::EVRInitError* peError,
                                   vr::EVRApplicationType eApplicationType,
                                   const char* pStartupInfo) {
    BRIDGE_FUNC(VR_Init, peError, eApplicationType, pStartupInfo);
}

uint32_t VR_CALLTYPE VR_InitInternal(vr::EVRInitError* peError, vr::EVRApplicationType eApplicationType) {
    BRIDGE_FUNC(VR_InitInternal, peError, eApplicationType);
}

uint32_t VR_CALLTYPE VR_InitInternal2(vr::EVRInitError* peError,
                                      vr::EVRApplicationType eApplicationType,
                                      const char* pStartupInfo) {
    BRIDGE_FUNC(VR_InitInternal2, peError, eApplicationType, pStartupInfo);
}

bool VR_CALLTYPE VR_IsHmdPresent() {
    BRIDGE_FUNC(VR_IsHmdPresent);
}

bool VR_CALLTYPE VR_IsInterfaceVersionValid(const char* pchInterfaceVersion) {
    BRIDGE_FUNC(VR_IsInterfaceVersionValid, pchInterfaceVersion);
}

bool VR_CALLTYPE VR_IsRuntimeInstalled() {
    BRIDGE_FUNC(VR_IsRuntimeInstalled);
}

const char* VR_CALLTYPE VR_RuntimePath() {
    BRIDGE_FUNC(VR_RuntimePath);
}

void VR_CALLTYPE VR_Shutdown() {
    BRIDGE_FUNC(VR_Shutdown);
}

void VR_CALLTYPE VR_ShutdownInternal() {
    BRIDGE_FUNC(VR_ShutdownInternal);
}

XrResult XRAPI_CALL xrNegotiateLoaderRuntimeInterface(const XrNegotiateLoaderInfo* loaderInfo,
                                                      XrNegotiateRuntimeRequest* runtimeRequest) {
    BRIDGE_FUNC_NO_LOAD(xrNegotiateLoaderRuntimeInterface, loaderInfo, runtimeRequest);
}

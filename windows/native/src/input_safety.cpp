#include "input_safety.h"

#include <objbase.h>
#include <uiautomation.h>
#include <wrl/client.h>

#include <algorithm>
#include <array>
#include <string_view>

namespace ruswitcher {
namespace {

constexpr LONG_PTR kPasswordStyle = ES_PASSWORD;

constexpr std::wstring_view kDeniedProcesses[] = {
    // Terminals / Consoles (protect from backspace/synthetic typing corruption)
    L"windowsterminal.exe",
    L"cmd.exe",
    L"powershell.exe",
    L"pwsh.exe",
    L"conhost.exe",
    L"bash.exe",
    L"wsl.exe",
    L"mintty.exe",
    L"alacritty.exe",
    L"wezterm-gui.exe",
    L"kitty.exe",
    L"putty.exe",

    // Password Managers (security boundary)
    L"1password.exe",
    L"bitwarden.exe",
    L"keepass.exe",
    L"keepassxc.exe",

    // Remote Desktop Clients (defer to remote host)
    L"mstsc.exe",
    L"teamviewer.exe",
    L"anydesk.exe"
};

Microsoft::WRL::ComPtr<IUIAutomation> g_automation;
bool g_automation_attempted = false;

void init_automation() noexcept {
    if (g_automation_attempted) return;
    g_automation_attempted = true;
    HRESULT hr = CoCreateInstance(CLSID_CUIAutomation, nullptr, CLSCTX_INPROC_SERVER,
                                  IID_PPV_ARGS(&g_automation));
    if (FAILED(hr)) {
        g_automation = nullptr;
    }
}

bool native_password_style(HWND focused) noexcept {
    return focused && (GetWindowLongPtrW(focused, GWL_STYLE) & kPasswordStyle) != 0;
}

}  // namespace

std::wstring get_window_process_name(HWND window) noexcept {
    if (!window || !IsWindow(window)) return {};
    DWORD pid = 0;
    GetWindowThreadProcessId(window, &pid);
    if (!pid) return {};

    HANDLE process = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, pid);
    if (!process) return {};

    wchar_t path[MAX_PATH]{};
    DWORD size = static_cast<DWORD>(std::size(path));
    if (!QueryFullProcessImageNameW(process, 0, path, &size)) {
        CloseHandle(process);
        return {};
    }
    CloseHandle(process);

    std::wstring_view full(path, size);
    auto last_slash = full.find_last_of(L"\\/");
    std::wstring name(last_slash != std::wstring_view::npos ? full.substr(last_slash + 1) : full);
    for (wchar_t& c : name) {
        c = static_cast<wchar_t>(towlower(c));
    }
    return name;
}

bool is_denied_process(std::wstring_view process_name) noexcept {
    if (process_name.empty()) return false;
    for (const auto& denied : kDeniedProcesses) {
        if (process_name == denied) return true;
    }
    return false;
}

bool is_protected_foreground() noexcept {
    const HWND foreground = GetForegroundWindow();
    if (!foreground) return false;

    // 1. Check denied process policy (Terminals, Password Managers, Remote Desktop)
    const std::wstring proc_name = get_window_process_name(foreground);
    if (is_denied_process(proc_name)) return true;

    // 2. Check native Win32 ES_PASSWORD style
    const DWORD thread = GetWindowThreadProcessId(foreground, nullptr);
    GUITHREADINFO gui{sizeof(gui)};
    const HWND focused = GetGUIThreadInfo(thread, &gui) && gui.hwndFocus ? gui.hwndFocus : foreground;
    if (native_password_style(focused)) return true;

    // 3. Fast check via cached UI Automation
    init_automation();
    if (!g_automation) return false;

    Microsoft::WRL::ComPtr<IUIAutomationElement> element;
    if (FAILED(g_automation->GetFocusedElement(&element)) || !element) return false;

    BOOL is_password = FALSE;
    return SUCCEEDED(element->get_CurrentIsPassword(&is_password)) && is_password;
}

}  // namespace ruswitcher

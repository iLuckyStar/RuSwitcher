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

// 1. Password Managers: Never inspect or modify input in credential managers
constexpr std::wstring_view kPasswordManagers[] = {
    L"1password.exe",
    L"bitwarden.exe",
    L"keepass.exe",
    L"keepassxc.exe",
    L"enpass.exe",
    L"dashlane.exe"
};

// 2. Terminals and consoles: Protect from accidental auto-convert on Space/Enter/Tab.
// Manual conversion (TriggerKey, Pause/Break, Shift+Pause, Double Ctrl) remains enabled!
constexpr std::wstring_view kTerminalProcesses[] = {
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
    L"putty.exe"
};

// 3. Code editors and IDEs: Protect code typing from accidental auto-convert on boundary
constexpr std::wstring_view kCodeEditors[] = {
    L"code.exe",
    L"code - insiders.exe",
    L"devenv.exe",
    L"sublime_text.exe",
    L"notepad++.exe",
    L"cursor.exe",
    L"idea64.exe",
    L"pycharm64.exe",
    L"clion64.exe",
    L"webstorm64.exe",
    L"rider64.exe",
    L"studio64.exe"
};

// 4. Remote Desktop clients: Let remote machine manage layout and conversion
constexpr std::wstring_view kRemoteDesktopProcesses[] = {
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

bool is_password_manager(std::wstring_view process_name) noexcept {
    if (process_name.empty()) return false;
    for (const auto& pm : kPasswordManagers) {
        if (process_name == pm) return true;
    }
    return false;
}

bool is_terminal_process(std::wstring_view process_name) noexcept {
    if (process_name.empty()) return false;
    for (const auto& term : kTerminalProcesses) {
        if (process_name == term) return true;
    }
    return false;
}

bool is_code_editor(std::wstring_view process_name) noexcept {
    if (process_name.empty()) return false;
    for (const auto& editor : kCodeEditors) {
        if (process_name == editor) return true;
    }
    return false;
}

bool is_remote_desktop_process(std::wstring_view process_name) noexcept {
    if (process_name.empty()) return false;
    for (const auto& rdp : kRemoteDesktopProcesses) {
        if (process_name == rdp) return true;
    }
    return false;
}

bool is_auto_convert_denied(std::wstring_view process_name) noexcept {
    return is_terminal_process(process_name) ||
           is_code_editor(process_name) ||
           is_remote_desktop_process(process_name) ||
           is_password_manager(process_name);
}

bool is_denied_process(std::wstring_view process_name) noexcept {
    return is_auto_convert_denied(process_name);
}

bool is_protected_foreground() noexcept {
    const HWND foreground = GetForegroundWindow();
    if (!foreground) return false;

    // 1. Check password manager processes
    const std::wstring proc_name = get_window_process_name(foreground);
    if (is_password_manager(proc_name)) return true;

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

#pragma once

#include <windows.h>
#include <string>
#include <string_view>

namespace ruswitcher {
bool is_protected_foreground() noexcept;
bool is_password_manager(std::wstring_view process_name) noexcept;
bool is_terminal_process(std::wstring_view process_name) noexcept;
bool is_code_editor(std::wstring_view process_name) noexcept;
bool is_remote_desktop_process(std::wstring_view process_name) noexcept;
bool is_auto_convert_denied(std::wstring_view process_name) noexcept;
bool is_denied_process(std::wstring_view process_name) noexcept;
std::wstring get_window_process_name(HWND window) noexcept;
}

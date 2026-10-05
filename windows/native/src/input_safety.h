#pragma once

#include <windows.h>
#include <string>
#include <string_view>

namespace ruswitcher {
bool is_protected_foreground() noexcept;
bool is_denied_process(std::wstring_view process_name) noexcept;
std::wstring get_window_process_name(HWND window) noexcept;
}

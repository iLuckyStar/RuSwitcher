#pragma once

#include <windows.h>

namespace ruswitcher {

class Engine;
class Settings;

class SettingsDialog {
public:
    static void show(HWND parent, HINSTANCE instance, Engine& engine, Settings& settings) noexcept;
};

}  // namespace ruswitcher

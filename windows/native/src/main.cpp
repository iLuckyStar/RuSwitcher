#include <objbase.h>
#include <ole2.h>
#include <windows.h>

#include "engine.h"
#include "settings.h"
#include "tray.h"

namespace {
constexpr UINT kTriggerMessage = WM_APP + 1;
constexpr UINT kBoundaryMessage = WM_APP + 3;
constexpr UINT kSwitchMessage = WM_APP + 4;
constexpr UINT kCaseMessage = WM_APP + 5;

ruswitcher::Engine* g_engine{};
ruswitcher::Tray* g_tray{};

LRESULT CALLBACK window_proc(HWND window, UINT message, WPARAM wparam, LPARAM lparam) noexcept {
    if (message == kTriggerMessage && g_engine) {
        g_engine->convert_or_undo();
        return 0;
    }
    if (message == kBoundaryMessage && g_engine) {
        g_engine->on_boundary_triggered();
        return 0;
    }
    if (message == kSwitchMessage && g_engine) {
        g_engine->switch_layout_direct();
        return 0;
    }
    if (message == kCaseMessage && g_engine) {
        g_engine->change_case();
        return 0;
    }
    LRESULT result{};
    if (g_tray && g_tray->handle(message, wparam, lparam, result)) return result;
    return DefWindowProcW(window, message, wparam, lparam);
}
}

int WINAPI wWinMain(HINSTANCE instance, HINSTANCE, PWSTR, int) {
    HANDLE single_instance = CreateMutexW(nullptr, TRUE, L"Local\\RuSwitcher");
    if (!single_instance || GetLastError() == ERROR_ALREADY_EXISTS) {
        if (single_instance) CloseHandle(single_instance);
        return 0;
    }

    const HRESULT com = OleInitialize(nullptr);

    const wchar_t class_name[] = L"RuSwitcher.Native.MessageWindow";
    WNDCLASSW window_class{};
    window_class.lpfnWndProc = window_proc;
    window_class.hInstance = instance;
    window_class.lpszClassName = class_name;
    if (!RegisterClassW(&window_class)) {
        if (SUCCEEDED(com)) OleUninitialize();
        CloseHandle(single_instance);
        return 1;
    }

    const HWND window = CreateWindowExW(WS_EX_TOOLWINDOW, class_name, L"RuSwitcher", WS_POPUP,
                                        0, 0, 0, 0, nullptr, nullptr, instance, nullptr);
    if (!window) {
        if (SUCCEEDED(com)) OleUninitialize();
        CloseHandle(single_instance);
        return 1;
    }

    ruswitcher::Settings settings;
    ruswitcher::Engine engine(window, &settings);
    engine.set_enabled(settings.enabled());
    engine.set_layout_pair(settings.first_layout(), settings.second_layout());
    g_engine = &engine;
    if (!engine.install()) {
        MessageBoxW(nullptr, L"RuSwitcher could not install the input hooks.", L"RuSwitcher",
                    MB_OK | MB_ICONERROR);
        g_engine = nullptr;
        DestroyWindow(window);
        if (SUCCEEDED(com)) OleUninitialize();
        CloseHandle(single_instance);
        return 1;
    }

    ruswitcher::Tray tray(window, instance, engine, settings);
    g_tray = &tray;
    if (!tray.show()) {
        MessageBoxW(nullptr, L"RuSwitcher could not create its notification icon.", L"RuSwitcher",
                    MB_OK | MB_ICONERROR);
        g_tray = nullptr;
        g_engine = nullptr;
        DestroyWindow(window);
        if (SUCCEEDED(com)) OleUninitialize();
        CloseHandle(single_instance);
        return 1;
    }

    MSG message{};
    while (GetMessageW(&message, nullptr, 0, 0) > 0) {
        TranslateMessage(&message);
        DispatchMessageW(&message);
    }

    g_tray = nullptr;
    g_engine = nullptr;
    DestroyWindow(window);
    if (SUCCEEDED(com)) OleUninitialize();
    ReleaseMutex(single_instance);
    CloseHandle(single_instance);
    return 0;
}

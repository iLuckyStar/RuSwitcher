#include "tray.h"

#include "engine.h"
#include "settings.h"
#include "settings_dialog.h"

#include <shellapi.h>

namespace ruswitcher {
namespace {
constexpr UINT kTrayMessage = WM_APP + 2;

constexpr UINT kCommandEnabled = 100;
constexpr UINT kCommandLine = 101;
constexpr UINT kCommandChangeCase = 102;
constexpr UINT kCommandFixTwoCaps = 103;
constexpr UINT kCommandFixNumbers = 104;
constexpr UINT kCommandAutoConvert = 105;
constexpr UINT kCommandWholeLine = 106;
constexpr UINT kCommandSound = 107;
constexpr UINT kCommandAutostart = 108;
constexpr UINT kCommandSettings = 109;
constexpr UINT kCommandAbout = 110;
constexpr UINT kCommandExit = 111;
constexpr UINT kCommandScopeWord = 112;
constexpr UINT kCommandScopeLine = 113;

constexpr UINT kTriggerBase = 200;      // 200..204
constexpr UINT kSwitchBase = 210;       // 210..215
constexpr UINT kCaseBase = 220;         // 220..224
constexpr UINT kFirstLayoutBase = 300;  // 300..389
constexpr UINT kSecondLayoutBase = 400; // 400..489
constexpr UINT kIconId = 1;

UINT checked(bool value) noexcept { return MF_STRING | (value ? MF_CHECKED : MF_UNCHECKED); }

std::wstring current_layout_display_name() {
    const HWND foreground = GetForegroundWindow();
    const DWORD thread = GetWindowThreadProcessId(foreground, nullptr);
    const HKL hkl = GetKeyboardLayout(thread);
    const LANGID lang = PRIMARYLANGID(LOWORD(reinterpret_cast<ULONG_PTR>(hkl)));
    if (lang == LANG_RUSSIAN) return L"Русский";
    if (lang == LANG_ENGLISH) return L"English";
    if (lang == LANG_UKRAINIAN) return L"Українська";
    if (lang == LANG_BELARUSIAN) return L"Беларуская";
    if (lang == LANG_HEBREW) return L"עברית";
    return L"Активная";
}

}  // namespace

struct Tray::Impl {
    HWND window{};
    HINSTANCE instance{};
    Engine& engine;
    Settings& settings;
    NOTIFYICONDATAW icon{};
    UINT taskbar_created{};
    bool visible{};

    Impl(HWND owner, HINSTANCE module, Engine& input, Settings& preferences) noexcept
        : window(owner), instance(module), engine(input), settings(preferences) {}

    bool add() noexcept {
        icon = {};
        icon.cbSize = sizeof(icon);
        icon.hWnd = window;
        icon.uID = 1;
        icon.uFlags = NIF_MESSAGE | NIF_ICON | NIF_TIP;
        icon.uCallbackMessage = kTrayMessage;
        icon.hIcon = LoadIconW(instance, MAKEINTRESOURCEW(kIconId));
        if (!icon.hIcon) icon.hIcon = LoadIconW(nullptr, IDI_APPLICATION);
        lstrcpynW(icon.szTip, engine.enabled() ? L"RuSwitcher — активен" : L"RuSwitcher — на паузе",
                  static_cast<int>(std::size(icon.szTip)));
        visible = Shell_NotifyIconW(NIM_ADD, &icon) != FALSE;
        if (visible) {
            icon.uVersion = NOTIFYICON_VERSION_4;
            Shell_NotifyIconW(NIM_SETVERSION, &icon);
        }
        return visible;
    }

    void refresh() noexcept {
        if (!visible) return;
        icon.uFlags = NIF_TIP;
        lstrcpynW(icon.szTip, engine.enabled() ? L"RuSwitcher — активен" : L"RuSwitcher — на паузе",
                  static_cast<int>(std::size(icon.szTip)));
        Shell_NotifyIconW(NIM_MODIFY, &icon);
    }

    void show_menu() noexcept {
        engine.remember_foreground();
        const HMENU menu = CreatePopupMenu();
        const HMENU scope_menu = CreatePopupMenu();
        const HMENU trigger_menu = CreatePopupMenu();
        const HMENU switch_menu = CreatePopupMenu();
        const HMENU case_menu = CreatePopupMenu();
        const HMENU first_menu = CreatePopupMenu();
        const HMENU second_menu = CreatePopupMenu();
        if (!menu || !scope_menu || !trigger_menu || !switch_menu || !case_menu || !first_menu || !second_menu) return;

        // Current layout indicator
        std::wstring indicator = L"⌨  Раскладка: " + current_layout_display_name();
        AppendMenuW(menu, MF_STRING | MF_GRAYED | MF_DISABLED, 0, indicator.c_str());
        AppendMenuW(menu, MF_SEPARATOR, 0, nullptr);

        AppendMenuW(menu, checked(engine.enabled()), kCommandEnabled, L"Включено");
        AppendMenuW(menu, MF_SEPARATOR, 0, nullptr);

        // Scope submenu (Whole line vs Word / Selection)
        AppendMenuW(scope_menu, checked(!settings.convert_whole_line()),
                    kCommandScopeWord, L"Последнее слово / выделенный текст (стандартно)");
        AppendMenuW(scope_menu, checked(settings.convert_whole_line()),
                    kCommandScopeLine, L"Вся строка целиком (Shift+Home)");
        AppendMenuW(menu, MF_POPUP, reinterpret_cast<UINT_PTR>(scope_menu), L"Область конвертации");

        // Trigger submenu (conversion hotkey)
        for (int i = 0; i <= 4; ++i) {
            auto k = static_cast<TriggerKey>(i);
            AppendMenuW(trigger_menu, checked(settings.trigger() == k),
                        kTriggerBase + static_cast<UINT>(i), Settings::trigger_name(k));
        }
        AppendMenuW(menu, MF_POPUP, reinterpret_cast<UINT_PTR>(trigger_menu), L"Способ вызова (Конвертация)");

        // Switch hotkey submenu (instant layout change)
        for (int i = 0; i <= 5; ++i) {
            auto k = static_cast<SwitchKey>(i);
            AppendMenuW(switch_menu, checked(settings.switch_hotkey() == k),
                        kSwitchBase + static_cast<UINT>(i), Settings::switch_name(k));
        }
        AppendMenuW(menu, MF_POPUP, reinterpret_cast<UINT_PTR>(switch_menu), L"Клавиша смены раскладки");

        // Case hotkey submenu
        for (int i = 0; i <= 4; ++i) {
            auto k = static_cast<CaseKey>(i);
            AppendMenuW(case_menu, checked(settings.case_hotkey() == k),
                        kCaseBase + static_cast<UINT>(i), Settings::case_name(k));
        }
        AppendMenuW(menu, MF_POPUP, reinterpret_cast<UINT_PTR>(case_menu), L"Смена регистра (a ↔ A)");

        AppendMenuW(menu, MF_SEPARATOR, 0, nullptr);
        AppendMenuW(menu, checked(settings.auto_convert()), kCommandAutoConvert,
                    L"Авто-конвертация при наборе (Space / Enter / Tab)");
        AppendMenuW(menu, checked(settings.fix_two_caps()), kCommandFixTwoCaps,
                    L"Исправлять две заглавные (ПРивет → Привет)");
        AppendMenuW(menu, checked(settings.fix_numbers()), kCommandFixNumbers,
                    L"Исправлять опечатки в цифрах (1ю8 → 1.8)");
        AppendMenuW(menu, checked(settings.sound_on_switch()), kCommandSound,
                    L"Звук при переключении раскладки");

        AppendMenuW(menu, MF_SEPARATOR, 0, nullptr);
        AppendMenuW(menu, MF_STRING, kCommandLine, L"Конвертировать текущую строку сейчас");
        AppendMenuW(menu, MF_STRING, kCommandChangeCase, L"Сменить регистр слова / выделенного");

        AppendMenuW(menu, MF_SEPARATOR, 0, nullptr);
        const auto& layouts = settings.layouts();
        for (std::size_t index = 0; index < layouts.size() && index < 90; ++index) {
            AppendMenuW(first_menu, checked(layouts[index].handle == settings.first_layout()),
                        kFirstLayoutBase + static_cast<UINT>(index), layouts[index].name.c_str());
            AppendMenuW(second_menu, checked(layouts[index].handle == settings.second_layout()),
                        kSecondLayoutBase + static_cast<UINT>(index), layouts[index].name.c_str());
        }
        AppendMenuW(menu, MF_POPUP, reinterpret_cast<UINT_PTR>(first_menu), L"Первая раскладка");
        AppendMenuW(menu, MF_POPUP, reinterpret_cast<UINT_PTR>(second_menu), L"Вторая раскладка");
        AppendMenuW(menu, checked(settings.autostart_enabled()), kCommandAutostart,
                    L"Запуск при входе в систему");
        AppendMenuW(menu, MF_STRING, kCommandSettings, L"Настройки...");

        AppendMenuW(menu, MF_SEPARATOR, 0, nullptr);
        AppendMenuW(menu, MF_STRING, kCommandAbout, L"О программе RuSwitcher...");
        AppendMenuW(menu, MF_STRING, kCommandExit, L"Выход");

        POINT point{};
        GetCursorPos(&point);
        engine.remember_foreground();
        SetForegroundWindow(window);
        const UINT cmd = TrackPopupMenu(menu, TPM_RIGHTBUTTON | TPM_BOTTOMALIGN | TPM_RETURNCMD,
                                        point.x, point.y, 0, window, nullptr);
        DestroyMenu(menu);
        PostMessageW(window, WM_NULL, 0, 0);

        if (cmd != 0) {
            if (cmd == kCommandLine || cmd == kCommandChangeCase) {
                Sleep(50);
            }
            command(cmd);
        }
    }

    void command(UINT id) noexcept {
        if (id == kCommandEnabled) {
            const bool value = !engine.enabled();
            engine.set_enabled(value);
            settings.set_enabled(value);
            refresh();
        } else if (id >= kTriggerBase && id <= kTriggerBase + 4) {
            settings.set_trigger(static_cast<TriggerKey>(id - kTriggerBase));
        } else if (id >= kSwitchBase && id <= kSwitchBase + 5) {
            settings.set_switch_hotkey(static_cast<SwitchKey>(id - kSwitchBase));
        } else if (id >= kCaseBase && id <= kCaseBase + 4) {
            settings.set_case_hotkey(static_cast<CaseKey>(id - kCaseBase));
        } else if (id == kCommandAutoConvert) {
            settings.set_auto_convert(!settings.auto_convert());
        } else if (id == kCommandFixTwoCaps) {
            settings.set_fix_two_caps(!settings.fix_two_caps());
        } else if (id == kCommandFixNumbers) {
            settings.set_fix_numbers(!settings.fix_numbers());
        } else if (id == kCommandScopeWord) {
            settings.set_convert_whole_line(false);
        } else if (id == kCommandScopeLine) {
            settings.set_convert_whole_line(true);
        } else if (id == kCommandSound) {
            settings.set_sound_on_switch(!settings.sound_on_switch());
        } else if (id == kCommandLine) {
            engine.convert_line();
        } else if (id == kCommandChangeCase) {
            engine.change_case();
        } else if (id == kCommandAutostart) {
            settings.set_autostart(!settings.autostart_enabled());
        } else if (id == kCommandSettings) {
            SettingsDialog::show(window, instance, engine, settings);
        } else if (id == kCommandAbout) {
            MessageBoxW(nullptr,
                        L"RuSwitcher для Windows v0.10.0-beta.2\n\n"
                        L"Единый функционал с флагманской версией macOS 3.5.0b:\n"
                        L"• Переключение раскладки: по Caps Lock, двойному Ctrl/Shift/Alt или Pause/Break.\n"
                        L"• Конвертация слова: на лету и по горячей клавише.\n"
                        L"• Конвейер конца слова: авто-исправление по Space, Enter и Tab.\n"
                        L"• Две заглавные буквы: «ПРивет» → «Привет».\n"
                        L"• Опечатки в цифрах: «1ю8» → «1.8», «5б2» → «5,2».\n"
                        L"• База брендов: 154 термина и AI-сервиса (ChatGPT, Claude, Docker, GitHub...).\n"
                        L"• Смена регистра: строчные → ПРОПИСНЫЕ → Заглавные.\n"
                        L"• Полностью автономный нативный Win32 C++ бинарник (~210 КБ).\n\n"
                        L"Автор macOS-версии: Rashid Sayfutdinov (@rashn | ruswitcher.app)\n"
                        L"Windows Native Port: @iLuckyStar",
                        L"О программе RuSwitcher", MB_OK | MB_ICONINFORMATION);
        } else if (id == kCommandExit) {
            PostQuitMessage(0);
        } else if (id >= kFirstLayoutBase && id < kFirstLayoutBase + 90) {
            const std::size_t index = id - kFirstLayoutBase;
            if (index < settings.layouts().size()) {
                settings.set_first_layout(settings.layouts()[index].handle);
                engine.set_layout_pair(settings.first_layout(), settings.second_layout());
            }
        } else if (id >= kSecondLayoutBase && id < kSecondLayoutBase + 90) {
            const std::size_t index = id - kSecondLayoutBase;
            if (index < settings.layouts().size()) {
                settings.set_second_layout(settings.layouts()[index].handle);
                engine.set_layout_pair(settings.first_layout(), settings.second_layout());
            }
        }
    }

    ~Impl() {
        if (visible) Shell_NotifyIconW(NIM_DELETE, &icon);
    }
};

Tray::Tray(HWND window, HINSTANCE instance, Engine& engine, Settings& settings) noexcept
    : impl_(new Impl(window, instance, engine, settings)) {
    impl_->taskbar_created = RegisterWindowMessageW(L"TaskbarCreated");
}
Tray::~Tray() { delete impl_; }
bool Tray::show() noexcept { return impl_->add(); }
bool Tray::handle(UINT message, WPARAM wparam, LPARAM lparam, LRESULT& result) noexcept {
    if (message == impl_->taskbar_created) {
        impl_->visible = false;
        impl_->add();
        result = 0;
        return true;
    }
    if (message == kTrayMessage) {
        const UINT event = LOWORD(lparam);
        if (event == WM_CONTEXTMENU || event == WM_RBUTTONUP || event == WM_LBUTTONUP)
            impl_->show_menu();
        result = 0;
        return true;
    }
    if (message == WM_COMMAND) {
        impl_->command(LOWORD(wparam));
        result = 0;
        return true;
    }
    return false;
}

}  // namespace ruswitcher

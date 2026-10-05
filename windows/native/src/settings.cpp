#include "settings.h"

#include <vector>

namespace ruswitcher {
namespace {

constexpr wchar_t kSettingsKey[] = L"Software\\RuSwitcher";
constexpr wchar_t kRunKey[] = L"Software\\Microsoft\\Windows\\CurrentVersion\\Run";
constexpr wchar_t kRunValue[] = L"RuSwitcher";

HKEY open_settings(REGSAM access) noexcept {
    HKEY key{};
    return RegCreateKeyExW(HKEY_CURRENT_USER, kSettingsKey, 0, nullptr, 0, access, nullptr,
                           &key, nullptr) == ERROR_SUCCESS
               ? key
               : nullptr;
}

ULONG_PTR read_layout(HKEY key, const wchar_t* name) noexcept {
    ULONGLONG value{};
    DWORD type{};
    DWORD size = sizeof(value);
    return RegQueryValueExW(key, name, nullptr, &type, reinterpret_cast<BYTE*>(&value), &size) ==
                       ERROR_SUCCESS &&
                   type == REG_QWORD
               ? static_cast<ULONG_PTR>(value)
               : 0;
}

bool read_bool(HKEY key, const wchar_t* name, bool default_val) noexcept {
    DWORD value = default_val ? 1 : 0;
    DWORD type{};
    DWORD size = sizeof(value);
    if (RegQueryValueExW(key, name, nullptr, &type, reinterpret_cast<BYTE*>(&value), &size) ==
            ERROR_SUCCESS &&
        type == REG_DWORD) {
        return value != 0;
    }
    return default_val;
}

DWORD read_dword(HKEY key, const wchar_t* name, DWORD default_val) noexcept {
    DWORD value = default_val;
    DWORD type{};
    DWORD size = sizeof(value);
    if (RegQueryValueExW(key, name, nullptr, &type, reinterpret_cast<BYTE*>(&value), &size) ==
            ERROR_SUCCESS &&
        type == REG_DWORD) {
        return value;
    }
    return default_val;
}

HKL match_layout(ULONG_PTR saved, const std::vector<LayoutChoice>& layouts) noexcept {
    if (!saved) return nullptr;
    for (const auto& layout : layouts) {
        const ULONG_PTR raw = reinterpret_cast<ULONG_PTR>(layout.handle);
        if (raw == saved || static_cast<DWORD>(raw) == static_cast<DWORD>(saved))
            return layout.handle;
    }
    return nullptr;
}

std::wstring layout_name(HKL layout) {
    const LANGID language = LOWORD(reinterpret_cast<ULONG_PTR>(layout));
    wchar_t locale_name[LOCALE_NAME_MAX_LENGTH]{};
    wchar_t display_name[128]{};
    if (LCIDToLocaleName(MAKELCID(language, SORT_DEFAULT), locale_name,
                         LOCALE_NAME_MAX_LENGTH, 0) &&
        GetLocaleInfoEx(locale_name, LOCALE_SLOCALIZEDDISPLAYNAME, display_name,
                        static_cast<int>(std::size(display_name))))
        return display_name;
    return L"Keyboard layout";
}

}  // namespace

const wchar_t* Settings::trigger_name(TriggerKey key) noexcept {
    switch (key) {
        case TriggerKey::CtrlDoubleTap: return L"Двойной Ctrl";
        case TriggerKey::ShiftDoubleTap: return L"Двойной Shift";
        case TriggerKey::AltDoubleTap: return L"Двойной Alt";
        case TriggerKey::CapsLock: return L"Caps Lock";
        case TriggerKey::PauseBreak: return L"Pause / Break";
        default: return L"Неизвестно";
    }
}

const wchar_t* Settings::switch_name(SwitchKey key) noexcept {
    switch (key) {
        case SwitchKey::Off: return L"Выключено";
        case SwitchKey::CapsLock: return L"Caps Lock (RU ↔ EN)";
        case SwitchKey::ShiftDoubleTap: return L"Двойной Shift";
        case SwitchKey::CtrlDoubleTap: return L"Двойной Ctrl";
        case SwitchKey::AltDoubleTap: return L"Двойной Alt";
        case SwitchKey::PauseBreak: return L"Pause / Break";
        default: return L"Выключено";
    }
}

const wchar_t* Settings::case_name(CaseKey key) noexcept {
    switch (key) {
        case CaseKey::Off: return L"Выключено";
        case CaseKey::PauseBreak: return L"Pause / Break";
        case CaseKey::ShiftDoubleTap: return L"Двойной Shift";
        case CaseKey::CtrlDoubleTap: return L"Двойной Ctrl";
        case CaseKey::AltDoubleTap: return L"Двойной Alt";
        default: return L"Выключено";
    }
}

Settings::Settings() noexcept {
    const int count = GetKeyboardLayoutList(0, nullptr);
    if (count > 0) {
        std::vector<HKL> handles(static_cast<std::size_t>(count));
        const int actual = GetKeyboardLayoutList(count, handles.data());
        for (int index = 0; index < actual; ++index)
            layouts_.push_back({handles[static_cast<std::size_t>(index)],
                                layout_name(handles[static_cast<std::size_t>(index)])});
    }

    HKEY key = open_settings(KEY_QUERY_VALUE);
    ULONG_PTR saved_first{};
    ULONG_PTR saved_second{};
    if (key) {
        enabled_ = read_bool(key, L"Enabled", true);
        trigger_ = static_cast<TriggerKey>(read_dword(key, L"Trigger", static_cast<DWORD>(TriggerKey::CtrlDoubleTap)));
        switch_hotkey_ = static_cast<SwitchKey>(read_dword(key, L"SwitchHotkey", static_cast<DWORD>(SwitchKey::CapsLock)));
        case_hotkey_ = static_cast<CaseKey>(read_dword(key, L"CaseHotkey", static_cast<DWORD>(CaseKey::Off)));
        fix_two_caps_ = read_bool(key, L"FixTwoCaps", true);
        fix_numbers_ = read_bool(key, L"FixNumbers", true);
        auto_convert_ = read_bool(key, L"AutoConvert", false);
        convert_whole_line_ = read_bool(key, L"ConvertWholeLine", false);
        sound_on_switch_ = read_bool(key, L"SoundOnSwitch", false);
        word_end_enter_tab_ = read_bool(key, L"WordEndEnterTab", true);
        per_app_layout_ = read_bool(key, L"PerAppLayout", true);
        saved_first = read_layout(key, L"FirstLayout");
        saved_second = read_layout(key, L"SecondLayout");
        RegCloseKey(key);
    }

    first_layout_ = match_layout(saved_first, layouts_);
    second_layout_ = match_layout(saved_second, layouts_);
    if (!first_layout_) {
        for (const auto& layout : layouts_)
            if (PRIMARYLANGID(LOWORD(reinterpret_cast<ULONG_PTR>(layout.handle))) == LANG_ENGLISH) {
                first_layout_ = layout.handle;
                break;
            }
    }
    if (!first_layout_ && !layouts_.empty()) first_layout_ = layouts_[0].handle;
    if (!second_layout_) {
        for (const auto& layout : layouts_)
            if (layout.handle != first_layout_ &&
                PRIMARYLANGID(LOWORD(reinterpret_cast<ULONG_PTR>(layout.handle))) == LANG_RUSSIAN) {
                second_layout_ = layout.handle;
                break;
            }
    }
    if (!second_layout_)
        for (const auto& layout : layouts_)
            if (layout.handle != first_layout_) {
                second_layout_ = layout.handle;
                break;
            }
}

void Settings::save_bool(const wchar_t* name, bool value) noexcept {
    HKEY key = open_settings(KEY_SET_VALUE);
    if (!key) return;
    const DWORD stored = value ? 1 : 0;
    RegSetValueExW(key, name, 0, REG_DWORD, reinterpret_cast<const BYTE*>(&stored), sizeof(stored));
    RegCloseKey(key);
}

void Settings::save_dword(const wchar_t* name, DWORD value) noexcept {
    HKEY key = open_settings(KEY_SET_VALUE);
    if (!key) return;
    RegSetValueExW(key, name, 0, REG_DWORD, reinterpret_cast<const BYTE*>(&value), sizeof(value));
    RegCloseKey(key);
}

void Settings::set_enabled(bool value) noexcept {
    enabled_ = value;
    save_bool(L"Enabled", value);
}

void Settings::set_trigger(TriggerKey value) noexcept {
    trigger_ = value;
    save_dword(L"Trigger", static_cast<DWORD>(value));
}

void Settings::set_switch_hotkey(SwitchKey value) noexcept {
    switch_hotkey_ = value;
    save_dword(L"SwitchHotkey", static_cast<DWORD>(value));
}

void Settings::set_case_hotkey(CaseKey value) noexcept {
    case_hotkey_ = value;
    save_dword(L"CaseHotkey", static_cast<DWORD>(value));
}

void Settings::set_fix_two_caps(bool value) noexcept {
    fix_two_caps_ = value;
    save_bool(L"FixTwoCaps", value);
}

void Settings::set_fix_numbers(bool value) noexcept {
    fix_numbers_ = value;
    save_bool(L"FixNumbers", value);
}

void Settings::set_auto_convert(bool value) noexcept {
    auto_convert_ = value;
    save_bool(L"AutoConvert", value);
}

void Settings::set_convert_whole_line(bool value) noexcept {
    convert_whole_line_ = value;
    save_bool(L"ConvertWholeLine", value);
}

void Settings::set_sound_on_switch(bool value) noexcept {
    sound_on_switch_ = value;
    save_bool(L"SoundOnSwitch", value);
}

void Settings::set_word_end_enter_tab(bool value) noexcept {
    word_end_enter_tab_ = value;
    save_bool(L"WordEndEnterTab", value);
}

void Settings::set_per_app_layout(bool value) noexcept {
    per_app_layout_ = value;
    save_bool(L"PerAppLayout", value);
}

void Settings::save_layout(const wchar_t* name, HKL value) noexcept {
    HKEY key = open_settings(KEY_SET_VALUE);
    if (!key) return;
    const ULONGLONG stored = reinterpret_cast<ULONG_PTR>(value);
    RegSetValueExW(key, name, 0, REG_QWORD, reinterpret_cast<const BYTE*>(&stored),
                   sizeof(stored));
    RegCloseKey(key);
}

void Settings::set_first_layout(HKL value) noexcept {
    if (!value || value == second_layout_) return;
    first_layout_ = value;
    save_layout(L"FirstLayout", value);
}

void Settings::set_second_layout(HKL value) noexcept {
    if (!value || value == first_layout_) return;
    second_layout_ = value;
    save_layout(L"SecondLayout", value);
}

bool Settings::autostart_enabled() const noexcept {
    HKEY key{};
    if (RegOpenKeyExW(HKEY_CURRENT_USER, kRunKey, 0, KEY_QUERY_VALUE, &key) != ERROR_SUCCESS)
        return false;
    DWORD size{};
    const bool exists = RegQueryValueExW(key, kRunValue, nullptr, nullptr, nullptr, &size) ==
                        ERROR_SUCCESS;
    RegCloseKey(key);
    return exists;
}

bool Settings::set_autostart(bool enabled) noexcept {
    HKEY key{};
    if (RegCreateKeyExW(HKEY_CURRENT_USER, kRunKey, 0, nullptr, 0, KEY_SET_VALUE, nullptr, &key,
                        nullptr) != ERROR_SUCCESS)
        return false;
    LONG result{};
    if (enabled) {
        wchar_t path[MAX_PATH]{};
        if (!GetModuleFileNameW(nullptr, path, MAX_PATH)) {
            RegCloseKey(key);
            return false;
        }
        std::wstring command = L"\"" + std::wstring(path) + L"\"";
        result = RegSetValueExW(key, kRunValue, 0, REG_SZ,
                                reinterpret_cast<const BYTE*>(command.c_str()),
                                static_cast<DWORD>((command.size() + 1) * sizeof(wchar_t)));
    } else {
        result = RegDeleteValueW(key, kRunValue);
        if (result == ERROR_FILE_NOT_FOUND) result = ERROR_SUCCESS;
    }
    RegCloseKey(key);
    return result == ERROR_SUCCESS;
}

}  // namespace ruswitcher

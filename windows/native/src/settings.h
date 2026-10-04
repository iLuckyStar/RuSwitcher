#pragma once

#include <windows.h>

#include <cstdint>
#include <string>
#include <vector>

namespace ruswitcher {

enum class TriggerKey : uint32_t {
    CtrlDoubleTap = 0,
    ShiftDoubleTap = 1,
    AltDoubleTap = 2,
    CapsLock = 3,
    PauseBreak = 4
};

enum class SwitchKey : uint32_t {
    Off = 0,
    CapsLock = 1,
    ShiftDoubleTap = 2,
    CtrlDoubleTap = 3,
    AltDoubleTap = 4,
    PauseBreak = 5
};

enum class CaseKey : uint32_t {
    Off = 0,
    PauseBreak = 1,
    ShiftDoubleTap = 2,
    CtrlDoubleTap = 3,
    AltDoubleTap = 4
};

struct LayoutChoice {
    HKL handle{};
    std::wstring name;
};

class Settings {
public:
    Settings() noexcept;

    bool enabled() const noexcept { return enabled_; }
    void set_enabled(bool value) noexcept;

    TriggerKey trigger() const noexcept { return trigger_; }
    void set_trigger(TriggerKey value) noexcept;

    SwitchKey switch_hotkey() const noexcept { return switch_hotkey_; }
    void set_switch_hotkey(SwitchKey value) noexcept;

    CaseKey case_hotkey() const noexcept { return case_hotkey_; }
    void set_case_hotkey(CaseKey value) noexcept;

    HKL first_layout() const noexcept { return first_layout_; }
    HKL second_layout() const noexcept { return second_layout_; }
    void set_first_layout(HKL value) noexcept;
    void set_second_layout(HKL value) noexcept;

    bool fix_two_caps() const noexcept { return fix_two_caps_; }
    void set_fix_two_caps(bool value) noexcept;

    bool fix_numbers() const noexcept { return fix_numbers_; }
    void set_fix_numbers(bool value) noexcept;

    bool auto_convert() const noexcept { return auto_convert_; }
    void set_auto_convert(bool value) noexcept;

    bool convert_whole_line() const noexcept { return convert_whole_line_; }
    void set_convert_whole_line(bool value) noexcept;

    bool sound_on_switch() const noexcept { return sound_on_switch_; }
    void set_sound_on_switch(bool value) noexcept;

    bool word_end_enter_tab() const noexcept { return word_end_enter_tab_; }
    void set_word_end_enter_tab(bool value) noexcept;

    const std::vector<LayoutChoice>& layouts() const noexcept { return layouts_; }

    bool autostart_enabled() const noexcept;
    bool set_autostart(bool enabled) noexcept;

    static const wchar_t* trigger_name(TriggerKey key) noexcept;
    static const wchar_t* switch_name(SwitchKey key) noexcept;
    static const wchar_t* case_name(CaseKey key) noexcept;

private:
    std::vector<LayoutChoice> layouts_;
    bool enabled_{true};
    TriggerKey trigger_{TriggerKey::CtrlDoubleTap};
    SwitchKey switch_hotkey_{SwitchKey::CapsLock};
    CaseKey case_hotkey_{CaseKey::Off};
    HKL first_layout_{};
    HKL second_layout_{};
    bool fix_two_caps_{true};
    bool fix_numbers_{true};
    bool auto_convert_{false};
    bool convert_whole_line_{false};
    bool sound_on_switch_{false};
    bool word_end_enter_tab_{true};

    void save_layout(const wchar_t* name, HKL value) noexcept;
    void save_bool(const wchar_t* name, bool value) noexcept;
    void save_dword(const wchar_t* name, DWORD value) noexcept;
};

}  // namespace ruswitcher

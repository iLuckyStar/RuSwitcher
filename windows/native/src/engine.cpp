#include "engine.h"

#include "brand_words.h"
#include "clipboard.h"
#include "diagnostics.h"
#include "dict.h"
#include "input_safety.h"
#include "settings.h"
#include "text_fixes.h"

#include <array>
#include <cstdint>
#include <cwctype>
#include <string>
#include <utility>
#include <vector>

namespace ruswitcher {
namespace {

constexpr ULONG_PTR kInjectedMarker = 0x52555357;
constexpr UINT kTriggerMessage = WM_APP + 1;
constexpr UINT kBoundaryMessage = WM_APP + 3;
constexpr ULONGLONG kDoubleTapWindowMs = 350;

bool is_control(DWORD vk) noexcept {
    return vk == VK_CONTROL || vk == VK_LCONTROL || vk == VK_RCONTROL;
}

bool is_typing_key(DWORD vk) noexcept {
    return (vk >= '0' && vk <= '9') || (vk >= 'A' && vk <= 'Z') ||
           (vk >= VK_OEM_1 && vk <= VK_OEM_3) ||
           (vk >= VK_OEM_4 && vk <= VK_OEM_8) || vk == VK_OEM_102;
}

bool is_boundary(DWORD vk) noexcept {
    return vk == VK_SPACE || vk == VK_RETURN || vk == VK_TAB || vk == VK_ESCAPE;
}

bool invalidates_buffer(DWORD vk) noexcept {
    switch (vk) {
        case VK_DELETE:
        case VK_INSERT:
        case VK_HOME:
        case VK_END:
        case VK_LEFT:
        case VK_UP:
        case VK_RIGHT:
        case VK_DOWN:
        case VK_PRIOR:
        case VK_NEXT:
            return true;
        default:
            return false;
    }
}

bool shortcut_modifier_down() noexcept {
    return (GetAsyncKeyState(VK_CONTROL) & 0x8000) != 0 ||
           (GetAsyncKeyState(VK_MENU) & 0x8000) != 0 ||
           (GetAsyncKeyState(VK_LWIN) & 0x8000) != 0 ||
           (GetAsyncKeyState(VK_RWIN) & 0x8000) != 0;
}

bool useful_foreground(HWND window, HWND own_window) noexcept {
    if (!window || window == own_window || !IsWindow(window)) return false;
    DWORD process{};
    GetWindowThreadProcessId(window, &process);
    if (process == GetCurrentProcessId()) return false;
    wchar_t class_name[64]{};
    GetClassNameW(window, class_name, static_cast<int>(std::size(class_name)));
    return lstrcmpW(class_name, L"Shell_TrayWnd") != 0 &&
           lstrcmpW(class_name, L"Shell_SecondaryTrayWnd") != 0 &&
           lstrcmpW(class_name, L"Progman") != 0 && lstrcmpW(class_name, L"WorkerW") != 0 &&
           lstrcmpW(class_name, L"#32768") != 0;
}

HKL current_layout() noexcept {
    const HWND foreground = GetForegroundWindow();
    const DWORD thread = GetWindowThreadProcessId(foreground, nullptr);
    return GetKeyboardLayout(thread);
}

HKL first_other_layout(HKL current) noexcept {
    const int count = GetKeyboardLayoutList(0, nullptr);
    if (count <= 1) return nullptr;

    std::vector<HKL> layouts(static_cast<std::size_t>(count));
    const int copied = GetKeyboardLayoutList(count, layouts.data());
    for (int index = 0; index < copied; ++index) {
        if (layouts[static_cast<std::size_t>(index)] != current)
            return layouts[static_cast<std::size_t>(index)];
    }
    return nullptr;
}

bool translate_key(const TypedKey& key, HKL layout, wchar_t& character) noexcept {
    std::array<BYTE, 256> state{};
    if (key.shift) state[VK_SHIFT] = 0x80;
    if (key.caps) state[VK_CAPITAL] = 0x01;

    wchar_t output[8]{};
    const int length = ToUnicodeEx(key.vk, key.scan, state.data(), output,
                                   static_cast<int>(std::size(output)), 0x4, layout);
    if (length < 1) return false;
    character = output[0];
    return true;
}

bool translate_keys(const std::vector<TypedKey>& keys, std::size_t count, HKL layout,
                    std::wstring& text) {
    text.clear();
    text.reserve(count);
    for (std::size_t index = 0; index < count && index < keys.size(); ++index) {
        wchar_t character{};
        if (!translate_key(keys[index], layout, character)) return false;
        text.push_back(character);
    }
    return true;
}

bool is_letter(wchar_t character) noexcept {
    WORD type{};
    return GetStringTypeW(CT_CTYPE1, &character, 1, &type) && (type & C1_ALPHA) != 0;
}

void add_text_pair(std::vector<std::pair<wchar_t, wchar_t>>& pairs, DWORD vk, bool shift,
                   HKL source, HKL target) {
    const DWORD scan = MapVirtualKeyExW(vk, MAPVK_VK_TO_VSC, source);
    const TypedKey key{vk, scan, shift, false};
    wchar_t from{};
    wchar_t to{};
    if (!translate_key(key, source, from) || !translate_key(key, target, to) || from == to ||
        !is_letter(from))
        return;
    for (const auto& pair : pairs)
        if (pair.first == from) return;
    pairs.emplace_back(from, to);
}

std::wstring convert_text(const std::wstring& text, HKL source, HKL target) {
    std::vector<std::pair<wchar_t, wchar_t>> pairs;
    pairs.reserve(96);
    for (DWORD vk = '0'; vk <= '9'; ++vk) {
        add_text_pair(pairs, vk, false, source, target);
        add_text_pair(pairs, vk, true, source, target);
    }
    for (DWORD vk = 'A'; vk <= 'Z'; ++vk) {
        add_text_pair(pairs, vk, false, source, target);
        add_text_pair(pairs, vk, true, source, target);
    }
    constexpr DWORD oem_keys[]{VK_OEM_1, VK_OEM_PLUS,  VK_OEM_COMMA, VK_OEM_MINUS,
                               VK_OEM_PERIOD, VK_OEM_2, VK_OEM_3,     VK_OEM_4,
                               VK_OEM_5,      VK_OEM_6, VK_OEM_7,     VK_OEM_8,
                               VK_OEM_102};
    for (const DWORD vk : oem_keys) {
        add_text_pair(pairs, vk, false, source, target);
        add_text_pair(pairs, vk, true, source, target);
    }

    std::wstring result = text;
    for (auto& character : result) {
        for (const auto& pair : pairs) {
            if (character == pair.first) {
                character = pair.second;
                break;
            }
        }
    }
    return result;
}

bool is_trailing_punctuation(wchar_t character) noexcept {
    switch (character) {
        case L',':
        case L'.':
        case L'!':
        case L'?':
        case L';':
        case L':':
        case L')':
            return true;
        default:
            return false;
    }
}

INPUT key_input(WORD vk, WORD scan, DWORD flags) noexcept {
    INPUT input{};
    input.type = INPUT_KEYBOARD;
    input.ki.wVk = vk;
    input.ki.wScan = scan;
    input.ki.dwFlags = flags;
    input.ki.dwExtraInfo = kInjectedMarker;
    return input;
}

bool send_line_selection() noexcept {
    INPUT inputs[]{key_input(VK_HOME, 0, 0),
                   key_input(VK_HOME, 0, KEYEVENTF_KEYUP),
                   key_input(VK_SHIFT, 0, 0),
                   key_input(VK_END, 0, 0),
                   key_input(VK_END, 0, KEYEVENTF_KEYUP),
                   key_input(VK_SHIFT, 0, KEYEVENTF_KEYUP)};
    return SendInput(static_cast<UINT>(std::size(inputs)), inputs, sizeof(INPUT)) ==
           std::size(inputs);
}

bool replace_text(std::size_t characters_to_delete, const std::wstring& replacement) {
    std::vector<INPUT> inputs;
    inputs.reserve((characters_to_delete + replacement.size()) * 2);

    for (std::size_t index = 0; index < characters_to_delete; ++index) {
        inputs.push_back(key_input(VK_BACK, 0, 0));
        inputs.push_back(key_input(VK_BACK, 0, KEYEVENTF_KEYUP));
    }
    for (std::size_t index = 0; index < replacement.size(); ++index) {
        const wchar_t character = replacement[index];
        if (character == L'\r' || character == L'\n') {
            if (character == L'\r' && index + 1 < replacement.size() &&
                replacement[index + 1] == L'\n')
                ++index;
            inputs.push_back(key_input(VK_RETURN, 0, 0));
            inputs.push_back(key_input(VK_RETURN, 0, KEYEVENTF_KEYUP));
            continue;
        }
        if (character == L'\t') {
            inputs.push_back(key_input(VK_TAB, 0, 0));
            inputs.push_back(key_input(VK_TAB, 0, KEYEVENTF_KEYUP));
            continue;
        }
        inputs.push_back(key_input(0, static_cast<WORD>(character), KEYEVENTF_UNICODE));
        inputs.push_back(
            key_input(0, static_cast<WORD>(character), KEYEVENTF_UNICODE | KEYEVENTF_KEYUP));
    }

    if (inputs.empty()) return false;
    const UINT sent = SendInput(static_cast<UINT>(inputs.size()), inputs.data(), sizeof(INPUT));
    return sent == inputs.size();
}

std::size_t editing_length(const std::wstring& text) noexcept {
    std::size_t length{};
    for (std::size_t index = 0; index < text.size(); ++index) {
        if (text[index] == L'\r' && index + 1 < text.size() && text[index + 1] == L'\n')
            ++index;
        ++length;
    }
    return length;
}

void switch_layout(HKL layout) noexcept {
    const HWND foreground = GetForegroundWindow();
    if (foreground)
        PostMessageW(foreground, WM_INPUTLANGCHANGEREQUEST, 0,
                     reinterpret_cast<LPARAM>(layout));
}

bool is_all_caps(std::wstring_view text) noexcept {
    bool has_letter = false;
    for (wchar_t c : text) {
        if (iswalpha(c)) {
            has_letter = true;
            if (!iswupper(c)) return false;
        }
    }
    return has_letter;
}

bool looks_like_code(std::wstring_view text) noexcept {
    // CamelCase check (capital after first letter)
    for (std::size_t i = 1; i < text.size(); ++i) {
        if (iswupper(text[i])) return true;
    }
    // Mixed Latin and Cyrillic script check
    bool latin = false;
    bool cyrillic = false;
    for (wchar_t c : text) {
        if ((c >= L'a' && c <= L'z') || (c >= L'A' && c <= L'Z')) {
            latin = true;
        } else if (c >= 0x0400 && c <= 0x04FF) {
            cyrillic = true;
        }
    }
    return latin && cyrillic;
}

bool should_auto_convert(std::wstring_view typed, std::wstring_view converted,
                         HKL source, HKL target, bool caps) noexcept {
    if (typed.size() < 3) return false;

    // Must be letters or apostrophe
    for (wchar_t c : typed) {
        if (!iswalpha(c) && c != L'\'' && c != L'\x2019') return false;
    }

    if (!caps) {
        if (is_all_caps(typed)) return false;
        if (looks_like_code(typed)) return false;
    }

    std::wstring lower_converted;
    lower_converted.reserve(converted.size());
    for (wchar_t c : converted) lower_converted.push_back(static_cast<wchar_t>(towlower(c)));

    const bool target_is_brand = (converted.size() >= 4 && is_brand_word(lower_converted));
    const bool valid_target = target_is_brand || Dict::is_valid_word(lower_converted, target);
    if (!valid_target) return false;

    std::wstring lower_typed;
    lower_typed.reserve(typed.size());
    for (wchar_t c : typed) lower_typed.push_back(static_cast<wchar_t>(towlower(c)));

    // If source word is already valid in current layout, do not convert
    if (Dict::is_valid_word(lower_typed, source)) return false;

    return true;
}

}  // namespace

struct Engine::Impl {
    HWND message_window{};
    Settings* settings{};
    HHOOK keyboard_hook{};
    HHOOK mouse_hook{};
    HWINEVENTHOOK foreground_hook{};
    HWINEVENTHOOK focus_hook{};
    std::vector<TypedKey> word;
    HWND word_owner{};
    HWND last_foreground{};
    bool enabled{true};
    HKL first_layout{};
    HKL second_layout{};

    bool control_down{};
    bool other_during_control{};
    ULONGLONG last_control_tap{};

    bool undo_available{};
    std::wstring screen_text;
    std::wstring alternate_text;
    HKL screen_layout{};
    HKL alternate_layout{};

    std::vector<TypedKey> pending_boundary_word;
    DWORD pending_boundary_vk{};
    HWND pending_boundary_owner{};

    static Impl* instance;

    explicit Impl(HWND window, Settings* prefs) noexcept
        : message_window(window), settings(prefs) {}

    HKL target_layout(HKL current) const noexcept {
        if (first_layout && second_layout) {
            if (current == first_layout) return second_layout;
            if (current == second_layout) return first_layout;
        }
        return first_other_layout(current);
    }

    void clear_word() noexcept {
        word.clear();
        word_owner = nullptr;
    }

    void clear_all() noexcept {
        clear_word();
        undo_available = false;
        screen_text.clear();
        alternate_text.clear();
        pending_boundary_word.clear();
        pending_boundary_vk = 0;
        pending_boundary_owner = nullptr;
    }

    void on_key_down(DWORD vk, DWORD scan) {
        if (!enabled) return;
        if (is_control(vk)) {
            if (!control_down) {
                control_down = true;
                other_during_control = false;
            }
            return;
        }

        if (control_down) other_during_control = true;
        last_control_tap = 0;

        if (vk == VK_BACK) {
            undo_available = false;
            if (shortcut_modifier_down()) {
                clear_word();
            } else if (!word.empty()) {
                word.pop_back();
                if (word.empty()) word_owner = nullptr;
            }
            return;
        }

        if (is_boundary(vk)) {
            if (vk == VK_SPACE || vk == VK_RETURN || vk == VK_TAB) {
                const HWND foreground = GetForegroundWindow();
                if (!word.empty() && word_owner && word_owner == foreground) {
                    pending_boundary_word = word;
                    pending_boundary_vk = vk;
                    pending_boundary_owner = word_owner;
                    PostMessageW(message_window, kBoundaryMessage, 0, 0);
                }
            }
            clear_word();
            undo_available = false;
            return;
        }

        if (is_typing_key(vk)) {
            if (shortcut_modifier_down()) {
                clear_all();
                return;
            }
            const HWND foreground = GetForegroundWindow();
            if (word_owner && foreground != word_owner) clear_word();
            if (word.empty()) word_owner = foreground;

            const bool shift = (GetAsyncKeyState(VK_SHIFT) & 0x8000) != 0;
            const bool caps = (GetKeyState(VK_CAPITAL) & 0x0001) != 0;
            word.push_back(TypedKey{vk, scan, shift, caps});
            undo_available = false;
        } else if (invalidates_buffer(vk)) {
            clear_all();
        }
    }

    void on_key_up(DWORD vk) noexcept {
        if (!enabled) return;
        if (!is_control(vk)) return;
        const bool tap = control_down && !other_during_control;
        control_down = false;
        other_during_control = false;
        if (!tap) return;

        const ULONGLONG now = GetTickCount64();
        if (last_control_tap && now - last_control_tap <= kDoubleTapWindowMs) {
            last_control_tap = 0;
            PostMessageW(message_window, kTriggerMessage, 0, 0);
        } else {
            last_control_tap = now;
        }
    }

    static LRESULT CALLBACK keyboard_proc(int code, WPARAM message, LPARAM data) noexcept {
        if (code == HC_ACTION && instance) {
            const auto* key = reinterpret_cast<const KBDLLHOOKSTRUCT*>(data);
            if (key->dwExtraInfo != kInjectedMarker) {
                if (message == WM_KEYDOWN || message == WM_SYSKEYDOWN)
                    instance->on_key_down(key->vkCode, key->scanCode);
                else if (message == WM_KEYUP || message == WM_SYSKEYUP)
                    instance->on_key_up(key->vkCode);
            }
        }
        return CallNextHookEx(instance ? instance->keyboard_hook : nullptr, code, message, data);
    }

    static LRESULT CALLBACK mouse_proc(int code, WPARAM message, LPARAM data) noexcept {
        if (code == HC_ACTION && instance &&
            (message == WM_LBUTTONDOWN || message == WM_RBUTTONDOWN ||
             message == WM_MBUTTONDOWN)) {
            instance->clear_all();
        }
        return CallNextHookEx(instance ? instance->mouse_hook : nullptr, code, message, data);
    }

    static void CALLBACK window_event_proc(HWINEVENTHOOK, DWORD event, HWND window, LONG,
                                           LONG, DWORD, DWORD) noexcept {
        if (!instance) return;
        if (event == EVENT_SYSTEM_FOREGROUND) {
            if (useful_foreground(window, instance->message_window))
                instance->last_foreground = window;
            instance->clear_all();
            return;
        }

        if (event == EVENT_OBJECT_FOCUS && window) {
            const HWND root = GetAncestor(window, GA_ROOT);
            if (root && root == GetForegroundWindow()) instance->clear_all();
        }
    }

    bool install() noexcept {
        instance = this;
        const HWND foreground = GetForegroundWindow();
        if (useful_foreground(foreground, message_window)) last_foreground = foreground;
        const HINSTANCE module = GetModuleHandleW(nullptr);
        keyboard_hook = SetWindowsHookExW(WH_KEYBOARD_LL, keyboard_proc, module, 0);
        if (!keyboard_hook) return false;
        mouse_hook = SetWindowsHookExW(WH_MOUSE_LL, mouse_proc, module, 0);
        if (!mouse_hook) {
            UnhookWindowsHookEx(keyboard_hook);
            keyboard_hook = nullptr;
            return false;
        }
        foreground_hook = SetWinEventHook(EVENT_SYSTEM_FOREGROUND, EVENT_SYSTEM_FOREGROUND,
                                          nullptr, window_event_proc, 0, 0,
                                          WINEVENT_OUTOFCONTEXT | WINEVENT_SKIPOWNPROCESS);
        focus_hook = SetWinEventHook(EVENT_OBJECT_FOCUS, EVENT_OBJECT_FOCUS, nullptr,
                                     window_event_proc, 0, 0,
                                     WINEVENT_OUTOFCONTEXT | WINEVENT_SKIPOWNPROCESS);
        if (!foreground_hook || !focus_hook) {
            if (focus_hook) UnhookWinEvent(focus_hook);
            if (foreground_hook) UnhookWinEvent(foreground_hook);
            UnhookWindowsHookEx(mouse_hook);
            UnhookWindowsHookEx(keyboard_hook);
            focus_hook = nullptr;
            foreground_hook = nullptr;
            mouse_hook = nullptr;
            keyboard_hook = nullptr;
            return false;
        }
        return true;
    }

    bool convert_selection(bool permit_undo) noexcept {
        const HKL source = current_layout();
        const HKL target = target_layout(source);
        if (!source || !target) return false;

        ClipboardSnapshot clipboard;
        if (!clipboard.capture()) {
            log_event(L"selection failed: snapshot");
            return false;
        }
        std::wstring selected;
        if (!copy_current_selection(selected)) {
            log_event(L"selection failed: copy");
            clipboard.restore();
            return false;
        }
        std::wstring converted = convert_text(selected, source, target);
        if (converted == selected) {
            log_event(L"selection failed: mapping no-op");
            clipboard.restore();
            return false;
        }
        if (!clipboard.restore()) {
            log_event(L"selection failed: restore");
            return false;
        }
        if (!replace_text(0, converted)) {
            log_event(L"selection failed: inject");
            return false;
        }
        switch_layout(target);

        if (permit_undo) {
            screen_text = std::move(converted);
            alternate_text = std::move(selected);
            screen_layout = target;
            alternate_layout = source;
            undo_available = true;
        }
        return true;
    }

    void on_boundary_triggered() noexcept {
        if (!enabled) return;
        if (pending_boundary_word.empty()) return;

        const HWND foreground = GetForegroundWindow();
        if (!foreground || foreground != pending_boundary_owner || is_protected_foreground()) {
            pending_boundary_word.clear();
            pending_boundary_owner = nullptr;
            return;
        }

        const std::vector<TypedKey> keys = std::move(pending_boundary_word);
        const DWORD b_vk = pending_boundary_vk;
        pending_boundary_word.clear();
        pending_boundary_vk = 0;
        pending_boundary_owner = nullptr;

        const HKL source = current_layout();
        const HKL target = target_layout(source);
        if (!source || !target) return;

        std::wstring typed;
        if (!translate_keys(keys, keys.size(), source, typed) || typed.empty()) return;

        std::wstring boundary_str;
        if (b_vk == VK_RETURN) boundary_str = L"\r\n";
        else if (b_vk == VK_TAB) boundary_str = L"\t";
        else boundary_str = L" ";

        // 1. TextFixes: Two initial capitals (e.g. "ПРивет" -> "Привет", "TWo" -> "Two")
        if (settings && settings->fix_two_caps()) {
            auto two_caps = fix_two_caps(typed, source, true);
            if (two_caps && *two_caps != typed) {
                if (replace_text(keys.size() + 1, *two_caps + boundary_str)) {
                    screen_text = *two_caps + boundary_str;
                    alternate_text = typed + boundary_str;
                    screen_layout = source;
                    alternate_layout = source;
                    undo_available = true;
                    return;
                }
            }
        }

        // 2. TextFixes: Misplaced dot/comma in numbers (e.g. "1ю8" -> "1.8", "5б2" -> "5,2")
        if (settings && settings->fix_numbers()) {
            auto num = fix_number(keys, typed, target);
            if (num && *num != typed) {
                if (replace_text(keys.size() + 1, *num + boundary_str)) {
                    screen_text = *num + boundary_str;
                    alternate_text = typed + boundary_str;
                    screen_layout = source;
                    alternate_layout = source;
                    undo_available = true;
                    return;
                }
            }
        }

        // 3. Layout Auto-Conversion (Space / Enter / Tab)
        if (settings && settings->auto_convert()) {
            std::wstring converted;
            if (translate_keys(keys, keys.size(), target, converted) && !converted.empty() && converted != typed) {
                bool caps = true;
                for (const auto& k : keys) {
                    if (!k.caps) { caps = false; break; }
                }
                if (should_auto_convert(typed, converted, source, target, caps)) {
                    std::wstring final_converted = converted;
                    if (settings->fix_two_caps()) {
                        auto fixed_target = fix_two_caps(converted, target, true);
                        if (fixed_target) final_converted = *fixed_target;
                    }

                    if (replace_text(keys.size() + 1, final_converted + boundary_str)) {
                        switch_layout(target);
                        screen_text = final_converted + boundary_str;
                        alternate_text = typed + boundary_str;
                        screen_layout = target;
                        alternate_layout = source;
                        undo_available = true;
                        return;
                    }
                }
            }
        }
    }

    void convert_or_undo() noexcept {
        if (!enabled) return;
        if (is_protected_foreground()) {
            log_event(L"trigger blocked: protected field");
            clear_all();
            return;
        }

        if (undo_available) {
            if (replace_text(editing_length(screen_text), alternate_text)) {
                switch_layout(alternate_layout);
                std::swap(screen_text, alternate_text);
                std::swap(screen_layout, alternate_layout);
            }
            return;
        }

        if (word.empty() || !word_owner || word_owner != GetForegroundWindow()) {
            clear_word();
            convert_selection(true);
            return;
        }

        const HKL source = current_layout();
        const HKL target = target_layout(source);
        if (!source || !target) return;

        std::size_t core_count = word.size();
        std::wstring suffix;
        while (core_count > 0) {
            wchar_t source_character{};
            if (!translate_key(word[core_count - 1], source, source_character) ||
                !is_trailing_punctuation(source_character))
                break;
            suffix.insert(suffix.begin(), source_character);
            --core_count;
        }
        if (core_count == 0) return;

        std::wstring original;
        std::wstring converted;
        if (!translate_keys(word, core_count, source, original) ||
            !translate_keys(word, core_count, target, converted) || converted.empty())
            return;
        original += suffix;
        converted += suffix;
        if (original == converted) return;

        if (!replace_text(word.size(), converted)) return;
        switch_layout(target);

        screen_text = std::move(converted);
        alternate_text = std::move(original);
        screen_layout = target;
        alternate_layout = source;
        undo_available = true;
        clear_word();
    }

    void convert_line() noexcept {
        if (!enabled || is_protected_foreground()) return;
        clear_all();
        if (last_foreground && IsWindow(last_foreground)) {
            SetForegroundWindow(last_foreground);
            Sleep(80);
        }
        if (!send_line_selection()) return;
        Sleep(90);
        convert_selection(true);
    }

    void change_case() noexcept {
        if (!enabled || is_protected_foreground()) return;

        const HWND foreground = GetForegroundWindow();
        if (!word.empty() && word_owner && word_owner == foreground) {
            const HKL layout = current_layout();
            std::wstring original;
            if (translate_keys(word, word.size(), layout, original) && !original.empty()) {
                std::wstring next = next_case(original);
                if (next != original) {
                    if (replace_text(word.size(), next)) {
                        clear_all();
                        return;
                    }
                }
            }
        }

        ClipboardSnapshot clipboard;
        if (!clipboard.capture()) return;

        std::wstring selected;
        if (!copy_current_selection(selected) || selected.empty()) {
            clipboard.restore();
            return;
        }

        std::wstring next = next_case(selected);
        if (next == selected) {
            clipboard.restore();
            return;
        }

        if (!clipboard.restore()) return;
        replace_text(0, next);
        clear_all();
    }

    ~Impl() {
        if (focus_hook) UnhookWinEvent(focus_hook);
        if (foreground_hook) UnhookWinEvent(foreground_hook);
        if (mouse_hook) UnhookWindowsHookEx(mouse_hook);
        if (keyboard_hook) UnhookWindowsHookEx(keyboard_hook);
        if (instance == this) instance = nullptr;
    }
};

Engine::Impl* Engine::Impl::instance = nullptr;

Engine::Engine(HWND message_window, Settings* settings) noexcept
    : impl_(new Impl(message_window, settings)) {}
Engine::~Engine() { delete impl_; }
bool Engine::install() noexcept { return impl_->install(); }
void Engine::convert_or_undo() noexcept { impl_->convert_or_undo(); }
void Engine::convert_line() noexcept { impl_->convert_line(); }
void Engine::change_case() noexcept { impl_->change_case(); }
void Engine::on_boundary_triggered() noexcept { impl_->on_boundary_triggered(); }
void Engine::set_enabled(bool enabled) noexcept {
    impl_->enabled = enabled;
    if (!enabled) impl_->clear_all();
}
bool Engine::enabled() const noexcept { return impl_->enabled; }
void Engine::set_layout_pair(HKL first, HKL second) noexcept {
    impl_->first_layout = first;
    impl_->second_layout = second;
    impl_->clear_all();
}
void Engine::remember_foreground() noexcept {
    const HWND current = GetForegroundWindow();
    if (useful_foreground(current, impl_->message_window)) impl_->last_foreground = current;
}

}  // namespace ruswitcher

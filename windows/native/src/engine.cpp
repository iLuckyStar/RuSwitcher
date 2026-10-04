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
constexpr UINT kSwitchMessage = WM_APP + 4;
constexpr UINT kCaseMessage = WM_APP + 5;
constexpr ULONGLONG kDoubleTapWindowMs = 350;

bool is_control(DWORD vk) noexcept {
    return vk == VK_CONTROL || vk == VK_LCONTROL || vk == VK_RCONTROL;
}

bool is_shift(DWORD vk) noexcept {
    return vk == VK_SHIFT || vk == VK_LSHIFT || vk == VK_RSHIFT;
}

bool is_alt(DWORD vk) noexcept {
    return vk == VK_MENU || vk == VK_LMENU || vk == VK_RMENU;
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

bool is_shell_window(HWND window) noexcept {
    if (!window || !IsWindow(window)) return true;
    wchar_t class_name[64]{};
    GetClassNameW(window, class_name, static_cast<int>(std::size(class_name)));
    if (lstrcmpW(class_name, L"Shell_TrayWnd") == 0 ||
        lstrcmpW(class_name, L"Shell_SecondaryTrayWnd") == 0 ||
        lstrcmpW(class_name, L"Progman") == 0 ||
        lstrcmpW(class_name, L"WorkerW") == 0 ||
        lstrcmpW(class_name, L"TrayNotifyWnd") == 0 ||
        lstrcmpW(class_name, L"NotifyIconOverflowWindow") == 0 ||
        lstrcmpW(class_name, L"TopLevelWindowForOverflowXamlIsland") == 0 ||
        lstrcmpW(class_name, L"Windows.UI.Core.CoreWindow") == 0 ||
        lstrcmpW(class_name, L"XamlExplorerHostIslandWindow") == 0 ||
        lstrcmpW(class_name, L"#32768") == 0) {
        return true;
    }
    const LONG ex_style = GetWindowLongW(window, GWL_EXSTYLE);
    if ((ex_style & WS_EX_TOOLWINDOW) != 0 && (ex_style & WS_EX_APPWINDOW) == 0) {
        return true;
    }
    return false;
}

bool useful_foreground(HWND window, HWND own_window) noexcept {
    if (!window || window == own_window || !IsWindow(window)) return false;
    DWORD process{};
    GetWindowThreadProcessId(window, &process);
    if (process == GetCurrentProcessId()) return false;
    return !is_shell_window(window);
}

HWND get_previous_active_window(HWND own_window) noexcept {
    HWND hwnd = GetTopWindow(GetDesktopWindow());
    const DWORD own_pid = GetCurrentProcessId();
    while (hwnd) {
        if (hwnd != own_window && IsWindow(hwnd) && IsWindowVisible(hwnd) && !IsIconic(hwnd)) {
            DWORD pid{};
            GetWindowThreadProcessId(hwnd, &pid);
            if (pid != own_pid && !is_shell_window(hwnd)) {
                const int title_len = GetWindowTextLengthW(hwnd);
                const LONG style = GetWindowLongW(hwnd, GWL_STYLE);
                if (title_len > 0 && (style & WS_VISIBLE) != 0) {
                    return hwnd;
                }
            }
        }
        hwnd = GetWindow(hwnd, GW_HWNDNEXT);
    }
    return nullptr;
}

void restore_target_focus(HWND target, HWND own_window = nullptr) noexcept {
    if (!target || !IsWindow(target) || is_shell_window(target)) {
        target = get_previous_active_window(own_window);
        if (!target || !IsWindow(target)) return;
    }

    HWND root = GetAncestor(target, GA_ROOT);
    if (!root) root = target;

    const HWND current_fore = GetForegroundWindow();
    if (current_fore == root || current_fore == target) return;

    LockSetForegroundWindow(LSFW_UNLOCK);
    AllowSetForegroundWindow(ASFW_ANY);

    // Neutral Alt key tap to grant SetForegroundWindow permission
    keybd_event(VK_MENU, 0, 0, 0);
    keybd_event(VK_MENU, 0, KEYEVENTF_KEYUP, 0);

    const DWORD current_tid = GetCurrentThreadId();
    const DWORD target_tid = GetWindowThreadProcessId(root, nullptr);
    const DWORD fore_tid = current_fore ? GetWindowThreadProcessId(current_fore, nullptr) : 0;

    if (fore_tid && fore_tid != current_tid) {
        AttachThreadInput(current_tid, fore_tid, TRUE);
    }
    if (target_tid && target_tid != current_tid) {
        AttachThreadInput(current_tid, target_tid, TRUE);
    }

    if (IsIconic(root)) {
        ShowWindow(root, SW_RESTORE);
    } else {
        ShowWindow(root, SW_SHOW);
    }

    BringWindowToTop(root);
    SetForegroundWindow(root);
    SetFocus(target);

    if (fore_tid && fore_tid != current_tid) {
        AttachThreadInput(current_tid, fore_tid, FALSE);
    }
    if (target_tid && target_tid != current_tid) {
        AttachThreadInput(current_tid, target_tid, FALSE);
    }

    for (int i = 0; i < 30; ++i) {
        HWND active = GetForegroundWindow();
        if (active == root || active == target) break;
        Sleep(10);
    }
    Sleep(50);
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
    return convert_text_bidirectional(text, source, target);
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
    INPUT to_start[]{key_input(VK_HOME, 0, 0), key_input(VK_HOME, 0, KEYEVENTF_KEYUP)};
    SendInput(static_cast<UINT>(std::size(to_start)), to_start, sizeof(INPUT));
    Sleep(25);

    INPUT select_to_end[]{key_input(VK_SHIFT, 0, 0),
                          key_input(VK_END, 0, 0),
                          key_input(VK_END, 0, KEYEVENTF_KEYUP),
                          key_input(VK_SHIFT, 0, KEYEVENTF_KEYUP)};
    return SendInput(static_cast<UINT>(std::size(select_to_end)), select_to_end, sizeof(INPUT)) ==
           std::size(select_to_end);
}

bool send_word_selection(bool to_left) noexcept {
    const WORD dir = to_left ? VK_LEFT : VK_RIGHT;
    INPUT inputs[]{key_input(VK_CONTROL, 0, 0),
                   key_input(VK_SHIFT, 0, 0),
                   key_input(dir, 0, 0),
                   key_input(dir, 0, KEYEVENTF_KEYUP),
                   key_input(VK_SHIFT, 0, KEYEVENTF_KEYUP),
                   key_input(VK_CONTROL, 0, KEYEVENTF_KEYUP)};
    return SendInput(static_cast<UINT>(std::size(inputs)), inputs, sizeof(INPUT)) ==
           std::size(inputs);
}

bool has_letter(std::wstring_view text) noexcept {
    for (wchar_t c : text) {
        if (iswalpha(c)) return true;
    }
    return false;
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
    if (!layout) return;
    const HWND foreground = GetForegroundWindow();
    if (!foreground) return;

    const DWORD current_thread = GetCurrentThreadId();
    const DWORD target_thread = GetWindowThreadProcessId(foreground, nullptr);

    HWND target_wnd = foreground;
    GUITHREADINFO gui{sizeof(gui)};
    if (GetGUIThreadInfo(target_thread, &gui) && gui.hwndFocus) {
        target_wnd = gui.hwndFocus;
    }

    if (target_thread && target_thread != current_thread) {
        if (AttachThreadInput(current_thread, target_thread, TRUE)) {
            ActivateKeyboardLayout(layout, KLF_SETFORPROCESS);
            AttachThreadInput(current_thread, target_thread, FALSE);
        }
    } else {
        ActivateKeyboardLayout(layout, KLF_SETFORPROCESS);
    }

    PostMessageW(target_wnd, WM_INPUTLANGCHANGEREQUEST, 0, reinterpret_cast<LPARAM>(layout));
    if (target_wnd != foreground) {
        PostMessageW(foreground, WM_INPUTLANGCHANGEREQUEST, 0, reinterpret_cast<LPARAM>(layout));
    }
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
    for (std::size_t i = 1; i < text.size(); ++i) {
        if (iswupper(text[i])) return true;
    }
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

    if (Dict::is_valid_word(lower_typed, source)) return false;

    return true;
}

struct ModTracker {
    bool down{};
    bool other_pressed{};
    ULONGLONG last_tap{};
};

}  // namespace

struct Engine::Impl {
    HWND message_window{};
    Settings* settings{};
    HHOOK keyboard_hook{};
    HHOOK mouse_hook{};
    HWINEVENTHOOK foreground_hook{};
    HWINEVENTHOOK focus_hook{};

    std::vector<TypedKey> word;
    std::vector<TypedKey> prev_word;
    int boundary_count{};
    HWND word_owner{};
    HWND last_foreground{};
    bool enabled{true};
    HKL first_layout{};
    HKL second_layout{};

    ModTracker ctrl_tracker{};
    ModTracker shift_tracker{};
    ModTracker alt_tracker{};

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

    HKL layout_for_language(LANGID lang_id) const noexcept {
        const HKL candidates[]{first_layout, second_layout, current_layout()};
        for (HKL hkl : candidates) {
            if (hkl && PRIMARYLANGID(LOWORD(reinterpret_cast<ULONG_PTR>(hkl))) == lang_id) {
                return hkl;
            }
        }
        const int count = GetKeyboardLayoutList(0, nullptr);
        if (count > 0) {
            std::vector<HKL> list(static_cast<std::size_t>(count));
            GetKeyboardLayoutList(count, list.data());
            for (HKL hkl : list) {
                if (hkl && PRIMARYLANGID(LOWORD(reinterpret_cast<ULONG_PTR>(hkl))) == lang_id) {
                    return hkl;
                }
            }
        }
        return nullptr;
    }


    void clear_word() noexcept {
        word.clear();
        word_owner = nullptr;
    }

    void clear_all() noexcept {
        clear_word();
        prev_word.clear();
        boundary_count = 0;
        undo_available = false;
        screen_text.clear();
        alternate_text.clear();
        pending_boundary_word.clear();
        pending_boundary_vk = 0;
        pending_boundary_owner = nullptr;
    }

    void dispatch_double_tap(TriggerKey t_match, SwitchKey s_match, CaseKey c_match) noexcept {
        if (settings) {
            if (settings->switch_hotkey() == s_match) {
                PostMessageW(message_window, kSwitchMessage, 0, 0);
                return;
            }
            if (settings->trigger() == t_match) {
                PostMessageW(message_window, kTriggerMessage, 0, 0);
                return;
            }
            if (settings->case_hotkey() == c_match) {
                PostMessageW(message_window, kCaseMessage, 0, 0);
                return;
            }
        } else if (t_match == TriggerKey::CtrlDoubleTap) {
            PostMessageW(message_window, kTriggerMessage, 0, 0);
        }
    }

    void on_key_down(DWORD vk, DWORD scan) {
        if (!enabled) return;

        if (is_control(vk)) {
            if (!ctrl_tracker.down) {
                ctrl_tracker.down = true;
                ctrl_tracker.other_pressed = false;
            }
            return;
        }
        if (is_shift(vk)) {
            if (!shift_tracker.down) {
                shift_tracker.down = true;
                shift_tracker.other_pressed = false;
            }
            return;
        }
        if (is_alt(vk)) {
            if (!alt_tracker.down) {
                alt_tracker.down = true;
                alt_tracker.other_pressed = false;
            }
            return;
        }

        if (ctrl_tracker.down) ctrl_tracker.other_pressed = true;
        if (shift_tracker.down) shift_tracker.other_pressed = true;
        if (alt_tracker.down) alt_tracker.other_pressed = true;
        ctrl_tracker.last_tap = 0;
        shift_tracker.last_tap = 0;
        alt_tracker.last_tap = 0;

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
                    prev_word = word;
                    boundary_count = 1;
                    pending_boundary_word = word;
                    pending_boundary_vk = vk;
                    pending_boundary_owner = word_owner;
                    PostMessageW(message_window, kBoundaryMessage, 0, 0);
                } else if (!prev_word.empty()) {
                    ++boundary_count;
                }
            } else {
                prev_word.clear();
                boundary_count = 0;
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
            if (useful_foreground(foreground, message_window)) last_foreground = foreground;
            if (word_owner && foreground != word_owner) clear_word();
            if (word.empty()) {
                word_owner = foreground;
                prev_word.clear();
                boundary_count = 0;
            }

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

        if (is_control(vk)) {
            const bool tap = ctrl_tracker.down && !ctrl_tracker.other_pressed;
            ctrl_tracker.down = false;
            ctrl_tracker.other_pressed = false;
            if (!tap) return;
            const ULONGLONG now = GetTickCount64();
            if (ctrl_tracker.last_tap && now - ctrl_tracker.last_tap <= kDoubleTapWindowMs) {
                ctrl_tracker.last_tap = 0;
                dispatch_double_tap(TriggerKey::CtrlDoubleTap, SwitchKey::CtrlDoubleTap, CaseKey::CtrlDoubleTap);
            } else {
                ctrl_tracker.last_tap = now;
            }
            return;
        }

        if (is_shift(vk)) {
            const bool tap = shift_tracker.down && !shift_tracker.other_pressed;
            shift_tracker.down = false;
            shift_tracker.other_pressed = false;
            if (!tap) return;
            const ULONGLONG now = GetTickCount64();
            if (shift_tracker.last_tap && now - shift_tracker.last_tap <= kDoubleTapWindowMs) {
                shift_tracker.last_tap = 0;
                dispatch_double_tap(TriggerKey::ShiftDoubleTap, SwitchKey::ShiftDoubleTap, CaseKey::ShiftDoubleTap);
            } else {
                shift_tracker.last_tap = now;
            }
            return;
        }

        if (is_alt(vk)) {
            const bool tap = alt_tracker.down && !alt_tracker.other_pressed;
            alt_tracker.down = false;
            alt_tracker.other_pressed = false;
            if (!tap) return;
            const ULONGLONG now = GetTickCount64();
            if (alt_tracker.last_tap && now - alt_tracker.last_tap <= kDoubleTapWindowMs) {
                alt_tracker.last_tap = 0;
                dispatch_double_tap(TriggerKey::AltDoubleTap, SwitchKey::AltDoubleTap, CaseKey::AltDoubleTap);
            } else {
                alt_tracker.last_tap = now;
            }
            return;
        }
    }

    static LRESULT CALLBACK keyboard_proc(int code, WPARAM message, LPARAM data) noexcept {
        if (code == HC_ACTION && instance) {
            const auto* key = reinterpret_cast<const KBDLLHOOKSTRUCT*>(data);
            if (key->dwExtraInfo != kInjectedMarker) {
                // 1. CapsLock Interception (macOS parity & Punto-style)
                if (key->vkCode == VK_CAPITAL && instance->settings) {
                    const bool is_switch_caps = (instance->settings->switch_hotkey() == SwitchKey::CapsLock);
                    const bool is_trig_caps = (instance->settings->trigger() == TriggerKey::CapsLock);
                    if (is_switch_caps || is_trig_caps) {
                        const bool shift_held = (GetAsyncKeyState(VK_SHIFT) & 0x8000) != 0;
                        if (!shift_held) {
                            if (message == WM_KEYDOWN || message == WM_SYSKEYDOWN) {
                                if (is_switch_caps) {
                                    PostMessageW(instance->message_window, kSwitchMessage, 0, 0);
                                } else {
                                    PostMessageW(instance->message_window, kTriggerMessage, 0, 0);
                                }
                            }
                            return 1; // Suppress CapsLock state change
                        }
                    }
                }

                // 2. Pause / Break Interception
                if (key->vkCode == VK_PAUSE && instance->settings) {
                    if (message == WM_KEYDOWN || message == WM_SYSKEYDOWN) {
                        if (instance->settings->switch_hotkey() == SwitchKey::PauseBreak) {
                            PostMessageW(instance->message_window, kSwitchMessage, 0, 0);
                            return 1;
                        }
                        if (instance->settings->trigger() == TriggerKey::PauseBreak) {
                            PostMessageW(instance->message_window, kTriggerMessage, 0, 0);
                            return 1;
                        }
                        if (instance->settings->case_hotkey() == CaseKey::PauseBreak) {
                            PostMessageW(instance->message_window, kCaseMessage, 0, 0);
                            return 1;
                        }
                    }
                }

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
            const HWND fg = GetForegroundWindow();
            if (useful_foreground(fg, instance->message_window))
                instance->last_foreground = fg;
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
        if (!copy_current_selection(selected) || selected.empty()) {
            clipboard.restore();
            return false;
        }
        std::wstring converted = convert_text_bidirectional(selected, source, target);
        if (converted == selected) {
            clipboard.restore();
            return false;
        }
        if (!clipboard.restore()) {
            return false;
        }
        if (!replace_text(0, converted)) {
            return false;
        }

        HKL final_target = target;
        wchar_t last_letter = 0;
        for (wchar_t c : converted) {
            if (is_cyrillic_char(c) || is_latin_char(c)) {
                last_letter = c;
            }
        }
        if (last_letter != 0) {
            if (is_cyrillic_char(last_letter)) {
                HKL ru = layout_for_language(LANG_RUSSIAN);
                if (ru) final_target = ru;
            } else if (is_latin_char(last_letter)) {
                HKL en = layout_for_language(LANG_ENGLISH);
                if (en) final_target = en;
            }
        }
        switch_layout(final_target);

        if (permit_undo) {
            screen_text = std::move(converted);
            alternate_text = std::move(selected);
            screen_layout = final_target;
            alternate_layout = (final_target == target) ? source : target;
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

    void switch_layout_direct() noexcept {
        if (!enabled) return;
        const HKL source = current_layout();
        const HKL target = target_layout(source);
        if (!source || !target) return;

        switch_layout(target);
        clear_all();

        if (settings && settings->sound_on_switch()) {
            MessageBeep(MB_OK);
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

        if (settings && settings->convert_whole_line()) {
            convert_line();
            return;
        }

        // Case A: Word in active buffer
        if (!word.empty() && word_owner == GetForegroundWindow()) {
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
            if (settings && settings->sound_on_switch()) MessageBeep(MB_OK);
            return;
        }

        // Case B: Word was just completed with boundary (macOS parity with prevWordKeys)
        if (word.empty() && !prev_word.empty() && boundary_count > 0 && word_owner == GetForegroundWindow()) {
            const HKL source = current_layout();
            const HKL target = target_layout(source);
            if (source && target) {
                std::size_t core_count = prev_word.size();
                std::wstring suffix;
                while (core_count > 0) {
                    wchar_t source_character{};
                    if (!translate_key(prev_word[core_count - 1], source, source_character) ||
                        !is_trailing_punctuation(source_character))
                        break;
                    suffix.insert(suffix.begin(), source_character);
                    --core_count;
                }
                if (core_count > 0) {
                    std::wstring original;
                    std::wstring converted;
                    if (translate_keys(prev_word, core_count, source, original) &&
                        translate_keys(prev_word, core_count, target, converted) && !converted.empty()) {
                        original += suffix;
                        converted += suffix;
                        std::wstring spaces(static_cast<std::size_t>(boundary_count), L' ');
                        const std::size_t to_delete = prev_word.size() + boundary_count;
                        if (replace_text(to_delete, converted + spaces)) {
                            switch_layout(target);
                            screen_text = converted + spaces;
                            alternate_text = original + spaces;
                            screen_layout = target;
                            alternate_layout = source;
                            undo_available = true;
                            prev_word.clear();
                            boundary_count = 0;
                            if (settings && settings->sound_on_switch()) MessageBeep(MB_OK);
                            return;
                        }
                    }
                }
            }
        }

        // Case C: Try selection conversion
        if (convert_selection(true)) {
            if (settings && settings->sound_on_switch()) MessageBeep(MB_OK);
            return;
        }

        // Case D: Buffer is empty and nothing is selected -> SWITCH LAYOUT DIRECTLY!
        // This ensures pressing the key ALWAYS performs a meaningful action.
        const HKL source = current_layout();
        const HKL target = target_layout(source);
        if (source && target) {
            switch_layout(target);
            clear_all();
            if (settings && settings->sound_on_switch()) MessageBeep(MB_OK);
        }
    }

    void convert_line() noexcept {
        if (!enabled) return;
        restore_target_focus(last_foreground, message_window);
        if (is_protected_foreground()) return;
        clear_all();

        // 1. If text was already selected (e.g. user selected it with mouse), convert selection
        if (convert_selection(true)) return;

        // 2. Select current line (Home -> Shift+End)
        if (!send_line_selection()) return;
        Sleep(60);

        if (!convert_selection(true)) {
            // Unselect on no-op so line does not stay highlighted
            INPUT unsel[]{key_input(VK_RIGHT, 0, 0), key_input(VK_RIGHT, 0, KEYEVENTF_KEYUP)};
            SendInput(static_cast<UINT>(std::size(unsel)), unsel, sizeof(INPUT));
        }
    }

    void change_case() noexcept {
        if (!enabled) return;
        restore_target_focus(last_foreground, message_window);
        if (is_protected_foreground()) return;

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
        // 1. If text was already selected
        if (copy_current_selection(selected) && !selected.empty() && has_letter(selected)) {
            std::wstring next = next_case(selected);
            if (next != selected) {
                if (clipboard.restore()) {
                    replace_text(0, next);
                    clear_all();
                    return;
                }
            }
        }

        // 2. Select the word to the left of caret (Ctrl+Shift+Left)
        if (send_word_selection(true)) {
            Sleep(35);
            if (copy_current_selection(selected) && !selected.empty() && has_letter(selected)) {
                std::wstring next = next_case(selected);
                if (next != selected) {
                    if (clipboard.restore()) {
                        replace_text(0, next);
                        clear_all();
                        return;
                    }
                }
            }
            // Collapse selection
            INPUT unsel[]{key_input(VK_RIGHT, 0, 0), key_input(VK_RIGHT, 0, KEYEVENTF_KEYUP)};
            SendInput(static_cast<UINT>(std::size(unsel)), unsel, sizeof(INPUT));
        }

        // 3. Fallback: try selecting word to the right (if cursor was at start of word)
        if (send_word_selection(false)) {
            Sleep(35);
            if (copy_current_selection(selected) && !selected.empty() && has_letter(selected)) {
                std::wstring next = next_case(selected);
                if (next != selected) {
                    if (clipboard.restore()) {
                        replace_text(0, next);
                        clear_all();
                        return;
                    }
                }
            }
            // Collapse selection
            INPUT unsel[]{key_input(VK_LEFT, 0, 0), key_input(VK_LEFT, 0, KEYEVENTF_KEYUP)};
            SendInput(static_cast<UINT>(std::size(unsel)), unsel, sizeof(INPUT));
        }

        clipboard.restore();
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
void Engine::switch_layout_direct() noexcept { impl_->switch_layout_direct(); }
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

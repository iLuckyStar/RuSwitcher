#include "settings_dialog.h"

#include "engine.h"
#include "settings.h"

#include <commctrl.h>
#include <string>
#include <vector>

namespace ruswitcher {
namespace {

constexpr int IDC_COMBO_TRIGGER = 1001;
constexpr int IDC_COMBO_SWITCH = 1002;
constexpr int IDC_COMBO_CASE = 1003;
constexpr int IDC_COMBO_FIRST = 1004;
constexpr int IDC_COMBO_SECOND = 1005;

constexpr int IDC_CHK_AUTOCONVERT = 1010;
constexpr int IDC_CHK_TWOCAPS = 1011;
constexpr int IDC_CHK_NUMBERS = 1012;
constexpr int IDC_CHK_WHOLELINE = 1013;
constexpr int IDC_CHK_SOUND = 1014;
constexpr int IDC_CHK_AUTOSTART = 1015;

constexpr int IDC_BTN_OK = 1020;
constexpr int IDC_BTN_CANCEL = 1021;

struct DialogContext {
    Engine& engine;
    Settings& settings;
    HWND hwnd{};
    HWND cb_trigger{};
    HWND cb_switch{};
    HWND cb_case{};
    HWND cb_first{};
    HWND cb_second{};
    HWND chk_autoconvert{};
    HWND chk_twocaps{};
    HWND chk_numbers{};
    HWND chk_wholeline{};
    HWND chk_sound{};
    HWND chk_autostart{};
    HFONT font{};
};

DialogContext* g_dlg_ctx = nullptr;

HWND create_label(HWND parent, HINSTANCE inst, const wchar_t* text, int x, int y, int w, int h, HFONT font) {
    HWND hnd = CreateWindowExW(0, L"STATIC", text, WS_CHILD | WS_VISIBLE, x, y, w, h, parent, nullptr, inst, nullptr);
    if (font) SendMessageW(hnd, WM_SETFONT, reinterpret_cast<WPARAM>(font), TRUE);
    return hnd;
}

HWND create_combo(HWND parent, HINSTANCE inst, int id, int x, int y, int w, int h, HFONT font) {
    HWND hnd = CreateWindowExW(0, L"COMBOBOX", L"", WS_CHILD | WS_VISIBLE | CBS_DROPDOWNLIST | WS_VSCROLL | WS_TABSTOP,
                              x, y, w, h, parent, reinterpret_cast<HMENU>(static_cast<INT_PTR>(id)), inst, nullptr);
    if (font) SendMessageW(hnd, WM_SETFONT, reinterpret_cast<WPARAM>(font), TRUE);
    return hnd;
}

HWND create_checkbox(HWND parent, HINSTANCE inst, int id, const wchar_t* text, int x, int y, int w, int h, HFONT font) {
    HWND hnd = CreateWindowExW(0, L"BUTTON", text, WS_CHILD | WS_VISIBLE | BS_AUTOCHECKBOX | WS_TABSTOP,
                              x, y, w, h, parent, reinterpret_cast<HMENU>(static_cast<INT_PTR>(id)), inst, nullptr);
    if (font) SendMessageW(hnd, WM_SETFONT, reinterpret_cast<WPARAM>(font), TRUE);
    return hnd;
}

HWND create_button(HWND parent, HINSTANCE inst, int id, const wchar_t* text, int x, int y, int w, int h, HFONT font, bool is_default = false) {
    DWORD style = WS_CHILD | WS_VISIBLE | WS_TABSTOP | (is_default ? BS_DEFPUSHBUTTON : BS_PUSHBUTTON);
    HWND hnd = CreateWindowExW(0, L"BUTTON", text, style, x, y, w, h, parent,
                              reinterpret_cast<HMENU>(static_cast<INT_PTR>(id)), inst, nullptr);
    if (font) SendMessageW(hnd, WM_SETFONT, reinterpret_cast<WPARAM>(font), TRUE);
    return hnd;
}

LRESULT CALLBACK dialog_proc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam) {
    switch (msg) {
        case WM_CREATE: {
            NONCLIENTMETRICSW ncm{sizeof(ncm)};
            SystemParametersInfoW(SPI_GETNONCLIENTMETRICS, sizeof(ncm), &ncm, 0);
            HFONT font = CreateFontIndirectW(&ncm.lfMessageFont);
            if (!font) font = static_cast<HFONT>(GetStockObject(DEFAULT_GUI_FONT));

            auto* ctx = g_dlg_ctx;
            ctx->hwnd = hwnd;
            ctx->font = font;
            HINSTANCE inst = reinterpret_cast<HINSTANCE>(GetWindowLongPtrW(hwnd, GWLP_HINSTANCE));

            int y = 16;
            create_label(hwnd, inst, L"Клавиша конвертации слова:", 16, y + 4, 210, 20, font);
            ctx->cb_trigger = create_combo(hwnd, inst, IDC_COMBO_TRIGGER, 230, y, 200, 200, font);
            SendMessageW(ctx->cb_trigger, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(L"Двойной Ctrl"));
            SendMessageW(ctx->cb_trigger, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(L"Двойной Shift"));
            SendMessageW(ctx->cb_trigger, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(L"Двойной Alt"));
            SendMessageW(ctx->cb_trigger, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(L"Caps Lock"));
            SendMessageW(ctx->cb_trigger, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(L"Pause / Break"));
            SendMessageW(ctx->cb_trigger, CB_SETCURSEL, static_cast<WPARAM>(ctx->settings.trigger()), 0);

            y += 36;
            create_label(hwnd, inst, L"Клавиша смены раскладки:", 16, y + 4, 210, 20, font);
            ctx->cb_switch = create_combo(hwnd, inst, IDC_COMBO_SWITCH, 230, y, 200, 200, font);
            SendMessageW(ctx->cb_switch, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(L"Выключено"));
            SendMessageW(ctx->cb_switch, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(L"Caps Lock (RU ↔ EN)"));
            SendMessageW(ctx->cb_switch, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(L"Двойной Shift"));
            SendMessageW(ctx->cb_switch, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(L"Двойной Ctrl"));
            SendMessageW(ctx->cb_switch, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(L"Двойной Alt"));
            SendMessageW(ctx->cb_switch, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(L"Pause / Break"));
            SendMessageW(ctx->cb_switch, CB_SETCURSEL, static_cast<WPARAM>(ctx->settings.switch_hotkey()), 0);

            y += 36;
            create_label(hwnd, inst, L"Клавиша смены регистра:", 16, y + 4, 210, 20, font);
            ctx->cb_case = create_combo(hwnd, inst, IDC_COMBO_CASE, 230, y, 200, 200, font);
            SendMessageW(ctx->cb_case, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(L"Выключено"));
            SendMessageW(ctx->cb_case, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(L"Pause / Break"));
            SendMessageW(ctx->cb_case, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(L"Двойной Shift"));
            SendMessageW(ctx->cb_case, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(L"Двойной Ctrl"));
            SendMessageW(ctx->cb_case, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(L"Двойной Alt"));
            SendMessageW(ctx->cb_case, CB_SETCURSEL, static_cast<WPARAM>(ctx->settings.case_hotkey()), 0);

            y += 42;
            ctx->chk_autoconvert = create_checkbox(hwnd, inst, IDC_CHK_AUTOCONVERT,
                L"Авто-конвертация на лету (по Space, Enter, Tab)", 16, y, 420, 22, font);
            SendMessageW(ctx->chk_autoconvert, BM_SETCHECK, ctx->settings.auto_convert() ? BST_CHECKED : BST_UNCHECKED, 0);

            y += 28;
            ctx->chk_twocaps = create_checkbox(hwnd, inst, IDC_CHK_TWOCAPS,
                L"Исправлять две заглавные буквы (ПРивет → Привет)", 16, y, 420, 22, font);
            SendMessageW(ctx->chk_twocaps, BM_SETCHECK, ctx->settings.fix_two_caps() ? BST_CHECKED : BST_UNCHECKED, 0);

            y += 28;
            ctx->chk_numbers = create_checkbox(hwnd, inst, IDC_CHK_NUMBERS,
                L"Исправлять опечатки в цифрах (1ю8 → 1.8, 5б2 → 5,2)", 16, y, 420, 22, font);
            SendMessageW(ctx->chk_numbers, BM_SETCHECK, ctx->settings.fix_numbers() ? BST_CHECKED : BST_UNCHECKED, 0);

            y += 28;
            ctx->chk_wholeline = create_checkbox(hwnd, inst, IDC_CHK_WHOLELINE,
                L"Конвертировать всю текущую строку (Shift+Home)", 16, y, 420, 22, font);
            SendMessageW(ctx->chk_wholeline, BM_SETCHECK, ctx->settings.convert_whole_line() ? BST_CHECKED : BST_UNCHECKED, 0);

            y += 28;
            ctx->chk_sound = create_checkbox(hwnd, inst, IDC_CHK_SOUND,
                L"Звуковой сигнал при переключении раскладки", 16, y, 420, 22, font);
            SendMessageW(ctx->chk_sound, BM_SETCHECK, ctx->settings.sound_on_switch() ? BST_CHECKED : BST_UNCHECKED, 0);

            y += 28;
            ctx->chk_autostart = create_checkbox(hwnd, inst, IDC_CHK_AUTOSTART,
                L"Запускать RuSwitcher при входе в систему", 16, y, 420, 22, font);
            SendMessageW(ctx->chk_autostart, BM_SETCHECK, ctx->settings.autostart_enabled() ? BST_CHECKED : BST_UNCHECKED, 0);

            y += 38;
            create_label(hwnd, inst, L"Первая раскладка:", 16, y + 4, 150, 20, font);
            ctx->cb_first = create_combo(hwnd, inst, IDC_COMBO_FIRST, 170, y, 260, 200, font);

            y += 34;
            create_label(hwnd, inst, L"Вторая раскладка:", 16, y + 4, 150, 20, font);
            ctx->cb_second = create_combo(hwnd, inst, IDC_COMBO_SECOND, 170, y, 260, 200, font);

            const auto& layouts = ctx->settings.layouts();
            int sel_first = 0;
            int sel_second = (layouts.size() > 1) ? 1 : 0;
            for (std::size_t i = 0; i < layouts.size(); ++i) {
                SendMessageW(ctx->cb_first, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(layouts[i].name.c_str()));
                SendMessageW(ctx->cb_second, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(layouts[i].name.c_str()));
                if (layouts[i].handle == ctx->settings.first_layout()) sel_first = static_cast<int>(i);
                if (layouts[i].handle == ctx->settings.second_layout()) sel_second = static_cast<int>(i);
            }
            SendMessageW(ctx->cb_first, CB_SETCURSEL, sel_first, 0);
            SendMessageW(ctx->cb_second, CB_SETCURSEL, sel_second, 0);

            y += 48;
            create_button(hwnd, inst, IDC_BTN_OK, L"Сохранить", 230, y, 100, 28, font, true);
            create_button(hwnd, inst, IDC_BTN_CANCEL, L"Отмена", 340, y, 90, 28, font, false);
            return 0;
        }

        case WM_COMMAND: {
            int id = LOWORD(wparam);
            if (id == IDC_BTN_OK) {
                auto* ctx = g_dlg_ctx;
                if (ctx) {
                    int trig_sel = static_cast<int>(SendMessageW(ctx->cb_trigger, CB_GETCURSEL, 0, 0));
                    if (trig_sel >= 0) ctx->settings.set_trigger(static_cast<TriggerKey>(trig_sel));

                    int sw_sel = static_cast<int>(SendMessageW(ctx->cb_switch, CB_GETCURSEL, 0, 0));
                    if (sw_sel >= 0) ctx->settings.set_switch_hotkey(static_cast<SwitchKey>(sw_sel));

                    int cs_sel = static_cast<int>(SendMessageW(ctx->cb_case, CB_GETCURSEL, 0, 0));
                    if (cs_sel >= 0) ctx->settings.set_case_hotkey(static_cast<CaseKey>(cs_sel));

                    ctx->settings.set_auto_convert(SendMessageW(ctx->chk_autoconvert, BM_GETCHECK, 0, 0) == BST_CHECKED);
                    ctx->settings.set_fix_two_caps(SendMessageW(ctx->chk_twocaps, BM_GETCHECK, 0, 0) == BST_CHECKED);
                    ctx->settings.set_fix_numbers(SendMessageW(ctx->chk_numbers, BM_GETCHECK, 0, 0) == BST_CHECKED);
                    ctx->settings.set_convert_whole_line(SendMessageW(ctx->chk_wholeline, BM_GETCHECK, 0, 0) == BST_CHECKED);
                    ctx->settings.set_sound_on_switch(SendMessageW(ctx->chk_sound, BM_GETCHECK, 0, 0) == BST_CHECKED);
                    ctx->settings.set_autostart(SendMessageW(ctx->chk_autostart, BM_GETCHECK, 0, 0) == BST_CHECKED);

                    const auto& layouts = ctx->settings.layouts();
                    int f_sel = static_cast<int>(SendMessageW(ctx->cb_first, CB_GETCURSEL, 0, 0));
                    if (f_sel >= 0 && static_cast<std::size_t>(f_sel) < layouts.size()) {
                        ctx->settings.set_first_layout(layouts[static_cast<std::size_t>(f_sel)].handle);
                    }
                    int s_sel = static_cast<int>(SendMessageW(ctx->cb_second, CB_GETCURSEL, 0, 0));
                    if (s_sel >= 0 && static_cast<std::size_t>(s_sel) < layouts.size()) {
                        ctx->settings.set_second_layout(layouts[static_cast<std::size_t>(s_sel)].handle);
                    }

                    ctx->engine.set_layout_pair(ctx->settings.first_layout(), ctx->settings.second_layout());
                }
                DestroyWindow(hwnd);
                return 0;
            }
            if (id == IDC_BTN_CANCEL || id == IDCANCEL) {
                DestroyWindow(hwnd);
                return 0;
            }
            break;
        }

        case WM_CLOSE:
            DestroyWindow(hwnd);
            return 0;

        case WM_DESTROY:
            if (g_dlg_ctx && g_dlg_ctx->font) {
                DeleteObject(g_dlg_ctx->font);
                g_dlg_ctx->font = nullptr;
            }
            PostQuitMessage(0);
            return 0;
    }
    return DefWindowProcW(hwnd, msg, wparam, lparam);
}

}  // namespace

void SettingsDialog::show(HWND parent, HINSTANCE instance, Engine& engine, Settings& settings) noexcept {
    DialogContext ctx{engine, settings};
    g_dlg_ctx = &ctx;

    const wchar_t class_name[] = L"RuSwitcher.SettingsDialog";
    WNDCLASSW wc{};
    wc.lpfnWndProc = dialog_proc;
    wc.hInstance = instance;
    wc.lpszClassName = class_name;
    wc.hbrBackground = reinterpret_cast<HBRUSH>(COLOR_BTNFACE + 1);
    wc.hCursor = LoadCursorW(nullptr, IDC_ARROW);
    RegisterClassW(&wc);

    const int width = 460;
    const int height = 480;
    const int x = (GetSystemMetrics(SM_CXSCREEN) - width) / 2;
    const int y = (GetSystemMetrics(SM_CYSCREEN) - height) / 2;

    HWND hwnd = CreateWindowExW(WS_EX_DLGMODALFRAME | WS_EX_TOPMOST, class_name,
                               L"Настройки RuSwitcher",
                               WS_POPUP | WS_CAPTION | WS_SYSMENU | WS_VISIBLE,
                               x, y, width, height, parent, nullptr, instance, nullptr);
    if (!hwnd) return;

    EnableWindow(parent, FALSE);
    SetForegroundWindow(hwnd);

    MSG msg{};
    while (GetMessageW(&msg, nullptr, 0, 0) > 0) {
        if (!IsDialogMessageW(hwnd, &msg)) {
            TranslateMessage(&msg);
            DispatchMessageW(&msg);
        }
    }

    EnableWindow(parent, TRUE);
    SetForegroundWindow(parent);
    g_dlg_ctx = nullptr;
    UnregisterClassW(class_name, instance);
}

}  // namespace ruswitcher

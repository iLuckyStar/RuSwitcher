#include "clipboard.h"

#include <windows.h>
#include <cstdint>
#include <utility>
#include <vector>

namespace ruswitcher {
namespace {

constexpr ULONG_PTR kInjectedMarker = 0x52555357;

struct SavedClipboardFormat {
    UINT format{};
    std::vector<uint8_t> data;
};

INPUT key_input(WORD vk, DWORD flags) noexcept {
    INPUT input{};
    input.type = INPUT_KEYBOARD;
    input.ki.wVk = vk;
    input.ki.dwFlags = flags;
    input.ki.dwExtraInfo = kInjectedMarker;
    return input;
}

bool send_copy_chord() noexcept {
    const BYTE scan = static_cast<BYTE>(
        MapVirtualKeyExW('C', MAPVK_VK_TO_VSC, GetKeyboardLayout(0)));
    keybd_event(VK_LCONTROL, 0x1D, KEYEVENTF_EXTENDEDKEY, kInjectedMarker);
    keybd_event('C', scan, 0, kInjectedMarker);
    keybd_event('C', scan, KEYEVENTF_KEYUP, kInjectedMarker);
    keybd_event(VK_LCONTROL, 0x1D, KEYEVENTF_EXTENDEDKEY | KEYEVENTF_KEYUP,
                kInjectedMarker);
    return true;
}

void wait_for_modifiers_released() noexcept {
    const bool down = (GetAsyncKeyState(VK_CONTROL) & 0x8000) != 0 ||
                      (GetAsyncKeyState(VK_SHIFT) & 0x8000) != 0 ||
                      (GetAsyncKeyState(VK_MENU) & 0x8000) != 0 ||
                      (GetAsyncKeyState(VK_LWIN) & 0x8000) != 0 ||
                      (GetAsyncKeyState(VK_RWIN) & 0x8000) != 0;
    if (!down) return;

    const ULONGLONG deadline = GetTickCount64() + 60;
    while (GetTickCount64() < deadline) {
        const bool still_down = (GetAsyncKeyState(VK_CONTROL) & 0x8000) != 0 ||
                               (GetAsyncKeyState(VK_SHIFT) & 0x8000) != 0 ||
                               (GetAsyncKeyState(VK_MENU) & 0x8000) != 0 ||
                               (GetAsyncKeyState(VK_LWIN) & 0x8000) != 0 ||
                               (GetAsyncKeyState(VK_RWIN) & 0x8000) != 0;
        if (!still_down) break;
        Sleep(2);
    }
}

bool read_unicode_text(std::wstring& text) noexcept {
    if (!OpenClipboard(nullptr)) return false;
    bool copied = false;
    if (IsClipboardFormatAvailable(CF_UNICODETEXT)) {
        const HANDLE handle = GetClipboardData(CF_UNICODETEXT);
        const auto* value = handle ? static_cast<const wchar_t*>(GlobalLock(handle)) : nullptr;
        if (value) {
            text.assign(value);
            GlobalUnlock(handle);
            copied = !text.empty();
        }
    }
    CloseClipboard();
    return copied;
}

bool wait_for_text(DWORD initial_sequence, DWORD timeout_ms, std::wstring& text) noexcept {
    const ULONGLONG deadline = GetTickCount64() + timeout_ms;
    while (GetTickCount64() < deadline) {
        if (GetClipboardSequenceNumber() != initial_sequence && read_unicode_text(text)) return true;
        Sleep(3);
    }
    return false;
}

}  // namespace

struct ClipboardSnapshot::Impl {
    std::vector<SavedClipboardFormat> items;
    bool captured{};
};

ClipboardSnapshot::ClipboardSnapshot() noexcept : impl_(new Impl) {}
ClipboardSnapshot::~ClipboardSnapshot() { delete impl_; }

bool ClipboardSnapshot::capture() noexcept {
    impl_->items.clear();
    impl_->captured = false;

    bool opened = false;
    for (int attempt = 0; attempt < 8; ++attempt) {
        if (OpenClipboard(nullptr)) {
            opened = true;
            break;
        }
        Sleep(5 + attempt * 5);
    }
    if (!opened) return false;

    UINT format = 0;
    while ((format = EnumClipboardFormats(format)) != 0) {
        // Skip GDI handle formats that cannot be serialized through GlobalSize/GlobalLock
        if (format == CF_BITMAP || format == CF_METAFILEPICT || format == CF_ENHMETAFILE || format == CF_PALETTE) {
            continue;
        }
        HANDLE handle = GetClipboardData(format);
        if (!handle) continue;

        SIZE_T size = GlobalSize(handle);
        if (size == 0) continue;

        const void* ptr = GlobalLock(handle);
        if (!ptr) continue;

        SavedClipboardFormat item;
        item.format = format;
        item.data.resize(size);
        CopyMemory(item.data.data(), ptr, size);
        GlobalUnlock(handle);

        impl_->items.push_back(std::move(item));
    }

    CloseClipboard();
    impl_->captured = true;
    return true;
}

bool ClipboardSnapshot::restore() noexcept {
    if (!impl_->captured) return false;

    bool opened = false;
    for (int attempt = 0; attempt < 8; ++attempt) {
        if (OpenClipboard(nullptr)) {
            opened = true;
            break;
        }
        Sleep(5 + attempt * 5);
    }
    if (!opened) return false;

    EmptyClipboard();

    for (const auto& item : impl_->items) {
        if (item.data.empty()) continue;
        HGLOBAL hMem = GlobalAlloc(GMEM_MOVEABLE, item.data.size());
        if (!hMem) continue;
        void* ptr = GlobalLock(hMem);
        if (ptr) {
            CopyMemory(ptr, item.data.data(), item.data.size());
            GlobalUnlock(hMem);
            if (!SetClipboardData(item.format, hMem)) {
                GlobalFree(hMem);
            }
        } else {
            GlobalFree(hMem);
        }
    }

    CloseClipboard();
    impl_->captured = false;
    return true;
}

bool copy_current_selection(std::wstring& text) noexcept {
    text.clear();
    wait_for_modifiers_released();
    const DWORD sequence = GetClipboardSequenceNumber();
    if (!send_copy_chord()) return false;
    return wait_for_text(sequence, 45, text);
}

bool paste_text(const std::wstring& text) noexcept {
    bool opened = false;
    for (int attempt = 0; attempt < 8; ++attempt) {
        if (OpenClipboard(nullptr)) {
            opened = true;
            break;
        }
        Sleep(5 + attempt * 5);
    }
    if (!opened) return false;

    EmptyClipboard();
    const std::size_t bytes = (text.size() + 1) * sizeof(wchar_t);
    HGLOBAL handle = GlobalAlloc(GMEM_MOVEABLE, bytes);
    if (!handle) {
        CloseClipboard();
        return false;
    }
    void* ptr = GlobalLock(handle);
    if (ptr) {
        CopyMemory(ptr, text.c_str(), bytes);
        GlobalUnlock(handle);
        SetClipboardData(CF_UNICODETEXT, handle);
    }
    CloseClipboard();

    const BYTE scan = static_cast<BYTE>(
        MapVirtualKeyExW('V', MAPVK_VK_TO_VSC, GetKeyboardLayout(0)));
    keybd_event(VK_LCONTROL, 0x1D, KEYEVENTF_EXTENDEDKEY, kInjectedMarker);
    keybd_event('V', scan, 0, kInjectedMarker);
    keybd_event('V', scan, KEYEVENTF_KEYUP, kInjectedMarker);
    keybd_event(VK_LCONTROL, 0x1D, KEYEVENTF_EXTENDEDKEY | KEYEVENTF_KEYUP, kInjectedMarker);
    return true;
}

}  // namespace ruswitcher

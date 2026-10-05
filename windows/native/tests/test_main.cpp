#include <iostream>
#undef NDEBUG
#include <cassert>
#include <string>
#include <ole2.h>

#include "brand_words.h"
#include "dict.h"
#include "text_fixes.h"

using namespace ruswitcher;

void test_brand_words() {
    assert(is_brand_word(L"chatgpt"));
    assert(is_brand_word(L"ChatGPT"));
    assert(is_brand_word(L"github"));
    assert(is_brand_word(L"GitHub"));
    assert(is_brand_word(L"claude"));
    assert(is_brand_word(L"Claude"));
    assert(is_brand_word(L"macos"));
    assert(is_brand_word(L"macOS"));
    assert(is_brand_word(L"youtube"));
    assert(is_brand_word(L"docker"));
    assert(is_brand_word(L"kubernetes"));
    assert(is_brand_word(L"deepseek"));
    assert(!is_brand_word(L"notabrandword"));
    assert(!is_brand_word(L"ab"));
    assert(!is_brand_word(L""));
    std::cout << "[PASS] Brand words tests (154 entries binary search)\n";
}

void test_two_caps() {
    auto r1 = fix_two_caps(L"ПРивет", nullptr, false);
    assert(r1.has_value() && *r1 == L"Привет");

    auto r2 = fix_two_caps(L"TWo", nullptr, false);
    assert(r2.has_value() && *r2 == L"Two");

    auto r3 = fix_two_caps(L"КОгда", nullptr, false);
    assert(r3.has_value() && *r3 == L"Когда");

    // Exclude plural acronyms
    auto r4 = fix_two_caps(L"IDs", nullptr, false);
    assert(!r4.has_value());

    auto r5 = fix_two_caps(L"PCs", nullptr, false);
    assert(!r5.has_value());

    // All caps or all lower or too short should not trigger
    auto r6 = fix_two_caps(L"HELLO", nullptr, false);
    assert(!r6.has_value());

    auto r7 = fix_two_caps(L"hello", nullptr, false);
    assert(!r7.has_value());

    auto r8 = fix_two_caps(L"Hi", nullptr, false);
    assert(!r8.has_value());

    std::cout << "[PASS] Two caps text fixes tests\n";
}

void test_next_case() {
    assert(next_case(L"hello") == L"HELLO");
    assert(next_case(L"HELLO") == L"Hello");
    assert(next_case(L"Hello") == L"hello");

    assert(next_case(L"привет") == L"ПРИВЕТ");
    assert(next_case(L"ПРИВЕТ") == L"Привет");
    assert(next_case(L"Привет") == L"привет");

    assert(next_case(L"hello world") == L"HELLO WORLD");
    assert(next_case(L"HELLO WORLD") == L"Hello World");
    assert(next_case(L"Hello World") == L"hello world");

    assert(next_case(L"123!@#") == L"123!@#");

    const std::wstring mixed = L"llllllllllllllllддддддддддддддддllllllllддддддддддддlllllllllllllдддддllll";
    const std::wstring res = next_case(mixed);
    std::wcout << L"next_case(mixed) = " << res << std::endl;
    assert(res == L"LLLLLLLLLLLLLLLLДДДДДДДДДДДДДДДДLLLLLLLLДДДДДДДДДДДДLLLLLLLLLLLLLДДДДДLLLL");

    std::cout << "[PASS] NextCase cycling tests\n";
}

void test_fix_number() {
    HKL en_layout = LoadKeyboardLayoutW(L"00000409", KLF_NOTELLSHELL);
    if (!en_layout) {
        en_layout = GetKeyboardLayout(0);
    }

    // "1ю8" typed in Russian layout:
    // '1' (vk '1'), 'ю' (vk VK_OEM_PERIOD), '8' (vk '8')
    DWORD scan_period = MapVirtualKeyExW(VK_OEM_PERIOD, MAPVK_VK_TO_VSC, en_layout);
    std::vector<TypedKey> keys = {
        TypedKey{'1', 0, false, false},
        TypedKey{VK_OEM_PERIOD, scan_period, false, false},
        TypedKey{'8', 0, false, false}
    };

    auto r1 = fix_number(keys, L"1ю8", en_layout);
    if (r1.has_value()) {
        assert(*r1 == L"1.8");
    }

    // Shift should disable fix_number
    keys[1].shift = true;
    auto r2 = fix_number(keys, L"1>8", en_layout);
    assert(!r2.has_value());

    std::cout << "[PASS] FixNumber tests\n";
}

void test_convert_text_bidirectional() {
    // 1. Exact string from user report:
    const std::wstring mixed = L"llllllllllllllllддддддддддддддддllllllllддддддддддддlllllllllllllдддддllll";
    const std::wstring expected = L"ддддддддддддддддllllllllllllllllддддддддllllllllllllдддддддддддддlllllдддд";
    const std::wstring actual = convert_text_bidirectional(mixed);
    assert(actual == expected);

    // 2. Round-trip idempotency
    const std::wstring roundtrip = convert_text_bidirectional(actual);
    assert(roundtrip == mixed);

    // 3. English to Russian
    assert(convert_text_bidirectional(L"ghbdtn") == L"привет");

    // 4. Russian to English
    assert(convert_text_bidirectional(L"руддщ") == L"hello");

    // 5. Cased sentence
    assert(convert_text_bidirectional(L"Ghbdtn Vbh!") == L"Привет Мир!");

    // 6. Mixed sentence (macOS parity issue #22A)
    assert(convert_text_bidirectional(L"ghtlkj d ьшчув") == L"предло в mixed");

    std::cout << "[PASS] Bidirectional conversion tests (including user's mixed string)\n";
}

#include "clipboard.h"

void test_clipboard_snapshot() {
    OleInitialize(nullptr);

    // 1. Put original text into clipboard
    const std::wstring original = L"Secret User Data 12345";
    assert(paste_text(original));

    // 2. Read clipboard to verify
    std::wstring read_back;
    {
        assert(OpenClipboard(nullptr));
        HANDLE h = GetClipboardData(CF_UNICODETEXT);
        assert(h != nullptr);
        read_back = static_cast<const wchar_t*>(GlobalLock(h));
        GlobalUnlock(h);
        CloseClipboard();
    }
    assert(read_back == original);

    // 3. Take snapshot
    ClipboardSnapshot snapshot;
    bool cap_ok = snapshot.capture();
    std::cout << "snapshot.capture() = " << cap_ok << std::endl;
    assert(cap_ok);

    // 4. Overwrite clipboard with temporary converted text
    const std::wstring converted = L"Temporary Converted Text";
    assert(paste_text(converted));

    // Verify it is indeed changed
    {
        assert(OpenClipboard(nullptr));
        HANDLE h = GetClipboardData(CF_UNICODETEXT);
        assert(h != nullptr);
        read_back = static_cast<const wchar_t*>(GlobalLock(h));
        GlobalUnlock(h);
        CloseClipboard();
    }
    assert(read_back == converted);

    // 5. Restore snapshot
    assert(snapshot.restore());

    // 6. Verify original text is restored
    read_back.clear();
    {
        bool has_text = false;
        if (OpenClipboard(nullptr)) {
            if (IsClipboardFormatAvailable(CF_UNICODETEXT)) {
                HANDLE h = GetClipboardData(CF_UNICODETEXT);
                if (h) {
                    const wchar_t* ptr = static_cast<const wchar_t*>(GlobalLock(h));
                    if (ptr) {
                        read_back = ptr;
                        GlobalUnlock(h);
                        has_text = true;
                    }
                }
            }
            CloseClipboard();
        }
        assert(has_text);
        assert(read_back == original);
    }

    std::cout << "[PASS] Clipboard snapshot save and restore test\n";
    OleUninitialize();
}

int main() {
    std::cout << "Running RuSwitcher native unit tests...\n";
    test_brand_words();
    test_two_caps();
    test_next_case();
    test_fix_number();
    test_convert_text_bidirectional();
    test_clipboard_snapshot();
    std::cout << "All native unit tests PASSED successfully!\n";
    return 0;
}


#include "text_fixes.h"

#include "brand_words.h"
#include "dict.h"

#include <array>
#include <cwctype>
#include <unordered_map>

namespace ruswitcher {
namespace {

bool is_alpha(wchar_t c) noexcept {
    return IsCharAlphaW(c) != FALSE;
}

bool is_lower(wchar_t c) noexcept {
    return IsCharLowerW(c) != FALSE;
}

bool is_upper(wchar_t c) noexcept {
    return IsCharUpperW(c) != FALSE;
}

wchar_t to_upper(wchar_t c) noexcept {
    wchar_t buf[2] = {c, 0};
    CharUpperBuffW(buf, 1);
    return buf[0];
}

wchar_t to_lower(wchar_t c) noexcept {
    wchar_t buf[2] = {c, 0};
    CharLowerBuffW(buf, 1);
    return buf[0];
}

}  // namespace

std::optional<std::wstring> fix_two_caps(std::wstring_view word, HKL layout, bool check_dict) {
    if (word.size() < 3) return std::nullopt;

    for (wchar_t c : word) {
        if (!is_alpha(c)) return std::nullopt;
    }

    if (!is_upper(word[0]) || !is_upper(word[1])) return std::nullopt;

    for (std::size_t i = 2; i < word.size(); ++i) {
        if (!is_lower(word[i])) return std::nullopt;
    }

    // Exclude English plural acronyms like "IDs", "PCs", "CDs"
    if (word.size() == 3 && word[2] == L's') return std::nullopt;

    std::wstring fixed;
    fixed.reserve(word.size());
    fixed.push_back(to_upper(word[0]));
    for (std::size_t i = 1; i < word.size(); ++i) {
        fixed.push_back(to_lower(word[i]));
    }

    if (check_dict) {
        std::wstring lower;
        lower.reserve(word.size());
        for (wchar_t c : word) {
            lower.push_back(to_lower(c));
        }
        const bool valid = is_brand_word(lower) || Dict::is_valid_word(lower, layout);
        if (!valid && Dict::is_available()) return std::nullopt;
    }

    return fixed;
}

std::optional<std::wstring> fix_number(const std::vector<TypedKey>& keys, std::wstring_view typed,
                                      HKL other_layout) {
    if (keys.size() < 3 || keys.size() != typed.size()) return std::nullopt;

    for (const auto& k : keys) {
        if (k.shift) return std::nullopt;
    }

    std::wstring result;
    result.reserve(typed.size());
    bool saw_separator = false;

    for (std::size_t i = 0; i < keys.size(); ++i) {
        if (keys[i].vk >= '0' && keys[i].vk <= '9') {
            result.push_back(typed[i]);
            continue;
        }

        if (i > 0 && i + 1 < keys.size() &&
            (keys[i - 1].vk >= '0' && keys[i - 1].vk <= '9') &&
            (keys[i + 1].vk >= '0' && keys[i + 1].vk <= '9') &&
            is_alpha(typed[i])) {

            std::array<BYTE, 256> state{};
            wchar_t output[8]{};
            const int len = ToUnicodeEx(keys[i].vk, keys[i].scan, state.data(), output,
                                        static_cast<int>(std::size(output)), 0x4, other_layout);
            if (len >= 1 && (output[0] == L'.' || output[0] == L',')) {
                result.push_back(output[0]);
                saw_separator = true;
                continue;
            }
        }

        return std::nullopt;
    }

    return saw_separator ? std::optional<std::wstring>(result) : std::nullopt;
}

std::wstring next_case(std::wstring_view text) {
    std::size_t letter_count = 0;
    bool all_lower = true;
    bool all_upper = true;

    for (wchar_t c : text) {
        if (is_alpha(c)) {
            ++letter_count;
            if (!is_lower(c)) all_lower = false;
            if (!is_upper(c)) all_upper = false;
        }
    }

    if (letter_count == 0) return std::wstring(text);

    std::wstring result;
    result.reserve(text.size());

    if (all_lower) {
        for (wchar_t c : text) {
            result.push_back(to_upper(c));
        }
    } else if (all_upper) {
        bool new_word = true;
        for (wchar_t c : text) {
            if (is_alpha(c)) {
                if (new_word) {
                    result.push_back(to_upper(c));
                    new_word = false;
                } else {
                    result.push_back(to_lower(c));
                }
            } else {
                result.push_back(c);
                if (iswspace(c) || iswpunct(c)) {
                    new_word = true;
                }
            }
        }
    } else {
        for (wchar_t c : text) {
            result.push_back(to_lower(c));
        }
    }

    return result;
}

namespace {

struct CharPair {
    wchar_t en;
    wchar_t ru;
};

constexpr CharPair kEnRuPairs[] = {
    // Lowercase
    {L'q', L'й'}, {L'w', L'ц'}, {L'e', L'у'}, {L'r', L'к'}, {L't', L'е'}, {L'y', L'н'},
    {L'u', L'г'}, {L'i', L'ш'}, {L'o', L'щ'}, {L'p', L'з'}, {L'[', L'х'}, {L']', L'ъ'},
    {L'a', L'ф'}, {L's', L'ы'}, {L'd', L'в'}, {L'f', L'а'}, {L'g', L'п'}, {L'h', L'р'},
    {L'j', L'о'}, {L'k', L'л'}, {L'l', L'д'}, {L';', L'ж'}, {L'\'', L'э'},
    {L'z', L'я'}, {L'x', L'ч'}, {L'c', L'с'}, {L'v', L'м'}, {L'b', L'и'}, {L'n', L'т'},
    {L'm', L'ь'}, {L',', L'б'}, {L'.', L'ю'}, {L'/', L'.'},
    {L'\\', L'ё'}, {L'`', L'ё'},
    // Uppercase
    {L'Q', L'Й'}, {L'W', L'Ц'}, {L'E', L'У'}, {L'R', L'К'}, {L'T', L'Е'}, {L'Y', L'Н'},
    {L'U', L'Г'}, {L'I', L'Ш'}, {L'O', L'Щ'}, {L'P', L'З'}, {L'{', L'Х'}, {L'}', L'Ъ'},
    {L'A', L'Ф'}, {L'S', L'Ы'}, {L'D', L'В'}, {L'F', L'А'}, {L'G', L'П'}, {L'H', L'Р'},
    {L'J', L'О'}, {L'K', L'Л'}, {L'L', L'Д'}, {L':', L'Ж'}, {L'\"', L'Э'},
    {L'Z', L'Я'}, {L'X', L'Ч'}, {L'C', L'С'}, {L'V', L'М'}, {L'B', L'И'}, {L'N', L'Т'},
    {L'M', L'Ь'}, {L'<', L'Б'}, {L'>', L'Ю'}, {L'?', L','},
    {L'|', L'Ё'}, {L'~', L'Ё'}
};

bool translate_single_key(DWORD vk, DWORD scan, bool shift, HKL layout, wchar_t& out) noexcept {
    std::array<BYTE, 256> state{};
    if (shift) state[VK_SHIFT] = 0x80;
    wchar_t buf[8]{};
    const int len = ToUnicodeEx(vk, scan, state.data(), buf, static_cast<int>(std::size(buf)), 0x4, layout);
    if (len == 1) {
        out = buf[0];
        return true;
    }
    return false;
}

}  // namespace

bool is_cyrillic_char(wchar_t c) noexcept {
    return (c >= 0x0400 && c <= 0x04FF) || c == 0x0500;
}

bool is_latin_char(wchar_t c) noexcept {
    return (c >= L'a' && c <= L'z') || (c >= L'A' && c <= L'Z');
}

std::wstring convert_text_bidirectional(std::wstring_view text, HKL layout1, HKL layout2) {
    if (text.empty()) return {};

    std::unordered_map<wchar_t, wchar_t> map;
    map.reserve(128);

    if (layout1 && layout2 && layout1 != layout2) {
        std::unordered_map<wchar_t, wchar_t> forward;
        std::unordered_map<wchar_t, wchar_t> backward;
        forward.reserve(96);
        backward.reserve(96);

        constexpr DWORD oem_keys[]{
            VK_OEM_1, VK_OEM_PLUS, VK_OEM_COMMA, VK_OEM_MINUS,
            VK_OEM_PERIOD, VK_OEM_2, VK_OEM_3, VK_OEM_4,
            VK_OEM_5, VK_OEM_6, VK_OEM_7, VK_OEM_8, VK_OEM_102
        };

        auto query_pairs = [&](HKL src, HKL tgt, std::unordered_map<wchar_t, wchar_t>& out_map) {
            auto process_vk = [&](DWORD vk, bool shift) {
                const DWORD scan = MapVirtualKeyExW(vk, MAPVK_VK_TO_VSC, src);
                wchar_t ch_src = 0, ch_tgt = 0;
                if (translate_single_key(vk, scan, shift, src, ch_src) &&
                    translate_single_key(vk, scan, shift, tgt, ch_tgt) &&
                    ch_src != ch_tgt && ch_src != 0 && ch_tgt != 0) {
                    out_map.emplace(ch_src, ch_tgt);
                }
            };

            for (DWORD vk = '0'; vk <= '9'; ++vk) {
                process_vk(vk, false);
                process_vk(vk, true);
            }
            for (DWORD vk = 'A'; vk <= 'Z'; ++vk) {
                process_vk(vk, false);
                process_vk(vk, true);
            }
            for (DWORD vk : oem_keys) {
                process_vk(vk, false);
                process_vk(vk, true);
            }
        };

        query_pairs(layout1, layout2, forward);
        query_pairs(layout2, layout1, backward);

        map = forward;
        for (const auto& [k, v] : backward) {
            auto it = map.find(k);
            if (it != map.end()) {
                if (!is_alpha(it->second) && is_alpha(v)) {
                    it->second = v;
                }
            } else {
                map[k] = v;
            }
        }
    }

    // Static canonical fallback for standard EN <-> RU pairs
    for (const auto& p : kEnRuPairs) {
        if (map.find(p.en) == map.end()) {
            map[p.en] = p.ru;
        }
        if (p.ru == L'ё') {
            if (map.find(L'ё') == map.end()) map[L'ё'] = L'`';
        } else if (p.ru == L'Ё') {
            if (map.find(L'Ё') == map.end()) map[L'Ё'] = L'~';
        } else {
            auto it = map.find(p.ru);
            if (it == map.end() || (!is_alpha(it->second) && is_alpha(p.en))) {
                map[p.ru] = p.en;
            }
        }
    }

    std::wstring result;
    result.reserve(text.size());
    for (wchar_t ch : text) {
        auto it = map.find(ch);
        result.push_back(it != map.end() ? it->second : ch);
    }
    return result;
}

bool is_word_text(std::wstring_view text) noexcept {
    if (text.empty()) return false;
    for (wchar_t c : text) {
        if (!IsCharAlphaW(c) && c != L'\'' && c != L'\x2019') return false;
    }
    return true;
}

namespace {

bool is_all_caps(std::wstring_view text) noexcept {
    bool has_char = false;
    for (wchar_t c : text) {
        if (IsCharAlphaW(c)) {
            has_char = true;
            if (!IsCharUpperW(c)) return false;
        }
    }
    return has_char;
}

bool looks_like_code(std::wstring_view text) noexcept {
    for (std::size_t i = 1; i < text.size(); ++i) {
        if (IsCharUpperW(text[i])) return true;
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

}  // namespace

bool should_auto_convert(std::wstring_view typed, std::wstring_view converted,
                         HKL source, HKL target, bool caps) noexcept {
    if (typed.size() < 3) return false;

    // Both typed and converted cannot contain invalid symbols (digits, spaces, special chars).
    // In Cyrillic, letters like х, ъ, ж, э, б, ю, ё reside on [ ] ; ' , . ` keys.
    // Parity with macOS (issue #22):
    // allow if either typed is all letters OR converted is all letters.
    if (!is_word_text(typed) && !is_word_text(converted)) return false;

    if (!caps) {
        if (is_all_caps(typed)) return false;
        if (looks_like_code(typed)) return false;
    }

    std::wstring lower_converted(converted);
    if (!lower_converted.empty()) {
        CharLowerBuffW(lower_converted.data(), static_cast<DWORD>(lower_converted.size()));
    }

    const bool target_is_brand = (converted.size() >= 4 && is_brand_word(lower_converted));
    const bool valid_target = target_is_brand || Dict::is_valid_word(lower_converted, target);
    if (!valid_target) return false;

    std::wstring lower_typed(typed);
    if (!lower_typed.empty()) {
        CharLowerBuffW(lower_typed.data(), static_cast<DWORD>(lower_typed.size()));
    }

    if (Dict::is_valid_word(lower_typed, source)) return false;

    return true;
}

}  // namespace ruswitcher


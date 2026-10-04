#include "text_fixes.h"

#include "brand_words.h"
#include "dict.h"

#include <array>
#include <cwctype>

namespace ruswitcher {

std::optional<std::wstring> fix_two_caps(std::wstring_view word, HKL layout, bool check_dict) {
    if (word.size() < 3) return std::nullopt;

    for (wchar_t c : word) {
        if (!iswalpha(c)) return std::nullopt;
    }

    if (!iswupper(word[0]) || !iswupper(word[1])) return std::nullopt;

    for (std::size_t i = 2; i < word.size(); ++i) {
        if (!iswlower(word[i])) return std::nullopt;
    }

    // Exclude English plural acronyms like "IDs", "PCs", "CDs"
    if (word.size() == 3 && word[2] == L's') return std::nullopt;

    std::wstring fixed;
    fixed.reserve(word.size());
    fixed.push_back(static_cast<wchar_t>(towupper(word[0])));
    for (std::size_t i = 1; i < word.size(); ++i) {
        fixed.push_back(static_cast<wchar_t>(towlower(word[i])));
    }

    if (check_dict) {
        std::wstring lower;
        lower.reserve(word.size());
        for (wchar_t c : word) {
            lower.push_back(static_cast<wchar_t>(towlower(c)));
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
            iswalpha(typed[i])) {

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
        if (iswalpha(c)) {
            ++letter_count;
            if (!iswlower(c)) all_lower = false;
            if (!iswupper(c)) all_upper = false;
        }
    }

    if (letter_count == 0) return std::wstring(text);

    std::wstring result;
    result.reserve(text.size());

    if (all_lower) {
        for (wchar_t c : text) {
            result.push_back(static_cast<wchar_t>(towupper(c)));
        }
    } else if (all_upper) {
        bool new_word = true;
        for (wchar_t c : text) {
            if (iswalpha(c)) {
                if (new_word) {
                    result.push_back(static_cast<wchar_t>(towupper(c)));
                    new_word = false;
                } else {
                    result.push_back(static_cast<wchar_t>(towlower(c)));
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
            result.push_back(static_cast<wchar_t>(towlower(c)));
        }
    }

    return result;
}

}  // namespace ruswitcher

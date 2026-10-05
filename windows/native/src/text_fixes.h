#pragma once

#include <windows.h>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace ruswitcher {

struct TypedKey {
    DWORD vk{};
    DWORD scan{};
    bool shift{};
    bool caps{};
};

// Fixes two initial capital letters: "КОгда" -> "Когда", "ПРивет" -> "Привет", "TWo" -> "Two".
// Only triggers if word has at least 3 letters, first two are uppercase, rest are lowercase,
// and the lowercased candidate is confirmed as a valid word by the dictionary or brand list.
// Excludes plural acronyms like "IDs", "PCs".
std::optional<std::wstring> fix_two_caps(std::wstring_view word, HKL layout, bool check_dict = true);

// Fixes numbers typed with dot/comma in wrong layout: "1ю8" -> "1.8", "5б2" -> "5,2".
// Checks if punctuation between digits was typed as letter in current layout but translates
// to '.' or ',' in the other layout.
std::optional<std::wstring> fix_number(const std::vector<TypedKey>& keys, std::wstring_view typed, HKL other_layout);

// Cycles the case of text: lower -> UPPER -> Title -> lower.
std::wstring next_case(std::wstring_view text);

// Checks if character is in Cyrillic script.
bool is_cyrillic_char(wchar_t c) noexcept;

// Checks if character is in Latin script.
bool is_latin_char(wchar_t c) noexcept;

// Checks whether string consists entirely of letters (or apostrophe/single quote).
bool is_word_text(std::wstring_view text) noexcept;

// Determines whether typed word should be auto-converted to converted word.
// Checks length (>= 3), valid word/brand in target dictionary, invalid in source dictionary,
// acronym/caps/code exclusion, and allows punctuation keys for Cyrillic letters (e.g. '[', ']', ';', etc.).
bool should_auto_convert(std::wstring_view typed, std::wstring_view converted,
                         HKL source, HKL target, bool caps = false) noexcept;

// Bidirectionally converts text between two layouts (Layout 1 <-> Layout 2) in a single pass.
// Each Latin character is converted to Cyrillic, and each Cyrillic character is converted to Latin.
// If layouts are null, falls back to the canonical EN (QWERTY) <-> RU (ЙЦУКЕН) mapping.
std::wstring convert_text_bidirectional(std::wstring_view text, HKL layout1 = nullptr, HKL layout2 = nullptr);

}  // namespace ruswitcher


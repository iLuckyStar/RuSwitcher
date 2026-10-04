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

}  // namespace ruswitcher

#pragma once

#include <windows.h>
#include <string_view>

namespace ruswitcher {

class Dict {
public:
    // Checks if the word is valid in the language associated with the layout.
    // Falls back gracefully (returns false) if Windows Spell Checking API is unavailable.
    static bool is_valid_word(std::wstring_view word, HKL layout) noexcept;
    static bool is_valid_word(std::wstring_view word, const wchar_t* lang_tag) noexcept;
    static const wchar_t* get_lang_tag(HKL layout) noexcept;
    static bool is_available() noexcept;
};

}  // namespace ruswitcher

#pragma once

#include <windows.h>
#include <cstddef>
#include <cstdint>
#include <vector>

#include "text_fixes.h"

namespace ruswitcher {

enum class ConversionScope : uint32_t {
    Word = 0,         // Последнее слово / выделенный текст
    Phrase = 1,       // Набранная строка / фраза (Punto Switcher)
    SystemLine = 2    // Вся строка целиком от начала (Home → End)
};

enum class TriggerAction {
    Reconvert,
    BufferedWord,
    BufferedLine,
    SelectedText,
    SystemLine,
};

inline bool is_word_boundary(DWORD vk) noexcept {
    return vk == VK_SPACE || vk == VK_RETURN || vk == VK_TAB || vk == VK_ESCAPE;
}

inline bool is_typing_key(DWORD vk) noexcept {
    return (vk >= '0' && vk <= '9') || (vk >= 'A' && vk <= 'Z') ||
           (vk >= VK_OEM_1 && vk <= VK_OEM_3) ||
           (vk >= VK_OEM_4 && vk <= VK_OEM_8) || vk == VK_OEM_102;
}

inline bool invalidates_buffer(DWORD vk) noexcept {
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

/// <summary>
/// Continuous keystroke buffer matching the canonical implementation in
/// windows/src/RuSwitcher.Win/Core/KeystrokeBuffer.cs and macOS KeyboardMonitor.
/// Keeps both active word and continuous line/phrase buffer across spaces.
/// </summary>
class KeystrokeBuffer {
public:
    const std::vector<TypedKey>& current_word() const noexcept { return current_; }
    const std::vector<TypedKey>& current_line() const noexcept { return line_; }
    std::vector<TypedKey>& current_line() noexcept { return line_; }
    bool is_empty() const noexcept { return current_.empty(); }
    bool is_line_empty() const noexcept { return line_.empty(); }

    void append(const TypedKey& key) {
        current_.push_back(key);
        line_.push_back(key);
    }

    /// <summary>Keep typed space in line buffer while starting new word.</summary>
    void append_space(const TypedKey& key) {
        line_.push_back(key);
        current_.clear();
    }

    void backspace() {
        if (line_.empty()) {
            if (!current_.empty()) current_.pop_back();
            return;
        }
        line_.pop_back();
        rebuild_current_word();
    }

    void reset() noexcept {
        current_.clear();
        line_.clear();
    }

    void clear_word() noexcept {
        current_.clear();
    }

    void clear_all() noexcept {
        reset();
    }

    void rebuild_current_word() noexcept {
        current_.clear();
        std::size_t start = line_.size();
        while (start > 0 && line_[start - 1].vk != VK_SPACE) {
            --start;
        }
        for (std::size_t i = start; i < line_.size(); ++i) {
            current_.push_back(line_[i]);
        }
    }

private:
    std::vector<TypedKey> current_;
    std::vector<TypedKey> line_;
};

/// <summary>
/// Factory parity decision engine matching windows/src/RuSwitcher.Win/Core/TriggerRouting.cs.
/// </summary>
inline TriggerAction decide_trigger_action(ConversionScope scope, bool can_reconvert,
                                          std::size_t word_keys, std::size_t line_keys) noexcept {
    if (can_reconvert && word_keys == 0 && line_keys == 0) {
        return TriggerAction::Reconvert;
    }
    if (scope == ConversionScope::Phrase && line_keys > 0) {
        return TriggerAction::BufferedLine;
    }
    if (scope == ConversionScope::SystemLine) {
        if (line_keys > 0) return TriggerAction::BufferedLine;
        return TriggerAction::SystemLine;
    }
    if (scope == ConversionScope::Word && word_keys > 0) {
        return TriggerAction::BufferedWord;
    }
    return (scope == ConversionScope::SystemLine) ? TriggerAction::SystemLine : TriggerAction::SelectedText;
}

}  // namespace ruswitcher

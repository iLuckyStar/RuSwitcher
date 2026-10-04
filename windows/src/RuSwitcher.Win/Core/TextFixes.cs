namespace RuSwitcher.Win.Core;

/// <summary>
/// Text correction at the end of a word (macOS 3.5.0b parity):
/// - Two initial capital letters: "ПРивет" -> "Привет", "TWo" -> "Two"
/// - Number punctuation errors: "1ю8" -> "1.8", "5б2" -> "5,2"
/// Pure functions for high testability.
/// </summary>
internal static class TextFixes
{
    private static readonly HashSet<uint> DigitVkCodes = new()
    {
        0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39 // '0'..'9'
    };

    /// <summary>
    /// Fixes two initial capital letters: "КОгда" -> "Когда", "ПРивет" -> "Привет".
    /// Only triggers if word has at least 3 letters, first two are uppercase, rest are lowercase,
    /// and the lowercased candidate is confirmed as a valid word by the dictionary.
    /// Excludes plural acronyms like "IDs", "PCs".
    /// </summary>
    public static string? FixTwoCaps(string word, Func<string, bool> isWord)
    {
        if (string.IsNullOrEmpty(word) || word.Length < 3) return null;
        if (!word.All(char.IsLetter)) return null;

        if (!char.IsUpper(word[0]) || !char.IsUpper(word[1])) return null;
        for (int i = 2; i < word.Length; i++)
        {
            if (!char.IsLower(word[i])) return null;
        }

        // Exclude English plural acronyms like "IDs", "PCs", "CDs"
        if (word.Length == 3 && word[2] == 's') return null;

        string fixedWord = char.ToUpperInvariant(word[0]) + word.Substring(1).ToLowerInvariant();
        return isWord(fixedWord.ToLowerInvariant()) ? fixedWord : null;
    }

    /// <summary>
    /// Fixes numbers typed with dot/comma in wrong layout: "1ю8" -> "1.8", "5б2" -> "5,2".
    /// </summary>
    public static string? FixNumber(IReadOnlyList<TypedKey> keys, string typed, Func<uint, char?> otherChar)
    {
        if (keys.Count < 3 || keys.Count != typed.Length) return null;
        if (keys.Any(k => k.Shift)) return null; // Shift+digit produces symbols, not plain numbers

        var sb = new System.Text.StringBuilder(typed.Length);
        bool sawSeparator = false;

        for (int i = 0; i < keys.Count; i++)
        {
            if (DigitVkCodes.Contains(keys[i].VkCode))
            {
                sb.Append(typed[i]);
                continue;
            }

            // Separator must be between digits
            if (i > 0 && i < keys.Count - 1 &&
                DigitVkCodes.Contains(keys[i - 1].VkCode) &&
                DigitVkCodes.Contains(keys[i + 1].VkCode) &&
                char.IsLetter(typed[i]))
            {
                char? o = otherChar(keys[i].VkCode);
                if (o is '.' or ',')
                {
                    sb.Append(o.Value);
                    sawSeparator = true;
                    continue;
                }
            }

            return null; // Unexpected character in number
        }

        return sawSeparator ? sb.ToString() : null;
    }
}

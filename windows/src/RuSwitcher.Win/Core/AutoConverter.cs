using System.Linq;

namespace RuSwitcher.Win.Core;

/// <summary>
/// As-you-type auto conversion and word-end pipeline — the Windows counterpart of the macOS
/// LayoutDetector.decide + AutoSwitch + WordEndPipeline (macOS 3.5.0b parity).
/// On a word boundary (Space, Enter, Tab) it evaluates the typed word:
/// 1. FixTwoCaps (e.g. "ПРивет" -> "Привет")
/// 2. FixNumber (e.g. "1ю8" -> "1.8")
/// 3. Layout conversion (with brand-name support and dictionary vetoes).
/// </summary>
internal static class AutoConverter
{
    private static string BoundaryString(uint vk) => vk switch
    {
        KeystrokeBuffer.VK_RETURN => "\r\n",
        KeystrokeBuffer.VK_TAB => "\t",
        _ => " "
    };

    /// <summary>
    /// Executes the word-end pipeline for the completed word.
    /// Runs on the message loop after the boundary key has been delivered.
    /// </summary>
    public static bool TryConvertWord(IReadOnlyList<TypedKey> keys, uint boundaryVk = KeystrokeBuffer.VK_SPACE)
    {
        if (keys.Count == 0) return false;
        var settings = Settings.Current;

        IntPtr sourceHkl = LayoutSwitcher.Current();
        if (LayoutSwitcher.Opposite() is not { } targetHkl) return false;

        string typed = KeyMapper.ConvertWord(keys, sourceHkl);
        string srcTag = SmartConvert.LangTag(sourceHkl);
        string tgtTag = SmartConvert.LangTag(targetHkl);
        string boundary = BoundaryString(boundaryVk);

        // 1. TextFixes: Two initial capitals (e.g. "ПРивет" -> "Привет", "TWo" -> "Two")
        if (settings.FixTwoCaps)
        {
            string? twoCaps = TextFixes.FixTwoCaps(typed, w => Dict.IsValidWord(w, srcTag));
            if (twoCaps != null && twoCaps != typed)
            {
                if (TextInjector.Replace(backspaces: keys.Count + 1, text: twoCaps + boundary))
                {
                    return true;
                }
            }
        }

        // 2. TextFixes: Misplaced dot/comma in numbers (e.g. "1ю8" -> "1.8")
        if (settings.FixNumbers)
        {
            string? num = TextFixes.FixNumber(keys, typed, vk => KeyMapper.TranslateIn(new TypedKey(vk, 0, false, false), targetHkl));
            if (num != null && num != typed)
            {
                if (TextInjector.Replace(backspaces: keys.Count + 1, text: num + boundary))
                {
                    return true;
                }
            }
        }

        // 3. Layout Auto-Conversion
        if (settings.AutoConvert)
        {
            string converted = KeyMapper.ConvertWord(keys, targetHkl);
            if (converted.Length > 0 && converted != typed)
            {
                bool caps = keys.All(k => k.Caps);
                if (ShouldConvert(typed, converted, srcTag, tgtTag, caps))
                {
                    // Apply two-caps fix to converted target if applicable
                    string finalConverted = settings.FixTwoCaps
                        ? (TextFixes.FixTwoCaps(converted, w => Dict.IsValidWord(w, tgtTag) || BrandWords.All.Contains(w.ToLowerInvariant())) ?? converted)
                        : converted;

                    if (TextInjector.Replace(backspaces: keys.Count + 1, text: finalConverted + boundary))
                    {
                        LayoutSwitcher.SwitchTo(targetHkl);
                        Converter.NoteAutoConversion(finalConverted, typed, targetHkl, sourceHkl);
                        return true;
                    }
                }
            }
        }

        return false;
    }

    private static bool ShouldConvert(string typed, string converted, string srcTag, string tgtTag, bool caps) =>
        ShouldConvertPure(typed, converted, srcTag, tgtTag, caps,
            Dict.Available, Dict.IsValidWord,
            Settings.Current.NeverConvert, Settings.Current.AlwaysConvert);

    /// <summary>
    /// Pure decision logic, ported from macOS LayoutDetector + BrandWords (issue #34, #35).
    /// </summary>
    internal static bool ShouldConvertPure(string typed, string converted, string srcTag, string tgtTag,
        bool caps, bool dictAvailable, Func<string, string, bool> isValidWord,
        ICollection<string> never, ICollection<string> always)
    {
        string typedLc = typed.ToLowerInvariant();
        string convertedLc = converted.ToLowerInvariant();

        // Explicit user overrides
        if (always.Contains(convertedLc)) return true;
        if (never.Contains(typedLc)) return false;

        // Vetoes
        if (typed.Length < 3) return false;
        // Allow apostrophes in words
        if (!typed.All(c => char.IsLetter(c) || c is '\'' or '’')) return false;

        if (!caps)
        {
            if (IsAllCaps(typed)) return false;
            if (LooksLikeCode(typed)) return false;
        }

        // Brand name target check (issue #34)
        bool targetIsBrand = converted.Length >= 4 && BrandWords.All.Contains(convertedLc);

        // Hebrew cross-script check
        bool srcHe = srcTag == "he", tgtHe = tgtTag == "he";
        if (srcHe || tgtHe)
        {
            string sideTag = srcHe ? tgtTag : srcTag;
            if (sideTag == "he" || !dictAvailable) return false;
            if (srcHe)
            {
                if (!converted.All(char.IsLetter)) return false;
                return isValidWord(convertedLc, sideTag) || targetIsBrand;
            }
            return false;
        }

        // Common layout swap
        if (!dictAvailable && !targetIsBrand) return false;
        bool validTarget = targetIsBrand || isValidWord(convertedLc, tgtTag);
        if (!validTarget) return false;
        if (isValidWord(typedLc, srcTag)) return false;

        return true;
    }

    private static bool IsAllCaps(string s) =>
        s == s.ToUpperInvariant() && s != s.ToLowerInvariant();

    private static bool LooksLikeCode(string s)
    {
        for (int i = 1; i < s.Length; i++)
            if (char.IsUpper(s[i])) return true;

        bool latin = false, cyr = false;
        foreach (char c in s)
        {
            if (c is >= 'a' and <= 'z' or >= 'A' and <= 'Z') latin = true;
            else if (c is >= 'Ѐ' and <= 'ӿ') cyr = true;
        }
        return latin && cyr;
    }
}

using RuSwitcher.Win.Core;
using Xunit;

namespace RuSwitcher.Win.Tests;

public class TextFixesTests
{
    [Theory]
    [InlineData("ПРивет", "Привет")]
    [InlineData("КОгда", "Когда")]
    [InlineData("TWo", "Two")]
    [InlineData("HEllo", "Hello")]
    public void FixTwoCaps_fixes_two_initial_capitals(string input, string expected)
    {
        // Mock dictionary that accepts common words
        string? result = TextFixes.FixTwoCaps(input, word => true);
        Assert.Equal(expected, result);
    }

    [Theory]
    [InlineData("IDs")]
    [InlineData("PCs")]
    [InlineData("CDs")]
    [InlineData("IT")]
    [InlineData("AI")]
    [InlineData("API")]
    [InlineData("camelCase")]
    [InlineData("Normal")]
    [InlineData("alllower")]
    public void FixTwoCaps_ignores_acronyms_and_normal_words(string input)
    {
        string? result = TextFixes.FixTwoCaps(input, word => true);
        Assert.Null(result);
    }

    [Fact]
    public void FixTwoCaps_rejects_word_not_in_dictionary()
    {
        // If dictionary rejects the lowercased word, fix should not apply
        string? result = TextFixes.FixTwoCaps("ХЗчто", word => false);
        Assert.Null(result);
    }

    [Fact]
    public void FixNumber_fixes_misplaced_decimal_separators()
    {
        // 1ю8: key 1, key ю (which is . in US), key 8
        var keys = new TypedKey[]
        {
            new(0x31, 2, false, false), // '1'
            new(0xBE, 52, false, false), // OEM_PERIOD / 'ю'
            new(0x38, 9, false, false), // '8'
        };

        string? result = TextFixes.FixNumber(keys, "1ю8", vk => vk == 0xBE ? '.' : null);
        Assert.Equal("1.8", result);
    }
}

using RuSwitcher.Win.Core;
using Xunit;

namespace RuSwitcher.Win.Tests;

public class BrandWordsTests
{
    [Theory]
    [InlineData("chatgpt")]
    [InlineData("docker")]
    [InlineData("github")]
    [InlineData("kubernetes")]
    [InlineData("vscode")]
    [InlineData("claude")]
    [InlineData("gemini")]
    public void BrandWords_contains_essential_tech_brands(string brand)
    {
        Assert.Contains(brand, BrandWords.All);
    }

    [Fact]
    public void ShouldConvertPure_accepts_brand_even_if_system_dict_missing()
    {
        // "срфепзе" -> "chatgpt": system dictionary returns false, but BrandWords confirms target
        bool convert = AutoConverter.ShouldConvertPure(
            typed: "срфепзе",
            converted: "chatgpt",
            srcTag: "ru",
            tgtTag: "en",
            caps: false,
            dictAvailable: false,
            isValidWord: (_, _) => false,
            never: new HashSet<string>(),
            always: new HashSet<string>());

        Assert.True(convert);
    }
}

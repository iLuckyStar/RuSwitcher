using RuSwitcher.Win.Core;
using Xunit;

namespace RuSwitcher.Win.Tests;

public class ChangeCaseTests
{
    [Theory]
    [InlineData("hello", "HELLO")]
    [InlineData("HELLO", "Hello")]
    [InlineData("Hello", "hello")]
    [InlineData("привет", "ПРИВЕТ")]
    [InlineData("ПРИВЕТ", "Привет")]
    [InlineData("Привет", "привет")]
    public void NextCase_cycles_lower_upper_title(string input, string expected)
    {
        string result = Converter.NextCase(input);
        Assert.Equal(expected, result);
    }

    [Fact]
    public void NextCase_handles_words_with_punctuation()
    {
        string step1 = Converter.NextCase("привет, мир!");
        Assert.Equal("ПРИВЕТ, МИР!", step1);

        string step2 = Converter.NextCase(step1);
        Assert.Equal("Привет, Мир!", step2);

        string step3 = Converter.NextCase(step2);
        Assert.Equal("привет, мир!", step3);
    }
}

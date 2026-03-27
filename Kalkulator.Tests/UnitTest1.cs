namespace Kalkulator.Tests;

public class CalculatorTests
{
    private readonly Calculator _calc = new();
    private const double Tolerance = 1e-9;

    // Basic arithmetic
    [Theory]
    [InlineData(2, 3, 5)]
    [InlineData(-1, 1, 0)]
    [InlineData(0, 0, 0)]
    [InlineData(1.5, 2.5, 4.0)]
    public void Add_ReturnsCorrectSum(double a, double b, double expected)
    {
        Assert.Equal(expected, _calc.Add(a, b), precision: 10);
    }

    [Theory]
    [InlineData(5, 3, 2)]
    [InlineData(0, 5, -5)]
    [InlineData(-3, -3, 0)]
    public void Subtract_ReturnsCorrectDifference(double a, double b, double expected)
    {
        Assert.Equal(expected, _calc.Subtract(a, b), precision: 10);
    }

    [Theory]
    [InlineData(3, 4, 12)]
    [InlineData(-2, 5, -10)]
    [InlineData(0, 100, 0)]
    [InlineData(1.5, 2, 3.0)]
    public void Multiply_ReturnsCorrectProduct(double a, double b, double expected)
    {
        Assert.Equal(expected, _calc.Multiply(a, b), precision: 10);
    }

    [Theory]
    [InlineData(10, 2, 5)]
    [InlineData(-6, 3, -2)]
    [InlineData(1, 3, 1.0 / 3.0)]
    public void Divide_ReturnsCorrectQuotient(double a, double b, double expected)
    {
        Assert.Equal(expected, _calc.Divide(a, b), Tolerance);
    }

    [Fact]
    public void Divide_ByZero_ThrowsDivideByZeroException()
    {
        Assert.Throws<DivideByZeroException>(() => _calc.Divide(10, 0));
    }

    [Theory]
    [InlineData(10, 3, 1)]
    [InlineData(7, 2, 1)]
    [InlineData(9, 3, 0)]
    public void Modulo_ReturnsCorrectRemainder(double a, double b, double expected)
    {
        Assert.Equal(expected, _calc.Modulo(a, b), Tolerance);
    }

    [Fact]
    public void Modulo_ByZero_ThrowsDivideByZeroException()
    {
        Assert.Throws<DivideByZeroException>(() => _calc.Modulo(5, 0));
    }

    // Power and roots
    [Theory]
    [InlineData(2, 10, 1024)]
    [InlineData(3, 3, 27)]
    [InlineData(5, 0, 1)]
    [InlineData(4, 0.5, 2)]
    public void Power_ReturnsCorrectResult(double b, double exp, double expected)
    {
        Assert.Equal(expected, _calc.Power(b, exp), Tolerance);
    }

    [Theory]
    [InlineData(4, 2)]
    [InlineData(9, 3)]
    [InlineData(0, 0)]
    [InlineData(2, 1.4142135623730951)]
    public void SquareRoot_ReturnsCorrectResult(double value, double expected)
    {
        Assert.Equal(expected, _calc.SquareRoot(value), Tolerance);
    }

    [Fact]
    public void SquareRoot_NegativeInput_ThrowsArgumentException()
    {
        Assert.Throws<ArgumentException>(() => _calc.SquareRoot(-1));
    }

    [Fact]
    public void NthRoot_ReturnsCorrectResult()
    {
        Assert.Equal(2.0, _calc.NthRoot(8, 3), Tolerance);
    }

    [Fact]
    public void NthRoot_ZeroDegree_ThrowsArgumentException()
    {
        Assert.Throws<ArgumentException>(() => _calc.NthRoot(8, 0));
    }

    // Absolute value
    [Theory]
    [InlineData(-5, 5)]
    [InlineData(5, 5)]
    [InlineData(0, 0)]
    public void Absolute_ReturnsAbsoluteValue(double value, double expected)
    {
        Assert.Equal(expected, _calc.Absolute(value), Tolerance);
    }

    // Factorial
    [Theory]
    [InlineData(0, 1)]
    [InlineData(1, 1)]
    [InlineData(5, 120)]
    [InlineData(10, 3628800)]
    public void Factorial_ReturnsCorrectResult(int n, double expected)
    {
        Assert.Equal(expected, _calc.Factorial(n), Tolerance);
    }

    [Fact]
    public void Factorial_NegativeInput_ThrowsArgumentException()
    {
        Assert.Throws<ArgumentException>(() => _calc.Factorial(-1));
    }

    [Fact]
    public void Factorial_TooLargeInput_ThrowsArgumentException()
    {
        Assert.Throws<ArgumentException>(() => _calc.Factorial(21));
    }

    // Percent
    [Fact]
    public void Percent_ReturnsCorrectValue()
    {
        Assert.Equal(0.5, _calc.Percent(50), Tolerance);
    }

    [Fact]
    public void PercentOf_ReturnsCorrectValue()
    {
        Assert.Equal(25, _calc.PercentOf(25, 100), Tolerance);
        Assert.Equal(50, _calc.PercentOf(50, 100), Tolerance);
    }

    // Trigonometry
    [Theory]
    [InlineData(0, 0)]
    [InlineData(90, 1)]
    [InlineData(30, 0.5)]
    public void Sin_ReturnsCorrectResult(double degrees, double expected)
    {
        Assert.Equal(expected, _calc.Sin(degrees), Tolerance);
    }

    [Theory]
    [InlineData(0, 1)]
    [InlineData(90, 0)]
    [InlineData(60, 0.5)]
    public void Cos_ReturnsCorrectResult(double degrees, double expected)
    {
        Assert.Equal(expected, _calc.Cos(degrees), Tolerance);
    }

    [Theory]
    [InlineData(0, 0)]
    [InlineData(45, 1)]
    public void Tan_ReturnsCorrectResult(double degrees, double expected)
    {
        Assert.Equal(expected, _calc.Tan(degrees), Tolerance);
    }

    [Fact]
    public void Asin_ReturnsCorrectDegrees()
    {
        Assert.Equal(90.0, _calc.Asin(1), Tolerance);
        Assert.Equal(30.0, _calc.Asin(0.5), Tolerance);
    }

    [Fact]
    public void Asin_OutOfRange_ThrowsArgumentException()
    {
        Assert.Throws<ArgumentException>(() => _calc.Asin(2));
    }

    [Fact]
    public void Acos_ReturnsCorrectDegrees()
    {
        Assert.Equal(0.0, _calc.Acos(1), Tolerance);
        Assert.Equal(60.0, _calc.Acos(0.5), Tolerance);
    }

    [Fact]
    public void Atan_ReturnsCorrectDegrees()
    {
        Assert.Equal(45.0, _calc.Atan(1), Tolerance);
        Assert.Equal(0.0, _calc.Atan(0), Tolerance);
    }

    // Logarithms
    [Fact]
    public void Log10_ReturnsCorrectResult()
    {
        Assert.Equal(2.0, _calc.Log10(100), Tolerance);
        Assert.Equal(0.0, _calc.Log10(1), Tolerance);
    }

    [Fact]
    public void Log10_NonPositiveInput_ThrowsArgumentException()
    {
        Assert.Throws<ArgumentException>(() => _calc.Log10(0));
        Assert.Throws<ArgumentException>(() => _calc.Log10(-1));
    }

    [Fact]
    public void Ln_ReturnsCorrectResult()
    {
        Assert.Equal(1.0, _calc.Ln(Math.E), Tolerance);
        Assert.Equal(0.0, _calc.Ln(1), Tolerance);
    }

    [Fact]
    public void Ln_NonPositiveInput_ThrowsArgumentException()
    {
        Assert.Throws<ArgumentException>(() => _calc.Ln(0));
    }

    [Fact]
    public void LogBase_ReturnsCorrectResult()
    {
        Assert.Equal(3.0, _calc.LogBase(8, 2), Tolerance);
    }

    [Fact]
    public void Exp_ReturnsCorrectResult()
    {
        Assert.Equal(Math.E, _calc.Exp(1), Tolerance);
        Assert.Equal(1.0, _calc.Exp(0), Tolerance);
    }

    // Memory
    [Fact]
    public void Memory_StoreAndRecall_Works()
    {
        _calc.MemoryStore(42);
        Assert.Equal(42, _calc.MemoryRecall(), Tolerance);
    }

    [Fact]
    public void Memory_Clear_ResetsToZero()
    {
        _calc.MemoryStore(99);
        _calc.MemoryClear();
        Assert.Equal(0, _calc.MemoryRecall(), Tolerance);
    }

    [Fact]
    public void Memory_Add_AddsToMemory()
    {
        _calc.MemoryStore(10);
        _calc.MemoryAdd(5);
        Assert.Equal(15, _calc.MemoryRecall(), Tolerance);
    }

    [Fact]
    public void Memory_Subtract_SubtractsFromMemory()
    {
        _calc.MemoryStore(10);
        _calc.MemorySubtract(3);
        Assert.Equal(7, _calc.MemoryRecall(), Tolerance);
    }

    // History
    [Fact]
    public void History_AddAndRetrieve_Works()
    {
        _calc.AddToHistory("2 + 2 = 4");
        var history = _calc.GetHistory();
        Assert.Single(history);
        Assert.Equal("2 + 2 = 4", history[0]);
    }

    [Fact]
    public void History_Clear_EmptiesHistory()
    {
        _calc.AddToHistory("test entry");
        _calc.ClearHistory();
        Assert.Empty(_calc.GetHistory());
    }

    [Fact]
    public void Negate_ReturnsNegatedValue()
    {
        Assert.Equal(-5, _calc.Negate(5), Tolerance);
        Assert.Equal(3, _calc.Negate(-3), Tolerance);
    }
}

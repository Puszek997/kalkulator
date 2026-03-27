using System;
using System.Collections.Generic;

namespace Kalkulator
{
    public class Calculator
    {
        private double _memory = 0;
        private readonly List<string> _history = new();

        // Basic arithmetic
        public double Add(double a, double b) => a + b;
        public double Subtract(double a, double b) => a - b;
        public double Multiply(double a, double b) => a * b;

        public double Divide(double a, double b)
        {
            if (b == 0)
                throw new DivideByZeroException("Cannot divide by zero.");
            return a / b;
        }

        public double Modulo(double a, double b)
        {
            if (b == 0)
                throw new DivideByZeroException("Cannot compute modulo by zero.");
            return a % b;
        }

        // Power and roots
        public double Power(double baseValue, double exponent) => Math.Pow(baseValue, exponent);

        public double SquareRoot(double value)
        {
            if (value < 0)
                throw new ArgumentException("Cannot take square root of a negative number.");
            return Math.Sqrt(value);
        }

        public double NthRoot(double value, double n)
        {
            if (n == 0)
                throw new ArgumentException("Root degree cannot be zero.");
            if (value < 0 && n % 2 == 0)
                throw new ArgumentException("Cannot take even root of a negative number.");
            return Math.Pow(value, 1.0 / n);
        }

        // Absolute value and sign
        public double Absolute(double value) => Math.Abs(value);
        public double Negate(double value) => -value;

        // Trigonometric functions (angles in degrees)
        public double Sin(double degrees) => Math.Sin(DegreesToRadians(degrees));
        public double Cos(double degrees) => Math.Cos(DegreesToRadians(degrees));
        public double Tan(double degrees)
        {
            double radians = DegreesToRadians(degrees);
            double cosValue = Math.Cos(radians);
            if (Math.Abs(cosValue) < 1e-10)
                throw new ArgumentException("Tangent is undefined at this angle.");
            return Math.Tan(radians);
        }

        public double Asin(double value)
        {
            if (value < -1 || value > 1)
                throw new ArgumentException("Arcsine input must be between -1 and 1.");
            return RadiansToDegrees(Math.Asin(value));
        }

        public double Acos(double value)
        {
            if (value < -1 || value > 1)
                throw new ArgumentException("Arccosine input must be between -1 and 1.");
            return RadiansToDegrees(Math.Acos(value));
        }

        public double Atan(double value) => RadiansToDegrees(Math.Atan(value));

        // Logarithmic functions
        public double Log10(double value)
        {
            if (value <= 0)
                throw new ArgumentException("Logarithm is defined only for positive numbers.");
            return Math.Log10(value);
        }

        public double Ln(double value)
        {
            if (value <= 0)
                throw new ArgumentException("Natural logarithm is defined only for positive numbers.");
            return Math.Log(value);
        }

        public double LogBase(double value, double base_)
        {
            if (value <= 0)
                throw new ArgumentException("Logarithm is defined only for positive numbers.");
            if (base_ <= 0 || base_ == 1)
                throw new ArgumentException("Logarithm base must be positive and not equal to 1.");
            return Math.Log(value, base_);
        }

        // Exponential
        public double Exp(double value) => Math.Exp(value);

        // Factorial
        public double Factorial(int n)
        {
            if (n < 0)
                throw new ArgumentException("Factorial is not defined for negative numbers.");
            if (n > 20)
                throw new ArgumentException("Input too large for factorial (max 20).");
            double result = 1;
            for (int i = 2; i <= n; i++)
                result *= i;
            return result;
        }

        // Percentage
        public double Percent(double value) => value / 100.0;
        public double PercentOf(double percent, double total) => (percent / 100.0) * total;

        // Memory operations
        public void MemoryStore(double value) => _memory = value;
        public double MemoryRecall() => _memory;
        public void MemoryClear() => _memory = 0;
        public void MemoryAdd(double value) => _memory += value;
        public void MemorySubtract(double value) => _memory -= value;

        // History
        public void AddToHistory(string entry) => _history.Add(entry);
        public IReadOnlyList<string> GetHistory() => _history.AsReadOnly();
        public void ClearHistory() => _history.Clear();

        // Helper
        private static double DegreesToRadians(double degrees) => degrees * Math.PI / 180.0;
        private static double RadiansToDegrees(double radians) => radians * 180.0 / Math.PI;
    }
}

using System;
using System.Collections.Generic;

namespace Kalkulator
{
    class Program
    {
        static readonly Calculator _calc = new();
        static double _lastResult = 0;

        static void Main(string[] args)
        {
            Console.OutputEncoding = System.Text.Encoding.UTF8;
            PrintBanner();

            while (true)
            {
                Console.WriteLine();
                PrintMenu();
                Console.Write("Wybierz opcję / Choose option: ");
                string? input = Console.ReadLine()?.Trim();

                if (string.IsNullOrEmpty(input))
                    continue;

                if (input == "0" || input.Equals("exit", StringComparison.OrdinalIgnoreCase) ||
                    input.Equals("quit", StringComparison.OrdinalIgnoreCase))
                {
                    Console.WriteLine("Do widzenia! / Goodbye!");
                    break;
                }

                HandleInput(input);
            }
        }

        static void PrintBanner()
        {
            Console.Clear();
            Console.ForegroundColor = ConsoleColor.Cyan;
            Console.WriteLine("╔══════════════════════════════════════════════════╗");
            Console.WriteLine("║           ★  GIGA KALKULATOR  ★                 ║");
            Console.WriteLine("║        Zaawansowany Kalkulator Naukowy           ║");
            Console.WriteLine("║        Advanced Scientific Calculator            ║");
            Console.WriteLine("╚══════════════════════════════════════════════════╝");
            Console.ResetColor();
        }

        static void PrintMenu()
        {
            Console.ForegroundColor = ConsoleColor.Yellow;
            Console.WriteLine("─── PODSTAWOWE / BASIC ───────────────────────────");
            Console.ResetColor();
            Console.WriteLine("  1. Dodawanie (+)         2. Odejmowanie (-)");
            Console.WriteLine("  3. Mnożenie (*)          4. Dzielenie (/)");
            Console.WriteLine("  5. Modulo (%)            6. Potęgowanie (^)");
            Console.ForegroundColor = ConsoleColor.Yellow;
            Console.WriteLine("─── ZAAWANSOWANE / ADVANCED ──────────────────────");
            Console.ResetColor();
            Console.WriteLine("  7. Pierwiastek (√)       8. Pierwiastek n-ty");
            Console.WriteLine("  9. Wartość bezwzględna   10. Silnia (n!)");
            Console.WriteLine("  11. Procent              12. Procent z liczby");
            Console.ForegroundColor = ConsoleColor.Yellow;
            Console.WriteLine("─── TRYGONOMETRIA / TRIGONOMETRY ─────────────────");
            Console.ResetColor();
            Console.WriteLine("  13. sin(x)   14. cos(x)   15. tan(x)");
            Console.WriteLine("  16. asin(x)  17. acos(x)  18. atan(x)");
            Console.ForegroundColor = ConsoleColor.Yellow;
            Console.WriteLine("─── LOGARYTMY / LOGARITHMS ───────────────────────");
            Console.ResetColor();
            Console.WriteLine("  19. log₁₀(x)  20. ln(x)   21. logₙ(x)");
            Console.WriteLine("  22. e^x");
            Console.ForegroundColor = ConsoleColor.Yellow;
            Console.WriteLine("─── PAMIĘĆ / MEMORY ──────────────────────────────");
            Console.ResetColor();
            Console.WriteLine("  23. Zapisz (MS)  24. Odczyt (MR)  25. Wyczyść (MC)");
            Console.WriteLine("  26. M+           27. M-");
            Console.ForegroundColor = ConsoleColor.Yellow;
            Console.WriteLine("─── HISTORIA / HISTORY ───────────────────────────");
            Console.ResetColor();
            Console.WriteLine("  28. Pokaż historię       29. Wyczyść historię");
            Console.ForegroundColor = ConsoleColor.Green;
            Console.WriteLine($"  Ostatni wynik / Last result: {_lastResult}");
            Console.WriteLine($"  Pamięć / Memory: {_calc.MemoryRecall()}");
            Console.ResetColor();
            Console.WriteLine("  0. Wyjście / Exit");
        }

        static void HandleInput(string input)
        {
            try
            {
                switch (input)
                {
                    case "1": TwoArgOperation("+", _calc.Add); break;
                    case "2": TwoArgOperation("-", _calc.Subtract); break;
                    case "3": TwoArgOperation("*", _calc.Multiply); break;
                    case "4": TwoArgOperation("/", _calc.Divide); break;
                    case "5": TwoArgOperation("%", _calc.Modulo); break;
                    case "6": TwoArgOperation("^", _calc.Power); break;
                    case "7": OneArgOperation("√", v => _calc.SquareRoot(v)); break;
                    case "8": NthRootOperation(); break;
                    case "9": OneArgOperation("|x|", v => _calc.Absolute(v)); break;
                    case "10": FactorialOperation(); break;
                    case "11": OneArgOperation("%", v => _calc.Percent(v)); break;
                    case "12": TwoArgOperation("% of", (p, t) => _calc.PercentOf(p, t)); break;
                    case "13": OneArgOperation("sin", v => _calc.Sin(v)); break;
                    case "14": OneArgOperation("cos", v => _calc.Cos(v)); break;
                    case "15": OneArgOperation("tan", v => _calc.Tan(v)); break;
                    case "16": OneArgOperation("asin", v => _calc.Asin(v)); break;
                    case "17": OneArgOperation("acos", v => _calc.Acos(v)); break;
                    case "18": OneArgOperation("atan", v => _calc.Atan(v)); break;
                    case "19": OneArgOperation("log₁₀", v => _calc.Log10(v)); break;
                    case "20": OneArgOperation("ln", v => _calc.Ln(v)); break;
                    case "21": LogBaseOperation(); break;
                    case "22": OneArgOperation("e^", v => _calc.Exp(v)); break;
                    case "23": MemoryStore(); break;
                    case "24": MemoryRecall(); break;
                    case "25": _calc.MemoryClear(); Console.WriteLine("Pamięć wyczyszczona / Memory cleared."); break;
                    case "26": MemoryAddSub(add: true); break;
                    case "27": MemoryAddSub(add: false); break;
                    case "28": ShowHistory(); break;
                    case "29": _calc.ClearHistory(); Console.WriteLine("Historia wyczyszczona / History cleared."); break;
                    default:
                        Console.ForegroundColor = ConsoleColor.Red;
                        Console.WriteLine("Nieznana opcja / Unknown option.");
                        Console.ResetColor();
                        break;
                }
            }
            catch (Exception ex)
            {
                Console.ForegroundColor = ConsoleColor.Red;
                Console.WriteLine($"Błąd / Error: {ex.Message}");
                Console.ResetColor();
            }
        }

        static void TwoArgOperation(string symbol, Func<double, double, double> op)
        {
            double a = ReadDouble("Podaj pierwszą liczbę / Enter first number: ");
            double b = ReadDouble("Podaj drugą liczbę / Enter second number: ");
            double result = op(a, b);
            string entry = $"{a} {symbol} {b} = {result}";
            _calc.AddToHistory(entry);
            _lastResult = result;
            PrintResult(result);
        }

        static void OneArgOperation(string symbol, Func<double, double> op)
        {
            double a = ReadDouble("Podaj liczbę / Enter number: ");
            double result = op(a);
            string entry = $"{symbol}({a}) = {result}";
            _calc.AddToHistory(entry);
            _lastResult = result;
            PrintResult(result);
        }

        static void NthRootOperation()
        {
            double value = ReadDouble("Podaj liczbę / Enter number: ");
            double n = ReadDouble("Podaj stopień pierwiastka / Enter root degree: ");
            double result = _calc.NthRoot(value, n);
            string entry = $"{n}√({value}) = {result}";
            _calc.AddToHistory(entry);
            _lastResult = result;
            PrintResult(result);
        }

        static void FactorialOperation()
        {
            Console.Write("Podaj liczbę całkowitą / Enter integer: ");
            if (int.TryParse(Console.ReadLine(), out int n))
            {
                double result = _calc.Factorial(n);
                string entry = $"{n}! = {result}";
                _calc.AddToHistory(entry);
                _lastResult = result;
                PrintResult(result);
            }
            else
            {
                Console.ForegroundColor = ConsoleColor.Red;
                Console.WriteLine("Nieprawidłowa liczba całkowita / Invalid integer.");
                Console.ResetColor();
            }
        }

        static void LogBaseOperation()
        {
            double value = ReadDouble("Podaj liczbę / Enter number: ");
            double base_ = ReadDouble("Podaj podstawę logarytmu / Enter logarithm base: ");
            double result = _calc.LogBase(value, base_);
            string entry = $"log{base_}({value}) = {result}";
            _calc.AddToHistory(entry);
            _lastResult = result;
            PrintResult(result);
        }

        static void MemoryStore()
        {
            double value = ReadDouble("Podaj wartość do zapisania / Enter value to store: ");
            _calc.MemoryStore(value);
            Console.WriteLine($"Zapisano {value} w pamięci / Stored {value} in memory.");
        }

        static void MemoryRecall()
        {
            double value = _calc.MemoryRecall();
            _lastResult = value;
            Console.ForegroundColor = ConsoleColor.Green;
            Console.WriteLine($"Wartość w pamięci / Memory value: {value}");
            Console.ResetColor();
        }

        static void MemoryAddSub(bool add)
        {
            double value = ReadDouble("Podaj wartość / Enter value: ");
            if (add)
                _calc.MemoryAdd(value);
            else
                _calc.MemorySubtract(value);
            Console.WriteLine($"Pamięć / Memory: {_calc.MemoryRecall()}");
        }

        static void ShowHistory()
        {
            var history = _calc.GetHistory();
            if (history.Count == 0)
            {
                Console.WriteLine("Historia jest pusta / History is empty.");
                return;
            }
            Console.ForegroundColor = ConsoleColor.Cyan;
            Console.WriteLine("─── Historia obliczeń / Calculation history ───");
            Console.ResetColor();
            for (int i = 0; i < history.Count; i++)
                Console.WriteLine($"  {i + 1}. {history[i]}");
            Console.ForegroundColor = ConsoleColor.Cyan;
            Console.WriteLine("──────────────────────────────────────────────");
            Console.ResetColor();
        }

        static double ReadDouble(string prompt)
        {
            while (true)
            {
                Console.Write(prompt);
                string? input = Console.ReadLine();
                if (input != null && input.Equals("ANS", StringComparison.OrdinalIgnoreCase))
                    return _lastResult;
                if (double.TryParse(input, System.Globalization.NumberStyles.Any,
                    System.Globalization.CultureInfo.InvariantCulture, out double value))
                    return value;
                Console.ForegroundColor = ConsoleColor.Red;
                Console.WriteLine("Nieprawidłowa liczba. Spróbuj ponownie / Invalid number. Try again.");
                Console.ResetColor();
            }
        }

        static void PrintResult(double result)
        {
            Console.ForegroundColor = ConsoleColor.Green;
            Console.WriteLine($"  ═══ Wynik / Result: {result} ═══");
            Console.ResetColor();
        }
    }
}

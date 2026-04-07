#include <iostream>
#include <string>
#include <vector>
#include <sstream>
#include <cmath>
#include <iomanip>

// Подключаем все заголовки
#include "lexer.h"
#include "parser.h"
#include "evaluator.h"
#include "differentiator.h"
#include "simplifier.h"

// Цвета для вывода
#ifdef _WIN32
    #define GREEN ""
    #define RED ""
    #define RESET ""
#else
    #define GREEN "\033[32m"
    #define RED "\033[31m"
    #define RESET "\033[0m"
#endif

struct TestResult {
    std::string name;
    bool passed;
    std::string expected;
    std::string got;
};

class TestRunner {
private:
    std::vector<TestResult> results;
    int passed = 0;
    int failed = 0;

    bool almost_equal(double a, double b, double eps = 1e-6) {
        return std::fabs(a - b) < eps;
    }

public:
    void test_lexer() {
        struct LexerTest {
            std::string name;
            std::string input;
            std::vector<lexem_t> expected;
        };

        std::vector<LexerTest> tests = {
            {"Simple number", "123", {lexem_t::NUMBER, lexem_t::EOEX}},
            {"Simple variable", "x", {lexem_t::VARIABLE, lexem_t::EOEX}},
            {"Binary operators", "x+y", {lexem_t::VARIABLE, lexem_t::PLUS, lexem_t::VARIABLE, lexem_t::EOEX}},
            {"All operators", "x+y-z*w/t^2", {lexem_t::VARIABLE, lexem_t::PLUS, lexem_t::VARIABLE, lexem_t::MINUS,
                                               lexem_t::VARIABLE, lexem_t::MULTIPLY, lexem_t::VARIABLE, lexem_t::DIVIDE,
                                               lexem_t::VARIABLE, lexem_t::POWER, lexem_t::NUMBER, lexem_t::EOEX}},
            {"Parentheses", "(x+2)", {lexem_t::LPAREN, lexem_t::VARIABLE, lexem_t::PLUS,
                                      lexem_t::NUMBER, lexem_t::RPAREN, lexem_t::EOEX}},
        };

        for (const auto& test : tests) {
            try {
                Lexer lexer(test.input);
                bool ok = true;
                for (size_t i = 0; i < test.expected.size() && ok; i++) {
                    Token tok = lexer.next();
                    if (tok.type() != test.expected[i]) {
                        ok = false;
                    }
                }
                results.push_back({"Lexer: " + test.name, ok, "All tokens match", ok ? "OK" : "Token mismatch"});
                if (ok) passed++; else failed++;
            } catch (const std::exception& e) {
                results.push_back({"Lexer: " + test.name, false, "No exception", e.what()});
                failed++;
            }
        }
    }

    void test_parser() {
        struct ParserTest {
            std::string name;
            std::string input;
            bool should_succeed;
        };

        std::vector<ParserTest> tests = {
            {"Simple number", "123", true},
            {"Simple variable", "x", true},
            {"Binary operation", "x+y", true},
            {"Nested parentheses", "((x+y)*2)", true},
            {"Function call", "sin(x)", true},
            {"Missing closing paren", "(x+2", false},
            {"Invalid token", "x@y", false},
            {"Extra after expression", "x+2 3", false},
        };

        for (const auto& test : tests) {
            try {
                Lexer lexer(test.input);
                std::vector<Token> tokens;
                Token tok = lexer.next();
                while (tok.type() != lexem_t::EOEX) {
                    tokens.push_back(tok);
                    tok = lexer.next();
                }
                tokens.push_back(Token(lexem_t::EOEX, ""));

                Parser parser(tokens);
                auto ast = parser.parse();

                if (test.should_succeed) {
                    results.push_back({"Parser: " + test.name, true, "Parse success", ast->toString()});
                    passed++;
                } else {
                    results.push_back({"Parser: " + test.name, false, "Should fail", "Parsed successfully"});
                    failed++;
                }
            } catch (const std::exception& e) {
                if (test.should_succeed) {
                    results.push_back({"Parser: " + test.name, false, "Should succeed", e.what()});
                    failed++;
                } else {
                    results.push_back({"Parser: " + test.name, true, "Should fail", e.what()});
                    passed++;
                }
            }
        }
    }

    void test_evaluate() {
        struct EvalTest {
            std::string name;
            std::string expression;
            std::vector<std::pair<std::string, double>> vars;
            double expected;
            bool expect_error = false;
        };

        std::vector<EvalTest> tests = {
            {"Simple addition", "x+2", {{"x", 3}}, 5.0},
            {"Multiplication", "x*y", {{"x", 3}, {"y", 4}}, 12.0},
            {"Division", "x/y", {{"x", 10}, {"y", 2}}, 5.0},
            {"Power", "2^3", {}, 8.0},
            {"Unary minus", "-5", {}, -5.0},
            {"Double unary minus", "--5", {}, 5.0},
            {"Sin function", "sin(0)", {}, 0.0},
            {"Cos function", "cos(0)", {}, 1.0},
            {"Sqrt function", "sqrt(4)", {}, 2.0},
            {"Log function", "log(1)", {}, 0.0},
            {"Complex expression", "x*x + y*sin(x)", {{"x", 3}, {"y", 4}}, 9 + 4*std::sin(3)},
            {"Division by zero", "5/x", {{"x", 0}}, 0, true},
            {"Sqrt negative", "sqrt(-1)", {}, 0, true},
            {"Log non-positive", "log(0)", {}, 0, true},
            {"Asin out of range", "asin(2)", {}, 0, true},
            {"Unknown variable", "x+2", {}, 0, true},
        };

        for (const auto& test : tests) {
            try {
                Lexer lexer(test.expression);
                std::vector<Token> tokens;
                Token tok = lexer.next();
                while (tok.type() != lexem_t::EOEX) {
                    tokens.push_back(tok);
                    tok = lexer.next();
                }
                tokens.push_back(Token(lexem_t::EOEX, ""));

                Parser parser(tokens);
                auto ast = parser.parse();

                Evaluator eval;
                for (const auto& v : test.vars) {
                    eval.setVariable(v.first, v.second);
                }

                double result = eval.evaluate(ast.get());

                if (test.expect_error) {
                    results.push_back({"Evaluate: " + test.name, false, "Should fail", "Got " + std::to_string(result)});
                    failed++;
                } else if (almost_equal(result, test.expected)) {
                    results.push_back({"Evaluate: " + test.name, true, std::to_string(test.expected), std::to_string(result)});
                    passed++;
                } else {
                    results.push_back({"Evaluate: " + test.name, false, std::to_string(test.expected), std::to_string(result)});
                    failed++;
                }
            } catch (const std::exception& e) {
                if (test.expect_error) {
                    results.push_back({"Evaluate: " + test.name, true, "Error expected", e.what()});
                    passed++;
                } else {
                    results.push_back({"Evaluate: " + test.name, false, std::to_string(test.expected), e.what()});
                    failed++;
                }
            }
        }
    }

    void test_derivative() {
        struct DerivTest {
            std::string name;
            std::string expression;
            std::string var;
            double test_point;
            double expected_derivative;
        };

        std::vector<DerivTest> tests = {
            {"Derivative of x", "x", "x", 5.0, 1.0},
            {"Derivative of constant", "5", "x", 5.0, 0.0},
            {"Derivative of x^2", "x^2", "x", 3.0, 6.0},
            {"Derivative of sin(x)", "sin(x)", "x", 0.0, 1.0},
            {"Derivative of cos(x)", "cos(x)", "x", 0.0, 0.0},
            {"Derivative of exp(x)", "exp(x)", "x", 1.0, std::exp(1.0)},
            {"Derivative of log(x)", "log(x)", "x", 2.0, 0.5},
            {"Derivative of x*sin(x)", "x*sin(x)", "x", 1.0, std::sin(1.0) + 1.0*std::cos(1.0)},
        };

        for (const auto& test : tests) {
            try {
                Lexer lexer(test.expression);
                std::vector<Token> tokens;
                Token tok = lexer.next();
                while (tok.type() != lexem_t::EOEX) {
                    tokens.push_back(tok);
                    tok = lexer.next();
                }
                tokens.push_back(Token(lexem_t::EOEX, ""));

                Parser parser(tokens);
                auto ast = parser.parse();

                Differentiator diff(test.var);
                auto derivative = diff.differentiate(ast.get());

                Evaluator eval;
                eval.setVariable(test.var, test.test_point);
                double result = eval.evaluate(derivative.get());

                if (almost_equal(result, test.expected_derivative)) {
                    results.push_back({"Derivative: " + test.name, true, std::to_string(test.expected_derivative), std::to_string(result)});
                    passed++;
                } else {
                    results.push_back({"Derivative: " + test.name, false, std::to_string(test.expected_derivative), std::to_string(result)});
                    failed++;
                }
            } catch (const std::exception& e) {
                results.push_back({"Derivative: " + test.name, false, "No exception", e.what()});
                failed++;
            }
        }
    }

    void print_results() {
        std::cout << "\n" << std::string(60, '=') << "\n";
        std::cout << "TEST RESULTS\n";
        std::cout << std::string(60, '=') << "\n\n";

        for (const auto& result : results) {
            std::cout << (result.passed ? GREEN "[PASS]" : RED "[FAIL]") << RESET " " << result.name << "\n";
            if (!result.passed) {
                std::cout << "  Expected: " << result.expected << "\n";
                std::cout << "  Got:      " << result.got << "\n";
            }
        }

        std::cout << "\n" << std::string(60, '-') << "\n";
        std::cout << "Summary: " << GREEN << passed << " passed" << RESET ", "
                  << RED << failed << " failed" << RESET "\n";
        std::cout << "Total: " << (passed + failed) << " tests\n";
        std::cout << std::string(60, '=') << "\n";
    }

    bool all_passed() const {
        return failed == 0;
    }
};

int main() {
    std::cout << "Running tests for Calcul...\n";

    TestRunner runner;

    runner.test_lexer();
    runner.test_parser();
    runner.test_evaluate();
    runner.test_derivative();
    runner.print_results();

    return runner.all_passed() ? 0 : 1;
}
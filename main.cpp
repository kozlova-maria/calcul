#include "lexer.h"       // твой лексер
#include "parser.h"
#include "evaluator.h"
#include "differentiator.h"
#include <iostream>
#include <sstream>
#include <vector>

int main() {
    std::string line;

    // Читаем команду
    std::getline(std::cin, line);
    std::istringstream iss(line);

    std::string command;
    iss >> command;

    int n;
    iss >> n;

    // Читаем имена переменных
    std::vector<std::string> var_names;
    for (int i = 0; i < n; i++) {
        std::string name;
        iss >> name;
        var_names.push_back(name);
    }

    // Читаем значения переменных (если нужно)
    std::vector<double> var_values;
    if (command == "evaluate" || command == "evaluate_derivative") {
        for (int i = 0; i < n; i++) {
            double val;
            std::cin >> val;
            var_values.push_back(val);
        }
        std::cin.ignore();  // очищаем перевод строки
    }

    // Читаем выражение
    std::string expression;
    std::getline(std::cin, expression);

    try {
        // === 1. ТОКЕНИЗАЦИЯ ===
        Lexer lexer(expression);
        std::vector<Token> tokens;
        Token tok = lexer.next();
        while (tok.type() != lexem_t::EOEX) {
            tokens.push_back(tok);
            tok = lexer.next();
        }
        tokens.push_back(Token(lexem_t::EOEX, ""));

        // === 2. ПАРСИНГ ===
        Parser parser(tokens);
        auto ast = parser.parse();

        // === 3. ВЫПОЛНЕНИЕ КОМАНДЫ ===
        if (command == "evaluate") {
            Evaluator eval;
            for (size_t i = 0; i < var_names.size(); i++) {
                eval.setVariable(var_names[i], var_values[i]);
            }
            double result = eval.evaluate(ast.get());
            std::cout << result << std::endl;

        } else if (command == "derivative") {
            if (var_names.empty()) {
                std::cout << "ERROR: No variable to differentiate by" << std::endl;
                return 0;
            }
            Differentiator diff(var_names[0]);  // по первой переменной
            auto derivative = diff.differentiate(ast.get());
            std::cout << derivative->toString() << std::endl;

        } else if (command == "evaluate_derivative") {
            if (var_names.empty()) {
                std::cout << "ERROR: No variable to differentiate by" << std::endl;
                return 0;
            }
            Differentiator diff(var_names[0]);
            auto derivative = diff.differentiate(ast.get());

            Evaluator eval;
            for (size_t i = 0; i < var_names.size(); i++) {
                eval.setVariable(var_names[i], var_values[i]);
            }
            double result = eval.evaluate(derivative.get());
            std::cout << result << std::endl;
        }

    } catch (const std::exception& e) {
        std::cout << e.what() << std::endl;
    }

    return 0;
}
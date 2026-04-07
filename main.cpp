#include "lexer.h"
#include "parser.h"
#include "evaluator.h"
#include "simplifier.h"
#include "differentiator.h"
#include <iostream>
#include <sstream>
#include <vector>
#include <string>

int main() {
    //читаем команду
    std::string command;
    std::getline(std::cin, command);

    //читаем количество переменных
    std::string n_str;
    std::getline(std::cin, n_str);
    int n = std::stoi(n_str);

    //читаем имена переменных
    std::string vars_line;
    std::getline(std::cin, vars_line);
    std::istringstream vars_iss(vars_line);
    std::vector<std::string> var_names;
    for (int i = 0; i < n; i++) {
        std::string name;
        vars_iss >> name;
        var_names.push_back(name);
    }

    //читаем значения переменных
    std::string values_line;
    std::getline(std::cin, values_line);

    std::vector<double> var_values;
    if (command == "evaluate" || command == "evaluate_derivative") {
        std::istringstream values_iss(values_line);
        for (int i = 0; i < n; i++) {
            double val;
            if (!(values_iss >> val)) {
                throw std::runtime_error("ERROR: not enough variable values");
            }
            var_values.push_back(val);
        }
    }


    //читаем выражение
    std::string expression;
    std::getline(std::cin, expression);

    try {
        //токенизация
        Lexer lexer(expression);
        std::vector<Token> tokens;
        Token tok = lexer.next();
        while (tok.type() != lexem_t::EOEX) {
            tokens.push_back(tok);
            tok = lexer.next();
        }
        tokens.push_back(Token(lexem_t::EOEX, ""));

        //парсинг
        Parser parser(tokens);
        auto ast = parser.parse();

        //выполняем команду
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
            Differentiator diff(var_names[0]);
            auto derivative = diff.differentiate(ast.get());

            // Упрощаем производную
            Simplifier simplifier;
            derivative = simplifier.simplify(std::move(derivative));

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
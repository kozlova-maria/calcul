#ifndef EVALUATOR_H
#define EVALUATOR_H

#include "ast.h"
#include <cmath>
#include <string>
#include <map>
#include <stdexcept>

class Evaluator {
private:
    std::map<std::string, double> vars_;  // значения переменных

    double evalNode(const Node* node) {     //считаем значение узла
        switch(node->type) {                //смотрим на тип узла
            case NodeType::NUMBER:          //если чиселка - возвращаем значение
                return node->num_value;

            case NodeType::VARIABLE: {      //если переменная, ищем ее в списке, и если не находим, кидаем ошибку
                auto it = vars_.find(node->name);
                if (it == vars_.end()) {
                    throw std::runtime_error("error: Unknown variable " + node->name);
                }
                return it->second;
            }

            case NodeType::UNARY_OP: {                  //если унарный оператор - применяем его
                double val = evalNode(node->left.get());
                if (node->op == '-') return -val;
                return val;  // для '+' просто возвращаем значение
            }

            case NodeType::BINARY_OP: {                 //если бинарный оператор - достаем левого и правого потомка и применяем оператор
                double left = evalNode(node->left.get());
                double right = evalNode(node->right.get());
                switch(node->op) {
                    case '+': return left + right;
                    case '-': return left - right;
                    case '*': return left * right;
                    case '/':
                        if (right == 0) throw std::runtime_error("error: you can't divide by zero bruh");
                        return left / right;
                    case '^': return std::pow(left, right);
                    default:
                        throw std::runtime_error("error: Unknown binary operator");
                }
            }

            case NodeType::FUNCTION: {                          //обработка функций с учетом области определения аргументов
                double arg = evalNode(node->left.get());
                if (node->name == "sin") return std::sin(arg);
                if (node->name == "cos") return std::cos(arg);
                if (node->name == "tan") return std::tan(arg);
                if (node->name == "asin") {
                    if (arg < -1 || arg > 1) throw std::runtime_error("error: asin argument out of range");
                    return std::asin(arg);
                }
                if (node->name == "acos") {
                    if (arg < -1 || arg > 1) throw std::runtime_error("error: acos argument out of range");
                    return std::acos(arg);
                }
                if (node->name == "atan") return std::atan(arg);
                if (node->name == "exp") return std::exp(arg);
                if (node->name == "log") {
                    if (arg <= 0) throw std::runtime_error("error: log argument must be > 0");
                    return std::log(arg);
                }
                if (node->name == "sqrt") {
                    if (arg < 0) throw std::runtime_error("error: sqrt argument must be >= 0");
                    return std::sqrt(arg);
                }
                throw std::runtime_error("error: Unknown function " + node->name);
            }
        }
        return 0;
    }

public:
    void setVariable(const std::string& name, double value) {
        vars_[name] = value;
    }

    double evaluate(const Node* node) {
        return evalNode(node);
    }
};

#endif
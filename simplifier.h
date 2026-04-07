#ifndef SIMPLIFIER_H
#define SIMPLIFIER_H

#include "ast.h"
#include <memory>

class Simplifier {
private:
    // Проверка, является ли узел нулем
    bool isZero(const Node* node) const {
        if (node->type == NodeType::NUMBER && node->num_value == 0.0) return true;
        return false;
    }

    // Проверка, является ли узел единицей
    bool isOne(const Node* node) const {
        if (node->type == NodeType::NUMBER && node->num_value == 1.0) return true;
        return false;
    }

    // Проверка, является ли узел числом
    bool isNumber(const Node* node, double& val) const {
        if (node->type == NodeType::NUMBER) {
            val = node->num_value;
            return true;
        }
        return false;
    }

public:
    std::unique_ptr<Node> simplify(std::unique_ptr<Node> node) {
        if (!node) return nullptr;


        if (node->left) {
            node->left = simplify(std::move(node->left));
        }
        if (node->right) {
            node->right = simplify(std::move(node->right));
        }

        // Упрощаем в зависимости от типа
        switch (node->type) {
            case NodeType::BINARY_OP:
                return simplifyBinaryOp(std::move(node));

            case NodeType::UNARY_OP:
                return simplifyUnaryOp(std::move(node));

            case NodeType::FUNCTION:

                return node;

            default:
                return node;
        }
    }

private:
    std::unique_ptr<Node> simplifyBinaryOp(std::unique_ptr<Node> node) {
        char op = node->op;
        auto& left = node->left;
        auto& right = node->right;

        double lval, rval;
        bool l_is_num = isNumber(left.get(), lval);
        bool r_is_num = isNumber(right.get(), rval);

        // === Арифметические упрощения ===

        // x + 0 = x
        if (op == '+' && isZero(right.get())) {
            return std::move(left);
        }
        // 0 + x = x
        if (op == '+' && isZero(left.get())) {
            return std::move(right);
        }

        // x - 0 = x
        if (op == '-' && isZero(right.get())) {
            return std::move(left);
        }
        // 0 - x = -x
        if (op == '-' && isZero(left.get())) {
            return std::make_unique<Node>('-', std::move(right));
        }

        // x * 0 = 0
        if (op == '*' && (isZero(left.get()) || isZero(right.get()))) {
            return std::make_unique<Node>(0.0);
        }
        // x * 1 = x
        if (op == '*' && isOne(right.get())) {
            return std::move(left);
        }
        // 1 * x = x
        if (op == '*' && isOne(left.get())) {
            return std::move(right);
        }

        // x / 1 = x
        if (op == '/' && isOne(right.get())) {
            return std::move(left);
        }
        // 0 / x = 0
        if (op == '/' && isZero(left.get())) {
            return std::make_unique<Node>(0.0);
        }
        // x / x = 1 (если x не зависит от переменной? упрощаем осторожно)

        // x ^ 0 = 1
        if (op == '^' && isZero(right.get())) {
            return std::make_unique<Node>(1.0);
        }
        // x ^ 1 = x
        if (op == '^' && isOne(right.get())) {
            return std::move(left);
        }
        // 0 ^ x = 0 (при x > 0)
        if (op == '^' && isZero(left.get())) {
            return std::make_unique<Node>(0.0);
        }
        // 1 ^ x = 1
        if (op == '^' && isOne(left.get())) {
            return std::make_unique<Node>(1.0);
        }

        // === Вычисление констант ===
        if (l_is_num && r_is_num) {
            double result;
            switch(op) {
                case '+': result = lval + rval; break;
                case '-': result = lval - rval; break;
                case '*': result = lval * rval; break;
                case '/':
                    if (rval == 0) return node;
                    result = lval / rval;
                    break;
                case '^': result = std::pow(lval, rval); break;
                default: return node;
            }
            return std::make_unique<Node>(result);
        }

        return node;
    }

    std::unique_ptr<Node> simplifyUnaryOp(std::unique_ptr<Node> node) {
        char op = node->op;
        auto& operand = node->left;

        double val;
        if (isNumber(operand.get(), val)) {
            if (op == '-') {
                return std::make_unique<Node>(-val);
            }
            // +x = x
            return std::move(operand);
        }

        // -(-x) = x
        if (op == '-' && operand->type == NodeType::UNARY_OP && operand->op == '-') {
            return std::move(operand->left);
        }

        return node;
    }
};

#endif
#ifndef DIFFERENTIATOR_H
#define DIFFERENTIATOR_H

#include "ast.h"
#include <memory>
#include <stdexcept>
#include <cmath>

class Differentiator {
private:
    std::string var_; //переменная, по которой дифференцируем

    std::unique_ptr<Node> diff(const Node* node) {
        switch(node->type) {
            case NodeType::NUMBER:                      //если чиселка - 0
                return std::make_unique<Node>(0.0);

            case NodeType::VARIABLE:                    // если переменная, по которой дифференцируем - 1, если нет - 0
                if (node->name == var_) {
                    return std::make_unique<Node>(1.0);
                } else {
                    return std::make_unique<Node>(0.0);
                }

            case NodeType::UNARY_OP: {                  //если унарная операция - обращаемся к потомку
                auto d = diff(node->left.get());
                return std::make_unique<Node>(node->op, std::move(d));
            }

            case NodeType::BINARY_OP: {                 //бинарную по правилам дифференцирования
                auto left = node->left.get();
                auto right = node->right.get();
                auto dleft = diff(left);
                auto dright = diff(right);

                switch(node->op) {
                    case '+':
                        return std::make_unique<Node>('+', std::move(dleft), std::move(dright));        //(u + v)' = u' + v'
                    case '-':
                        return std::make_unique<Node>('-', std::move(dleft), std::move(dright));        //(u - v)' = u' - v'
                    case '*': {
                        auto left_clone = std::make_unique<Node>(*left);                                            //(u * v)' = u'v + v'u
                        auto right_clone = std::make_unique<Node>(*right);
                        return std::make_unique<Node>('+',
                            std::make_unique<Node>('*', std::move(dleft), std::move(right_clone)),
                            std::make_unique<Node>('*', std::move(left_clone), std::move(dright))
                        );
                    }
                    case '/': {                                                                                     //(u / v)' = (u'v - v'u)/v^2
                        auto left_clone = std::make_unique<Node>(*left);
                        auto right_clone = std::make_unique<Node>(*right);
                        auto right_clone2 = std::make_unique<Node>(*right);
                        return std::make_unique<Node>('/',
                            std::make_unique<Node>('-',
                                std::make_unique<Node>('*', std::move(dleft), std::move(right_clone)),
                                std::make_unique<Node>('*', std::move(left_clone), std::move(dright))
                            ),
                            std::make_unique<Node>('^', std::move(right_clone2), std::make_unique<Node>(2.0))
                        );
                    }
                    case '^': {
                        if (right->type == NodeType::NUMBER) {              //смотрим только случай х^n
                            double n = right->num_value;
                            auto left_clone = std::make_unique<Node>(*left);
                            auto pow_node = std::make_unique<Node>('^',
                                std::move(left_clone),
                                std::make_unique<Node>(n - 1.0));
                            auto mul1 = std::make_unique<Node>('*',
                                std::make_unique<Node>(n),
                                std::move(pow_node));
                            return std::make_unique<Node>('*', std::move(mul1), std::move(dleft));
                        }
                        throw std::runtime_error("ERROR: Derivative of variable exponent not implemented");
                    }
                    default:
                        throw std::runtime_error("ERROR: Unknown binary operator");
                }
            }

            case NodeType::FUNCTION: {              //дифференцируем функции по правилам
                auto arg = node->left.get();
                auto darg = diff(arg);
                std::unique_ptr<Node> d_func;

                if (node->name == "sin") {
                    d_func = std::make_unique<Node>("cos", std::make_unique<Node>(*arg));
                }
                else if (node->name == "cos") {
                    auto sin_node = std::make_unique<Node>("sin", std::make_unique<Node>(*arg));
                    d_func = std::make_unique<Node>('-', std::move(sin_node));
                }
                else if (node->name == "tan") {
                    auto cos_u = std::make_unique<Node>("cos", std::make_unique<Node>(*arg));
                    auto cos2 = std::make_unique<Node>('^', std::move(cos_u), std::make_unique<Node>(2.0));
                    d_func = std::make_unique<Node>('/', std::make_unique<Node>(1.0), std::move(cos2));
                }
                else if (node->name == "exp") {
                    d_func = std::make_unique<Node>("exp", std::make_unique<Node>(*arg));
                }
                else if (node->name == "log") {
                    d_func = std::make_unique<Node>('/', std::make_unique<Node>(1.0), std::make_unique<Node>(*arg));
                }
                else if (node->name == "sqrt") {
                    auto sqrt_u = std::make_unique<Node>("sqrt", std::make_unique<Node>(*arg));
                    auto two_sqrt = std::make_unique<Node>('*', std::make_unique<Node>(2.0), std::move(sqrt_u));
                    d_func = std::make_unique<Node>('/', std::make_unique<Node>(1.0), std::move(two_sqrt));
                }
                else if (node->name == "asin") {
                    auto u2 = std::make_unique<Node>('^', std::make_unique<Node>(*arg), std::make_unique<Node>(2.0));
                    auto one_minus = std::make_unique<Node>('-', std::make_unique<Node>(1.0), std::move(u2));
                    auto sqrt_node = std::make_unique<Node>("sqrt", std::move(one_minus));
                    d_func = std::make_unique<Node>('/', std::make_unique<Node>(1.0), std::move(sqrt_node));
                }
                else if (node->name == "acos") {
                    auto u2 = std::make_unique<Node>('^', std::make_unique<Node>(*arg), std::make_unique<Node>(2.0));
                    auto one_minus = std::make_unique<Node>('-', std::make_unique<Node>(1.0), std::move(u2));
                    auto sqrt_node = std::make_unique<Node>("sqrt", std::move(one_minus));
                    auto div_node = std::make_unique<Node>('/', std::make_unique<Node>(1.0), std::move(sqrt_node));
                    d_func = std::make_unique<Node>('-', std::move(div_node));
                }
                else if (node->name == "atan") {
                    auto u2 = std::make_unique<Node>('^', std::make_unique<Node>(*arg), std::make_unique<Node>(2.0));
                    auto one_plus = std::make_unique<Node>('+', std::make_unique<Node>(1.0), std::move(u2));
                    d_func = std::make_unique<Node>('/', std::make_unique<Node>(1.0), std::move(one_plus));
                }
                else {
                    throw std::runtime_error("ERROR: Unknown function " + node->name);
                }

                return std::make_unique<Node>('*', std::move(d_func), std::move(darg));
            }
        }

        throw std::runtime_error("ERROR: Derivative not implemented for this node");
    }

public:
    Differentiator(const std::string& var) : var_(var) {}

    std::unique_ptr<Node> differentiate(const Node* node) {
        return diff(node);
    }
};

#endif
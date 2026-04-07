#ifndef DIFFERENTIATOR_H
#define DIFFERENTIATOR_H

#include "ast.h"
#include <memory>
#include <stdexcept>
#include <cmath>

class Differentiator {
private:
    std::string var_; //переменная, по которой дифференцируем

    //глубокое копирование узла
    std::unique_ptr<Node> clone(const Node* node) const {
        return node ? std::make_unique<Node>(*node) : nullptr;
    }

    //проверка на зависимость детей от переменной
    bool dependsOnVar(const Node* node) const {
        if (!node) return false;
        if (node->type == NodeType::VARIABLE) return node->name == var_;
        if (node->type == NodeType::NUMBER) return false;

        return dependsOnVar(node->left.get()) || dependsOnVar(node->right.get());
    }

    std::unique_ptr<Node> diff(const Node* node) const {                //если чиселка - то 0
        switch(node->type) {
            case NodeType::NUMBER:
                return std::make_unique<Node>(0.0);

            case NodeType::VARIABLE:
                return std::make_unique<Node>((node->name == var_) ? 1.0 : 0.0);  //если переменная, по которой дифференцируем, то 1

            case NodeType::UNARY_OP: {                                  //жестко считаем по правилам производных
                // (-f)' = -f'
                auto d = diff(node->left.get());
                return std::make_unique<Node>(node->op, std::move(d));
            }

            case NodeType::BINARY_OP: {
                const Node* L = node->left.get();
                const Node* R = node->right.get();
                auto dL = diff(L);
                auto dR = diff(R);

                switch(node->op) {
                    case '+':
                        return std::make_unique<Node>('+', std::move(dL), std::move(dR));
                    case '-':
                        return std::make_unique<Node>('-', std::move(dL), std::move(dR));

                    case '*': {
                        // (u*v)' = u'*v + u*v'
                        return std::make_unique<Node>('+',
                            std::make_unique<Node>('*', std::move(dL), clone(R)),
                            std::make_unique<Node>('*', clone(L), std::move(dR))
                        );
                    }

                    case '/': {
                        // (u/v)' = (u'*v - u*v') / v^2
                        return std::make_unique<Node>('/',
                            std::make_unique<Node>('-',
                                std::make_unique<Node>('*', std::move(dL), clone(R)),
                                std::make_unique<Node>('*', clone(L), std::move(dR))
                            ),
                            std::make_unique<Node>('^', clone(R), std::make_unique<Node>(2.0))
                        );
                    }

                    case '^': {                         //расширение функционала степенной функции
                        bool L_dep = dependsOnVar(L);
                        bool R_dep = dependsOnVar(R);

                        if (!R_dep) {
                            // основание - переменная, степень - константа: d/dx[u^c] = c * u^(c-1) * u'
                            auto exp_node = std::make_unique<Node>('-', clone(R), std::make_unique<Node>(1.0));
                            auto pow_node = std::make_unique<Node>('^', clone(L), std::move(exp_node));
                            auto coeff = std::make_unique<Node>('*', clone(R), std::move(pow_node));
                            return std::make_unique<Node>('*', std::move(coeff), std::move(dL));
                        }
                        else if (!L_dep) {
                            // основание - константа, степень - переменная: d/dx[c^v] = c^v * ln(c) * v'
                            if (L->type == NodeType::NUMBER && L->num_value <= 0) {
                                throw std::runtime_error("ERROR: Base must be > 0 for exponential differentiation");
                            }
                            auto exp_node = std::make_unique<Node>('^', clone(L), clone(R));
                            auto ln_node = std::make_unique<Node>("log", clone(L));
                            auto term1 = std::make_unique<Node>('*', std::move(exp_node), std::move(ln_node));
                            return std::make_unique<Node>('*', std::move(term1), std::move(dR));
                        }
                        else {
                            // общий случай, как же хорошо, что я ходила на матан: d/dx[u^v] = u^v * (v'*ln(u) + v*u'/u)
                            auto exp_node = std::make_unique<Node>('^', clone(L), clone(R));
                            auto ln_u = std::make_unique<Node>("log", clone(L));

                            auto term1 = std::make_unique<Node>('*', std::move(dR), std::move(ln_u));
                            auto term2 = std::make_unique<Node>('*', clone(R), std::make_unique<Node>('/', std::move(dL), clone(L)));
                            auto sum = std::make_unique<Node>('+', std::move(term1), std::move(term2));

                            return std::make_unique<Node>('*', std::move(exp_node), std::move(sum));
                        }
                    }

                    default:
                        throw std::runtime_error("ERROR: Unknown binary operator in differentiation");
                }
            }

            case NodeType::FUNCTION: {
                const Node* arg = node->left.get();
                auto darg = diff(arg); // производная аргумента
                std::unique_ptr<Node> d_func; // производная внешней функции по аргументу

                if (node->name == "sin") {
                    d_func = std::make_unique<Node>("cos", clone(arg));
                }
                else if (node->name == "cos") {
                    auto sin_node = std::make_unique<Node>("sin", clone(arg));
                    d_func = std::make_unique<Node>('-', std::move(sin_node));
                }
                else if (node->name == "tan") {
                    auto cos_node = std::make_unique<Node>("cos", clone(arg));
                    auto cos2 = std::make_unique<Node>('^', std::move(cos_node), std::make_unique<Node>(2.0));
                    d_func = std::make_unique<Node>('/', std::make_unique<Node>(1.0), std::move(cos2));
                }
                else if (node->name == "exp") {
                    d_func = std::make_unique<Node>("exp", clone(arg));
                }
                else if (node->name == "log") {
                    d_func = std::make_unique<Node>('/', std::make_unique<Node>(1.0), clone(arg));
                }
                else if (node->name == "sqrt") {
                    auto sqrt_node = std::make_unique<Node>("sqrt", clone(arg));
                    auto denom = std::make_unique<Node>('*', std::make_unique<Node>(2.0), std::move(sqrt_node));
                    d_func = std::make_unique<Node>('/', std::make_unique<Node>(1.0), std::move(denom));
                }
                else if (node->name == "asin") {
                    auto u2 = std::make_unique<Node>('^', clone(arg), std::make_unique<Node>(2.0));
                    auto one_minus = std::make_unique<Node>('-', std::make_unique<Node>(1.0), std::move(u2));
                    auto sqrt_node = std::make_unique<Node>("sqrt", std::move(one_minus));
                    d_func = std::make_unique<Node>('/', std::make_unique<Node>(1.0), std::move(sqrt_node));
                }
                else if (node->name == "acos") {
                    auto u2 = std::make_unique<Node>('^', clone(arg), std::make_unique<Node>(2.0));
                    auto one_minus = std::make_unique<Node>('-', std::make_unique<Node>(1.0), std::move(u2));
                    auto sqrt_node = std::make_unique<Node>("sqrt", std::move(one_minus));
                    auto div_node = std::make_unique<Node>('/', std::make_unique<Node>(1.0), std::move(sqrt_node));
                    d_func = std::make_unique<Node>('-', std::move(div_node));
                }
                else if (node->name == "atan") {
                    auto u2 = std::make_unique<Node>('^', clone(arg), std::make_unique<Node>(2.0));
                    auto one_plus = std::make_unique<Node>('+', std::make_unique<Node>(1.0), std::move(u2));
                    d_func = std::make_unique<Node>('/', std::make_unique<Node>(1.0), std::move(one_plus));
                }
                else {
                    throw std::runtime_error("ERROR: Unknown function " + node->name);
                }

                // f'(g(x)) * g'(x)
                return std::make_unique<Node>('*', std::move(d_func), std::move(darg));
            }
        }
        throw std::runtime_error("ERROR: Derivative not implemented for this node type");
    }

public:
    Differentiator(const std::string& var) : var_(var) {}

    std::unique_ptr<Node> differentiate(const Node* node) const {
        return diff(node);
    }
};

#endif
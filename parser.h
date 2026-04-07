#ifndef PARSER_H
#define PARSER_H

#include "ast.h"
#include "lexer.h"
#include <vector>
#include <set>

class Parser {
    std::vector<Token> tokens_;
    size_t pos_;
    std::set<std::string> functions_ = {"sin", "cos", "tan", "asin", "acos", "atan", "exp", "log", "sqrt"};     //список возможных функций

    Token peek() const { return pos_ < tokens_.size() ? tokens_[pos_] : Token(lexem_t::EOEX); }
    void advance() { pos_++; }
    bool match(lexem_t type) { if (peek().type() == type) { advance(); return true; } return false; }                       //вспомогательные функции для кидания ошибок
    void expect(lexem_t type, const std::string& msg) { if (!match(type)) throw std::runtime_error("ERROR: " + msg); }
    void error(const std::string& msg) { throw std::runtime_error("ERROR: " + msg); }

    std::unique_ptr<Node> parseExpression();
    std::unique_ptr<Node> parseTerm();
    std::unique_ptr<Node> parsePower();
    std::unique_ptr<Node> parseUnary();
    std::unique_ptr<Node> parsePrimary();

public:
    Parser(const std::vector<Token>& tokens) : tokens_(tokens), pos_(0) {}
    std::unique_ptr<Node> parse();
};

//парсим бинарные плюсики и минусики
inline std::unique_ptr<Node> Parser::parseExpression() {
    auto node = parseTerm();
    while (peek().type() == lexem_t::PLUS || peek().type() == lexem_t::MINUS) {
        char op = (peek().type() == lexem_t::PLUS) ? '+' : '-';
        advance();
        auto right = parseTerm();
        node = std::make_unique<Node>(op, std::move(node), std::move(right));
    }
    return node;
}

//парсим бинарные умножения деления
inline std::unique_ptr<Node> Parser::parseTerm() {
    auto node = parsePower();  // КЛЮЧЕВОЕ: Term → Power, не Unary
    while (peek().type() == lexem_t::MULTIPLY || peek().type() == lexem_t::DIVIDE) {
        char op = (peek().type() == lexem_t::MULTIPLY) ? '*' : '/';
        advance();
        auto right = parsePower();
        node = std::make_unique<Node>(op, std::move(node), std::move(right));
    }
    return node;
}

//парсим степени
inline std::unique_ptr<Node> Parser::parsePower() {
    auto node = parseUnary();  // Основание может иметь унарный +/-
    if (peek().type() == lexem_t::POWER) {
        advance();
        auto right = parsePower();  // Рекурсия для правоассоциативности + унарные в экспоненте
        node = std::make_unique<Node>('^', std::move(node), std::move(right));
    }
    return node;
}

//парсим унарные плюсеки минусики
inline std::unique_ptr<Node> Parser::parseUnary() {
    if (peek().type() == lexem_t::PLUS || peek().type() == lexem_t::MINUS) {
        char op = (peek().type() == lexem_t::PLUS) ? '+' : '-';
        advance();
        auto operand = parsePower();  // Унарный оператор применяется к степени
        if (op == '+') {
            return operand;
        } else {
            return std::make_unique<Node>('-', std::move(operand));
        }
    }
    return parsePrimary();//страдаем рекурсивно
}
//парсим остатки
inline std::unique_ptr<Node> Parser::parsePrimary() {
    Token tok = peek();

    if (tok.type() == lexem_t::NUMBER) {
        advance();
        return std::make_unique<Node>(tok.number());
    }

    if (tok.type() == lexem_t::VARIABLE) {
        std::string name = tok.value();
        advance();

        if (functions_.find(name) != functions_.end()) {
            expect(lexem_t::LPAREN, "Expected '(' after function");
            auto arg = parseExpression();
            expect(lexem_t::RPAREN, "Expected ')'");
            return std::make_unique<Node>(name, std::move(arg));
        }
        return std::make_unique<Node>(name);
    }

    if (tok.type() == lexem_t::LPAREN) {
        advance();
        auto node = parseExpression();
        expect(lexem_t::RPAREN, "Expected ')'");
        return node;
    }

    error("Unexpected token");
    return nullptr;
}

inline std::unique_ptr<Node> Parser::parse() {
    auto result = parseExpression();
    if (peek().type() != lexem_t::EOEX) {
        error("Unexpected tokens after expression");
    }
    return result;
}

#endif
#ifndef PARSER_H
#define PARSER_H

#include "ast.h"
#include "lexer.h"
#include <vector>
#include <set>

class Parser {
    std::vector<Token> tokens_;     //список токенов
    size_t pos_;                    //текущая позиция
    std::set<std::string> functions_ = {"sin", "cos", "tan", "asin", "acos", "atan", "exp", "log", "sqrt"};     //возможные функции

    Token peek() const { return pos_ < tokens_.size() ? tokens_[pos_] : Token(lexem_t::EOEX); }                     //смотрим текущий токен
    void advance() { pos_++; }                                                                                              //для прохождения по токенам
    bool match(lexem_t type) { if (peek().type() == type) { advance(); return true; } return false; }                       //если все хорошо с типом - идем дальше
    void expect(lexem_t type, const std::string& msg) { if (!match(type)) throw std::runtime_error("ERROR: " + msg); }  //если тип не тот - бросаем ошибку
    void error(const std::string& msg) { throw std::runtime_error("ERROR: " + msg); }                                   //тоже для обработки ошибок

    //храним методы парсинга, каждому методу соответствует свой узел в дереве

    std::unique_ptr<Node> parseExpression();    //парсинг плюсиков минусиков
    std::unique_ptr<Node> parseTerm();          //парсинг умножения деления
    std::unique_ptr<Node> parsePower();         //парсинг степеней
    std::unique_ptr<Node> parseFactor();        //парсинг унарных плюсиков минусиков
    std::unique_ptr<Node> parsePrimary();       //чиселка,

public:
    Parser(const std::vector<Token>& tokens) : tokens_(tokens), pos_(0) {}
    std::unique_ptr<Node> parse();
};

//парсинг плюсиков минусиков
inline std::unique_ptr<Node> Parser::parseExpression() {
    auto node = parseTerm();                                        //создаем левый одночлен
    while (peek().type() == lexem_t::PLUS || peek().type() == lexem_t::MINUS) {
        char op = (peek().type() == lexem_t::PLUS) ? '+' : '-';                     //заносим операцию
        advance();
        auto right = parseTerm();                                   //создаем правый одночлен
        node = std::make_unique<Node>(op, std::move(node), std::move(right));   //воссоединяем их
    }
    return node;
}
//парсинг одночленов
inline std::unique_ptr<Node> Parser::parseTerm() {                                  //аналогично
    auto node = parsePower();
    while (peek().type() == lexem_t::MULTIPLY || peek().type() == lexem_t::DIVIDE) {
        char op = (peek().type() == lexem_t::MULTIPLY) ? '*' : '/';
        advance();
        auto right = parsePower();
        node = std::make_unique<Node>(op, std::move(node), std::move(right));
    }
    return node;
}
//парсинг степени
inline std::unique_ptr<Node> Parser::parsePower() {                                 //аналогично
    auto node = parseFactor();
    if (peek().type() == lexem_t::POWER) {
        advance();
        auto right = parsePower();
        node = std::make_unique<Node>('^', std::move(node), std::move(right));
    }
    return node;
}
//парсинг унарных плюсиков минусиков
inline std::unique_ptr<Node> Parser::parseFactor() {
    if (peek().type() == lexem_t::PLUS || peek().type() == lexem_t::MINUS) {
        char op = (peek().type() == lexem_t::PLUS) ? '+' : '-';
        advance();
        auto operand = parsePrimary();
        return std::make_unique<Node>(op, std::move(operand));
    }
    return parsePrimary();
}
//парсинг всего оставшегося
inline std::unique_ptr<Node> Parser::parsePrimary() {
    Token tok = peek();

    if (tok.type() == lexem_t::NUMBER) {                //если чиселка, создаем узел - чиселку
        advance();
        return std::make_unique<Node>(tok.number());
    }

    if (tok.type() == lexem_t::VARIABLE) {              //если переменная, создаем узел - переменную
        std::string name = tok.value();
        advance();

        if (functions_.find(name) != functions_.end()) {                        //ищем название функции в функциях
            expect(lexem_t::LPAREN, "Expected '(' after function"); //если не левая скобка - кикаем
            auto arg = parseExpression();                        //парсим выражение в скобках
            expect(lexem_t::RPAREN, "Expected ')'");                //если не парвая скобка - кикаем
            return std::make_unique<Node>(name, std::move(arg));                //создаем узелочек
        }
        return std::make_unique<Node>(name);
    }

    if (tok.type() == lexem_t::LPAREN) {        //нашли левую скобку
        advance();
        auto node = parseExpression();          //парсим выражение внутри
        expect(lexem_t::RPAREN, "Expected ')'");        //ожидаем найти правую скобку, если не нашли - кикаем
        return node;
    }

    error("Unexpected token");
    return nullptr;
}

inline std::unique_ptr<Node> Parser::parse() {
    auto result = parseExpression();
    if (peek().type() != lexem_t::EOEX) error("Unexpected tokens after expression");
    return result;
}

#endif
#ifndef LEXER_H
#define LEXER_H

#include <string>
#include <vector>
#include <cctype>
#include <stdexcept>

enum class lexem_t { //класс типов лексем
    NUMBER, VARIABLE, PLUS, MINUS, MULTIPLY, DIVIDE, POWER, LPAREN, RPAREN, EOEX
};

class Token { //класс токена (тип + значение)
    lexem_t type_;
    std::string value_;
public:
    Token(lexem_t type, const std::string& value = "") : type_(type), value_(value) {}      //методы
    lexem_t type() const { return type_; }
    std::string value() const { return value_; }
    double number() const { return std::stod(value_); }
    std::string view() const { return value_.empty() ? std::string(1, char(type_)) : value_; }
};

class Lexer {       //класс лексера
    std::string input_;     //входная строка
    size_t pos_;            //текущая позиция

    char peek() const { return pos_ < input_.size() ? input_[pos_] : '\0'; }        //смотрим текущую позицию, если больше длины строки - выдаем конец
    void advance() { if (pos_ < input_.size()) pos_++; }                            //переходим к следующему символу

    void skipWhitespace() {
        while (std::isspace(static_cast<unsigned char>(peek()))) advance();         //пропускаем все пробелы
    }

    Token readNumber() {                            //пытаемся читать символ
        std::string result;
        bool zero_first = (peek() == '0');      //проверка на первый 0

        while (std::isdigit(peek())) {
            result += peek();
            advance();
        }

        if (zero_first && result.size() > 1) {      //мы все еще в числе, первый символ 0, размер больше чем 1 - че за бред то
            throw std::runtime_error("error: leading zeros!! you should learn how to write numbers!!");
        }

        if (peek() == '.') {                        //если текущий символ точка, добавляем ее и идем дальше
            result += '.';
            advance();
            if (!std::isdigit(peek())) throw std::runtime_error("error: after dot should be numbers!!");    //если после точки не чиселки - кикаем
            while (std::isdigit(peek())) {                                                                  //если все хорошо - идем дальше
                result += peek();
                advance();
            }
        }

        if (std::isalpha(peek())) {             //если в чиселке затесались буквы - кикаем
            throw std::runtime_error("error: after numbers should be NUMBERS!! NOT LETTERS!!");
        }

        return Token(lexem_t::NUMBER, result);      //возвращаем четенькое чиселко
    }

    Token readIdentifier() {
        std::string result;
        if (!std::isalpha(peek()) && peek() != '_') {       //если символ число и не _ - кикаем (переменные не должны так начинаться)
            throw std::runtime_error("error: identifier must start with letter");
        }

        while (std::isalnum(peek()) || peek() == '_') {
            result += std::tolower(peek());
            advance();
        }

        return Token(lexem_t::VARIABLE, result);
    }

public:
    Lexer(const std::string& input) : input_(input), pos_(0) {}     //по символу возвращаем тип

    Token next() {
        skipWhitespace();
        if (pos_ >= input_.size()) return Token(lexem_t::EOEX);

        char c = peek();

        if (c == '+') { advance(); return Token(lexem_t::PLUS); }
        if (c == '-') { advance(); return Token(lexem_t::MINUS); }
        if (c == '*') { advance(); return Token(lexem_t::MULTIPLY); }
        if (c == '/') { advance(); return Token(lexem_t::DIVIDE); }
        if (c == '^') { advance(); return Token(lexem_t::POWER); }
        if (c == '(') { advance(); return Token(lexem_t::LPAREN); }
        if (c == ')') { advance(); return Token(lexem_t::RPAREN); }

        if (std::isdigit(c)) return readNumber();
        if (std::isalpha(c) || c == '_') return readIdentifier();

        throw std::runtime_error(std::string("error: what kind of character is this??? ") + c);
    }
};

#endif
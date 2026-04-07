#pragma once
#include <memory>
#include <string>
#include <vector>


enum class NodeType {       //класс типа узла
    NUMBER,         //чиселка
    VARIABLE,       //переменная
    BINARY_OP,      //бинарная операция
    UNARY_OP,       //унарная операция
    FUNCTION        //функция
};


struct Node {           //структура узла
    NodeType type;      //тип узла
    double num_value;   //значение, если это чиселка
    std::string name;   //имя переменной, если это она
    char op;            //тип операции, если это она

    std::unique_ptr<Node> left;     //левое дитя
    std::unique_ptr<Node> right;    //правое дитя

//конструктор для чиселки
    Node(double val) : type(NodeType::NUMBER), num_value(val), op(0) {} //тип - число, значение - значение, операция никакая

//конструктор для переменной
    Node(const std::string& var_name) : type(NodeType::VARIABLE), name(var_name), op(0) {} //тип - переменная, имя - имя, операция - никакая

//конструктор для бинарной операции
    Node(char operation, std::unique_ptr<Node> l, std::unique_ptr<Node> r)
        : type(NodeType::BINARY_OP), op(operation), left(std::move(l)), right(std::move(r)) {} //у нее есть оба потомка, создаем для них умные ссылки, тип - бинарная операция, операция - операция

//конструктор для унарной операции
    Node(char operation, std::unique_ptr<Node> operand)
        : type(NodeType::UNARY_OP), op(operation), left(std::move(operand)) {} //создаем умную ссылку на потомка, тип - бинарная операция

//конструктор для функции
    Node(const std::string& func_name, std::unique_ptr<Node> arg)
        : type(NodeType::FUNCTION), name(func_name), left(std::move(arg)) {} //тоже ток один потомок, тип - функция


    std::string toString() const {      //превращаем в строчку
        switch(type) {
        case NodeType::NUMBER:          //если чиселка, то возвращаем ее как строку
            return std::to_string(num_value);
        case NodeType::VARIABLE:        //если переменная, возвращаем ее
            return name;
        case NodeType::UNARY_OP:        //если унарная операция - ее + левого потомка
            return std::string(1, op) + left->toString();
        case NodeType::BINARY_OP:       //если бинарная - сумма строк левого и правого потомков
            return "(" + left->toString() + " " + op + " " + right->toString() + ")";
        case NodeType::FUNCTION:        //если функция, ее + ее аргумент
            return name + "(" + left->toString() + ")";
        }
        return ""; //в ином случае ниче
    }

    Node(const Node& other)     //конструктор копирования узелочка
        : type(other.type)
        , num_value(other.num_value)
        , name(other.name)
        , op(other.op)
        , left(other.left ? std::make_unique<Node>(*other.left) : nullptr)
        , right(other.right ? std::make_unique<Node>(*other.right) : nullptr)
    {}
};
//
// Created by noyex on 24-3-23.
//

#ifndef COY_LEXER_H
#define COY_LEXER_H

#include <string>
#include <sstream>
#include <regex>
#include <list>
#include "Token.h"

namespace coy {

    const std::regex IDENTIFIER("^[a-zA-Z_][a-zA-Z0-9_]*");
    const std::regex INTEGER("^[0-9]+");
    const std::regex FLOAT("^[0-9]+\\.[0-9]+");
    const std::regex OPERATOR(R"(^([\+\-\*\/]|==|!=|<=|>=|<|>|=|&&|\|\|))");
    const std::regex SEPARATOR(R"(^[\(\)\[\]\{\};,])");
    const std::regex DATA_TYPE(R"(^(int|float|char|bool|void))");
    const std::regex KEY_WORD(R"(^(if|else|while))");
    
    //TODO: 补充其他正则表达式

    const std::list<std::pair<std::regex, int>> DEFAULT_PATTERNS = {
            {FLOAT,      TYPE_FLOAT},
            {INTEGER,    TYPE_INTEGER},
            {OPERATOR,   TYPE_OPERATOR},
            {SEPARATOR,  TYPE_SEPARATOR},
            {DATA_TYPE,  TYPE_DATA_TYPE},
            {KEY_WORD,   TYPE_KEYWORD},
            {IDENTIFIER, TYPE_IDENTIFIER},
    };

    class Lexer {
    private:
        std::stringstream stream;
        std::list<std::pair<std::regex, int>> patterns;
    public:
        explicit Lexer(const std::string &string, std::list<std::pair<std::regex, int>> patterns = DEFAULT_PATTERNS);

        Token nextToken();

        std::vector<Token> tokenize();
    };

} // coy

#endif //COY_LEXER_H

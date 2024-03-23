//
// Created by noyex on 24-3-23.
//

#ifndef COY_PARSER_H
#define COY_PARSER_H

#include <string>
#include <functional>
#include <optional>
#include <utility>
#include <set>

#include "Token.h"
#include "Node.h"

namespace coy {

    struct ParseResult {
        Node *value;
        bool success{};
        std::string message;
    };

    class Parser {
    private:
        std::function<ParseResult(const std::vector<Token> &, int, int)> _parseFunction;
    public:
        explicit Parser(std::function<ParseResult(const std::vector<Token> &, int, int)> parseFunction);
        Parser(const Parser &other);

        [[nodiscard]] ParseResult parse(const std::vector<Token> &tokens) const;

        [[nodiscard]] ParseResult parse(const std::vector<Token> &tokens, int from, int to) const;

        [[nodiscard]] Parser orElse(const Parser &other) const;

    };


    Parser createBinaryOperatorParser(const std::set<std::string> &ops);

    extern Parser parseInteger;
    extern Parser parseFloat;
    extern Parser parseNumber;
    extern Parser parseAddSub;
    extern Parser parseMulDiv;
    extern Parser parseExpression;

} // coy

#endif //COY_PARSER_H

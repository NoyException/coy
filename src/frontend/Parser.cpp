//
// Created by noyex on 24-3-23.
//

#include "Parser.h"

namespace coy {

    Parser::Parser(std::function<ParseResult(const std::vector<Token> &, int, int)> parseFunction) : _parseFunction(
            std::move(parseFunction)) {
        if(_parseFunction == nullptr) {
            throw std::invalid_argument("parseFunction cannot be null");
        }
    }

    Parser::Parser(const Parser &other) : _parseFunction(other._parseFunction) {
        if(_parseFunction == nullptr) {
            throw std::invalid_argument("parseFunction cannot be null");
        }
    }

    ParseResult Parser::parse(const std::vector<Token> &tokens) const {
        return _parseFunction(tokens, 0, (int) tokens.size());
    }

    ParseResult Parser::parse(const std::vector<Token> &tokens, int from, int to) const {
        if (from < 0 || to > tokens.size()) {
            throw std::out_of_range("Index out of range");
        }
        return _parseFunction(tokens, from, to);
    }

    Parser Parser::orElse(const Parser &other) const {
        return Parser([&](const std::vector<Token> &tokens, int from, int to) {
            auto result = this->parse(tokens, from, to);
            if (result.success) {
                return result;
            } else {
                return other.parse(tokens, from, to);
            }
        });
    }
    
    Parser createBinaryOperatorParser(const std::set<std::string> &ops) {
        return Parser([&](const std::vector<Token> &tokens, int from, int to){
            if (to - from < 3) {
                return ParseResult{nullptr, false, "Not enough tokens"};
            }
            int opIndex = -1;
            for (int i = from + 1; i < to - 1; ++i) {
                if (tokens[i].type == TYPE_OPERATOR && ops.count(tokens[i].value) > 0) {
                    opIndex = i;
                    break;
                }
            }
            if (opIndex == -1) {
                return ParseResult{nullptr, false, "Not an expected operator"};
            }

            auto left = parseExpression.parse(tokens, from, opIndex);
            auto right = parseExpression.parse(tokens, opIndex + 1, to);

            if (!left.success || !right.success) {
                return ParseResult{nullptr, false, "Invalid operands"};
            }
            return ParseResult{new NodeBinaryOperator(left.value, right.value, tokens[opIndex].value), true, ""};
        });
    }

    Parser parseInteger = Parser([](const std::vector<Token> &tokens, int from, int to) {
        if (to - from != 1) {
            return ParseResult{nullptr, false, "Invalid number of tokens"};
        }
        if (tokens[from].type != TYPE_NUMBER) {
            return ParseResult{nullptr, false, "Not a number"};
        }
        return ParseResult{new NodeInteger(std::stoi(tokens[from].value)), true, ""};
    });

    Parser parseFloat = Parser([](const std::vector<Token> &tokens, int from, int to)  {
        if (to - from != 1) {
            return ParseResult{nullptr, false, "Invalid number of tokens"};
        }
        if (tokens[from].type != TYPE_NUMBER) {
            return ParseResult{nullptr, false, "Not a number"};
        }
        return ParseResult{new NodeFloat(std::stof(tokens[from].value)), true, ""};
    });

    Parser parseNumber = parseInteger.orElse(parseFloat);

    std::set<std::string> addSubOps = {"+", "-"};
    std::set<std::string> mulDivOps = {"*", "/"};
    Parser parseAddSub = createBinaryOperatorParser(addSubOps);
    Parser parseMulDiv = createBinaryOperatorParser(mulDivOps);

    static Parser temp = parseAddSub.orElse(parseMulDiv);
    Parser parseExpression = temp.orElse(parseNumber);
} // coy
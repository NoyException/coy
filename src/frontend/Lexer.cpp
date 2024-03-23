//
// Created by noyex on 24-3-23.
//

#include "Lexer.h"
#include <regex>

namespace coy {

    Lexer::Lexer(const std::string &string, std::list<std::pair<std::regex, int>> patterns) : stream(string), patterns(std::move(patterns)) {

    }

    Token Lexer::nextToken() {
        stream >> std::ws;
        if (stream.eof()) {
            return {TYPE_EOF, ""};
        }
        std::string token;
        stream >> token;
        for (const auto & pair : patterns) {
            std::smatch match;
            if (std::regex_search(token, match, pair.first, std::regex_constants::match_continuous)) {
                //将没有匹配的部分放回流中
                size_t len = match[0].length();
                for (size_t i = token.length()-1; i >= len; --i) {
                    stream.putback(token[i]);
                }
                return {pair.second, token.substr(0, len)};
            }
        }
        return {TYPE_UNKNOWN, token};
    }

    std::vector<Token> Lexer::tokenize() {
        std::vector<Token> tokens;
        while (true) {
            auto token = nextToken();
            if (token.type == TYPE_EOF) {
                break;
            }
            tokens.push_back(token);
        }
        return tokens;
    }

} // coy
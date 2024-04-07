//
// Created by noyex on 24-3-23.
//

#include "Lexer.h"
#include <regex>
#include <utility>

namespace coy {

    Lexer::Lexer(std::string content, std::list<std::pair<std::regex, int>> patterns) : _content(std::move(content)), _patterns(std::move(patterns)) {

    }

    Token Lexer::nextToken() {
        skipSpace();
        if (_index >= _content.size()) {
            return {TYPE_EOF, _index, ""};
        }
        for (const auto & pair : _patterns) {
            std::smatch match;
            std::string toMatch = _content.substr(_index);
            if (std::regex_search(toMatch, match, pair.first, std::regex_constants::match_continuous)) {
                std::string token = match[0];
                size_t len = token.length();
                _index += len;
                return {pair.second, _index-len, token};
            }
        }
        _index++;
        return {TYPE_UNKNOWN, _index-1, _content.substr(_index-1, _index)};
    }

    std::vector<Token> Lexer::tokenize(const std::function<bool(const Token &)>& filter) {
        std::vector<Token> tokens;
        while (true) {
            auto token = nextToken();
            if (!filter(token)) {
                continue;
            }
            if (token.type == TYPE_EOF) {
                break;
            }
            tokens.push_back(token);
        }
        return tokens;
    }

    void Lexer::skipSpace() {
        const std::string WHITESPACE = " \n\r\t\f\v";
        std::string content = _content.substr(_index);
        size_t start = content.find_first_not_of(WHITESPACE);
        if (start == std::string::npos) {
            _index = _content.size();
            return;
        }
        _index += start;
    }

} // coy
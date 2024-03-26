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
        trim();
        if (_content.empty()) {
            return {TYPE_EOF, ""};
        }
        for (const auto & pair : _patterns) {
            std::smatch match;
            if (std::regex_search(_content, match, pair.first, std::regex_constants::match_continuous)) {
                //将匹配的部分从_content中删除
                size_t len = match[0].length();
                std::string token = match[0];
                _content = _content.substr(len);
                return {pair.second, token.substr(0, len)};
            }
        }
        return {TYPE_UNKNOWN, _content.substr(0, 1)};
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

    void Lexer::trim() {
        const std::string WHITESPACE = " \n\r\t\f\v";
        size_t start = _content.find_first_not_of(WHITESPACE);
        if (start == std::string::npos) {
            // _content 全是空格
            _content.clear();
            return;
        }
        size_t end = _content.find_last_not_of(WHITESPACE);
        _content = _content.substr(start, end - start + 1);
    }

} // coy
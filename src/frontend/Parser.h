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
#include "Lexer.h"

namespace coy {

    template<typename I>
    class Input {
    private:
        std::shared_ptr<std::vector<I>> _data;
        int _index = 0;
    public:
        explicit Input(const std::shared_ptr<std::vector<I>> &data, int index = 0)
                : _data(data), _index(index) {}

        [[nodiscard]] I current() const {
            if (_index < 0 || _index >= _data->size())
                throw std::runtime_error("Index out of range");
            return _data->at(_index);
        }

        [[nodiscard]] Input next(int n = 1) const {
            return Input(_data, _index + n);
        }

        [[nodiscard]] bool end() const {
            return _index >= _data->size();
        }

        Input operator++(int) {
            return next();
        }

        Input operator+(int n) {
            return next(n);
        }
    };

    template<typename I, typename O>
    class Output {
    private:
        const bool _success;
        std::shared_ptr<O> _data;
        std::function<std::string()> _message;
        std::optional<Input<I>> _next;
    public:
        Output(bool success, const std::shared_ptr<O> &data, std::function<std::string()> message,
               std::optional<Input<I>> next)
                : _success(success), _data(data), _message(std::move(message)), _next(std::move(next)) {}

        static Output success(const std::shared_ptr<O> &data, Input<I> next) {
            return Output(true, data, [] { return ""; }, next);
        }

        static Output fail(std::string message) {
            return Output(false, nullptr, [message] { return message; }, std::nullopt);
        }

        [[nodiscard]] bool isSuccess() const {
            return _success;
        }

        [[nodiscard]] std::shared_ptr<O> data() const {
            return _data;
        }

        [[nodiscard]] std::string message() const {
            return _message();
        }

        [[nodiscard]] std::optional<Input<I>> next() const {
            return _next;
        }
    };


    template<typename I, typename O>
    class Parser : public std::enable_shared_from_this<Parser<I, O>> {
    private:
        std::function<Output<I, O>(Input<I>)> _parseFunction;
    public:
        explicit Parser(std::function<Output<I, O>(Input<I>)> parseFunction) : _parseFunction(
                std::move(parseFunction)) {}

        [[nodiscard]] Output<I, O> parse(Input<I> input) const {
            return _parseFunction(input);
        }
        
        void set(std::function<Output<I, O>(Input<I>)> parseFunction){
            _parseFunction = parseFunction;
        }

        Parser& operator=(std::function<Output<I, O>(Input<I>)> parseFunction){
            _parseFunction = parseFunction;
        }

        template<typename O2>
        std::shared_ptr<Parser<I, O2>> as() {
            auto self = this->shared_from_this();
            return std::make_shared<Parser<I, O2>>([self](Input<I> input) -> Output<I, O2> {
                auto result = self->parse(input);
                if (result.isSuccess()) {
                    std::shared_ptr<O2> data = std::dynamic_pointer_cast<O2>(result.data());
                    return Output<I, O2>::success(data, result.next().value());
                } else {
                    return Output<I, O2>::fail(result.message());
                }
            });
        }

        /**
         * 返回一个功能如下的Parser：先尝试解析当前Parser，如果失败则尝试解析other。
         * @param other 
         * @return 
         */
        [[nodiscard]] std::shared_ptr<Parser> orElse(const std::shared_ptr<Parser> &other) const {
            auto self = this->shared_from_this();
            return std::make_shared<Parser>([self, other](Input<I> input) -> Output<I, O> {
                auto result = self->parse(input);
                if (result.isSuccess()) {
                    return result;
                } else {
                    return other->parse(input);
                }
            });
        }

        /**
         * 返回一个功能如下的Parser：先尝试解析当前Parser，如果成功则尝试解析other。
         * 第二个Parser的解析结果会被返回。
         * @param other 
         * @return 
         */
        template<typename O2>
        [[nodiscard]] std::shared_ptr<Parser<I, O2>> then(const std::shared_ptr<Parser<I, O2>> &other) const {
            auto self = this->shared_from_this();
            return std::make_shared<Parser<I, O2>>([self, other](Input<I> input) -> Output<I, O2> {
                auto result = self->parse(input);
                if (result.isSuccess()) {
                    return other->parse(result.next().value());
                } else {
                    return Output<I, O2>::fail(result.message());
                }
            });
        }

        /**
         * 返回一个功能如下的Parser：将当前Parser解析出的结果映射为另一种类型。
         * @param other 
         * @return 
         */
        template<typename O2>
        [[nodiscard]] std::shared_ptr<Parser<I, O2>>
        map(const std::function<std::shared_ptr<O2>(const std::shared_ptr<O> &)> &mapper) const {
            auto self = this->shared_from_this();
            return std::make_shared<Parser<I, O2>>([self, mapper](Input<I> input) -> Output<I, O2> {
                auto result = self->parse(input);
                if (result.isSuccess()) {
                    return Output<I, O2>::success(mapper(result.data()), result.next().value());
                } else {
                    return Output<I, O2>::fail(result.message());
                }
            });
        }

        /**
         * 返回一个功能如下的Parser：将当前Parser解析出的结果绑定到另一个Parser，然后解析另一个Parser。
         * @param binder 
         * @return 
         */
        template<typename O2>
        [[nodiscard]] std::shared_ptr<Parser<I, O2>>
        bind(const std::function<std::shared_ptr<Parser<I, O2>>(const std::shared_ptr<O> &)> &binder) const {
            auto self = this->shared_from_this();
            return std::make_shared<Parser<I, O2>>([self, binder](Input<I> input) -> Output<I, O2> {
                auto result = self->parse(input);
                if (result.isSuccess()) {
                    return binder(result.data())->parse(result.next().value());
                } else {
                    return Output<I, O2>::fail(result.message());
                }
            });
        }

        /**
         * 返回一个功能如下的Parser：先尝试解析当前Parser，如果成功则尝试解析other，如果other也成功则返回之前的结果。
         * @tparam O2 
         * @param other 
         * @return 
         */
        template<typename O2>
        std::shared_ptr<Parser<I, O>> skip(const std::shared_ptr<Parser<I, O2>> &other) const {
            auto self = this->shared_from_this();
            return std::make_shared<Parser<I, O>>([self, other](Input<I> input) -> Output<I, O> {
                auto result = self->parse(input);
                if (result.isSuccess()) {
                    auto next = other->parse(result.next().value());
                    if (next.isSuccess()) {
                        return Output<I, O>::success(result.data(), next.next().value());
                    } else {
                        return Output<I, O>::fail(next.message());
                    }
                } else {
                    return Output<I, O>::fail(result.message());
                }
            });
        }

        /**
         * 左结合，this (op this)*
         * @param op 
         * @return 
         */
        std::shared_ptr<Parser<I, O>>
        chainLeft(const std::shared_ptr<Parser<I, std::function<std::shared_ptr<O>(const std::shared_ptr<O> &,
                                                                                   const std::shared_ptr<O> &)>>> &op) const {
            auto self = this->shared_from_this();
            return std::make_shared<Parser<I, O>>([self, op](Input<I> input) -> Output<I, O> {
                auto result = self->parse(input);
                if (result.isSuccess()) {
                    std::shared_ptr<O> left = result.data();
                    Input<I> current = result.next().value();
                    while (true) {
                        auto next = op->parse(current);
                        if (next.isSuccess()) {
                            auto right = self->parse(next.next().value());
                            if (right.isSuccess()) {
                                left = (*next.data())(left, right.data());
                                current = right.next().value();
                            } else {
                                return Output<I, O>::fail(right.message());
                            }
                        } else {
                            return Output<I, O>::success(left, current);
                        }
                    }
                } else {
                    return Output<I, O>::fail(result.message());
                }
            });
        }
    };

    class Parsers {
    public:
        /**
         * 返回一个空Parser
         * @tparam I 
         * @tparam O 
         * @return 
         */
        template<typename I, typename O>
        static std::shared_ptr<Parser<I, O>> lazy(){
            return std::make_shared<Parser<I, O>>([](Input<I> input) -> Output<I, O> {
                return Output<I, O>::fail("Not implemented");
            });
        }

        /**
         * 返回一个功能如下的Parser：如果输入已经结束，则返回一个成功的结果。
         * @tparam I 
         * @tparam O 
         * @return 
         */
        template<typename I, typename O>
        static std::shared_ptr<Parser<I, O>> end() {
            return std::make_shared<Parser<I, O>>([](Input<I> input) -> Output<I, O> {
                if (input.end()) {
                    return Output<I, O>::success(nullptr, input);
                } else {
                    return Output<I, O>::fail("End expected");
                }
            });
        }

        /**
         * 返回一个功能如下的Parser：不论输入如何，都返回一个成功的结果，结果为data。
         * @tparam I 
         * @tparam O 
         * @param data 
         * @return 
         */
        template<typename I, typename O>
        static std::shared_ptr<Parser<I, O>> pure(const std::shared_ptr<O> &data) {
            return std::make_shared<Parser<I, O>>([data](Input<I> input) -> Output<I, O> {
                return Output<I, O>::success(data, input);
            });
        }

        /**
         * 返回一个功能如下的Parser：不论输入如何，都返回一个失败的结果，结果为message。
         * @tparam I 
         * @tparam O 
         * @param message 
         * @return 
         */
        template<typename I, typename O>
        static std::shared_ptr<Parser<I, O>> fail(const std::string &message) {
            return std::make_shared<Parser<I, O>>([message](Input<I>) -> Output<I, O> {
                return Output<I, O>::fail(message);
            });
        }

        /**
         * 返回一个功能如下的Parser：解析一个任意输入，返回一个成功的结果，结果为当前输入。
         * @tparam I 
         * @tparam O 
         * @return 
         */
        template<typename I, typename O>
        static std::shared_ptr<Parser<I, O>> skip() {
            return std::make_shared<Parser<I, O>>([](Input<I> input) -> Output<I, O> {
                return Output<I, O>::success(std::make_shared<O>(input.current()), input + 1);
            });
        }

        /**
         * 返回一个功能如下的Parser：按照顺序尝试解析多个Parser，返回第一个成功的结果。
         * @tparam I 
         * @tparam O 
         * @param parsers 
         * @return 
         */
        template<typename I, typename O>
        static std::shared_ptr<Parser<I, O>> any(std::initializer_list<std::shared_ptr<Parser<I, O>>> parsers) {
            std::list<std::shared_ptr<Parser<I, O>>> list(parsers);
            return std::make_shared<Parser<I, O>>([list](Input<I> input) -> Output<I, O> {
                for (const auto &parser: list) {
                    auto result = parser->parse(input);
                    if (result.isSuccess()) {
                        return result;
                    }
                }
                return Output<I, O>::fail("No parser matched");
            });
        }

        /**
         * 返回一个功能如下的Parser：尝试多次解析一个Parser，返回一个成功的结果，结果为所有解析结果的列表。
         * 如果atLeastOne为true，则至少要解析一次。
         * @tparam I 
         * @tparam O 
         * @param parser 
         * @param atLeastOne 
         * @return 
         */
        template<typename I, typename O>
        static std::shared_ptr<Parser<I, std::vector<std::shared_ptr<O>>>> many(const std::shared_ptr<Parser<I, O>> &parser, bool atLeastOne = false) {
            return std::make_shared<Parser<I, std::vector<std::shared_ptr<O>>>>([parser, atLeastOne](Input<I> input) -> Output<I, std::vector<std::shared_ptr<O>>> {
                std::vector<std::shared_ptr<O>> results;
                Input<I> current = input;
                while (true) {
                    auto result = parser->parse(current);
                    if (result.isSuccess()) {
                        results.push_back(result.data());
                        current = result.next().value();
                    } else {
                        if (results.empty() && atLeastOne) {
                            return Output<I, std::vector<std::shared_ptr<O>>>::fail("At least one expected");
                        } else {
                            return Output<I, std::vector<std::shared_ptr<O>>>::success(std::make_shared<std::vector<std::shared_ptr<O>>>(results), current);
                        }
                    }
                }
            });
        }
        
        template<typename I, typename O>
        static std::shared_ptr<Parser<I, O>> sequence(std::initializer_list<std::shared_ptr<Parser<I, O>>> parsers) {
            return std::make_shared<Parser<I, O>>([parsers](Input<I> input) -> Output<I, O> {
                std::vector<std::shared_ptr<O>> results;
                Input<I> current = input;
                for (const auto &parser: parsers) {
                    auto result = parser->parse(current);
                    if (result.isSuccess()) {
                        results.push_back(result.data());
                        current = result.next().value();
                    } else {
                        return Output<I, O>::fail(result.message());
                    }
                }
                return Output<I, O>::success(std::make_shared<O>(results), current);
            });
        }

        /**
         * 返回一个功能如下的Parser：解析一个满足条件的输入，返回一个成功的结果，结果为当前输入。
         * @tparam I 
         * @tparam O 
         * @param predicate 
         * @return 
         */
        template<typename I>
        static std::shared_ptr<Parser<I, I>> satisfy(const std::function<bool(const I &)> &predicate) {
            return std::make_shared<Parser<I, I>>([predicate](Input<I> input) -> Output<I, I> {
                if (input.end())
                    return Output<I, I>::fail("End of input");
                if (predicate(input.current())) {
                    return Output<I, I>::success(std::make_shared<I>(input.current()), input + 1);
                } else {
                    return Output<I, I>::fail("Predicate not satisfied");
                }
            });
        }
    };

    class CoyParsers {
    public:

        using BinaryOperator = std::function<std::shared_ptr<Node>(const std::shared_ptr<Node>&,const std::shared_ptr<Node>&)>;
        static const std::shared_ptr<Parser<Token, NodeIdentifier>> IDENTIFIER;
        static const std::shared_ptr<Parser<Token, Node>> INTEGER;
        static const std::shared_ptr<Parser<Token, Node>> FLOAT;
        static const std::shared_ptr<Parser<Token, Node>> NUMBER;
        static const std::shared_ptr<Parser<Token, BinaryOperator>> ADD_SUB;
        static const std::shared_ptr<Parser<Token, BinaryOperator>> MUL_DIV;
        static const std::shared_ptr<Parser<Token, BinaryOperator>> INEQUALITY_OPERATOR;
        static const std::shared_ptr<Parser<Token, BinaryOperator>> EQUALITY_OPERATOR;
        static const std::shared_ptr<Parser<Token, BinaryOperator>> LOGICAL_AND;
        static const std::shared_ptr<Parser<Token, BinaryOperator>> LOGICAL_OR;
        static const std::shared_ptr<Parser<Token, Token>> PLUS;
        static const std::shared_ptr<Parser<Token, Token>> MINUS;
        static const std::shared_ptr<Parser<Token, Token>> NOT;
        static const std::shared_ptr<Parser<Token, Token>> UNARY_OPERATOR;
        static const std::shared_ptr<Parser<Token, Token>> LEFT_ROUND_BRACKET;
        static const std::shared_ptr<Parser<Token, Token>> RIGHT_ROUND_BRACKET;
        static const std::shared_ptr<Parser<Token, Token>> LEFT_SQUARE_BRACKET;
        static const std::shared_ptr<Parser<Token, Token>> RIGHT_SQUARE_BRACKET;
        static const std::shared_ptr<Parser<Token, Token>> LEFT_BRACE;
        static const std::shared_ptr<Parser<Token, Token>> RIGHT_BRACE;
        static const std::shared_ptr<Parser<Token, Node>> TERM;
        static const std::shared_ptr<Parser<Token, Node>> SIGNED_TERM;
        static const std::shared_ptr<Parser<Token, Node>> PRODUCT;
        static const std::shared_ptr<Parser<Token, Node>> SUM;
        static const std::shared_ptr<Parser<Token, Node>> INEQUALITY;
        static const std::shared_ptr<Parser<Token, Node>> EQUALITY;
        static const std::shared_ptr<Parser<Token, Node>> AND_EXPRESSION;
        static const std::shared_ptr<Parser<Token, Node>> OR_EXPRESSION;
        static const std::shared_ptr<Parser<Token, Node>> EXPRESSION;
        static const std::shared_ptr<Parser<Token, Node>> ROUND_BRACKET_EXPRESSION;
        static const std::shared_ptr<Parser<Token, Node>> SQUARE_BRACKET_EXPRESSION;
        static const std::shared_ptr<Parser<Token, Node>> LEFT_VALUE;

        static const std::shared_ptr<Parser<Token, Token>> END_STATEMENT;
        static const std::shared_ptr<Parser<Token, Node>> ASSIGNMENT;
        static const std::shared_ptr<Parser<Token, Node>> STATEMENT;
        static const std::shared_ptr<Parser<Token, Node>> IF_STATEMENT;
        static const std::shared_ptr<Parser<Token, Node>> CODE_BLOCK;
        static const std::shared_ptr<Parser<Token, Node>> PARSER;
        static const int initializer;
    };

} // coy

#endif //COY_PARSER_H

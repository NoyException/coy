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

        [[nodiscard]] int getIndex() const {
            return _index;
        }

        Input operator++(int) {
            return next();
        }

        Input operator+(int n) {
            return next(n);
        }
    };

//    template<typename I>
//    class Message{
//    public:
//        virtual ~Message() = default;
//        virtual int getPosition() = 0;
//        virtual I* getSymbol() = 0;
//        virtual std::string getMessage() = 0;
//        std::shared_ptr<Message<I>> merge(std::shared_ptr<Message<I>> another);
//    };
//
//    template<typename I>
//    class MessageImpl : public Message<I>{
//    private:
//        I* _symbol;
//        int _position = 0;
//        std::string _message;
//    public:
//        MessageImpl(I* symbol, int position, std::string message)
//        : _symbol(symbol), _position(position), _message(std::move(message)){}
//    };
//
//    template<typename I>
//    std::shared_ptr<Message<I>> Message<I>::merge(std::shared_ptr<Message<I>> another) {
//        return std::make_shared<MessageImpl<I>>(this->getSymbol(), this->getPosition(),);
//    }

    template<typename I, typename O>
    class Output {
    private:
        bool _success;
        std::optional<O> _data;
        std::function<std::string()> _message;
        Input<I> _next;
    public:
        Output(bool success, const std::optional<O> &data, std::function<std::string()> message, Input<I> next)
                : _success(success), _data(data), _message(std::move(message)), _next(next) {}

        static Output success(const O &data, Input<I> next) {
            return Output(true, data, [] { return ""; }, next);
        }

        static Output fail(std::string message, Input<I> position) {
            return Output(false, std::nullopt, [message] { return message; }, position);
        }

        static Output fail(std::function<std::string()> message, Input<I> position) {
            return Output(false, std::nullopt, message, position);
        }

        Output merge(const Output<I, O> &another) {
            if (this->isSuccess()) {
                return *this;
            }
            if (another.isSuccess()) {
                return another;
            }
            auto message = this->_message;
            auto anotherMessage = another._message;
            return Output::fail([message, anotherMessage]() { return message() + "\n" + anotherMessage(); }, _next);
        }

        Output operator+(const Output<I, O> &another) {
            return merge(another);
        }

        [[nodiscard]] bool isSuccess() const {
            return _success;
        }

        [[nodiscard]] O data() const {
            return _data.value();
        }

        [[nodiscard]] std::string message() const {
            return _message();
        }

        [[nodiscard]] std::function<std::string()> messageSupplier() const {
            return _message;
        }

        [[nodiscard]] Input<I> next() const {
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

        void set(std::function<Output<I, O>(Input<I>)> parseFunction) {
            _parseFunction = parseFunction;
        }

        Parser &operator=(std::function<Output<I, O>(Input<I>)> parseFunction) {
            _parseFunction = parseFunction;
        }

        template<typename O2>
        std::shared_ptr<Parser<I, O2>> as() {
            auto self = this->shared_from_this();
            return std::make_shared<Parser<I, O2>>([self](Input<I> input) -> Output<I, O2> {
                auto result = self->parse(input);
                if (result.isSuccess()) {
                    return Output<I, O2>::success(O2(result.data()), result.next());
                } else {
                    return Output<I, O2>::fail(result.message(), result.next());
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
                    return result + other->parse(input);
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
                    return other->parse(result.next());
                } else {
                    return Output<I, O2>::fail(result.messageSupplier(), result.next());
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
        map(const std::function<O2(const O &)> &mapper) const {
            auto self = this->shared_from_this();
            return std::make_shared<Parser<I, O2>>([self, mapper](Input<I> input) -> Output<I, O2> {
                auto result = self->parse(input);
                if (result.isSuccess()) {
                    return Output<I, O2>::success(mapper(result.data()), result.next());
                } else {
                    return Output<I, O2>::fail(result.message(), result.next());
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
        bind(const std::function<std::shared_ptr<Parser<I, O2>>(const O &)> &binder) const {
            auto self = this->shared_from_this();
            return std::make_shared<Parser<I, O2>>([self, binder](Input<I> input) -> Output<I, O2> {
                auto result = self->parse(input);
                if (result.isSuccess()) {
                    return binder(result.data())->parse(result.next());
                } else {
                    return Output<I, O2>::fail(result.message(), result.next());
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
            return std::make_shared<Parser<I, O>>([self, other](Input<I> input) {
                auto result = self->parse(input);
                if (result.isSuccess()) {
                    auto next = other->parse(result.next());
                    if (next.isSuccess()) {
                        return Output<I, O>::success(result.data(), next.next());
                    } else {
                        return Output<I, O>::fail(next.message(), next.next());
                    }
                } else {
                    return Output<I, O>::fail(result.message(), result.next());
                }
            });
        }

        /**
         * 左结合，this (op this)*
         * @param op 
         * @return 
         */
        std::shared_ptr<Parser<I, O>>
        chainLeft(const std::shared_ptr<Parser<I, std::function<O(const O &, const O &)>>> &op) const {
            auto self = this->shared_from_this();
            return std::make_shared<Parser<I, O>>([self, op](Input<I> input) -> Output<I, O> {
                auto result = self->parse(input);
                if (result.isSuccess()) {
                    O left = result.data();
                    Input<I> current = result.next();
                    while (true) {
                        auto next = op->parse(current);
                        if (next.isSuccess()) {
                            auto right = self->parse(next.next());
                            if (right.isSuccess()) {
                                left = next.data()(left, right.data());
                                current = right.next();
                            } else {
                                return right;
                            }
                        } else {
                            return Output<I, O>::success(left, current);
                        }
                    }
                } else {
                    return result;
                }
            });
        }

        std::shared_ptr<Parser> label(const std::string &label) {
            auto self = this->shared_from_this();
            return std::make_shared<Parser<I, O>>([self, label](Input<I> input) -> Output<I, O> {
                auto result = self->parse(input);
                if (result.isSuccess()) {
                    return result;
                } else {
                    return Output<I, O>::fail([result, label]() {
                        return result.message() + "\n" + label;
                    }, result.next());
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
        static std::shared_ptr<Parser<I, O>> lazy() {
            return std::make_shared<Parser<I, O>>([](Input<I> input) -> Output<I, O> {
                return Output<I, O>::fail("Not implemented", input);
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
                    return Output<I, O>::fail("End expected", input);
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
        static std::shared_ptr<Parser<I, O>> pure(const O &data) {
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
            return std::make_shared<Parser<I, O>>([message](Input<I> input) -> Output<I, O> {
                return Output<I, O>::fail(message, input);
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
                return Output<I, O>::success(input.current(), input + 1);
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
                Output<I, O> failed = Output<I, O>::fail("No parser matched", input);
                for (const auto &parser: list) {
                    auto result = parser->parse(input);
                    if (result.isSuccess()) {
                        return result;
                    }
                    failed = failed + result;
                }
                return failed;
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
        static std::shared_ptr<Parser<I, std::vector<O>>>
        many(const std::shared_ptr<Parser<I, O>> &parser, bool atLeastOne = false, bool allConsumed = false) {
            return std::make_shared<Parser<I, std::vector<O>>>(
                    [parser, atLeastOne, allConsumed](Input<I> input) -> Output<I, std::vector<O>> {
                        std::vector<O> results;
                        Input<I> current = input;
                        while (true) {
                            auto result = parser->parse(current);
                            if (result.isSuccess()) {
                                results.push_back(result.data());
                                current = result.next();
                            } else {
                                if (results.empty() && atLeastOne) {
                                    return Output<I, std::vector<O>>::fail("At least one expected", input)
                                           + Output<I, std::vector<O>>::fail(result.message(), current);
                                } else if (allConsumed && !current.end()) {
                                    return Output<I, std::vector<O>>::fail("All input should be consumed", current)
                                           + Output<I, std::vector<O>>::fail(result.message(), current);
                                } else {
                                    return Output<I, std::vector<O>>::success(results, current);
                                }
                            }
                        }
                    });
        }

        /**
         * 返回一个功能如下的Parser：依次解析多个Parser，返回一个成功的结果，结果为所有解析结果的列表。
         * @tparam I 
         * @tparam O 
         * @param parsers 
         * @return 
         */
        template<typename I, typename O>
        static std::shared_ptr<Parser<I, std::vector<O>>>
        sequence(std::initializer_list<std::shared_ptr<Parser<I, O>>> parsers) {
            return std::make_shared<Parser<I, std::vector<O>>>([parsers](Input<I> input) {
                std::vector<O> results;
                Input<I> current = input;
                for (const auto &parser: parsers) {
                    auto result = parser->parse(current);
                    if (result.isSuccess()) {
                        results.push_back(result.data());
                        current = result.next();
                    } else {
                        return result;
                    }
                }
                return Output<I, std::vector<O>>::success(results, current);
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
        static std::shared_ptr<Parser<I, I>> satisfy(const std::function<bool(const I &)> &predicate,
                                                     const std::string &message = "Predicate not satisfied") {
            return std::make_shared<Parser<I, I>>([predicate, message](Input<I> input) -> Output<I, I> {
                if (input.end())
                    return Output<I, I>::fail("End of input", input);
                if (predicate(input.current())) {
                    return Output<I, I>::success(input.current(), input + 1);
                } else {
                    return Output<I, I>::fail("At " + std::to_string(input.getIndex()) + ": " + message, input);
                }
            });
        }
    };

    class CoyParsers {
    public:

        using BinaryOperator = std::function<std::shared_ptr<Node>(const std::shared_ptr<Node> &,
                                                                   const std::shared_ptr<Node> &)>;
        static const std::shared_ptr<Parser<Token, std::shared_ptr<NodeIdentifier>>> IDENTIFIER;
        static const std::shared_ptr<Parser<Token, std::shared_ptr<NodeDataType>>> DATA_TYPE;
        static const std::shared_ptr<Parser<Token, std::shared_ptr<NodeInteger>>> INTEGER;
        static const std::shared_ptr<Parser<Token, std::shared_ptr<NodeFloat>>> FLOAT;
        static const std::shared_ptr<Parser<Token, std::shared_ptr<Node>>> NUMBER;
        static const std::shared_ptr<Parser<Token, BinaryOperator>> ADD_SUB;
        static const std::shared_ptr<Parser<Token, BinaryOperator>> MUL_DIV_MOD;
        static const std::shared_ptr<Parser<Token, BinaryOperator>> INEQUALITY_OPERATOR;
        static const std::shared_ptr<Parser<Token, BinaryOperator>> EQUALITY_OPERATOR;
        static const std::shared_ptr<Parser<Token, BinaryOperator>> LOGICAL_AND;
        static const std::shared_ptr<Parser<Token, BinaryOperator>> LOGICAL_OR;
        static const std::shared_ptr<Parser<Token, Token>> ASSIGN;
        static const std::shared_ptr<Parser<Token, Token>> PLUS;
        static const std::shared_ptr<Parser<Token, Token>> MINUS;
        static const std::shared_ptr<Parser<Token, Token>> NOT;
        static const std::shared_ptr<Parser<Token, Token>> UNARY_OPERATOR;
        static const std::shared_ptr<Parser<Token, Token>> COMMA;
        static const std::shared_ptr<Parser<Token, Token>> LEFT_ROUND_BRACKET;
        static const std::shared_ptr<Parser<Token, Token>> RIGHT_ROUND_BRACKET;
        static const std::shared_ptr<Parser<Token, Token>> LEFT_SQUARE_BRACKET;
        static const std::shared_ptr<Parser<Token, Token>> RIGHT_SQUARE_BRACKET;
        static const std::shared_ptr<Parser<Token, Token>> LEFT_BRACE;
        static const std::shared_ptr<Parser<Token, Token>> RIGHT_BRACE;
        static const std::shared_ptr<Parser<Token, std::shared_ptr<Node>>> TERM;
        static const std::shared_ptr<Parser<Token, std::shared_ptr<Node>>> SIGNED_TERM;
        static const std::shared_ptr<Parser<Token, std::shared_ptr<Node>>> PRODUCT;
        static const std::shared_ptr<Parser<Token, std::shared_ptr<Node>>> SUM;
        static const std::shared_ptr<Parser<Token, std::shared_ptr<Node>>> INEQUALITY;
        static const std::shared_ptr<Parser<Token, std::shared_ptr<Node>>> EQUALITY;
        static const std::shared_ptr<Parser<Token, std::shared_ptr<Node>>> AND_EXPRESSION;
        static const std::shared_ptr<Parser<Token, std::shared_ptr<Node>>> OR_EXPRESSION;
        static const std::shared_ptr<Parser<Token, std::shared_ptr<Node>>> EXPRESSION;
        static const std::shared_ptr<Parser<Token, std::shared_ptr<Node>>> ROUND_BRACKET_EXPRESSION;
        static const std::shared_ptr<Parser<Token, std::shared_ptr<Node>>> SQUARE_BRACKET_EXPRESSION;
        static const std::shared_ptr<Parser<Token, std::shared_ptr<NodeLeftValue>>> LEFT_VALUE;

        static const std::shared_ptr<Parser<Token, Token>> IF;
        static const std::shared_ptr<Parser<Token, Token>> ELSE;
        static const std::shared_ptr<Parser<Token, Token>> WHILE;
        static const std::shared_ptr<Parser<Token, Token>> RETURN;
        static const std::shared_ptr<Parser<Token, Token>> BREAK;
        static const std::shared_ptr<Parser<Token, Token>> CONTINUE;

        static const std::shared_ptr<Parser<Token, Token>> END_LINE;
        static const std::shared_ptr<Parser<Token, std::shared_ptr<NodeDefinition>>> VARIABLE_DEFINITION;
        static const std::shared_ptr<Parser<Token, std::shared_ptr<NodeDeclaration>>> VARIABLE_DECLARATION;
        static const std::shared_ptr<Parser<Token, std::shared_ptr<Node>>> ASSIGNMENT;
        static const std::shared_ptr<Parser<Token, std::shared_ptr<Node>>> STATEMENT;
        static const std::shared_ptr<Parser<Token, std::shared_ptr<NodeIf>>> IF_STATEMENT;
        static const std::shared_ptr<Parser<Token, std::shared_ptr<NodeWhile>>> WHILE_STATEMENT;
        static const std::shared_ptr<Parser<Token, std::shared_ptr<NodeBreak>>> BREAK_STATEMENT;
        static const std::shared_ptr<Parser<Token, std::shared_ptr<NodeContinue>>> CONTINUE_STATEMENT;
        static const std::shared_ptr<Parser<Token, std::shared_ptr<NodeReturn>>> RETURN_STATEMENT;
        static const std::shared_ptr<Parser<Token, std::shared_ptr<NodeBlock>>> CODE_BLOCK;
        static const std::shared_ptr<Parser<Token, std::shared_ptr<NodeFunctionParameter>>> FUNCTION_PARAMETER;
        static const std::shared_ptr<Parser<Token, std::shared_ptr<NodeFunction>>> FUNCTION;
        static const std::shared_ptr<Parser<Token, std::shared_ptr<NodeFunctionCall>>> FUNCTION_CALL;

        static const std::shared_ptr<Parser<Token, std::shared_ptr<NodeProgram>>> PROGRAM;
        static const std::shared_ptr<Parser<Token, std::shared_ptr<Node>>> PARSER;
        static const int initializer;
    };

} // coy

#endif //COY_PARSER_H

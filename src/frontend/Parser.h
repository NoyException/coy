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
#include <variant>

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

    class Failure {
    private:
        std::unordered_multimap<int, std::function<std::string()>> _message{};
    public:

        explicit Failure(const std::unordered_multimap<int, std::function<std::string()>> &message)
                : _message(message) {}

        explicit Failure(int position, const std::function<std::string()> &message) {
            _message.insert({position, message});
        }

        Failure(const Failure &a, const Failure &b) {
            for (const auto &error: a._message) {
                _message.insert(error);
            }
            for (const auto &error: b._message) {
                _message.insert(error);
            }
        }

        inline void add(int index, const std::function<std::string()> &message) {
            _message.insert({index, message});
        }

        [[nodiscard]] inline std::string allMessage() const {
            std::string str;
            for (const auto &error: _message) {
                str += "At " + std::to_string(error.first) + ": " + error.second() + "\n";
            }
            return str;
        }

        [[nodiscard]] inline std::pair<int, std::string> message() const {
            //寻找position最大的错误
            auto it = _message.begin();
            for (auto i = _message.begin(); i != _message.end(); ++i) {
                if (i->first > it->first) {
                    it = i;
                }
            }
            return std::make_pair(it->first, it->second());
        }
    };

    template<typename I, typename O>
    class Success {
    private:
        O _data;
        Input<I> _next;
    public:
        Success(const O &data, Input<I> next) : _data(data), _next(next) {}

        [[nodiscard]] O data() const {
            return _data;
        }

        [[nodiscard]] Input<I> next() const {
            return _next;
        }
    };

    template<typename I, typename O>
    class Output {
    private:
        std::variant<Success<I, O>, Failure> _result;
    public:
        explicit Output(const Success<I, O> &result) : _result(result) {}

        Output(const O &data, Input<I> next) : _result(Success<I, O>(data, next)) {}

        explicit Output(const Failure &result) : _result(result) {}

        Output(int position, const std::function<std::string()> &message) : _result(Failure(position, message)) {}

        static Output success(const O &data, Input<I> next) {
            return Output(data, next);
        }

        static Output failure(int position, const std::function<std::string()> &message) {
            return Output(position, message);
        }

        static Output failure(int position, const std::string &message) {
            return Output(position, [message]() { return message; });
        }

        template<typename O2>
        static Output<I, O> failure(const Output<I, O2> &output) {
            return Output<I, O>(output.getFailure());
        }

        [[nodiscard]] bool isSuccess() const {
            return std::holds_alternative<Success<I, O>>(_result);
        }

        [[nodiscard]] bool isFailure() const {
            return std::holds_alternative<Failure>(_result);
        }

        [[nodiscard]] Success<I, O> getSuccess() const {
            return std::get<Success<I, O>>(_result);
        }

        [[nodiscard]] Failure getFailure() const {
            return std::get<Failure>(_result);
        }

        [[nodiscard]] std::string message() const {
            return std::get<Failure>(_result).message().second;
        }

        [[nodiscard]] O data() const {
            return std::get<Success<I, O>>(_result).data();
        }

        [[nodiscard]] Input<I> next() const {
            return std::get<Success<I, O>>(_result).next();
        }

        Output merge(const Output &another) {
            if (this->isSuccess()) {
                return *this;
            }
            if (another.isSuccess()) {
                return another;
            }
            return Output(Failure(getFailure(), another.getFailure()));
        }

        Output operator+(const Output &another) {
            return merge(another);
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

        /**
         * 返回一个功能如下的Parser：将当前Parser解析出的结果强制转换为另一个类型
         * @tparam O2 
         * @return 
         * @see Parser::map
         */
        template<typename O2>
        std::shared_ptr<Parser<I, O2>> as() {
            auto self = this->shared_from_this();
            return std::make_shared<Parser<I, O2>>([self](Input<I> input) {
                auto result = self->parse(input);
                if (result.isSuccess()) {
                    return Output<I, O2>::success(O2(result.data()), result.next());
                } else {
                    return Output<I, O2>::failure(result);
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
            return std::make_shared<Parser>([self, other](Input<I> input) {
                auto result = self->parse(input);
                if (result.isSuccess()) {
                    return result;
                } else {
                    return result + other->parse(input);
                }
            });
        }

        /**
         * 返回一个功能如下的Parser：先尝试解析当前Parser，如果失败则返回o。
         * @param o 
         * @return 
         */
        [[nodiscard]] std::shared_ptr<Parser> orElse(const O &o) {
            return orElse(std::make_shared<Parser>([o](Input<I> input) {
                return Output<I, O>::success(o, input);
            }));
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
                    return Output<I, O2>::failure(result);
                }
            });
        }

        /**
         * 返回一个功能如下的Parser：先尝试解析当前Parser，如果成功则返回o。
         * @tparam O2 
         * @param o 
         * @return 
         */
        template<typename O2>
        [[nodiscard]] std::shared_ptr<Parser<I, O2>> then(const O2 &o) const {
            return then(std::make_shared<Parser<I, O2>>([o](Input<I> input) {
                return Output<I, O2>::success(o, input);
            }));
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
                    return Output<I, O2>::failure(result);
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
            return std::make_shared<Parser<I, O2>>([self, binder](Input<I> input) {
                auto result = self->parse(input);
                if (result.isSuccess()) {
                    return binder(result.data())->parse(result.next());
                } else {
                    return Output<I, O2>::failure(result);
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
                        return Output<I, O>::failure(next);
                    }
                } else {
                    return Output<I, O>::failure(result);
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

        /**
         * 在解析失败时，将指定信息添加到错误信息中。
         * @param label 
         * @return 
         */
        std::shared_ptr<Parser> label(const std::string &label) {
            auto self = this->shared_from_this();
            return std::make_shared<Parser<I, O>>([self, label](Input<I> input) -> Output<I, O> {
                auto result = self->parse(input);
                if (result.isSuccess()) {
                    return result;
                } else {
                    return result + Output<I, O>::failure(input.getIndex(), label);
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
                return Output<I, O>::failure(input.getIndex(), "Not implemented");
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
                    return Output<I, O>::failure(input.getIndex(), "End expected");
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
            return std::make_shared<Parser<I, O>>([message](Input<I> input) {
                return Output<I, O>::failure(input.getIndex(), message);
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
         * 返回一个功能如下的Parser：如果输入满足predicate，则解析trueParser，否则解析falseParser。
         * @tparam I 
         * @tparam O 
         * @tparam O2 
         * @param predicate 
         * @param trueParser 
         * @param falseParser 
         * @return 
         */
        template<typename I, typename O, typename O2>
        static std::shared_ptr<Parser<I, O>> ifElse(const std::shared_ptr<Parser<I, O2>> &predicate,
                                                    const std::shared_ptr<Parser<I, O>> &trueParser,
                                                    const std::shared_ptr<Parser<I, O>> &falseParser) {
            return std::make_shared<Parser<I, O>>([predicate, trueParser, falseParser](Input<I> input) {
                auto result = predicate->parse(input);
                if (result.isSuccess()) {
                    return trueParser->parse(result.next());
                } else {
                    return falseParser->parse(input);
                }
            });
        }

        /**
         * 返回一个功能如下的Parser：如果输入满足predicate，则解析trueParser，否则解析falseParser。
         * @tparam I 
         * @tparam O 
         * @param predicate 
         * @param trueParser 
         * @param falseParser 
         * @return 
         */
        template<typename I, typename O>
        static std::shared_ptr<Parser<I, O>> ifElse(const std::function<bool(const I &)> &predicate,
                                                    const std::shared_ptr<Parser<I, O>> &trueParser,
                                                    const std::shared_ptr<Parser<I, O>> &falseParser) {
            return std::make_shared<Parser<I, O>>([predicate, trueParser, falseParser](Input<I> input) {
                if (predicate(input.current())) {
                    return trueParser->parse(input);
                } else {
                    return falseParser->parse(input);
                }
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
                Output<I, O> failed = Output<I, O>::failure(input.getIndex(), "No parser matched");
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
        many(const std::shared_ptr<Parser<I, O>> &parser, bool atLeastOne = false) {
            return std::make_shared<Parser<I, std::vector<O>>>(
                    [parser, atLeastOne](Input<I> input) {
                        std::vector<O> results;
                        Input<I> current = input;
                        while (true) {
                            auto result = parser->parse(current);
                            if (result.isSuccess()) {
                                results.push_back(result.data());
                                current = result.next();
                            } else {
                                if (results.empty() && atLeastOne) {
                                    return Output<I, std::vector<O>>::failure(input.getIndex(), "At least one expected")
                                           + Output<I, std::vector<O>>::failure(result);
                                } else {
                                    return Output<I, std::vector<O>>::success(results, current);
                                }
                            }
                        }
                    });
        }

        /**
         * 返回一个功能如下的Parser：在predicate成立的情况下尝试多次解析一个Parser，如果解析失败则直接返回失败的结果。
         * 当predicate不成立后，返回一个成功的结果，结果为所有解析结果的列表。
         * 如果atLeastOne为true，则至少要解析一次。
         * @tparam I 
         * @tparam O 
         * @param predicate 
         * @param parser 
         * @param atLeastOne 
         * @return 
         */
        template<typename I, typename O>
        static std::shared_ptr<Parser<I, std::vector<O>>>
        whileSatisfy(const std::function<bool(const I &)> &predicate, const std::shared_ptr<Parser<I, O>> &parser,
                     bool atLeastOne = false) {
            return std::make_shared<Parser<I, std::vector<O>>>(
                    [predicate, parser, atLeastOne](Input<I> input) {
                        std::vector<O> results;
                        Input<I> current = input;
                        while (predicate(input.current())) {
                            auto result = parser->parse(current);
                            if (result.isSuccess()) {
                                results.push_back(result.data());
                                current = result.next();
                            } else {
                                return Output<I, std::vector<O>>::failure(result);
                            }
                        }
                        if (results.empty() && atLeastOne)
                            return Output<I, std::vector<O>>::failure(input.getIndex(), "At least one expected");
                        return Output<I, std::vector<O>>::success(results, current);
                    });
        }

        /**
         * 返回一个功能如下的Parser：在predicate成立的情况下尝试多次解析一个Parser，如果解析失败则直接返回失败的结果。
         * 当predicate不成立后，返回一个成功的结果，结果为所有解析结果的列表。
         * 如果atLeastOne为true，则至少要解析一次。
         * @tparam I 
         * @tparam O 
         * @param predicate 
         * @param parser 
         * @param atLeastOne 
         * @return 
         */
        template<typename I, typename O, typename O2>
        static std::shared_ptr<Parser<I, std::vector<O>>>
        whileSatisfy(const std::shared_ptr<Parser<I, O2>> &predicate, const std::shared_ptr<Parser<I, O>> &parser,
                     bool atLeastOne = false) {
            return std::make_shared<Parser<I, std::vector<O>>>(
                    [predicate, parser, atLeastOne](Input<I> input) {
                        std::vector<O> results;
                        auto condition = predicate->parse(input);
                        std::optional<Output<I, O>> result = std::nullopt;
                        while (condition.isSuccess()) {
                            result = parser->parse(condition.next());
                            if (result->isSuccess()) {
                                results.push_back(result->data());
                            } else {
                                return Output<I, std::vector<O>>::failure(*result);
                            }
                            condition = predicate->parse(result->next());
                        }
                        if (results.empty() && atLeastOne)
                            return Output<I, std::vector<O>>::failure(input.getIndex(), "At least one expected");
                        return Output<I, std::vector<O>>::success(results,
                                                                  result.has_value() ? result->next() : input);
                    });
        }

        /**
         * 返回一个功能如下的Parser：尝试以separator为分隔符多次解析一个Parser，返回一个成功的结果，
         * 结果为所有parser的解析结果的列表。
         * @tparam I 
         * @tparam O 
         * @param parser 
         * @param separator 
         * @return 
         */
        template<typename I, typename O, typename O2>
        static std::shared_ptr<Parser<I, std::vector<O>>>
        seperatedBy(const std::shared_ptr<Parser<I, O>> &parser, const std::shared_ptr<Parser<I, O2>> &separator,
                    bool atLeastOne = false) {
            if (!atLeastOne)
                return seperatedBy(parser, separator, true)->orElse(pure<std::vector<O>>({}));
            return parser->bind([parser, separator](const O &first) {
                return parser->many(separator->then(parser)).map([first](const std::vector<O> &rest) {
                    std::vector<O> results;
                    results.push_back(first);
                    results.insert(results.end(), rest.begin(), rest.end());
                    return results;
                });
            });
        }

        /**
         * 返回一个功能如下的Parser：尝试多次解析一个Parser，然后解析end，返回一个成功的结果，结果为所有解析结果的列表。
         * @tparam I 
         * @tparam O 
         * @param parser 
         * @param end 
         * @param atLeastOne 
         * @return 
         */
        template<typename I, typename O, typename O2>
        static std::shared_ptr<Parser<I, std::vector<O>>>
        endBy(const std::shared_ptr<Parser<I, O>> &parser, const std::shared_ptr<Parser<I, O2>> &end,
              bool atLeastOne = false) {
            return std::make_shared<Parser<I, std::vector<O>>>(
                    [parser, atLeastOne, end](Input<I> input) {
                        std::vector<O> results;
                        Input<I> current = input;
                        while (true) {
                            auto result = parser->parse(current);
                            if (result.isSuccess()) {
                                results.push_back(result.data());
                                current = result.next();
                            } else {
                                if (results.empty() && atLeastOne) {
                                    return Output<I, std::vector<O>>::failure(input.getIndex(),
                                                                              "At least one expected")
                                           + Output<I, std::vector<O>>::failure(result);
                                } else {
                                    auto endResult = end->parse(current);
                                    if (endResult.isSuccess()) {
                                        return Output<I, std::vector<O>>::success(results, endResult.next());
                                    } else {
                                        return Output<I, std::vector<O>>::failure(result);
                                    }
                                }
                            }
                        }
                    });
        }

        template<typename I, typename O, typename O2, typename O3>
        static std::shared_ptr<Parser<I, std::vector<O>>>
        seperatedEndBy(const std::shared_ptr<Parser<I, O>> &parser, const std::shared_ptr<Parser<I, O2>> &separator,
                       const std::shared_ptr<Parser<I, O3>> &end, bool atLeastOne = false) {
            if (!atLeastOne)
                return seperatedEndBy(parser, separator, end, true)
                        ->orElse(end->then(pure<I, std::vector<O>>({})));
            return parser->template bind<std::vector<O>>([parser, separator, end](const O &first) {
                return endBy(separator->then(parser), end)->template map<std::vector<O>>(
                        [first](const std::vector<O> &rest) {
                            std::vector<O> results;
                            results.push_back(first);
                            results.insert(results.end(), rest.begin(), rest.end());
                            return results;
                        });
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
                                                     const std::function<std::string(const I &)> &message = [](
                                                             const I &i) {
                                                         return "Unexpected symbol";
                                                     }) {
            return std::make_shared<Parser<I, I>>([predicate, message](Input<I> input) -> Output<I, I> {
                if (input.end())
                    return Output<I, I>::failure(input.getIndex(), "End of input");
                if (predicate(input.current())) {
                    return Output<I, I>::success(input.current(), input + 1);
                } else {
                    return Output<I, I>::failure(input.getIndex(), message(input.current()));
                }
            });
        }
    };

    class CoyParsers {
    public:

        using BinaryOperator = std::function<std::shared_ptr<NodeTyped>(const std::shared_ptr<NodeTyped> &,
                                                                   const std::shared_ptr<NodeTyped> &)>;
        static const std::shared_ptr<Parser<Token, std::shared_ptr<NodeIdentifier>>> IDENTIFIER;
        static const std::shared_ptr<Parser<Token, std::shared_ptr<NodeDataType>>> DATA_TYPE;
        static const std::shared_ptr<Parser<Token, std::shared_ptr<NodeInteger>>> INTEGER;
        static const std::shared_ptr<Parser<Token, std::shared_ptr<NodeFloat>>> FLOAT;
        static const std::shared_ptr<Parser<Token, std::shared_ptr<NodeTyped>>> NUMBER;
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
        static const std::shared_ptr<Parser<Token, std::shared_ptr<NodeTyped>>> TERM;
        static const std::shared_ptr<Parser<Token, std::shared_ptr<NodeTyped>>> SIGNED_TERM;
        static const std::shared_ptr<Parser<Token, std::shared_ptr<NodeTyped>>> PRODUCT;
        static const std::shared_ptr<Parser<Token, std::shared_ptr<NodeTyped>>> SUM;
        static const std::shared_ptr<Parser<Token, std::shared_ptr<NodeTyped>>> INEQUALITY;
        static const std::shared_ptr<Parser<Token, std::shared_ptr<NodeTyped>>> EQUALITY;
        static const std::shared_ptr<Parser<Token, std::shared_ptr<NodeTyped>>> AND_EXPRESSION;
        static const std::shared_ptr<Parser<Token, std::shared_ptr<NodeTyped>>> OR_EXPRESSION;
        static const std::shared_ptr<Parser<Token, std::shared_ptr<NodeTyped>>> EXPRESSION;
        static const std::shared_ptr<Parser<Token, std::shared_ptr<NodeTyped>>> ROUND_BRACKET_EXPRESSION;
        static const std::shared_ptr<Parser<Token, std::shared_ptr<NodeTyped>>> SQUARE_BRACKET_EXPRESSION;
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

//
// Created by noyex on 24-3-23.
//

#include "Parser.h"

namespace coy {
    const std::shared_ptr<Parser<Token, NodeIdentifier>> CoyParsers::IDENTIFIER =
            Parsers::satisfy<Token>([](const Token &token) {
                return token.type == TYPE_IDENTIFIER;
            })->map<NodeIdentifier>([](const std::shared_ptr<Token> &token) {
                return std::make_shared<NodeIdentifier>(token->value);
            });

    const std::shared_ptr<Parser<Token, Node>> CoyParsers::INTEGER =
            Parsers::satisfy<Token>([](const Token &token) {
                return token.type == TYPE_INTEGER;
            })->map<Node>([](const std::shared_ptr<Token> &token) {
                return std::make_shared<NodeInteger>(std::stoi(token->value));
            });

    const std::shared_ptr<Parser<Token, Node>> CoyParsers::FLOAT =
            Parsers::satisfy<Token>([](const Token &token) {
                return token.type == TYPE_FLOAT;
            })->map<Node>([](const std::shared_ptr<Token> &token) {
                return std::make_shared<NodeFloat>(std::stof(token->value));
            });

    const std::shared_ptr<Parser<Token, Node>> CoyParsers::NUMBER =
            Parsers::any({INTEGER, FLOAT});

    std::shared_ptr<Parser<Token, CoyParsers::BinaryOperator>>
    generateBinaryOperators(const std::set<std::string> &ops) {
        return Parsers::satisfy<Token>([ops](const Token &token) {
            return token.type == TYPE_OPERATOR && ops.count(token.value) > 0;
        })->map<CoyParsers::BinaryOperator>([](const std::shared_ptr<Token> &token) {
            return std::make_shared<CoyParsers::BinaryOperator>(
                    [token](const std::shared_ptr<Node> &left, const std::shared_ptr<Node> &right) {
                        return (std::shared_ptr<Node>) std::make_shared<NodeBinaryOperator>(token->value, left, right);
                    });
        });
    }

    const std::shared_ptr<Parser<Token, CoyParsers::BinaryOperator>> CoyParsers::ADD_SUB = generateBinaryOperators(
            {"+", "-"});

    const std::shared_ptr<Parser<Token, CoyParsers::BinaryOperator>> CoyParsers::MUL_DIV = generateBinaryOperators(
            {"*", "/"});

    const std::shared_ptr<Parser<Token, CoyParsers::BinaryOperator>> CoyParsers::INEQUALITY_OPERATOR = generateBinaryOperators(
            {"<", ">", "<=", ">="});

    const std::shared_ptr<Parser<Token, CoyParsers::BinaryOperator>> CoyParsers::EQUALITY_OPERATOR = generateBinaryOperators(
            {"==", "!="});

    const std::shared_ptr<Parser<Token, CoyParsers::BinaryOperator>> CoyParsers::LOGICAL_AND = generateBinaryOperators(
            {"&&"});

    const std::shared_ptr<Parser<Token, CoyParsers::BinaryOperator>> CoyParsers::LOGICAL_OR = generateBinaryOperators(
            {"||"});

    std::shared_ptr<Parser<Token, Token>> generateUnaryOperator(const std::string &op) {
        return Parsers::satisfy<Token>([op](const Token &token) {
            return token.type == TYPE_OPERATOR && token.value == op;
        });
    }

    const std::shared_ptr<Parser<Token, Token>> CoyParsers::PLUS = generateUnaryOperator("+");

    const std::shared_ptr<Parser<Token, Token>> CoyParsers::MINUS = generateUnaryOperator("-");

    const std::shared_ptr<Parser<Token, Token>> CoyParsers::NOT = generateUnaryOperator("!");

    const std::shared_ptr<Parser<Token, Token>> CoyParsers::UNARY_OPERATOR = Parsers::any({PLUS, MINUS, NOT});

    std::shared_ptr<Parser<Token, Token>> generateSeparator(const std::string &separator) {
        return Parsers::satisfy<Token>([separator](const Token &token) {
            return token.type == TYPE_SEPARATOR && token.value == separator;
        });
    }

    const std::shared_ptr<Parser<Token, Token>> CoyParsers::LEFT_ROUND_BRACKET = generateSeparator("(");

    const std::shared_ptr<Parser<Token, Token>> CoyParsers::RIGHT_ROUND_BRACKET = generateSeparator(")");

    const std::shared_ptr<Parser<Token, Token>> CoyParsers::LEFT_SQUARE_BRACKET = generateSeparator("[");

    const std::shared_ptr<Parser<Token, Token>> CoyParsers::RIGHT_SQUARE_BRACKET = generateSeparator("]");

    //term = INTEGER | FLOAT | paren_expr
    const std::shared_ptr<Parser<Token, Node>> CoyParsers::TERM = Parsers::lazy<Token, Node>();

    //TODO: 补充identity "(" [FuncRParams] ")"
    //signed_term = ('+' | '-' | '!') signed_term | term
    const std::shared_ptr<Parser<Token, Node>> CoyParsers::SIGNED_TERM =
            UNARY_OPERATOR->bind<Node>(
                    [](const std::shared_ptr<Token> &token) {
                        if (token->value == "+")
                            return SIGNED_TERM;
                        else
                            return SIGNED_TERM->map<Node>(
                                    [token](const std::shared_ptr<Node> &node) {
                                        return (std::shared_ptr<Node>)
                                                std::make_shared<NodeUnaryOperator>(token->value, node);
                                    }
                            );
                    }
            )->orElse(CoyParsers::TERM);

    //product = term (MUL_DIV term)*
    const std::shared_ptr<Parser<Token, Node>> CoyParsers::PRODUCT =
            SIGNED_TERM->chainLeft(MUL_DIV);

    //sum = product (ADD_SUB product)*
    const std::shared_ptr<Parser<Token, Node>> CoyParsers::SUM =
            PRODUCT->chainLeft(ADD_SUB);

    //comparison = sum (COMP sum)*
    const std::shared_ptr<Parser<Token, Node>> CoyParsers::INEQUALITY =
            SUM->chainLeft(INEQUALITY_OPERATOR);

    //equality = comparison (EQUALITY comparison)*
    const std::shared_ptr<Parser<Token, Node>> CoyParsers::EQUALITY =
            INEQUALITY->chainLeft(EQUALITY_OPERATOR);

    //and_expr = equality (AND equality)*
    const std::shared_ptr<Parser<Token, Node>> CoyParsers::AND_EXPRESSION =
            EQUALITY->chainLeft(LOGICAL_AND);

    //or_expr = and_expr (OR and_expr)*
    const std::shared_ptr<Parser<Token, Node>> CoyParsers::OR_EXPRESSION =
            AND_EXPRESSION->chainLeft(LOGICAL_OR);

    //expr = or_expr
    const std::shared_ptr<Parser<Token, Node>> CoyParsers::EXPRESSION =
            OR_EXPRESSION;

    //round_bracket_expr = '(' expr ')'
    const std::shared_ptr<Parser<Token, Node>> CoyParsers::ROUND_BRACKET_EXPRESSION =
            CoyParsers::LEFT_ROUND_BRACKET->then(CoyParsers::EXPRESSION)->skip(CoyParsers::RIGHT_ROUND_BRACKET);

    //square_bracket_expr = '[' expr ']'
    const std::shared_ptr<Parser<Token, Node>> CoyParsers::SQUARE_BRACKET_EXPRESSION =
            CoyParsers::LEFT_SQUARE_BRACKET->then(CoyParsers::EXPRESSION)->skip(CoyParsers::RIGHT_SQUARE_BRACKET);

    //left_value = IDENTIFIER ('[' expr ']')*
    const std::shared_ptr<Parser<Token, Node>> CoyParsers::LEFT_VALUE =
            CoyParsers::IDENTIFIER->bind<Node>(
                    [](const std::shared_ptr<NodeIdentifier> &node) -> std::shared_ptr<Parser<Token, Node>> {
                        return Parsers::many(SQUARE_BRACKET_EXPRESSION)->map<Node>(
                                [node](const std::shared_ptr<std::vector<std::shared_ptr<Node>>> &nodes) {
                                    for (const auto &item: *nodes) {
                                        node->addArg(item);
                                    }
                                    return (std::shared_ptr<Node>) node;
                                }
                        );
                    });

    const std::shared_ptr<Parser<Token, Node>> CoyParsers::PARSER = EXPRESSION->skip(Parsers::end<Token, Node>());

    const int CoyParsers::initializer = []() -> int {
        (*TERM) = *Parsers::any({NUMBER, LEFT_VALUE, ROUND_BRACKET_EXPRESSION});
        return 0;
    }();
} // coy
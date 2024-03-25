//
// Created by noyex on 24-3-23.
//

#include "Parser.h"

namespace coy {
    const std::shared_ptr<Parser<Token, NodeInteger>> CoyParsers::INTEGER =
            Parsers::satisfy<Token>([](const Token &token) {
                return token.type == TYPE_INTEGER;
            })->map<NodeInteger>([](const std::shared_ptr<Token> &token) {
                return std::make_shared<NodeInteger>(std::stoi(token->value));
            });

    const std::shared_ptr<Parser<Token, NodeFloat>> CoyParsers::FLOAT =
            Parsers::satisfy<Token>([](const Token &token) {
                return token.type == TYPE_FLOAT;
            })->map<NodeFloat>([](const std::shared_ptr<Token> &token) {
                return std::make_shared<NodeFloat>(std::stof(token->value));
            });

    const std::shared_ptr<Parser<Token, CoyParsers::BinaryOperator>> CoyParsers::ADD_SUB =
            Parsers::satisfy<Token>([](const Token &token) {
                return token.type == TYPE_OPERATOR && (token.value == "+" || token.value == "-");
            })->map<BinaryOperator>([](const std::shared_ptr<Token> &token) {
                return std::make_shared<BinaryOperator>(
                        [token](const std::shared_ptr<Node> &left, const std::shared_ptr<Node> &right) {
                            return (std::shared_ptr<Node>) std::make_shared<NodeBinaryOperator>(token->value, left,
                                                                                                right);
                        });
            });

    const std::shared_ptr<Parser<Token, CoyParsers::BinaryOperator>> CoyParsers::MUL_DIV =
            Parsers::satisfy<Token>([](const Token &token) {
                return token.type == TYPE_OPERATOR && (token.value == "*" || token.value == "/");
            })->map<BinaryOperator>([](const std::shared_ptr<Token> &token) {
                return std::make_shared<BinaryOperator>(
                        [token](const std::shared_ptr<Node> &left, const std::shared_ptr<Node> &right) {
                            return (std::shared_ptr<Node>) std::make_shared<NodeBinaryOperator>(token->value, left,
                                                                                                right);
                        });
            });

    const std::shared_ptr<Parser<Token, Token>> CoyParsers::PLUS =
            Parsers::satisfy<Token>([](const Token &token) {
                return token.type == TYPE_OPERATOR && token.value == "+";
            });

    const std::shared_ptr<Parser<Token, Token>> CoyParsers::MINUS =
            Parsers::satisfy<Token>([](const Token &token) {
                return token.type == TYPE_OPERATOR && token.value == "-";
            });

    const std::shared_ptr<Parser<Token, Token>> CoyParsers::OPEN_PAREN =
            Parsers::satisfy<Token>([](const Token &token) {
                return token.type == TYPE_SEPARATOR && token.value == "(";
            });

    const std::shared_ptr<Parser<Token, Token>> CoyParsers::CLOSE_PAREN =
            Parsers::satisfy<Token>([](const Token &token) {
                return token.type == TYPE_SEPARATOR && token.value == ")";
            });

    //term = INTEGER | FLOAT | '(' expr ')'
    const std::shared_ptr<Parser<Token, Node>> CoyParsers::TERM = Parsers::lazy<Token, Node>();

    //signed_term = ('+' | '-') term | term
    const std::shared_ptr<Parser<Token, Node>> CoyParsers::SIGNED_TERM =
            Parsers::any({CoyParsers::PLUS, CoyParsers::MINUS})
                    ->bind<Node>(
                            [](const std::shared_ptr<Token> &token) {
                                if (token->value == "+")
                                    return CoyParsers::TERM;
                                else
                                    return CoyParsers::TERM->map<Node>(
                                            [token](const std::shared_ptr<Node> &node) {
                                                return (std::shared_ptr<Node>)
                                                        std::make_shared<NodeUnaryOperator>("-", node);
                                            }
                                    );
                            }
                    )->orElse(CoyParsers::TERM);
    
    //production = term (MUL_DIV term)*
    const std::shared_ptr<Parser<Token, Node>> CoyParsers::PRODUCTION =
            SIGNED_TERM->chainLeft(MUL_DIV);
    
    //expr = production (ADD_SUB production)*
    const std::shared_ptr<Parser<Token, Node>> CoyParsers::EXPRESSION =
            PRODUCTION->chainLeft(ADD_SUB);

    //paren_expr = '(' expr ')'
    const std::shared_ptr<Parser<Token, Node>> CoyParsers::PAREN_EXPRESSION =
            CoyParsers::OPEN_PAREN->then(CoyParsers::EXPRESSION)->skip(CoyParsers::CLOSE_PAREN);

    const std::shared_ptr<Parser<Token, Node>> CoyParsers::PARSER = EXPRESSION->skip(Parsers::end<Token, Node>());

    const int CoyParsers::initializer = []() -> int {
        (*TERM) = *Parsers::any({INTEGER->as<Node>(), FLOAT->as<Node>(), PAREN_EXPRESSION});
        return 0;
    }();
} // coy
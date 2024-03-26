//
// Created by noyex on 24-3-23.
//

#include "Parser.h"

namespace coy {
    const std::shared_ptr<Parser<Token, std::shared_ptr<NodeIdentifier>>> CoyParsers::IDENTIFIER =
            Parsers::satisfy<Token>([](const Token &token) {
                return token.type == TYPE_IDENTIFIER;
            })->map<std::shared_ptr<NodeIdentifier>>([](const Token &token) {
                return std::make_shared<NodeIdentifier>(token.value);
            });

    const std::shared_ptr<Parser<Token, std::shared_ptr<Node>>> CoyParsers::INTEGER =
            Parsers::satisfy<Token>([](const Token &token) {
                return token.type == TYPE_INTEGER;
            })->map<std::shared_ptr<Node>>([](const Token &token) {
                return std::make_shared<NodeInteger>(std::stoi(token.value));
            });

    const std::shared_ptr<Parser<Token, std::shared_ptr<Node>>> CoyParsers::FLOAT =
            Parsers::satisfy<Token>([](const Token &token) {
                return token.type == TYPE_FLOAT;
            })->map<std::shared_ptr<Node>>([](const Token &token) {
                return std::make_shared<NodeFloat>(std::stof(token.value));
            });

    const std::shared_ptr<Parser<Token, std::shared_ptr<Node>>> CoyParsers::NUMBER =
            Parsers::any({INTEGER, FLOAT});

    std::shared_ptr<Parser<Token, CoyParsers::BinaryOperator>>
    generateBinaryOperators(const std::set<std::string> &ops) {
        return Parsers::satisfy<Token>([ops](const Token &token) {
            return token.type == TYPE_OPERATOR && ops.count(token.value) > 0;
        })->map<CoyParsers::BinaryOperator>([](const Token &token) {
            return [token](const std::shared_ptr<Node> &left, const std::shared_ptr<Node> &right) {
                return (std::shared_ptr<Node>) std::make_shared<NodeBinaryOperator>(token.value, left, right);
            };
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

    const std::shared_ptr<Parser<Token, Token>> CoyParsers::ASSIGN = Parsers::satisfy<Token>([](const Token &token) {
        return token.type == TYPE_OPERATOR && token.value == "=";
    });

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

    const std::shared_ptr<Parser<Token, Token>> CoyParsers::LEFT_BRACE = generateSeparator("{");

    const std::shared_ptr<Parser<Token, Token>> CoyParsers::RIGHT_BRACE = generateSeparator("}");

    //term = INTEGER | FLOAT | paren_expr
    const std::shared_ptr<Parser<Token, std::shared_ptr<Node>>> CoyParsers::TERM = Parsers::lazy<Token, std::shared_ptr<Node>>();

    //TODO: 补充identity "(" [FuncRParams] ")"
    //signed_term = ('+' | '-' | '!') signed_term | term
    const std::shared_ptr<Parser<Token, std::shared_ptr<Node>>> CoyParsers::SIGNED_TERM =
            UNARY_OPERATOR->bind<std::shared_ptr<Node>>(
                    [](const Token &token) {
                        if (token.value == "+")
                            return SIGNED_TERM;
                        else
                            return SIGNED_TERM->map<std::shared_ptr<Node>>(
                                    [token](const std::shared_ptr<Node> &node) {
                                        return (std::shared_ptr<Node>)
                                                std::make_shared<NodeUnaryOperator>(token.value, node);
                                    }
                            );
                    }
            )->orElse(CoyParsers::TERM);

    //product = term (MUL_DIV term)*
    const std::shared_ptr<Parser<Token, std::shared_ptr<Node>>> CoyParsers::PRODUCT =
            SIGNED_TERM->chainLeft(MUL_DIV);

    //sum = product (ADD_SUB product)*
    const std::shared_ptr<Parser<Token, std::shared_ptr<Node>>> CoyParsers::SUM =
            PRODUCT->chainLeft(ADD_SUB);

    //comparison = sum (COMP sum)*
    const std::shared_ptr<Parser<Token, std::shared_ptr<Node>>> CoyParsers::INEQUALITY =
            SUM->chainLeft(INEQUALITY_OPERATOR);

    //equality = comparison (EQUALITY comparison)*
    const std::shared_ptr<Parser<Token, std::shared_ptr<Node>>> CoyParsers::EQUALITY =
            INEQUALITY->chainLeft(EQUALITY_OPERATOR);

    //and_expr = equality (AND equality)*
    const std::shared_ptr<Parser<Token, std::shared_ptr<Node>>> CoyParsers::AND_EXPRESSION =
            EQUALITY->chainLeft(LOGICAL_AND);

    //or_expr = and_expr (OR and_expr)*
    const std::shared_ptr<Parser<Token, std::shared_ptr<Node>>> CoyParsers::OR_EXPRESSION =
            AND_EXPRESSION->chainLeft(LOGICAL_OR);

    //expr = or_expr
    const std::shared_ptr<Parser<Token, std::shared_ptr<Node>>> CoyParsers::EXPRESSION =
            OR_EXPRESSION;

    //round_bracket_expr = '(' expr ')'
    const std::shared_ptr<Parser<Token, std::shared_ptr<Node>>> CoyParsers::ROUND_BRACKET_EXPRESSION =
            LEFT_ROUND_BRACKET->then(EXPRESSION)->skip(RIGHT_ROUND_BRACKET);

    //square_bracket_expr = '[' expr ']'
    const std::shared_ptr<Parser<Token, std::shared_ptr<Node>>> CoyParsers::SQUARE_BRACKET_EXPRESSION =
            LEFT_SQUARE_BRACKET->then(EXPRESSION)->skip(RIGHT_SQUARE_BRACKET);

    //left_value = identifier ('[' expr ']')*
    const std::shared_ptr<Parser<Token, std::shared_ptr<Node>>> CoyParsers::LEFT_VALUE =
            IDENTIFIER->bind<std::shared_ptr<Node>>([](const std::shared_ptr<NodeIdentifier> &node) {
                return Parsers::many(SQUARE_BRACKET_EXPRESSION)->map<std::shared_ptr<Node>>(
                        [node](const std::vector<std::shared_ptr<Node>> &nodes) {
                            for (const auto &item: nodes) {
                                node->addIndex(item);
                            }
                            return (std::shared_ptr<Node>) node;
                        }
                );
            });

    const std::shared_ptr<Parser<Token, Token>> CoyParsers::END_LINE =
            generateSeparator(";")->label("';' expected");
    
    //var_def = identifier ('[' int ']')* ('=' expr)?
    const std::shared_ptr<Parser<Token, std::shared_ptr<Node>>> CoyParsers::VARIABLE_DEFINITION =
            IDENTIFIER->bind<std::shared_ptr<Node>>(
                    [](const std::shared_ptr<Node> &identifier) -> std::shared_ptr<Parser<Token, std::shared_ptr<Node>>> {
                        return Parsers::many(
                                LEFT_SQUARE_BRACKET->then(INTEGER)->skip(RIGHT_SQUARE_BRACKET)
                        )->map<std::vector<int>>([identifier](
                                const std::vector<std::shared_ptr<Node>> &nodes) -> std::vector<int> {
                            std::vector<int> result;
                            result.reserve(nodes.size());
                            for (const auto &node: nodes) {
                                result.push_back(std::dynamic_pointer_cast<NodeInteger>(node)->getNumber());
                            }
                            return result;
                        })->bind<std::shared_ptr<Node>>(
                                [identifier](
                                        const std::vector<int> &args) -> std::shared_ptr<Parser<Token, std::shared_ptr<Node>>> {
                                    return ASSIGN->then(EXPRESSION)->orElse(
                                                    Parsers::pure<Token, std::shared_ptr<Node>>(nullptr))
                                            ->map<std::shared_ptr<Node>>([identifier, args](
                                                    const std::shared_ptr<Node> &expr) -> std::shared_ptr<Node> {
                                                auto node = std::make_shared<NodeDefinition>(identifier, expr);
                                                for (const auto &item: args) {
                                                    node->addDimension(item);
                                                }
                                                return node;
                                            });
                                }
                        );
                    });
    
    //var_decl = data_type var_def (',' var_def)* ';'
    const std::shared_ptr<Parser<Token, std::shared_ptr<Node>>> CoyParsers::VARIABLE_DECLARATION =
            Parsers::satisfy<Token>([](const Token &token) {
                return token.type == TYPE_DATA_TYPE;
            })->bind<std::shared_ptr<Node>>(
                    [](const Token &token) {
                        return VARIABLE_DEFINITION->bind<std::shared_ptr<Node>>(
                                [token](const std::shared_ptr<Node> &node) {
                                    return Parsers::many(
                                            Parsers::satisfy<Token>([](const Token &token) {
                                                return token.type == TYPE_SEPARATOR && token.value == ",";
                                            })->then(VARIABLE_DEFINITION)
                                    )->map<std::shared_ptr<Node>>([token, node](
                                            const std::vector<std::shared_ptr<Node>> &nodes) -> std::shared_ptr<Node> {
                                        auto result = std::make_shared<NodeDeclaration>(token.value);
                                        result->addDefinition(node);
                                        for (const auto &item: nodes) {
                                            result->addDefinition(item);
                                        }
                                        return result;
                                    })->skip(END_LINE);
                                }
                        );
                    }
            );

    //assignment = left_value '=' expr ';'
    const std::shared_ptr<Parser<Token, std::shared_ptr<Node>>> CoyParsers::ASSIGNMENT =
            LEFT_VALUE
                    ->skip(ASSIGN)
                    ->bind<std::shared_ptr<Node>>(
                            [](const std::shared_ptr<Node> &leftValue) {
                                return EXPRESSION->map<std::shared_ptr<Node>>(
                                        [leftValue](const std::shared_ptr<Node> &expr) {
                                            return std::make_shared<NodeAssignment>(leftValue, expr);
                                        }
                                );
                            });


    //statement = left_value "=" expr ";" | expr ";" | if_statement
    const std::shared_ptr<Parser<Token, std::shared_ptr<Node>>> CoyParsers::STATEMENT = Parsers::lazy<Token, std::shared_ptr<Node>>();

    //if_statement = 'if' '(' expr ')' statement ('else' statement)?
    const std::shared_ptr<Parser<Token, std::shared_ptr<Node>>> CoyParsers::IF_STATEMENT =
            Parsers::satisfy<Token>([](const Token &token) {
                return token.type == TYPE_KEYWORD && token.value == "if";
            })
                    ->then(LEFT_ROUND_BRACKET)
                    ->then(EXPRESSION)
                    ->skip(RIGHT_ROUND_BRACKET)
                    ->bind<std::shared_ptr<Node>>(
                            [](const std::shared_ptr<Node> &condition) {
                                return STATEMENT->bind<std::shared_ptr<Node>>(
                                        [condition](
                                                const std::shared_ptr<Node> &statement) {
                                            return Parsers::satisfy<Token>([](const Token &token) {
                                                return token.type == TYPE_KEYWORD && token.value == "else";
                                            })
                                                    ->then(STATEMENT)
                                                    ->orElse(Parsers::pure<Token, std::shared_ptr<Node>>(nullptr))
                                                    ->map<std::shared_ptr<Node>>([condition, statement](
                                                            const std::shared_ptr<Node> &elseStatement) -> std::shared_ptr<Node> {
                                                        return std::make_shared<NodeIf>(condition, statement,
                                                                                        elseStatement);
                                                    });
                                        }
                                );
                            });

    //code_block = '{' (statement | var_decl)* '}'
    const std::shared_ptr<Parser<Token, std::shared_ptr<Node>>> CoyParsers::CODE_BLOCK =
            LEFT_BRACE->then(Parsers::many(STATEMENT->orElse(VARIABLE_DECLARATION)))->skip(RIGHT_BRACE)->map<std::shared_ptr<Node>>(
                    [](const std::vector<std::shared_ptr<Node>> &nodes) {
                        auto block = std::make_shared<NodeBlock>();
                        for (const auto &node: nodes) {
                            block->addStatement(node);
                        }
                        return block;
                    });

//    const std::shared_ptr<Parser<Token, std::shared_ptr<Node>>> CoyParsers::PARSER = EXPRESSION->skip(Parsers::end<Token, std::shared_ptr<Node>>());

    const std::shared_ptr<Parser<Token, std::shared_ptr<Node>>> CoyParsers::PARSER = STATEMENT->skip(
            Parsers::end<Token, std::shared_ptr<Node>>());

    const int CoyParsers::initializer = []() -> int {
        (*TERM) = *Parsers::any({NUMBER, LEFT_VALUE, ROUND_BRACKET_EXPRESSION});
        (*STATEMENT) = *Parsers::any({
                                             CODE_BLOCK,
                                             EXPRESSION->skip(END_LINE),
                                             ASSIGNMENT->skip(END_LINE),
                                             IF_STATEMENT});
        return 0;
    }();
} // coy
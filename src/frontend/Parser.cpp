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

    const std::shared_ptr<Parser<Token, std::shared_ptr<NodeDataType>>> CoyParsers::DATA_TYPE =
            Parsers::satisfy<Token>([](const Token &token) {
                return token.type == TYPE_DATA_TYPE;
            })->map<std::shared_ptr<NodeDataType>>([](const Token &token) {
                return std::make_shared<NodeDataType>(token.value);
            });

    const std::shared_ptr<Parser<Token, std::shared_ptr<NodeInteger>>> CoyParsers::INTEGER =
            Parsers::satisfy<Token>([](const Token &token) {
                return token.type == TYPE_INTEGER;
            })->map<std::shared_ptr<NodeInteger>>([](const Token &token) {
                return std::make_shared<NodeInteger>(std::stoi(token.value));
            });

    const std::shared_ptr<Parser<Token, std::shared_ptr<NodeFloat>>> CoyParsers::FLOAT =
            Parsers::satisfy<Token>([](const Token &token) {
                return token.type == TYPE_FLOAT;
            })->map<std::shared_ptr<NodeFloat>>([](const Token &token) {
                return std::make_shared<NodeFloat>(std::stof(token.value));
            });

    const std::shared_ptr<Parser<Token, std::shared_ptr<Node>>> CoyParsers::NUMBER =
            Parsers::any({INTEGER->as<std::shared_ptr<Node>>(), FLOAT->as<std::shared_ptr<Node>>()});

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

    const std::shared_ptr<Parser<Token, Token>> CoyParsers::COMMA = generateSeparator(",");

    const std::shared_ptr<Parser<Token, Token>> CoyParsers::LEFT_ROUND_BRACKET = generateSeparator("(");

    const std::shared_ptr<Parser<Token, Token>> CoyParsers::RIGHT_ROUND_BRACKET = generateSeparator(")");

    const std::shared_ptr<Parser<Token, Token>> CoyParsers::LEFT_SQUARE_BRACKET = generateSeparator("[");

    const std::shared_ptr<Parser<Token, Token>> CoyParsers::RIGHT_SQUARE_BRACKET = generateSeparator("]");

    const std::shared_ptr<Parser<Token, Token>> CoyParsers::LEFT_BRACE = generateSeparator("{");

    const std::shared_ptr<Parser<Token, Token>> CoyParsers::RIGHT_BRACE = generateSeparator("}");

    //term = number | left_value | identifier(function_arguments) | paren_expr
    const std::shared_ptr<Parser<Token, std::shared_ptr<Node>>> CoyParsers::TERM = Parsers::lazy<Token, std::shared_ptr<Node>>();

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
    const std::shared_ptr<Parser<Token, std::shared_ptr<NodeLeftValue>>> CoyParsers::LEFT_VALUE =
            IDENTIFIER->bind<std::shared_ptr<NodeLeftValue>>([](const std::shared_ptr<NodeIdentifier> &identifier) {
                return Parsers::many(SQUARE_BRACKET_EXPRESSION)->map<std::shared_ptr<NodeLeftValue>>(
                        [identifier](const std::vector<std::shared_ptr<Node>> &nodes) {
                            auto node = std::make_shared<NodeLeftValue>(identifier);
                            for (const auto &item: nodes) {
                                node->addIndex(item);
                            }
                            return node;
                        }
                );
            });

    std::shared_ptr<Parser<Token, Token>> generateKeyword(const std::string &keyword) {
        return Parsers::satisfy<Token>([keyword](const Token &token) {
            return token.type == TYPE_KEYWORD && token.value == keyword;
        });
    }

    const std::shared_ptr<Parser<Token, Token>> CoyParsers::IF = generateKeyword("if");

    const std::shared_ptr<Parser<Token, Token>> CoyParsers::ELSE = generateKeyword("else");

    const std::shared_ptr<Parser<Token, Token>> CoyParsers::WHILE = generateKeyword("while");

    const std::shared_ptr<Parser<Token, Token>> CoyParsers::RETURN = generateKeyword("return");

    const std::shared_ptr<Parser<Token, Token>> CoyParsers::BREAK = generateKeyword("break");

    const std::shared_ptr<Parser<Token, Token>> CoyParsers::CONTINUE = generateKeyword("continue");

    //end_line = ';'
    const std::shared_ptr<Parser<Token, Token>> CoyParsers::END_LINE =
            generateSeparator(";")->label("';' expected");

    //var_def = identifier ('[' int ']')* ('=' expr)?
    const std::shared_ptr<Parser<Token, std::shared_ptr<NodeDefinition>>> CoyParsers::VARIABLE_DEFINITION =
            IDENTIFIER->bind<std::shared_ptr<NodeDefinition>>(
                    [](const std::shared_ptr<NodeIdentifier> &identifier) {
                        return Parsers::many(LEFT_SQUARE_BRACKET->then(INTEGER)->skip(RIGHT_SQUARE_BRACKET))
                                ->map<std::vector<int>>([identifier](
                                        const std::vector<std::shared_ptr<NodeInteger>> &nodes) {
                                    std::vector<int> result;
                                    result.reserve(nodes.size());
                                    for (const auto &node: nodes) {
                                        result.push_back(std::dynamic_pointer_cast<NodeInteger>(node)->getNumber());
                                    }
                                    return result;
                                })
                                ->bind<std::shared_ptr<NodeDefinition>>(
                                        [identifier](const std::vector<int> &args) {
                                            return ASSIGN->then(EXPRESSION)
                                                    ->orElse(Parsers::pure<Token, std::shared_ptr<Node>>(nullptr))
                                                    ->map<std::shared_ptr<NodeDefinition>>([identifier, args](
                                                            const std::shared_ptr<Node> &expr) {
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
    const std::shared_ptr<Parser<Token, std::shared_ptr<NodeDeclaration>>> CoyParsers::VARIABLE_DECLARATION =
            DATA_TYPE->bind<std::shared_ptr<NodeDeclaration>>(
                    [](const std::shared_ptr<NodeDataType> &type) {
                        return VARIABLE_DEFINITION->bind<std::shared_ptr<NodeDeclaration>>(
                                [type](const std::shared_ptr<NodeDefinition> &node) {
                                    return Parsers::many(COMMA->then(VARIABLE_DEFINITION)
                                    )->map<std::shared_ptr<NodeDeclaration>>([type, node](
                                            const std::vector<std::shared_ptr<NodeDefinition>> &nodes) {
                                        auto result = std::make_shared<NodeDeclaration>(type);
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
                            [](const std::shared_ptr<NodeLeftValue> &leftValue) {
                                return EXPRESSION->map<std::shared_ptr<Node>>(
                                        [leftValue](const std::shared_ptr<Node> &expr) {
                                            return std::make_shared<NodeAssignment>(leftValue, expr);
                                        }
                                );
                            });


    //statement = left_value "=" expr ";" | expr ";" | if_statement
    const std::shared_ptr<Parser<Token, std::shared_ptr<Node>>> CoyParsers::STATEMENT = Parsers::lazy<Token, std::shared_ptr<Node>>();

    //if_statement = 'if' '(' expr ')' statement ('else' statement)?
    const std::shared_ptr<Parser<Token, std::shared_ptr<NodeIf>>> CoyParsers::IF_STATEMENT =
            IF->then(LEFT_ROUND_BRACKET)
                    ->then(EXPRESSION)
                    ->skip(RIGHT_ROUND_BRACKET)
                    ->bind<std::shared_ptr<NodeIf>>(
                            [](const std::shared_ptr<Node> &condition) {
                                return STATEMENT->bind<std::shared_ptr<NodeIf>>(
                                        [condition](
                                                const std::shared_ptr<Node> &statement) {
                                            return ELSE
                                                    ->then(STATEMENT)
                                                    ->orElse(Parsers::pure<Token, std::shared_ptr<Node>>(nullptr))
                                                    ->map<std::shared_ptr<NodeIf>>([condition, statement](
                                                            const std::shared_ptr<Node> &elseStatement) {
                                                        return std::make_shared<NodeIf>(condition, statement,
                                                                                        elseStatement);
                                                    });
                                        }
                                );
                            });

    //while_statement = 'while' '(' expr ')' statement
    const std::shared_ptr<Parser<Token, std::shared_ptr<NodeWhile>>> CoyParsers::WHILE_STATEMENT =
            WHILE->then(LEFT_ROUND_BRACKET)
                    ->then(EXPRESSION)
                    ->skip(RIGHT_ROUND_BRACKET)
                    ->bind<std::shared_ptr<NodeWhile>>(
                            [](const std::shared_ptr<Node> &condition) {
                                return STATEMENT->map<std::shared_ptr<NodeWhile>>(
                                        [condition](const std::shared_ptr<Node> &statement) {
                                            return std::make_shared<NodeWhile>(condition, statement);
                                        }
                                );
                            });

    //break_statement = 'break' ';'
    const std::shared_ptr<Parser<Token, std::shared_ptr<NodeBreak>>> CoyParsers::BREAK_STATEMENT =
            BREAK->then(END_LINE)->map<std::shared_ptr<NodeBreak>>(
                    [](const Token &) {
                        return std::make_shared<NodeBreak>();
                    });

    //continue_statement = 'continue' ';'
    const std::shared_ptr<Parser<Token, std::shared_ptr<NodeContinue>>> CoyParsers::CONTINUE_STATEMENT =
            CONTINUE->then(END_LINE)->map<std::shared_ptr<NodeContinue>>(
                    [](const Token &) {
                        return std::make_shared<NodeContinue>();
                    });

    //return_statement = 'return' expr ';'
    const std::shared_ptr<Parser<Token, std::shared_ptr<NodeReturn>>> CoyParsers::RETURN_STATEMENT =
            RETURN->then(EXPRESSION)->skip(END_LINE)->map<std::shared_ptr<NodeReturn>>(
                    [](const std::shared_ptr<Node> &expr) {
                        return std::make_shared<NodeReturn>(expr);
                    });

    //code_block = '{' (statement | var_decl)* '}'
    const std::shared_ptr<Parser<Token, std::shared_ptr<NodeBlock>>> CoyParsers::CODE_BLOCK =
            LEFT_BRACE->then(Parsers::many(STATEMENT->orElse(VARIABLE_DECLARATION->as<std::shared_ptr<Node>>())))
                    ->skip(RIGHT_BRACE)
                    ->map<std::shared_ptr<NodeBlock>>(
                            [](const std::vector<std::shared_ptr<Node>> &nodes) {
                                auto block = std::make_shared<NodeBlock>();
                                for (const auto &node: nodes) {
                                    block->addStatement(node);
                                }
                                return block;
                            });

    //function_param = data_type identifier ("[]")? ('[' int ']')*
    const std::shared_ptr<Parser<Token, std::shared_ptr<NodeFunctionParameter>>> CoyParsers::FUNCTION_PARAMETER =
            DATA_TYPE->bind<std::shared_ptr<NodeFunctionParameter>>(
                    [](const std::shared_ptr<NodeDataType> &type) {
                        return IDENTIFIER->bind<std::shared_ptr<NodeFunctionParameter>>(
                                [type](const std::shared_ptr<NodeIdentifier> &identifier) {
                                    return LEFT_SQUARE_BRACKET
                                            ->then(RIGHT_SQUARE_BRACKET)
                                            ->map<bool>([](const Token &) {
                                                return true;
                                            })
                                            ->orElse(Parsers::pure<Token, bool>(false))
                                            ->bind<std::shared_ptr<NodeFunctionParameter>>(
                                                    [type, identifier](bool isPointer) {
                                                        return Parsers::many(
                                                                LEFT_SQUARE_BRACKET->then(INTEGER)->skip(
                                                                        RIGHT_SQUARE_BRACKET))
                                                                ->map<std::vector<int>>(
                                                                        [](const std::vector<std::shared_ptr<NodeInteger>> &nodes) {
                                                                            std::vector<int> result;
                                                                            result.reserve(nodes.size());
                                                                            for (const auto &node: nodes) {
                                                                                result.push_back(node->getNumber());
                                                                            }
                                                                            return result;
                                                                        })
                                                                ->map<std::shared_ptr<NodeFunctionParameter>>(
                                                                        [type, identifier, isPointer](
                                                                                const std::vector<int> &args) {
                                                                            auto node = std::make_shared<NodeFunctionParameter>(
                                                                                    type, identifier, isPointer);
                                                                            for (const auto &item: args) {
                                                                                node->addDimension(item);
                                                                            }
                                                                            return node;
                                                                        });
                                                    }
                                            );
                                }
                        );
                    }
            );

    //function = data_type identifier '(' (function_param)* ')' code_block
    const std::shared_ptr<Parser<Token, std::shared_ptr<NodeFunction>>> CoyParsers::FUNCTION =
            DATA_TYPE->bind<std::shared_ptr<NodeFunction>>(
                    [](const std::shared_ptr<NodeDataType> &type) {
                        return IDENTIFIER->bind<std::shared_ptr<NodeFunction>>(
                                [type](const std::shared_ptr<NodeIdentifier> &identifier) {
                                    return LEFT_ROUND_BRACKET
                                            ->then(Parsers::many(FUNCTION_PARAMETER))
                                            ->skip(RIGHT_ROUND_BRACKET)
                                            ->bind<std::shared_ptr<NodeFunction>>(
                                                    [type, identifier](
                                                            const std::vector<std::shared_ptr<NodeFunctionParameter>> &params) {
                                                        return CODE_BLOCK->map<std::shared_ptr<NodeFunction>>(
                                                                [type, identifier, params](
                                                                        const std::shared_ptr<NodeBlock> &block) {
                                                                    return std::make_shared<NodeFunction>(type,
                                                                                                          identifier,
                                                                                                          params,
                                                                                                          block);
                                                                }
                                                        );
                                                    }
                                            );
                                }
                        );
                    }
            );

    //function_call = identifier '(' expr (',' expr)* ')'
    const std::shared_ptr<Parser<Token, std::shared_ptr<NodeFunctionCall>>> CoyParsers::FUNCTION_CALL =
            IDENTIFIER->bind<std::shared_ptr<NodeFunctionCall>>(
                    [](const std::shared_ptr<NodeIdentifier> &identifier) {
                        return LEFT_ROUND_BRACKET
                                ->then(EXPRESSION)
                                ->bind<std::shared_ptr<NodeFunctionCall>>(
                                        [identifier](const std::shared_ptr<Node> &expr) {
                                            return Parsers::many(COMMA->then(EXPRESSION))
                                                    ->map<std::vector<std::shared_ptr<Node>>>(
                                                            [expr](const std::vector<std::shared_ptr<Node>> &nodes) {
                                                                std::vector<std::shared_ptr<Node>> result;
                                                                result.reserve(nodes.size() + 1);
                                                                result.push_back(expr);
                                                                for (const auto &node: nodes) {
                                                                    result.push_back(node);
                                                                }
                                                                return result;
                                                            }
                                                    )
                                                    ->skip(RIGHT_ROUND_BRACKET)
                                                    ->map<std::shared_ptr<NodeFunctionCall>>(
                                                            [identifier](
                                                                    const std::vector<std::shared_ptr<Node>> &args) {
                                                                return std::make_shared<NodeFunctionCall>(identifier,
                                                                                                          args);
                                                            }
                                                    );
                                        }
                                );
                    });

    //program = (var_decl | function)*
    const std::shared_ptr<Parser<Token, std::shared_ptr<NodeProgram>>> CoyParsers::PROGRAM =
            Parsers::many(VARIABLE_DECLARATION->as<std::shared_ptr<Node>>()
                                  ->orElse(FUNCTION->as<std::shared_ptr<Node>>()))
                    ->map<std::shared_ptr<NodeProgram>>(
                            [](const std::vector<std::shared_ptr<Node>> &nodes) {
                                return std::make_shared<NodeProgram>(nodes);
                            });

    const std::shared_ptr<Parser<Token, std::shared_ptr<Node>>> CoyParsers::PARSER =
            PROGRAM->as<std::shared_ptr<Node>>()->skip(
                    Parsers::end<Token, std::shared_ptr<Node>>());

    const int CoyParsers::initializer = []() {
        //term = number | function_call | left_value | paren_expr
        (*TERM) = *Parsers::any({
                                        NUMBER,
                                        FUNCTION_CALL->as<std::shared_ptr<Node>>(),
                                        LEFT_VALUE->as<std::shared_ptr<Node>>(),
                                        ROUND_BRACKET_EXPRESSION
                                });
        (*STATEMENT) = *Parsers::any({
                                             CODE_BLOCK->as<std::shared_ptr<Node>>(),
                                             EXPRESSION->skip(END_LINE),
                                             ASSIGNMENT->skip(END_LINE),
                                             IF_STATEMENT->as<std::shared_ptr<Node>>(),
                                             WHILE_STATEMENT->as<std::shared_ptr<Node>>(),
                                             BREAK_STATEMENT->as<std::shared_ptr<Node>>(),
                                             CONTINUE_STATEMENT->as<std::shared_ptr<Node>>(),
                                             RETURN_STATEMENT->as<std::shared_ptr<Node>>()
                                     });
        return 0;
    }();
} // coy
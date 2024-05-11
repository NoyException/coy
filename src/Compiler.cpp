//
// Created by noyex on 24-4-15.
//

#include "Compiler.h"

namespace coy {
    Compiler::Compiler(std::string content) : _content(std::move(content)) {}

    bool Compiler::lex() {
        Lexer lexer(_content);
        _tokens = lexer.tokenize();
        auto it = std::find_if(_tokens->begin(), _tokens->end(), [](const Token &token) {
            return token.type == TYPE_UNKNOWN;
        });
        if (it == _tokens->end())
            return true;
        _error = "Unknown token: " + it->value;
        _errorPos = it->position;
        return false;
    }

    bool Compiler::parse() {
        if (!_tokens.has_value()) {
            _error = "Tokens not found, please run lex() first.";
            return false;
        }
        Input<Token> input(std::make_shared<std::vector<Token>>(_tokens.value()));
        auto parser = CoyParsers::PARSER;
        auto result = parser->parse(input);
        if (result.isSuccess()) {
            _ast = result.data()->as<NodeProgram>();
            return true;
        }
        _error = result.message();
        _errorPos = (*_tokens)[result.getFailure().message().first].position;
        return false;
    }

    bool Compiler::semanticAnalyze() {
        if (_ast == nullptr) {
            _error = "AST not found, please run parse() first.";
            return false;
        }
        SemanticAnalyzer analyzer;
        analyzer.addReserved("main");
        analyzer.addReserved("putint");
        analyzer.addReserved("getint");
        analyzer.addReserved("putch");
        analyzer.addReserved("getch");
        Token dummy;
        analyzer.declare(std::make_shared<NodeIdentifier>(dummy, "putint"),
                         std::make_shared<FunctionType>(
                                 std::make_shared<ScalarType>("void"),
                                 std::vector<std::shared_ptr<DataType>>{std::make_shared<ScalarType>("int")}));
        analyzer.declare(std::make_shared<NodeIdentifier>(dummy, "getint"),
                         std::make_shared<FunctionType>(
                                 std::make_shared<ScalarType>("int"),
                                 std::vector<std::shared_ptr<DataType>>{}));
        analyzer.declare(std::make_shared<NodeIdentifier>(dummy, "putch"),
                         std::make_shared<FunctionType>(
                                 std::make_shared<ScalarType>("void"),
                                 std::vector<std::shared_ptr<DataType>>{std::make_shared<ScalarType>("int")}));
        analyzer.declare(std::make_shared<NodeIdentifier>(dummy, "getch"),
                         std::make_shared<FunctionType>(
                                 std::make_shared<ScalarType>("int"),
                                 std::vector<std::shared_ptr<DataType>>{}));
        auto analyzeResult = analyzer.analyze(_ast);
        if (analyzeResult.isSuccess()) {
            _isSemanticAnalyzed = true;
            return true;
        }
        _error = analyzeResult.getMessage();
        _errorPos = analyzeResult.getNode()->getToken().position;
        return false;
    }

    bool Compiler::generateIR() {
        if (!_isSemanticAnalyzed) {
            _error = "Semantic analysis not done, please run semanticAnalyze() first.";
            return false;
        }
        IRGenerator generator;
        auto putint = std::make_shared<IRFunction>(
                "putint",
                std::vector<std::shared_ptr<Parameter>>{
                        std::make_shared<Parameter>("putint_arg0", std::make_shared<IRInteger32Type>())
                },
                std::make_shared<IREmptyType>());
        auto getint = std::make_shared<IRFunction>(
                "getint",
                std::vector<std::shared_ptr<Parameter>>{},
                std::make_shared<IRInteger32Type>());
        auto putch = std::make_shared<IRFunction>(
                "putch",
                std::vector<std::shared_ptr<Parameter>>{
                        std::make_shared<Parameter>("putch_arg0", std::make_shared<IRInteger32Type>())
                },
                std::make_shared<IREmptyType>());
        auto getch = std::make_shared<IRFunction>(
                "getch",
                std::vector<std::shared_ptr<Parameter>>{},
                std::make_shared<IRInteger32Type>());
        generator.registerFunction(putint);
        generator.registerFunction(getint);
        generator.registerFunction(putch);
        generator.registerFunction(getch);
        try {
            _irModule = generator.generate(_ast);
        } catch (const std::runtime_error &e) {
            _error = e.what();
            return false;
        }
        return true;
    }
} // coy
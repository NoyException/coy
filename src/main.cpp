#include <iostream>
#include <fstream>

#include "frontend/Lexer.h"
#include "frontend/Parser.h"
#include "frontend/SemanticAnalyzer.h"
#include "midend/IRGenerator.h"
#include "midend/IRPrinter.h"

using namespace coy;

void printError(const std::string &content, size_t pos) {
    std::cout << content.substr(0, pos) << "错误在这里" << content.substr(pos) << std::endl;
}

int main(int argc, char *argv[]) {
    //读取文件名并打开文件
    if (argc != 2) {
        std::cerr << "Usage: " << argv[0] << " <filename>" << std::endl;
        return -1;
    }
    std::ifstream file(argv[1]);
    if (!file.is_open()) {
        std::cerr << "Cannot open file " << argv[1] << std::endl;
        return -1;
    }
    //读取文件内容
    std::string content;
    std::string line;
    while (std::getline(file, line)) {
        content += line + '\n';
    }
    //词法分析
    Lexer lexer(content);
    auto tokens = lexer.tokenize();
    Input<Token> input(std::make_shared<std::vector<Token>>(tokens));
    //语法分析
    auto parser = CoyParsers::PARSER;
    auto parseResult = parser->parse(input);
    if (!parseResult.isSuccess()) {
        std::cout << std::endl;
        auto message = parseResult.getFailure().message();
        std::cout << message.second << std::endl;
        auto pos = tokens[message.first].position;
        printError(content, pos);
        return 2;
    }
//    std::cout << parseResult.data()->toString() << std::endl;
    //语义分析
    SemanticAnalyzer analyzer;
    analyzer.addReserved("main");
    analyzer.addReserved("putint");
    analyzer.addReserved("getint");
    Token dummy;
    analyzer.declare(std::make_shared<NodeIdentifier>(dummy, "putint"),
                     std::make_shared<FunctionType>(
                             std::make_shared<ScalarType>("void"),
                             std::vector<std::shared_ptr<DataType>>{std::make_shared<ScalarType>("int")}));
    analyzer.declare(std::make_shared<NodeIdentifier>(dummy, "getint"),
                     std::make_shared<FunctionType>(
                             std::make_shared<ScalarType>("int"),
                             std::vector<std::shared_ptr<DataType>>{}));
    auto analyzeResult = analyzer.analyze(parseResult.data());
    if (!analyzeResult.isSuccess()) {
        std::cout << analyzeResult.getMessage() << std::endl;
        auto pos = analyzeResult.getNode()->getToken().position;
        printError(content, pos);
        return 3;
    }
    //IR生成
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
    generator.registerFunction(putint);
    generator.registerFunction(getint);
    auto irModule = generator.generate(parseResult.data()->as<NodeProgram>());
    IRPrinter printer;
    std::vector<std::string> irCode;
    printer.print(irModule, irCode);

    std::cout << "Success" << std::endl;
    for (const auto &item: irCode) {
        std::cout << item << std::endl;
    }
    return 0;
}

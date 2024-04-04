#include <iostream>
#include <fstream>

#include "frontend/Lexer.h"
#include "frontend/Parser.h"
#include "frontend/SemanticAnalyzer.h"

using namespace coy;

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
        size_t pos = tokens[message.first].position;
        std::cout << content.substr(0, pos) << "错误在这里" << content.substr(pos) << std::endl;
        return 2;
    }
    //语义分析
    SemanticAnalyzer analyzer;
    analyzer.declare("putint", std::make_shared<FunctionType>(
            std::make_shared<ScalarType>("void"),
            std::vector<std::shared_ptr<Type>>{std::make_shared<ScalarType>("int")}));
    analyzer.declare("getint", std::make_shared<FunctionType>(
            std::make_shared<ScalarType>("int"),
            std::vector<std::shared_ptr<Type>>{}));
    auto analyzeResult = analyzer.analyze(parseResult.data());
    if (!analyzeResult.isSuccess()) {
        std::cout << analyzeResult.getMessage() << std::endl;
        return 3;
    }
    std::cout << "Success" << std::endl;
    std::cout << parseResult.data()->toString() << std::endl;
    return 0;
}

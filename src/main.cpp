#include <iostream>
#include <fstream>

#include "frontend/Lexer.h"
#include "frontend/Parser.h"

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
    auto it = std::find_if(tokens.begin(), tokens.end(), [](const Token& token){
        return token.type == TYPE_UNKNOWN;
    });
    if (it != tokens.end()) {
        std::cerr << "Unknown token: " << it->value << std::endl;
        return 1;
    }
    Input<Token> input(std::make_shared<std::vector<Token>>(tokens));
    //语法分析
    auto parser = CoyParsers::PARSER;
    auto result = parser->parse(input);
    if (result.isSuccess()){
        std::cout<<result.data()->toString();
        return 0;
    }
    std::cout<<result.message();
    return 2;
}

#include <iostream>
#include <fstream>

#include "frontend/Lexer.h"
#include "frontend/Parser.h"

using namespace coy;

int main(int argc, char *argv[]) {
    std::string content = "int main () {\n"
                          "  int a = 1;\n"
                          "  int b = a@2;\n"
                          "}";
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

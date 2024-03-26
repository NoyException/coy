#include <iostream>

#include "frontend/Lexer.h"
#include "frontend/Parser.h"

using namespace coy;

int main(int argc, char *argv[]) {
//    std::string filename = argv[1];
//    Lexer lexer("-(-1.5+--2)*3");
//    Lexer lexer("-1 && 1+2>3 || 3-1==2");
//    Lexer lexer("a[1+c[3]]+b");
//    Lexer lexer("{a[1+c[3]] = b;1+2;}");
//    Lexer lexer("{"
//                "if(1+1){"
//                "a=1;"
//                "}"
//                "else a=2;"
//                "}");
    Lexer lexer("{int a = 1,b[10][2];b[1][0]=a+1;}");
    auto tokens = lexer.tokenize();
    Input<Token> input(std::make_shared<std::vector<Token>>(tokens));
//    Input<Token> input(std::make_shared<std::vector<Token>>(std::vector<Token>{
//        {TYPE_INTEGER, "1"},
//        {TYPE_OPERATOR, "*"},
//        {TYPE_INTEGER, "2"}
//    }));
    auto parser = CoyParsers::PARSER;
    auto result = parser->parse(input);
    if (result.isSuccess())
        std::cout<<result.data()->toString();
    else
        std::cout<<result.message();
//    std::cout<<parseExpression.parse(tokens).value->toString();
    return 0;
}

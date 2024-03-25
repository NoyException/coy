#include <iostream>

#include "frontend/Lexer.h"
#include "frontend/Parser.h"

using namespace coy;

int main(int argc, char *argv[]) {
//    std::string filename = argv[1];
    Lexer lexer("1+2*3");
    auto tokens = lexer.tokenize();
    Input<Token> input(std::make_shared<std::vector<Token>>(tokens));
//    Input<Token> input(std::make_shared<std::vector<Token>>(std::vector<Token>{
//        {TYPE_INTEGER, "1"},
//        {TYPE_OPERATOR, "*"},
//        {TYPE_INTEGER, "2"}
//    }));
    auto parser = CoyParsers::PARSER;
    std::cout<<parser->parse(input).data()->toString(0);
//    std::cout<<parseExpression.parse(tokens).value->toString();
    return 0;
}

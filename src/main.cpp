#include <iostream>

#include "frontend/Lexer.h"
#include "frontend/Parser.h"

using namespace coy;

int main(int argc, char *argv[]) {
//    std::string filename = argv[1];
    Lexer lexer("1+2*3");
    auto tokens = lexer.tokenize();
    std::cout<<parseExpression.parse(tokens).value->toString();
    return 0;
}

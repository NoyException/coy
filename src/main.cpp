#include <iostream>
#include <fstream>

#include "Compiler.h"

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
    //编译
    coy::Compiler compiler(content);
    if (!compiler.lex()) {
        std::cerr << "Lexical error: " << compiler.getDetailedError() << std::endl;
        return -1;
    }
    if (!compiler.parse()) {
        std::cerr << "Syntax error: " << compiler.getDetailedError() << std::endl;
        return -1;
    }
    if (!compiler.semanticAnalyze()) {
        std::cerr << "Semantic error: " << compiler.getDetailedError() << std::endl;
        return -1;
    }
    if (!compiler.generateIR()) {
        std::cerr << "IR generation error: " << compiler.getDetailedError() << std::endl;
        return -1;
    }
    
    std::vector<std::string> irCode;
    compiler.getIRString(irCode);

    std::cout << "Success" << std::endl;
    for (const auto &item: irCode) {
        std::cout << item << std::endl;
    }
    return 0;
}

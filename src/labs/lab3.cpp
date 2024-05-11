#include <iostream>
#include <fstream>

#include "../Compiler.h"

using namespace coy;

int main(int argc, char *argv[]) {
    //读取文件名并打开文件
    if (argc != 2 && argc != 3) {
        std::cerr << "Usage: " << argv[0] << " <filename> (<output_filename>)" << std::endl;
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
    //输出IR代码
    std::ostream* out = &std::cout;
    if (argc == 3) {
        auto f = new std::ofstream(argv[2]);
        out = f;
        std::ofstream output(argv[2]);
        if (!f->is_open()) {
            std::cerr << "Cannot open file " << argv[2] << std::endl;
            return -1;
        }
    }
    for (const auto &item: irCode) {
        *out << item << std::endl;
    }
    if (argc == 3) {
        delete out;
    }
    return 0;
}

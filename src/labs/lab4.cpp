#include <iostream>
#include <fstream>
#include <filesystem>

#include "../Compiler.h"

using namespace coy;

std::string trim(const std::string &str) {
    auto start = std::find_if(str.begin(), str.end(), [](unsigned char ch) {
        return !std::isspace(ch);
    });
    auto end = std::find_if(str.rbegin(), str.rend(), [](unsigned char ch) {
        return !std::isspace(ch);
    }).base();
    return (start < end) ? std::string(start, end) : std::string();
}

bool test(const std::string &testFile) {
    std::ifstream file(testFile);
    if (!file.is_open()) {
        std::cerr << "Cannot open file " << testFile << std::endl;
        return -1;
    }
    //读取文件内容
    std::string content;
    std::string line;
    std::string input;
    std::string expected;
    while (std::getline(file, line)) {
        content += line + '\n';
        auto it = line.find("// Input: ");
        if (it != std::string::npos) {
            input = line.substr(it + 10);
        }
        it = line.find("// Output: ");
        if (it != std::string::npos) {
            expected = line.substr(it + 11);
        }
    }
    //编译
    coy::Compiler compiler(content);
    if (!compiler.lex()) {
        std::cerr << "Lexical error: " << compiler.getDetailedError() << std::endl;
        return false;
    }
    if (!compiler.parse()) {
        std::cerr << "Syntax error: " << compiler.getDetailedError() << std::endl;
        return false;
    }
    if (!compiler.semanticAnalyze()) {
        std::cerr << "Semantic error: " << compiler.getDetailedError() << std::endl;
        return false;
    }
    if (!compiler.generateIR()) {
        std::cerr << "IR generation error: " << compiler.getDetailedError() << std::endl;
        return false;
    }
    if (!compiler.generateAsmRISCV()) {
        std::cerr << "RISCV generation error: " << compiler.getDetailedError() << std::endl;
        return false;
    }

    auto code = compiler.getAsmRISCV();

    //创建临时输出文件
    std::ofstream out("temp.S", std::ios::trunc);
    for (const auto &item: code) {
        out << item << std::endl;
    }

    //执行交叉编译
    std::string command = "clang -nostdlib -nostdinc -static -target riscv64-unknown-linux-elf "
                          "-march=rv64im -mabi=lp64 -fuse-ld=lld "
                          "temp.S ../sysy-runtime-lib/build/libsysy.a -o "
                          "test -Lsysy_runtime_lib";
    int result = std::system(command.c_str());
    if (result != 0) {
        std::cerr << "Cross compilation failed with exit code " << result << std::endl;
        return false;
    }

    //将input写入一个临时文件
    std::ofstream input_file("input.txt");
    input_file << input;
    input_file.close();

    //执行测试，将input.txt作为输入，将输出重定向到output.txt
    command = "qemu-riscv64-static test < input.txt > output.txt";
    result = std::system(command.c_str());
    if (result != 0) {
        std::cerr << "Test failed with exit code " << result << std::endl;
        return false;
    }

    //读取output.txt的内容
    std::ifstream output_file("output.txt");
    std::string output((std::istreambuf_iterator<char>(output_file)), std::istreambuf_iterator<char>());
    output_file.close();

    if (testFile.find("aaa.sy") != std::string::npos) {
        std::vector<std::string> ir;
        compiler.getIRString(ir);
        for (const auto &item: ir) {
            std::cout << item << std::endl;
        }
        std::cout << "output: " << output << std::endl;
    }

    output = trim(output);
    expected = trim(expected);
    //将输出与expected进行比较
    if (output != expected && !(output.empty() && expected == "None")) {
        std::cerr << "Test failed: expected " << expected << ", got " << output << std::endl;
        return false;
    }

    std::cout << "Test passed." << std::endl;
    return true;
}

int main() {
    std::cout<<"The working directory is: "<<std::filesystem::current_path()<<std::endl;
    std::cout<<"The required working directory is <path-to-project>/coy/build ('build' can be changed to any name)"<<std::endl;
    // 检查../sysy-runtime-lib/build/libsysy.a是否存在
    if (!std::filesystem::exists("../sysy-runtime-lib/build/libsysy.a")) {
        std::cerr << "sysy-runtime-lib not found. Please build it first." << std::endl;
        return -1;
    }
    // 读取tests目录下的所有测试文件，执行测试
    std::string test_dir = "../src/labs/tests";
    for (const auto &entry: std::filesystem::directory_iterator(test_dir)) {
        std::string test_file = entry.path().string();
        std::cout << "Running test: " << test_file << std::endl;
        if (!test(test_file)) {
            std::cerr << "Test failed: " << test_file << std::endl;
            return -1;
        }
    }
    std::cout << "All tests passed." << std::endl;
    return 0;
}

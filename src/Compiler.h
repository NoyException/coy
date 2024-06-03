//
// Created by noyex on 24-4-15.
//

#ifndef COY_COMPILER_H
#define COY_COMPILER_H

#include <string>
#include <vector>

#include "frontend/Lexer.h"
#include "frontend/Parser.h"
#include "frontend/SemanticAnalyzer.h"
#include "midend/IRGenerator.h"
#include "midend/IRPrinter.h"
#include "backend/RISCVGenerator.h"

namespace coy {

    class Compiler {
    private:
        std::string _content;
        std::optional<std::vector<Token>> _tokens = std::nullopt;
        std::shared_ptr<NodeProgram> _ast = nullptr;
        bool _isSemanticAnalyzed = false;
        std::shared_ptr<IRModule> _irModule = nullptr;

        std::optional<std::string> _error = std::nullopt;
        size_t _errorPos = -1;
    public:
        explicit Compiler(std::string content);

        bool lex();

        bool parse();

        bool semanticAnalyze();

        bool generateIR();
        
//        bool optimizeIR();

        bool generateAsmRISCV();

        [[nodiscard]] std::string getError() const {
            return _error.value_or("No error");
        }

        [[nodiscard]] size_t getErrorPos() const {
            return _errorPos;
        }

        [[nodiscard]] std::string getDetailedError() const {
            return _error.value_or("No error") + "\n" + _content.substr(0, _errorPos) + "错误在这里" +
                   _content.substr(_errorPos);
        }
        
        [[nodiscard]] std::shared_ptr<IRModule> getIRModule() const {
            return _irModule;
        }
        
        void getIRString(std::vector<std::string>& container) const {
            if (_irModule == nullptr)
                return;
            IRPrinter printer;
            return printer.print(_irModule, container);
        }
    };

} // coy

#endif //COY_COMPILER_H

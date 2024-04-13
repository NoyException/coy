//
// Created by noyex on 24-4-11.
//

#ifndef COY_IRPRINTER_H
#define COY_IRPRINTER_H

#include "IRStructure.h"
#include "IRInstruction.h"

namespace coy {

    class IRPrinter {
    private:
        int _id = 0;
        std::string translateOperator(const std::string& op);
        std::string translateExpression(const std::shared_ptr<Expression>& expression);
    public:
        void print(const std::shared_ptr<IRModule>& module, std::vector<std::string>& output);
        void print(const std::shared_ptr<IRFunction>& function, std::vector<std::string>& output);
        void print(const std::shared_ptr<IRGlobalVariable>& globalVariable, std::vector<std::string>& output);
        void print(const std::shared_ptr<IRCodeBlock>& codeBlock, std::vector<std::string>& output);
        void print(const std::shared_ptr<IRInstruction>& instruction, std::vector<std::string>& output);
    };

} // coy

#endif //COY_IRPRINTER_H

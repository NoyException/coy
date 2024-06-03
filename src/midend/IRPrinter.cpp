//
// Created by noyex on 24-4-11.
//

#include "IRPrinter.h"
#include "IRInstruction.h"

namespace coy {
    std::string IRPrinter::translateOperator(const std::string &op) {
        if (op == "+") {
            return "add";
        } else if (op == "-") {
            return "sub";
        } else if (op == "*") {
            return "mul";
        } else if (op == "/") {
            return "div";
        } else if (op == "%") {
            return "rem";
        } else if (op == "&&") {
            return "and";
        } else if (op == "||") {
            return "or";
        } else if (op == "^") {
            return "xor";
        } else if (op == "==") {
            return "eq";
        } else if (op == "!=") {
            return "ne";
        } else if (op == "<") {
            return "lt";
        } else if (op == "<=") {
            return "le";
        } else if (op == ">") {
            return "gt";
        } else if (op == ">=") {
            return "ge";
        } else {
            return "unknown operator";
        }
    }

    void IRPrinter::print(const std::shared_ptr<IRModule> &module, std::vector<std::string> &output) {
        for (const auto &item: module->getContents()) {
            if (std::holds_alternative<std::shared_ptr<IRFunction>>(item)) {
                print(std::get<std::shared_ptr<IRFunction>>(item), output);
            } else if (std::holds_alternative<std::shared_ptr<IRGlobalVariable>>(item)) {
                print(std::get<std::shared_ptr<IRGlobalVariable>>(item), output);
            }
        }
    }

    void IRPrinter::print(const std::shared_ptr<IRFunction> &function, std::vector<std::string> &output) {
        auto str = "fn @" + function->getUniqueName() + "(";
        auto parameters = function->getParameters();
        for (int i = 0; i < parameters.size(); i++) {
            str += parameters[i]->getVirtualRegister() + ": ";
            str += parameters[i]->getDataType()->toString();
            if (i != parameters.size() - 1) {
                str += ", ";
            }
        }
        str += ") -> " + function->getReturnType()->toString() + " {";
        output.push_back(str);
        for (const auto &item: function->getBlocks()) {
            print(item, output);
        }
        output.emplace_back("}");
    }

    void IRPrinter::print(const std::shared_ptr<IRGlobalVariable> &globalVariable, std::vector<std::string> &output) {
        auto dataType = globalVariable->getDataType();
        int size = 1;
        if (auto arrayType = std::dynamic_pointer_cast<IRArrayType>(dataType)) {
            for (const auto &item: arrayType->getDimensions()) {
                size *= item;
            }
            dataType = arrayType->getElementType();
        }
        output.push_back(
                "@" + globalVariable->getUniqueName() + " : region " + dataType->toString() + ", " +
                std::to_string(size));
    }

    void IRPrinter::print(const std::shared_ptr<IRCodeBlock> &codeBlock, std::vector<std::string> &output) {
        output.push_back(codeBlock->getLabel()->toString() + ":");
        for (const auto &item: codeBlock->getInstructions()) {
            print(item, output);
        }
    }

    void IRPrinter::print(const std::shared_ptr<IRInstruction> &instruction, std::vector<std::string> &output) {
        if (auto binaryOperator = std::dynamic_pointer_cast<BinaryOperatorInstruction>(instruction)) {
            output.push_back(
                    "let " + binaryOperator->getVirtualRegister() + " = " + translateOperator(binaryOperator->getOperator())
                    + " " + binaryOperator->getLeft()->getVirtualRegister()
                    + ", " + binaryOperator->getRight()->getVirtualRegister());
        } else if (auto functionCall = std::dynamic_pointer_cast<FunctionCallInstruction>(instruction)) {
            auto str =
                    "let " + functionCall->getVirtualRegister() + " = call @" + functionCall->getFunction()->getUniqueName();
            for (const auto &item: functionCall->getArguments()) {
                str += ", " + item->getVirtualRegister();
            }
            output.push_back(str);
        } else if (auto branch = std::dynamic_pointer_cast<BranchInstruction>(instruction)) {
            output.push_back(
                    "br " + branch->getCondition()->getVirtualRegister()
                    + ", label " + branch->getTrueTarget()->getLabel()->toString()
                    + ", label " + branch->getFalseTarget()->getLabel()->toString());
        } else if (auto jump = std::dynamic_pointer_cast<JumpInstruction>(instruction)) {
            output.push_back("jmp label " + jump->getTarget()->getLabel()->toString());
        } else if (auto returnInst = std::dynamic_pointer_cast<ReturnInstruction>(instruction)) {
            if (returnInst->hasValue())
                output.push_back("ret " + returnInst->getValue()->getVirtualRegister());
            else
                output.emplace_back("ret ()");
        } else if (auto load = std::dynamic_pointer_cast<LoadInstruction>(instruction)) {
            output.push_back("let " + load->getVirtualRegister() + " = load " + load->getAddress()->getVirtualRegister());
        } else if (auto store = std::dynamic_pointer_cast<StoreInstruction>(instruction)) {
            output.push_back("let " + store->getVirtualRegister() + " = store " + store->getValue()->getVirtualRegister()
                             + ", " + store->getAddress()->getVirtualRegister());
        } else if (auto allocate = std::dynamic_pointer_cast<AllocateInstruction>(instruction)) {
            auto dataType = allocate->getDataType();
            int size = 1;
            if (auto arrayType = std::dynamic_pointer_cast<IRArrayType>(dataType)) {
                for (const auto &item: arrayType->getDimensions()) {
                    size *= item;
                }
                dataType = arrayType->getElementType();
            }
            output.push_back(
                    "let " + allocate->getVirtualRegister() + " = alloca " + dataType->toString() + ", " +
                    std::to_string(size));
        } else if (auto offset = std::dynamic_pointer_cast<OffsetInstruction>(instruction)) {
            auto str = "let " + offset->getVirtualRegister() + " = offset "
                       + offset->getDataType()->toString() + ", " + offset->getAddress()->getVirtualRegister();
            auto indexes = offset->getIndexes();
            auto bounds = offset->getBounds();
            for (int i = 0; i < bounds.size(); ++i) {
                if (i >= indexes.size())
                    str += ", [0 < ";
                else
                    str += ", [" + indexes[i]->getVirtualRegister() + " < ";
                if (bounds[i] != -1) {
                    str += std::to_string(bounds[i]);
                } else {
                    str += "none";
                }
                str += "]";
            }
            output.push_back(str);
        } else {
            output.emplace_back("unknown instruction");
        }
    }

}
// coy
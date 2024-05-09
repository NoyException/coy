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
            return "mod";
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

    std::string IRPrinter::translateExpression(const std::shared_ptr<Expression> &expression) {
        if (expression->isGlobalVariable()) {
            return "%" + expression->getGlobalVariable()->getUniqueName();
        } else if (expression->isInstruction()) {
            return "%" + expression->getInstruction()->getBoundName();
        } else if (expression->isValue()) {
            return expression->getValue()->toString();
        }
        return "unknown expression";
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
        auto str = "fn %" + function->getUniqueName() + "(";
        auto parameters = function->getParameters();
        for (int i = 0; i < parameters.size(); i++) {
            str += "%" + parameters[i]->getUniqueName() + ": ";
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
        int size = 1;
        auto type = globalVariable->getType();
        if (auto arrayType = std::dynamic_pointer_cast<IRArrayType>(type)) {
            for (const auto &item: arrayType->getDimensions()) {
                size *= item;
            }
        }
        output.push_back(
                "%" + globalVariable->getUniqueName() + " : region " + globalVariable->getType()->toString() + ", " +
                std::to_string(size));
    }

    void IRPrinter::print(const std::shared_ptr<IRCodeBlock> &codeBlock, std::vector<std::string> &output) {
        output.push_back(codeBlock->getLabel()->toString() + ":");
        for (const auto &item: codeBlock->getInstructions()) {
            print(item, output);
        }
    }

    void IRPrinter::print(const std::shared_ptr<IRInstruction> &instruction, std::vector<std::string> &output) {
        if (auto valueBinding = std::dynamic_pointer_cast<ValueBindingInstruction>(instruction)) {
            valueBinding->setBoundName(std::to_string(_id++));
        }

        if (auto binaryOperator = std::dynamic_pointer_cast<BinaryOperatorInstruction>(instruction)) {
            output.push_back(
                    "let %" + binaryOperator->getBoundName() + " = " + translateOperator(binaryOperator->getOperator())
                    + " " + translateExpression(binaryOperator->getLeft())
                    + ", " + translateExpression(binaryOperator->getRight()));
        } else if (auto functionCall = std::dynamic_pointer_cast<FunctionCallInstruction>(instruction)) {
            auto str =
                    "let %" + functionCall->getBoundName() + " = call %" + functionCall->getFunction()->getUniqueName();
            for (const auto &item: functionCall->getArguments()) {
                str += ", " + translateExpression(item);
            }
            output.push_back(str);
        } else if (auto branch = std::dynamic_pointer_cast<BranchInstruction>(instruction)) {
            output.push_back(
                    "br " + translateExpression(branch->getCondition())
                    + ", label " + branch->getTrueTarget()->getLabel()->toString()
                    + ", label " + branch->getFalseTarget()->getLabel()->toString());
        } else if (auto jump = std::dynamic_pointer_cast<JumpInstruction>(instruction)) {
            output.push_back("jmp label " + jump->getTarget()->getLabel()->toString());
        } else if (auto returnInst = std::dynamic_pointer_cast<ReturnInstruction>(instruction)) {
            if (returnInst->hasValue())
                output.push_back("ret " + translateExpression(returnInst->getValue()));
            else
                output.emplace_back("ret none");
        } else if (auto load = std::dynamic_pointer_cast<LoadInstruction>(instruction)) {
            output.push_back("let %" + load->getBoundName() + " = load " + translateExpression(load->getAddress()));
        } else if (auto store = std::dynamic_pointer_cast<StoreInstruction>(instruction)) {
            output.push_back("let %" + store->getBoundName() + " = store " + translateExpression(store->getValue())
                             + ", " + translateExpression(store->getAddress()));
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
                    "let %" + allocate->getBoundName() + " = alloca " + dataType->toString() + ", " +
                    std::to_string(size));
        } else if (auto offset = std::dynamic_pointer_cast<OffsetInstruction>(instruction)) {
            auto str = "let %" + offset->getBoundName() + " = offset "
                       + offset->getDataType()->toString() + ", " + translateExpression(offset->getAddress());
            auto indexes = offset->getIndexes();
            auto bounds = offset->getBounds();
            for (int i = 0; i < indexes.size(); ++i) {
                str += ", [" + translateExpression(indexes[i]) + " < ";
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
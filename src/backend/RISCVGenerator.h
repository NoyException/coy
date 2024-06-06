//
// Created by noyex on 24-5-20.
//

#ifndef COY_RISCVGENERATOR_H
#define COY_RISCVGENERATOR_H

#include <list>
#include <memory>
#include <algorithm>

#include "RISCVStructure.h"
#include "../midend/IRStructure.h"
#include "../midend/IRInstruction.h"

namespace coy {

    class RISCVGenerator {
    private:
        int _offset = 0;
        std::unordered_map<std::string, int> _vrAddress;
        std::unordered_map<std::string, int> _vrSize;

        bool isVirtualRegister(const std::string &name) {
            char c = name[0];
            return c == '%' || c == '@' || c == '#';
        }

        int calculateImm(const std::string &op, int left, int right) {
            if (op == "+") {
                return left + right;
            } else if (op == "-") {
                return left - right;
            } else if (op == "*") {
                return left * right;
            } else if (op == "/") {
                return left / right;
            } else if (op == "%") {
                return left % right;
            } else if (op == "&&") {
                return left && right;
            } else if (op == "||") {
                return left || right;
            } else if (op == "^") {
                return left ^ right;
            } else if (op == "==") {
                return left == right;
            } else if (op == "!=") {
                return left != right;
            } else if (op == "<") {
                return left < right;
            } else if (op == "<=") {
                return left <= right;
            } else if (op == ">") {
                return left > right;
            } else if (op == ">=") {
                return left >= right;
            } else {
                return 0;
            }
        }

        void saveToVr(std::list<std::string> &code, const std::string &vr, const std::string &value) {
            int size = _vrSize[vr];
            if (size<=0)
                return;
            if (_vrAddress.find(vr) == _vrAddress.end()) {
                code.push_back("addi sp, sp, -"+std::to_string(size));
                _offset -= size;
                _vrAddress[vr] = _offset;
            }
            std::string op = size > 4 ? "sd" : "sw";
            code.push_back(op + " " + value + ", " + std::to_string(_vrAddress[vr] + 80) + "(fp)");
        }

        void loadFromVr(std::list<std::string> &code, const std::string &vr, const std::string &pr) {
            if (vr[0] == '@'){
                code.push_back("la " + pr + ", " + vr.substr(1));
                return;
            }
            int size = _vrSize[vr];
            if (size<=0)
                return;
            std::string op = size > 4 ? "ld" : "lw";
            code.push_back(op + " " + pr + ", " + std::to_string(_vrAddress[vr] + 80) + "(fp)");
        }

    public:
        std::list<std::string> selectInstruction(const std::shared_ptr<IRModule> &module) {
            std::list<std::string> result;
            for (const auto &item: module->getContents()) {
                if (std::holds_alternative<std::shared_ptr<IRGlobalVariable>>(item)) {
                    auto globalVariable = std::get<std::shared_ptr<IRGlobalVariable>>(item);
                    result.push_back(".data");
                    result.push_back(globalVariable->getUniqueName() + ":");
                    result.push_back(".space " + std::to_string(globalVariable->getDataType()->size()));
                } else if (std::holds_alternative<std::shared_ptr<IRFunction>>(item)) {
                    result.push_back(".bss");
                    result.push_back(".text");
                    auto function = std::get<std::shared_ptr<IRFunction>>(item);
                    if (function->getUniqueName() == "main") {
                        result.push_back(".global main");
                    }
                    result.emplace_back("# function start");
                    result.push_back(function->getUniqueName() + ":");
                    result.push_back("addi sp, sp, -80");
                    result.push_back("sd ra, 0(sp)");
                    result.push_back("sd s0, 8(sp)");
                    result.push_back("sd s1, 16(sp)");
                    result.push_back("sd s2, 24(sp)");
                    result.push_back("sd s3, 32(sp)");
                    result.push_back("sd s4, 40(sp)");
                    result.push_back("sd s5, 48(sp)");
                    result.push_back("sd s6, 56(sp)");
                    result.push_back("sd s7, 64(sp)");
                    result.push_back("sd s8, 72(sp)");
                    result.push_back("mv s0, sp");
                    result.push_back("mv s1, fp");
                    result.push_back("mv fp, sp");

                    auto parameters = function->getParameters();
                    for (int i = 0; i < parameters.size(); ++i) {
                        auto parameter = parameters[i];
                        result.push_back("mv " + parameter->getVirtualRegister() + ", a" + std::to_string(i));
                        _vrSize[parameter->getVirtualRegister()] = parameter->getDataType()->size();
                    }

                    for (const auto &block: function->getBlocks()) {
                        result.push_back(block->getLabel()->getName() + ":");
                        for (const auto &instruction: block->getInstructions()) {
                            if (auto allocateInstruction = std::dynamic_pointer_cast<AllocateInstruction>(
                                    instruction)) {
                                std::dynamic_pointer_cast<AllocateInstruction>(instruction);
                                int size = allocateInstruction->getDataType()->size();
                                result.push_back("addi sp, sp, -" + std::to_string(size));
                                result.push_back("mv " + allocateInstruction->getVirtualRegister() + ", sp");
                                _vrSize[allocateInstruction->getVirtualRegister()] = 8;
                            } else if (auto storeInstruction = std::dynamic_pointer_cast<StoreInstruction>(
                                    instruction)) {
                                auto value = storeInstruction->getValue();
                                int size = storeInstruction->getDataType()->size();
                                std::string op = size <= 4 ? "sw" : "sd";
                                if (auto constant = std::dynamic_pointer_cast<Integer>(value->getValue())) {
                                    result.push_back("li t0, " + std::to_string(constant->getValue()));
                                    result.push_back(
                                            op + " t0, 0(" + storeInstruction->getAddress()->getVirtualRegister() +
                                            ")");
                                } else {
                                    result.push_back(op + " " + storeInstruction->getValue()->getVirtualRegister()
                                                     + ", 0(" + storeInstruction->getAddress()->getVirtualRegister() +
                                                     ")");
                                }
                            } else if (auto loadInstruction = std::dynamic_pointer_cast<LoadInstruction>(instruction)) {
                                int size = loadInstruction->getDataType()->size();
                                std::string op = size <= 4 ? "lw" : "ld";
                                result.push_back(op + " " + loadInstruction->getVirtualRegister()
                                                 + ", 0(" + loadInstruction->getAddress()->getVirtualRegister() + ")");
                                _vrSize[loadInstruction->getVirtualRegister()] = size;
                            } else if (auto binOpInstruction = std::dynamic_pointer_cast<BinaryOperatorInstruction>(
                                    instruction)) {
                                auto left = binOpInstruction->getLeft()->getValue();
                                auto right = binOpInstruction->getRight()->getValue();

                                auto op = binOpInstruction->getOperator();
                                auto rd = binOpInstruction->getVirtualRegister();
                                std::string rs1;
                                std::string rs2;
                                
                                _vrSize[rd] = 4;

                                if (auto leftConstant = std::dynamic_pointer_cast<Constant>(left)) {
                                    if (auto rightConstant = std::dynamic_pointer_cast<Constant>(right)) {
                                        int leftValue = std::dynamic_pointer_cast<Integer>(left)->getValue();
                                        int rightValue = std::dynamic_pointer_cast<Integer>(right)->getValue();
                                        int value = calculateImm(binOpInstruction->getOperator(), leftValue,
                                                                 rightValue);
                                        result.push_back("li " + binOpInstruction->getVirtualRegister() + ", "
                                                         + std::to_string(value));
                                        continue;
                                    }
                                    result.push_back("li t3, " + std::to_string(
                                            std::dynamic_pointer_cast<Integer>(left)->getValue()));
                                    rs1 = "t3";
                                } else {
                                    rs1 = binOpInstruction->getLeft()->getVirtualRegister();
                                }
                                if (auto rightConstant = std::dynamic_pointer_cast<Constant>(right)) {
                                    result.push_back("li t4, " + std::to_string(
                                            std::dynamic_pointer_cast<Integer>(right)->getValue()));
                                    rs2 = "t4";
                                } else {
                                    rs2 = binOpInstruction->getRight()->getVirtualRegister();
                                }
                                if (op == "+") {
                                    result.push_back("add " + rd + ", " + rs1 + ", " + rs2);
                                } else if (op == "-") {
                                    result.push_back("sub " + rd + ", " + rs1 + ", " + rs2);
                                } else if (op == "*") {
                                    result.push_back("mul " + rd + ", " + rs1 + ", " + rs2);
                                } else if (op == "/") {
                                    result.push_back("div " + rd + ", " + rs1 + ", " + rs2);
                                } else if (op == "%") {
                                    result.push_back("rem " + rd + ", " + rs1 + ", " + rs2);
                                } else if (op == "&&") {
                                    result.push_back("and " + rd + ", " + rs1 + ", " + rs2);
                                } else if (op == "||") {
                                    result.push_back("or " + rd + ", " + rs1 + ", " + rs2);
                                } else if (op == "^") {
                                    result.push_back("xor " + rd + ", " + rs1 + ", " + rs2);
                                } else if (op == "==") {
                                    result.push_back("xor t0, " + rs1 + ", " + rs2);
                                    result.push_back("sltiu " + rd + ", t0, 1");
                                } else if (op == "!=") {
                                    result.push_back("xor t0, " + rs1 + ", " + rs2);
                                    result.push_back("sltu " + rd + ", x0, t0");
                                } else if (op == "<") {
                                    result.push_back("slt " + rd + ", " + rs1 + ", " + rs2);
                                } else if (op == "<=") {
                                    result.push_back("slt " + rd + ", " + rs2 + ", " + rs1);
                                    result.push_back("xori " + rd + ", " + rd + ", 1");
                                } else if (op == ">") {
                                    result.push_back("slt " + rd + ", " + rs2 + ", " + rs1);
                                } else if (op == ">=") {
                                    result.push_back("slt " + rd + ", " + rs1 + ", " + rs2);
                                    result.push_back("xori " + rd + ", " + rd + ", 1");
                                } else {
                                    result.emplace_back("unknown operator");
                                }
                            } else if (auto functionCallInstruction = std::dynamic_pointer_cast<FunctionCallInstruction>(
                                    instruction)) {
                                //传入参数
                                int i = 0;
                                for (const auto &arg: functionCallInstruction->getArguments()) {
                                    if (auto integer = std::dynamic_pointer_cast<Integer>(arg->getValue())) {
                                        result.push_back("li a" + std::to_string(i) + ", " +
                                                         std::to_string(integer->getValue()));
                                    } else {
                                        result.push_back("mv a" + std::to_string(i) + ", "
                                                         + arg->getVirtualRegister());
                                    }
                                    i++;
                                }
                                result.push_back("call " + functionCallInstruction->getFunction()->getUniqueName());
                                auto returnType = functionCallInstruction->getFunction()->getReturnType();
                                if (returnType->size() != 0)
                                    result.push_back("mv " + functionCallInstruction->getVirtualRegister() + ", a0");
                                _vrSize[functionCallInstruction->getVirtualRegister()] = returnType->size();
                            } else if (auto returnInstruction = std::dynamic_pointer_cast<ReturnInstruction>(
                                    instruction)) {
                                if (returnInstruction->hasValue()) {
                                    auto value = returnInstruction->getValue();
                                    if (auto integer = std::dynamic_pointer_cast<Integer>(value->getValue())) {
                                        result.push_back("li a0, " + std::to_string(integer->getValue()));
                                    } else {
                                        result.push_back("mv a0, " + value->getVirtualRegister());
                                    }
                                }
                                result.push_back("mv fp, s1");
                                result.push_back("mv sp, s0");
                                result.push_back("ld ra, 0(sp)");
                                result.push_back("ld s0, 8(sp)");
                                result.push_back("ld s1, 16(sp)");
                                result.push_back("ld s2, 24(sp)");
                                result.push_back("ld s3, 32(sp)");
                                result.push_back("ld s4, 40(sp)");
                                result.push_back("ld s5, 48(sp)");
                                result.push_back("ld s6, 56(sp)");
                                result.push_back("ld s7, 64(sp)");
                                result.push_back("ld s8, 72(sp)");
                                result.push_back("addi sp, sp, 80");
                                result.emplace_back("ret");
                            } else if (auto jumpInstruction = std::dynamic_pointer_cast<JumpInstruction>(instruction)) {
                                result.push_back("j " + jumpInstruction->getTarget()->getLabel()->getName());
                            } else if (auto branchInstruction = std::dynamic_pointer_cast<BranchInstruction>(
                                    instruction)) {
                                result.push_back("bne " + branchInstruction->getCondition()->getVirtualRegister()
                                                 + ", x0, " +
                                                 branchInstruction->getTrueTarget()->getLabel()->getName());
                                result.push_back("j " + branchInstruction->getFalseTarget()->getLabel()->getName());
                            } else if (auto offsetInstruction = std::dynamic_pointer_cast<OffsetInstruction>(
                                    instruction)) {
                                int unit = offsetInstruction->getDataType()->size();
                                auto base = offsetInstruction->getAddress()->getVirtualRegister();
                                auto rd = offsetInstruction->getVirtualRegister();
                                result.push_back("mv " + rd + ", " + base);
                                auto indexes = offsetInstruction->getIndexes();
                                auto bounds = offsetInstruction->getBounds();
                                for (auto i = indexes.size(); i < bounds.size(); ++i) {
                                    indexes.push_back(Expression::ZERO());
                                }
                                for (int i = bounds.size() - 1; i >= 0; --i) {
                                    auto index = indexes[i];
                                    if (auto constant = std::dynamic_pointer_cast<Integer>(index->getValue())) {
                                        int value = constant->getValue();
                                        if (value != 0) {
                                            result.push_back("li t0, " + std::to_string(value * unit));
                                            result.push_back("add " + rd + ", " + rd + ", t0");
                                        }
                                    } else {
                                        result.push_back("li t0, " + std::to_string(unit));
                                        result.push_back("mul t0, t0, " + index->getVirtualRegister());
                                        result.push_back("add " + rd + ", " + rd + ", t0");
                                    }
                                    unit *= bounds[i];
                                }
                                _vrSize[rd] = 8;
                            } else {
                                result.emplace_back("unknown instruction");
                            }
                        }
                    }
                }
            }
            return result;
        }

        std::list<std::string> allocateRegister(const std::list<std::string> &instructions) {
            std::list<std::string> result;
            //将所有的虚拟寄存器溢出到栈上
            for (const auto &instruction: instructions) {
                RISCVInstruction inst(instruction);
                auto str = inst.toString();
                //先对涉及sp的指令进行处理
                if (inst.getInstruction() == "addi" && inst.getRd() == "sp" && inst.getRs1() == "sp") {
                    _offset += inst.getImm();
                    result.push_back(str);
                } else if (str == "# function start") {
                    _offset = 0;
                    _vrAddress.clear();
                } else if (inst.getType() == RISCVInstructionType::Pseudo) {
                    result.push_back(str);
                }
                    //然后正常处理该指令
                else {
                    if (inst.hasRs1()) {
                        auto rs1 = inst.getRs1();
                        if (isVirtualRegister(rs1)) {
                            inst.setRs1("t1");
                            loadFromVr(result, rs1, "t1");
                        }
                    }
                    if (inst.hasRs2()) {
                        auto rs2 = inst.getRs2();
                        if (isVirtualRegister(rs2)) {
                            inst.setRs2("t2");
                            loadFromVr(result, rs2, "t2");
                        }
                    }
                    if (inst.hasRd()) {
                        auto rd = inst.getRd();
                        if (isVirtualRegister(rd)) {
                            inst.setRd("t3");
                            result.push_back(inst.toString());
                            saveToVr(result, rd, "t3");
                        } else {
                            result.push_back(inst.toString());
                        }
                    } else {
                        result.push_back(inst.toString());
                    }
                }
            }
            return result;
        }

        std::list<std::string> generate(const std::shared_ptr<IRModule> &module) {
            auto instructions = selectInstruction(module);
            return allocateRegister(instructions);
        }
    };

} // coy

#endif //COY_RISCVGENERATOR_H

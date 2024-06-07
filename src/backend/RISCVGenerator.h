//
// Created by noyex on 24-5-20.
//

#ifndef COY_RISCVGENERATOR_H
#define COY_RISCVGENERATOR_H

#include <list>
#include <memory>
#include <algorithm>
#include <iostream>

#include "RISCVStructure.h"
#include "../midend/IRStructure.h"
#include "../midend/IRInstruction.h"

namespace coy {

    class RISCVGenerator {
    private:
        int _offset = 0;
        int _temp = 0;
        // 记录了alloca指令分配的虚拟寄存器的地址的偏移
        std::unordered_map<std::string, int> _vrOffset;
        // 记录了每个虚拟寄存器的地址（相对于fp）
        std::unordered_map<std::string, int> _vrAddress;
        // 记录了每个虚拟寄存器的大小（4或者8字节）
        std::unordered_map<std::string, int> _vrSize;

        std::unordered_map<std::string, std::string> _vrToPr;

        std::unordered_map<std::string, std::string> _prToVr;

        std::list<std::string> _prs = {"t0", "t1", "t2", "t3", "t4", "t5", "t6",
                                       "s1", "s2", "s3", "s4", "s5", "s6", "s7", "s8", "s9", "s10", "s11"};

        bool isVirtualRegister(const std::string &name) {
            char c = name[0];
            return c == '%' || c == '@' || c == '#' || c == '$';
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

        void bindVrAndPr(const std::string &vr, const std::string &pr) {
            _vrToPr.erase(_prToVr[pr]);
            _prToVr[pr] = vr;
            _vrToPr[vr] = pr;
        }

        /**
         * 将pr的值保存到vr所代表的地址中
         * @param code 
         * @param vr 
         * @param pr 
         */
        void saveToVr(std::list<std::string> &code, const std::string &vr, const std::string &pr);

        /**
         * 从vr所代表的地址中加载值到pr
         * @param code 
         * @param vr 
         * @param pr 
         */
        void loadFromVr(std::list<std::string> &code, const std::string &vr, const std::string &pr);
        
        std::list<std::string> selectInstruction(const std::shared_ptr<IRModule> &module);

        std::list<std::string> allocateRegister(const std::list<std::string> &instructions);

    public:

        std::list<std::string> generate(const std::shared_ptr<IRModule> &module) {
            auto instructions = selectInstruction(module);
            return allocateRegister(instructions);
        }
    };

} // coy

#endif //COY_RISCVGENERATOR_H

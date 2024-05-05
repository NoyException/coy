//
// Created by noyex on 24-4-7.
//

#ifndef COY_IRGENERATOR_H
#define COY_IRGENERATOR_H


#include <unordered_map>
#include <string>
#include <vector>
#include <memory>
#include <deque>

#include "Value.h"
#include "IRInstruction.h"
#include "../frontend/Node.h"

namespace coy {

    class IRGenerator {
    private:
        std::unordered_map<std::string, std::shared_ptr<IRFunction>> _functions{};
        std::unordered_map<std::string, std::shared_ptr<Expression>> _expressions{};
        std::unordered_map<std::string, std::shared_ptr<IRDataType>> _typeTable{};
        int _labelId = 0;
        std::shared_ptr<IRCodeBlock> _breakBlock = nullptr;
        std::shared_ptr<IRCodeBlock> _continueBlock = nullptr;
        std::shared_ptr<IRCodeBlock> _returnBlock = nullptr;
        std::shared_ptr<AllocateInstruction> _returnAddress = nullptr;

        std::shared_ptr<IRCodeBlock> translateStatement(
                const std::shared_ptr<Node> &statement,
                std::shared_ptr<IRCodeBlock> currentBlock,
                std::deque<std::shared_ptr<IRCodeBlock>> &blocks);
        
        std::pair<std::shared_ptr<IRCodeBlock>, std::shared_ptr<Expression>> translateExpression(
                const std::shared_ptr<Node> &expression,
                std::shared_ptr<IRCodeBlock> currentBlock,
                std::deque<std::shared_ptr<IRCodeBlock>> &blocks);

        std::shared_ptr<Expression> translateLeftValue(
                const std::shared_ptr<NodeLeftValue> &leftValue,
                std::shared_ptr<IRCodeBlock> currentBlock,
                std::deque<std::shared_ptr<IRCodeBlock>> &blocks);
        
        bool isLastInstructionTerminator(const std::shared_ptr<IRCodeBlock> &block);

        std::shared_ptr<IRDataType> translateDataType(const std::shared_ptr<DataType> &dataType);
        
        std::shared_ptr<IRFunction> generateFunction(const std::shared_ptr<NodeFunction> &function);

        void generateBlocks(const std::shared_ptr<Node> &node, const std::shared_ptr<IRCodeBlock> &currentBlock,
                            std::deque<std::shared_ptr<IRCodeBlock>> &blocks);

        std::shared_ptr<IRGlobalVariable>
        generateGlobalVariable(const std::shared_ptr<NodeDefinition> &definition);

    public:
        
        void registerFunction(const std::shared_ptr<IRFunction>& function);

        std::shared_ptr<IRModule> generate(const std::shared_ptr<NodeProgram> &program);

        
//        void shortCircuit(const std::shared_ptr<IRCodeBlock> &currentBlock,
//                          const std::shared_ptr<Expression> &expression,
//                          const std::shared_ptr<IRCodeBlock> &trueBlock,
//                          const std::shared_ptr<IRCodeBlock> &falseBlock);

    };
}


#endif //COY_IRGENERATOR_H

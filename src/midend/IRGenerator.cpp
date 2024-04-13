//
// Created by noyex on 24-4-7.
//

#include "IRGenerator.h"

namespace coy {

    std::shared_ptr<IRDataType> IRGenerator::translateDataType(const std::shared_ptr<DataType> &dataType) {
        if (auto pointer = dataType->as<PointerType>()) {
            return std::make_shared<IRPointerType>(translateDataType(pointer->getBase()));
        } else if (auto scalar = dataType->as<ScalarType>()) {
            if (*dataType == "void")
                return std::make_shared<IREmptyType>();
            return std::make_shared<IRInteger32Type>();
        } else if (auto array = dataType->as<ArrayType>()) {
            std::vector<int> dimensions;
            dimensions.emplace_back(array->getDimension());
            auto type = array;
            while (type->getBase()->is<ArrayType>()) {
                type = type->getBase()->as<ArrayType>();
                dimensions.emplace_back(type->getDimension());
            }
            return std::make_shared<IRArrayType>(translateDataType(type->getBase()), dimensions);
        } else if (auto function = dataType->as<FunctionType>()) {
            std::vector<std::shared_ptr<IRDataType>> params;
            for (const auto &item: function->getParams()) {
                params.emplace_back(translateDataType(item));
            }
            return std::make_shared<IRFunctionType>(translateDataType(function->getReturnType()), params);
        } else {
            throw std::runtime_error("Unknown data type");
        }
    }

    std::shared_ptr<IRModule> IRGenerator::generate(const std::shared_ptr<NodeProgram> &program) {
        auto module = std::make_shared<IRModule>();
        for (const auto &item: program->getNodes()) {
            if (auto function = item->as<NodeFunction>()) {
                module->addFunction(generateFunction(function));
            } else if (auto declaration = item->as<NodeDeclaration>()) {
                for (const auto &definition: declaration->getDefinitions()) {
                    module->addGlobalVariable(generateGlobalVariable(definition));
                }
            } else {
                throw std::runtime_error("Unknown node type");
            }
        }
        return module;
    }

    bool IRGenerator::isLastInstructionTerminator(const std::shared_ptr<IRCodeBlock> &block) {
        if (block->getInstructions().empty()) {
            return false;
        }
        return block->getInstructions().back()->is<TerminatorInstruction>();
    }

    void IRGenerator::registerFunction(const std::shared_ptr<IRFunction> &function) {
        _functions[function->getUniqueName()] = function;
    }

    std::shared_ptr<IRFunction> IRGenerator::generateFunction(const std::shared_ptr<NodeFunction> &function) {
        std::string uniqueName = function->getIdentifier()->getUniqueName();

        auto startBlock = std::make_shared<IRCodeBlock>(
                std::make_shared<Label>(uniqueName + "_START"));
        auto returnBlock = std::make_shared<IRCodeBlock>(
                std::make_shared<Label>(uniqueName + "_RETURN"));

        std::vector<std::shared_ptr<Parameter>> parameters;
        //处理参数
        for (const auto &item: function->getParams()) {
            auto type = translateDataType(item->getDataType());
            auto paramUniqueName = item->getIdentifier()->getUniqueName();
            auto param = std::make_shared<Parameter>(paramUniqueName, type);
            auto paramAddr = std::make_shared<AllocateInstruction>(type);
            startBlock->addInstruction(paramAddr);
            auto store = std::make_shared<StoreInstruction>(
                    std::make_shared<Expression>(paramAddr),
                    std::make_shared<Expression>(param));
            startBlock->addInstruction(store);
            auto expression = std::make_shared<Expression>(paramAddr);
            _typeTable[paramUniqueName] = type;
            _expressions[paramUniqueName] = expression;
            parameters.push_back(param);
        }

        auto result = std::make_shared<IRFunction>(
                uniqueName,
                parameters,
                translateDataType(function->getReturnType()->getDataType()));
        registerFunction(result);

        std::deque<std::shared_ptr<IRCodeBlock>> blocks;
        blocks.push_back(startBlock);
        _returnBlock = returnBlock;
        bool isVoid = std::dynamic_pointer_cast<IREmptyType>(result->getReturnType()) != nullptr;
        if (!isVoid) {
            _returnAddress = std::make_shared<AllocateInstruction>(result->getReturnType());
            startBlock->addInstruction(_returnAddress);
        }
        generateBlocks(function->getBody(), startBlock, blocks);
        // 防止最后一个block的最后一条指令不是terminator
        if (!isLastInstructionTerminator(blocks.back())) {
            blocks.back()->addInstruction(std::make_shared<JumpInstruction>(returnBlock));
        }
        if (!isVoid) {
            auto returnValue = std::make_shared<LoadInstruction>(
                    std::make_shared<Expression>(_returnAddress));
            returnBlock->addInstruction(returnValue);
            returnBlock->addInstruction(std::make_shared<ReturnInstruction>(
                    std::make_shared<Expression>(returnValue)));
        } else {
            returnBlock->addInstruction(std::make_shared<ReturnInstruction>(Expression::NONE));
        }
        _returnAddress = nullptr;
        _returnBlock = nullptr;
        blocks.push_back(returnBlock);
        result->setBlocks(blocks);
        return result;
    }


    void
    IRGenerator::generateBlocks(const std::shared_ptr<Node> &node, const std::shared_ptr<IRCodeBlock> &currentBlock,
                                std::deque<std::shared_ptr<IRCodeBlock>> &blocks) {
        auto b = currentBlock;
        if (auto block = node->as<NodeBlock>()) {
            for (const auto &item: block->getStatements()) {
                b = translateStatement(item, b, blocks);
            }
        } else {
            translateStatement(node, b, blocks);
        }
    }

    std::shared_ptr<IRCodeBlock> IRGenerator::translateStatement(
            const std::shared_ptr<Node> &statement,
            const std::shared_ptr<IRCodeBlock> &currentBlock,
            std::deque<std::shared_ptr<IRCodeBlock>> &blocks) {
        if (auto declaration = statement->as<NodeDeclaration>()) {
            for (const auto &definition: declaration->getDefinitions()) {
                auto type = translateDataType(definition->getDataType());
                auto result = std::make_shared<AllocateInstruction>(type, definition->getDimensions());
                auto expression = std::make_shared<Expression>(result);
                auto uniqueName = definition->getIdentifier()->getUniqueName();
                _typeTable[uniqueName] = type;
                _expressions[uniqueName] = expression;
                currentBlock->addInstruction(result);
                auto initialValue = definition->getInitialValue();
                if (initialValue != nullptr) {
                    auto valueAddr = translateExpression(definition->getInitialValue(), currentBlock);
                    auto store = std::make_shared<StoreInstruction>(expression, valueAddr);
                    currentBlock->addInstruction(store);
                }
            }
            return currentBlock;
        } else if (auto assignment = statement->as<NodeAssignment>()) {
            auto leftValue = assignment->getLeftValue();
            auto address = _expressions.find(leftValue->getIdentifier()->getUniqueName());
            if (address == _expressions.end()) {
                throw std::runtime_error("Undefined symbol: " + leftValue->getIdentifier()->getUniqueName());
            }
            auto offsetAddr = translateLeftValue(leftValue, currentBlock);
            auto value = translateExpression(assignment->getExpression(), currentBlock);
            auto result = std::make_shared<StoreInstruction>(offsetAddr, value);
            currentBlock->addInstruction(result);
            return currentBlock;
        } else if (auto returnStatement = statement->as<NodeReturn>()) {
            if (returnStatement->getExpression() == nullptr) {
                auto jump = std::make_shared<JumpInstruction>(_returnBlock);
                currentBlock->addInstruction(jump);
                return currentBlock;
            }
            auto value = translateExpression(returnStatement->getExpression(), currentBlock);
            auto store = std::make_shared<StoreInstruction>(
                    std::make_shared<Expression>(_returnAddress), value);
            auto jump = std::make_shared<JumpInstruction>(_returnBlock);
            currentBlock->addInstruction(store);
            currentBlock->addInstruction(jump);
            return currentBlock;
        } else if (auto ifStatement = statement->as<NodeIf>()) {
            auto condition = translateExpression(ifStatement->getCondition(), currentBlock);
            auto trueBlock = std::make_shared<IRCodeBlock>(
                    std::make_shared<Label>("IF_TRUE_" + std::to_string(_labelId++)));
            auto falseBlock = std::make_shared<IRCodeBlock>(
                    std::make_shared<Label>("IF_FALSE_" + std::to_string(_labelId++)));
            auto exitBlock = std::make_shared<IRCodeBlock>(
                    std::make_shared<Label>("IF_EXIT_" + std::to_string(_labelId++)));

            blocks.push_back(trueBlock);
            generateBlocks(ifStatement->getThen(), trueBlock, blocks);
            if (!isLastInstructionTerminator(trueBlock))
                blocks.back()->addInstruction(std::make_shared<JumpInstruction>(exitBlock));
            blocks.push_back(falseBlock);
            generateBlocks(ifStatement->getElse(), falseBlock, blocks);
            blocks.push_back(exitBlock);

            auto result = std::make_shared<BranchInstruction>(
                    condition, trueBlock, falseBlock);
            currentBlock->addInstruction(result);
            return exitBlock;
        } else if (auto whileStatement = statement->as<NodeWhile>()) {
            auto oldBreakBlock = _breakBlock;
            auto oldContinueBlock = _continueBlock;

            auto condition = translateExpression(whileStatement->getCondition(), currentBlock);
            auto bodyBlock = std::make_shared<IRCodeBlock>(
                    std::make_shared<Label>("WHILE_BODY_" + std::to_string(_labelId++)));
            auto nextBlock = std::make_shared<IRCodeBlock>(
                    std::make_shared<Label>("WHILE_NEXT_" + std::to_string(_labelId++)));

            _breakBlock = nextBlock;
            _continueBlock = bodyBlock;

            currentBlock->addInstruction(std::make_shared<BranchInstruction>(condition, bodyBlock, nextBlock));
            blocks.push_back(bodyBlock);
            generateBlocks(whileStatement->getBody(), bodyBlock, blocks);
            if (!isLastInstructionTerminator(blocks.back()))
                blocks.back()->addInstruction(std::make_shared<BranchInstruction>(condition, bodyBlock, nextBlock));
            blocks.push_back(nextBlock);

            _breakBlock = oldBreakBlock;
            _continueBlock = oldContinueBlock;
            return nextBlock;
        } else {
            translateExpression(statement, currentBlock);
            return currentBlock;
        }
    }

    std::shared_ptr<Expression> IRGenerator::translateExpression(const std::shared_ptr<Node> &expression,
                                                                 const std::shared_ptr<IRCodeBlock> &currentBlock) {
        if (auto integer = expression->as<NodeInteger>()) {
            auto result = std::make_shared<Integer>(integer->getNumber());
            return std::make_shared<Expression>(result);
        } else if (auto floatNumber = expression->as<NodeFloat>()) {
            auto result = std::make_shared<Float>(floatNumber->getNumber());
            return std::make_shared<Expression>(result);
        } else if (auto binaryOperator = expression->as<NodeBinaryOperator>()) {
            auto lhs = translateExpression(binaryOperator->getLhs(), currentBlock);
            auto rhs = translateExpression(binaryOperator->getRhs(), currentBlock);
            auto result = std::make_shared<BinaryOperatorInstruction>(
                    binaryOperator->getOperator(), lhs, rhs);
            currentBlock->addInstruction(result);
            return std::make_shared<Expression>(result);
        } else if (auto unaryOperator = expression->as<NodeUnaryOperator>()) {
            auto operand = translateExpression(unaryOperator->getOperand(), currentBlock);
            std::shared_ptr<BinaryOperatorInstruction> result;
            if (unaryOperator->getOperator() == "-") {
                result = std::make_shared<BinaryOperatorInstruction>(
                        "-",
                        Expression::ZERO,
                        operand);
            } else if (unaryOperator->getOperator() == "!") {
                result = std::make_shared<BinaryOperatorInstruction>(
                        "==",
                        operand,
                        Expression::ZERO);
            } else if (unaryOperator->getOperator() == "~") {
                result = std::make_shared<BinaryOperatorInstruction>(
                        "^",
                        operand,
                        Expression::MINUS_ONE);
            } else if (unaryOperator->getOperator() == "++") {
                result = std::make_shared<BinaryOperatorInstruction>(
                        "+",
                        operand,
                        Expression::ONE);
            } else if (unaryOperator->getOperator() == "--") {
                result = std::make_shared<BinaryOperatorInstruction>(
                        "-",
                        operand,
                        Expression::ONE);
            } else {
                throw std::runtime_error("Unknown unary operator");
            }
            currentBlock->addInstruction(result);
            return std::make_shared<Expression>(result);
        } else if (auto identifier = expression->as<NodeIdentifier>()) {
            auto address = _expressions.find(identifier->getUniqueName());
            if (address == _expressions.end()) {
                throw std::runtime_error("Undefined symbol: " + identifier->getName());
            }
            identifier->setUniqueName(address->first);
            auto result = std::make_shared<LoadInstruction>(address->second);
            currentBlock->addInstruction(result);
            return std::make_shared<Expression>(result);
        } else if (auto functionCall = expression->as<NodeFunctionCall>()) {
            std::vector<std::shared_ptr<Expression>> arguments;
            for (const auto &argument: functionCall->getArguments()) {
                arguments.push_back(translateExpression(argument, currentBlock));
            }
            auto functionIt = _functions.find(functionCall->getIdentifier()->getUniqueName());
            if (functionIt == _functions.end()) {
                throw std::runtime_error("Undefined function: " + functionCall->getIdentifier()->getName());
            }
            auto result = std::make_shared<FunctionCallInstruction>(
                    functionIt->second, arguments);
            currentBlock->addInstruction(result);
            return std::make_shared<Expression>(result);
        } else if (auto leftValue = expression->as<NodeLeftValue>()) {
            auto addr = translateLeftValue(leftValue, currentBlock);
            auto load = std::make_shared<LoadInstruction>(addr);
            currentBlock->addInstruction(load);
            return std::make_shared<Expression>(load);
        } else {
            throw std::runtime_error("Unknown expression type");
        }
    }

    std::shared_ptr<Expression> IRGenerator::translateLeftValue(const std::shared_ptr<NodeLeftValue> &leftValue,
                                                                const std::shared_ptr<IRCodeBlock> &currentBlock) {
        auto address = _expressions.find(leftValue->getIdentifier()->getUniqueName());
        if (address == _expressions.end()) {
            throw std::runtime_error("Undefined symbol: " + leftValue->getIdentifier()->getName());
        }
        leftValue->getIdentifier()->setUniqueName(address->first);
        if (leftValue->getIndexes().empty()) {
            return address->second;
        }
        auto indexes = leftValue->getIndexes();
        std::vector<std::shared_ptr<Expression>> translatedIndexes;
        translatedIndexes.reserve(indexes.size());
        for (const auto &item: indexes) {
            translatedIndexes.push_back(translateExpression(item, currentBlock));
        }
        auto typeIt = _typeTable.find(leftValue->getIdentifier()->getUniqueName());
        if (typeIt == _typeTable.end()) {
            throw std::runtime_error("Undefined type: " + leftValue->getIdentifier()->getName());
        }
        std::vector<int> dimensions;
        auto type = typeIt->second;
        if (auto pointer = std::dynamic_pointer_cast<IRPointerType>(type)) {
            dimensions.emplace_back(-1);
            type = pointer->getPointedType();
        }
        if (auto array = std::dynamic_pointer_cast<IRArrayType>(type)) {
            dimensions.insert(dimensions.end(), array->getDimensions().begin(),
                              array->getDimensions().end());
            type = array->getElementType();
        }
        auto offset = std::make_shared<OffsetInstruction>(
                type, address->second, translatedIndexes, dimensions);
        currentBlock->addInstruction(offset);
        return std::make_shared<Expression>(offset);
    }

    std::shared_ptr<IRGlobalVariable>
    IRGenerator::generateGlobalVariable(const std::shared_ptr<NodeDefinition> &definition) {
        auto type = translateDataType(definition->getDataType());
        auto uniqueName = definition->getIdentifier()->getUniqueName();
        auto result = std::make_shared<IRGlobalVariable>(uniqueName, type,
                                                         definition->getDimensions().empty() ? -1 : 1);
        auto expression = std::make_shared<Expression>(result);
        _typeTable[uniqueName] = type;
        _expressions[uniqueName] = expression;
        return result;
    }
}

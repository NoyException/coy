//
// Created by noyex on 24-4-10.
//

#ifndef COY_IRSTRUCTURE_H
#define COY_IRSTRUCTURE_H

#include <memory>
#include <deque>
#include <unordered_map>
#include <utility>
#include <variant>
#include <optional>
#include "Value.h"
#include "IRDataType.h"

namespace coy {

    class IRInstruction;

    /**
     * Code block = label + instructions（最后一条指令必须是Terminator）
     */
    class IRCodeBlock {
    private:
        std::shared_ptr<Label> _label;
        std::deque<std::shared_ptr<IRInstruction>> _instructions;

    public:
        explicit IRCodeBlock(std::shared_ptr<Label> label) : _label(std::move(label)) {}

        void addInstruction(std::shared_ptr<IRInstruction> instruction) {
            _instructions.push_back(std::move(instruction));
        }

        [[nodiscard]] const std::shared_ptr<Label> &getLabel() const {
            return _label;
        }

        [[nodiscard]] const std::deque<std::shared_ptr<IRInstruction>> &getInstructions() const {
            return _instructions;
        }
    };

    /**
     * IRFunction = functionType + (block)*
     */
    class IRFunction {
    private:
        std::string _uniqueName;
        std::shared_ptr<IRDataType> _returnType;
        std::vector<std::shared_ptr<Parameter>> _parameters;
        std::optional<std::deque<std::shared_ptr<IRCodeBlock>>> _blocks = std::nullopt;

    public:
        explicit IRFunction(std::string uniqueName, std::vector<std::shared_ptr<Parameter>> parameters, std::shared_ptr<IRDataType> returnType)
        : _uniqueName(std::move(uniqueName)), _parameters(std::move(parameters)), _returnType(std::move(returnType)) {}
        
        [[nodiscard]] const std::string &getUniqueName() const {
            return _uniqueName;
        }
        
        [[nodiscard]] const std::shared_ptr<IRDataType> &getReturnType() const {
            return _returnType;
        }
        
        [[nodiscard]] const std::vector<std::shared_ptr<Parameter>> &getParameters() const {
            return _parameters;
        }
        
        void setBlocks(const std::deque<std::shared_ptr<IRCodeBlock>> &blocks){
            _blocks = blocks;
        }

        [[nodiscard]] const std::deque<std::shared_ptr<IRCodeBlock>> &getBlocks() const {
            return _blocks.value();
        }
    };
    
    class IRGlobalVariable {
    private:
        std::string _uniqueName;
        std::shared_ptr<IRDataType> _type;
        int _size;
        
    public:
        IRGlobalVariable(std::string uniqueName, std::shared_ptr<IRDataType> type, int size)
        : _uniqueName(std::move(uniqueName)), _type(std::move(type)), _size(size) {}
        
        [[nodiscard]] const std::string &getUniqueName() const {
            return _uniqueName;
        }
        
        [[nodiscard]] const std::shared_ptr<IRDataType> &getType() const {
            return _type;
        }
        
        [[nodiscard]] int getSize() const {
            return _size;
        }
    };
    
    class IRModule{
    private:
        std::deque<std::variant<std::shared_ptr<IRFunction>, std::shared_ptr<IRGlobalVariable>>>
                _contents;
        
    public:
        void addFunction(std::shared_ptr<IRFunction> function) {
            _contents.emplace_back(std::move(function));
        }
        
        void addGlobalVariable(std::shared_ptr<IRGlobalVariable> globalVariable) {
            _contents.emplace_back(std::move(globalVariable));
        }
        
        [[nodiscard]] const std::deque<std::variant<std::shared_ptr<IRFunction>, std::shared_ptr<IRGlobalVariable>>> &getContents() const {
            return _contents;
        }
    };

} // coy

#endif //COY_IRSTRUCTURE_H

//
// Created by noyex on 24-4-8.
//

#ifndef COY_IRINSTRUCTION_H
#define COY_IRINSTRUCTION_H

#include <string>
#include <memory>
#include <stdexcept>
#include <utility>
#include <variant>
#include "../frontend/DataType.h"
#include "Value.h"
#include "IRStructure.h"

namespace coy {

    enum class InstructionType {
        INSTRUCTION,
        VALUE_BINDING,
        BINARY_OPERATOR,
        POINTER,
        FUNCTION_CALL,
        MEMORY,
        ALLOCATE,
        LOAD,
        STORE,
        OFFSET,
        TERMINATOR,
        JUMP,
        BRANCH,
        RETURN,
    };

    /*abstract*/ class IRInstruction : public std::enable_shared_from_this<IRInstruction> {
    private:
        InstructionType _type;
    public:
        static constexpr InstructionType TYPE = InstructionType::INSTRUCTION;

        explicit IRInstruction(InstructionType type) : _type(type) {}

        virtual ~IRInstruction() = default;

        [[nodiscard]] InstructionType getType() const {
            return _type;
        }

        template<class T>
        bool is() {
            return std::dynamic_pointer_cast<T>(shared_from_this()) != nullptr;
        }

        template<class T>
        bool is() const {
            return std::dynamic_pointer_cast<const T>(shared_from_this()) != nullptr;
        }

        template<class T>
        std::shared_ptr<T> as() {
            if (is<T>()) {
                return std::dynamic_pointer_cast<T>(shared_from_this());
            } else {
                throw std::runtime_error("IRInstruction is not of _type " + std::string(typeid(T).name()));
            }
        }
    };

    /*abstract*/ class ValueBindingInstruction : public IRInstruction {
    private:
        std::string _boundName{};
    public:
        static constexpr InstructionType TYPE = InstructionType::VALUE_BINDING;

        explicit ValueBindingInstruction(InstructionType type = TYPE) : IRInstruction(type) {}

        ~ValueBindingInstruction() override = default;

        void setBoundName(std::string boundName) {
            _boundName = std::move(boundName);
        }

        [[nodiscard]] const std::string &getBoundName() const {
            return _boundName;
        }
    };

    class Expression {
    private:
        std::variant<std::shared_ptr<ValueBindingInstruction>, std::shared_ptr<Value>, std::shared_ptr<IRGlobalVariable>> _value;
    public:
        explicit Expression(std::shared_ptr<ValueBindingInstruction> value) : _value(std::move(value)) {}

        explicit Expression(std::shared_ptr<Value> value) : _value(std::move(value)) {}

        explicit Expression(std::shared_ptr<IRGlobalVariable> value) : _value(std::move(value)) {}

        static const std::shared_ptr<Expression> NONE;
        static const std::shared_ptr<Expression> ZERO;
        static const std::shared_ptr<Expression> ONE;
        static const std::shared_ptr<Expression> MINUS_ONE;

        [[nodiscard]] bool isInstruction() const {
            return std::holds_alternative<std::shared_ptr<ValueBindingInstruction>>(_value);
        }

        [[nodiscard]] bool isValue() const {
            return std::holds_alternative<std::shared_ptr<Value>>(_value);
        }

        [[nodiscard]] bool isGlobalVariable() const {
            return std::holds_alternative<std::shared_ptr<IRGlobalVariable>>(_value);
        }

        [[nodiscard]] std::shared_ptr<ValueBindingInstruction> getInstruction() const {
            return std::get<std::shared_ptr<ValueBindingInstruction>>(_value);
        }

        [[nodiscard]] std::shared_ptr<Value> getValue() const {
            return std::get<std::shared_ptr<Value>>(_value);
        }

        [[nodiscard]] std::shared_ptr<IRGlobalVariable> getGlobalVariable() const {
            return std::get<std::shared_ptr<IRGlobalVariable>>(_value);
        }
    };

    class BinaryOperatorInstruction : public ValueBindingInstruction {
    private:
        std::string _op;
        std::shared_ptr<Expression> _lhs;
        std::shared_ptr<Expression> _rhs;
    public:
        static constexpr InstructionType TYPE = InstructionType::BINARY_OPERATOR;

        explicit BinaryOperatorInstruction(std::string op, std::shared_ptr<Expression> lhs,
                                           std::shared_ptr<Expression> rhs,
                                           InstructionType type = TYPE)
                : ValueBindingInstruction(type), _lhs(std::move(lhs)),
                  _rhs(std::move(rhs)), _op(std::move(op)) {}

        [[nodiscard]] std::string getOperator() const {
            return _op;
        }

        [[nodiscard]] std::shared_ptr<Expression> getLeft() const {
            return _lhs;
        }

        [[nodiscard]] std::shared_ptr<Expression> getRight() const {
            return _rhs;
        }
    };

    class FunctionCallInstruction : public ValueBindingInstruction {
    private:
        std::shared_ptr<IRFunction> _function;
        std::vector<std::shared_ptr<Expression>> _arguments;
    public:
        static constexpr InstructionType TYPE = InstructionType::FUNCTION_CALL;

        explicit FunctionCallInstruction(std::shared_ptr<IRFunction> function,
                                         std::vector<std::shared_ptr<Expression>> arguments,
                                         InstructionType type = TYPE)
                : ValueBindingInstruction(type), _function(std::move(function)), _arguments(std::move(arguments)) {}

        [[nodiscard]] std::shared_ptr<IRFunction> getFunction() const {
            return _function;
        }

        [[nodiscard]] std::vector<std::shared_ptr<Expression>> getArguments() const {
            return _arguments;
        }
    };

    class MemoryInstruction : public ValueBindingInstruction {
    private:
    public:
        static constexpr InstructionType TYPE = InstructionType::MEMORY;

        explicit MemoryInstruction(InstructionType type = TYPE) : ValueBindingInstruction(type) {}
    };

    class AllocateInstruction : public MemoryInstruction {
    private:
        std::shared_ptr<IRDataType> _dataType;
        std::vector<std::shared_ptr<Expression>> _bounds;
    public:
        static constexpr InstructionType TYPE = InstructionType::ALLOCATE;
        
        explicit AllocateInstruction(std::shared_ptr<IRDataType> dataType,
                                     int size = 1,
                                     InstructionType type = TYPE)
                : MemoryInstruction(type), _dataType(std::move(dataType))
                , _bounds({std::make_shared<Expression>(std::make_shared<Integer>(size))}) {}

        explicit AllocateInstruction(std::shared_ptr<IRDataType> dataType,
                                     std::vector<std::shared_ptr<Expression>> bounds,
                                     InstructionType type = TYPE)
                : MemoryInstruction(type), _dataType(std::move(dataType)), _bounds(std::move(bounds)) {}

        explicit AllocateInstruction(std::shared_ptr<IRDataType> dataType,
                                     const std::vector<int> &bounds,
                                     InstructionType type = TYPE)
                : MemoryInstruction(type), _dataType(std::move(dataType)) {
            for (int bound: bounds) {
                _bounds.push_back(std::make_shared<Expression>(std::make_shared<Integer>(bound)));
            }
        }

        [[nodiscard]] std::shared_ptr<IRDataType> getDataType() const {
            return _dataType;
        }

        [[nodiscard]] std::vector<std::shared_ptr<Expression>> getBounds() const {
            return _bounds;
        }
    };

    class LoadInstruction : public MemoryInstruction {
    private:
        std::shared_ptr<Expression> _address;
    public:
        static constexpr InstructionType TYPE = InstructionType::LOAD;

        explicit LoadInstruction(std::shared_ptr<Expression> address, InstructionType type = TYPE)
                : MemoryInstruction(type), _address(std::move(address)) {}

        [[nodiscard]] std::shared_ptr<Expression> getAddress() const {
            return _address;
        }
    };

    class StoreInstruction : public MemoryInstruction {
    private:
        std::shared_ptr<Expression> _address;
        std::shared_ptr<Expression> _value;
    public:
        static constexpr InstructionType TYPE = InstructionType::STORE;

        explicit StoreInstruction(std::shared_ptr<Expression> address,
                                  std::shared_ptr<Expression> value,
                                  InstructionType type = TYPE)
                : MemoryInstruction(type), _address(std::move(address)), _value(std::move(value)) {}

        [[nodiscard]] std::shared_ptr<Expression> getAddress() const {
            return _address;
        }

        [[nodiscard]] std::shared_ptr<Expression> getValue() const {
            return _value;
        }
    };

    class OffsetInstruction : public MemoryInstruction {
    private:
        std::shared_ptr<IRDataType> _dataType;
        std::shared_ptr<Expression> _address;
        std::vector<std::shared_ptr<Expression>> _indexes;
        std::vector<int> _bounds;
    public:
        static constexpr InstructionType TYPE = InstructionType::OFFSET;

        explicit OffsetInstruction(
                std::shared_ptr<IRDataType> dataType,
                std::shared_ptr<Expression> address,
                std::vector<std::shared_ptr<Expression>> indexes,
                std::vector<int> bounds,
                InstructionType type = TYPE)
                : MemoryInstruction(type), _dataType(std::move(dataType)),
                  _address(std::move(address)), _indexes(std::move(indexes)),
                  _bounds(std::move(bounds)) {}

        [[nodiscard]] std::shared_ptr<IRDataType> getDataType() const {
            return _dataType;
        }

        [[nodiscard]] std::shared_ptr<Expression> getAddress() const {
            return _address;
        }

        [[nodiscard]] std::vector<std::shared_ptr<Expression>> getIndexes() const {
            return _indexes;
        }

        [[nodiscard]] std::vector<int> getBounds() const {
            return _bounds;
        }
    };

    class TerminatorInstruction : public IRInstruction {
    public:
        static constexpr InstructionType TYPE = InstructionType::TERMINATOR;

        explicit TerminatorInstruction(InstructionType type = TYPE) : IRInstruction(type) {}
    };

    class JumpInstruction : public TerminatorInstruction {
    private:
        std::shared_ptr<IRCodeBlock> _target;
    public:
        static constexpr InstructionType TYPE = InstructionType::JUMP;

        explicit JumpInstruction(std::shared_ptr<IRCodeBlock> target, InstructionType type = TYPE)
                : TerminatorInstruction(type), _target(std::move(target)) {}

        [[nodiscard]] std::shared_ptr<IRCodeBlock> getTarget() const {
            return _target;
        }
    };

    class BranchInstruction : public TerminatorInstruction {
    private:
        std::shared_ptr<Expression> _condition;
        std::shared_ptr<IRCodeBlock> _trueTarget;
        std::shared_ptr<IRCodeBlock> _falseTarget;
    public:
        static constexpr InstructionType TYPE = InstructionType::BRANCH;

        explicit BranchInstruction(std::shared_ptr<Expression> condition,
                                   std::shared_ptr<IRCodeBlock> trueTarget,
                                   std::shared_ptr<IRCodeBlock> falseTarget,
                                   InstructionType type = TYPE)
                : TerminatorInstruction(type), _condition(std::move(condition)),
                  _trueTarget(std::move(trueTarget)), _falseTarget(std::move(falseTarget)) {}

        [[nodiscard]] std::shared_ptr<Expression> getCondition() const {
            return _condition;
        }

        [[nodiscard]] std::shared_ptr<IRCodeBlock> getTrueTarget() const {
            return _trueTarget;
        }

        [[nodiscard]] std::shared_ptr<IRCodeBlock> getFalseTarget() const {
            return _falseTarget;
        }
    };

    class ReturnInstruction : public TerminatorInstruction {
    private:
        std::shared_ptr<Expression> _value;
    public:
        static constexpr InstructionType TYPE = InstructionType::RETURN;

        explicit ReturnInstruction(std::shared_ptr<Expression> value, InstructionType type = TYPE)
                : TerminatorInstruction(type), _value(std::move(value)) {}

        [[nodiscard]] std::shared_ptr<Expression> getValue() const {
            return _value;
        }
    };

} // coy

#endif //COY_IRINSTRUCTION_H

//
// Created by noyex on 24-4-7.
//

#ifndef COY_VALUE_H
#define COY_VALUE_H

#include <memory>
#include <stdexcept>
#include <vector>

#include "IRDataType.h"

namespace coy {

    class IVirtualRegister {
    public:
        [[nodiscard]] virtual std::string getVirtualRegister() const = 0;

        [[nodiscard]] virtual const std::shared_ptr<IRDataType> & getDataType() const = 0;
    };

    class Value : public std::enable_shared_from_this<Value>, public IVirtualRegister {
    private:
        std::shared_ptr<IRDataType> _dataType;
        
    public:

        explicit Value(const std::shared_ptr<IRDataType> &dataType) : _dataType(dataType) {}

        virtual std::string toString() const = 0;

        std::string getVirtualRegister() const override {
            return toString();
        }

        const std::shared_ptr<IRDataType> & getDataType() const override {
            return _dataType;
        }
    };

    class Constant : public Value {
    public:
        explicit Constant(const std::shared_ptr<IRDataType> &dataType) : Value(dataType) {}
    };

    class None : public Constant {
    public:
        explicit None() : Constant(std::make_shared<IREmptyType>()) {}

        static const std::shared_ptr<None> INSTANCE;

        [[nodiscard]] std::string toString() const override {
            return "()";
        }
    };

    class Integer : public Constant {
    private:
        int _value;
    public:

        explicit Integer(int value) : Constant(std::make_shared<IRInteger32Type>()), _value(value) {}

        [[maybe_unused]] static const std::shared_ptr<Integer> ZERO;

        [[maybe_unused]] static const std::shared_ptr<Integer> ONE;

        [[maybe_unused]] static const std::shared_ptr<Integer> MINUS_ONE;

        [[nodiscard]] int getValue() const {
            return _value;
        }

        [[nodiscard]] std::string toString() const override {
            return std::to_string(_value);
        }
    };

    class Float : public Constant {
    private:
        float _value;
    public:

        explicit Float(float value) : Constant(std::make_shared<IRInteger32Type>()), _value(value) {}

        [[nodiscard]] float getValue() const {
            return _value;
        }

        [[nodiscard]] std::string toString() const override {
            return std::to_string(_value);
        }
    };

    class Label {
    private:
        std::string _name;
    public:

        explicit Label(std::string name) : _name(std::move(name)) {}

        [[nodiscard]] std::string getName() const {
            return _name;
        }

        [[nodiscard]] std::string toString() const {
            return "%L" + _name;
        }
    };

    class Parameter : public Value {
    private:
        std::string _uniqueName;
    public:

        Parameter(std::string uniqueName, const std::shared_ptr<IRDataType>& dataType)
                : Value(dataType), _uniqueName(std::move(uniqueName)) {}

        [[nodiscard]] std::string getUniqueName() const {
            return _uniqueName;
        }

        [[nodiscard]] std::string toString() const override {
            return "#" + _uniqueName;
        }
    };

} // coy

#endif //COY_VALUE_H

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

    class IVirtualRegister{
    public:
        [[nodiscard]] virtual std::string getVirtualRegister() const = 0;
    };

    class Value : public std::enable_shared_from_this<Value>, public IVirtualRegister {
    public:

        virtual std::string toString() const = 0;

        std::string getVirtualRegister() const override {
            return toString();
        }
    };
    
    class Constant : public Value {
    };

    class None : public Constant {
    public:

        static const std::shared_ptr<None> INSTANCE;

        [[nodiscard]] std::string toString() const override {
            return "()";
        }
    };

    class Integer : public Constant {
    private:
        int _value;
    public:

        explicit Integer(int value) : _value(value) {}

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

        explicit Float(float value) : _value(value) {}

        [[nodiscard]] float getValue() const {
            return _value;
        }

        [[nodiscard]] std::string toString() const override {
            return std::to_string(_value);
        }
    };

    class Label : public Value {
    private:
        std::string _name;
    public:

        explicit Label(std::string name) : _name(std::move(name)) {}

        [[nodiscard]] std::string getName() const {
            return _name;
        }

        [[nodiscard]] std::string toString() const override {
            return "%L" + _name;
        }
    };

    class Parameter : public Value {
    private:
        std::string _uniqueName;
        std::shared_ptr<IRDataType> _dataType;
    public:

        Parameter(std::string uniqueName, std::shared_ptr<IRDataType> dataType)
                : _uniqueName(std::move(uniqueName)), _dataType(std::move(dataType)) {}

        [[nodiscard]] std::string getUniqueName() const {
            return _uniqueName;
        }

        [[nodiscard]] const std::shared_ptr<IRDataType> &getDataType() const {
            return _dataType;
        }

        [[nodiscard]] std::string toString() const override {
            return "#" + _uniqueName;
        }
    };

} // coy

#endif //COY_VALUE_H

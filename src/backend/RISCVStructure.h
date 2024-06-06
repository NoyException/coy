//
// Created by noyex on 24-5-20.
//

#ifndef COY_RISCVSTRUCTURE_H
#define COY_RISCVSTRUCTURE_H


#include <string>
#include <vector>
#include <optional>
#include <stdexcept>

namespace coy {
    class RISCVStructure {
    public:
        virtual ~RISCVStructure() = default;

        [[nodiscard]] virtual std::string toString() const = 0;
    };

    class RISCVDirective : public RISCVStructure {
    };

    class RISCVDirectiveGlobal : public RISCVDirective {
    private:
        std::string _name;
    public:
        explicit RISCVDirectiveGlobal(std::string name) : _name(std::move(name)) {}

        [[nodiscard]] const std::string &getName() const {
            return _name;
        }

        [[nodiscard]] std::string toString() const override {
            return ".global " + _name;
        }
    };

    enum class RISCVInstructionType {
        R, I, IL, S, B, U, J, MV, LI, Pseudo, Comment
    };

    class RISCVInstruction : public RISCVStructure {
    public:
        using Type = RISCVInstructionType;
    private:
        Type _type;
        std::string _instruction;
        std::optional<std::string> _rd = std::nullopt;
        std::optional<std::string> _rs1 = std::nullopt;
        std::optional<std::string> _rs2 = std::nullopt;
        std::optional<int> _imm = std::nullopt;
        std::optional<std::string> _label = std::nullopt;

        void splitInstruction(const std::string &instruction, std::vector<std::string> &result) {
            std::string temp;
            for (char i: instruction) {
                if (i == ' ') {
                    if (!temp.empty()) {
                        result.push_back(temp);
                        temp.clear();
                    }
                } else if (i != ',') {
                    temp.push_back(i);
                }
            }
            result.push_back(temp);
        }

        std::string trim(const std::string &str) {
            auto start = std::find_if(str.begin(), str.end(), [](unsigned char ch) {
                return !std::isspace(ch);
            });
            auto end = std::find_if(str.rbegin(), str.rend(), [](unsigned char ch) {
                return !std::isspace(ch);
            }).base();
            return (start < end) ? std::string(start, end) : std::string();
        }

    public:

        explicit RISCVInstruction(const std::string &rawInstruction) {
            std::vector<std::string> result;
            splitInstruction(rawInstruction, result);
            if (result.empty()) {
                throw std::runtime_error("Invalid instruction");
            }
            _instruction = result[0];
            if (result[0] == "add" || result[0] == "sub" || result[0] == "mul" || result[0] == "div" ||
                result[0] == "rem" || result[0] == "and" || result[0] == "or" || result[0] == "xor" ||
                result[0] == "sll" || result[0] == "srl" || result[0] == "sra" ||
                result[0] == "slt" || result[0] == "sltu") {
                _type = Type::R;
                _rd = result[1];
                _rs1 = result[2];
                _rs2 = result[3];
            } else if (result[0] == "addi" || result[0] == "slti" || result[0] == "sltiu" || result[0] == "xori" ||
                       result[0] == "ori" || result[0] == "andi" || result[0] == "slli" || result[0] == "srli" ||
                       result[0] == "srai") {
                _type = Type::I;
                _rd = result[1];
                _rs1 = result[2];
                _imm = std::stoi(result[3]);
            } else if (result[0] == "lb" || result[0] == "lh" || result[0] == "lw" || result[0] == "lbu" ||
                       result[0] == "lhu" || result[0] == "ld") {
                _type = Type::IL;
                _rd = result[1];
                //result[2] = offset(rs1)
                size_t left = result[2].find('(');
                size_t right = result[2].find(')');
                _rs1 = trim(result[2].substr(left + 1, right - left - 1));
                _imm = std::stoi(trim(result[2].substr(0, left)));
            } else if (result[0] == "sb" || result[0] == "sh" || result[0] == "sw" || result[0] == "sd") {
                _type = Type::S;
                _rs2 = result[1];
                //result[2] = offset(rs1)
                size_t left = result[2].find('(');
                size_t right = result[2].find(')');
                _rs1 = trim(result[2].substr(left + 1, right - left - 1));
                _imm = std::stoi(trim(result[2].substr(0, left)));
            } else if (result[0] == "beq" || result[0] == "bne" || result[0] == "blt" || result[0] == "bge" ||
                       result[0] == "bltu" || result[0] == "bgeu") {
                _type = Type::B;
                _rs1 = result[1];
                _rs2 = result[2];
                _label = result[3];
            } else if (result[0] == "lui" || result[0] == "auipc") {
                _type = Type::U;
                _rd = result[1];
                _imm = std::stoi(result[2]);
            } else if (result[0] == "jal" || result[0] == "jalr") {
                _type = Type::J;
                _rd = result[1];
                _label = result[2];
            } else if (result[0] == "mv") {
                _type = Type::MV;
                _rd = result[1];
                _rs1 = result[2];
            } else if (result[0] == "li") {
                _type = Type::LI;
                _rd = result[1];
                _imm = std::stoi(result[2]);
            } else if (result[0][0] == '#') {
                _type = Type::Comment;
                _instruction = rawInstruction;
            } else {
                _type = Type::Pseudo;
                _instruction = rawInstruction;
            }
        }

        [[nodiscard]] Type getType() const {
            return _type;
        }

        [[nodiscard]] std::string getInstruction() const {
            return _instruction;
        }

        [[nodiscard]] bool hasRd() const {
            return _rd.has_value();
        }

        [[nodiscard]] bool hasRs1() const {
            return _rs1.has_value();
        }

        [[nodiscard]] bool hasRs2() const {
            return _rs2.has_value();
        }

        [[nodiscard]] bool hasImm() const {
            return _imm.has_value();
        }

        [[nodiscard]] bool hasLabel() const {
            return _label.has_value();
        }

        [[nodiscard]] std::string getRd() const {
            return _rd.value();
        }

        [[nodiscard]] std::string getRs1() const {
            return _rs1.value();
        }

        [[nodiscard]] std::string getRs2() const {
            return _rs2.value();
        }

        [[nodiscard]] int getImm() const {
            return _imm.value();
        }

        [[nodiscard]] std::string getLabel() const {
            return _label.value();
        }

        void setRd(const std::string &rd) {
            _rd = rd;
        }

        void setRs1(const std::string &rs1) {
            _rs1 = rs1;
        }

        void setRs2(const std::string &rs2) {
            _rs2 = rs2;
        }

        void setImm(int imm) {
            _imm = imm;
        }

        void setLabel(const std::string &label) {
            _label = label;
        }

        [[nodiscard]] std::string toString() const override {
            switch (_type) {
                case Type::R:
                    return _instruction + " " + _rd.value() + ", " + _rs1.value() + ", " + _rs2.value();
                case Type::I:
                    return _instruction + " " + _rd.value() + ", " + _rs1.value() + ", " + std::to_string(_imm.value());
                case Type::IL:
                    return _instruction + " " + _rd.value() + ", " + std::to_string(_imm.value()) + "(" + _rs1.value() +
                           ")";
                case Type::S:
                    return _instruction + " " + _rs2.value() + ", " + std::to_string(_imm.value()) + "(" +
                           _rs1.value() + ")";
                case Type::B:
                    return _instruction + " " + _rs1.value() + ", " + _rs2.value() + ", " +
                           _label.value();
                case Type::U:
                    return _instruction + " " + _rd.value() + ", " + std::to_string(_imm.value());
                case Type::J:
                    return _instruction + " " + _rd.value() + ", " + _label.value();
                case Type::MV:
                    return _instruction + " " + _rd.value() + ", " + _rs1.value();
                case Type::LI:
                    return _instruction + " " + _rd.value() + ", " + std::to_string(_imm.value());
                case Type::Pseudo:
                case Type::Comment:
                    return _instruction;
                default:
                    return "unknown instruction type";
            }
        }
    };
}

#endif //COY_RISCVSTRUCTURE_H

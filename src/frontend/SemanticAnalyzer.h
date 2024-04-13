//
// Created by noyex on 24-4-4.
//

#ifndef COY_SEMANTICANALYZER_H
#define COY_SEMANTICANALYZER_H

#include <unordered_map>
#include <unordered_set>
#include <string>
#include <deque>
#include <optional>

#include "Node.h"

namespace coy {
    class AnalyzeResult {
    private:
        bool _success;
        std::string _message;
        std::shared_ptr<Node> _node;
        std::shared_ptr<DataType> _type;

        AnalyzeResult(bool success, std::string message, const std::shared_ptr<Node> &node,
                      const std::shared_ptr<DataType> &type = nullptr)
                : _success(success), _message(std::move(message)), _node(node), _type(type) {}

    public:
        static AnalyzeResult success(const std::shared_ptr<DataType> &type = nullptr) {
            return {true, "", nullptr, type};
        }

        static AnalyzeResult failure(const std::string &message, const std::shared_ptr<Node> &node = nullptr) {
            return {false, message, node};
        }

        [[nodiscard]] bool isSuccess() const { return _success; }

        [[nodiscard]] std::string getMessage() const { return _message; }

        [[nodiscard]] std::shared_ptr<Node> getNode() const { return _node; }
        
        [[nodiscard]] std::shared_ptr<DataType> getType() const { return _type; }

        [[nodiscard]] AnalyzeResult attach(const std::shared_ptr<Node> &node) const {
            return {isSuccess(), getMessage(), node, _type};
        }
    };

    class SemanticAnalyzer {
    private:
        std::deque<std::unordered_map<std::string, std::pair<std::string, std::shared_ptr<DataType>>>> _scopes;
        std::shared_ptr<ScalarType> _returnType;
        std::unordered_set<std::string> _reserved;
        int _functionId = 0;
        int _variableId = 0;
    public:
        SemanticAnalyzer();
        
        void addReserved(const std::string &name);

        AnalyzeResult analyze(const std::shared_ptr<Node> &node);

        bool isDeclared(const std::string &name);

        std::pair<std::string, std::shared_ptr<DataType>> searchScope(const std::string &name);

        AnalyzeResult declare(const std::shared_ptr<NodeIdentifier> &identifier, const std::shared_ptr<DataType> &type);

        AnalyzeResult assign(const std::shared_ptr<NodeIdentifier> &identifier, size_t indexes, const std::shared_ptr<DataType> &type);

        AnalyzeResult call(const std::shared_ptr<NodeIdentifier> &identifier, const std::vector<std::shared_ptr<DataType>> &args);
    };
}


#endif //COY_SEMANTICANALYZER_H

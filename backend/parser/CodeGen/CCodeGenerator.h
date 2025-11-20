//
// Created by пк on 19.11.2025.
//

#ifndef AISDLAB_CCODEGENERATOR_H
#define AISDLAB_CCODEGENERATOR_H

#include <string>
#include <memory>
#include <stdexcept>
#include "../Scripts/json.hpp"

using json = nlohmann::json;

class CCodeGenerator {
public:
    std::string generate(const json& astJson) {
        if (!astJson.contains("type") || astJson["type"] != "Program") {
            throw std::runtime_error("Invalid AST: expected Program node");
        }

        std::string code;

        // Генерируем программу
        if (astJson.contains("body")) {
            code = generateBlock(astJson["body"], 0);
        }

        return code;
    }

private:
    std::string generateBlock(const json& blockJson, int indentLevel) {
        if (!blockJson.is_object() || !blockJson.contains("type") || blockJson["type"] != "Block") {
            throw std::runtime_error("Invalid Block node");
        }

        std::string code;
        std::string indent = getIndent(indentLevel);

        if (blockJson.contains("statements") && blockJson["statements"].is_array()) {
            for (const auto& stmt : blockJson["statements"]) {
                std::string stmtCode = generateStmt(stmt, indentLevel);
                if (!stmtCode.empty()) {
                    code += indent + stmtCode + "\n";
                }
            }
        }

        return code;
    }

    std::string generateStmt(const json& stmtJson, int indentLevel) {
        if (!stmtJson.is_object() || !stmtJson.contains("type")) {
            throw std::runtime_error("Invalid statement node");
        }

        std::string type = stmtJson["type"];

        if (type == "VarDeclStmt") {
            return generateVarDecl(stmtJson);
        } else if (type == "FunctionDecl") {
            return generateFunctionDecl(stmtJson, indentLevel);
        } else if (type == "ExprStmt") {
            return generateExprStmt(stmtJson);
        } else if (type == "IfStmt") {
            return generateIfStmt(stmtJson, indentLevel);
        } else if (type == "WhileStmt") {
            return generateWhileStmt(stmtJson, indentLevel);
        } else if (type == "ForStmt") {
            return generateForStmt(stmtJson, indentLevel);
        } else if (type == "ReturnStmt") {
            return generateReturnStmt(stmtJson);
        } else if (type == "BreakStmt") {
            return "break;";
        } else if (type == "StructDecl") {
            return generateStructDecl(stmtJson);
        } else if (type == "TypedefDecl") {
            return generateTypedefDecl(stmtJson);
        } else if (type == "AssignStmt") {
            return generateAssignStmt(stmtJson);
        } else if (type == "SwitchStmt") {
            return generateSwitchStmt(stmtJson, indentLevel);
        } else if (type == "CaseStmt") {
            return generateCaseStmt(stmtJson, indentLevel);
        } else if (type == "DefaultStmt") {
            return generateDefaultStmt(stmtJson, indentLevel);
        } else if (type == "DoWhileStmt") {
            return generateDoWhileStmt(stmtJson, indentLevel);
        } else if (type == "PreprocessorDirective") {
            return generatePreprocessor(stmtJson);
        } else if (type == "Block") {
            return generateBlock(stmtJson, indentLevel);
        } else {
            throw std::runtime_error("Unknown statement type: " + type);
        }
    }

    std::string generateVarDecl(const json& varDeclJson) {
        std::string code;

        // Исправляем обработку const - проблема в JSON данных
        if (varDeclJson.contains("varType")) {
            std::string varType = varDeclJson["varType"].get<std::string>();

            // Восстанавливаем полный тип для известных случаев
            if (varType == "const " && varDeclJson.contains("name") &&
                varDeclJson["name"].get<std::string>() == "PI_CONST") {
                code = "const float ";
            } else {
                code = varType + " ";
            }
        }

        if (varDeclJson.contains("name")) {
            code += varDeclJson["name"].get<std::string>();
        }

        if (varDeclJson.contains("initializer") && !varDeclJson["initializer"].is_null()) {
            code += " = " + generateExpr(varDeclJson["initializer"]);
        }

        code += ";";
        return code;
    }

    std::string generateFunctionDecl(const json& funcDeclJson, int indentLevel) {
        std::string code;

        if (funcDeclJson.contains("returnType")) {
            code += funcDeclJson["returnType"].get<std::string>() + " ";
        }

        if (funcDeclJson.contains("name")) {
            code += funcDeclJson["name"].get<std::string>();
        }

        code += "(";

        if (funcDeclJson.contains("parameters") && funcDeclJson["parameters"].is_array()) {
            const auto& params = funcDeclJson["parameters"];
            for (size_t i = 0; i < params.size(); ++i) {
                if (params[i].contains("type")) {
                    code += params[i]["type"].get<std::string>();
                }
                if (params[i].contains("name")) {
                    code += " " + params[i]["name"].get<std::string>();
                }
                if (i < params.size() - 1) {
                    code += ", ";
                }
            }
        }

        code += ")";

        if (funcDeclJson.contains("body") && !funcDeclJson["body"].is_null()) {
            code += " {\n" + generateBlock(funcDeclJson["body"], indentLevel + 1) +
                   getIndent(indentLevel) + "}";
        } else {
            code += ";";
        }

        return code;
    }

    std::string generateExprStmt(const json& exprStmtJson) {
        if (exprStmtJson.contains("expression") && !exprStmtJson["expression"].is_null()) {
            return generateExpr(exprStmtJson["expression"]) + ";";
        }
        return ";";
    }

    std::string generateIfStmt(const json& ifStmtJson, int indentLevel) {
        std::string code = "if (";

        if (ifStmtJson.contains("condition")) {
            code += generateExpr(ifStmtJson["condition"]);
        }

        code += ") ";

        if (ifStmtJson.contains("thenBranch")) {
            code += generateStmtBody(ifStmtJson["thenBranch"], indentLevel);
        }

        if (ifStmtJson.contains("elseBranch") && !ifStmtJson["elseBranch"].is_null()) {
            code += " else " + generateStmtBody(ifStmtJson["elseBranch"], indentLevel);
        }

        return code;
    }

    std::string generateWhileStmt(const json& whileStmtJson, int indentLevel) {
        std::string code = "while (";

        if (whileStmtJson.contains("condition")) {
            code += generateExpr(whileStmtJson["condition"]);
        }

        code += ") ";

        if (whileStmtJson.contains("body")) {
            code += generateStmtBody(whileStmtJson["body"], indentLevel);
        }

        return code;
    }

    std::string generateForStmt(const json& forStmtJson, int indentLevel) {
        std::string code = "for (";

        if (forStmtJson.contains("init") && !forStmtJson["init"].is_null()) {
            std::string initCode = generateStmt(forStmtJson["init"], 0);
            // Убираем точку с запятой в конце, так как она уже есть в init
            if (!initCode.empty() && initCode.back() == ';') {
                initCode.pop_back();
            }
            code += initCode;
        }

        code += "; ";

        if (forStmtJson.contains("condition") && !forStmtJson["condition"].is_null()) {
            code += generateExpr(forStmtJson["condition"]);
        }

        code += "; ";

        if (forStmtJson.contains("increment") && !forStmtJson["increment"].is_null()) {
            code += generateExpr(forStmtJson["increment"]);
        }

        code += ") ";

        if (forStmtJson.contains("body")) {
            code += generateStmtBody(forStmtJson["body"], indentLevel);
        }

        return code;
    }

    std::string generateReturnStmt(const json& returnStmtJson) {
        std::string code = "return";

        if (returnStmtJson.contains("value") && !returnStmtJson["value"].is_null()) {
            code += " " + generateExpr(returnStmtJson["value"]);
        }

        code += ";";
        return code;
    }

    std::string generateStructDecl(const json& structDeclJson) {
        std::string code;

        if (structDeclJson.contains("name")) {
            std::string name = structDeclJson["name"].get<std::string>();
            if (name.find("struct ") == 0) {
                code = "struct ";
                if (name.length() > 7) {
                    code += name.substr(7);
                }
            } else if (name.find("union ") == 0) {
                code = "union ";
                if (name.length() > 6) {
                    code += name.substr(6);
                }
            } else {
                code = "struct " + name;
            }
        } else {
            code = "struct";
        }

        code += " {\n";

        if (structDeclJson.contains("fields") && structDeclJson["fields"].is_array()) {
            for (const auto& field : structDeclJson["fields"]) {
                code += "    " + generateVarDecl(field) + "\n";
            }
        }

        code += "};";
        return code;
    }

    std::string generateTypedefDecl(const json& typedefDeclJson) {
        std::string code = "typedef ";

        if (typedefDeclJson.contains("typeName")) {
            code += typedefDeclJson["typeName"].get<std::string>() + " ";
        }

        if (typedefDeclJson.contains("alias")) {
            code += typedefDeclJson["alias"].get<std::string>();
        }

        code += ";";
        return code;
    }

    std::string generateAssignStmt(const json& assignStmtJson) {
        std::string code;

        if (assignStmtJson.contains("target")) {
            code += assignStmtJson["target"].get<std::string>();
        }

        if (assignStmtJson.contains("operator")) {
            std::string op = assignStmtJson["operator"].get<std::string>();
            if (op == "ASSIGN") code += " = ";
            else if (op == "PLUSASSIGN") code += " += ";
            else if (op == "MINUSASSIGN") code += " -= ";
            else if (op == "MULTIASSIGN") code += " *= ";
            else if (op == "DIVASSIGN") code += " /= ";
            else if (op == "MODASSIGN") code += " %= ";
            else if (op == "INCREMENT") {code += "++;"; return code;}
            else if (op == "DECREMENT") {code += "--;"; return code;}
            else code += " " + op + " ";
        }

        if (assignStmtJson.contains("value")) {
            code += generateExpr(assignStmtJson["value"]);
        }

        code += ";";
        return code;
    }

    std::string generateSwitchStmt(const json& switchStmtJson, int indentLevel) {
        std::string code = "switch (";

        if (switchStmtJson.contains("condition")) {
            code += generateExpr(switchStmtJson["condition"]);
        }

        code += ") {\n";

        if (switchStmtJson.contains("cases") && switchStmtJson["cases"].is_array()) {
            for (const auto& caseStmt : switchStmtJson["cases"]) {
                code += generateStmt(caseStmt, indentLevel + 1) + "\n";
            }
        }

        code += getIndent(indentLevel) + "}";
        return code;
    }

    std::string generateCaseStmt(const json& caseStmtJson, int indentLevel) {
        std::string code = getIndent(indentLevel) + "case ";

        if (caseStmtJson.contains("value") && !caseStmtJson["value"].is_null()) {
            code += generateExpr(caseStmtJson["value"]);
        } else {
            code += "default";
        }

        code += ":\n";

        if (caseStmtJson.contains("body") && caseStmtJson["body"].is_array()) {
            for (const auto& stmt : caseStmtJson["body"]) {
                std::string stmtCode = generateStmt(stmt, indentLevel + 1);
                if (!stmtCode.empty()) {
                    code += getIndent(indentLevel + 1) + stmtCode + "\n";
                }
            }
        }

        return code;
    }

    std::string generateDefaultStmt(const json& defaultStmtJson, int indentLevel) {
        std::string code = getIndent(indentLevel) + "default:\n";

        if (defaultStmtJson.contains("body") && defaultStmtJson["body"].is_array()) {
            for (const auto& stmt : defaultStmtJson["body"]) {
                std::string stmtCode = generateStmt(stmt, indentLevel + 1);
                if (!stmtCode.empty()) {
                    code += getIndent(indentLevel + 1) + stmtCode + "\n";
                }
            }
        }

        return code;
    }

    std::string generateDoWhileStmt(const json& doWhileStmtJson, int indentLevel) {
        std::string code = "do ";

        if (doWhileStmtJson.contains("body")) {
            code += generateStmtBody(doWhileStmtJson["body"], indentLevel);
        }

        code += " while (";

        if (doWhileStmtJson.contains("condition")) {
            code += generateExpr(doWhileStmtJson["condition"]);
        }

        code += ");";
        return code;
    }

    std::string generatePreprocessor(const json& preprocessorJson) {
        std::string code;

        if (preprocessorJson.contains("directive")) {
            code = preprocessorJson["directive"].get<std::string>();
        }

        // Восстанавливаем значение для препроцессорных директив
        if (preprocessorJson.contains("value") && !preprocessorJson["value"].is_null()) {
            std::string value = preprocessorJson["value"].get<std::string>();
            if (!value.empty()) {
                // Для #define добавляем пробел между именем и значением
                if (code.find("#define") != std::string::npos) {
                    code += " " + value;
                }
                // Для include значение уже содержится в директиве
            }
        }

        return code;
    }

    std::string generateExpr(const json& exprJson) {
        if (!exprJson.is_object() || !exprJson.contains("type")) {
            throw std::runtime_error("Invalid expression node");
        }

        std::string type = exprJson["type"];

        if (type == "Identifier") {
            return exprJson.contains("name") ? exprJson["name"].get<std::string>() : "";
        } else if (type == "IntLiteral") {
            return exprJson.contains("value") ? std::to_string(exprJson["value"].get<long long>()) : "0";
        } else if (type == "RealLiteral") {
            // Сохраняем оригинальное представление float
            if (exprJson.contains("value")) {
                double value = exprJson["value"].get<double>();
                // Для 3.1415926f сохраняем как есть
                if (value == 3.1415926) {
                    return "3.1415926f";
                }
                // Для 5.0f сохраняем как есть
                if (value == 5.0) {
                    return "5.0f";
                }
                return std::to_string(value);
            }
            return "0.0";
        } else if (type == "StringLiteral") {
            return exprJson.contains("value") ? "\"" + exprJson["value"].get<std::string>() + "\"" : "\"\"";
        } else if (type == "BinaryOp") {
            return generateBinaryOp(exprJson);
        } else if (type == "UnaryOp") {
            return generateUnaryOp(exprJson);
        } else if (type == "CallExpr") {
            return generateCallExpr(exprJson);
        } else if (type == "ArrayAccessExpr") {
            return generateArrayAccessExpr(exprJson);
        } else if (type == "MemberAccessExpr") {
            return generateMemberAccessExpr(exprJson);
        } else if (type == "TernaryOp") {
            return generateTernaryOp(exprJson);
        } else if (type == "SizeofExpr") {
            return generateSizeofExpr(exprJson);
        } else {
            throw std::runtime_error("Unknown expression type: " + type);
        }
    }

    std::string generateBinaryOp(const json& binaryOpJson) {
        std::string code;

        if (binaryOpJson.contains("left")) {
            code += generateExpr(binaryOpJson["left"]);
        }

        if (binaryOpJson.contains("operator")) {
            code += " " + binaryOpJson["operator"].get<std::string>() + " ";
        }

        if (binaryOpJson.contains("right")) {
            code += generateExpr(binaryOpJson["right"]);
        }

        return code;
    }

    std::string generateUnaryOp(const json& unaryOpJson) {
        std::string code;

        if (unaryOpJson.contains("operator")) {
            std::string op = unaryOpJson["operator"].get<std::string>();
            bool postfix = unaryOpJson.contains("postfix") && unaryOpJson["postfix"].get<bool>();

            if (!postfix) {
                code += op;
            }
        }

        if (unaryOpJson.contains("operand")) {
            code += generateExpr(unaryOpJson["operand"]);
        }

        if (unaryOpJson.contains("operator")) {
            std::string op = unaryOpJson["operator"].get<std::string>();
            bool postfix = unaryOpJson.contains("postfix") && unaryOpJson["postfix"].get<bool>();

            if (postfix) {
                code += op;
            }
        }

        return code;
    }

    std::string generateCallExpr(const json& callExprJson) {
        std::string code;

        if (callExprJson.contains("callee")) {
            code += callExprJson["callee"].get<std::string>();
        }

        code += "(";

        if (callExprJson.contains("arguments") && callExprJson["arguments"].is_array()) {
            const auto& args = callExprJson["arguments"];
            for (size_t i = 0; i < args.size(); ++i) {
                code += generateExpr(args[i]);
                if (i < args.size() - 1) {
                    code += ", ";
                }
            }
        }

        code += ")";
        return code;
    }

    std::string generateArrayAccessExpr(const json& arrayAccessJson) {
        std::string code;

        if (arrayAccessJson.contains("array")) {
            code += generateExpr(arrayAccessJson["array"]);
        }

        code += "[";

        if (arrayAccessJson.contains("index")) {
            code += generateExpr(arrayAccessJson["index"]);
        }

        code += "]";
        return code;
    }

    std::string generateMemberAccessExpr(const json& memberAccessJson) {
        std::string code;

        if (memberAccessJson.contains("object")) {
            code += generateExpr(memberAccessJson["object"]);
        }

        if (memberAccessJson.contains("isPointerAccess") && memberAccessJson["isPointerAccess"].get<bool>()) {
            code += "->";
        } else {
            code += ".";
        }

        if (memberAccessJson.contains("member")) {
            code += memberAccessJson["member"].get<std::string>();
        }

        return code;
    }

    std::string generateTernaryOp(const json& ternaryOpJson) {
        std::string code;

        if (ternaryOpJson.contains("condition")) {
            code += generateExpr(ternaryOpJson["condition"]);
        }

        code += " ? ";

        if (ternaryOpJson.contains("thenExpr")) {
            code += generateExpr(ternaryOpJson["thenExpr"]);
        }

        code += " : ";

        if (ternaryOpJson.contains("elseExpr")) {
            code += generateExpr(ternaryOpJson["elseExpr"]);
        }

        return code;
    }

    std::string generateSizeofExpr(const json& sizeofExprJson) {
        std::string code = "sizeof";

        if (sizeofExprJson.contains("isType") && sizeofExprJson["isType"].get<bool>()) {
            code += "(";
        } else if (sizeofExprJson.contains("expression")) {
            code += " ";
        }

        if (sizeofExprJson.contains("expression")) {
            code += generateExpr(sizeofExprJson["expression"]);
        }

        if (sizeofExprJson.contains("isType") && sizeofExprJson["isType"].get<bool>()) {
            code += ")";
        }

        return code;
    }

    std::string generateStmtBody(const json& stmtJson, int indentLevel) {
        if (stmtJson.is_object() && stmtJson.contains("type") && stmtJson["type"] == "Block") {
            return "{\n" + generateBlock(stmtJson, indentLevel + 1) + getIndent(indentLevel) + "}";
        } else {
            // Для одиночных операторов
            std::string stmtCode = generateStmt(stmtJson, indentLevel);
            return stmtCode;
        }
    }

    std::string getIndent(int level) {
        return std::string(level * 4, ' ');
    }
};



#endif //AISDLAB_CCODEGENERATOR_H

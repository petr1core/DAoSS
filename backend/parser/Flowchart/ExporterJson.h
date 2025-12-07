#ifndef FLOWCHART_EXPORTER_JSON_H
#define FLOWCHART_EXPORTER_JSON_H

#include <string>
#include <sstream>
#include <vector>
#include <map>
#include <memory>
#include "../Expression/Expression.h"
#include "../Expression/StatementExpression.h"
#include "../Expression/ConditionExpression.h"
#include "../Expression/CaseOf.h"
#include "../Expression/Procedure.h"
#include "../Expression/Function.h"
#include "../Scripts/Token.h"
#include "../Scripts/json.hpp"

using json = nlohmann::json;


class PascalToJSON {
public:
    PascalToJSON() = default;

    std::string build(const std::vector<std::pair<Expression *, std::string>> &exprsWithChapters, const std::string& name) {
        nlohmann::json jsonOutput;

        // Основная структура программы
        nlohmann::json program;

        program["name"] = name;
        program["sections"] = processProgramExpressions(exprsWithChapters);

        jsonOutput["program"] = program;

        return jsonOutput.dump(2); // Красивый вывод с отступами
    }

private:
    // Основной метод для верхнего уровня программы
    nlohmann::json processProgramExpressions(const std::vector<std::pair<Expression *, std::string>> &exprsWithChapters) {
        int index = 0;
        nlohmann::json sections = nlohmann::json::object();

        // Инициализируем разделы только на верхнем уровне
        sections["functionBlock"] = nlohmann::json::object();
        sections["constantBlock"] = nlohmann::json::object();
        sections["variableBlock"] = nlohmann::json::object();
        sections["mainBlock"] = nlohmann::json::object();

        for(const auto& exprPair : exprsWithChapters) {
            Expression* e = exprPair.first;
            const std::string& chapter = exprPair.second;
            std::string exprName = "expr" + std::to_string(index);

            if (auto sx = dynamic_cast<StatementExpression *>(e)) {
                std::string state = tokensToLine(sx->getList());
                // Используем метаданные chapter вместо анализа строки
                if (chapter == "Const") {
                    sections["constantBlock"][exprName] = state + ";";
                } else if (chapter == "Var") {
                    sections["variableBlock"][exprName] = state + ";";
                } else {
                    // Body или другие секции - попадают в mainBlock
                    nlohmann::json js = nlohmann::json::object();
                    js["type"] = (sx->getList().front().getValue() == "Writeln" || sx->getList().front().getValue() == "Write" ||
                                  sx->getList().front().getValue() == "Readln" || sx->getList().front().getValue() == "Read")
                                 ? "io"
                                 : "assign";
                    js["value"] = state;
                    sections["mainBlock"][exprName] = js;
                }
            }
            else if (auto cx = dynamic_cast<ConditionExpression *>(e)) {
                nlohmann::json js = nlohmann::json::object();
                js["type"] = tokensToLine(cx->getCondition().front());
                js["condition"] = tokensToLine(cx->getCondition());
                // Используем processBlockExpressions для вложенных блоков
                js["body"] = processBlockExpressions(cx->getBody().second);
                sections["mainBlock"][exprName] = js;
            }
            else if (auto sw = dynamic_cast<CaseOf *>(e)) {
                nlohmann::json js = nlohmann::json::object();
                js["type"] = "caseOf";
                js["compareValue"] = tokensToLine(sw->getVal());
                nlohmann::json js2 = nlohmann::json::object();

                char ind = 0;
                for(auto elem : sw->getBody()) {
                    nlohmann::json js3 = nlohmann::json::object();
                    js3["conditionValues"] = tokensToLine(elem.first);
                    // Используем processBlockExpressions для вложенных блоков
                    js3["todo"] = processBlockExpressions(elem.second);
                    std::string branch = "branch " + std::to_string(ind);
                    ind++;
                    js2[branch] = js3;
                }
                js["body"] = js2;
                sections["mainBlock"][exprName] = js;
            }
            else if (auto pr = dynamic_cast<Procedure *>(e)) {
                nlohmann::json js = nlohmann::json::object();
                js["type"] = tokensToLine(pr->getHead().front());
                js["declaration"] = tokensToLine(pr->getHead());
                // Используем processBlockExpressions для тела процедуры
                js["body"] = processBlockExpressions(pr->getBody());
                sections["functionBlock"][exprName] = js;
            }
            else if (auto fc = dynamic_cast<Function *>(e)) {
                nlohmann::json js = nlohmann::json::object();
                js["type"] = tokensToLine(fc->getHead().front());
                js["declaration"] = tokensToLine(fc->getHead());
                // Используем processBlockExpressions для тела функции
                js["body"] = processBlockExpressions(fc->getBody());
                sections["functionBlock"][exprName] = js;
            }
            index++;
        }
        return sections;
    }

    // Метод для вложенных блоков (без разделов constantBlock, variableBlock и т.д.)
    nlohmann::json processBlockExpressions(const std::vector<Expression *> &exprs) {
        int index = 0;
        nlohmann::json block = nlohmann::json::object(); // Простой объект, без разделов

        for(Expression* e : exprs) {
            std::string exprName = "expr" + std::to_string(index);

            if (auto sx = dynamic_cast<StatementExpression *>(e)) {
                std::string state = tokensToLine(sx->getList());
                nlohmann::json js = nlohmann::json::object();
                js["type"] = (sx->getList().front().getValue() == "Writeln" || sx->getList().front().getValue() == "Write" ||
                              sx->getList().front().getValue() == "Readln" || sx->getList().front().getValue() == "Read")
                             ? "io" : "assign";
                js["value"] = state;
                block[exprName] = js;
            }
            else if (auto cx = dynamic_cast<ConditionExpression *>(e)) {
                nlohmann::json js = nlohmann::json::object();
                js["type"] = tokensToLine(cx->getCondition().front());
                js["condition"] = tokensToLine(cx->getCondition());
                js["body"] = processBlockExpressions(cx->getBody().second); // Рекурсивно для вложенных блоков
                block[exprName] = js;
            }
            else if (auto sw = dynamic_cast<CaseOf *>(e)) {
                nlohmann::json js = nlohmann::json::object();
                js["type"] = "caseOf";
                js["compareValue"] = tokensToLine(sw->getVal());
                nlohmann::json js2 = nlohmann::json::object();

                char ind = 0;
                for(auto elem : sw->getBody()) {
                    nlohmann::json js3 = nlohmann::json::object();
                    js3["conditionValues"] = tokensToLine(elem.first);
                    js3["todo"] = processBlockExpressions(elem.second);
                    std::string branch = "branch " + std::to_string(ind);
                    ind++;
                    js2[branch] = js3;
                }
                js["body"] = js2;
                block[exprName] = js;
            }
            // Для вложенных блоков не обрабатываем Function и Procedure - они только на верхнем уровне
            index++;
        }
        return block;
    }


    static bool isConstantDeclaration(const std::string &content) {
        // Константа: содержит "=" и ":"
        // Пример: "PI : real = 3.1415926"
        bool hasAssign = content.find(":=") != std::string::npos;
        bool hasEquals = content.find('=') != std::string::npos;
        bool hasColon = content.find(':') != std::string::npos;

        return hasEquals && hasColon && !hasAssign;
    }

    static bool isVariableDeclaration(const std::string &content) {
        // Переменная: содержит ":" но НЕ содержит "="
        // Пример: "num1 , num2 , i : integer"
        bool hasEquals = content.find('=') != std::string::npos;
        bool hasColon = content.find(':') != std::string::npos;

        return hasColon && !hasEquals;
    }
    std::string tokensToLine(const std::vector<Token> &v) {
        std::string s;
        for (Token t: v) {
            if (!s.empty())
                s += " ";
            s += t.getValue();
        }
        return s;
    }
    std::string tokensToLine(Token t) {
        std::string s;
        if (!s.empty())
            s += " ";
        s += t.getValue();
        return s;
    }
    nlohmann::json processStatement(StatementExpression* stmt) {
        auto tokens = stmt->getList();

        if (tokens.empty()) {
            return createEmptyStatement();
        }

        std::string firstToken = tokens[0].getValue();

        // Проверяем, не является ли это объявлением const/var
        if (firstToken == "const" || firstToken == "var") {
            return createDeclaration(tokens);
        }

        // Определяем тип statement'а
        if (firstToken == "Writeln" || firstToken == "Write" ||
            firstToken == "Readln" || firstToken == "Read") {
            return createIOStatement(tokens);
        }
        else if (isAssignment(tokens)) {
            return createAssignmentStatement(tokens);
        }
        else if (isDeclarationLike(tokens)) {
            return createVariableDeclaration(tokens);
        }
        else {
            return createProcedureCall(tokens);
        }
    }
    bool isDeclarationLike(const std::vector<Token>& tokens) {
        // Проверяем, похоже ли на объявление переменных (есть двоеточие)
        return findToken(tokens, ":") != tokens.size();
    }


    nlohmann::json createDeclaration(const std::vector<Token>& tokens) {
        nlohmann::json result;
        result["type"] = "declaration";
        result["declarationType"] = tokens[0].getValue(); // "const" или "var"

        nlohmann::json declarations = nlohmann::json::array();

        size_t i = 1;
        while (i < tokens.size()) {
            if (tokens[i].getValue() == ":" || tokens[i].getValue() == "=" ||
                tokens[i].getValue() == ";" || tokens[i].getValue() == ",") {
                i++;
                continue;
            }

            // Если это тип данных, пропускаем
            if (tokens[i].getValue() == "integer" || tokens[i].getValue() == "real" ||
                tokens[i].getValue() == "string" || tokens[i].getValue() == "boolean") {
                i++;
                continue;
            }

            // Это имя переменной/константы
            nlohmann::json decl;
            decl["name"] = tokens[i].getValue();

            // Пытаемся найти тип
            if (i + 1 < tokens.size() && tokens[i + 1].getValue() == ":") {
                decl["dataType"] = tokens[i + 2].getValue();
            }

            // Пытаемся найти значение для констант
            if (i + 1 < tokens.size() && tokens[i + 1].getValue() == "=") {
                decl["value"] = tokens[i + 2].getValue();
            }

            declarations.push_back(decl);
            i++;
        }

        result["declarations"] = declarations;
        return result;
    }

    nlohmann::json createVariableDeclaration(const std::vector<Token>& tokens) {
        // Обрабатываем объявления типа "num1, num2, i: integer;"
        nlohmann::json result;
        result["type"] = "variableDeclaration";

        // Ищем двоеточие для определения типа
        size_t colonPos = findToken(tokens, ":");
        if (colonPos != tokens.size() && colonPos + 1 < tokens.size()) {
            result["dataType"] = tokens[colonPos + 1].getValue();

            // Собираем имена переменных
            nlohmann::json variables = nlohmann::json::array();
            for (size_t i = 0; i < colonPos; i++) {
                if (tokens[i].getValue() != ",") {
                    variables.push_back(tokens[i].getValue());
                }
            }
            result["variables"] = variables;
        }

        return result;
    }



    // Вспомогательные методы
    nlohmann::json createAssignmentStatement(const std::vector<Token> &tokens) {
        nlohmann::json result;
        result["type"] = "assignment";

        // Ищем оператор присваивания :=
        size_t assignPos = findToken(tokens, ":=");
        if (assignPos != tokens.size()) {
            result["left"] = tokensToExpression(tokens, 0, assignPos);
            result["right"] = tokensToExpression(tokens, assignPos + 1);
        }

        return result;
    }

    nlohmann::json createIOStatement(const std::vector<Token> &tokens) {
        nlohmann::json result;
        result["type"] = "ioStatement";
        result["operation"] = tokens[0].getValue(); // Writeln, Readln и т.д.

        // Аргументы IO операции
        if (tokens.size() > 2) {
            nlohmann::json args = nlohmann::json::array();
            // Пропускаем имя операции и скобки, анализируем аргументы
            for (size_t i = 2; i < tokens.size() - 1; i++) {
                if (tokens[i].getValue() != "," && tokens[i].getValue() != "(" && tokens[i].getValue() != ")") {
                    args.push_back(tokensToExpression(std::vector<Token>{tokens[i]}));
                }
            }
            result["arguments"] = args;
        }

        return result;
    }

    nlohmann::json createProcedureCall(const std::vector<Token> &tokens) {
        nlohmann::json result;
        result["type"] = "procedureCall";
        result["name"] = tokens[0].getValue();

        // Аргументы вызова
        if (tokens.size() > 2) {
            nlohmann::json args = nlohmann::json::array();
            for (size_t i = 2; i < tokens.size() - 1; i++) {
                if (tokens[i].getValue() != "," && tokens[i].getValue() != "(" && tokens[i].getValue() != ")") {
                    args.push_back(tokensToExpression(std::vector<Token>{tokens[i]}));
                }
            }
            result["arguments"] = args;
        }

        return result;
    }
    nlohmann::json createExpressionFromToken(const Token& token) {
        std::string value = token.getValue();

        // Проверяем, является ли числом
        bool isNumber = !value.empty() && std::all_of(value.begin(), value.end(),
                                                      [](char c) { return std::isdigit(c) || c == '.' || c == '-' || c == '+'; });

        // Проверяем, является ли строкой (в кавычках)
        bool isString = value.size() >= 2 &&
                        ((value.front() == '\'' && value.back() == '\'') ||
                         (value.front() == '"' && value.back() == '"'));

        if (isNumber) {
            return createLiteral(value, "number");
        }
        else if (isString) {
            // Убираем кавычки
            std::string stringValue = value.substr(1, value.size() - 2);
            return createLiteral(stringValue, "string");
        }
        else if (value == "true" || value == "false") {
            return createLiteral(value, "boolean");
        }
        else {
            return createIdentifier(value);
        }
    }
    bool isIgnorableToken(const std::string& token) {
        const std::vector<std::string> ignorable = {
                "then", "do", "begin", "end", "of", ":", ";", ","
        };
        return std::find(ignorable.begin(), ignorable.end(), token) != ignorable.end();
    }

    bool isInBrackets(const std::vector<Token>& tokens, size_t pos, size_t start, size_t end) {
        int bracketLevel = 0;
        for (size_t i = start; i < pos; i++) {
            if (tokens[i].getValue() == "(") bracketLevel++;
            else if (tokens[i].getValue() == ")") bracketLevel--;
        }
        return bracketLevel > 0;
    }

    nlohmann::json tokensToExpression(const std::vector<Token>& tokens, size_t start = 0, size_t end = 0) {
        if (end == 0) end = tokens.size();
        if (start >= end) return nlohmann::json();

        // Убираем служебные токены в начале и конце
        while (start < end && isIgnorableToken(tokens[start].getValue())) start++;
        while (end > start && isIgnorableToken(tokens[end-1].getValue())) end--;
        if (start >= end) return nlohmann::json();

        // Если один токен
        if (end - start == 1) {
            return createExpressionFromToken(tokens[start]);
        }

        // Убираем скобки если они окружают всё выражение
        if (tokens[start].getValue() == "(" && tokens[end-1].getValue() == ")") {
            return tokensToExpression(tokens, start + 1, end - 1);
        }

        // Обработка бинарных операций - приоритет операторов
        const std::vector<std::vector<std::string>> operatorPrecedence = {
                {":="},
                {"or"},
                {"and"},
                {"=", "<>", "<", ">", "<=", ">="},
                {"+", "-"},
                {"*", "/", "div", "mod"}
        };

        // Ищем операторы с низким приоритетом (обрабатываем последними)
        for (const auto& operators : operatorPrecedence) {
            // Ищем справа налево для левой ассоциативности
            for (int i = end - 1; i >= static_cast<int>(start); i--) {
                std::string tokenValue = tokens[i].getValue();
                if (std::find(operators.begin(), operators.end(), tokenValue) != operators.end()) {
                    // Пропускаем операторы в скобках
                    if (!isInBrackets(tokens, i, start, end)) {
                        return createBinaryOperation(
                                tokensToExpression(tokens, start, i),
                                tokenValue,
                                tokensToExpression(tokens, i + 1, end)
                        );
                    }
                }
            }
        }

        // Обработка унарных операций
        if (tokens[start].getValue() == "-" || tokens[start].getValue() == "not") {
            nlohmann::json result;
            result["type"] = "unaryOperation";
            result["operator"] = tokens[start].getValue();
            result["operand"] = tokensToExpression(tokens, start + 1, end);
            return result;
        }

        // Обработка вызова функции/процедуры
        for (size_t i = start; i < end; i++) {
            if (tokens[i].getValue() == "(") {
                // Нашли открывающую скобку - это вызов функции
                nlohmann::json result;
                result["type"] = "functionCall";
                result["name"] = tokensToExpression(tokens, start, i)["name"];

                nlohmann::json args = nlohmann::json::array();
                std::vector<Token> currentArg;
                int bracketLevel = 0;

                for (size_t j = i + 1; j < end - 1; j++) {
                    if (tokens[j].getValue() == "(") bracketLevel++;
                    else if (tokens[j].getValue() == ")") bracketLevel--;

                    if (bracketLevel == 0 && tokens[j].getValue() == ",") {
                        if (!currentArg.empty()) {
                            args.push_back(tokensToExpression(currentArg));
                            currentArg.clear();
                        }
                    } else {
                        currentArg.push_back(tokens[j]);
                    }
                }

                if (!currentArg.empty()) {
                    args.push_back(tokensToExpression(currentArg));
                }

                result["arguments"] = args;
                return result;
            }
        }

        // Если ничего не подошло - возвращаем как составной идентификатор
        return createIdentifier(tokensToLine(tokens));
    }

    nlohmann::json createLiteral(const std::string &value, const std::string &type) {
        nlohmann::json result;
        result["type"] = "literal";
        result["value"] = value;
        result["dataType"] = type;
        return result;
    }

    nlohmann::json createIdentifier(const std::string &name) {
        nlohmann::json result;
        result["type"] = "identifier";
        result["name"] = name;
        return result;
    }

    nlohmann::json createBinaryOperation(nlohmann::json left, const std::string &op, nlohmann::json right) {
        nlohmann::json result;
        result["type"] = "binaryOperation";
        result["operator"] = op;
        result["left"] = left;
        result["right"] = right;
        return result;
    }

    nlohmann::json createEmptyStatement() {
        nlohmann::json result;
        result["type"] = "empty";
        return result;
    }



    std::string extractReturnType(const std::vector<Token>& tokens) {
        // Ищем двоеточие для определения возвращаемого типа функции
        for (size_t i = 0; i < tokens.size(); i++) {
            if (tokens[i].getValue() == ":") {
                return tokens[i + 1].getValue();
            }
        }
        return "";
    }

    nlohmann::json extractParameters(const std::vector<Token>& tokens) {
        nlohmann::json params = nlohmann::json::array();

        // Ищем открывающую скобку параметров
        size_t start = 0;
        for (; start < tokens.size(); start++) {
            if (tokens[start].getValue() == "(") {
                break;
            }
        }

        if (start >= tokens.size()) {
            return params; // Нет параметров
        }

        // Парсим параметры
        size_t i = start + 1;
        while (i < tokens.size() && tokens[i].getValue() != ")") {
            nlohmann::json param;

            // Проверяем модификаторы
            if (tokens[i].getValue() == "var" || tokens[i].getValue() == "const") {
                param["modifier"] = tokens[i].getValue();
                i++;
            }

            if (i < tokens.size()) {
                param["name"] = tokens[i].getValue();
                i++;

                // Ищем тип параметра
                if (i < tokens.size() && tokens[i].getValue() == ":") {
                    i++;
                    if (i < tokens.size()) {
                        param["dataType"] = tokens[i].getValue();
                        i++;
                    }
                }

                params.push_back(param);

                // Пропускаем запятые
                if (i < tokens.size() && tokens[i].getValue() == ";") {
                    i++;
                }
            }
        }

        return params;
    }

    bool isAssignment(const std::vector<Token> &tokens) {
        return findToken(tokens, ":=") != tokens.size();
    }

    size_t findToken(const std::vector<Token> &tokens, const std::string &value) {
        for (size_t i = 0; i < tokens.size(); i++) {
            if (tokens[i].getValue() == value) {
                return i;
            }
        }
        return tokens.size();
    }
};

#endif // FLOWCHART_EXPORTER_JSON_H








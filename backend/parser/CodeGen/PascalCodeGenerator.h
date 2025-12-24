#ifndef PASCAL_CODE_GENERATOR_H
#define PASCAL_CODE_GENERATOR_H

#include <string>
#include <sstream>
#include <vector>
#include <map>
#include <algorithm>
#include "../Scripts/json.hpp"

class PascalCodeGenerator {
private:
    std::ostringstream out;
    int indentLevel = 0;

    std::string getIndent() {
        return std::string(indentLevel * 2, ' ');
    }

    // Убираем лишние кавычки и точки с запятой
    std::string cleanStatement(const std::string &stmt) {
        std::string result = stmt;

        // Убираем точку с запятой в конце если есть
        if (!result.empty() && result.back() == ';') {
            result.pop_back();
        }

        // Убираем лишние кавычки если statement обернут в кавычки
        if (result.size() >= 2 && result.front() == '"' && result.back() == '"') {
            result = result.substr(1, result.size() - 2);
        }

        return result;
    }

    void generateConstantBlock(const nlohmann::json &constants) {
        if (!constants.empty()) {
            out << getIndent() << "const\n";
            indentLevel++;

            for (auto it = constants.begin(); it != constants.end(); ++it) {
                std::string constantValue = cleanStatement(it.value());
                out << getIndent() << constantValue << ";\n";
            }

            indentLevel--;
            out << "\n";
        }
    }

    void generateVariableBlock(const nlohmann::json &variables) {
        if (!variables.empty()) {
            out << getIndent() << "var\n";
            indentLevel++;

            for (auto it = variables.begin(); it != variables.end(); ++it) {
                std::string variableValue = cleanStatement(it.value());
                out << getIndent() << variableValue << ";\n";
            }

            indentLevel--;
            out << "\n";
        }
    }

    void generateFunctionBlock(const nlohmann::json &functions) {
        for (auto it = functions.begin(); it != functions.end(); ++it) {
            const auto &func = it.value();

            if (func.contains("declaration") && func.contains("body")) {
                std::string declaration = cleanStatement(func["declaration"]);
                out << getIndent() << declaration << ";\n";

                out << getIndent() << "begin\n";
                indentLevel++;

                generateBody(func["body"]);

                indentLevel--;
                out << getIndent() << "end;\n\n";
            }
        }
    }

    void generateMainBlock(const nlohmann::json &mainBlock) {
        // Собираем все statements в правильном порядке
        std::vector<std::pair<int, nlohmann::json>> orderedStatements;

        for (auto it = mainBlock.begin(); it != mainBlock.end(); ++it) {
            std::string key = it.key();
            if (key.find("expr") == 0) {
                int index = std::stoi(key.substr(4)); // "expr" имеет длину 4
                orderedStatements.emplace_back(index, it.value());
            }
        }

        // Сортируем по индексу
        std::sort(orderedStatements.begin(), orderedStatements.end(),
                  [](const auto &a, const auto &b) { return a.first < b.first; });

        // Генерируем в правильном порядке
        for (const auto &[index, statement]: orderedStatements) {
            generateStatement(statement);
        }
    }

    void generateBody(const nlohmann::json &body) {
        if (body.contains("mainBlock")) {
            generateMainBlock(body["mainBlock"]);
        } else if (body.is_object()) {
            // Если это простой объект с выражениями
            for (auto it = body.begin(); it != body.end(); ++it) {
                if (it.key().find("expr") == 0) {
                    generateStatement(it.value());
                }
            }
        }
    }

    void generateStatement(const nlohmann::json &statement) {
        if (!statement.is_object() || !statement.contains("type")) return;

        std::string type = statement["type"];

        if (type == "io" || type == "assign") {
            if (statement.contains("value")) {
                std::string value = cleanStatement(statement["value"]);
                out << getIndent() << value << ";\n";
            }
        } else if (type == "if" || type == "while" || type == "for" || type == "until") {
            generateConditionalStatement(statement);
        } else if (type == "caseOf") {
            generateCaseStatement(statement);
        } else if (type == "else") {
            generateElseStatement(statement);
        }
    }

    void generateConditionalStatement(const nlohmann::json &statement) {
        std::string type = statement["type"];

        if (type == "if") {
            std::string condition = cleanStatement(statement["condition"]);
            // Убираем лишний "if" если он уже есть в условии
            if (condition.find("if ") == 0) {
                condition = condition.substr(3); // Убираем "if "
            }
            if (condition.find(" then") != std::string::npos) {
                condition = condition.substr(0, condition.find(" then"));
            }

            out << getIndent() << "if " << condition << " then\n";
            out << getIndent() << "begin\n";
            indentLevel++;

            if (statement.contains("body")) {
                generateBody(statement["body"]);
            }

            indentLevel--;
            out << getIndent() << "end\n";
        } else if (type == "while") {
            std::string condition = cleanStatement(statement["condition"]);
            // Убираем лишний "while" если он уже есть в условии
            if (condition.find("while ") == 0) {
                condition = condition.substr(6); // Убираем "while "
            }
            if (condition.find(" do") != std::string::npos) {
                condition = condition.substr(0, condition.find(" do"));
            }

            out << getIndent() << "while " << condition << " do\n";
            out << getIndent() << "begin\n";
            indentLevel++;

            if (statement.contains("body")) {
                generateBody(statement["body"]);
            }

            indentLevel--;
            out << getIndent() << "end;\n";
        } else if (type == "for") {
            std::string condition = cleanStatement(statement["condition"]);
            // Убираем лишний "for" если он уже есть в условии
            if (condition.find("for ") == 0) {
                condition = condition.substr(4); // Убираем "for "
            }
            if (condition.find(" do") != std::string::npos) {
                condition = condition.substr(0, condition.find(" do"));
            }

            out << getIndent() << "for " << condition << " do\n";
            out << getIndent() << "begin\n";
            indentLevel++;

            if (statement.contains("body")) {
                generateBody(statement["body"]);
            }

            indentLevel--;
            out << getIndent() << "end;\n";
        } else if (type == "until") {
            out << getIndent() << "repeat\n";
            out << getIndent() << "begin\n";
            indentLevel++;

            if (statement.contains("body")) {
                generateBody(statement["body"]);
            }

            indentLevel--;
            out << getIndent() << "end;\n";

            std::string condition = cleanStatement(statement["condition"]);
            // Убираем лишний "until" если он уже есть в условии
            if (condition.find("until ") == 0) {
                condition = condition.substr(6); // Убираем "until "
            }

            out << getIndent() << "until " << condition << ";\n";
        }
    }

    void generateElseStatement(const nlohmann::json &statement) {
        out << getIndent() << "else\n";
        out << getIndent() << "begin\n";
        indentLevel++;

        if (statement.contains("body")) {
            generateBody(statement["body"]);
        }

        indentLevel--;
        out << getIndent() << "end;\n";  // else всегда заканчивается на end;
    }

    void generateCaseStatement(const nlohmann::json &statement) {
        if (!statement.contains("compareValue") || !statement.contains("body")) return;

        std::string compareValue = cleanStatement(statement["compareValue"]);
        out << getIndent() << "case " << compareValue << " of\n";
        indentLevel++;

        const auto &body = statement["body"];
        for (auto it = body.begin(); it != body.end(); ++it) {
            const auto &branch = it.value();

            if (branch.contains("conditionValues") && branch.contains("todo")) {
                std::string conditionValues = cleanStatement(branch["conditionValues"]);

                // Заменяем пробелы на запятые для правильного формата Pascal
                std::string formattedValues = conditionValues;
                std::replace(formattedValues.begin(), formattedValues.end(), ' ', ',');

                // Обработка else ветки
                if (formattedValues == "else") {
                    out << getIndent() << "else:\n";
                } else {
                    out << getIndent() << formattedValues << ":\n";
                }

                out << getIndent() << "begin\n";
                indentLevel++;

                generateBody(branch["todo"]);

                indentLevel--;
                out << getIndent() << "end";

                // Добавляем точку с запятой для всех веток кроме последней
                if (std::next(it) != body.end()) {
                    out << ";";
                }
                out << "\n";
            }
        }

        indentLevel--;
        out << getIndent() << "end;\n";
    }

public:
    PascalCodeGenerator() = default;

    std::string generatePascal(const nlohmann::json &jsonData) {
        out.str("");
        out.clear();
        indentLevel = 0;

        try {
            if (!jsonData.contains("program")) {
                throw std::runtime_error("Invalid JSON structure: missing 'program'");
            }

            const auto &program = jsonData["program"];

            if (!program.contains("name")) {
                throw std::runtime_error("Invalid JSON structure: missing program name");
            }

            if (!program.contains("sections")) {
                throw std::runtime_error("Invalid JSON structure: missing 'sections'");
            }

            const auto &sections = program["sections"];

            // Заголовок программы
            std::string programName = cleanStatement(program["name"]);
            out << programName << ";\n\n";

            // Генерируем блоки в правильном порядке Pascal
            if (sections.contains("functionBlock")) {
                generateFunctionBlock(sections["functionBlock"]);
            }

            if (sections.contains("constantBlock")) {
                generateConstantBlock(sections["constantBlock"]);
            }

            if (sections.contains("variableBlock")) {
                generateVariableBlock(sections["variableBlock"]);
            }

            // Основной блок программы
            out << "begin\n";
            indentLevel++;

            if (sections.contains("mainBlock")) {
                generateMainBlock(sections["mainBlock"]);
            }

            indentLevel--;
            out << "end.\n";

        } catch (const std::exception &e) {
            std::cerr << "Error generating Pascal code: " << e.what() << std::endl;
            return "Error: " + std::string(e.what());
        }

        return out.str();
    }
};

#endif // PASCAL_CODE_GENERATOR_H
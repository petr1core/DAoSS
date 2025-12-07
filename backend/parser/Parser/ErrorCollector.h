//
// Created by пк on 06.12.2025.
// Сборщик ошибок парсинга
//

#ifndef AISDLAB_ERRORCOLLECTOR_H
#define AISDLAB_ERRORCOLLECTOR_H

#include <string>
#include <vector>
#include <stdexcept>
#include "../Scripts/json.hpp"
#include "../Scripts/Token.h"

using json = nlohmann::json;

struct ParserError {
    int position{0};
    int line{1};
    int column{1};
    std::string type;
    std::string message;
    std::string value;
    std::string expected;
    std::string found;
};

class ErrorCollector {
private:
    std::vector<ParserError> lexerErrors;
    std::vector<ParserError> parserErrors;
    std::string sourceCode;

    // Вычисляет строку и колонку по позиции в исходном коде
    void calculateLineAndColumn(int position, int &line, int &column) {
        line = 1;
        column = 1;
        
        if (position < 0 || position >= static_cast<int>(sourceCode.length())) {
            return;
        }
        
        for (int i = 0; i < position && i < static_cast<int>(sourceCode.length()); ++i) {
            if (sourceCode[i] == '\n') {
                line++;
                column = 1;
            } else {
                column++;
            }
        }
    }

    // Парсит сообщение об ошибке для извлечения дополнительной информации
    void parseErrorMessage(const std::string &errorMsg, std::string &expected, std::string &found) {
        // Попытка извлечь "expected X, found Y" из сообщения
        size_t expectedPos = errorMsg.find("expected");
        size_t foundPos = errorMsg.find("found");
        
        if (expectedPos != std::string::npos) {
            size_t start = expectedPos + 8; // длина "expected "
            size_t end = foundPos != std::string::npos ? foundPos : errorMsg.length();
            expected = errorMsg.substr(start, end - start);
            // Убираем запятые и пробелы
            while (!expected.empty() && (expected.back() == ',' || expected.back() == ' ')) {
                expected.pop_back();
            }
        }
        
        if (foundPos != std::string::npos) {
            size_t start = foundPos + 5; // длина "found "
            found = errorMsg.substr(start);
            // Убираем пробелы в начале
            while (!found.empty() && found.front() == ' ') {
                found.erase(0, 1);
            }
        }
    }

public:
    ErrorCollector(const std::string &code) : sourceCode(code) {}

    void addLexerError(int position, const std::string &message, const std::string &value = "") {
        ParserError error;
        error.position = position;
        error.type = "LexerError";
        error.message = message;
        error.value = value;
        
        calculateLineAndColumn(position, error.line, error.column);
        
        lexerErrors.push_back(error);
    }

    void addParserError(int position, const std::string &message, const std::string &expected = "", const std::string &found = "") {
        ParserError error;
        error.position = position;
        error.type = "ParserError";
        error.message = message;
        error.expected = expected;
        error.found = found;
        
        calculateLineAndColumn(position, error.line, error.column);
        
        parserErrors.push_back(error);
    }

    // Конвертирует исключение в ошибку парсера
    void addException(const std::exception &e, int position = -1, const Token *token = nullptr) {
        ParserError error;
        
        if (token) {
            error.position = token->getPos();
            error.found = token->getValue();
        } else if (position >= 0) {
            error.position = position;
        } else {
            error.position = 0;
        }
        
        error.type = "ParserError";
        error.message = e.what();
        
        parseErrorMessage(e.what(), error.expected, error.found);
        
        calculateLineAndColumn(error.position, error.line, error.column);
        
        parserErrors.push_back(error);
    }

    // Конвертирует исключение с информацией о токене
    void addExceptionWithToken(const std::exception &e, const Token &token, const std::string &expected = "") {
        ParserError error;
        error.position = token.getPos();
        error.type = "ParserError";
        error.message = e.what();
        error.found = token.getValue();
        error.expected = expected;
        
        parseErrorMessage(e.what(), error.expected, error.found);
        
        calculateLineAndColumn(error.position, error.line, error.column);
        
        parserErrors.push_back(error);
    }

    const std::vector<ParserError> &getLexerErrors() const {
        return lexerErrors;
    }

    const std::vector<ParserError> &getParserErrors() const {
        return parserErrors;
    }

    bool hasErrors() const {
        return !lexerErrors.empty() || !parserErrors.empty();
    }

    void clear() {
        lexerErrors.clear();
        parserErrors.clear();
    }

    // Конвертирует ошибки в JSON
    json toJson() const {
        json result;
        
        json lexerErrorsJson = json::array();
        for (const auto &err : lexerErrors) {
            json e;
            e["position"] = err.position;
            e["line"] = err.line;
            e["column"] = err.column;
            e["type"] = err.type;
            e["message"] = err.message;
            if (!err.value.empty()) {
                e["value"] = err.value;
            }
            if (!err.expected.empty()) {
                e["expected"] = err.expected;
            }
            if (!err.found.empty()) {
                e["found"] = err.found;
            }
            lexerErrorsJson.push_back(e);
        }
        result["lexerErrors"] = lexerErrorsJson;
        
        json parserErrorsJson = json::array();
        for (const auto &err : parserErrors) {
            json e;
            e["position"] = err.position;
            e["line"] = err.line;
            e["column"] = err.column;
            e["type"] = err.type;
            e["message"] = err.message;
            if (!err.value.empty()) {
                e["value"] = err.value;
            }
            if (!err.expected.empty()) {
                e["expected"] = err.expected;
            }
            if (!err.found.empty()) {
                e["found"] = err.found;
            }
            parserErrorsJson.push_back(e);
        }
        result["parserErrors"] = parserErrorsJson;
        
        return result;
    }
};

#endif //AISDLAB_ERRORCOLLECTOR_H






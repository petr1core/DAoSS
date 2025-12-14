#include <iostream>
#include <string>
#include <cstdlib>
#include "thirdparty/httplib.h"
#include "Scripts/json.hpp"
#include "Scripts/Lexer.h"
#include "Parser/PascalParserToExpression.h"
#include "Parser/CParserToAST.h"
#include "Parser/CppParserToAST.h"
#include "Parser/ASTParserToJSON.h"
#include "Parser/CppASTParserToJSON.h"
#include "Parser/ErrorCollector.h"
#include "Flowchart/ExporterJson.h"
#include "Auth/JwtValidator.h"

using json = nlohmann::json;

// Конфигурация сервера (можно вынести в переменные окружения или конфиг файл)
struct ServerConfig {
    std::string jwtSecret = "DaehLytkwbnnjfypS1YZADihusW4JmzzQb9Hxv0F";
    std::string jwtIssuer = "daoss-dev";
    std::string jwtAudience = "daoss-client";
    int port = 8080;
};

// Проверка авторизации
bool checkAuthorization(const httplib::Request& req, const JwtValidator& validator) {
    auto authHeader = req.get_header_value("Authorization");
    
    if (authHeader.empty()) {
        return false;
    }
    
    std::string token = JwtValidator::extractTokenFromHeader(authHeader);
    
    if (token.empty()) {
        return false;
    }
    
    bool isValid = validator.validate(token);
    
    if (!isValid) {
        std::cerr << "[AUTH] Token validation failed" << std::endl;
    }
    
    return isValid;
}

// Парсинг Pascal кода
json parsePascal(const std::string& code, ErrorCollector& errorCollector) {
    try {
        Lexer lexer(code, LangType::LANG_PASCAL);
        auto tokens = lexer.getTokenList();
        
        PascalParserToExpression parser(lexer, LangType::LANG_PASCAL);
        parser.parseOnly();
        
        const auto& exprsWithChapters = parser.getExpressions();
        
        PascalToJSON exporter;
        std::string title = parser.getTitle();
        
        std::string jsonStr;
        try {
            jsonStr = exporter.build(exprsWithChapters, title);
        } catch (const std::exception& e) {
            std::cerr << "[ERROR] Exception in exporter.build(): " << e.what() << std::endl;
            errorCollector.addException(e);
            return json();
        }
        
        json result;
        try {
            result = json::parse(jsonStr);
        } catch (const json::parse_error& e) {
            std::cerr << "[ERROR] JSON parse error: " << e.what() << std::endl;
            errorCollector.addException(e);
            return json();
        }
        
        return result;
    } catch (const std::exception& e) {
        std::cerr << "[ERROR] Exception in parsePascal: " << e.what() << std::endl;
        errorCollector.addException(e);
        return json();
    }
}

// Парсинг C кода
json parseC(const std::string& code, ErrorCollector& errorCollector) {
    try {
        CParserToAST parser;
        auto ast = parser.parse(code);
        
        AstToJsonConverter converter;
        return converter.convertProgram(ast);
    } catch (const std::exception& e) {
        errorCollector.addException(e);
        return json();
    }
}

// Парсинг C++ кода
json parseCpp(const std::string& code, ErrorCollector& errorCollector) {
    try {
        CppParserToAST parser;
        auto ast = parser.parse(code);
        
        CppAstToJsonConverter converter;
        return converter.convertProgram(ast);
    } catch (const std::exception& e) {
        errorCollector.addException(e);
        return json();
    }
}

// Обработка запроса на парсинг
json handleParseRequest(const json& request, ErrorCollector& errorCollector) {
    std::string code = request.value("code", "");
    std::string language = request.value("language", "");
    
    if (code.empty() || language.empty()) {
        json response;
        response["success"] = false;
        response["error"] = "Code and language are required";
        return response;
    }
    
    json representation;
    std::string representationType;
    
    if (language == "pascal") {
        representation = parsePascal(code, errorCollector);
        representationType = "SPR";
    } else if (language == "c") {
        representation = parseC(code, errorCollector);
        representationType = "AST";
    } else if (language == "cpp") {
        representation = parseCpp(code, errorCollector);
        representationType = "AST";
    } else {
        json response;
        response["success"] = false;
        response["error"] = "Unsupported language: " + language;
        return response;
    }
    
    json response;
    response["success"] = !errorCollector.hasErrors();
    
        if (errorCollector.hasErrors()) {
        auto errorsJson = errorCollector.toJson();
        response["lexerErrors"] = errorsJson["lexerErrors"];
        response["parserErrors"] = errorsJson["parserErrors"];
        response["error"] = "Parsing failed";
    } else {
        response["representation"] = representation;
        response["representationType"] = representationType;
        response["error"] = nullptr;
    }
    
    return response;
}

// Обработка запроса на валидацию (детальная)
json handleValidateRequest(const json& request, ErrorCollector& errorCollector) {
    std::string code = request.value("code", "");
    std::string language = request.value("language", "");
    
    if (code.empty() || language.empty()) {
        json response;
        response["valid"] = false;
        return response;
    }
    
    // Пытаемся распарсить код
    if (language == "pascal") {
        parsePascal(code, errorCollector);
    } else if (language == "c") {
        parseC(code, errorCollector);
    } else if (language == "cpp") {
        parseCpp(code, errorCollector);
    }
    
    json response;
    response["valid"] = !errorCollector.hasErrors();
    
    auto errorsJson = errorCollector.toJson();
    response["lexerErrors"] = errorsJson["lexerErrors"];
    response["parserErrors"] = errorsJson["parserErrors"];
    
    return response;
}

// Обработка запроса на упрощенную валидацию
json handleSimpleValidateRequest(const json& request, ErrorCollector& errorCollector) {
    std::string code = request.value("code", "");
    std::string language = request.value("language", "");
    
    if (code.empty() || language.empty()) {
        json response;
        response["valid"] = false;
        response["hasErrors"] = true;
        response["lexerErrorsCount"] = 0;
        response["parserErrorsCount"] = 0;
        return response;
    }
    
    // Пытаемся распарсить код
    if (language == "pascal") {
        parsePascal(code, errorCollector);
    } else if (language == "c") {
        parseC(code, errorCollector);
    } else if (language == "cpp") {
        parseCpp(code, errorCollector);
    }
    
    json response;
    bool hasErrors = errorCollector.hasErrors();
    response["valid"] = !hasErrors;
    response["hasErrors"] = hasErrors;
    response["lexerErrorsCount"] = static_cast<int>(errorCollector.getLexerErrors().size());
    response["parserErrorsCount"] = static_cast<int>(errorCollector.getParserErrors().size());
    
    return response;
}

int main(int argc, char* argv[]) {
    ServerConfig config;
    
    // Парсинг аргументов командной строки (порт)
    if (argc > 1) {
        config.port = std::stoi(argv[1]);
    }
    
    // Инициализация JWT валидатора
    JwtValidator jwtValidator(config.jwtSecret, config.jwtIssuer, config.jwtAudience);
    
    httplib::Server svr;
    
    // CORS заголовки
    svr.set_default_headers({
        {"Access-Control-Allow-Origin", "*"},
        {"Access-Control-Allow-Methods", "POST, OPTIONS"},
        {"Access-Control-Allow-Headers", "Content-Type, Authorization"}
    });
    
    // Обработка OPTIONS запросов для CORS
    svr.Options(".*", [](const httplib::Request&, httplib::Response& res) {
        res.set_header("Access-Control-Allow-Origin", "*");
        res.set_header("Access-Control-Allow-Methods", "POST, OPTIONS");
        res.set_header("Access-Control-Allow-Headers", "Content-Type, Authorization");
    });
    
    // Эндпоинт /api/parse
    svr.Post("/api/parse", [&jwtValidator](const httplib::Request& req, httplib::Response& res) {
        // Проверка авторизации
        if (!checkAuthorization(req, jwtValidator)) {
            res.status = 401;
            res.set_content("{\"error\":\"Unauthorized\"}", "application/json");
            return;
        }
        
        try {
            json request = json::parse(req.body);
            ErrorCollector errorCollector(request.value("code", ""));
            
            json response = handleParseRequest(request, errorCollector);
            
            res.status = 200;
            res.set_content(response.dump(2), "application/json");
        } catch (const json::parse_error& e) {
            res.status = 400;
            json error;
            error["error"] = "Invalid JSON: " + std::string(e.what());
            res.set_content(error.dump(), "application/json");
        } catch (const std::exception& e) {
            res.status = 500;
            json error;
            error["error"] = std::string(e.what());
            res.set_content(error.dump(), "application/json");
        }
    });
    
    // Эндпоинт /api/validate
    svr.Post("/api/validate", [&jwtValidator](const httplib::Request& req, httplib::Response& res) {
        // Проверка авторизации
        if (!checkAuthorization(req, jwtValidator)) {
            res.status = 401;
            res.set_content("{\"error\":\"Unauthorized\"}", "application/json");
            return;
        }
        
        try {
            json request = json::parse(req.body);
            ErrorCollector errorCollector(request.value("code", ""));
            
            json response = handleValidateRequest(request, errorCollector);
            
            res.status = 200;
            res.set_content(response.dump(2), "application/json");
        } catch (const json::parse_error& e) {
            std::cerr << "[ERROR] JSON parse error: " << e.what() << std::endl;
            res.status = 400;
            json error;
            error["error"] = "Invalid JSON: " + std::string(e.what());
            res.set_content(error.dump(), "application/json");
        } catch (const std::exception& e) {
            std::cerr << "[ERROR] Exception: " << e.what() << std::endl;
            res.status = 500;
            json error;
            error["error"] = std::string(e.what());
            res.set_content(error.dump(), "application/json");
        }
    });
    
    // Эндпоинт /api/validate/simple
    svr.Post("/api/validate/simple", [&jwtValidator](const httplib::Request& req, httplib::Response& res) {
        // Проверка авторизации
        if (!checkAuthorization(req, jwtValidator)) {
            res.status = 401;
            res.set_content("{\"error\":\"Unauthorized\"}", "application/json");
            return;
        }
        
        try {
            json request = json::parse(req.body);
            ErrorCollector errorCollector(request.value("code", ""));
            
            json response = handleSimpleValidateRequest(request, errorCollector);
            
            res.status = 200;
            res.set_content(response.dump(2), "application/json");
        } catch (const json::parse_error& e) {
            res.status = 400;
            json error;
            error["error"] = "Invalid JSON: " + std::string(e.what());
            res.set_content(error.dump(), "application/json");
        } catch (const std::exception& e) {
            res.status = 500;
            json error;
            error["error"] = std::string(e.what());
            res.set_content(error.dump(), "application/json");
        }
    });
    
    // Health check эндпоинт
    svr.Get("/health", [](const httplib::Request&, httplib::Response& res) {
        res.status = 200;
        res.set_content("{\"status\":\"ok\"}", "application/json");
    });
    
    std::cout << "Parser server starting on port " << config.port << std::endl;
    
    if (!svr.listen("0.0.0.0", config.port)) {
        std::cerr << "Failed to start server on port " << config.port << std::endl;
        return 1;
    }
    
    return 0;
}

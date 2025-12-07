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
    std::cout << "[AUTH] Checking authorization..." << std::endl;
    
    auto authHeader = req.get_header_value("Authorization");
    std::cout << "[AUTH] Authorization header: " << (authHeader.empty() ? "(empty)" : authHeader.substr(0, 50) + "...") << std::endl;
    
    if (authHeader.empty()) {
        std::cout << "[AUTH] Authorization header is empty" << std::endl;
        return false;
    }
    
    std::string token = JwtValidator::extractTokenFromHeader(authHeader);
    std::cout << "[AUTH] Extracted token: " << (token.empty() ? "(empty)" : token.substr(0, 50) + "...") << std::endl;
    
    if (token.empty()) {
        std::cout << "[AUTH] Token is empty after extraction" << std::endl;
        return false;
    }
    
    bool isValid = validator.validate(token);
    std::cout << "[AUTH] Token validation result: " << (isValid ? "VALID" : "INVALID") << std::endl;
    
    return isValid;
}

// Парсинг Pascal кода
json parsePascal(const std::string& code, ErrorCollector& errorCollector) {
    std::cout << "[DEBUG] parsePascal started" << std::endl;
    std::cout << "[DEBUG] Code length: " << code.length() << std::endl;
    std::cout << "[DEBUG] Code: " << code << std::endl;
    
    try {
        std::cout << "[DEBUG] Creating Lexer..." << std::endl;
        Lexer lexer(code, LangType::LANG_PASCAL);
        auto tokens = lexer.getTokenList();

        std::cout << "[DEBUG] First 20 tokens:" << std::endl;
        for (size_t i = 0; i < std::min(size_t(20), tokens.size()); i++) {
            std::cout << "  [" << i << "] Type: " << tokens[i].getType() 
                      << ", Value: " << tokens[i].getValue() << std::endl;
        }

        std::cout << "[DEBUG] Lexer created, creating parser..." << std::endl;
        
        PascalParserToExpression parser(lexer, LangType::LANG_PASCAL);
        std::cout << "[DEBUG] Parser created, parsing..." << std::endl;
        
        parser.parseOnly();
        std::cout << "[DEBUG] Parsing completed" << std::endl;
        
        const auto& exprsWithChapters = parser.getExpressions();
        std::cout << "[DEBUG] Got expressions: " << exprsWithChapters.size() << std::endl;
        if (exprsWithChapters.size() > 0) {
            std::cout << "[DEBUG] First expression chapter: " << exprsWithChapters[0].second << std::endl;
        }
        
        PascalToJSON exporter;
        std::string title = parser.getTitle();
        std::cout << "[DEBUG] Title: " << title << std::endl;
        
        std::string jsonStr;
        try {
            jsonStr = exporter.build(exprsWithChapters, title);
        } catch (const std::exception& e) {
            std::cout << "[DEBUG] Exception in exporter.build(): " << e.what() << std::endl;
            errorCollector.addException(e);
            return json();
        }
        std::cout << "[DEBUG] JSON string built, length: " << jsonStr.length() << std::endl;
        if (jsonStr.length() > 0) {
            std::cout << "[DEBUG] JSON string first 200 chars: " << jsonStr.substr(0, 200) << std::endl;
        }
        
        json result;
        try {
            result = json::parse(jsonStr);
        } catch (const json::parse_error& e) {
            std::cout << "[DEBUG] JSON parse error: " << e.what() << std::endl;
            errorCollector.addException(e);
            return json();
        }
        
        std::cout << "[DEBUG] Parsed JSON result is_null: " << result.is_null() << std::endl;
        std::cout << "[DEBUG] Parsed JSON result type: " << (result.is_object() ? "object" : "other") << std::endl;
        std::cout << "[DEBUG] Parsed JSON result contains 'program': " << result.contains("program") << std::endl;
        if (result.contains("program")) {
            std::cout << "[DEBUG] Program name in result: " << result["program"].value("name", "NOT_FOUND") << std::endl;
        }
        std::cout << "[DEBUG] parsePascal completed successfully" << std::endl;
        return result;
    } catch (const std::exception& e) {
        std::cout << "[DEBUG] Exception in parsePascal: " << e.what() << std::endl;
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
        std::cout << "[DEBUG] Setting representation, is_null: " << representation.is_null() << std::endl;
        std::cout << "[DEBUG] RepresentationType: " << representationType << std::endl;
        if (!representation.is_null()) {
            std::cout << "[DEBUG] Representation dump (first 200 chars): " << representation.dump(2).substr(0, 200) << std::endl;
        }
        response["representation"] = representation;
        response["representationType"] = representationType;
        response["error"] = nullptr;
    }
    
    return response;
}

// Обработка запроса на валидацию (детальная)
json handleValidateRequest(const json& request, ErrorCollector& errorCollector) {
    std::cout << "\n=== [DEBUG] handleValidateRequest ===" << std::endl;
    std::cout << "[DEBUG] Request: " << request.dump(2) << std::endl;
    
    std::string code = request.value("code", "");
    std::string language = request.value("language", "");
    
    std::cout << "[DEBUG] Code: '" << code << "'" << std::endl;
    std::cout << "[DEBUG] Language: '" << language << "'" << std::endl;
    
    if (code.empty() || language.empty()) {
        std::cout << "[DEBUG] Code or language is empty!" << std::endl;
        json response;
        response["valid"] = false;
        return response;
    }
    
    // Пытаемся распарсить код
    std::cout << "[DEBUG] Starting parsing for language: " << language << std::endl;
    if (language == "pascal") {
        parsePascal(code, errorCollector);
    } else if (language == "c") {
        parseC(code, errorCollector);
    } else if (language == "cpp") {
        parseCpp(code, errorCollector);
    }
    
    std::cout << "[DEBUG] Parsing finished" << std::endl;
    std::cout << "[DEBUG] Has errors: " << errorCollector.hasErrors() << std::endl;
    
    json response;
    response["valid"] = !errorCollector.hasErrors();
    
    auto errorsJson = errorCollector.toJson();
    std::cout << "[DEBUG] Errors JSON: " << errorsJson.dump(2) << std::endl;
    
    response["lexerErrors"] = errorsJson["lexerErrors"];
    response["parserErrors"] = errorsJson["parserErrors"];
    
    std::cout << "[DEBUG] Response: " << response.dump(2) << std::endl;
    std::cout << "=== [DEBUG] handleValidateRequest END ===\n" << std::endl;
    
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
        std::cout << "\n>>> [ENDPOINT] /api/validate called <<<" << std::endl;
        std::cout << "[DEBUG] Request body: " << req.body << std::endl;
        
        // Проверка авторизации
        std::cout << "[DEBUG] Checking authorization..." << std::endl;
        if (!checkAuthorization(req, jwtValidator)) {
            std::cout << "[DEBUG] Authorization FAILED" << std::endl;
            res.status = 401;
            res.set_content("{\"error\":\"Unauthorized\"}", "application/json");
            return;
        }
        std::cout << "[DEBUG] Authorization OK" << std::endl;
        
        try {
            std::cout << "[DEBUG] Parsing JSON request..." << std::endl;
            json request = json::parse(req.body);
            std::cout << "[DEBUG] JSON parsed successfully" << std::endl;
            
            ErrorCollector errorCollector(request.value("code", ""));
            
            json response = handleValidateRequest(request, errorCollector);
            
            std::cout << "[DEBUG] Sending response with status 200" << std::endl;
            res.status = 200;
            res.set_content(response.dump(2), "application/json");
        } catch (const json::parse_error& e) {
            std::cout << "[ERROR] JSON parse error: " << e.what() << std::endl;
            res.status = 400;
            json error;
            error["error"] = "Invalid JSON: " + std::string(e.what());
            res.set_content(error.dump(), "application/json");
        } catch (const std::exception& e) {
            std::cout << "[ERROR] Exception: " << e.what() << std::endl;
            res.status = 500;
            json error;
            error["error"] = std::string(e.what());
            res.set_content(error.dump(), "application/json");
        }
        std::cout << "<<< [ENDPOINT] /api/validate finished >>>\n" << std::endl;
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
    std::cout << "Endpoints:" << std::endl;
    std::cout << "  POST /api/parse" << std::endl;
    std::cout << "  POST /api/validate" << std::endl;
    std::cout << "  POST /api/validate/simple" << std::endl;
    std::cout << "  GET /health" << std::endl;
    
    if (!svr.listen("0.0.0.0", config.port)) {
        std::cerr << "Failed to start server on port " << config.port << std::endl;
        return 1;
    }
    
    return 0;
}

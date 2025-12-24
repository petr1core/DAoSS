#include <iostream>
#include <string>
#include <cstdlib>
#include <fstream>
#include <filesystem>
#include <cerrno>
#include <cstring>
#include "thirdparty/httplib.h"
#include "Scripts/json.hpp"
#include "Scripts/Lexer.h"
#include "Parser/PascalParserToExpression.h"
#include "Parser/CParserToAST.h"
#include "Parser/CppParserToAST.h"
#include "Parser/ASTParserToJSON.h"
#include "Parser/CppASTParserToJSON.h"
#include "Parser/ErrorCollector.h"
#include "Ast/ExprAcceptImpl.h"
#include "Flowchart/ExporterJson.h"
#include "Auth/JwtValidator.h"
#include "CodeGen/PascalCodeGenerator.h"
#include "CodeGen/CCodeGenerator.h"

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
    try {
        std::cout << "[HYPOTHESIS 3] parsePascal: Starting lexer, code length: " << code.length() << std::endl;
        Lexer lexer(code, LangType::LANG_PASCAL);
        std::cout << "[HYPOTHESIS 3] parsePascal: Lexer completed, getting token list" << std::endl;
        auto tokens = lexer.getTokenList();
        std::cout << "[HYPOTHESIS 3] parsePascal: Got " << tokens.size() << " tokens, starting parser" << std::endl;
        
        if (tokens.empty()) {
            std::cerr << "[ERROR] parsePascal: Token list is empty!" << std::endl;
        } else {
            std::cout << "[HYPOTHESIS 3] parsePascal: First token: " << tokens[0].getType() 
                      << " = \"" << tokens[0].getValue() << "\"" << std::endl;
            std::cout << "[HYPOTHESIS 3] parsePascal: Last token: " << tokens[tokens.size()-1].getType() 
                      << " = \"" << tokens[tokens.size()-1].getValue() << "\"" << std::endl;
        }

        PascalParserToExpression parser(lexer, LangType::LANG_PASCAL);
        parser.parseOnly();
        
        const auto& exprsWithChapters = parser.getExpressionsOnly();
        
        PascalToJSON exporter;
        std::string title = parser.getTitle();
        
        std::string jsonStr = exporter.build(exprsWithChapters, title);
        
        json result = json::parse(jsonStr);
        return result;
    } catch (const std::bad_alloc& e) {
        std::cerr << "[ERROR] bad_alloc in parsePascal: " << e.what() << std::endl;
        errorCollector.addException(e);
        return json();
    } catch (const std::exception& e) {
        std::cerr << "[ERROR] Exception in parsePascal: " << e.what() << std::endl;
        errorCollector.addException(e);
        return json();
    } catch (...) {
        std::cerr << "[ERROR] Unknown exception in parsePascal" << std::endl;
        errorCollector.addParserError(0, "Unknown exception occurred during parsing");
        return json();
    }
}

// Парсинг C кода
json parseC(const std::string& code, ErrorCollector& errorCollector) {
    try {
        std::cout << "[parseC] Starting C parser, code length: " << code.length() << std::endl;
        CParserToAST parser;
        auto ast = parser.parse(code);
        
        if (!ast) {
            std::cerr << "[ERROR] [parseC] AST is null!" << std::endl;
            errorCollector.addParserError(0, "Failed to create AST");
            return json();
        }
        
        std::cout << "[parseC] AST created successfully, program name: " << ast->name << std::endl;
        if (ast->body) {
            std::cout << "[parseC] Body has " << ast->body->statements.size() << " statements" << std::endl;
        } else {
            std::cerr << "[WARNING] [parseC] Body is null!" << std::endl;
        }
        
        AstToJsonConverter converter;
        auto result = converter.convertProgram(ast);
        std::cout << "[parseC] JSON conversion completed" << std::endl;
        return result;
    } catch (const std::bad_alloc& e) {
        std::cerr << "[ERROR] [parseC] bad_alloc: " << e.what() << std::endl;
        errorCollector.addException(e);
        return json();
    } catch (const std::exception& e) {
        std::cerr << "[ERROR] [parseC] Exception: " << e.what() << std::endl;
        errorCollector.addException(e);
        return json();
    } catch (...) {
        std::cerr << "[ERROR] [parseC] Unknown exception" << std::endl;
        errorCollector.addParserError(0, "Unknown exception in C parser");
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
    
    try {
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
    } catch (const std::bad_alloc& e) {
        std::cerr << "[ERROR] bad_alloc in handleParseRequest: " << e.what() << std::endl;
        errorCollector.addException(e);
        representation = json();
    } catch (const std::exception& e) {
        std::cerr << "[ERROR] Exception in handleParseRequest: " << e.what() << std::endl;
        errorCollector.addException(e);
        representation = json();
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

// Глобальный файл для логов
static std::ofstream logFile;
static bool loggingInitialized = false;

// Простая функция для записи в файл и консоль одновременно
void writeToLog(const std::string& message) {
    // Всегда пишем в консоль
    std::cout << message;
    
    // Если файл открыт, пишем и в файл
    if (logFile.is_open()) {
        logFile << message;
        logFile.flush(); // Принудительно записываем в файл
    }
}

// Переопределяем оператор << для удобства (простой вариант через макрос)
// Но лучше использовать функцию выше

// Функция для инициализации логирования в файл
void initLogging() {
    if (loggingInitialized) {
        return; // Уже инициализировано
    }
    
    try {
        std::string logPath = "D:/DevLeb/DAoSS/examples/parser_debug.log";
        
        // Создаем директорию, если её нет
        std::filesystem::path dirPath = std::filesystem::path(logPath).parent_path();
        if (!std::filesystem::exists(dirPath)) {
            bool created = std::filesystem::create_directories(dirPath);
            std::cout << "[LOG INIT] Directory creation " << (created ? "succeeded" : "failed") 
                      << ": " << dirPath.string() << std::endl;
        }
        
        // Открываем файл для записи (truncate mode - перезаписываем файл)
        logFile.open(logPath, std::ios::out | std::ios::trunc);
        
        if (logFile.is_open()) {
            loggingInitialized = true;
            
            // Записываем заголовок в файл
            logFile << "\n========== LOGGING STARTED ==========\n";
            logFile << "Log file: " << logPath << "\n";
            logFile << "======================================\n\n";
            logFile.flush();
            
            std::cout << "[LOG INIT] Logging initialized successfully!" << std::endl;
            std::cout << "[LOG INIT] Log file: " << logPath << std::endl;
        } else {
            std::cerr << "[LOG INIT] ERROR: Could not open log file: " << logPath << std::endl;
            std::cerr << "[LOG INIT] Error code: " << errno << " - " << strerror(errno) << std::endl;
        }
    } catch (const std::exception& e) {
        std::cerr << "[LOG INIT] EXCEPTION: Could not initialize logging: " << e.what() << std::endl;
    }
}

// Функция для записи в лог-файл (вызывается из всех мест, где нужно логировать)
void logToFile(const std::string& message) {
    if (logFile.is_open()) {
        logFile << message;
        logFile.flush();
    }
}

// Кастомный streambuf для перенаправления std::cout и std::cerr
class DualStreamBuf : public std::streambuf {
private:
    std::streambuf* consoleBuf;
    std::ofstream* fileBuf;
    
public:
    DualStreamBuf(std::streambuf* console, std::ofstream* file) 
        : consoleBuf(console), fileBuf(file) {}
    
protected:
    virtual int overflow(int c) override {
        if (c != EOF) {
            // Пишем в консоль
            if (consoleBuf) {
                consoleBuf->sputc(c);
            }
            // Пишем в файл
            if (fileBuf && fileBuf->is_open()) {
                fileBuf->put(static_cast<char>(c));
            }
        }
        return c;
    }
    
    virtual std::streamsize xsputn(const char* s, std::streamsize count) override {
        // Пишем в консоль
        if (consoleBuf) {
            consoleBuf->sputn(s, count);
        }
        // Пишем в файл
        if (fileBuf && fileBuf->is_open()) {
            fileBuf->write(s, count);
            fileBuf->flush();
        }
        return count;
    }
    
    virtual int sync() override {
        if (consoleBuf) {
            consoleBuf->pubsync();
        }
        if (fileBuf && fileBuf->is_open()) {
            fileBuf->flush();
        }
        return 0;
    }
};

static DualStreamBuf* coutBuf = nullptr;
static DualStreamBuf* cerrBuf = nullptr;
static std::streambuf* originalCoutBuf = nullptr;
static std::streambuf* originalCerrBuf = nullptr;

// Функция для активации перенаправления потоков
void activateStreamRedirection() {
    if (!loggingInitialized || !logFile.is_open()) {
        return;
    }
    
    try {
        // Сохраняем оригинальные буферы
        originalCoutBuf = std::cout.rdbuf();
        originalCerrBuf = std::cerr.rdbuf();
        
        // Создаем новые буферы
        coutBuf = new DualStreamBuf(originalCoutBuf, &logFile);
        cerrBuf = new DualStreamBuf(originalCerrBuf, &logFile);
        
        // Устанавливаем новые буферы
        std::cout.rdbuf(coutBuf);
        std::cerr.rdbuf(cerrBuf);
        
        std::cout << "[LOG] Stream redirection activated!" << std::endl;
    } catch (const std::exception& e) {
        std::cerr << "[LOG] Failed to activate stream redirection: " << e.what() << std::endl;
    }
}

int main(int argc, char* argv[]) {
    ServerConfig config;
    
    // Инициализируем логирование в файл (ДО всех других операций)
    initLogging();
    
    // Активируем перенаправление потоков после инициализации
    activateStreamRedirection();
    
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
            std::string code = request.value("code", "");
            std::string language = request.value("language", "");
            
            // ГИПОТЕЗА 1: Логируем как получаем код из запроса
            std::cout << "[HYPOTHESIS 1] Code received from JSON, length: " << code.length() << " bytes" << std::endl;
            std::cout << "[HYPOTHESIS 1] First 100 chars: \"" << code.substr(0, std::min(100, (int)code.length())) << "...\"" << std::endl;
            std::cout << "[HYPOTHESIS 1] Last 50 chars: \"..." << (code.length() > 50 ? code.substr(code.length() - 50) : code) << "\"" << std::endl;
            std::cout << "[HYPOTHESIS 1] Has newlines: " << (code.find('\n') != std::string::npos ? "YES" : "NO") << std::endl;
            std::cout << "[HYPOTHESIS 1] Newline count: " << std::count(code.begin(), code.end(), '\n') << std::endl;
            std::cout << "[HYPOTHESIS 1] First char: '" << (code.empty() ? ' ' : code[0]) << "' (0x" 
                      << std::hex << (code.empty() ? 0 : static_cast<unsigned char>(code[0])) << std::dec << ")" << std::endl;
            
            // Если код выглядит как JSON-строка (начинается и заканчивается кавычками), попробуем декодировать
            if (code.length() >= 2 && code[0] == '"' && code[code.length() - 1] == '"') {
                std::cout << "[HYPOTHESIS 1] Code appears to be a JSON string literal (starts and ends with quotes)" << std::endl;
                std::cout << "[HYPOTHESIS 1] Code appears to be a JSON string literal, attempting to parse..." << std::endl;
                try {
                    json codeJson = json::parse(code);
                    if (codeJson.is_string()) {
                        std::string decodedCode = codeJson.get<std::string>();
                        std::cout << "[HYPOTHESIS 1] Successfully decoded JSON string, old length: " << code.length() 
                                  << ", new length: " << decodedCode.length() << " bytes" << std::endl;
                        std::cout << "[HYPOTHESIS 1] First 100 chars after decode: \"" 
                                  << decodedCode.substr(0, std::min(100, (int)decodedCode.length())) << "...\"" << std::endl;
                        code = decodedCode;
                    } else {
                        std::cout << "[HYPOTHESIS 1] Parsed JSON is not a string (type: " 
                                  << codeJson.type_name() << "), leaving as is" << std::endl;
                    }
                } catch (const std::exception& e) {
                    std::cerr << "[WARNING] [HYPOTHESIS 1] Failed to parse code as JSON string: " << e.what() << std::endl;
                    std::cerr << "[WARNING] [HYPOTHESIS 1] Leaving code as is" << std::endl;
                }
            }
            
            // Также проверим, может быть код - это массив строк (конкатенируем их)
            if (!code.empty() && code[0] == '[') {
                std::cout << "[HYPOTHESIS 1] Code appears to be a JSON array (starts with '['), attempting to parse..." << std::endl;
                try {
                    json codeArray = json::parse(code);
                    if (codeArray.is_array()) {
                        std::string concatenatedCode;
                        for (const auto& item : codeArray) {
                            if (item.is_string()) {
                                concatenatedCode += item.get<std::string>() + "\n";
                            }
                        }
                        if (!concatenatedCode.empty()) {
                            std::cout << "[HYPOTHESIS 1] Successfully concatenated array, new length: " 
                                      << concatenatedCode.length() << " bytes" << std::endl;
                            code = concatenatedCode;
                        }
                    }
                } catch (const std::exception& e) {
                    std::cerr << "[WARNING] [HYPOTHESIS 1] Failed to parse code as JSON array: " << e.what() << std::endl;
                }
            }
            
            // Финальная проверка кода после всех обработок
            std::cout << "[HYPOTHESIS 1] Final code after processing, length: " << code.length() << " bytes" << std::endl;
            std::cout << "[HYPOTHESIS 1] Final code first 100 chars: \"" << code.substr(0, std::min(100, (int)code.length())) << "...\"" << std::endl;
            if (code.length() > 0) {
            std::cout << "[HYPOTHESIS 1] Final code first char: '" << code[0] << "' (0x" 
                      << std::hex << static_cast<unsigned char>(code[0]) << std::dec << ")" << std::endl;
            }
            
            std::cout << "[HYPOTHESIS 1] Language received: \"" << language << "\"" << std::endl;
            
            ErrorCollector errorCollector(code);
            
            // ГИПОТЕЗА 3: Отмечаем где происходит bad_alloc
            std::cout << "[HYPOTHESIS 3] Starting parsing, language: " << language << std::endl;
            json response = handleParseRequest(request, errorCollector);
            
            // Сохраняем финальный JSON в файл в зависимости от языка
            try {
                std::string jsonStr = response.dump(2);
                
                // Выбираем файл в зависимости от языка
                std::string filePath;
                if (language == "pascal") {
                    filePath = "D:/DevLeb/DAoSS/examples/flowchart.json";
                } else if (language == "c" || language == "cpp") {
                    filePath = "D:/DevLeb/DAoSS/examples/flowchart2.json";
                } else {
                    // По умолчанию flowchart.json
                    filePath = "D:/DevLeb/DAoSS/examples/flowchart.json";
                }
                
                std::cout << "[DEBUG] Attempting to save JSON to: " << filePath << " (language: " << language << ")" << std::endl;
                std::cout << "[DEBUG] JSON string length: " << jsonStr.length() << " bytes" << std::endl;
                
                // Создаем директорию, если её нет
                try {
                    std::filesystem::path dirPath = std::filesystem::path(filePath).parent_path();
                    if (!std::filesystem::exists(dirPath)) {
                        bool created = std::filesystem::create_directories(dirPath);
                        std::cout << "[DEBUG] Directory creation " << (created ? "succeeded" : "failed") << ": " << dirPath.string() << std::endl;
                    } else {
                        std::cout << "[DEBUG] Directory already exists: " << dirPath.string() << std::endl;
                    }
                } catch (const std::filesystem::filesystem_error& fsErr) {
                    std::cerr << "[WARNING] Filesystem error while creating directory: " << fsErr.what() << std::endl;
                }
                
                std::ofstream outFile(filePath, std::ios::out | std::ios::trunc);
                if (outFile.is_open()) {
                    outFile << jsonStr;
                    outFile.close();
                    std::cout << "[DEBUG] JSON saved successfully to " << filePath << " (" << jsonStr.length() << " bytes)" << std::endl;
                    
                    // Проверяем, что файл действительно создан
                    if (std::filesystem::exists(filePath)) {
                        auto fileSize = std::filesystem::file_size(filePath);
                        std::cout << "[DEBUG] File verified: exists, size = " << fileSize << " bytes" << std::endl;
                    } else {
                        std::cerr << "[WARNING] File was not created despite successful write!" << std::endl;
                    }
                } else {
                    std::cerr << "[WARNING] Failed to open file for writing: " << filePath << std::endl;
                    std::cerr << "[WARNING] Error details: " << strerror(errno) << std::endl;
                }
            } catch (const std::filesystem::filesystem_error& e) {
                std::cerr << "[WARNING] Filesystem error while saving JSON: " << e.what() << std::endl;
            } catch (const std::exception& e) {
                std::cerr << "[WARNING] Failed to save JSON to file: " << e.what() << std::endl;
            }
            
            res.status = 200;
            res.set_content(response.dump(2), "application/json");
        } catch (const std::bad_alloc& e) {
            // Специальная обработка ошибок памяти
            std::cerr << "[ERROR] bad_alloc caught: " << e.what() << std::endl;
            res.status = 500;
            json error;
            error["success"] = false;
            error["error"] = "Memory allocation failed (bad_alloc). Code may be too complex.";
            error["details"] = std::string(e.what());
            res.set_content(error.dump(), "application/json");
        } catch (const json::parse_error& e) {
            res.status = 400;
            json error;
            error["error"] = "Invalid JSON: " + std::string(e.what());
            res.set_content(error.dump(), "application/json");
        } catch (const std::exception& e) {
            std::cerr << "[ERROR] Exception caught: " << e.what() << std::endl;
            res.status = 500;
            json error;
            error["success"] = false;
            error["error"] = std::string(e.what());
            res.set_content(error.dump(), "application/json");
        } catch (...) {
            // Перехватываем все остальные исключения (включая системные)
            std::cerr << "[ERROR] Unknown exception caught" << std::endl;
            res.status = 500;
            json error;
            error["success"] = false;
            error["error"] = "Unknown exception occurred during parsing";
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
    
    // Эндпоинт /api/generate - генерация кода из JSON (AST/SPR)
    svr.Post("/api/generate", [&jwtValidator](const httplib::Request& req, httplib::Response& res) {
        // Проверка авторизации
        if (!checkAuthorization(req, jwtValidator)) {
            res.status = 401;
            res.set_content("{\"error\":\"Unauthorized\"}", "application/json");
            return;
        }
        
        try {
            json request = json::parse(req.body);
            // Поддерживаем оба варианта: camelCase (representation) и PascalCase (Representation)
            json representation = request.contains("representation") 
                ? request["representation"] 
                : (request.contains("Representation") ? request["Representation"] : json());
            // Поддерживаем оба варианта: camelCase (language) и PascalCase (Language)
            std::string language = request.contains("language") 
                ? request.value("language", "") 
                : request.value("Language", "");
            
            if (representation.is_null() || language.empty()) {
                res.status = 400;
                json error;
                error["error"] = "Missing 'representation' or 'language' field";
                res.set_content(error.dump(), "application/json");
                return;
            }
            
            std::string generatedCode;
            
            if (language == "pascal") {
                // Генерация Pascal кода из SPR
                PascalCodeGenerator generator;
                generatedCode = generator.generatePascal(representation);
            } else if (language == "c" || language == "cpp") {
                // Генерация C/C++ кода из AST
                CCodeGenerator generator;
                generatedCode = generator.generate(representation);
            } else {
                res.status = 400;
                json error;
                error["error"] = "Unsupported language: " + language;
                res.set_content(error.dump(), "application/json");
                return;
            }
            
            json response;
            response["success"] = true;
            response["code"] = generatedCode;
            response["language"] = language;
            
            res.status = 200;
            res.set_content(response.dump(2), "application/json");
        } catch (const json::parse_error& e) {
            res.status = 400;
            json error;
            error["error"] = "Invalid JSON: " + std::string(e.what());
            res.set_content(error.dump(), "application/json");
        } catch (const std::exception& e) {
            std::cerr << "[ERROR] Exception in /api/generate: " << e.what() << std::endl;
            res.status = 500;
            json error;
            error["success"] = false;
            error["error"] = std::string(e.what());
            res.set_content(error.dump(), "application/json");
        } catch (...) {
            std::cerr << "[ERROR] Unknown exception in /api/generate" << std::endl;
            res.status = 500;
            json error;
            error["success"] = false;
            error["error"] = "Unknown exception occurred during code generation";
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
    std::cout << "  POST /api/generate" << std::endl;
    std::cout << "  GET /health" << std::endl;
    
    if (!svr.listen("0.0.0.0", config.port)) {
        std::cerr << "Failed to start server on port " << config.port << std::endl;
        return 1;
    }
    
    return 0;
}

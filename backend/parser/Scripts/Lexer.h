//
// Created by egorm on 19.04.2024.
//

#ifndef LEXER_H
#define LEXER_H
#include <iostream>
#include <iomanip>
#include <regex>
#include <vector>
#include <string>
#include "Token.h"

enum class LangType{
    LANG_PASCAL, LANG_C, LANG_CPP
};
class Lexer {
private:
    // Вектор Токенов в коде Паскаль/С/С++
    std::vector<Token> tokenList;
    std::string input_string;
    std::string copy_input_string;
    // Вектор всех возможных типов слов в ПаскальАБС/C/C++
    std::vector<std::pair<std::string, std::string> > vector;
    size_t pos = 0;

public:
    Lexer(const std::string &input_string, LangType type) {
        this->input_string = input_string;
        copy_input_string = input_string;
        
        // DEBUG: показать первые 30 байт в HEX
        std::cout << "[LEXER] Input length: " << input_string.length() << std::endl;
        std::cout << "[LEXER] First 30 bytes HEX: ";
        for (size_t i = 0; i < std::min(size_t(30), input_string.length()); i++) {
            std::cout << std::hex << std::setw(2) << std::setfill('0') 
                      << (int)(unsigned char)input_string[i] << " ";
        }
        std::cout << std::dec << std::endl;
        std::cout << "[LEXER] First 30 chars: \"" << input_string.substr(0, 30) << "\"" << std::endl;
        
        switch (type){
            case LangType::LANG_PASCAL:
                vector =getTokenTypePascal();
                while (hasNext()) {}
                break;
            case LangType::LANG_C:
                vector =getTokenTypeC();
                while (hasNext2()) {}
                break;
            case LangType::LANG_CPP:
                vector =getTokenTypeCPlusPlus();
                while (hasNext3()) {}
                break;
            default:
                break;
        }

    }

    bool hasNext() {
        if (pos >= input_string.length()) return false;
        
        const std::string& s = input_string;
        
        // 1. Сначала пропускаем пробелы и переводы строк
        while (pos < s.length() && (s[pos] == ' ' || s[pos] == '\t' || s[pos] == '\n' || s[pos] == '\r')) {
            pos++;
        }
        if (pos >= s.length()) return false;
        
        // DEBUG: показать первые 50 символов от текущей позиции
        if (tokenList.size() < 5) {
            std::cout << "[LEXER DEBUG] pos=" << pos << ", remaining: \"" 
                      << input_string.substr(pos, 50) << "...\"" << std::endl;
        }
        
        // 2. Затем проверяем паттерны (исключая SPACE, так как пробелы уже пропущены)
        for (const auto &item: vector) {
            if (item.first == "SPACE") continue; // Пробелы уже обработаны
            
            std::regex rgx("^" + item.second);
            std::smatch match;
            std::string remaining = s.substr(pos);
            if (std::regex_search(remaining, match, rgx)) {
                // КРИТИЧНО: проверяем, что совпадение начинается в позиции 0 (начало remaining строки)
                if (match.position() != 0) {
                    continue; // Совпадение не в начале - пропускаем этот паттерн
                }
                
                std::string res = static_cast<std::string>(match[0]);
                if (res.empty()) {
                    continue; // Пустое совпадение - пропускаем
                }
                
                // DEBUG: показать совпадение
                if (tokenList.size() < 10) {
                    std::cout << "[LEXER DEBUG] pos=" << pos << ", char='"<< (pos < s.length() ? s[pos] : '?') 
                              << "', Matched pattern \"" << item.second 
                              << "\" -> type=" << item.first << ", value=\"" << res << "\"" << std::endl;
                }
                
                pos += res.length();
                tokenList.emplace_back(item.first, res, pos - res.length());
                return true;
            }
        }
        
        // 3. Если ничего не найдено, выводим предупреждение и пропускаем символ
        std::cout << "[LEXER WARNING] Unknown character: '" << s[pos] << "' at position " << pos << std::endl;
        pos++;
        return true;
    }
    bool hasNext2() {
        if (pos >= input_string.length()) return false;

        const std::string s = input_string;

        // 1. Сначала пробелы
        std::regex spaceRegex("^[ \\t\\n\\r]+");
        std::smatch match;

        if (std::regex_search(s.begin() + pos, s.end(), match, spaceRegex)) {
            pos += match[0].length();
            return true;
        }

        // 2. Затем строковые литералы (ОСОБАЯ ОБРАБОТКА)
        if (s[pos] == '"') {
            size_t end_pos = pos + 1;
            bool escaped = false;

            // Ищем закрывающую кавычку, учитывая экранирование
            while (end_pos < s.length()) {
                if (s[end_pos] == '\\' && !escaped) {
                    escaped = true;
                } else if (s[end_pos] == '"' && !escaped) {
                    break;
                } else {
                    escaped = false;
                }
                end_pos++;
            }

            if (end_pos < s.length()) {
                std::string res = s.substr(pos, end_pos - pos + 1);
                tokenList.emplace_back("VALUESTRING", res, pos);
                pos = end_pos + 1;
                return true;
            }
        }

        // 3. Символьные литералы
        if (s[pos] == '\'') {
            size_t end_pos = pos + 1;
            bool escaped = false;

            while (end_pos < s.length()) {
                if (s[end_pos] == '\\' && !escaped) {
                    escaped = true;
                } else if (s[end_pos] == '\'' && !escaped) {
                    break;
                } else {
                    escaped = false;
                }
                end_pos++;
            }

            if (end_pos < s.length()) {
                std::string res = s.substr(pos, end_pos - pos + 1);
                tokenList.emplace_back("VALUECHAR", res, pos);
                pos = end_pos + 1;
                return true;
            }
        }

        // 4. Затем комментарии
        std::regex commentSingle("^//[^\\n]*");
        std::regex commentMulti("^/\\*.*?\\*/");

        if (std::regex_search(s.begin() + pos, s.end(), match, commentSingle)) {
            std::string res = match[0];
            pos += res.length();
            tokenList.emplace_back("COMMENT", res, pos - res.length());
            return true;
        }

        if (std::regex_search(s.begin() + pos, s.end(), match, commentMulti)) {
            std::string res = match[0];
            pos += res.length();
            tokenList.emplace_back("COMMENT", res, pos - res.length());
            return true;
        }

        // 5. Затем все остальные токены
        for (const auto &item: vector) {
            if (item.first == "SPACE" || item.first == "COMMENT" ||
                item.first == "VALUESTRING" || item.first == "VALUECHAR") continue;

            std::regex rgx("^" + item.second);
            if (std::regex_search(s.begin() + pos, s.end(), match, rgx)) {
                std::string res = match[0];
                pos += res.length();
                tokenList.emplace_back(item.first, res, pos - res.length());
                return true;
            }
        }

        // 6. Если ничего не найдено
        std::cout << "Unknown character: '" << s[pos] << "' at position " << pos << std::endl;
        pos++;
        return true;
    }
    bool hasNext3() {
        if (pos >= input_string.length()) return false;

        const std::string& s = input_string; // Используем ссылку!

        // 1. Пропускаем пробелы быстро (без regex)
        while (pos < s.length() && std::isspace(static_cast<unsigned char>(s[pos]))) {
            pos++;
        }
        if (pos >= s.length()) return false;

        // 2. Обработка строковых литералов (оптимизировано)
        if (s[pos] == '"') {
            size_t end_pos = pos + 1;
            while (end_pos < s.length() && s[end_pos] != '"') {
                if (s[end_pos] == '\\' && end_pos + 1 < s.length()) {
                    end_pos += 2; // Пропускаем экранированный символ
                } else {
                    end_pos++;
                }
            }
            if (end_pos < s.length()) {
                tokenList.emplace_back("VALUESTRING", s.substr(pos, end_pos - pos + 1), pos);
                pos = end_pos + 1;
                return true;
            }
        }

        // 3. Символьные литералы
        if (s[pos] == '\'') {
            size_t end_pos = pos + 1;
            while (end_pos < s.length() && s[end_pos] != '\'') {
                if (s[end_pos] == '\\' && end_pos + 1 < s.length()) {
                    end_pos += 2;
                } else {
                    end_pos++;
                }
            }
            if (end_pos < s.length()) {
                tokenList.emplace_back("VALUECHAR", s.substr(pos, end_pos - pos + 1), pos);
                pos = end_pos + 1;
                return true;
            }
        }

        // 4. Комментарии (оптимизировано)
        if (pos + 1 < s.length()) {
            if (s[pos] == '/' && s[pos + 1] == '/') {
                // Однострочный комментарий
                size_t end_pos = s.find('\n', pos + 2);
                if (end_pos == std::string::npos) end_pos = s.length();
                tokenList.emplace_back("COMMENT", s.substr(pos, end_pos - pos), pos);
                pos = end_pos;
                return true;
            }
            if (s[pos] == '/' && s[pos + 1] == '*') {
                // Многострочный комментарий
                size_t end_pos = s.find("*/", pos + 2);
                if (end_pos != std::string::npos) {
                    tokenList.emplace_back("COMMENT", s.substr(pos, end_pos - pos + 2), pos);
                    pos = end_pos + 2;
                    return true;
                }
            }
        }

        // 5. Быстрая проверка односимвольных токенов (без regex)
        char current_char = s[pos];
        switch (current_char) {
            case ';': tokenList.emplace_back("SEMICOLON", ";", pos); pos++; return true;
            case ',': tokenList.emplace_back("COMMA", ",", pos); pos++; return true;
            case ':':
                if (pos + 1 < s.length() && s[pos + 1] == ':') {
                    tokenList.emplace_back("SCOPE", "::", pos); pos += 2; return true;
                }
                tokenList.emplace_back("COLON", ":", pos); pos++; return true;
            case '(': tokenList.emplace_back("OPENPARENTHESES", "(", pos); pos++; return true;
            case ')': tokenList.emplace_back("CLOSEPARENTHESES", ")", pos); pos++; return true;
            case '{': tokenList.emplace_back("OPENCURLY", "{", pos); pos++; return true;
            case '}': tokenList.emplace_back("CLOSECURLY", "}", pos); pos++; return true;
            case '[': tokenList.emplace_back("OPENBRACKET", "[", pos); pos++; return true;
            case ']': tokenList.emplace_back("CLOSEBRACKET", "]", pos); pos++; return true;
            case '?': tokenList.emplace_back("QUESTION", "?", pos); pos++; return true;
            case '~': tokenList.emplace_back("BITNOT", "~", pos); pos++; return true;
        }

        // 6. Многосимвольные операторы (быстрая проверка)
        if (pos + 1 < s.length()) {
            std::string two_chars = s.substr(pos, 2);
            std::string three_chars = pos + 2 < s.length() ? s.substr(pos, 3) : "";

            // Проверяем трехсимвольные операторы
            if (!three_chars.empty()) {
                if (three_chars == "...") { tokenList.emplace_back("ELLIPSIS", "...", pos); pos += 3; return true; }
                if (three_chars == "->*") { tokenList.emplace_back("PTRMEMBER", "->*", pos); pos += 3; return true; }
                if (three_chars == ".*") { tokenList.emplace_back("MEMBERPTR", ".*", pos); pos += 2; return true; }
                if (three_chars == "<<=") { tokenList.emplace_back("SHIFTLEFTASSIGN", "<<=", pos); pos += 3; return true; }
                if (three_chars == ">>=") { tokenList.emplace_back("SHIFTRIGHTASSIGN", ">>=", pos); pos += 3; return true; }
            }

            // Двухсимвольные операторы
            if (two_chars == "->") { tokenList.emplace_back("PTRACCESS", "->", pos); pos += 2; return true; }
            if (two_chars == "++") { tokenList.emplace_back("INCREMENT", "++", pos); pos += 2; return true; }
            if (two_chars == "--") { tokenList.emplace_back("DECREMENT", "--", pos); pos += 2; return true; }
            if (two_chars == "+=") { tokenList.emplace_back("PLUSASSIGN", "+=", pos); pos += 2; return true; }
            if (two_chars == "-=") { tokenList.emplace_back("MINUSASSIGN", "-=", pos); pos += 2; return true; }
            if (two_chars == "*=") { tokenList.emplace_back("MULTIASSIGN", "*=", pos); pos += 2; return true; }
            if (two_chars == "/=") { tokenList.emplace_back("DIVASSIGN", "/=", pos); pos += 2; return true; }
            if (two_chars == "%=") { tokenList.emplace_back("MODASSIGN", "%=", pos); pos += 2; return true; }
            if (two_chars == "&=") { tokenList.emplace_back("ANDASSIGN", "&=", pos); pos += 2; return true; }
            if (two_chars == "|=") { tokenList.emplace_back("ORASSIGN", "|=", pos); pos += 2; return true; }
            if (two_chars == "^=") { tokenList.emplace_back("XORASSIGN", "^=", pos); pos += 2; return true; }
            if (two_chars == "<<") { tokenList.emplace_back("BITSHIFTLEFT", "<<", pos); pos += 2; return true; }
            if (two_chars == ">>") { tokenList.emplace_back("BITSHIFTRIGHT", ">>", pos); pos += 2; return true; }
            if (two_chars == ">=") { tokenList.emplace_back("JGE", ">=", pos); pos += 2; return true; }
            if (two_chars == "<=") { tokenList.emplace_back("JLE", "<=", pos); pos += 2; return true; }
            if (two_chars == "!=") { tokenList.emplace_back("JNE", "!=", pos); pos += 2; return true; }
            if (two_chars == "==") { tokenList.emplace_back("JE", "==", pos); pos += 2; return true; }
            if (two_chars == "&&") { tokenList.emplace_back("AND", "&&", pos); pos += 2; return true; }
            if (two_chars == "||") { tokenList.emplace_back("OR", "||", pos); pos += 2; return true; }
        }

        // 7. Односимвольные операторы (продолжение)
        switch (current_char) {
            case '+': tokenList.emplace_back("PLUS", "+", pos); pos++; return true;
            case '-': tokenList.emplace_back("MINUS", "-", pos); pos++; return true;
            case '*': tokenList.emplace_back("MULTI", "*", pos); pos++; return true;
            case '/': tokenList.emplace_back("DIV", "/", pos); pos++; return true;
            case '%': tokenList.emplace_back("MOD", "%", pos); pos++; return true;
            case '=': tokenList.emplace_back("ASSIGN", "=", pos); pos++; return true;
            case '>': tokenList.emplace_back("JG", ">", pos); pos++; return true;
            case '<': tokenList.emplace_back("JL", "<", pos); pos++; return true;
            case '!': tokenList.emplace_back("NOT", "!", pos); pos++; return true;
            case '&': tokenList.emplace_back("BITAND", "&", pos); pos++; return true;
            case '|': tokenList.emplace_back("BITOR", "|", pos); pos++; return true;
            case '^': tokenList.emplace_back("BITXOR", "^", pos); pos++; return true;
            case '.': tokenList.emplace_back("MEMBERACCESS", ".", pos); pos++; return true;
        }

        // 8. Только теперь используем regex для сложных случаев
        std::smatch match;
        for (const auto &item: vector) {
            // Пропускаем уже обработанные типы
            if (item.first == "SPACE" || item.first == "COMMENT" ||
                item.first == "VALUESTRING" || item.first == "VALUECHAR") {
                continue;
            }

            std::regex rgx("^" + item.second);
            if (std::regex_search(s.begin() + pos, s.end(), match, rgx)) {
                std::string res = match[0];
                pos += res.length();
                tokenList.emplace_back(item.first, res, pos - res.length());
                return true;
            }
        }

        // 9. Если ничего не найдено, продвигаемся
        std::cout << "Unknown character: '" << s[pos] << "' at position " << pos << std::endl;
        pos++;
        return true;
    }

    void printTokenList() {
        for (auto item: tokenList) {
            std::cout << "Token(type=" << item.getType() << ",value=" << item.getValue() << ")" << std::endl;
        }
    }

    std::vector<Token> getTokenList() {
        return this->tokenList;
    }

   static std::vector<std::pair<std::string, std::string>> getTokenTypeCPlusPlus() {
        return {
                // === ВЫСОКОПРИОРИТЕТНЫЕ ТОКЕНЫ ===

                // Комментарии (обрабатываются отдельно в hasNext3)
                {"COMMENT", "//[^\\n]*|/\\*.*?\\*/"},

                // Строковые и символьные литералы (обрабатываются отдельно)
                {"VALUESTRING", "\"[^\"]*\""},
                {"VALUECHAR", "'[^']*'"},

                // === ПРЕПРОЦЕССОР (оптимизировано) ===
                {"INCLUDE", "#include\\s*<[^>]+>"},
                {"INCLUDE", "#include\\s*\"[^\"]+\""},
                {"DEFINE", "#define\\b"},
                {"IFDEF", "#ifdef\\b"},
                {"IFNDEF", "#ifndef\\b"},
                {"ENDIF", "#endif\\b"},
                {"PRAGMA", "#pragma\\b"},

                // === КЛЮЧЕВЫЕ СЛОВА (группированы по длине для оптимизации) ===

                // Длинные ключевые слова сначала
                {"STATIC_ASSERT", "static_assert\\b"},
                {"CONSTEXPR", "constexpr\\b"},
                {"DECLTYPE", "decltype\\b"},
                {"NOEXCEPT", "noexcept\\b"},
                {"OPERATOR", "operator\\b"},
                {"NAMESPACE", "namespace\\b"},
                {"TYPENAME", "typename\\b"},
                {"TEMPLATE", "template\\b"},
                {"EXPLICIT", "explicit\\b"},
                {"MUTABLE", "mutable\\b"},
                {"VOLATILE", "volatile\\b"},
                {"REGISTER", "register\\b"},
                {"OVERRIDE", "override\\b"},
                {"VIRTUAL", "virtual\\b"},
                {"PRIVATE", "private\\b"},
                {"PUBLIC", "public\\b"},
                {"PROTECTED", "protected\\b"},
                {"CONTINUE", "continue\\b"},
                {"STATIC", "static\\b"},
                {"EXTERN", "extern\\b"},
                {"INLINE", "inline\\b"},
                {"SIZEOF", "sizeof\\b"},
                {"TYPEDEF", "typedef\\b"},
                {"DELETE", "delete\\b"},
                {"FRIEND", "friend\\b"},
                {"RETURN", "return\\b"},
                {"STRUCT", "struct\\b"},
                {"SWITCH", "switch\\b"},
                {"THROW", "throw\\b"},
                {"CATCH", "catch\\b"},
                {"CLASS", "class\\b"},
                {"CONST", "const\\b"},
                {"FINAL", "final\\b"},
                {"USING", "using\\b"},
                {"WHILE", "while\\b"},
                {"BREAK", "break\\b"},
                {"CASE", "case\\b"},
                {"ELSE", "else\\b"},
                {"ENUM", "enum\\b"},
                {"GOTO", "goto\\b"},
                {"AUTO", "auto\\b"},
                {"BOOL", "bool\\b"},
                {"CHAR", "char\\b"},
                {"DOUBLE", "double\\b"},
                {"FLOAT", "float\\b"},
                {"LONG", "long\\b"},
                {"SHORT", "short\\b"},
                {"UNION", "union\\b"},
                {"UNSIGNED", "unsigned\\b"},
                {"SIGNED", "signed\\b"},
                {"TRY", "try\\b"},
                {"VOID", "void\\b"},
                {"THIS", "this\\b"},
                {"NEW", "new\\b"},
                {"DO", "do\\b"},
                {"IF", "if\\b"},
                {"FOR", "for\\b"},
                {"INT", "int\\b"},

                // === ОПЕРАТОРЫ (группированы по уникальности) ===

                // Многосимвольные операторы сначала
                {"PTRMEMBER", "->\\*"},
                {"MEMBERPTR", "\\.\\*"},
                {"SHIFTLEFTASSIGN", "<<="},
                {"SHIFTRIGHTASSIGN", ">>="},
                {"PLUSASSIGN", "\\+="},
                {"MINUSASSIGN", "-="},
                {"MULTIASSIGN", "\\*="},
                {"DIVASSIGN", "/="},
                {"MODASSIGN", "%="},
                {"ANDASSIGN", "&="},
                {"ORASSIGN", "\\|="},
                {"XORASSIGN", "\\^="},
                {"INCREMENT", "\\+\\+"},
                {"DECREMENT", "--"},
                {"PTRACCESS", "->"},
                {"BITSHIFTLEFT", "<<"},
                {"BITSHIFTRIGHT", ">>"},
                {"ELLIPSIS", "\\.\\.\\."},
                {"SPACESHIP", "<=>"},
                {"JGE", ">="},
                {"JLE", "<="},
                {"JNE", "!="},
                {"JE", "=="},
                {"AND", "&&"},
                {"OR", "\\|\\|"},
                {"SCOPE", "::"},

                // Односимвольные операторы
                {"PLUS", "\\+"},
                {"MINUS", "-"},
                {"MULTI", "\\*"},
                {"DIV", "/"},
                {"MOD", "%"},
                {"ASSIGN", "="},
                {"JG", ">"},
                {"JL", "<"},
                {"NOT", "!"},
                {"BITAND", "&"},
                {"BITOR", "\\|"},
                {"BITXOR", "\\^"},
                {"BITNOT", "~"},
                {"QUESTION", "\\?"},
                {"MEMBERACCESS", "\\."},  // Убрали DOT - дублирование

                // === РАЗДЕЛИТЕЛИ ===
                {"SEMICOLON", ";"},
                {"COMMA", ","},
                {"COLON", ":"},
                {"OPENPARENTHESES", "\\("},
                {"CLOSEPARENTHESES", "\\)"},
                {"OPENCURLY", "\\{"},
                {"CLOSECURLY", "\\}"},
                {"OPENBRACKET", "\\["},
                {"CLOSEBRACKET", "\\]"},

                // === ЛИТЕРАЛЫ ===
                {"VALUEBOOL", "true\\b|false\\b"},
                {"VALUENULLPTR", "nullptr\\b"},
                {"VALUEHEX", "0[xX][0-9a-fA-F]+"},
                {"VALUEOCTAL", "0[0-7]+"},
                {"VALUEFLOAT", "[0-9]*\\.[0-9]+([eE][-+]?[0-9]+)?[fF]?"},
                {"VALUEINTEGER", "[0-9]+"},

                // === ИДЕНТИФИКАТОРЫ (в конце - самый общий паттерн) ===
                {"IDENTIFIER", "[a-zA-Z_][a-zA-Z0-9_]*"},

        };
    }
    static std::vector<std::pair<std::string, std::string>> getTokenTypeC() {
        return {
                // Препроцессорные директивы
                {"INCLUDE", "#include[ ]*<[^>]+>"},
                {"INCLUDE", "#include[ ]*\"[^\"]+\""},
                {"DEFINE", "#define\\b"},
                {"PREPROCESSOR", "#[a-zA-Z_][a-zA-Z0-9_]*"},

                // Ключевые слова
                {"IF", "\\bif\\b"},
                {"ELSE", "\\belse\\b"},
                {"WHILE", "\\bwhile\\b"},
                {"FOR", "\\bfor\\b"},
                {"DO", "\\bdo\\b"},
                {"SWITCH", "\\bswitch\\b"},
                {"CASE", "\\bcase\\b"},
                {"DEFAULT", "\\bdefault\\b"},
                {"BREAK", "\\bbreak\\b"},
                {"CONTINUE", "\\bcontinue\\b"},
                {"RETURN", "\\breturn\\b"},
                {"VOID", "\\bvoid\\b"},
                {"INT", "\\bint\\b"},
                {"FLOAT", "\\bfloat\\b"},
                {"DOUBLE", "\\bdouble\\b"},
                {"CHAR", "\\bchar\\b"},
                {"SHORT", "\\bshort\\b"},
                {"LONG", "\\blong\\b"},
                {"SIGNED", "\\bsigned\\b"},
                {"UNSIGNED", "\\bunsigned\\b"},
                {"CONST", "\\bconst\\b"},
                {"STATIC", "\\bstatic\\b"},
                {"STRUCT", "\\bstruct\\b"},
                {"TYPEDEF", "\\btypedef\\b"},
                {"SIZEOF", "\\bsizeof\\b"},

                // Многосимвольные операторы
                {"PTRACCESS", "->"},
                {"INCREMENT", "\\+\\+"},
                {"DECREMENT", "--"},
                {"PLUSASSIGN", "\\+="},
                {"MINUSASSIGN", "-="},
                {"MULTIASSIGN", "\\*="},
                {"DIVASSIGN", "/="},
                {"MODASSIGN", "%="},
                {"JGE", ">="},
                {"JLE", "<="},
                {"JNE", "!="},
                {"JE", "=="},
                {"AND", "&&"},
                {"OR", "\\|\\|"},
                {"BITSHIFTLEFT", "<<"},
                {"BITSHIFTRIGHT", ">>"},
                {"QUESTION", "\\?"},  // Тернарный оператор

                // Односимвольные операторы
                {"PLUS", "\\+"},
                {"MINUS", "-"},
                {"MULTI", "\\*"},
                {"DIV", "/"},
                {"MOD", "%"},
                {"ASSIGN", "="},
                {"JG", ">"},
                {"JL", "<"},
                {"NOT", "!"},
                {"BITAND", "&"},
                {"BITOR", "\\|"},
                {"BITXOR", "\\^"},
                {"BITNOT", "~"},

                // Разделители
                {"SEMICOLON", ";"},
                {"COMMA", ","},
                {"COLON", ":"},
                {"OPENPARENTHESES", "\\("},
                {"CLOSEPARENTHESES", "\\)"},
                {"OPENCURLY", "\\{"},
                {"CLOSECURLY", "\\}"},
                {"OPENBRACKET", "\\["},
                {"CLOSEBRACKET", "\\]"},
                {"DOT", "\\."},

                // Литералы (УПРОЩЕННЫЕ)
                {"VALUESTRING", "\"[^\"]*\"?"},  // Простая строка до закрывающей кавычки
                {"VALUECHAR", "'[^']*'?"},
                {"VALUEHEX", "0[xX][0-9a-fA-F]+"},
                {"VALUEOCTAL", "0[0-7]+"},
                {"VALUEFLOAT", "[0-9]+\\.[0-9]+[fF]?"},
                {"VALUEDOUBLE", "[0-9]+\\.[0-9]+"},
                {"VALUEINTEGER", "[0-9]+"},

                // Идентификаторы
                {"IDENTIFIER", "[a-zA-Z_][a-zA-Z0-9_]*"},

                // Пробелы и комментарии
                {"SPACE", "[ \\t\\n\\r]"},
              //  {"COMMENT", "//[^\\n]*|/\\*.*?\\*/"}
        };
    }
    static std::vector<std::pair<std::string, std::string>> getTokenTypePascal() {
        return {
            // TITLE должен быть первым, так как он более специфичный паттерн
            {"TITLE", "program [A-Za-z0-9_]+"},
            // Разделители проверяем ДО ключевых слов
            {"SEMICOLON", ";"},
            {"COLON", ":"},
            {"COMMA", ","},
            {"OPENPARENTHESES", "[(]"},
            {"CLOSEPARENTHESES", "[)]"},
            {"OPENSQUARE", "[\\[]"},
            {"CLOSESQUARE", "[\\]]"},
            // Ключевые слова с границами слова
            {"FUNCTION", "\\bfunction\\b"},
            {"PROCEDURE","\\bprocedure\\b"},
            {"CONST", "\\bconst\\b"},
            {"VAR", "\\bvar\\b"},
            {"INC","\\bto\\b"},
            {"DEC","\\bdownto\\b"},
            {"THEN", "\\bthen\\b"},
            {"DO", "\\bdo\\b"},
            {"OF", "\\bof\\b"},
            {"MOD", "\\bmod\\b"},
            {"DIV", "\\bdiv\\b"},
            {"SWITCH", "\\bcase\\b"},
            {"BEGIN", "\\bbegin\\b"},
            {"ENDofCycle", "end;"},
            {"ENDofPROGRAM", "end[\\.]"},
            {"ENDofIF","\\bend\\b"},
            {"TYPEINTEGER", "\\binteger\\b"},
            {"TYPEREAL", "\\breal\\b"},
            {"TYPESTRING", "\\bstring\\b"},
            {"TYPECHAR", "\\bchar\\b"},
            {"TYPEBOOLEAN", "\\bboolean\\b"},
            {"CONDITION", "\\bif\\b"},
            {"UNCONDITION", "\\belse\\b"},
            {"WRITELN","\\bWriteln\\b"},
            {"READLN","\\bReadln\\b"},
            {"WRITE", "\\bWrite\\b"},
            {"READ", "\\bRead\\b"},
            {"CYCLEFOR", "\\bfor\\b"},
            {"CYCLEWHILE", "\\bwhile\\b"},
            {"CYCLEDOWHILE","\\brepeat\\b"},
            {"UNTIL","\\buntil\\b"},
            {"AND", "\\band\\b"},
            {"OR", "\\bor\\b"},
            {"NOT", "\\bnot\\b"},
            {"XOR", "\\bxor\\b"},
            {"VALUEBOOLEANTrue", "\\bTrue\\b"},
            {"VALUEBOOLEANFalse", "\\bFalse\\b"},
            // Операторы
            {"PLUS", "[+]"},
            {"MINUS","[-]"},
            {"MULTI","[*]"},
            {"ASSIGN", ":="},
            {"JGE", ">="},
            {"JLE", "<="},
            {"JNE", "<>"},
            {"JG", ">"},
            {"JL", "<"},
            {"JE", "="},
            // Литералы
            {"VALUEREAL", "[0-9]+\\.[0-9]+"},
            {"VALUEINTEGER", "[0-9]+"},
            {"VALUECHAR", "['][A-Za-z0-9][']"},
            {"VALUESTRING", "['][A-Za-z0-9!?,\\.: _-]+[']"},
            // Идентификаторы и пробелы в конце
            {"VARIABLE", "[a-z0-9A-Z_-]+"},
            {"SPACE", "[ \t\n]"}
        };
    }
};


#endif //LEXER_H

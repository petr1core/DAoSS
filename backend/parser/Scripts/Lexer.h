//
// Created by egorm on 19.04.2024.
//

#ifndef LEXER_H
#define LEXER_H
#include <iostream>
#include <regex>
#include <vector>
#include <string>
#include "Token.h"

enum types{
    PASCAL, C, CPLUSPLUS, MERMAID
};
class Lexer {
private:
    // Вектор Токенов в коде Паскаль/С/С++
    std::vector<Token> tokenList;
    std::string input_string;
    std::string copy_input_string;
    // Вектор всех возможных типов слов в ПаскальАБС/C/C++
    std::vector<std::pair<std::string, std::string> > vector;
    int pos = 0;

public:
    Lexer(const std::string &input_string, types type) {
        this->input_string = input_string;
        copy_input_string = input_string;
        switch (type){
            case PASCAL:
                vector =getTokenTypePascal();
                while (hasNext()) {}
                break;
            case C:
                vector =getTokenTypeC();
                while (hasNext2()) {}
                break;
            case CPLUSPLUS:
                vector =getTokenTypeCPlusPlus();
                break;
            case MERMAID:
                vector =getTokenTypeMermaid();
                while(hasNext3()){}
                break;
            default:
                break;
        }

    }

    bool hasNext() {
        for (const auto &item: vector) {
            const std::string s = input_string;
            std::regex rgx("^" + item.second);
            std::smatch match;
            if (std::regex_search(s.begin() + pos, s.end(), match, rgx)) {
                std::string res = static_cast<std::string>(match[0]);
                pos += res.length();
                if (item.first != "SPACE") {
                    tokenList.emplace_back(item.first, res, pos - res.length());
                   /* std::cout<<"Type of lexema: " <<item.first <<
                     "; Pos: " << ++i <<
                      "; Value of lexema: "<<res<<" ;"<<std::endl;*/
                }
                return true;
            }
        }
        return false;
    }
    // Специальный метод для языка C с приоритетами
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
            for (const auto &item: vector) {
                const std::string s = input_string;
                std::regex rgx("^" + item.second);
                std::smatch match;
                if (std::regex_search(s.begin() + pos, s.end(), match, rgx)) {
                    std::string res = static_cast<std::string>(match[0]);
                    pos += res.length();
                    if (item.first != "SPACE") {
                        tokenList.emplace_back(item.first, res, pos - res.length());
                    }
                    return true;
                }
            }
            return false;

    }

    void printTokenList() {
        for (auto item: tokenList) {
            std::cout << "Token(type=" << item.getType() << ",value=" << item.getValue() << ")" << std::endl;
        }
    }

    std::vector<Token> getTokenList() {
        return this->tokenList;
    }
    
    /**
     * Разбирает метку узла из Mermaid на токены
     * Пример: "if 5 mod 3 > 0 then" -> [CONDITION:if, VALUEINTEGER:5, MOD:mod, ...]
     */
    static std::vector<std::pair<std::string, std::string>> getTokenTypeMermaid() {
        return {
                // Основные ключевые слова блок-схем
                {"FLOWCHART", "flowchart"},
                {"GRAPH", "graph"},

                // Направления
                {"DIRECTION_TB", "TB"},
                {"DIRECTION_BT", "BT"},
                {"DIRECTION_LR", "LR"},
                {"DIRECTION_RL", "RL"},
                {"DIRECTION_TD", "TD"},

                // Типы блоков (узлов)
                {"START_END", "\\(\\[.*?\\]\\)"},                   // Начало/конец ([ ])
                {"NEWFOR", "\\{\\{.*?\\}\\}"},                      // Отдельный блок для for {{ }}
                {"DECISION", "\\{.*?\\}"},                          // Ромбик { }
                {"INPUT_OUTPUT", "\\[\\/.*?\\/\\]"},                // Ввод/вывод [/ /]
                {"SUBROUTINE", "\\[\\[.*?\\]\\]"},                  // Подпрограмма [[ ]]
                {"DATABASE", "\\[\\(\\|.*?\\|\\)\\]"},              // База данных [(| |)]
                {"JUSTBLOCK", "\\[.*?\\]"},                         // Блок [ ]

                {"NODE_ID", "N\\d+"},
                // Операторы связей
                {"CONDITION_TRUE", "--\\>\\|true\\|"},
                {"CONDITION_FALSE", "--\\>\\|false\\|"},
                {"CONDITION_SWITCH", "--\\>\\|[0-9a-zA-Z'\" ]+\\|"},
                {"ARROW_SOLID", "-->"},

                {"LINK_SOLID", "---"},
                {"LINK_THICK", "==="},

                // Ветки условий (true/false)

                {"ARROW_TEXT", "\\|.*?\\|"},

                // Подграфы (группы)
                {"SUBGRAPH_START", "subgraph"},
                {"SUBGRAPH_END", "end"},

                // Стили и классы
                {"STYLE", "style"},
                {"CLASS_DEF", "classDef"},
                {"CLASS", "class"},
                {"CLICK", "click"},
                {"LINK_STYLE", "linkStyle"},

                // Текст и содержимое
                {"QUOTED_STRING", "\"[^\"]*\""},
                {"IDENTIFIER", "N\\d*"},
                {"TEXT_CONTENT", "[^\\s\\[\\]\\(\\)\\{\\}<>,;\"]+"},

                // Символы и разделители
                {"BRACKET_OPEN", "\\["},
                {"BRACKET_CLOSE", "\\]"},
                {"PAREN_OPEN", "\\("},
                {"PAREN_CLOSE", "\\)"},
                {"BRACE_OPEN", "\\{"},
                {"BRACE_CLOSE", "\\}"},
                {"ANGLE_OPEN", "<"},
                {"ANGLE_CLOSE", ">"},
                {"COMMA", ","},
                {"SEMICOLON", ";"},
                {"COLON", ":"},
                {"EQUALS", "="},
                {"PIPE", "\\|"},

                // Комментарии
                {"COMMENT", "%%[^\\n]*"},

                // Пробельные символы
                {"SPACE", "[ \t\n]"},
                {"NEWLINE", "\\n"},

        };
    }
    static std::vector<std::pair<std::string, std::string>> getTokenTypeCPlusPlus() {
        return {
                {"CONST", "const"},
                {"VAR", "var"},
                {"INC","to"},
                {"DEC","downto"},
                {"THEN", "then"},
                {"DO", "do"},
                {"OF", "of"},
                {"MOD", "mod"},
                {"DIV", "div"},
                {"PLUS", "[\+]"},
                {"MINUS","[-]"},
                {"MULTI","[\*]"},
                {"SWITCH", "case"},
                {"FUNCTION", "function"},
                {"PROCEDURE","procedure"},
                {"BEGIN", "begin"},
                {"ENDofCycle", "end;"},
                {"ENDofPROGRAM", "end[\.]"},
                {"ENDofIF","end"},
                {"TYPEINTEGER", "integer"},
                {"TYPEREAL", "real"},
                {"TYPESTRING", "string"},
                {"TYPECHAR", "char"},
                {"TYPEBOOLEAN", "boolean"},
                {"VALUEREAL", "[0-9]+\.[0-9]+"},
                {"VALUEINTEGER", "[0-9]+"},
                {"VALUECHAR", "['][A-Za-z0-9][']"},
                {"VALUESTRING", "['][A-Za-z0-9!?,\.: _-]+[']"},
                {"VALUEBOOLEANTrue", "True"},
                {"VALUEBOOLEANFalse", "False"},
                {"ASSIGN", ":="},
                {"JGE", ">="},
                {"JLE", "<="},
                {"JNE", "<>"},
                {"JG", ">"},
                {"JL", "<"},
                {"JE", "="},
                {"AND", "and"},
                {"OR", "or"},
                {"NOT", "not"},
                {"XOR", "xor"},
                {"COLON", ":"},
                {"COMMA", ","},
                {"SEMICOLON", ";"},
                {"TITLE", "program [A-Za-z0-9_]+"},
                {"OPENPARENTHESES", "[(]"},
                {"CLOSEPARENTHESES", "[)]"},
                {"OPENSQUARE", "[\[]"},
                {"CLOSESQUARE", "[\]]"},
                {"CONDITION", "if"},
                {"UNCONDITION", "else"},
                {"WRITELN","Writeln"},
                {"READLN","Readln"},
                {"WRITE", "Write"},
                {"READ", "Read"},
                {"CYCLEFOR", "for"},
                {"CYCLEWHILE", "while"},
                {"CYCLEDOWHILE","repeat"},
                {"UNTIL","until"},
                {"VARIABLE", "[a-z0-9A-Z_-]+"},
                {"SPACE", "[ \t\n]"}
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
                {"OPENSQUARE", "\\["},
                {"CLOSESQUARE", "\\]"},
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
            {"CONST", "const"},
            {"VAR", "var"},
            {"INC","to"},
            {"DEC","downto"},
            {"THEN", "then"},
            {"DO", "do"},
            {"OF", "of"},
            {"MOD", "mod"},
            {"DIV", "div"},
            {"PLUS", "[\+]"},
            {"MINUS","[-]"},
            {"MULTI","[\*]"},
            {"SWITCH", "case"},
            {"FUNCTION", "function"},
            {"PROCEDURE","procedure"},
            {"BEGIN", "begin"},
            {"ENDofCycle", "end;"},
            {"ENDofPROGRAM", "end[\.]"},
            {"ENDofIF","end"},
            {"TYPEINTEGER", "integer"},
            {"TYPEREAL", "real"},
            {"TYPESTRING", "string"},
            {"TYPECHAR", "char"},
            {"TYPEBOOLEAN", "boolean"},
            {"VALUEREAL", "[0-9]+\.[0-9]+"},
            {"VALUEINTEGER", "[0-9]+"},
            {"VALUECHAR", "['][A-Za-z0-9][']"},
            {"VALUESTRING", "['][A-Za-z0-9!?,\.: _-]+[']"},
            {"VALUEBOOLEANTrue", "True"},
            {"VALUEBOOLEANFalse", "False"},
            {"ASSIGN", ":="},
            {"JGE", ">="},
            {"JLE", "<="},
            {"JNE", "<>"},
            {"JG", ">"},
            {"JL", "<"},
            {"JE", "="},
            {"AND", "and"},
            {"OR", "or"},
            {"NOT", "not"},
            {"XOR", "xor"},
            {"COLON", ":"},
            {"COMMA", ","},
            {"SEMICOLON", ";"},
            {"TITLE", "program [A-Za-z0-9_]+"},
            {"OPENPARENTHESES", "[(]"},
            {"CLOSEPARENTHESES", "[)]"},
            {"OPENSQUARE", "[\[]"},
            {"CLOSESQUARE", "[\]]"},
            {"CONDITION", "if"},
            {"UNCONDITION", "else"},
            {"WRITELN","Writeln"},
            {"READLN","Readln"},
            {"WRITE", "Write"},
            {"READ", "Read"},
            {"CYCLEFOR", "for"},
            {"CYCLEWHILE", "while"},
            {"CYCLEDOWHILE","repeat"},
            {"UNTIL","until"},
            {"VARIABLE", "[a-z0-9A-Z_-]+"},
            {"SPACE", "[ \t\n]"}
        };
    }
};


#endif //LEXER_H

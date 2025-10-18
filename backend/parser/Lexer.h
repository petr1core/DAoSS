//
// Created by egorm on 19.04.2024.
//

#ifndef LEXER_H
#define LEXER_H
#include <iostream>
#include <regex>
#include<vector>
#include<string>
#include "Token.h"

//Класс вроде как закончен, мб дополнения
class Lexer {
private:
    // Вектор Токенов в коде Паскаль
    std::vector<Token> tokenList;
    std::string input_string;
    std::string copy_input_string;
    // Вектор всех возможных типов слов в ПаскальАБС
    std::vector<std::pair<std::string, std::string> > vector;
    int pos = 0;
    int i=0;

public:
    Lexer(const std::string &input_string) {
        this->input_string = input_string;
        copy_input_string = input_string;
        vector = getLitTokenType();
        while (hasNext()) {
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
    std::vector<Token> getTokenList() {
        return this->tokenList;
    }
    void printTokenList() {
        for (auto item: tokenList) {
            std::cout << "Token(type=" << item.getType() << ",value=" << item.getValue() << ")" << std::endl;
        }
    }

    std::vector<std::pair<std::string, std::string> > getLitTokenType() {
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

//
// Created by egorm on 19.04.2024.
//

#ifndef TOKEN_H
#define TOKEN_H
#include <string>
#include <utility>

using namespace std;

class Token {
private:
    std::string type;
    std::string value;
    int pos;

public:
    Token(){
        this->type=std::string();
        this->value=std::string();
        this->pos=int();

    }
    Token(std::string type, std::string value, int pos) {
        this->type = std::move(type);
        this->value = std::move(value);
        this->pos = pos;
    }
    Token(const Token& other) {
        this->type = other.type;
        this->value = other.value;
        this->pos = other.pos;
    };
    
    std::string getType() {
        return this->type;
    }
    std::string getValue() {
        return this->value;
    }
    int getPos() {
        return this->pos;
    }
    Token& operator=(const Token& other) {
        this->type = other.type;
        this->value = other.value;
        this->pos=other.pos;
        return *this;
    }
    friend std::ostream& operator<<(std::ostream& ostr, const Token& v)
    {
        std::cout<<v.type;
       return ostr;
    }
};


#endif //TOKEN_H

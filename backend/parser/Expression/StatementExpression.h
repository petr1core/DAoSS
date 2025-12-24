//
// Created by egorm on 20.04.2024.
//

#ifndef RUNNABLEEXPRESSION_H
#define RUNNABLEEXPRESSION_H
#include <utility>
#include <vector>
#include "Expression.h"
#include "stdexcept"
#include "../Scripts/Token.h"
static int i=0;
class StatementExpression : public Expression {
private:
    int i1;
    std::vector<Token> list;
public:
    // ИСПРАВЛЕНО: принимаем vector по значению для поддержки move семантики
    // Это критично для избежания bad_alloc при большом количестве statement-ов
    StatementExpression(std::vector<Token> _list)
    {
        i1=++i;
        list=std::move(_list);
    }
    StatementExpression(const StatementExpression& ex){
        i1=++i;
        this->list=ex.list;
    }
    StatementExpression& operator=(const StatementExpression& other) {
        i1=++i;
        this->list=other.list;
        return *this;
    }
    vector<Token>getList(){return list;}
    void add(Token t){list.push_back(t);}
    void print(int tab) override{
        for(int j=0;j<tab;j++){
            cout<<"   ";
        }
        std::cout<<"StateExpression "<<i1<<" = ";
        for(auto token:list){
          std::cout<<token.getValue()<<" ";
        }
        std::cout<<endl;
    }
};
#endif //RUNNABLEEXPRESSION_H
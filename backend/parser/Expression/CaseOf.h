//
// Created by shuri on 11.05.2024.
//

#ifndef CASEOF_H
#define CASEOF_H
#include <vector>
#include "Expression.h"
#include "../Scripts/Token.h"
static int c=0;
class CaseOf: public Expression {
private:
    int c1;
    Token value;
    vector<std::pair<vector<Token>,vector<Expression*>>> body;
    int globalPosCase;
public:
    CaseOf(){
        c1=++c;
        value=Token();};
    // ИСПРАВЛЕНО: передаем vector по константной ссылке
    CaseOf(int pos, const vector<Token>& list){
        c1=++c;
        doSwitch(pos,list);
    }
    Token getVal(){return value;}
    vector<std::pair<vector<Token>,vector<Expression*>>> getBody(){return body;}
    // ИСПРАВЛЕНО: передаем vector по константной ссылке
    void doSwitch(int pos, const vector<Token>& list){
        globalPosCase=pos;
        
        // Проверка границ перед началом обработки
        if (globalPosCase >= static_cast<int>(list.size())) {
            std::cerr << "[ERROR] [CaseOf] doSwitch: pos out of bounds, pos=" 
                      << globalPosCase << ", list.size()=" << list.size() << std::endl;
            return;
        }
        
        globalPosCase++;
        if (globalPosCase >= static_cast<int>(list.size())) {
            std::cerr << "[ERROR] [CaseOf] doSwitch: out of bounds after first increment, pos=" 
                      << globalPosCase << ", list.size()=" << list.size() << std::endl;
            return;
        }
        value=list[globalPosCase];
        if(value.getType()=="VALUESTRING"||value.getType()=="VALUEREAL"){
            throw std::runtime_error{"invalid argument for switch"};}
        globalPosCase++;
        if (globalPosCase >= static_cast<int>(list.size())) {
            std::cerr << "[ERROR] [CaseOf] doSwitch: out of bounds, pos=" 
                      << globalPosCase << ", list.size()=" << list.size() << std::endl;
            return;
        }
        globalPosCase++;
        if (globalPosCase >= static_cast<int>(list.size())) {
            std::cerr << "[ERROR] [CaseOf] doSwitch: out of bounds, pos=" 
                      << globalPosCase << ", list.size()=" << list.size() << std::endl;
            return;
        }
        globalPosCase++;
        if (globalPosCase >= static_cast<int>(list.size())) {
            std::cerr << "[ERROR] [CaseOf] doSwitch: out of bounds before main loop, pos=" 
                      << globalPosCase << ", list.size()=" << list.size() << std::endl;
            return;
        }
        
        // Пропускаем BEGIN после "of", если он есть (неправильный синтаксис, но обрабатываем)
        if (globalPosCase < list.size() && list[globalPosCase].getType() == "BEGIN") {
            globalPosCase++;
            if (globalPosCase >= static_cast<int>(list.size())) {
                std::cerr << "[ERROR] [CaseOf] doSwitch: out of bounds after BEGIN, pos=" 
                          << globalPosCase << ", list.size()=" << list.size() << std::endl;
                return;
            }
        }
        
        vector<Expression*>expressionList; //для хранения действий при определённом кейсе
        vector<Token>localList;
        vector<Token>argueList;//условия вхождения в кейс
        
        int endCycleSearchLimit = globalPosCase + 500; // Максимум 500 токенов в case
        if (endCycleSearchLimit > static_cast<int>(list.size())) {
            endCycleSearchLimit = static_cast<int>(list.size());
        }
        
        while(globalPosCase < list.size() && 
              globalPosCase < endCycleSearchLimit &&
              list[globalPosCase].getType()!="ENDofCycle")
        {
            int colonSearchLimit = globalPosCase + 50;
            if (colonSearchLimit > static_cast<int>(list.size())) {
                colonSearchLimit = static_cast<int>(list.size());
            }
            while(globalPosCase < list.size() && 
                  globalPosCase < colonSearchLimit &&
                  list[globalPosCase].getType()!="COLON")
            {
                if(list[globalPosCase].getType()!="COMMA")
                {
                    argueList.push_back(list[globalPosCase]);
                }
                globalPosCase++;
            }
            if (globalPosCase >= list.size() || globalPosCase >= colonSearchLimit) {
                std::cerr << "[ERROR] [CaseOf] doSwitch: COLON not found, pos=" 
                          << globalPosCase << ", list.size()=" << list.size() << std::endl;
                return;
            }
            globalPosCase++;
            if (globalPosCase >= static_cast<int>(list.size())) {
                std::cerr << "[ERROR] [CaseOf] doSwitch: out of bounds after COLON, pos=" 
                          << globalPosCase << ", list.size()=" << list.size() << std::endl;
                return;
            }
            globalPosCase++;
            if (globalPosCase >= static_cast<int>(list.size())) {
                std::cerr << "[ERROR] [CaseOf] doSwitch: out of bounds before body loop, pos=" 
                          << globalPosCase << ", list.size()=" << list.size() << std::endl;
                return;
            }
            
            int bodyEndSearchLimit = globalPosCase + 200;
            if (bodyEndSearchLimit > static_cast<int>(list.size())) {
                bodyEndSearchLimit = static_cast<int>(list.size());
            }
            
            while(globalPosCase < list.size() && 
                  globalPosCase < bodyEndSearchLimit &&
                  list[globalPosCase].getType()!="ENDofCycle"){
                if(((list[globalPosCase].getType()=="CONDITION"))||
                   (list[globalPosCase].getType()=="CYCLEFOR")||
                   (list[globalPosCase].getType()=="CYCLEWHILE")||
                   (list[globalPosCase].getType()=="CYCLEDOWHILE"))
                {
                    if (globalPosCase >= static_cast<int>(list.size())) {
                        std::cerr << "[ERROR] [CaseOf] doSwitch: out of bounds before ConditionExpression, pos=" 
                                  << globalPosCase << ", list.size()=" << list.size() << std::endl;
                        break;
                    }
                    try {
                        auto* cx =new ConditionExpression(globalPosCase,list);
                        int newPos = ConditionExpression::getGlobalPos();
                        if (newPos > static_cast<int>(list.size())) {
                            std::cerr << "[ERROR] [CaseOf] doSwitch: GlobalPos out of bounds, new pos=" 
                                      << newPos << ", list.size()=" << list.size() << std::endl;
                            globalPosCase = static_cast<int>(list.size());
                            break;
                        }
                        globalPosCase = newPos;
                        expressionList.push_back(cx);
                    } catch (const std::bad_alloc& e) {
                        std::cerr << "[ERROR] [CaseOf] doSwitch: bad_alloc creating ConditionExpression: " 
                                  << e.what() << std::endl;
                        throw;
                    }
                }
                else{
                    int semicolonSearchLimit = globalPosCase + 50;
                    if (semicolonSearchLimit > static_cast<int>(list.size())) {
                        semicolonSearchLimit = static_cast<int>(list.size());
                    }
                    while(globalPosCase < list.size() && 
                          globalPosCase < semicolonSearchLimit &&
                          list[globalPosCase].getType()!="SEMICOLON"){
                        localList.push_back(list[globalPosCase]);
                        globalPosCase++;
                    }
                    if (globalPosCase >= list.size() || globalPosCase >= semicolonSearchLimit) {
                        std::cerr << "[ERROR] [CaseOf] doSwitch: SEMICOLON not found, pos=" 
                                  << globalPosCase << ", list.size()=" << list.size() << std::endl;
                        // Сохраняем localList перед возвратом
                        if (!localList.empty()) {
                            try {
                                auto* rx = new StatementExpression(std::move(localList));
                                expressionList.push_back(rx);
                            } catch (...) {
                                // Игнорируем ошибки
                            }
                        }
                        return;
                    }
                    // ИСПРАВЛЕНО: используем move для передачи localList
                    auto* rx= new StatementExpression(std::move(localList));
                    expressionList.push_back(rx);
                    localList.clear();
                    globalPosCase++;
                    if (globalPosCase >= static_cast<int>(list.size())) {
                        std::cerr << "[ERROR] [CaseOf] doSwitch: out of bounds after SEMICOLON, pos=" 
                                  << globalPosCase << ", list.size()=" << list.size() << std::endl;
                        break;
                    }
                }
            }
            if (globalPosCase >= list.size() || globalPosCase >= bodyEndSearchLimit) {
                std::cerr << "[ERROR] [CaseOf] doSwitch: ENDofCycle not found in case body, pos=" 
                          << globalPosCase << ", list.size()=" << list.size() << std::endl;
                return;
            }
            
            std::pair t{argueList,expressionList};
            body.push_back(t);
            expressionList.clear();
            argueList.clear();
            globalPosCase++;
            if (globalPosCase >= static_cast<int>(list.size())) {
                std::cerr << "[ERROR] [CaseOf] doSwitch: out of bounds after case body, pos=" 
                          << globalPosCase << ", list.size()=" << list.size() << std::endl;
                return;
            }
        }
        if (globalPosCase >= list.size() || globalPosCase >= endCycleSearchLimit) {
            std::cerr << "[ERROR] [CaseOf] doSwitch: ENDofCycle not found for case statement, pos=" 
                      << globalPosCase << ", list.size()=" << list.size() << std::endl;
            return;
        }
        globalPosCase++;
    }
    void print(int tab)override{
        for(int j=0;j<tab;j++){
            cout<<"   ";
        }
        std::cout<<"CaseOf "<<c1<<" of "<<value.getValue()<<" =";
        std::cout<<endl;

        if(!body.empty())
        {
            ++tab;
            for(auto token2:body)
            {
                for(int j=0;j<tab;j++){
                    std::cout<<"   ";
                }
                std::cout<<"in case:";
                for(auto token3:token2.first){
                    std::cout<<" "<< token3.getValue();
                }
                std::cout<<std::endl;
                for(int j=0;j<tab;j++){
                    std::cout<<"   ";
                }
                std::cout<<"corresponds to:";
                for(auto token3:token2.second){
                    std::cout<<"   ";
                    token3->print(tab);
                }
                std::cout<<std::endl;
            }
        }
        --tab;
    }
    int getPos(){return globalPosCase;}
};


#endif //CASEOF_H
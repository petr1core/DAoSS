//
// Created by shuri on 26.04.2024.
//
#ifndef CONDITIONEXPRESSION_H
#define CONDITIONEXPRESSION_H
#include <vector>
#include <iostream>
#include "Expression.h"
#include "stdexcept"
#include "../Scripts/Token.h"
static int y=0;
static int posofEndofIf=0; //только для вложенных случаев нужен
static const int MAX_RECURSION_DEPTH = 50; // Максимальная глубина вложенности для защиты от переполнения памяти
class ConditionExpression: public Expression{
private:
    int y1;
    std::vector<Expression*> expressionList;
    std::vector<Token> condition;
    std::vector<Token> localList;
    // Используем индексы вместо копирования токенов для экономии памяти
    // std::vector<size_t> conditionIndices; // Индексы токенов в исходном списке
public:
    ConditionExpression(const ConditionExpression& other) = default;
    static int getGlobalPos(){return posofEndofIf;}
    ConditionExpression() {
        y1=++y;
    }
    // ИСПРАВЛЕНО: передаем vector по константной ссылке, чтобы избежать копирования
    // Это критично для больших кодов с вложенными конструкциями
    ConditionExpression(int pos, const vector<Token>& list){
        y1=++y;
        
        // Защита от слишком глубокой рекурсии
        if (y1 > MAX_RECURSION_DEPTH) {
            std::cerr << "[ERROR] [ConditionExpression] Maximum recursion depth exceeded: y1=" << y1 
                      << ", pos=" << pos << std::endl;
            throw std::runtime_error("Maximum recursion depth exceeded in ConditionExpression");
        }
        
        // УБРАНО резервирование: при глубокой вложенности резервирование может вызвать bad_alloc,
        // так как каждый объект резервирует память, которая может не использоваться.
        // Векторы будут расти по мере необходимости, что более эффективно по памяти.
        
        // Логируем перед началом обработки для отладки
        if (y1 >= 10 && y1 <= 15) {
            std::cout << "[ConditionExpression] Constructor: y1=" << y1 
                      << ", pos=" << pos << ", list.size()=" << list.size() << std::endl;
        }
        
        try {
            doCondition(pos,list);
            if (y1 >= 10 && y1 <= 15) {
                std::cout << "[ConditionExpression] Constructor completed: y1=" << y1 
                          << ", condition.size()=" << condition.size()
                          << ", expressionList.size()=" << expressionList.size() << std::endl;
            }
        } catch (const std::bad_alloc& e) {
            std::cerr << "[ERROR] [ConditionExpression] bad_alloc in constructor at pos=" << pos 
                      << ", y1=" << y1 << ", condition.size()=" << condition.size()
                      << ", expressionList.size()=" << expressionList.size() << ": " << e.what() << std::endl;
            std::cerr << "[ERROR] Memory info: condition.capacity()=" << condition.capacity()
                      << ", expressionList.capacity()=" << expressionList.capacity() << std::endl;
            throw;
        }
    }

    vector<Token> getCondition(){return condition;}
    void setCondition(vector<Token> con){this->condition=con;}
    std::pair<vector<Token>, vector<Expression*>> getBody() {
        std::pair<vector<Token>, vector<Expression*>> condAndList(condition, expressionList);
        return condAndList;
    }
    // Add method to add body expressions
    void addBodyExpression(Expression* expr) {
        expressionList.push_back(expr);
    }
    // ИСПРАВЛЕНО: передаем vector по константной ссылке
    void doCondition(int pos, const vector<Token>& list){
        posofEndofIf=pos;
        
        // Защита от бесконечной рекурсии и проверка границ
        if (posofEndofIf >= list.size()) {
            std::cerr << "[ERROR] [ConditionExpression] doCondition: pos out of bounds, pos=" 
                      << posofEndofIf << ", list.size()=" << list.size() << ", y1=" << y1 << std::endl;
            return;
        }
        
        if (y1 <= 15 || y1 % 5 == 0) {
            std::cout << "[ConditionExpression] doCondition: y1=" << y1 
                      << ", pos=" << pos << ", list.size()=" << list.size() << std::endl;
        }
        
        // КРИТИЧНО: Проверяем границы ПЕРЕД каждым обращением к массиву
        if (posofEndofIf >= static_cast<int>(list.size())) {
            std::cerr << "[ERROR] [ConditionExpression] doCondition: posofEndofIf out of bounds before CONDITION check: "
                      << "posofEndofIf=" << posofEndofIf << ", list.size()=" << list.size() << ", y1=" << y1 << std::endl;
            return;
        }
        
        if(list[posofEndofIf].getType()=="CONDITION")
        {
            // Защита от бесконечного цикла при поиске BEGIN
            int beginSearchLimit = posofEndofIf + 20;
            while(posofEndofIf < list.size() && 
                  list[posofEndofIf].getType()!="BEGIN" && 
                  posofEndofIf < beginSearchLimit)
            {
                condition.push_back(list[posofEndofIf]);
                posofEndofIf++;
            }
            if (posofEndofIf >= beginSearchLimit || posofEndofIf >= list.size()) {
                std::cerr << "[ERROR] [ConditionExpression] BEGIN not found, y1=" << y1 
                          << ", posofEndofIf=" << posofEndofIf << ", list.size()=" << list.size() << std::endl;
                // Устанавливаем posofEndofIf в безопасное значение перед возвратом
                if (posofEndofIf >= static_cast<int>(list.size())) {
                    posofEndofIf = static_cast<int>(list.size()) - 1; // Последний валидный индекс
                }
                return;
            }
            posofEndofIf++;
            
            // Защита от бесконечного цикла при поиске ENDofIF
            int endIfSearchLimit = posofEndofIf + 200; // Максимум 200 токенов в теле if
            if (endIfSearchLimit > static_cast<int>(list.size())) {
                endIfSearchLimit = static_cast<int>(list.size());
            }
            while(posofEndofIf < list.size() && 
                  list[posofEndofIf].getType()!="ENDofIF" && 
                  posofEndofIf < endIfSearchLimit){ //пока не дойдём до конца тела текущего if
                if(((list[posofEndofIf].getType()=="CONDITION"))|| //если хоть какую-то в нем вложенность находим
                   (list[posofEndofIf].getType()=="CYCLEFOR")|| // хоть вложенное условие, хоть вложенный цикл, то создаём новый объект
                   (list[posofEndofIf].getType()=="CYCLEWHILE")|| // не забываем про static переменную, она указывает новое место где мы окажемся
                   (list[posofEndofIf].getType()=="CYCLEDOWHILE")||
                   (list[posofEndofIf].getType()=="UNCONDITION"))//поднявшись обратно наверх от вложенного объекта
                {
                    // КРИТИЧНО: Проверяем границы перед созданием нового объекта
                    if (posofEndofIf >= list.size()) {
                        std::cerr << "[ERROR] [ConditionExpression] Out of bounds before creating nested ConditionExpression #1 "
                                  << "at pos=" << posofEndofIf << ", list.size()=" << list.size() 
                                  << ", parent y1=" << y1 << std::endl;
                        return; // Выходим из цикла, чтобы не создавать объект с невалидной позицией
                    }
                    
                    try {
                        if (y1 <= 15 || y1 % 5 == 0) {
                            std::cout << "[ConditionExpression] Creating nested ConditionExpression #1 at pos=" 
                                      << posofEndofIf << ", parent y1=" << y1 << ", list.size()=" << list.size() << std::endl;
                        }
                        // КРИТИЧНО: Дополнительная проверка границ перед new
                        if (posofEndofIf >= static_cast<int>(list.size())) {
                            std::cerr << "[ERROR] [ConditionExpression] Cannot create nested ConditionExpression #1 - pos out of bounds: "
                                      << "pos=" << posofEndofIf << ", list.size()=" << list.size() << ", parent y1=" << y1 << std::endl;
                            return; // Выходим из цикла
                        }
                        ConditionExpression* cx = new ConditionExpression(posofEndofIf,list);
                        int newPos = cx->getGlobalPos();
                        // Проверяем, что новая позиция не вышла за границы
                        if (newPos > static_cast<int>(list.size())) {
                            std::cerr << "[ERROR] [ConditionExpression] GlobalPos out of bounds after creating nested ConditionExpression #1 "
                                      << "new pos=" << newPos << ", list.size()=" << list.size() << ", parent y1=" << y1 << std::endl;
                            posofEndofIf = static_cast<int>(list.size()); // Устанавливаем безопасное значение
                            break; // Выходим из цикла
                        }
                        posofEndofIf = newPos;
                        expressionList.push_back(cx);
                    } catch (const std::bad_alloc& e) {
                        std::cerr << "[ERROR] [ConditionExpression] bad_alloc creating nested ConditionExpression #1 "
                                  << "at pos=" << posofEndofIf << ", list.size()=" << list.size()
                                  << ", parent y1=" << y1 << ": " << e.what() << std::endl;
                        throw;
                    }
                }
                else
                {
                    // Защита от выхода за границы при поиске SEMICOLON
                    int semicolonSearchLimit = posofEndofIf + 50;
                    if (semicolonSearchLimit > static_cast<int>(list.size())) {
                        semicolonSearchLimit = static_cast<int>(list.size());
                    }
                    while(posofEndofIf < list.size() && 
                          posofEndofIf < semicolonSearchLimit &&
                          list[posofEndofIf].getType()!="SEMICOLON"){ //если вложенности нет или мы с ней уже закончили, то формируем обычные выражения
                        localList.push_back(list[posofEndofIf]);
                        posofEndofIf++;
                    }
                    if (posofEndofIf >= list.size() || posofEndofIf >= semicolonSearchLimit) {
                        std::cerr << "[ERROR] [ConditionExpression] SEMICOLON not found, pos=" 
                                  << posofEndofIf << ", list.size()=" << list.size() << ", y1=" << y1 << std::endl;
                        // Устанавливаем posofEndofIf в безопасное значение перед возвратом
                        if (posofEndofIf >= static_cast<int>(list.size())) {
                            posofEndofIf = static_cast<int>(list.size()) - 1; // Последний валидный индекс
                        }
                        // Если localList не пустой, сохраняем его как StatementExpression
                        if (!localList.empty()) {
                            try {
                                StatementExpression* rx = new StatementExpression(std::move(localList));
                                expressionList.push_back(rx);
                            } catch (...) {
                                // Игнорируем ошибки при создании StatementExpression в случае ошибки
                            }
                        }
                        return;
                    }
                    // ИСПРАВЛЕНО: используем move для передачи localList, чтобы избежать копирования
                    StatementExpression* rx= new StatementExpression(std::move(localList));
                    expressionList.push_back(rx);
                    localList.clear();
                    posofEndofIf++;
                    // КРИТИЧНО: Проверяем границы после инкремента
                    if (posofEndofIf >= static_cast<int>(list.size())) {
                        std::cerr << "[ERROR] [ConditionExpression] posofEndofIf out of bounds after semicolon: "
                                  << posofEndofIf << ", list.size()=" << list.size() << ", y1=" << y1 << std::endl;
                        break; // Выходим из цикла
                    }
                }
            }
            if (posofEndofIf >= endIfSearchLimit || posofEndofIf >= list.size()) {
                std::cerr << "[ERROR] [ConditionExpression] ENDofIF not found, y1=" << y1 
                          << ", posofEndofIf=" << posofEndofIf << ", list.size()=" << list.size() << std::endl;
                // Устанавливаем posofEndofIf в безопасное значение перед возвратом
                if (posofEndofIf >= static_cast<int>(list.size())) {
                    posofEndofIf = static_cast<int>(list.size()) - 1; // Последний валидный индекс
                }
                return;
            }
            posofEndofIf++;
            return;
            //после ENDofIF перескакивать не надо, чтобы было разделение на отдельные объекты у if и else
        }
        
        // КРИТИЧНО: Проверяем границы перед обращением к массиву
        if (posofEndofIf >= static_cast<int>(list.size())) {
            std::cerr << "[ERROR] [ConditionExpression] doCondition: posofEndofIf out of bounds before UNCONDITION check: "
                      << "posofEndofIf=" << posofEndofIf << ", list.size()=" << list.size() << ", y1=" << y1 << std::endl;
            return;
        }
        
        if(list[posofEndofIf].getType()=="UNCONDITION"){
            condition.push_back(list[posofEndofIf]);
            posofEndofIf++;
            // Проверяем границы перед циклом поиска BEGIN
            if (posofEndofIf >= static_cast<int>(list.size())) {
                std::cerr << "[ERROR] [ConditionExpression] posofEndofIf out of bounds before BEGIN search: "
                          << posofEndofIf << ", list.size()=" << list.size() << ", y1=" << y1 << std::endl;
                return;
            }
            int beginSearchLimit = posofEndofIf + 20;
            if (beginSearchLimit > static_cast<int>(list.size())) {
                beginSearchLimit = static_cast<int>(list.size());
            }
            while(posofEndofIf < list.size() && 
                  posofEndofIf < beginSearchLimit &&
                  list[posofEndofIf].getType()!="BEGIN")
            {
                posofEndofIf++;
            }
            if (posofEndofIf >= beginSearchLimit || posofEndofIf >= list.size()) {
                std::cerr << "[ERROR] [ConditionExpression] BEGIN not found in UNCONDITION, y1=" << y1 << std::endl;
                return;
            }
            posofEndofIf++;
            // Проверяем границы перед циклом поиска ENDofCycle
            if (posofEndofIf >= static_cast<int>(list.size())) {
                std::cerr << "[ERROR] [ConditionExpression] posofEndofIf out of bounds before ENDofCycle search: "
                          << posofEndofIf << ", list.size()=" << list.size() << ", y1=" << y1 << std::endl;
                return;
            }
            int endCycleSearchLimit = posofEndofIf + 200;
            if (endCycleSearchLimit > static_cast<int>(list.size())) {
                endCycleSearchLimit = static_cast<int>(list.size());
            }
            // те же шаги, что и при condition, но уже в цикле до ENDofCycle
            while(posofEndofIf < list.size() && 
                  posofEndofIf < endCycleSearchLimit &&
                  list[posofEndofIf].getType()!="ENDofCycle"){
                if(((list[posofEndofIf].getType()=="CONDITION"))||
                   (list[posofEndofIf].getType()=="CYCLEFOR")||
                   (list[posofEndofIf].getType()=="CYCLEWHILE")||
                   (list[posofEndofIf].getType()=="CYCLEDOWHILE"))
                    {
                    try {
                        if (y1 <= 10 || y1 % 5 == 0) {
                            std::cout << "[ConditionExpression] Creating nested ConditionExpression #2 at pos=" 
                                      << posofEndofIf << ", parent y1=" << y1 << std::endl;
                        }
                        ConditionExpression* cx =new ConditionExpression(posofEndofIf,list);
                        int newPos = cx->getGlobalPos();
                        if (newPos > static_cast<int>(list.size())) {
                            std::cerr << "[ERROR] [ConditionExpression] GlobalPos out of bounds: new pos=" 
                                      << newPos << ", list.size()=" << list.size() << ", parent y1=" << y1 << std::endl;
                            posofEndofIf = static_cast<int>(list.size());
                            break;
                        }
                        posofEndofIf = newPos;
                        expressionList.push_back(cx);
                    } catch (const std::bad_alloc& e) {
                        std::cerr << "[ERROR] [ConditionExpression] bad_alloc creating nested ConditionExpression #2 "
                                  << "at pos=" << posofEndofIf << ", parent y1=" << y1 << ": " << e.what() << std::endl;
                        throw;
                    }
                    }
               else{
                   // Защита от выхода за границы при поиске SEMICOLON
                   int semicolonSearchLimit = posofEndofIf + 50;
                   if (semicolonSearchLimit > static_cast<int>(list.size())) {
                       semicolonSearchLimit = static_cast<int>(list.size());
                   }
                   while(posofEndofIf < list.size() && 
                         posofEndofIf < semicolonSearchLimit &&
                         list[posofEndofIf].getType()!="SEMICOLON")
                   {
                       localList.push_back(list[posofEndofIf]);
                       posofEndofIf++;
                   }
                   if (posofEndofIf >= list.size() || posofEndofIf >= semicolonSearchLimit) {
                       std::cerr << "[ERROR] [ConditionExpression] SEMICOLON not found in UNCONDITION, pos=" 
                                 << posofEndofIf << ", list.size()=" << list.size() << ", y1=" << y1 << std::endl;
                       return;
                   }
                   // ИСПРАВЛЕНО: используем move для передачи localList
                   StatementExpression* rx= new StatementExpression(std::move(localList));
                   expressionList.push_back(rx);
                   localList.clear();
                   posofEndofIf++;
                   if (posofEndofIf >= static_cast<int>(list.size())) {
                       std::cerr << "[ERROR] [ConditionExpression] posofEndofIf out of bounds after semicolon in UNCONDITION: "
                                 << posofEndofIf << ", list.size()=" << list.size() << ", y1=" << y1 << std::endl;
                       break;
                   }
               }
            }
            if (posofEndofIf >= static_cast<int>(list.size())) {
                std::cerr << "[ERROR] [ConditionExpression] posofEndofIf out of bounds before increment after UNCONDITION: "
                          << posofEndofIf << ", list.size()=" << list.size() << ", y1=" << y1 << std::endl;
                return;
            }
            posofEndofIf++;
            return;
        }
        
        // КРИТИЧНО: Проверяем границы перед проверкой CYCLEFOR/CYCLEWHILE
        if (posofEndofIf >= static_cast<int>(list.size())) {
            std::cerr << "[ERROR] [ConditionExpression] doCondition: posofEndofIf out of bounds before CYCLEFOR/CYCLEWHILE check: "
                      << "posofEndofIf=" << posofEndofIf << ", list.size()=" << list.size() << ", y1=" << y1 << std::endl;
            return;
        }
        
        if((list[posofEndofIf].getType()=="CYCLEFOR")||(list[posofEndofIf].getType()=="CYCLEWHILE")){
            int beginSearchLimit = posofEndofIf + 20;
            if (beginSearchLimit > static_cast<int>(list.size())) {
                beginSearchLimit = static_cast<int>(list.size());
            }
            while(posofEndofIf < list.size() && 
                  posofEndofIf < beginSearchLimit &&
                  list[posofEndofIf].getType()!="BEGIN"){
                condition.push_back(list[posofEndofIf]);
                posofEndofIf++;  
            }
            if (posofEndofIf >= beginSearchLimit || posofEndofIf >= list.size()) {
                std::cerr << "[ERROR] [ConditionExpression] BEGIN not found in CYCLEFOR/CYCLEWHILE, y1=" << y1 << std::endl;
                return;
            }
            posofEndofIf++;
            if (posofEndofIf >= static_cast<int>(list.size())) {
                std::cerr << "[ERROR] [ConditionExpression] posofEndofIf out of bounds before ENDofCycle search in CYCLEFOR/CYCLEWHILE: "
                          << posofEndofIf << ", list.size()=" << list.size() << ", y1=" << y1 << std::endl;
                return;
            }
            int endCycleSearchLimit = posofEndofIf + 200;
            if (endCycleSearchLimit > static_cast<int>(list.size())) {
                endCycleSearchLimit = static_cast<int>(list.size());
            }
            // те же шаги, что и при uncondition
            while(posofEndofIf < list.size() && 
                  posofEndofIf < endCycleSearchLimit &&
                  list[posofEndofIf].getType()!="ENDofCycle"){
                if((list[posofEndofIf].getType()=="CONDITION")||
                   (list[posofEndofIf].getType()=="CYCLEFOR")||
                   (list[posofEndofIf].getType()=="CYCLEWHILE")||
                   (list[posofEndofIf].getType()=="CYCLEDOWHILE"))                    {
                    try {
                        if (y1 <= 10 || y1 % 5 == 0) {
                            std::cout << "[ConditionExpression] Creating nested ConditionExpression #3 at pos=" 
                                      << posofEndofIf << ", parent y1=" << y1 << std::endl;
                        }
                        ConditionExpression* cx= new ConditionExpression(posofEndofIf,list);
                        int newPos = cx->getGlobalPos();
                        if (newPos > static_cast<int>(list.size())) {
                            std::cerr << "[ERROR] [ConditionExpression] GlobalPos out of bounds: new pos=" 
                                      << newPos << ", list.size()=" << list.size() << ", parent y1=" << y1 << std::endl;
                            posofEndofIf = static_cast<int>(list.size());
                            break;
                        }
                        posofEndofIf = newPos;
                        expressionList.push_back(cx);
                    } catch (const std::bad_alloc& e) {
                        std::cerr << "[ERROR] [ConditionExpression] bad_alloc creating nested ConditionExpression #3 "
                                  << "at pos=" << posofEndofIf << ", parent y1=" << y1 << ": " << e.what() << std::endl;
                        throw;
                    }
                    }
               else{
                   // Защита от выхода за границы при поиске SEMICOLON
                   int semicolonSearchLimit = posofEndofIf + 50;
                   if (semicolonSearchLimit > static_cast<int>(list.size())) {
                       semicolonSearchLimit = static_cast<int>(list.size());
                   }
                   while(posofEndofIf < list.size() && 
                         posofEndofIf < semicolonSearchLimit &&
                         list[posofEndofIf].getType()!="SEMICOLON"){
                       localList.push_back(list[posofEndofIf]);
                       posofEndofIf++;
                   }
                   if (posofEndofIf >= list.size() || posofEndofIf >= semicolonSearchLimit) {
                       std::cerr << "[ERROR] [ConditionExpression] SEMICOLON not found in CYCLEFOR/CYCLEWHILE, pos=" 
                                 << posofEndofIf << ", list.size()=" << list.size() << ", y1=" << y1 << std::endl;
                       // Устанавливаем posofEndofIf в безопасное значение перед возвратом
                       if (posofEndofIf >= static_cast<int>(list.size())) {
                           posofEndofIf = static_cast<int>(list.size()) - 1; // Последний валидный индекс
                       }
                       // Если localList не пустой, сохраняем его как StatementExpression
                       if (!localList.empty()) {
                           try {
                               StatementExpression* rx = new StatementExpression(std::move(localList));
                               expressionList.push_back(rx);
                           } catch (...) {
                               // Игнорируем ошибки при создании StatementExpression в случае ошибки
                           }
                       }
                       return;
                   }
                   // ИСПРАВЛЕНО: используем move для передачи localList
                   StatementExpression* rx=new StatementExpression(std::move(localList));
                   expressionList.push_back(rx);
                   localList.clear();
                   posofEndofIf++;
                   if (posofEndofIf >= static_cast<int>(list.size())) {
                       std::cerr << "[ERROR] [ConditionExpression] posofEndofIf out of bounds after semicolon in CYCLEFOR/CYCLEWHILE: "
                                 << posofEndofIf << ", list.size()=" << list.size() << ", y1=" << y1 << std::endl;
                       break;
                   }
               }
            }
            if (posofEndofIf >= static_cast<int>(list.size())) {
                std::cerr << "[ERROR] [ConditionExpression] posofEndofIf out of bounds before increment after CYCLEFOR/CYCLEWHILE: "
                          << posofEndofIf << ", list.size()=" << list.size() << ", y1=" << y1 << std::endl;
                return;
            }
            posofEndofIf++;
            return;

       }
       
       // КРИТИЧНО: Проверяем границы перед проверкой CYCLEDOWHILE
       if (posofEndofIf >= static_cast<int>(list.size())) {
           std::cerr << "[ERROR] [ConditionExpression] doCondition: posofEndofIf out of bounds before CYCLEDOWHILE check: "
                     << "posofEndofIf=" << posofEndofIf << ", list.size()=" << list.size() << ", y1=" << y1 << std::endl;
           return;
       }
       
        if(list[posofEndofIf].getType()=="CYCLEDOWHILE"){
            posofEndofIf++;
            if (posofEndofIf >= static_cast<int>(list.size())) {
                std::cerr << "[ERROR] [ConditionExpression] posofEndofIf out of bounds after first increment in CYCLEDOWHILE: "
                          << posofEndofIf << ", list.size()=" << list.size() << ", y1=" << y1 << std::endl;
                return;
            }
            // Пропускаем BEGIN после repeat, если он есть (неправильный синтаксис, но обрабатываем)
            if (posofEndofIf < list.size() && list[posofEndofIf].getType() == "BEGIN") {
                posofEndofIf++;
                if (posofEndofIf >= static_cast<int>(list.size())) {
                    std::cerr << "[ERROR] [ConditionExpression] posofEndofIf out of bounds after BEGIN in CYCLEDOWHILE: "
                              << posofEndofIf << ", list.size()=" << list.size() << ", y1=" << y1 << std::endl;
                    return;
                }
            }
            if (posofEndofIf >= static_cast<int>(list.size())) {
                std::cerr << "[ERROR] [ConditionExpression] posofEndofIf out of bounds before ENDofCycle search in CYCLEDOWHILE: "
                          << posofEndofIf << ", list.size()=" << list.size() << ", y1=" << y1 << std::endl;
                return;
            }
            int endCycleSearchLimit = posofEndofIf + 200;
            if (endCycleSearchLimit > static_cast<int>(list.size())) {
                endCycleSearchLimit = static_cast<int>(list.size());
            }
            // те же шаги, что и при uncondition
            // В repeat-until тело заканчивается на UNTIL, а не на ENDofCycle
            while(posofEndofIf < list.size() && 
                  posofEndofIf < endCycleSearchLimit &&
                  list[posofEndofIf].getType()!="ENDofCycle" &&
                  list[posofEndofIf].getType()!="UNTIL"){  // Добавлена проверка на UNTIL для repeat-until
                if((list[posofEndofIf].getType()=="CONDITION")||
                   (list[posofEndofIf].getType()=="CYCLEFOR")||
                   (list[posofEndofIf].getType()=="CYCLEWHILE")||
                   (list[posofEndofIf].getType()=="CYCLEDOWHILE"))                    {
                    try {
                        if (y1 <= 10 || y1 % 5 == 0) {
                            std::cout << "[ConditionExpression] Creating nested ConditionExpression #3 at pos=" 
                                      << posofEndofIf << ", parent y1=" << y1 << std::endl;
                        }
                        ConditionExpression* cx= new ConditionExpression(posofEndofIf,list);
                        int newPos = cx->getGlobalPos();
                        if (newPos > static_cast<int>(list.size())) {
                            std::cerr << "[ERROR] [ConditionExpression] GlobalPos out of bounds: new pos=" 
                                      << newPos << ", list.size()=" << list.size() << ", parent y1=" << y1 << std::endl;
                            posofEndofIf = static_cast<int>(list.size());
                            break;
                        }
                        posofEndofIf = newPos;
                        expressionList.push_back(cx);
                    } catch (const std::bad_alloc& e) {
                        std::cerr << "[ERROR] [ConditionExpression] bad_alloc creating nested ConditionExpression #3 "
                                  << "at pos=" << posofEndofIf << ", parent y1=" << y1 << ": " << e.what() << std::endl;
                        throw;
                    }
                    }
               else{
                   // В repeat-until тело заканчивается на UNTIL, а не на ENDofCycle
                   // Поэтому проверяем UNTIL перед поиском SEMICOLON
                   if (posofEndofIf < list.size() && list[posofEndofIf].getType() == "UNTIL") {
                       // Встретили UNTIL - это конец тела цикла repeat-until
                       // Вызываем break, чтобы выйти из цикла поиска ENDofCycle
                       break;
                   }
                   
                   // Защита от выхода за границы при поиске SEMICOLON
                   int semicolonSearchLimit = posofEndofIf + 50;
                   if (semicolonSearchLimit > static_cast<int>(list.size())) {
                       semicolonSearchLimit = static_cast<int>(list.size());
                   }
                   while(posofEndofIf < list.size() && 
                         posofEndofIf < semicolonSearchLimit &&
                         list[posofEndofIf].getType()!="SEMICOLON" &&
                         list[posofEndofIf].getType()!="UNTIL"){  // Добавлена проверка на UNTIL
                       localList.push_back(list[posofEndofIf]);
                       posofEndofIf++;
                   }
                   // Если встретили UNTIL, это конец тела цикла
                   if (posofEndofIf < list.size() && list[posofEndofIf].getType() == "UNTIL") {
                       // Если localList не пустой, сохраняем его как StatementExpression
                       if (!localList.empty()) {
                           try {
                               StatementExpression* rx = new StatementExpression(std::move(localList));
                               expressionList.push_back(rx);
                               localList.clear();
                           } catch (...) {
                               // Игнорируем ошибки при создании StatementExpression
                           }
                       }
                       // Вызываем break, чтобы выйти из цикла поиска ENDofCycle
                       break;
                   }
                   if (posofEndofIf >= list.size() || posofEndofIf >= semicolonSearchLimit) {
                       std::cerr << "[ERROR] [ConditionExpression] SEMICOLON not found in CYCLEDOWHILE, pos=" 
                                 << posofEndofIf << ", list.size()=" << list.size() << ", y1=" << y1 << std::endl;
                       // Устанавливаем posofEndofIf в безопасное значение перед возвратом
                       if (posofEndofIf >= static_cast<int>(list.size())) {
                           posofEndofIf = static_cast<int>(list.size()) - 1; // Последний валидный индекс
                       }
                       // Если localList не пустой, сохраняем его как StatementExpression
                       if (!localList.empty()) {
                           try {
                               StatementExpression* rx = new StatementExpression(std::move(localList));
                               expressionList.push_back(rx);
                           } catch (...) {
                               // Игнорируем ошибки при создании StatementExpression в случае ошибки
                           }
                       }
                       return;
                   }
                   // ИСПРАВЛЕНО: используем move для передачи localList
                   StatementExpression* rx=new StatementExpression(std::move(localList));
                   expressionList.push_back(rx);
                   localList.clear();
                   posofEndofIf++;
                   if (posofEndofIf >= static_cast<int>(list.size())) {
                       std::cerr << "[ERROR] [ConditionExpression] posofEndofIf out of bounds after semicolon in CYCLEDOWHILE: "
                                 << posofEndofIf << ", list.size()=" << list.size() << ", y1=" << y1 << std::endl;
                       break;
                   }
               }
            }
            // Проверяем, встретили ли мы UNTIL (конец тела repeat-until)
            if (posofEndofIf < list.size() && list[posofEndofIf].getType() == "UNTIL") {
                // Для repeat-until: until условие; уже включено в condition при обработке
                // Пропускаем until и идём дальше до SEMICOLON после условия
                while (posofEndofIf < list.size() && list[posofEndofIf].getType() != "SEMICOLON") {
                    condition.push_back(list[posofEndofIf]);
                    posofEndofIf++;
                }
                if (posofEndofIf < list.size() && list[posofEndofIf].getType() == "SEMICOLON") {
                    posofEndofIf++;  // Пропускаем SEMICOLON после until условия
                }
                return;  // Завершаем обработку repeat-until
            }
            
            if (posofEndofIf >= static_cast<int>(list.size())) {
                std::cerr << "[ERROR] [ConditionExpression] posofEndofIf out of bounds before increment after CYCLEDOWHILE: "
                          << posofEndofIf << ", list.size()=" << list.size() << ", y1=" << y1 << std::endl;
                return;
            }
            posofEndofIf++;
            if (posofEndofIf >= static_cast<int>(list.size())) {
                std::cerr << "[ERROR] [ConditionExpression] posofEndofIf out of bounds before SEMICOLON search in CYCLEDOWHILE: "
                          << posofEndofIf << ", list.size()=" << list.size() << ", y1=" << y1 << std::endl;
                return;
            }
            int semicolonSearchLimit = posofEndofIf + 50;
            if (semicolonSearchLimit > static_cast<int>(list.size())) {
                semicolonSearchLimit = static_cast<int>(list.size());
            }
            while(posofEndofIf < list.size() && 
                  posofEndofIf < semicolonSearchLimit &&
                  list[posofEndofIf].getType()!="SEMICOLON")
            {
                condition.push_back(list[posofEndofIf]);
                posofEndofIf++;
            }
            if (posofEndofIf >= list.size() || posofEndofIf >= semicolonSearchLimit) {
                std::cerr << "[ERROR] [ConditionExpression] SEMICOLON not found at end of CYCLEDOWHILE, pos=" 
                          << posofEndofIf << ", list.size()=" << list.size() << ", y1=" << y1 << std::endl;
                return;
            }
            posofEndofIf++;
            return;
        }
    }
    ConditionExpression& operator=(const ConditionExpression& other) {
        this->condition = other.condition;
        this->expressionList = other.expressionList;
        return *this;
    }
    void print(int tab) override{
        for(int j=0;j<tab;j++){
            cout<<"   ";
        }
       std::cout<<"ConditionExpression "<<y1<<" = ";
       if(condition[0].getValue()!="else"){
           for(auto token:condition)
           {
               std::cout<<token.getValue()<<" ";
           }
           std::cout<<endl;
       }
       else{std::cout<<"else"<<endl;}
       if(!expressionList.empty())
       {
           ++tab;
           for(auto token2:expressionList)
           {
               std::cout<<"   ";
               token2->print(tab);
           }
       }
       --tab;
    }
};

#endif //CONDITIONEXPRESSION_H
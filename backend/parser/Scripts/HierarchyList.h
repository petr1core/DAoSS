//
// Created by shuri on 23.04.2024.
//
#ifndef HIERARCHYLIST_H
#define HIERARCHYLIST_H
#include <iostream>
#include "Postfix.h"
#include "Token.h"
#include "Lexer.h"
#include "SearchTreeTable.h"
using namespace std;
template <class Priority, class Value>
class HierarchyList {
private:
    struct Node {
        Node* nextDescription;
        Value value;
        Node(Value v) {
            nextDescription=nullptr;
            value = v;
        }
        Node( Node* d, Value v) {
            nextDescription = d;
            value = v;
        }
        void toSolve(){
            if(this==nullptr)return;
            if (auto statementExpr = dynamic_cast<StatementExpression*>(value)) {
                calc.ChangeEquation(*statementExpr); // Вызов метода для StatementExpression
            } else if (auto conditionExpr = dynamic_cast<ConditionExpression*>(value)) {
                calc.ChangeEquation(*conditionExpr); // Вызов метода для ConditionExpression
            }
            this->nextDescription->toSolve();
        }
    };
    struct Chapter{
        Priority chapter;
        Chapter* nextChapter;
        Node* description;
        Chapter(Priority p){
            chapter=p;
            nextChapter=nullptr;
            description=nullptr;
        }
        void toSolve(){
            if (this==nullptr) return;
            this->description->toSolve();
            this->nextChapter->toSolve();
        }
        void print(Node* node){
            if(node==nullptr) return;
            cout<<node->value;
            print(node->nextDescription);
        }
    };
    Chapter* root;
    static TPostfixCalc calc;
public:
    HierarchyList() {
        root=new Chapter("Const");
        root->nextChapter=new Chapter("Var");
        (root->nextChapter)->nextChapter=new Chapter("Body");
    }
    void toAddNext(Value val, Priority pr) {
        Node* new_node = new Node(val);
        Chapter* currentChapter = root;
        while (currentChapter != nullptr && currentChapter->chapter != pr) {
            currentChapter = currentChapter->nextChapter;
        }

        // Проверяем, что глава найдена
        if (currentChapter != nullptr) {
            // Если у главы нет описания, устанавливаем новый узел как первое описание
            if (currentChapter->description == nullptr) {
                currentChapter->description = new_node;
            } else {
                // Иначе находим последний узел описания и добавляем новый узел
                Node* currentNode = currentChapter->description;
                while (currentNode->nextDescription != nullptr) {
                    currentNode = currentNode->nextDescription;
                }
                currentNode->nextDescription = new_node;
            }
        } else {
            //Обработка случая, когда глава не найдена
            //никогда не будет действовать
        }
    }
    Chapter* getRoot(){ return root;}
    void toSolve(){
        root->toSolve();
    }
    void print(Chapter* chapter)
    {
        if (chapter == nullptr) return;
        cout<<"In Chapter "<<chapter->chapter<<": "<<endl;
        chapter->print(chapter->description);
    }
};

#endif //HIERARCHYLIST_H

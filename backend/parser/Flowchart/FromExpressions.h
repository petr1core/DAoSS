#ifndef FLOWCHART_FROM_EXPRESSIONS_H
#define FLOWCHART_FROM_EXPRESSIONS_H

#include <string>
#include <sstream>
#include <vector>
#include "../Expression/Expression.h"
#include "../Expression/StatementExpression.h"
#include "../Expression/ConditionExpression.h"
#include "../Expression/CaseOf.h"
#include "../Expression/Procedure.h"
#include "../Expression/Function.h"

static int id = 1; //id элемента блок-схемы
static int layer = 0;  //слой, глубина рекурсии

class FlowchartFromExpressions {
public:
    int tmpCondId[3] = {-1, -1, -1}; //tmp массив, преимущественно для if/else конструкций
    FlowchartFromExpressions() = default;

    std::string build(const std::vector<Expression *> &exprs) {
        layer++;
        std::ostringstream out; // Будет содержать итоговую Mermaid-разметку
        std::string last = "N" + std::to_string(id);; // Текущая позиция в графе для соединения узлов
        size_t sz = 0;
        while (sz < exprs.size()) {
            Expression *e = exprs[sz];
            //e->print(0);
            if (auto sx = dynamic_cast<StatementExpression *>(e)) {
                std::string node =
                        sx->getList()[0].getValue() == "Writeln" || sx->getList()[0].getValue() == "Write" ||
                        sx->getList()[0].getValue() == "Readln" || sx->getList()[0].getValue() == "Read"
                        ? newOut(out, ++id, tokensToLine(sx->getList()))
                        : newProcess(out, ++id, tokensToLine(sx->getList()));
                if (tmpCondId[2] != -1) {
                    link(out, "N" + to_string(tmpCondId[1]), node);
                    link(out, "N" + to_string(tmpCondId[2]), node);
                    tmpCondId[1] = -1;
                    tmpCondId[2] = -1;
                } else {
                    if (sz != 0) {
                        if (auto tmpst = dynamic_cast<ConditionExpression *>(exprs[sz - 1])) {
                            if (tmpst->getCondition().front().getValue() == "if") {
                                link(out, "N" + to_string(tmpCondId[0]), node, "false");
                                link(out, "N" + to_string(tmpCondId[1]), node);
                            }
                        }
                        else{
                            if (dynamic_cast<CaseOf *>(exprs[sz - 1]) == nullptr) {
                                link(out, last, node);
                            }
                        }
                    }
                }
                last = node;
                ++sz;
                continue;
            }
            if (auto cx = dynamic_cast<ConditionExpression *>(e)) {
                if (cx->getCondition().front().getValue() != "else" &&
                    cx->getCondition().front().getValue() != "until") {
                    std::string node;
                    if (cx->getCondition().front().getValue() == "for") {
                        node = newFor(out, ++id, tokensToLine(cx->getCondition())); // у for своя атмосфера
                    } else {
                        node = newDecision(out, ++id, tokensToLine(cx->getCondition()));  //создаём ромбик условия
                    }
                    if (tmpCondId[2] != -1) { //если предыдущий блок содержал в себе две ветки (пресловутый if/else), то соединяем
                        link(out, "N" + to_string(tmpCondId[1]), node);
                        link(out, "N" + to_string(tmpCondId[2]), node);
                        tmpCondId[1] = -1;
                        tmpCondId[2] = -1;
                    } else { //если одну, то тоже соединяем
                        if (auto tmpst = dynamic_cast<ConditionExpression *>(exprs[sz - 1])) {
                            if (tmpst->getCondition().front().getValue() == "if") {
                                link(out, "N" + to_string(tmpCondId[0]), node, "false");
                                link(out, "N" + to_string(tmpCondId[1]), node);
                            }
                        }
                        else{
                            if (dynamic_cast<CaseOf *>(exprs[sz - 1])==nullptr) {
                                link(out, last, node);
                            }
                        }
                    }
                    last = node; //курсор на новом ромбике
                }
                tmpCondId[0] = id;//запомнить начальный элемент (условие для while/for и действие для until)
                const auto body = cx->getBody().second;
                std::string f = build(body); //обрабатываем тело
                if (cx->getCondition().front().getValue() == "if") { //если тип if, то запускаем тело по ветке true
                    tmpCondId[1] = id;
                    link(out, last, f, "true");
                } else if (cx->getCondition().front().getValue() == "else") { //в противном случае по false
                    tmpCondId[2] = id;
                    link(out, last, f, "false");
                } else if (cx->getCondition().front().getValue() == "while" ||
                           cx->getCondition().front().getValue() == "for") {
                    link(out, last, f, "true");
                    link(out, "N" + to_string(id), "N" + to_string(tmpCondId[0]));
                    link(out, "N" + to_string(tmpCondId[0]), "N" + to_string(id + 1), "false");
                } else if (cx->getCondition().front().getValue() == "until") {
                    out << f;
                    std::string node = newDecision(out, ++id,
                                                   tokensToLine(cx->getCondition())); //создаём ромбик условия
                    link(out, "N" + to_string(id - 1), node);
                    last = node;
                    link(out, last, "N" + to_string(tmpCondId[0]+1), "true");
                    link(out, last, "N" + to_string(id + 1), "false");
                } else {
                    std::cout << "DIRBIRBEB" << std::endl;
                }
                ++sz; //следующий элемент
                continue;
            }
            if (auto sw = dynamic_cast<CaseOf *>(e)) {
                std::string node = newDecision(out, ++id, "case " + tokensToLine(sw->getVal()) + " of");
                if (tmpCondId[2] != -1) { //если предыдущий блок содержал в себе две ветки (пресловутый if/else), то соединяем
                    link(out, "N" + to_string(tmpCondId[1]), node);
                    link(out, "N" + to_string(tmpCondId[2]), node);
                    tmpCondId[1] = -1;
                    tmpCondId[2] = -1;
                } else { //если одну, то тоже соединяем
                    if (auto tmpst = dynamic_cast<ConditionExpression *>(exprs[sz - 1])) {
                        if (tmpst->getCondition().front().getValue() == "if") {
                            link(out, "N" + to_string(tmpCondId[0]), node, "false");
                            link(out, "N" + to_string(tmpCondId[1]), node);
                        }
                    }
                    else{
                        if (auto tmpst = dynamic_cast<CaseOf *>(exprs[sz - 1])) {
                            last = node;
                            ++sz;
                            continue;
                        }
                        else{
                            link(out, last, node);
                        }
                    }
                }
                last = node; //курсор на новом ромбике
                const auto body = sw->getBody();
                int *tmpSwitchId = new int[body.size()];
                for (int i = 0; i < body.size(); i++) {
                    std::string f = build(body[i].second); //обрабатываем тело
                    tmpSwitchId[i] = id;
                    link(out, last, f, tokensToLine(body[i].first));
                }
                for (int i = 0; i < body.size(); i++) {
                    link(out, "N" + to_string(tmpSwitchId[i]), "N" + to_string(id + 1));
                }
                ++sz;
                continue;
            }
            if (auto pr = dynamic_cast<Procedure *>(e)) {
                std::string node = newFunction(out, ++id, tokensToLine(pr->getHead()));
                if ((layer == 1 && sz != 0) || sz != 0) {
                    link(out, last, node);
                }
                last = node;
                ++sz;
                continue;
            }
            if (auto fc = dynamic_cast<Function *>(e)) {
                std::string node = newFunction(out, ++id, tokensToLine(fc->getHead()));
                if ((layer == 1 && sz != 0) || sz != 0) {
                    link(out, last, node);
                }
                last = node;
                ++sz;
                continue;
            }
        }
        if (layer == 1) {
            out << "N" << (++id) << "([End])\n";
            if (tmpCondId[2] != -1) {
                link(out, "N" + std::to_string(tmpCondId[1]), "N" + std::to_string(id));
                link(out, "N" + std::to_string(tmpCondId[2]), "N" + std::to_string(id));
            } else {
                link(out, "N" + std::to_string(id - 1), "N" + std::to_string(id));
            }
        }
        layer--;
        return out.str();
    }

private:
    std::string tokensToLine(const std::vector<Token> &v) {
        std::string s;
        for (Token t: v) {
            if (!s.empty())
                s += " ";
            s += t.getValue();
        }
        return s;
    }
    std::string tokensToLine(Token t) {
        std::string s;
        if (!s.empty())
            s += " ";
        s += t.getValue();
        return s;
    }

    std::string newProcess(std::ostringstream &out, int idd, const std::string &label) {
        std::string node = "N" + std::to_string(idd);
        std::string lab = escape(label);
        out << node << "[\"" << lab << "\"]\n";

        return node;
    }

    std::string newFunction(std::ostringstream &out, int idd, const std::string &label) {
        std::string node = "N" + std::to_string(idd);
        std::string lab = escape(label);
        out << node << "[[\"" << lab << "\"]]\n";

        return node;
    }

    std::string newOut(std::ostringstream &out, int idd, const std::string &label) {
        std::string node = "N" + std::to_string(idd);
        std::string lab = escape(label);
        out << node << "[/\"" << lab << "\"/]\n";
        return node;
    }

    std::string newDecision(std::ostringstream &out, int idd, const std::string &label) {
        std::string node = "N" + std::to_string(idd);
        out << node << "{\"" << escape(label) << "\"}\n";
        return node;
    }

    std::string newFor(std::ostringstream &out, int idd, const std::string &label) {
        std::string node = "N" + std::to_string(idd);
        out << node << "{{\"" << escape(label) << "\"}}\n";
        return node;
    }

    void link(std::ostringstream &out, const std::string &a, const std::string &b) {
        out << a << " --> " << b << "\n";
    }

    void link(std::ostringstream &out, const std::string &a, const std::string &b, const std::string &direction) {
        out << a << " -->|" << direction << "| " << b << "\n";
    }

    std::string escape(const std::string &s) {
        std::string r;
        r.reserve(s.size());
        for (char c: s) {
            if (c == '"')
                r += "\\\"";
            else
                r += c;
        }
        return r;
    }
};

#endif // FLOWCHART_FROM_EXPRESSIONS_H



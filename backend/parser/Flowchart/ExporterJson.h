#ifndef FLOWCHART_EXPORTER_JSON_H
#define FLOWCHART_EXPORTER_JSON_H

#include <string>
#include <sstream>
#include <vector>
#include "../Expression/Expression.h"
#include "../Expression/StatementExpression.h"
#include "../Expression/ConditionExpression.h"
#include "../Expression/CaseOf.h"
#include "../Token.h"
#include "../json.hpp"
using json = nlohmann::json;

class FlowchartExporterJson {
private:
    json programJson;
    struct BuildState {
        int nextId{1};
        std::ostringstream nodes;
        std::ostringstream edges;
        bool firstNode{true};
        bool firstEdge{true};
    };
public:
    std::string toJson(const std::vector<Expression *> &exprs) {
        BuildState st;
        std::string start = addNode(st, "Start", "");
        std::string last = start;

        size_t i = 0;
        while (i < exprs.size()) {
            Expression *e = exprs[i];
            if (auto stx = dynamic_cast<StatementExpression *>(e)) {
                std::string n = addNode(st, "Process", tokensToLine(stx->getList()));
                addEdge(st, last, n, "");
                last = n;
                ++i;
                continue;
            }
            if (auto cx = dynamic_cast<ConditionExpression *>(e)) {
                bool isElse = !cx->getCondition().empty() && cx->getCondition().front().getValue() == "else";
                if (isElse) {
                    std::string n = addNode(st, "Process", "else");
                    addEdge(st, last, n, "");
                    last = n;
                    ++i;
                    continue;
                }
                std::string dec = addNode(st, "Decision", tokensToLine(cx->getCondition()));
                addEdge(st, last, dec, "");

                // yes branch
                std::string yesHead = dec;
                std::string yesTail = dec;
                for (auto *inner: cx->getBody().second) {
                    if (auto s2 = dynamic_cast<StatementExpression *>(inner)) {
                        std::string n2 = addNode(st, "Process", tokensToLine(s2->getList()));
                        addEdge(st, yesTail, n2, "Yes");
                        yesTail = n2;
                    } else {
                        std::string n2 = addNode(st, "Process", "...");
                        addEdge(st, yesTail, n2, "Yes");
                        yesTail = n2;
                    }
                }

                // no branch
                std::string noTail = dec;
                if (i + 1 < exprs.size()) {
                    if (auto nx = dynamic_cast<ConditionExpression *>(exprs[i + 1])) {
                        bool isElse2 = !nx->getCondition().empty() && nx->getCondition().front().getValue() == "else";
                        if (isElse2) {
                            for (auto *inner: nx->getBody().second) {
                                if (auto s3 = dynamic_cast<StatementExpression *>(inner)) {
                                    std::string n3 = addNode(st, "Process", tokensToLine(s3->getList()));
                                    addEdge(st, noTail, n3, "No");
                                    noTail = n3;
                                } else {
                                    std::string n3 = addNode(st, "Process", "...");
                                    addEdge(st, noTail, n3, "No");
                                    noTail = n3;
                                }
                            }
                            ++i; // consume else
                        } else {
                            // no else body
                            std::string skip = addNode(st, "Connector", "");
                            addEdge(st, noTail, skip, "No");
                            noTail = skip;
                        }
                    } else {
                        std::string skip = addNode(st, "Connector", "");
                        addEdge(st, noTail, skip, "No");
                        noTail = skip;
                    }
                }

                // merge
                std::string merge = addNode(st, "Connector", "");
                addEdge(st, yesTail, merge, "");
                addEdge(st, noTail, merge, "");
                last = merge;
                ++i;
                continue;
            }
            if (auto sw = dynamic_cast<CaseOf *>(e)) {
                std::string n = addNode(st, "Decision", "case ... of");
                addEdge(st, last, n, "");
                Token t =sw->getVal();
                last = n;
                ++i;
                continue;
            }
            ++i;
        }

        std::string end = addNode(st, "End", "");
        addEdge(st, last, end, "");

        std::ostringstream json;
        json << "{\n";
        json << "  \"nodes\": [" << st.nodes.str() << "],\n";
        json << "  \"edges\": [" << st.edges.str() << "],\n";
        json << "  \"meta\": {\"language\": \"pascal\", \"errors\": []}\n";
        json << "}\n";
        return json.str();
    }

private:
    static std::string tokensToLine(const std::vector<Token> &v) {
        std::string s;
        for (Token t: v) {
            if (!s.empty())
                s += " ";
            s += t.getValue();
        }
        return s;
    }

    static std::string esc(const std::string &s) {
        std::string r;
        r.reserve(s.size());
        for (char c: s) {
            if (c == '"')
                r += "\\\"";
            else if (c == '\\')
                r += "\\\\";
            else if (c == '\n')
                r += "\\n";
            else r += c;
        }
        return r;
    }

    static std::string idStr(int id) {
        return std::string("n") + std::to_string(id);
    }

    static std::string addNode(BuildState &st, const std::string &type, const std::string &label) {
        std::string id = idStr(st.nextId++);
        if (!st.firstNode)
            st.nodes << ",";
        st.firstNode = false;
        st.nodes << "{\"id\":\"" << id << "\",\"type\":\"" << type << "\",\"label\":\"" << esc(label) << "\"}";
        return id;
    }

    static void addEdge(BuildState &st, const std::string &from, const std::string &to, const std::string &label) {
        if (!st.firstEdge)
            st.edges << ",";
        st.firstEdge = false;
        st.edges << "{\"from\":\"" << from << "\",\"to\":\"" << to << "\"";
        if (!label.empty())
            st.edges << ",\"label\":\"" << esc(label) << "\"";
        st.edges << "}";
    }
};

#endif // FLOWCHART_EXPORTER_JSON_H




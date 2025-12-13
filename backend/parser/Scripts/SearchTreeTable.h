//
// Created by shuri on 30.04.2024.
//

#ifndef SEARCHTREETABLE_H
#define SEARCHTREETABLE_H

#include <iostream>
#include "string"
using namespace std;

template<class Key, class Value>
class SearchTreeTable {
protected:
    struct Row {
        Key key;
        Value value;
        string type;
    };
    struct Node
    {
        Row data;
        Node* left;
        Node* right;
        Node* parent;
        Node(Key key, Value value, string type) noexcept {
            left = nullptr;
            right = nullptr;
            data.key = key;
            data.value = value;
            this->parent = nullptr;
            data.type=type;
        }
        Node(Key key, string type) noexcept {
            left = nullptr;
            right = nullptr;
            data.key = key;
            data.type=type;
            this->parent = nullptr;
        }
        Node(Node* l, Node* r, Key key, Value value, string type) noexcept {
            left = l;
            right = r;
            data.key = key;
            data.value = value;
            data.type=type;
            l->parent = r->parent = this;
        }
        Node(Node* l, Node* r, Key key, string type) noexcept {
            left = l;
            right = r;
            data.key = key;
            data.type=type;
            l->parent = r->parent = this;
        }
        void print()  {
            if (this == nullptr) return;
            this->left->print();
            if (this->parent == nullptr)
                cout << "Parent: null, ";
            else
                cout << "Parent: " << this->parent->data.key << ", ";
            cout << "Key:" << this->data.key  <<" , Value: " << this->data.value <<", Type: "<<this->data.type<<", Left potomok: ";
            if (this->left == nullptr)
                cout << "null " << ", ";
            else { cout << this->left->data.key << " ,"; }
            if (this->right == nullptr)
                cout << "Right potomok: null " << " )" << endl;
            else { cout << "Right potomok: " << this->right->data.key << " )" << endl; }
            this->right->print();
        }
        Node& operator=(const Node& other) noexcept {
            parent = other.parent;
            left = other.left;
            right = other.right;
            data.key = other.data.key;
            data.type=other.data.type;
            data.value = other.data.value;
        }
    };
public:
    Node* root;

    SearchTreeTable()  noexcept
    {
        //TPolinom p;
        //root = new Node(Key(), Value());
        root = nullptr;
        //root->parent = nullptr;
    }
    SearchTreeTable(int key, string value, string type)noexcept
    {
        root = new Node(key, value, type);
    }
    SearchTreeTable(SearchTreeTable p1, SearchTreeTable p2, int key, string value,string type)noexcept
    {
        root = new Node(p1.root, p2.root, key, value, type);
        root->parent = nullptr;
    }
    string toString()noexcept {
        string res;
        res += "(" + std::to_string(root->data.key) + ", " + std::to_string(root->data.value) + ")";
        return res;
    }
    Node* findNode(Key _key, Node* node)noexcept
    {
        if (node == nullptr) {
            return nullptr;
        }
        if (_key < node->data.key) {
            return findNode(_key, node->left);
        }
        else if (_key > node->data.key) {
            return findNode(_key, node->right);
        }
        else {
            return node; // Âîçâðàùàåì óçåë, åñëè êëþ÷ ñîâïàäàåò
        }
    }
    void Change(Key key,Value val, string ty){
        Node* node=findNode(key,root);
        if(node == nullptr){ throw std::runtime_error{"Variable wasn't declared, fix pls"};}//Insert(key,val,"TYPEINTEGER");}
        else{
            node->data.value=val;
            node->data.type=ty;}
    }
    Node* deleteNode(Node* currentNode, Key _key)noexcept
    {
        if (currentNode == nullptr) return nullptr;

        if (_key < currentNode->data.key) {
            currentNode->left = deleteNode(currentNode->left, _key);
        }
        else if (_key > currentNode->data.key) {
            currentNode->right = deleteNode(currentNode->right, _key);
        }
        else {
            // ñëó÷àé äëÿ 0 ïîòîìêîâ
            if ((currentNode->left == nullptr) && (currentNode->right == nullptr)) {
                if ((currentNode->parent)->left == currentNode) { (currentNode->parent)->left = nullptr; }
                else { (currentNode->parent)->right = nullptr; }
                delete currentNode;
                return nullptr;
            }
            else if (currentNode->left != nullptr && currentNode->right != nullptr) {
                Node* temp = maxValueNode(currentNode->left);
                currentNode->data = temp->data;
                deleteNode(currentNode->left, temp->data.key);
                return currentNode;
            }
            else {
                Node* temp = (currentNode->left != nullptr) ? currentNode->left : currentNode->right;
                if (currentNode->parent == nullptr) {
                    currentNode = temp;
                    delete temp;
                    return currentNode;
                }
                else {
                    if (currentNode == currentNode->parent->left) {
                        currentNode->parent->left = temp;
                    }
                    else {
                        currentNode->parent->right = temp;
                    }
                    temp->parent = currentNode->parent;
                    delete currentNode;
                    return temp;
                }
            }
        }
        return currentNode;
    }
    Node* maxValueNode(Node* node)noexcept
    {
        Node* current = node;
        while (current->right != nullptr) {

            current = current->right;
        }
        return current;
    }

    int Insert(Key _key, Value _val, string type);
    int Insert(Key key, string type);
    int Delete(Key _key) ;
    Key* FindKey(Key _key) ;
    Value* FindValue(Key _key);
    void Reset(void) ;
    int GoNext(void) ;
    bool IsTabEnded(void) const ;

    Key GetKey(void) const ;
    Value* GetValuePtr(void) ;

    void print(Node* node) ;
};
template<class Key, class Value>
Value* SearchTreeTable<Key,Value>::GetValuePtr(void)
{
    //return this->GetValuePtr();
    return &Value();
}

template<class Key, class Value>
Key SearchTreeTable<Key, Value>::GetKey(void) const  {
    //return this->GetKey();
    return Key();
}

template<class Key, class Value>
int SearchTreeTable<Key, Value>::GoNext(void)
{
    return 0;
}

template<class Key, class Value>
bool SearchTreeTable<Key, Value>::IsTabEnded(void) const
{
    return true;
}

template<class Key, class Value>
void SearchTreeTable<Key, Value>::Reset(void)
{
    int i = 0;
}


template<class Key, class Value>
int SearchTreeTable<Key, Value>::Delete(Key _key)
{
    if ((this->FindKey(_key) == nullptr) || (root == nullptr)) {
        // Óçåë ñ òàêèì êëþ÷îì íå ñóùåñòâóåò, óäàëåíèå íå òðåáóåòñÿ
        return 1;
    }
    else {
        root = deleteNode(root, _key);
        return 0; // Óñïåøíîå óäàëåíèå óçëà
    }
}

template<class Key, class Value>
int SearchTreeTable<Key, Value>::Insert(Key _key, Value _val, string type)
{
    if (this->FindKey(_key) != nullptr) {
        // Óçåë ñ òàêèì êëþ÷îì óæå ñóùåñòâóåò, âñòàâêà íå òðåáóåòñÿ
        return 0;
    }
    else {
        // Ñîçäàåì íîâûé óçåë
        Node* new_node = new Node(_key, _val, type);
        if (root == nullptr) {
            root = new_node; // Åñëè äåðåâî ïóñòîå, íîâûé óçåë ñòàíîâèòñÿ êîðíåì
            root->parent = nullptr;
            return 0; // Óñïåøíàÿ âñòàâêà íîâîãî óçëà
        }
        Node* current = root;
        Node* parent = nullptr;
        // Íàõîäèì ìåñòî äëÿ âñòàâêè íîâîé âåðøèíû
        while (current != nullptr) {
            parent = current;
            if (_key < current->data.key) {
                current = current->left;
            }
            else {
                current = current->right;
            }
        }
        // Âñòàâëÿåì íîâóþ âåðøèíó è óñòàíàâëèâàåì ññûëêó íà ðîäèòåëÿ
        if (_key < parent->data.key) {
            parent->left = new_node;
        }
        else {
            parent->right = new_node;
        }
        new_node->parent = parent;
        return 0; // Óñïåøíàÿ âñòàâêà íîâîãî óçëà
    }
}
template<class Key, class Value>
int SearchTreeTable<Key, Value>::Insert(Key _key, string type){
    if (this->FindKey(_key) != nullptr) {
        // Óçåë ñ òàêèì êëþ÷îì óæå ñóùåñòâóåò, âñòàâêà íå òðåáóåòñÿ
        return 0;
    }
    else {
        // Ñîçäàåì íîâûé óçåë
        Node* new_node = new Node(_key, string(), type);
        if (root == nullptr) {
            root = new_node; // Åñëè äåðåâî ïóñòîå, íîâûé óçåë ñòàíîâèòñÿ êîðíåì
            root->parent = nullptr;
            return 0; // Óñïåøíàÿ âñòàâêà íîâîãî óçëà
        }
        Node* current = root;
        Node* parent = nullptr;
        // Íàõîäèì ìåñòî äëÿ âñòàâêè íîâîé âåðøèíû
        while (current != nullptr) {
            parent = current;
            if (_key < current->data.key) {
                current = current->left;
            }
            else {
                current = current->right;
            }
        }
        // Âñòàâëÿåì íîâóþ âåðøèíó è óñòàíàâëèâàåì ññûëêó íà ðîäèòåëÿ
        if (_key < parent->data.key) {
            parent->left = new_node;
        }
        else {
            parent->right = new_node;
        }
        new_node->parent = parent;
        return 0; // Óñïåøíàÿ âñòàâêà íîâîãî óçëà
    }
}
template<class Key, class Value>
Key* SearchTreeTable<Key, Value>::FindKey(Key _key)
{
    Node* node = findNode(_key, root);
    if (node == nullptr) { return nullptr; }
    return &node->data.key;
}
template<class Key, class Value>
Value* SearchTreeTable<Key, Value>::FindValue(Key _key)
{
    Node* node = findNode(_key, root);
    if (node == nullptr) { return nullptr; }
    return &node->data.value;
}
template<class Key, class Value>
void SearchTreeTable<Key, Value>::print(Node* node)
{
    if (node == nullptr) return;
    print(node->left);
    //cout << "(Left potomok: ";
    if (node->parent == nullptr)
        cout << "Parent: null, ";
    else
        cout << "Parent: " << node->parent->data.key << ", ";
    cout << "Key:" << node->data.key << ", Value: " << node->data.value <<", Type: "<<node->data.type<< ", Left potomok: ";
    if (node->left == nullptr)
        cout << "null, Right potomok: ";
    else { cout << node->left->data.key << " Right Potomok: "; }
    if (node->right == nullptr)
        cout << "null " << ")" << endl;
    else { cout << node->right->data.key << " )" << endl; }
    print(node->right);
}


#endif //SEARCHTREETABLE_H

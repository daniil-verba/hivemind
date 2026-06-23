#ifndef BINARYTREE_H
#define BINARYTREE_H

#include <iostream>
#include <fstream>
#include <cstring>

// Шаблон класса "Упорядоченное бинарное дерево" (по убыванию)
template <typename T>
class BinaryTree {
private:
    // Структура узла дерева
    struct Node {
        T data;
        Node* left;
        Node* right;
        Node(const T& value) : data(value), left(nullptr), right(nullptr) {}
    };

    Node* root;          // Корень дерева
    int nodeCount;       // Количество узлов

    // Вспомогательные рекурсивные методы
    void insert(Node*& node, const T& value);
    bool search(Node* node, const T& value) const;
    void inorderDesc(Node* node, std::ostream& out) const;   // Обход по убыванию
    void clear(Node* node);
    void copyTree(Node*& dest, Node* src);

public:
    // Конструктор, деструктор, копирование
    BinaryTree();
    BinaryTree(const BinaryTree& other);
    ~BinaryTree();
    BinaryTree& operator=(const BinaryTree& other);

    // Set/Get методы
    void insert(const T& value);
    bool find(const T& value) const;
    int getSize() const { return nodeCount; }
    bool isEmpty() const { return root == nullptr; }

    // Метод доступа (по убыванию) - вывод на экран
    void display() const;

    // Вывод в файл и чтение из файла
    void saveToFile(const char* filename) const;
    void loadFromFile(const char* filename);

    // Поиск элемента по содержанию
    bool searchByValue(const T& value) const { return find(value); }
};

// -------------------- Реализация методов --------------------

template <typename T>
BinaryTree<T>::BinaryTree() : root(nullptr), nodeCount(0) {}

template <typename T>
BinaryTree<T>::BinaryTree(const BinaryTree& other) : root(nullptr), nodeCount(0) {
    copyTree(root, other.root);
}

template <typename T>
BinaryTree<T>::~BinaryTree() {
    clear(root);
}

template <typename T>
BinaryTree<T>& BinaryTree<T>::operator=(const BinaryTree& other) {
    if (this != &other) {
        clear(root);
        root = nullptr;
        nodeCount = 0;
        copyTree(root, other.root);
    }
    return *this;
}

// Вставка с сохранением порядка по убыванию
template <typename T>
void BinaryTree<T>::insert(Node*& node, const T& value) {
    if (node == nullptr) {
        node = new Node(value);
        nodeCount++;
    }
    else if (value > node->data) {   // По убыванию: большее влево
        insert(node->left, value);
    }
    else if (value < node->data) {   // Меньшее вправо
        insert(node->right, value);
    }
    else {
        // Равные не вставляем (или можно вставить в левое поддерево — по желанию)
        return;
    }
}

template <typename T>
void BinaryTree<T>::insert(const T& value) {
    insert(root, value);
}

// Поиск по значению
template <typename T>
bool BinaryTree<T>::search(Node* node, const T& value) const {
    if (node == nullptr) return false;
    if (node->data == value) return true;
    if (value > node->data) return search(node->left, value);
    else return search(node->right, value);
}

template <typename T>
bool BinaryTree<T>::find(const T& value) const {
    return search(root, value);
}

// Обход в порядке убывания (правое-корень-левое даст возрастание, нам нужно левое-корень-правое для убывания? 
// При нашей вставке: большие слева, маленькие справа. Для убывания идём: правое поддерево (меньшие) -> корень -> левое (большие) — нет.
// Правильно: если большие слева, то чтобы вывести по убыванию, нужно сначала левое (большие), потом корень, потом правое (меньшие).
// Это обычный inorder, но из-за того что left > data > right, мы просто делаем inorder: left, root, right и получаем убывание.
// Да, так как в left большие, в right маленькие, то left -> root -> right даст убывание.
template <typename T>
void BinaryTree<T>::inorderDesc(Node* node, std::ostream& out) const {
    if (node != nullptr) {
        inorderDesc(node->left, out);   // сначала большие
        out << node->data << " ";
        inorderDesc(node->right, out);  // потом мен


ьшие
    }
}

template <typename T>
void BinaryTree<T>::display() const {
    if (root == nullptr) {
        std::cout << "Дерево пусто.\n";
        return;
    }
    std::cout << "Элементы дерева (по убыванию): ";
    inorderDesc(root, std::cout);
    std::cout << std::endl;
}

// Сохранение в файл (прямой обход для сохранения структуры - используем preorder)
// Но для простоты сохраним как список в порядке убывания (только значения, без структуры)
// При загрузке будем вставлять заново.
template <typename T>
void BinaryTree<T>::saveToFile(const char* filename) const {
    std::ofstream fout(filename);
    if (!fout.is_open()) {
        std::cout << "Ошибка открытия файла для записи!\n";
        return;
    }
    // Сохраняем количество элементов, затем сами элементы в порядке убывания
    fout << nodeCount << "\n";

    struct SaveHelper {
        static void save(Node* node, std::ofstream& ofs) {
            if (!node) return;
            save(node->left, ofs);
            ofs << node->data << "\n";
            save(node->right, ofs);
        }
    };
    SaveHelper::save(root, fout);
    fout.close();
}

template <typename T>
void BinaryTree<T>::loadFromFile(const char* filename) {
    std::ifstream fin(filename);
    if (!fin.is_open()) {
        std::cout << "Ошибка открытия файла для чтения!\n";
        return;
    }
    // Очищаем текущее дерево
    clear(root);
    root = nullptr;
    nodeCount = 0;
    
    T value;
    while (fin >> value) {
        insert(value);
    }
    fin.close();
}

template <typename T>
void BinaryTree<T>::clear(Node* node) {
    if (node != nullptr) {
        clear(node->left);
        clear(node->right);
        delete node;
    }
}

template <typename T>
void BinaryTree<T>::copyTree(Node*& dest, Node* src) {
    if (src == nullptr) {
        dest = nullptr;
        return;
    }
    dest = new Node(src->data);
    copyTree(dest->left, src->left);
    copyTree(dest->right, src->right);
    nodeCount++;
}

#endif
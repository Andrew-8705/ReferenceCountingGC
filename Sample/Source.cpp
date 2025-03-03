#include "malloc.h"
#include <iostream>

struct Node {
    int data;
    Node* next;

    Node(int value) : data(value), next(nullptr) {}
};

int main() {
    initMemMgr(1024); // Инициализация менеджера памяти

    // Создание узлов списка
    Node* head = (Node*)newMalloc(sizeof(Node));
    head->data = 10;
    (*mmptr).inc(head); // Увеличение счетчика ссылок

    Node* second = (Node*)newMalloc(sizeof(Node));
    second->data = 20;
    (*mmptr).inc(second); // Увеличение счетчика ссылок

    head->next = second;
    (*mmptr).inc(second); // Увеличение счетчика ссылок, так как на него ссылается head

    // Использование списка
    std::cout << "head->data: " << head->data << std::endl;
    std::cout << "head->next->data: " << head->next->data << std::endl;

    // Удаление списка
    (*mmptr).dec(head->next); // Уменьшение счетчика ссылок, так как head больше не ссылается на second
    (*mmptr).dec(head); // Уменьшение счетчика ссылок для head
    (*mmptr).dec(second); // Уменьшение счетчика ссылок для second

    closeMemMgr(); // Закрытие менеджера памяти

    return 0;
}
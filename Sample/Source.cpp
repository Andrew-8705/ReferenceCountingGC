#include "malloc.h"
#include <iostream>

struct Node {
    int data;
    Node* next;

    Node(int value) : data(value), next(nullptr) {}
};

int main() {
    initMemMgr(1024); // ������������� ��������� ������

    // �������� ����� ������
    Node* head = (Node*)newMalloc(sizeof(Node));
    head->data = 10;
    (*mmptr).inc(head); // ���������� �������� ������

    Node* second = (Node*)newMalloc(sizeof(Node));
    second->data = 20;
    (*mmptr).inc(second); // ���������� �������� ������

    head->next = second;
    (*mmptr).inc(second); // ���������� �������� ������, ��� ��� �� ���� ��������� head

    // ������������� ������
    std::cout << "head->data: " << head->data << std::endl;
    std::cout << "head->next->data: " << head->next->data << std::endl;

    // �������� ������
    (*mmptr).dec(head->next); // ���������� �������� ������, ��� ��� head ������ �� ��������� �� second
    (*mmptr).dec(head); // ���������� �������� ������ ��� head
    (*mmptr).dec(second); // ���������� �������� ������ ��� second

    closeMemMgr(); // �������� ��������� ������

    return 0;
}
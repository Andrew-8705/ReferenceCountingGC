/*
#include <iostream>
#include "../MultiThreading/config.h"
#include <unordered_map>

using namespace std;

intptr_t* __rbp;
intptr_t* __rsp;
intptr_t* __stackBegin;

template <typename T>
class MallocAllocator {
public:
    using value_type = T;

    MallocAllocator() noexcept = default;

    template <typename U>
    MallocAllocator(const MallocAllocator<U>&) noexcept {}

    T* allocate(size_t n) {

        void* ptr = malloc(n * sizeof(T));
        if (!ptr) {
            throw std::bad_alloc();
        }
        return static_cast<T*>(ptr);
    }

    void deallocate(T* ptr, size_t) noexcept {
        free(ptr);
    }

    template <typename U>
    bool operator==(const MallocAllocator<U>&) const noexcept { return true; }

    template <typename U>
    bool operator!=(const MallocAllocator<U>&) const noexcept { return false; }
};

void gcInit() {
    __READ_RBP();
    __stackBegin = (intptr_t*)*__rbp;
}

class GarbageCollector
{
private:
    unordered_map<
        void*,
        size_t,
        std::hash<void*>,
        std::equal_to<void*>,
        MallocAllocator<std::pair<void* const, size_t>>
    > ref_count;
    bool initialized = false;
    bool destructed = false;

    void scanStack() {
        cout << "SCANNING STACK" << '\n';
        //jmp_buf jb;
        //setjmp(jb); // Сохраняем контекст

        uintptr_t* rsp;
        __asm { mov rsp, esp } // Получаем текущий RSP

        uintptr_t* top = (uintptr_t*)__stackBegin;

        while (rsp < top) {
            void* ptr = (void*)*rsp;
            cout << ptr << '\n';
            if (ref_count.find(ptr) != ref_count.end()) {
                cout << "qokdqpkmqpldmqplmd[qlmldmqlmdq;dm'L;ALDMAL;SKMN;LMFPW;MFP;" << '\n';
                ref_count[ptr]++;
            }
            rsp += sizeof(uintptr_t);
            //rsp += 1;
        }
    }

public:
    GarbageCollector() {
        cout << "GC constructor" << '\n';
        initialized = true;
    }

    void collect() {
        cout << "[GC] reference counting...\n";
        cout << "SIZE OF REF_COUNT: " << ref_count.size() << '\n';
        for (auto& pair : ref_count) {
            cout << pair.first << ' ' << pair.second << '\n';
            pair.second = 0;
        }

        scanStack();

        cout << "AFTER COUNTING: \n";
        for (const auto& pair : ref_count) cout << pair.first << ' ' << pair.second << '\n';
        std::vector<void*> keys_to_delete;

        for (const auto& pair : ref_count) {
            if (pair.second == 0) {
                keys_to_delete.push_back(pair.first);
            }
        }

        for (void* key : keys_to_delete) {
            size_t cnt = ref_count.erase(key);
            if (cnt > 0) {
                cout << "Key was deleted: " << key << std::endl;
            }
            delete(key);
        }

        cout << "AFTER DELETING: \n";
        for (auto& pair : ref_count) {
            cout << pair.first << ' ' << pair.second << '\n';
        }
    }

    void registerPtr(void* ptr) {
        ref_count[ptr] = 1;
    }

    bool findPtr(void* ptr) {
        if (!initialized) return false;
        if (destructed) return false;
        return ptr && ref_count.find(ptr) != ref_count.end();
    }

    void decPtr(void* ptr) {
        auto it = ref_count.find(ptr);
        if (it != ref_count.end() && it->second > 0) {
            it->second--;
        }
    }

    void removePtr(void* ptr) {
        if (ref_count[ptr] == 0) ref_count.erase(ptr);
    }

    ~GarbageCollector() {
        destructed = true;
    }
};

GarbageCollector gc;

void* operator new(size_t size) {
    void* ptr = malloc(size);
    if (!ptr) throw std::bad_alloc();
    gc.registerPtr(ptr);
    return ptr;
}

void operator delete(void* ptr) noexcept {
    if (!ptr) return;
    if (gc.findPtr(ptr)) {
        cout << "DELETE THIS PTR: " << ptr << '\n';
        gc.decPtr(ptr);
        gc.removePtr(ptr);
    }
    else free(ptr);
}

class MyClass {
public:
    int value;
    //int* ptr;
    MyClass* next = nullptr;
    MyClass(int v) : value(v) {
        cout << "Create object " << this << endl;
        //ptr = new int(1);
        //cout << *ptr << '\n' << "JSONSLJKLSJLJLSSKL" << '\n';
    }
    ~MyClass()
    {
        cout << "Delete object " << this << endl;
        //delete(ptr);
    }
};

void foo1()
{
    vector<MyClass*> v(1000);
    for (int i = 0; i < 1000; ++i) {
        v[i] = new MyClass(i);
    }
}
void foo2()
{
    //MyClass c(10);
    MyClass* ptr = new MyClass(1);
    cout << "!1111!!!!!!!!!!!!!!!!!!!!!!!!!!!!!" << '\n';
    gc.collect();
    //MyClass* ptr10 = ptr;
    //MyClass* ptr1 = new MyClass(42);
    //int* n = new int(10);
    //gc.collect();
    //MyClass* ptr2 = new MyClass(51);
    //cout << "COLECT INSIDE foo2" << '\n';
    //gc.collect();
}

void foo3()
{
    MyClass* a = new MyClass(1);
    MyClass* b = new MyClass(2);
    a->next = b;
    b->next = a;
    //gc.collect();
}

void foo4()
{
    int* a = new int(1);
    int* b = new int(2);
    int* c = new int(3);
    int* d = new int(4);
    cout << a << '\n' << b << '\n' << c << '\n' << d << '\n';
    //delete b;
    //delete a;
    gc.collect();
}

void foo5()
{
    vector<int*> v(10);
    cout << "ADRESS OF V: " << &v << '\n';
    for (int i = 0; i < 10; i++)
        v[i] = new int(i);
    int* ptr = new int(1);
    vector<int*> v1 = { new int(1) };
    gc.collect();
}

void foo6()
{
    struct Node { Node* next; };
    Node* n1 = new Node();
    Node* n2 = new Node();
    n1->next = n2;
    gc.collect();
}

void foo7()
{
    vector<int*> v1(10);
    vector<int*> v2(9);
    vector<int*> v3(8);

    for (int i = 0; i < 10; i++) v1[i] = new int(i);
    for (int i = 0; i < 9; i++) v2[i] = new int(i);
    for (int i = 0; i < 8; i++) v3[i] = new int(i);
}

void foo8()
{
    vector<int*> v(10);
    for (int i = 0; i < 10; i++)
        v[i] = new int(i);
    //gc.collect();
}

int* foo9()
{
    int* a = new int(1);
    cout << "A: " << a << '\n';
    return a;
}

int* global_ptr;
void foo10()
{
    global_ptr = new int(1);
}

struct s {
    int* ptr;
};

void foo11()
{
    s* ptr = new s;
    cout << ptr << '\n';
    ptr->ptr = new int(42);
    cout << ptr->ptr << '\n';
}

int main() {
    gcInit();
    cout << __rsp << ' ' << __rbp << '\n';
    int* vip_ptr = new int(42);
    cout << "VIP_PTR: " << vip_ptr << '\n';
    cout << "EVERYTHING IS OKAY" << '\n';
    foo4();
    cout << "ptr out of scope" << '\n';
    gc.collect();
    cout << "END!!!!!!!!!!!!!!!!!!" << '\n';
    return 0;
}
*/
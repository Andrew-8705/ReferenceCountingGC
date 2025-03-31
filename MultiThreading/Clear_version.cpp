#include <iostream>
#include <thread>
#include <mutex>
#include <chrono>
#include <new>
#include <vector>
#include "CustomVector.h"
#include "CustomHashMap.h"
#include <windows.h>
#include <stdio.h>
#include <unordered_map>
#include <vector>

using namespace std;

#define getTOS() _asm{ MOV frameBase, EBP}
void* frameBase;

/*
namespace MyNamespace {
    void* operator new(size_t size) {
        cout << "Custom new called, size: " << size << '\n';
        return ::operator new(size); 
    }

    void operator delete(void* ptr) noexcept {
        std::cout << "Custom delete called" << '\n';
        ::operator delete(ptr); 
    }
}
*/

/*
�MyNamespace::operator delete�: ����� ��� ��������� ������� � ����������, �� ���������� ������, ����������
�������� ������������ ��� � ������������ ����, �������� �� ����������� ������������ ����
�MyNamespace::operator new": ����� ��� ��������� ������� � ����������, �� ���������� ������, ���������� ��������
������������ ��� � ������������ ����, �������� �� ����������� ������������ ����
*/

/*
int main() {
    // 1. �������� ������ � ������� �������������� new
    void* Memory = MyNamespace::operator new(sizeof(int));

    // 2. ��������� ������ � ���������� ������
    int* myInt = new (Memory) int(42); // placement new

    cout << *myInt << '\n';

    // 3. �������� ���������� ����
    myInt->~int();

    // 4. ����������� ������ � ������� �������������� delete
    MyNamespace::operator delete(Memory);

    return 0;
}
*/

class GarbageCollector
{
private:
    mutex gc_mutex;
    thread gc_thread;
    bool running;
    CustomHashMap<void*, size_t> ref_count; // ���������� ���� ���������� ���-�������
    CustomHashMap<void*, CustomVector<void**>> ptrs;  
    //unordered_map<void*, size_t> ref_count;
    //unordered_map<void*, vector<void**>> ptrs;

    //void scanStack() {
    //    void* stackBottom = _AddressOfReturnAddress();  // ������ ������� (ESP � x86 ��� RSP � x64)
    //    void* stackTop = getStackTop();
    //    size_t stackSize = (char*)stackBottom - (char*)stackTop;

    //    cout << "[scanStack] stackTop: " << stackTop << ", stackBottom: " << stackBottom << ", size: " << stackSize << endl;

    //    traverseMemory(stackTop, stackSize);
    //}

    void traverseMemory(void* start, size_t size) {
        /*char* begin = static_cast<char*>(start);
        char* end = begin + size;

        cout << "[traverseMemory] start: " << (void*)begin << " end: " << (void*)end << " size: " << size << endl;

        for (char* ptr = begin; ptr < end; ptr += sizeof(void*)) {
            void* possiblePtr = *reinterpret_cast<void**>(ptr);
            cout << "[traverseMemory] Checking address: " << (void*)ptr << " -> value: " << possiblePtr << endl;

            if (ref_count.find(possiblePtr)) {
                ref_count[possiblePtr]++;
                cout << "[traverseMemory] Incrementing ref count for: " << possiblePtr << endl;
            }
        }*/
        cout << "[TraverseMemory]" << '\n';
    }


    void scanStack() {
        void* currentAddr;
        _asm { MOV currentAddr, ESP };

        cout << "[scanStack] Start stack ESP: " << frameBase << " End stack EBP: " << currentAddr << endl;

        traverseMemory(currentAddr, (char*)frameBase - (char*)currentAddr);
    }


    void scanHeap() {

    }

    void scanMemory() {
        scanStack();
        scanHeap();
    }
public:
    GarbageCollector() {
        cout << "GC constructor" << '\n';
        gc_thread = thread(&GarbageCollector::gc_loop, this);
        running = true;
    }
    
    void gc_loop() {
        while (running) {
            std::this_thread::sleep_for(std::chrono::milliseconds(1000)); 

            lock_guard<mutex> lock(gc_mutex);
            cout << "[GC] reference counting...\n";

            cout << "SIZE OF REF_COUNT: " << ref_count.Size() << '\n';
            for (auto& pair : ref_count) {
                cout << pair.first << ' ' << pair.second << '\n';
                pair.second = 0;
            }
            
            scanMemory();

            //this_thread::sleep_for(chrono::milliseconds(100));
        }
    }
    void registerPtr(void* ptr) {
        std::lock_guard<std::mutex> lock(gc_mutex);
        //ref_count[ptr] = 1;
        ref_count.insert(ptr, 1);
        ptrs.insert_or_assign(ptr, {});
    }

    void trackPointer(void** pointer, void* obj) {
        std::lock_guard<std::mutex> lock(gc_mutex);
        if (obj) ptrs[obj].push_back(pointer);
    }

    ~GarbageCollector() {
        running = false;
        if (gc_thread.joinable()) {
            gc_thread.join();
        }
    }
};

GarbageCollector gc;

// �������������� ��������� new � delete
void* operator new(size_t n) { 
    cout << "Use operator new..." << '\n';
    if (n == 0) {
        n = 1;
    }

    while (true) {
        void* p = malloc(n); // ���������� malloc
        if (p) {
            gc.registerPtr(p);
            cout << "after get..." << '\n';
            return p;
        }

        new_handler handle = get_new_handler();
        if (!handle) {
            throw bad_alloc();
        }

        (*handle)();
    }
}

void operator delete(void* ptr) noexcept {
    free(ptr); 
}


void DoWork() {
    for (int i = 0; i < 10; i++) {
        cout << "ID thread = " << this_thread::get_id() << " DoWork " << i << '\n';
        this_thread::sleep_for(chrono::milliseconds(100));
    }
}

class MyClass {
public:
    int value;
    MyClass(int v) : value(v) { cout << "Create object " << this << endl; }
    ~MyClass() { cout << "Delete object " << this << endl; }
};



int main()
{
    getTOS();
    MyClass* ptr = new MyClass(10);
    //MyClass* p = ptr;
    for (int i = 0; i < 10; i++) {
        cout << "ID thread = " << this_thread::get_id() << " main " << i << '\n';
        this_thread::sleep_for(chrono::milliseconds(500));
    }
    return 0;
}


/*
���� ������ ����� ��������� ��������� new -> ���������� registerPtr, 
��� � ref_count ��������� ����� ������ (ptr, 1), 
� � ptrs ��������� ������ ��������� ������ (ptr, {})

���� �������� ��������� �� ������ � ��� ������������� �������� ������� ��������� -> 
�� ���������� new -> ������ ��������� ���������� ������. 
������ ��� �������� �������� �� ����� ����� � ����. 
�������� � ���� �������� ��������� ����������� �� ���������� ������, ��� �� ����� � ����
*/
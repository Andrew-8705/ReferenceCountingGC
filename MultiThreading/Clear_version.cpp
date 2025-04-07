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
#include <setjmp.h>
#include <memory>

using namespace std;

intptr_t* __rbp;

intptr_t* __rsp;

intptr_t* __stackBegin;

#define __READ_RBP() __asm__ volatile("movq %%rbp,  %0" :  "=r"(__rbp))
#define __READ_RSP() __asm__ volatile("movq %%rsp,  %0" :  "=r"(__rsp))


class GarbageCollector
{
private:
    mutex gc_mutex;
    thread gc_thread;
    bool running;
    CustomHashMap<void*, size_t> ref_count; // Используем нашу самописную хэш-таблицу
    CustomHashMap<void*, CustomVector<void**>> ptrs;  

    void scanStack() {
        
    }

    void scanMemory() {
        scanStack();
    }
public:
    GarbageCollector() {
        cout << "GC constructor" << '\n';
        gc_thread = thread(&GarbageCollector::gc_loop, this);
        running = true;
        __READ_RBP();
        __stackBegin = (intptr_t*)*__rbp;
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
            
            scanStack();

            cout << "AFTER COUNTING: \n";
            for (auto& pair : ref_count) {
                cout << pair.first << ' ' << pair.second << '\n';
                if (pair.second == 0) {
                    delete pair.first; 
                    ref_count.remove(pair.first);
                }
            }
            //this_thread::sleep_for(chrono::milliseconds(100));
        }
    }
    void registerPtr(void* ptr) {
        std::lock_guard<std::mutex> lock(gc_mutex);
        //ref_count[ptr] = 1;
        ref_count.insert(ptr, 1);
        ptrs.insert_or_assign(ptr, {});
    }

    /*void trackPointer(void** pointer, void* obj) {
        std::lock_guard<std::mutex> lock(gc_mutex);
        if (obj) ptrs[obj].push_back(pointer);
    }*/

    ~GarbageCollector() {
        running = false;
        if (gc_thread.joinable()) {
            gc_thread.join();
        }
    }
};

GarbageCollector gc;

// переопределяем глобально new и delete
void* operator new(size_t n) { 
    cout << "Use operator new..." << '\n';
    if (n == 0) {
        n = 1;
    }

    while (true) {
        void* p = malloc(n); // Используем malloc
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

    MyClass* ptr = new MyClass(10);
    //MyClass* p = ptr;
    for (int i = 0; i < 10; i++) {
        cout << "ID thread = " << this_thread::get_id() << " main " << i << '\n';
        this_thread::sleep_for(chrono::milliseconds(500));
    }
    return 0;
}

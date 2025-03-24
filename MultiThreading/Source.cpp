#include <iostream>
#include <thread>
#include <mutex>
#include <chrono>
#include <unordered_map>
#include <new>
#include <memory>

using namespace std;

template <typename T>
class MallocAllocator : public std::allocator<T> {
public:
    using value_type = T;

    MallocAllocator() = default;
    template <typename U>
    MallocAllocator(const MallocAllocator<U>&) {}

    T* allocate(std::size_t n) {
        return static_cast<T*>(malloc(n * sizeof(T)));
    }

    void deallocate(T* ptr, std::size_t n) {
        free(ptr);
    }
};

class GarbageCollector
{
private:
    mutex gc_mutex;
    thread gc_thread;
    bool running;
    unordered_map<void*, size_t> ref_count;

public:
    GarbageCollector() {
        cout << "GC constructor" << '\n';
        gc_thread = thread(&GarbageCollector::gc_loop, this);
        running = true;
    }
    
    void gc_loop() {
        while (running) {

            lock_guard<mutex> lock(gc_mutex); // задача lock_guard захватить mutex в конструкторе и освободить в деструкторе 
            cout << "[GC] reference counting...\n";

            this_thread::sleep_for(chrono::milliseconds(100));

            // вызывается деструктор lock -> mtx.unlock()
            // mtx.lock(), mtx.unlock() для указания конкретной области для блокировки или обернуть в {}
        }
    }

    void registerPtr(void* ptr) {
        cout << "pointer was registered: " << ptr << '\n';
        ref_count[ptr] = 1;

    }

    ~GarbageCollector() {
        running = false;
        if (gc_thread.joinable()) {
            gc_thread.join();
        }
    }
};

GarbageCollector gc;

void* operator new(size_t n) {
    cout << "Use operator new..." << '\n';
    if (n == 0) {
        n = 1;
    }

    while (true) {
        void* p = malloc(n);
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
    for (int i = 0; i < 10; i++) {
        cout << "ID thread = " << this_thread::get_id() << " main " << i << '\n';
        this_thread::sleep_for(chrono::milliseconds(500));
    }

    return 0;
}
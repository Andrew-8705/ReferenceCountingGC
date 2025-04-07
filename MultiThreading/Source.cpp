/*//#include <iostream>
//#include <thread>
//#include <mutex>
//#include <chrono>
//#include <unordered_map>
//#include <new>
//#include <memory>
//#include "hash_table.h"
//
//using namespace std;
//
//template <typename T>
//class MyAllocator : public std::allocator<T> {
//public:
//    using value_type = T;
//
//    MyAllocator() = default;
//    template <typename U> MyAllocator(const MyAllocator<U>&) {}
//
//    T* allocate(std::size_t n) {
//        T* ptr = static_cast<T*>(malloc(n * sizeof(T)));
//        std::cout << "MyAllocator: выделено " << n * sizeof(T) << " байт" << std::endl;
//        return ptr;
//    }
//
//    void deallocate(T* ptr, std::size_t n) {
//        free(ptr);
//        std::cout << "MyAllocator: освобождено " << n * sizeof(T) << " байт" << std::endl;
//    }
//};
//
//
//class GarbageCollector
//{
//private:
//    mutex gc_mutex;
//    thread gc_thread;
//    bool running;
//    //unordered_map<void*, size_t> ref_count;
//    //unordered_map<void*, size_t, std::hash<void*>, std::equal_to<void*>, MyAllocator<std::pair<void* const, size_t>>> ref_count;
//    Hash_open_addr_table<void*, size_t> ref_count;
//public:
//    GarbageCollector() {
//        cout << "GC constructor" << '\n';
//        gc_thread = thread(&GarbageCollector::gc_loop, this);
//        running = true;
//    }
//    
//    void gc_loop() {
//        while (running) {
//
//            lock_guard<mutex> lock(gc_mutex); // задача lock_guard захватить mutex в конструкторе и освободить в деструкторе 
//            cout << "[GC] reference counting...\n";
//
//            this_thread::sleep_for(chrono::milliseconds(100));
//
//            // вызывается деструктор lock -> mtx.unlock()
//            // mtx.lock(), mtx.unlock() для указания конкретной области для блокировки или обернуть в {}
//        }
//    }
//
//    void registerPtr(void* ptr) {
//        cout << "pointer was registered: " << ptr << '\n';
//        ref_count.insert(ptr, 1);
//        //ref_count[ptr] = 1;
//
//    }
//
//    ~GarbageCollector() {
//        running = false;
//        if (gc_thread.joinable()) {
//            gc_thread.join();
//        }
//    }
//};
//
//GarbageCollector gc;
//
////void* operator new(size_t n) {
////    cout << "Use operator new..." << '\n';
////    if (n == 0) {
////        n = 1;
////    }
////
////    while (true) {
////        void* p = malloc(n);
////        if (p) {
////            gc.registerPtr(p);
////            cout << "after get..." << '\n';
////            return p;
////        }
////
////        new_handler handle = get_new_handler();
////        if (!handle) {
////            throw bad_alloc();
////        }
////
////        (*handle)();
////    }
////}
//
////void operator delete(void* ptr) noexcept {
////    free(ptr);
////}
//
//void* operator new(size_t n) {
//    cout << "Use operator new..." << '\n';
//    if (n == 0) {
//        n = 1;
//    }
//
//    while (true) {
//        void* p = ::operator new(n); // Используем глобальный new
//        if (p) {
//            gc.registerPtr(p);
//            cout << "after get..." << '\n';
//            return p;
//        }
//
//        new_handler handle = get_new_handler();
//        if (!handle) {
//            throw bad_alloc();
//        }
//
//        (*handle)();
//    }
//}
//
//void operator delete(void* ptr) noexcept {
//    ::operator delete(ptr);
//}
//
//void DoWork() {
//    for (int i = 0; i < 10; i++) {
//        cout << "ID thread = " << this_thread::get_id() << " DoWork " << i << '\n';
//        this_thread::sleep_for(chrono::milliseconds(100));
//    }
//}
//
//class MyClass {
//public:
//    int value;
//    MyClass(int v) : value(v) { cout << "Create object " << this << endl; }
//    ~MyClass() { cout << "Delete object " << this << endl; }
//};
//
//
//int main()
//{
//    MyClass* ptr = new MyClass(10);
//    cout << "++++++++++++++++++++" << '\n';
//    for (int i = 0; i < 10; i++) {
//        cout << "ID thread = " << this_thread::get_id() << " main " << i << '\n';
//        this_thread::sleep_for(chrono::milliseconds(500));
//    }
//
//    return 0;
//} 
#include <iostream>
#include <thread>
#include <mutex>
#include <chrono>
#include <new>
#include <vector>
#include <cstdlib>   // malloc, realloc, free
#include <cstring>   // memcpy
#include <cassert>   // assert

using namespace std;

template <typename T>
class CustomVector {
private:
    T* data;          // Указатель на массив данных
    size_t size;      // Текущий размер
    size_t capacity;  // Вместимость

    void grow() {
        size_t new_capacity = (capacity == 0) ? 2 : capacity * 2;
        void* new_data = std::realloc(data, new_capacity * sizeof(T));
        assert(new_data); // Проверка, что память выделилась
        data = static_cast<T*>(new_data);
        capacity = new_capacity;
    }

public:
    CustomVector() : data(nullptr), size(0), capacity(0) {}

    ~CustomVector() {
        if (data) std::free(data);
    }

    void push_back(const T& value) {
        if (size == capacity) {
            grow();
        }
        std::memcpy(&data[size], &value, sizeof(T)); // Копируем память без вызова конструктора
        size++;
    }

    void pop_back() {
        if (size > 0) {
            size--;
        }
    }

    size_t get_size() const { return size; }

    T& operator[](size_t index) {
        assert(index < size);
        return data[index];
    }

    const T& operator[](size_t index) const {
        assert(index < size);
        return data[index];
    }

    bool empty() const { return size == 0; }

    void clear() { size = 0; }

    void erase(size_t index) {
        assert(index < size);
        std::memmove(&data[index], &data[index + 1], (size - index - 1) * sizeof(T));
        size--;
    }

    // Итераторы
    T* begin() {
        return data;
    }

    T* end() {
        return data + size;
    }
};

template <typename KeyType, typename ValueType>
class CustomHashMap {
private:
    static const size_t TABLE_SIZE = 10;
    pair<KeyType, ValueType> table[TABLE_SIZE];
    size_t size;

    size_t hash(const KeyType& key) {
        return reinterpret_cast<uintptr_t>(key) % TABLE_SIZE;
    }

public:
    CustomHashMap() : size(0) {
        for (size_t i = 0; i < TABLE_SIZE; ++i) {
            table[i].first = nullptr;
        }
    }

    void insert(const KeyType& key, const ValueType& value) {
        size_t index = hash(key);
        while (table[index].first != nullptr) {
            index = (index + 1) % TABLE_SIZE;
        }
        table[index].first = key;
        table[index].second = value;
        size++;
    }

    ValueType find(const KeyType& key) {
        size_t index = hash(key);
        while (table[index].first != nullptr) {
            if (table[index].first == key) {
                return table[index].second;
            }
            index = (index + 1) % TABLE_SIZE;
        }
        return ValueType(); // Возвращаем значение по умолчанию, если не найдено
    }

    void insert_or_assign(const KeyType& key, const ValueType& value) {
        size_t index = hash(key);
        while (table[index].first != nullptr) {
            if (table[index].first == key) {
                table[index].second = value; // Assign
                return;
            }
            index = (index + 1) % TABLE_SIZE;
        }
        insert(key, value); // Insert
    }

    // Итераторы
    pair<KeyType, ValueType>* begin() {
        return table;
    }

    pair<KeyType, ValueType>* end() {
        return table + TABLE_SIZE;
    }

    size_t Size() {
        return size;
    }

    ValueType& operator[](const KeyType& key) {
        size_t index = hash(key);
        while (table[index].first != nullptr && table[index].first != key) {
            index = (index + 1) % TABLE_SIZE;
        }

        if (table[index].first == key) {
            return table[index].second;
        }
        else {
            // Если ключ не найден, вставляем его с значением по умолчанию и возвращаем ссылку
            insert(key, ValueType());
            return table[hash(key)].second; // Возвращаем ссылку на вставленный элемент.
        }
    }
    // Добавьте методы для удаления, изменения и т.д.
};


class GarbageCollector
{
private:
    mutex gc_mutex;
    thread gc_thread;
    bool running;
    CustomHashMap<void*, size_t> ref_count; // Используем нашу самописную хэш-таблицу
    CustomHashMap<void*, CustomVector<void**>> ptrs;

public:
    GarbageCollector() {
        cout << "GC constructor" << '\n';
        gc_thread = thread(&GarbageCollector::gc_loop, this);
        running = true;
    }

    void gc_loop() {
        while (running) {
            std::this_thread::sleep_for(std::chrono::milliseconds(1000)); // Каждые 10 мс

            lock_guard<mutex> lock(gc_mutex);
            cout << "[GC] reference counting...\n";

            cout << "SIZE OF REF_COUNT: " << ref_count.Size() << '\n';
            for (auto& pair : ref_count) {
                cout << pair.first << ' ' << pair.second << '\n';
                pair.second = 0;
            }

            for (auto& pair : ptrs) {
                void* obj = pair.first;
                for (void** ref : pair.second) {
                    cout << "VOID**: " << ref << '\n';
                    if (*ref == obj) {
                        ref_count[obj]++;
                    }
                }
            }


            //this_thread::sleep_for(chrono::milliseconds(100));
        }
    }
    void registerPtr(void* ptr) {
        std::lock_guard<std::mutex> lock(gc_mutex);
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
    free(ptr); // Используем free
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
*/
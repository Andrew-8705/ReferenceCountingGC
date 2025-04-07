#include <iostream>
#include <cstdlib>
#include <cstring>
#include <cassert>
#include <setjmp.h>
#include <memory>
#include <vector>

using namespace std;

template <typename KeyType, typename ValueType>
class CustomHashMap {
private:
    static const size_t TABLE_SIZE = 200;
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

    bool find(const KeyType& key) {
        size_t index = hash(key);
        while (table[index].first != nullptr) {
            if (table[index].first == key) {
                return true;
            }
            index = (index + 1) % TABLE_SIZE;
        }
        return false; // Возвращаем значение по умолчанию, если не найдено
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

    bool remove(const KeyType& key) {
        size_t index = hash(key);
        size_t initial_index = index;

        while (table[index].first != nullptr) {
            if (table[index].first == key) {
                table[index].first = nullptr; // Помечаем как удаленный
                // Rehash последующих элементов, которые могли быть вставлены после коллизии
                size_t next_index = (index + 1) % TABLE_SIZE;
                while (table[next_index].first != nullptr) {
                    KeyType temp_key = table[next_index].first;
                    ValueType temp_value = table[next_index].second;
                    table[next_index].first = nullptr;
                    size--; // Уменьшаем размер перед повторной вставкой
                    insert(temp_key, temp_value);
                    next_index = (next_index + 1) % TABLE_SIZE;
                }
                size--;
                return true;
            }
            index = (index + 1) % TABLE_SIZE;
            if (index == initial_index) { // Прошли всю таблицу
                break;
            }
        }
        return false;
    }
};

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


intptr_t* __rbp;

intptr_t* __rsp; // указатель на текущую "вершину стека"

intptr_t* __stackBegin; // указатель на начало текущего фрейма стека 

#define __READ_RBP() __asm__ volatile("movq %%rbp,  %0" :  "=r"(__rbp))
#define __READ_RSP() __asm__ volatile("movq %%rsp,  %0" :  "=r"(__rsp))

void gcInit() {
    __READ_RBP();
    __stackBegin = (intptr_t*)*__rbp;
}

class GarbageCollector
{
private:
    CustomHashMap<void*, size_t> ref_count;
    CustomHashMap<void*, CustomVector<void**>> ptrs;

    void scanStack() {
        jmp_buf jb;
        cout << "SCANSTACK: " << '\n';
        cout << "+++" << '\n';
        __READ_RSP();
        auto rsp = (uint8_t*)__rsp;
        auto top = (uint8_t*)__stackBegin;
        cout << "rsp: " << (void*)rsp << ", top: " << (void*)top << '\n';
        cout << "+++" << '\n';
        int cnt = ref_count.Size();
        while (rsp < top) {
            auto address = (void*)*(uintptr_t*)rsp;
            cout << "Address on stack: " << address << '\n';
            if (ref_count.find(address)) {
                cout << "IS HERE: " << '\n';
                ref_count[address]++;
                cnt--;
            }
            //rsp += sizeof(uintptr_t); // Инкрементируем на размер указателя
            rsp += 1;
        }
    }

public:
    GarbageCollector() {
        cout << "GC constructor" << '\n';
    }

    void colect() {
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

        cout << "AFTER DELETING: \n";
        for (auto& pair : ref_count) {
            cout << pair.first << ' ' << pair.second << '\n';
        }
    }

    void registerPtr(void* ptr) {
        ref_count.insert(ptr, 1);
        ptrs.insert_or_assign(ptr, {});
    }

    ~GarbageCollector() {}
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
            cout << "Ptr was registered..." << '\n';
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


class MyClass {
public:
    int value;
    MyClass(int v) : value(v) { cout << "Create object " << this << endl; }
    ~MyClass() { cout << "Delete object " << this << endl; }
};

void foo1()
{
    for (int i = 0; i < 100; ++i) {
        MyClass* obj = new MyClass(i);
    }
}
void foo2()
{
    MyClass* ptr = new MyClass(1);
}
int main() {
    gcInit();
    cout << __rsp << ' ' << __rbp << '\n';
    cout << "EVERYTHING IS OKAY" << '\n';
    foo1();
    cout << "ptr out of scope" << '\n';
    gc.colect();
    return 0;
}

#pragma once

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
#pragma once

using namespace std;

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

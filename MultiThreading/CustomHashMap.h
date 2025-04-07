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
        return false; // ���������� �������� �� ���������, ���� �� �������
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

    // ���������
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
            // ���� ���� �� ������, ��������� ��� � ��������� �� ��������� � ���������� ������
            insert(key, ValueType());
            return table[hash(key)].second; // ���������� ������ �� ����������� �������.
        }
    }
    
    bool remove(const KeyType& key) {
        size_t index = hash(key);
        size_t initial_index = index;

        while (table[index].first != nullptr) {
            if (table[index].first == key) {
                table[index].first = nullptr; // �������� ��� ���������
                // Rehash ����������� ���������, ������� ����� ���� ��������� ����� ��������
                size_t next_index = (index + 1) % TABLE_SIZE;
                while (table[next_index].first != nullptr) {
                    KeyType temp_key = table[next_index].first;
                    ValueType temp_value = table[next_index].second;
                    table[next_index].first = nullptr;
                    size--; // ��������� ������ ����� ��������� ��������
                    insert(temp_key, temp_value);
                    next_index = (next_index + 1) % TABLE_SIZE;
                }
                size--;
                return true;
            }
            index = (index + 1) % TABLE_SIZE;
            if (index == initial_index) { // ������ ��� �������
                break;
            }
        }
        return false;
    }
};

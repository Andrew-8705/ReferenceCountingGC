#pragma once

#include "config.h"

class GarbageCollector {
private:

    GarbageCollector() = default;

	static intptr_t* __rbp;
	static intptr_t* __rsp;
	static intptr_t* __stackBegin;

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

        uintptr_t* rsp;
        __asm { mov rsp, esp }

        uintptr_t* top = (uintptr_t*)__stackBegin;

        while (rsp < top) {
            void* ptr = (void*)*rsp;
            cout << ptr << '\n';
            if (ref_count.find(ptr) != ref_count.end()) {
                cout << "POINTER WAS FOUND" << '\n';
                ref_count[ptr]++;
            }
            rsp += sizeof(uintptr_t);
            //rsp += 1;
        }
    }


public:
    static GarbageCollector& getInstance() {
        static GarbageCollector instance;
        return instance;
    }

    GarbageCollector(const GarbageCollector&) = delete;
    GarbageCollector& operator=(const GarbageCollector&) = delete;

    static void init() {
        __READ_RBP();
        __stackBegin = (intptr_t*)*__rbp;
        getInstance().initialized = true;
    }

    void collect() {
        cout << "[GC] reference counting...\n"; // Logger
        cout << "SIZE OF REF_COUNT: " << ref_count.size() << '\n'; // Logger
        for (auto& pair : ref_count) {
            cout << pair.first << ' ' << pair.second << '\n';
            pair.second = 0;
        }

        scanStack();

        cout << "AFTER COUNTING: " << '\n'; // Logger
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
                cout << "Key was deleted: " << key << '\n'; // Logger
            }
            delete(key);
        }

        cout << "AFTER DELETING: \n"; // Logger
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
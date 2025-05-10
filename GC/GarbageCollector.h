#pragma once

#include "config.h"

class GarbageCollector {
private:

    GarbageCollector() = default;

	static intptr_t* __rbp;
	static intptr_t* __rsp;
	static intptr_t* __stackBegin;

    static uintptr_t* get_current_rsp() {
        uintptr_t* rsp;

        #if defined(_MSC_VER)
        #if defined(_M_IX86)
                __asm { mov rsp, esp }
        #endif
        #elif defined(__GNUC__) || defined(__clang__)
        #if defined(__i386__)
                __asm__ volatile("movl %%esp, %0" : "=r"(rsp));
        #elif defined(__x86_64__)
                __asm__ volatile("movq %%rsp, %0" : "=r"(rsp));
        #endif
        #endif

        return rsp;
    }

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
        GC_LOG_DEBUG("SCANNING STACK");

        uintptr_t* rsp = get_current_rsp();
        uintptr_t* top = (uintptr_t*)__stackBegin;

        while (rsp < top) {
            void* ptr = (void*)*rsp;
            GC_LOG_DEBUG("Checking pointer: " << ptr);
            if (ref_count.find(ptr) != ref_count.end()) {
                GC_LOG_DEBUG("Pointer found in ref_count: " << ptr);
                ref_count[ptr]++;
            }
            //rsp++;
            rsp += sizeof(uintptr_t*);
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
        GC_FUNC_TRACE();
        __READ_RBP();
        __stackBegin = (intptr_t*)*__rbp;
        getInstance().initialized = true;
        GC_LOG_INFO("Garbage collector initialized");
    }

    void collect() {
        GC_LOG_INFO("[GC] Starting collection");
        GC_LOG_DEBUG("Ref count size: " << ref_count.size());
        for (auto& pair : ref_count) {
            GC_LOG_DEBUG("Resetting counter for: " << pair.first);
            pair.second = 0;
        }

        scanStack();

        GC_LOG_DEBUG("After counting:");
        for (const auto& pair : ref_count) GC_LOG_DEBUG(pair.first << " : " << pair.second);

        std::vector<void*> keys_to_delete;

        for (const auto& pair : ref_count) {
            if (pair.second == 0) {
                keys_to_delete.push_back(pair.first);
            }
        }

        for (void* key : keys_to_delete) {
            size_t cnt = ref_count.erase(key);
            if (cnt > 0) {
                GC_LOG_INFO("Deleting key: " << key);
            }
            delete(key);
        }

        GC_LOG_DEBUG("After deleting:");
        for (auto& pair : ref_count) {
            GC_LOG_DEBUG(pair.first << " : " << pair.second);
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
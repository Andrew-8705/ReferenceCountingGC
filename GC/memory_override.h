#pragma once

#include "GarbageCollector.h"
#include <new>

using namespace std;

void* operator new(size_t size) noexcept(false) {
    if (size == 0) size = 1;

    while (true) {
        void* ptr = malloc(size);
        if (ptr) {
            GarbageCollector::getInstance().registerPtr(ptr);
            // Logger
            return ptr;
        }

        new_handler handler = get_new_handler();
        if (!handler) {
            throw bad_alloc();
        }
        (*handler)();
    }
}

void* operator new[](size_t size) {
    return operator new(size);
}

void operator delete(void* ptr) noexcept {
    if (!ptr) return;
    if (GarbageCollector::getInstance().findPtr(ptr)) {
        // Logger: cout << "DELETE THIS PTR: " << ptr << '\n';
        GarbageCollector::getInstance().decPtr(ptr);
        GarbageCollector::getInstance().removePtr(ptr);
    }
    else free(ptr);
}

void operator delete[](void* ptr) noexcept {
    operator delete(ptr);
}

void operator delete(void* ptr, std::nothrow_t) noexcept {
    operator delete(ptr);
}

void operator delete[](void* ptr, std::nothrow_t) noexcept {
    operator delete(ptr);
}
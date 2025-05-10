#pragma once

#include "GarbageCollector.h"
#include <new>

using namespace std;

void* operator new(size_t size) noexcept(false) {
    GC_FUNC_TRACE();
    if (size == 0) size = 1;

    while (true) {
        void* ptr = malloc(size);
        if (ptr) {
            GC_LOG_DEBUG("Allocated " << size << " bytes at " << ptr);
            GarbageCollector::getInstance().registerPtr(ptr);
            return ptr;
        }

        new_handler handler = get_new_handler();
        if (!handler) {
            GC_LOG_ERROR("Allocation failed for size: " << size);
            throw bad_alloc();
        }
        GC_LOG_DEBUG("Invoking new_handler");
        (*handler)();
    }
}

void* operator new[](size_t size) {
    return operator new(size);
}

void operator delete(void* ptr) noexcept {
    if (!ptr) return;
    GC_LOG_DEBUG("Deleting pointer: " << ptr);
    if (GarbageCollector::getInstance().findPtr(ptr)) {
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
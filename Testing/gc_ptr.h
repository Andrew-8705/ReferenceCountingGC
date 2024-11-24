#pragma once

#include <stdexcept>

using namespace std;

template<typename T>
class ControlBlock {
public:
    T* ptr;
    int ref_count;

    ControlBlock(T* p) : ptr(p), ref_count(1) {}

    ~ControlBlock() { delete ptr; }
};

template<typename T>
class gc_ptr {
private:
    ControlBlock<T>* control_block;

    void release() {
        if (control_block) {
            control_block->ref_count--;
            if (control_block->ref_count == 0) {
                delete control_block;
            }
        }
    }

public:
    gc_ptr() : control_block(nullptr) {}

    explicit gc_ptr(T* ptr) {
        control_block = new ControlBlock<T>(ptr);
    }

    gc_ptr(const gc_ptr& other) : control_block(other.control_block) {
        if (control_block) {
            control_block->ref_count++;
        }
    }

    gc_ptr(gc_ptr&& other) noexcept : control_block(other.control_block) {
        other.control_block = nullptr;
    }

    gc_ptr& operator=(const gc_ptr& other) {
        if (this != &other) {
            release();
            control_block = other.control_block;
            if (control_block) {
                control_block->ref_count++;
            }
        }
        return *this;
    }

    gc_ptr& operator=(gc_ptr&& other) noexcept {
        if (this != &other) {
            release();
            control_block = other.control_block;
            other.control_block = nullptr;
        }
        return *this;
    }


    ~gc_ptr() {
        release();
    }

    T& operator*() const {
        if (control_block == nullptr)
            throw runtime_error("Accessing null pointer");
        return *(control_block->ptr);
    }

    T* operator->() const {
        if (control_block == nullptr)
            throw runtime_error("Accessing null pointer");
        return control_block->ptr;
    }

    int count() const {
        return control_block ? control_block->ref_count : 0;
    }

    bool is_null() const {
        return control_block == nullptr;
    }
};
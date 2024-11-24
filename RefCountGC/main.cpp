#include <iostream>

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

class B;

class A {
public:
    gc_ptr<B> a_ptr;

    A() { cout << "A created" << '\n'; }
    ~A() { cout << "A destroyed" << '\n'; }
    void just_do() { cout << "A doing something" << '\n'; }
};

class B {
public:
    gc_ptr<A>  b_ptr;
    B() { cout << "B created" << '\n'; }
    ~B() { cout << "B destroyed" << '\n'; }
    void just_do() { cout << "B doing something" << '\n'; }
};


template <typename T>
gc_ptr<T> gc_new() {
    return gc_ptr<T>(new T());
}

int main() {
    /*A a;
    gc_ptr<A> ptr_a1 = gc_new<A>();
    gc_ptr<A> ptr_a2 = gc_new<A>();*/

    gc_ptr<A> a = gc_new<A>();
    gc_ptr<B> b = gc_new<B>();
    gc_ptr<B> c = b;
    gc_ptr<B> d = gc_new<B>();

    cout << a.count() << ' ' << b.count() << ' ' << c.count() << '\n';

    a->just_do();
    b->just_do();

    //не работают циклические ссылки
    //a->a_ptr = b;
    //b->b_ptr = a;
}


// добавлены необходимые проверки на null ptr
// добавлен метод для проверки на nul ptr
// добавлены методы перемещения (move semantics) для обработки правых ссылок

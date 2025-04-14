#include <iostream>
#include <intrin.h>
#include <stdint.h>
#include <stdio.h>

#include <cstdlib>
#include <cstring>
#include <cassert>
#include <memory>
#include <vector>
#include <Windows.h>
#include <setjmp.h>
#include <dbghelp.h>
#include <algorithm>
#include <map>
#include <unordered_map>
#include <set>

#pragma comment(lib, "dbghelp.lib") // для того чтобы автоматически добавить dbghelp в список библиотек для линковщика

using namespace std;

intptr_t* __rbp;
intptr_t* __rsp;
intptr_t* __stackBegin;

// Чтение RBP (x86)
#define __READ_RBP() __asm { mov __rbp, ebp }

// Чтение RSP (x86)
#define __READ_RSP() __asm { mov __rsp, esp }

void gcInit() {
    __READ_RBP();
    __stackBegin = (intptr_t*)*__rbp;
}

bool isHeapPointer(void* ptr) {
    if (!ptr) return false; // nullptr не считается валидным

    // Получаем информацию о текущем процессе
    HANDLE process = GetCurrentProcess();
    MEMORY_BASIC_INFORMATION memInfo;

    // Запрашиваем информацию о памяти по указателю
    if (VirtualQueryEx(process, ptr, &memInfo, sizeof(memInfo))) {
        // Проверяем, что память выделена и принадлежит куче
        return (memInfo.State == MEM_COMMIT) &&
            (memInfo.Type == MEM_PRIVATE) &&
            (memInfo.Protect & (PAGE_READWRITE | PAGE_READONLY));
    }
    return false;
}

bool IsSTLAllocation(const std::vector<std::string>& callstack) {
    // Список ключевых STL-функций аллокации
    const std::vector<std::string> stlKeywords = {
        "std::", "allocator", "_Allocate", "::allocate",
        // Контейнеры
        "vector", "deque", "list", "map", "set", "queue", "stack",
        "forward_list", "unordered_map", "unordered_set",
        "multimap", "multiset", "priority_queue",
        // Вспомогательные классы
        "basic_string", "regex", "function", "shared_ptr",
        // Аллокаторы
        "allocator_traits", "scoped_allocator", "polymorphic_allocator"
    };

    for (const auto& func : callstack) {
        for (const auto& keyword : stlKeywords) {
            if (func.find(keyword) != std::string::npos) {
                return true;
            }
        }
    }
    return false;
}

bool IsUserConstructor(const std::string& funcName) {
    // Ищем шаблон ClassName::ClassName
    size_t colonPos = funcName.find("::");
    if (colonPos == std::string::npos) return false;

    // Проверяем что после второго двоеточия ничего нет
    size_t secondColon = funcName.find("::", colonPos + 2);
    if (secondColon != std::string::npos) return false;

    // Проверяем что имя функции совпадает с именем класса
    std::string className = funcName.substr(0, colonPos);
    std::string ctorName = funcName.substr(colonPos + 2);
    return className == ctorName;
}

class GarbageCollector
{
private:
    //CustomHashMap<void*, size_t> ref_count;
    //CustomHashMap<void*, CustomVector<void**>> ptrs;
    unordered_map<void*, size_t> ref_count;

    /*void scanStack() {
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
            rsp += 1;
        }
    }*/
    void scanStack() {
        jmp_buf jb;
        setjmp(jb); // Сохраняем контекст

        uintptr_t* rsp;
        __asm { mov rsp, esp } // Получаем текущий RSP

        uintptr_t* top = (uintptr_t*)__stackBegin;

        while (rsp < top) {
            void* ptr = (void*)*rsp;
            if (isHeapPointer(ptr) && (ref_count.find(ptr) != ref_count.end())) {
                ref_count[ptr]++;
            }
            rsp += sizeof(uintptr_t);
        }
    }

    void scanRegisters() {
        void* regs[4]; // EAX, EBX, ECX, EDX
        __asm {
            mov regs[0], eax
            mov regs[1], ebx
            mov regs[2], ecx
            mov regs[3], edx
        }

        for (void* ptr : regs) {
            if (isHeapPointer(ptr) && (ref_count.find(ptr) != ref_count.end())) {
                ref_count[ptr]++;
            }
        }
    }

public:
    GarbageCollector() {
        cout << "GC constructor" << '\n';
    }

    void colect() {
        cout << "[GC] reference counting...\n";
        //cout << "SIZE OF REF_COUNT: " << ref_count.Size() << '\n';
        cout << "SIZE OF REF_COUNT: " << ref_count.size() << '\n';
        for (auto& pair : ref_count) {
            cout << pair.first << ' ' << pair.second << '\n';
            pair.second = 0;
        }

        scanStack();
        scanRegisters();

        /*cout << "AFTER COUNTING: \n";
        for (auto& pair : ref_count) {
            cout << pair.first << ' ' << pair.second << '\n';
            if (pair.second == 0) {
                delete pair.first;
                size_t cnt = ref_count.erase(pair.first);
            }
        }*/
        cout << "AFTER COUNTING: \n";
        std::vector<void*> keys_to_delete;
        // Итерируем по таблице и собираем ключи для удаления
        for (const auto& pair : ref_count) {
            std::cout << pair.first << ' ' << pair.second << '\n';
            if (pair.second == 0) {
                keys_to_delete.push_back(pair.first);
            }
        }

        // Удаляем элементы после завершения итерации
        for (void* key : keys_to_delete) {
            size_t cnt = ref_count.erase(key);
            if (cnt > 0) {
                std::cout << "Key was deleted: " << key << std::endl;
            }
            delete(key);
        }

        cout << "AFTER DELETING: \n";
        for (auto& pair : ref_count) {
            cout << pair.first << ' ' << pair.second << '\n';
        }
    }

    void registerPtr(void* ptr) {
        ref_count[ptr] = 1;
        //ptrs.insert_or_assign(ptr, {});
    }

    ~GarbageCollector() {}
};

GarbageCollector gc;
void* last_ptr = nullptr;

//// переопределяем глобально new и delete
//void* operator new(size_t n) {
//    cout << "Use operator new..." << '\n';
//    if (n == 0) {
//        n = 1;
//    }
//
//    while (true) {
//        void* p = malloc(n); // Используем malloc
//        if (p) {
//            gc.registerPtr(p);
//            cout << "Ptr was registered..." << '\n';
//            last_ptr = p;
//            return p;
//        }
//
//        new_handler handle = get_new_handler();
//        if (!handle) {
//            throw bad_alloc();
//        }
//
//        (*handle)();
//    }
//}

static bool g_inAlloc = false;

void* operator new(size_t size) {
    if (g_inAlloc) return malloc(size);
    g_inAlloc = true;

    void* ptr = malloc(size);

    // Получаем полный стек вызовов
    HANDLE process = GetCurrentProcess(); // Получаем дескриптор процесса

    SymInitialize(process, NULL, TRUE); // Инициализация символьной системы для процессора
    // (дескриптор, путь к символам(NULL = стандартный путь), TRUE - загружать символы всех модулей)

    SymSetOptions(SYMOPT_UNDNAME | SYMOPT_DEFERRED_LOADS); // SYMOPT_UNDNAME - преобразовать в читаемый вид, SYMOPT_DEFERRED_LOADS - отложенная загрузка символов

    void* stack[20];
    USHORT frames = CaptureStackBackTrace(0, 20, stack, NULL);
    // (пропустить первые n фреймов, макс число фреймов, указатель на массив для хранения, NULL - не использовать хэш)

    static int counter = 0;
    counter++;
    cout << counter << '\n';
    std::vector<std::string> callstack;
    char buffer[sizeof(SYMBOL_INFO) + MAX_SYM_NAME] = { 0 };
    SYMBOL_INFO* symbol = (SYMBOL_INFO*)buffer;
    symbol->SizeOfStruct = sizeof(SYMBOL_INFO);
    symbol->MaxNameLen = MAX_SYM_NAME;

    cout << "Stack of functions: " << '\n';
    for (USHORT i = 0; i < frames; ++i) {
        if (SymFromAddr(process, (DWORD64)stack[i], NULL, symbol)) { // получение символа по адресу
            callstack.push_back(symbol->Name);
            cout << symbol->Name << '\n';
        }
    }

    // Анализируем стек
    if (IsSTLAllocation(callstack)) {
        cout << "NEW for STL container!!!" << '\n';
        g_inAlloc = false;
        last_ptr = ptr;
        return ptr;
    }
    else {
        // Ищем пользовательский конструктор
        for (const auto& func : callstack) {
            if (IsUserConstructor(func)) {
                std::cout << "User constructor: " << func << " (" << size << " bytes)\n";
                SymCleanup(process);
                g_inAlloc = false;
                gc.registerPtr(ptr);
                cout << "Ptr was registered in 'constructor'..." << '\n';
                last_ptr = ptr;
                return ptr;
            }
        }

        // Если конструктор не нашли, но вызов был из main или другой пользовательской функции
        bool fromUserCode = std::any_of(callstack.begin(), callstack.end(),
            [](const std::string& func) {
                return func.find("main") != std::string::npos ||
                    func.find("::") == std::string::npos;
            });

        if (fromUserCode) {
            std::cout << "User allocation: " << size << " bytes\n";
            gc.registerPtr(ptr);
            cout << "Ptr was registered in 'fromUserCode'..." << '\n';
        }
        else {
            std::cout << "Internal allocation: " << size << " bytes\n";
        }
    }

    SymCleanup(process);
    g_inAlloc = false;
    last_ptr = ptr;
    return ptr;
}

void operator delete(void* ptr) noexcept {
    free(ptr);
}


class MyClass {
public:
    int value;
    int* ptr;
    MyClass(int v) : value(v) {
        cout << "Create object " << this << endl;
        ptr = new int(1);
    }
    ~MyClass() { cout << "Delete object " << this << endl; }
};

void foo1()
{
    vector<MyClass*> v(1000);
    for (int i = 0; i < 1000; ++i) {
        v[i] = new MyClass(i);
    }
}
void foo2()
{
    MyClass* ptr = new MyClass(1);
    MyClass* ptr10 = ptr;
    MyClass* ptr1 = new MyClass(42);
    int* n = new int(10);
    //gc.colect();
    //MyClass* ptr2 = new MyClass(51);
    //cout << "COLECT INSIDE foo2" << '\n';
    //gc.colect();
}
int main() {
    gcInit();
    //vector<int> v(10);
    //map<int, string> m;
    //m[10] = "apple";
    cout << __rsp << ' ' << __rbp << '\n';
    cout << "EVERYTHING IS OKAY" << '\n';
    foo2();
    vector<int> v(100);
    map<int, string> m;
    set<string> s;
    cout << "ptr out of scope" << '\n';
    gc.colect();
    cout << "last_ptr: " << last_ptr << '\n';
    cout << "second time collect" << '\n';
    //gc.colect();
    return 0;
}


/* Example of function call stack
Stack of functions:
operator new
std::_Default_allocate_traits::_Allocate
std::_Allocate<8,std::_Default_allocate_traits>
std::allocator<std::_Container_proxy>::allocate
std::_Container_proxy_ptr12<std::allocator<std::_Container_proxy> >::_Container_proxy_ptr12<std::allocator<std::_Container_proxy> >
std::list<std::pair<void * const,unsigned int>,std::allocator<std::pair<void * const,unsigned int> > >::_Alloc_sentinel_and_proxy
std::list<std::pair<void * const,unsigned int>,std::allocator<std::pair<void * const,unsigned int> > >::list<std::pair<void * const,unsigned int>,std::allocator<std::pair<void * const,unsigned int> > >
std::_Hash<std::_Umap_traits<void *,unsigned int,std::_Uhash_compare<void *,std::hash<void *>,std::equal_to<void *> >,std::allocator<std::pair<void * const,unsigned int> >,0> >::_Hash<std::_Umap_traits<void *,unsigned int,std::_Uhash_compare<void *,std::hash<void *>,std::equal_to<void *> >,std::allocator<std::pair<void * const,unsigned int> >,0> >
std::unordered_map<void *,unsigned int,std::hash<void *>,std::equal_to<void *>,std::allocator<std::pair<void * const,unsigned int> > >::unordered_map<void *,unsigned int,std::hash<void *>,std::equal_to<void *>,std::allocator<std::pair<void * const,unsigned int> > >
GarbageCollector::GarbageCollector
`dynamic initializer for 'gc''
initterm
__scrt_common_main_seh
__scrt_common_main
mainCRTStartup
timeGetTime
timeGetTime
fltused
fltused
fltused
*/

/* Another example
Stack of functions:
operator new
MyClass::MyClass
foo2
main
invoke_main
__scrt_common_main_seh
__scrt_common_main
mainCRTStartup
timeGetTime
timeGetTime
fltused
fltused
fltused
User constructor: MyClass::MyClass (4 bytes)
*/

/* Another example
Stack of functions:
operator new
foo2
main
invoke_main
__scrt_common_main_seh
__scrt_common_main
mainCRTStartup
timeGetTime
timeGetTime
fltused
fltused
fltused
User allocation: 8 bytes
Ptr was registered in 'fromUserCode'...
*/

/* Example of output
1
Stack of functions:
operator new
std::_Default_allocate_traits::_Allocate
std::_Allocate<8,std::_Default_allocate_traits>
std::allocator<std::_Container_proxy>::allocate
std::_Container_proxy_ptr12<std::allocator<std::_Container_proxy> >::_Container_proxy_ptr12<std::allocator<std::_Container_proxy> >
std::list<std::pair<void * const,unsigned int>,std::allocator<std::pair<void * const,unsigned int> > >::_Alloc_sentinel_and_proxy
std::list<std::pair<void * const,unsigned int>,std::allocator<std::pair<void * const,unsigned int> > >::list<std::pair<void * const,unsigned int>,std::allocator<std::pair<void * const,unsigned int> > >
std::_Hash<std::_Umap_traits<void *,unsigned int,std::_Uhash_compare<void *,std::hash<void *>,std::equal_to<void *> >,std::allocator<std::pair<void * const,unsigned int> >,0> >::_Hash<std::_Umap_traits<void *,unsigned int,std::_Uhash_compare<void *,std::hash<void *>,std::equal_to<void *> >,std::allocator<std::pair<void * const,unsigned int> >,0> >
std::unordered_map<void *,unsigned int,std::hash<void *>,std::equal_to<void *>,std::allocator<std::pair<void * const,unsigned int> > >::unordered_map<void *,unsigned int,std::hash<void *>,std::equal_to<void *>,std::allocator<std::pair<void * const,unsigned int> > >
GarbageCollector::GarbageCollector
`dynamic initializer for 'gc''
initterm
__scrt_common_main_seh
__scrt_common_main
mainCRTStartup
timeGetTime
timeGetTime
fltused
fltused
fltused
NEW for STL container!!!
2
Stack of functions:
operator new
std::_Default_allocate_traits::_Allocate
std::_Allocate<8,std::_Default_allocate_traits>
std::allocator<std::_List_node<std::pair<void * const,unsigned int>,void *> >::allocate
std::list<std::pair<void * const,unsigned int>,std::allocator<std::pair<void * const,unsigned int> > >::_Alloc_sentinel_and_proxy
std::list<std::pair<void * const,unsigned int>,std::allocator<std::pair<void * const,unsigned int> > >::list<std::pair<void * const,unsigned int>,std::allocator<std::pair<void * const,unsigned int> > >
std::_Hash<std::_Umap_traits<void *,unsigned int,std::_Uhash_compare<void *,std::hash<void *>,std::equal_to<void *> >,std::allocator<std::pair<void * const,unsigned int> >,0> >::_Hash<std::_Umap_traits<void *,unsigned int,std::_Uhash_compare<void *,std::hash<void *>,std::equal_to<void *> >,std::allocator<std::pair<void * const,unsigned int> >,0> >
std::unordered_map<void *,unsigned int,std::hash<void *>,std::equal_to<void *>,std::allocator<std::pair<void * const,unsigned int> > >::unordered_map<void *,unsigned int,std::hash<void *>,std::equal_to<void *>,std::allocator<std::pair<void * const,unsigned int> > >
GarbageCollector::GarbageCollector
`dynamic initializer for 'gc''
initterm
__scrt_common_main_seh
__scrt_common_main
mainCRTStartup
timeGetTime
timeGetTime
fltused
fltused
fltused
NEW for STL container!!!
3
Stack of functions:
operator new
std::_Default_allocate_traits::_Allocate
std::_Allocate<8,std::_Default_allocate_traits>
std::allocator<std::_Container_proxy>::allocate
std::_Container_base12::_Alloc_proxy<std::allocator<std::_Container_proxy> >
std::_Hash_vec<std::allocator<std::_List_unchecked_iterator<std::_List_val<std::_List_simple_types<std::pair<void * const,unsigned int> > > > > >::_Hash_vec<std::allocator<std::_List_unchecked_iterator<std::_List_val<std::_List_simple_types<std::pair<void * const,unsigned int> > > > > ><std::allocator<std::pair<void * const,unsigned int> > const &,0>
std::_Hash<std::_Umap_traits<void *,unsigned int,std::_Uhash_compare<void *,std::hash<void *>,std::equal_to<void *> >,std::allocator<std::pair<void * const,unsigned int> >,0> >::_Hash<std::_Umap_traits<void *,unsigned int,std::_Uhash_compare<void *,std::hash<void *>,std::equal_to<void *> >,std::allocator<std::pair<void * const,unsigned int> >,0> >
std::unordered_map<void *,unsigned int,std::hash<void *>,std::equal_to<void *>,std::allocator<std::pair<void * const,unsigned int> > >::unordered_map<void *,unsigned int,std::hash<void *>,std::equal_to<void *>,std::allocator<std::pair<void * const,unsigned int> > >
GarbageCollector::GarbageCollector
`dynamic initializer for 'gc''
initterm
__scrt_common_main_seh
__scrt_common_main
mainCRTStartup
timeGetTime
timeGetTime
fltused
fltused
fltused
NEW for STL container!!!
4
Stack of functions:
operator new
std::_Default_allocate_traits::_Allocate
std::_Allocate<8,std::_Default_allocate_traits>
std::allocator<std::_List_unchecked_iterator<std::_List_val<std::_List_simple_types<std::pair<void * const,unsigned int> > > > >::allocate
std::_Hash_vec<std::allocator<std::_List_unchecked_iterator<std::_List_val<std::_List_simple_types<std::pair<void * const,unsigned int> > > > > >::_Assign_grow
std::_Hash<std::_Umap_traits<void *,unsigned int,std::_Uhash_compare<void *,std::hash<void *>,std::equal_to<void *> >,std::allocator<std::pair<void * const,unsigned int> >,0> >::_Hash<std::_Umap_traits<void *,unsigned int,std::_Uhash_compare<void *,std::hash<void *>,std::equal_to<void *> >,std::allocator<std::pair<void * const,unsigned int> >,0> >
std::unordered_map<void *,unsigned int,std::hash<void *>,std::equal_to<void *>,std::allocator<std::pair<void * const,unsigned int> > >::unordered_map<void *,unsigned int,std::hash<void *>,std::equal_to<void *>,std::allocator<std::pair<void * const,unsigned int> > >
GarbageCollector::GarbageCollector
`dynamic initializer for 'gc''
initterm
__scrt_common_main_seh
__scrt_common_main
mainCRTStartup
timeGetTime
timeGetTime
fltused
fltused
fltused
NEW for STL container!!!
GC constructor
00000000 00AFFA1C
EVERYTHING IS OKAY
5
Stack of functions:
operator new
foo2
main
invoke_main
__scrt_common_main_seh
__scrt_common_main
mainCRTStartup
timeGetTime
timeGetTime
fltused
fltused
fltused
User allocation: 8 bytes
Ptr was registered in 'fromUserCode'...
Create object 077657F8
6
Stack of functions:
operator new
MyClass::MyClass
foo2
main
invoke_main
__scrt_common_main_seh
__scrt_common_main
mainCRTStartup
timeGetTime
timeGetTime
fltused
fltused
fltused
User constructor: MyClass::MyClass (4 bytes)
7
Stack of functions:
operator new
std::_Default_allocate_traits::_Allocate
std::_Allocate<8,std::_Default_allocate_traits>
std::allocator<std::_List_node<std::pair<void * const,unsigned int>,void *> >::allocate
std::_Alloc_construct_ptr<std::allocator<std::_List_node<std::pair<void * const,unsigned int>,void *> > >::_Allocate
std::_List_node_emplace_op2<std::allocator<std::_List_node<std::pair<void * const,unsigned int>,void *> > >::_List_node_emplace_op2<std::allocator<std::_List_node<std::pair<void * const,unsigned int>,void *> > ><std::piecewise_construct_t const &,std::tuple<void * const &>,std::tuple<> >
std::_Hash<std::_Umap_traits<void *,unsigned int,std::_Uhash_compare<void *,std::hash<void *>,std::equal_to<void *> >,std::allocator<std::pair<void * const,unsigned int> >,0> >::_Try_emplace<void * const &>
std::unordered_map<void *,unsigned int,std::hash<void *>,std::equal_to<void *>,std::allocator<std::pair<void * const,unsigned int> > >::operator[]
GarbageCollector::registerPtr
operator new
MyClass::MyClass
foo2
main
invoke_main
__scrt_common_main_seh
__scrt_common_main
mainCRTStartup
timeGetTime
timeGetTime
fltused
NEW for STL container!!!
Ptr was registered in 'constructor'...
8
Stack of functions:
operator new
foo2
main
invoke_main
__scrt_common_main_seh
__scrt_common_main
mainCRTStartup
timeGetTime
timeGetTime
fltused
fltused
fltused
User allocation: 8 bytes
Ptr was registered in 'fromUserCode'...
Create object 0770A400
9
Stack of functions:
operator new
MyClass::MyClass
foo2
main
invoke_main
__scrt_common_main_seh
__scrt_common_main
mainCRTStartup
timeGetTime
timeGetTime
fltused
fltused
fltused
User constructor: MyClass::MyClass (4 bytes)
10
Stack of functions:
operator new
std::_Default_allocate_traits::_Allocate
std::_Allocate<8,std::_Default_allocate_traits>
std::allocator<std::_List_node<std::pair<void * const,unsigned int>,void *> >::allocate
std::_Alloc_construct_ptr<std::allocator<std::_List_node<std::pair<void * const,unsigned int>,void *> > >::_Allocate
std::_List_node_emplace_op2<std::allocator<std::_List_node<std::pair<void * const,unsigned int>,void *> > >::_List_node_emplace_op2<std::allocator<std::_List_node<std::pair<void * const,unsigned int>,void *> > ><std::piecewise_construct_t const &,std::tuple<void * const &>,std::tuple<> >
std::_Hash<std::_Umap_traits<void *,unsigned int,std::_Uhash_compare<void *,std::hash<void *>,std::equal_to<void *> >,std::allocator<std::pair<void * const,unsigned int> >,0> >::_Try_emplace<void * const &>
std::unordered_map<void *,unsigned int,std::hash<void *>,std::equal_to<void *>,std::allocator<std::pair<void * const,unsigned int> > >::operator[]
GarbageCollector::registerPtr
operator new
MyClass::MyClass
foo2
main
invoke_main
__scrt_common_main_seh
__scrt_common_main
mainCRTStartup
timeGetTime
timeGetTime
fltused
NEW for STL container!!!
Ptr was registered in 'constructor'...
11
Stack of functions:
operator new
foo2
main
invoke_main
__scrt_common_main_seh
__scrt_common_main
mainCRTStartup
timeGetTime
timeGetTime
fltused
fltused
fltused
User allocation: 4 bytes
Ptr was registered in 'fromUserCode'...
12
Stack of functions:
operator new
std::_Default_allocate_traits::_Allocate
std::_Allocate<8,std::_Default_allocate_traits>
std::allocator<std::_Container_proxy>::allocate
std::_Container_proxy_ptr12<std::allocator<std::_Container_proxy> >::_Container_proxy_ptr12<std::allocator<std::_Container_proxy> >
std::vector<int,std::allocator<int> >::_Construct_n<>
std::vector<int,std::allocator<int> >::vector<int,std::allocator<int> >
main
invoke_main
__scrt_common_main_seh
__scrt_common_main
mainCRTStartup
timeGetTime
timeGetTime
fltused
fltused
fltused
NEW for STL container!!!
13
Stack of functions:
operator new
std::_Default_allocate_traits::_Allocate
std::_Allocate<8,std::_Default_allocate_traits>
std::allocator<int>::allocate
std::_Allocate_at_least_helper<std::allocator<int> >
std::vector<int,std::allocator<int> >::_Buy_raw
std::vector<int,std::allocator<int> >::_Buy_nonzero
std::vector<int,std::allocator<int> >::_Construct_n<>
std::vector<int,std::allocator<int> >::vector<int,std::allocator<int> >
main
invoke_main
__scrt_common_main_seh
__scrt_common_main
mainCRTStartup
timeGetTime
timeGetTime
fltused
fltused
fltused
NEW for STL container!!!
14
Stack of functions:
operator new
std::_Default_allocate_traits::_Allocate
std::_Allocate<8,std::_Default_allocate_traits>
std::allocator<std::_Container_proxy>::allocate
std::_Container_proxy_ptr12<std::allocator<std::_Container_proxy> >::_Container_proxy_ptr12<std::allocator<std::_Container_proxy> >
std::_Tree<std::_Tmap_traits<int,std::basic_string<char,std::char_traits<char>,std::allocator<char> >,std::less<int>,std::allocator<std::pair<int const ,std::basic_string<char,std::char_traits<char>,std::allocator<char> > > >,0> >::_Alloc_sentinel_and_proxy
std::_Tree<std::_Tmap_traits<int,std::basic_string<char,std::char_traits<char>,std::allocator<char> >,std::less<int>,std::allocator<std::pair<int const ,std::basic_string<char,std::char_traits<char>,std::allocator<char> > > >,0> >::_Tree<std::_Tmap_traits<int,std::basic_string<char,std::char_traits<char>,std::allocator<char> >,std::less<int>,std::allocator<std::pair<int const ,std::basic_string<char,std::char_traits<char>,std::allocator<char> > > >,0> >
std::map<int,std::basic_string<char,std::char_traits<char>,std::allocator<char> >,std::less<int>,std::allocator<std::pair<int const ,std::basic_string<char,std::char_traits<char>,std::allocator<char> > > > >::map<int,std::basic_string<char,std::char_traits<char>,std::allocator<char> >,std::less<int>,std::allocator<std::pair<int const ,std::basic_string<char,std::char_traits<char>,std::allocator<char> > > > >
main
invoke_main
__scrt_common_main_seh
__scrt_common_main
mainCRTStartup
timeGetTime
timeGetTime
fltused
fltused
fltused
NEW for STL container!!!
15
Stack of functions:
operator new
std::_Default_allocate_traits::_Allocate
std::_Allocate<8,std::_Default_allocate_traits>
std::allocator<std::_Tree_node<std::pair<int const ,std::basic_string<char,std::char_traits<char>,std::allocator<char> > >,void *> >::allocate
std::_Tree_node<std::pair<int const ,std::basic_string<char,std::char_traits<char>,std::allocator<char> > >,void *>::_Buyheadnode<std::allocator<std::_Tree_node<std::pair<int const ,std::basic_string<char,std::char_traits<char>,std::allocator<char> > >,void *> > >
std::_Tree<std::_Tmap_traits<int,std::basic_string<char,std::char_traits<char>,std::allocator<char> >,std::less<int>,std::allocator<std::pair<int const ,std::basic_string<char,std::char_traits<char>,std::allocator<char> > > >,0> >::_Alloc_sentinel_and_proxy
std::_Tree<std::_Tmap_traits<int,std::basic_string<char,std::char_traits<char>,std::allocator<char> >,std::less<int>,std::allocator<std::pair<int const ,std::basic_string<char,std::char_traits<char>,std::allocator<char> > > >,0> >::_Tree<std::_Tmap_traits<int,std::basic_string<char,std::char_traits<char>,std::allocator<char> >,std::less<int>,std::allocator<std::pair<int const ,std::basic_string<char,std::char_traits<char>,std::allocator<char> > > >,0> >
std::map<int,std::basic_string<char,std::char_traits<char>,std::allocator<char> >,std::less<int>,std::allocator<std::pair<int const ,std::basic_string<char,std::char_traits<char>,std::allocator<char> > > > >::map<int,std::basic_string<char,std::char_traits<char>,std::allocator<char> >,std::less<int>,std::allocator<std::pair<int const ,std::basic_string<char,std::char_traits<char>,std::allocator<char> > > > >
main
invoke_main
__scrt_common_main_seh
__scrt_common_main
mainCRTStartup
timeGetTime
timeGetTime
fltused
fltused
fltused
NEW for STL container!!!
16
Stack of functions:
operator new
std::_Default_allocate_traits::_Allocate
std::_Allocate<8,std::_Default_allocate_traits>
std::allocator<std::_Container_proxy>::allocate
std::_Container_proxy_ptr12<std::allocator<std::_Container_proxy> >::_Container_proxy_ptr12<std::allocator<std::_Container_proxy> >
std::_Tree<std::_Tset_traits<std::basic_string<char,std::char_traits<char>,std::allocator<char> >,std::less<std::basic_string<char,std::char_traits<char>,std::allocator<char> > >,std::allocator<std::basic_string<char,std::char_traits<char>,std::allocator<char> > >,0> >::_Alloc_sentinel_and_proxy
std::_Tree<std::_Tset_traits<std::basic_string<char,std::char_traits<char>,std::allocator<char> >,std::less<std::basic_string<char,std::char_traits<char>,std::allocator<char> > >,std::allocator<std::basic_string<char,std::char_traits<char>,std::allocator<char> > >,0> >::_Tree<std::_Tset_traits<std::basic_string<char,std::char_traits<char>,std::allocator<char> >,std::less<std::basic_string<char,std::char_traits<char>,std::allocator<char> > >,std::allocator<std::basic_string<char,std::char_traits<char>,std::allocator<char> > >,0> >
std::set<std::basic_string<char,std::char_traits<char>,std::allocator<char> >,std::less<std::basic_string<char,std::char_traits<char>,std::allocator<char> > >,std::allocator<std::basic_string<char,std::char_traits<char>,std::allocator<char> > > >::set<std::basic_string<char,std::char_traits<char>,std::allocator<char> >,std::less<std::basic_string<char,std::char_traits<char>,std::allocator<char> > >,std::allocator<std::basic_string<char,std::char_traits<char>,std::allocator<char> > > >
main
invoke_main
__scrt_common_main_seh
__scrt_common_main
mainCRTStartup
timeGetTime
timeGetTime
fltused
fltused
fltused
NEW for STL container!!!
17
Stack of functions:
operator new
std::_Default_allocate_traits::_Allocate
std::_Allocate<8,std::_Default_allocate_traits>
std::allocator<std::_Tree_node<std::basic_string<char,std::char_traits<char>,std::allocator<char> >,void *> >::allocate
std::_Tree_node<std::basic_string<char,std::char_traits<char>,std::allocator<char> >,void *>::_Buyheadnode<std::allocator<std::_Tree_node<std::basic_string<char,std::char_traits<char>,std::allocator<char> >,void *> > >
std::_Tree<std::_Tset_traits<std::basic_string<char,std::char_traits<char>,std::allocator<char> >,std::less<std::basic_string<char,std::char_traits<char>,std::allocator<char> > >,std::allocator<std::basic_string<char,std::char_traits<char>,std::allocator<char> > >,0> >::_Alloc_sentinel_and_proxy
std::_Tree<std::_Tset_traits<std::basic_string<char,std::char_traits<char>,std::allocator<char> >,std::less<std::basic_string<char,std::char_traits<char>,std::allocator<char> > >,std::allocator<std::basic_string<char,std::char_traits<char>,std::allocator<char> > >,0> >::_Tree<std::_Tset_traits<std::basic_string<char,std::char_traits<char>,std::allocator<char> >,std::less<std::basic_string<char,std::char_traits<char>,std::allocator<char> > >,std::allocator<std::basic_string<char,std::char_traits<char>,std::allocator<char> > >,0> >
std::set<std::basic_string<char,std::char_traits<char>,std::allocator<char> >,std::less<std::basic_string<char,std::char_traits<char>,std::allocator<char> > >,std::allocator<std::basic_string<char,std::char_traits<char>,std::allocator<char> > > >::set<std::basic_string<char,std::char_traits<char>,std::allocator<char> >,std::less<std::basic_string<char,std::char_traits<char>,std::allocator<char> > >,std::allocator<std::basic_string<char,std::char_traits<char>,std::allocator<char> > > >
main
invoke_main
__scrt_common_main_seh
__scrt_common_main
mainCRTStartup
timeGetTime
timeGetTime
fltused
fltused
fltused
NEW for STL container!!!
ptr out of scope
[GC] reference counting...
SIZE OF REF_COUNT: 5
077657F8 1
07694630 1
07694660 1
076947E0 1
0770A400 1
AFTER COUNTING:
18
Stack of functions:
operator new
std::_Default_allocate_traits::_Allocate
std::_Allocate<8,std::_Default_allocate_traits>
std::allocator<std::_Container_proxy>::allocate
std::_Container_base12::_Alloc_proxy<std::allocator<std::_Container_proxy> >
std::vector<void *,std::allocator<void *> >::vector<void *,std::allocator<void *> >
GarbageCollector::colect
main
invoke_main
__scrt_common_main_seh
__scrt_common_main
mainCRTStartup
timeGetTime
timeGetTime
fltused
fltused
fltused
NEW for STL container!!!
077657F8 0
19
Stack of functions:
operator new
std::_Default_allocate_traits::_Allocate
std::_Allocate<8,std::_Default_allocate_traits>
std::allocator<void *>::allocate
std::_Allocate_at_least_helper<std::allocator<void *> >
std::vector<void *,std::allocator<void *> >::_Emplace_reallocate<void * const &>
std::vector<void *,std::allocator<void *> >::_Emplace_one_at_back<void * const &>
std::vector<void *,std::allocator<void *> >::push_back
GarbageCollector::colect
main
invoke_main
__scrt_common_main_seh
__scrt_common_main
mainCRTStartup
timeGetTime
timeGetTime
fltused
fltused
fltused
NEW for STL container!!!
07694630 0
20
Stack of functions:
operator new
std::_Default_allocate_traits::_Allocate
std::_Allocate<8,std::_Default_allocate_traits>
std::allocator<void *>::allocate
std::_Allocate_at_least_helper<std::allocator<void *> >
std::vector<void *,std::allocator<void *> >::_Emplace_reallocate<void * const &>
std::vector<void *,std::allocator<void *> >::_Emplace_one_at_back<void * const &>
std::vector<void *,std::allocator<void *> >::push_back
GarbageCollector::colect
main
invoke_main
__scrt_common_main_seh
__scrt_common_main
mainCRTStartup
timeGetTime
timeGetTime
fltused
fltused
fltused
NEW for STL container!!!
07694660 0
21
Stack of functions:
operator new
std::_Default_allocate_traits::_Allocate
std::_Allocate<8,std::_Default_allocate_traits>
std::allocator<void *>::allocate
std::_Allocate_at_least_helper<std::allocator<void *> >
std::vector<void *,std::allocator<void *> >::_Emplace_reallocate<void * const &>
std::vector<void *,std::allocator<void *> >::_Emplace_one_at_back<void * const &>
std::vector<void *,std::allocator<void *> >::push_back
GarbageCollector::colect
main
invoke_main
__scrt_common_main_seh
__scrt_common_main
mainCRTStartup
timeGetTime
timeGetTime
fltused
fltused
fltused
NEW for STL container!!!
076947E0 0
22
Stack of functions:
operator new
std::_Default_allocate_traits::_Allocate
std::_Allocate<8,std::_Default_allocate_traits>
std::allocator<void *>::allocate
std::_Allocate_at_least_helper<std::allocator<void *> >
std::vector<void *,std::allocator<void *> >::_Emplace_reallocate<void * const &>
std::vector<void *,std::allocator<void *> >::_Emplace_one_at_back<void * const &>
std::vector<void *,std::allocator<void *> >::push_back
GarbageCollector::colect
main
invoke_main
__scrt_common_main_seh
__scrt_common_main
mainCRTStartup
timeGetTime
timeGetTime
fltused
fltused
fltused
NEW for STL container!!!
0770A400 0
23
Stack of functions:
operator new
std::_Default_allocate_traits::_Allocate
std::_Allocate<8,std::_Default_allocate_traits>
std::allocator<void *>::allocate
std::_Allocate_at_least_helper<std::allocator<void *> >
std::vector<void *,std::allocator<void *> >::_Emplace_reallocate<void * const &>
std::vector<void *,std::allocator<void *> >::_Emplace_one_at_back<void * const &>
std::vector<void *,std::allocator<void *> >::push_back
GarbageCollector::colect
main
invoke_main
__scrt_common_main_seh
__scrt_common_main
mainCRTStartup
timeGetTime
timeGetTime
fltused
fltused
fltused
NEW for STL container!!!
Key was deleted: 077657F8
Key was deleted: 07694630
Key was deleted: 07694660
Key was deleted: 076947E0
Key was deleted: 0770A400
AFTER DELETING:
last_ptr: 07691450
second time collect
*/
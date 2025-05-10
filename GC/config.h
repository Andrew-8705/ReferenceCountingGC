#pragma once

#include <iostream>
#include <cstdint>
#include <unordered_map>
#include <fstream>
#include <string>

using namespace std;

#if defined(_MSC_VER)
// MSVC
#if defined(_M_IX86)
#define __READ_RBP() do { __asm { mov __rbp, ebp } } while(0)
#define __READ_RSP() do { __asm { mov __rsp, esp } } while(0)
#endif
#elif defined(__GNUC__) || defined(__clang__)
// GCC/Clang
#if defined(__i386__)
#define __READ_RBP() do { __asm__ volatile("movl %%ebp, %0" : "=r"(__rbp)); } while(0)
#define __READ_RSP() do { __asm__ volatile("movl %%esp, %0" : "=r"(__rsp)); } while(0)
#elif defined(__x86_64__)
#define __READ_RBP() do { __asm__ volatile("movq %%rbp, %0" : "=r"(__rbp)); } while(0)
#define __READ_RSP() do { __asm__ volatile("movq %%rsp, %0" : "=r"(__rsp)); } while(0)
#endif
#endif

// Уровни логгирования
#define GC_LOG_LEVEL_NONE    0
#define GC_LOG_LEVEL_ERROR   1
#define GC_LOG_LEVEL_INFO    2
#define GC_LOG_LEVEL_DEBUG   3

// Текущий уровень
#ifndef GC_LOG_LEVEL
#define GC_LOG_LEVEL GC_LOG_LEVEL_INFO
#endif

// Цвет вывода
#define GC_COLOR_RED    "\033[31m"
#define GC_COLOR_GREEN  "\033[32m"
#define GC_COLOR_BLUE   "\033[34m"
#define GC_COLOR_RESET  "\033[0m"

// Теги уровней
#define GC_LOG_TAG_ERROR  GC_COLOR_RED "[E]" GC_COLOR_RESET
#define GC_LOG_TAG_INFO   GC_COLOR_GREEN "[I]" GC_COLOR_RESET
#define GC_LOG_TAG_DEBUG  GC_COLOR_BLUE "[D]" GC_COLOR_RESET

// Базовый макрос
#define GC_LOG(level, tag, msg) \
    do { \
        if (level <= GC_LOG_LEVEL) { \
            std::cerr << tag << " " << msg << '\n'; \
        } \
    } while(0)

// Конкретные уровни
#define GC_LOG_ERROR(msg) GC_LOG(GC_LOG_LEVEL_ERROR, GC_LOG_TAG_ERROR, msg)
#define GC_LOG_INFO(msg)  GC_LOG(GC_LOG_LEVEL_INFO, GC_LOG_TAG_INFO, msg)
#define GC_LOG_DEBUG(msg) GC_LOG(GC_LOG_LEVEL_DEBUG, GC_LOG_TAG_DEBUG, msg)

// Макрос для входа в функцию
#define GC_FUNC_TRACE() GC_LOG_DEBUG("-> " << __FUNCTION__)

#ifdef NDEBUG
#undef GC_LOG_LEVEL
#define GC_LOG_LEVEL GC_LOG_LEVEL_NONE
#endif



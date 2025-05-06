#pragma once

#include <iostream>
#include <cstdint>
#include <unordered_map>

using namespace std;

#define __READ_RBP() __asm { mov __rbp, ebp }
#define __READ_RSP() __asm { mov __rsp, esp }

// TODO: добавить макросы для других систем
// TODO: добавить mutex для потокобезопасности
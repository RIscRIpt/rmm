#pragma once

#ifndef uintptr_t
#ifdef _WIN64
typedef unsigned __int64  uintptr_t;
typedef __int64 intptr_t;
#else
typedef unsigned int uintptr_t;
typedef int intptr_t;
#endif
#endif

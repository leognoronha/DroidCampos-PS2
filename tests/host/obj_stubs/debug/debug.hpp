#pragma once
#include <cassert>
#define TYRA_ASSERT(c,...) assert(c)
#define TYRA_WARN(...) ((void)0)
#define TYRA_TRAP(...) assert(false)

#pragma once

//////////////////////////////////////////////////////////////////////////
// General platform abstraction 

#ifdef _WIN32		

#define FLING_WINDOWS
#include "FlingWindowsExports.h"

#elif __APPLE__							

#define FLING_APPLE

#error Fling does not support Apple... does it?

#elif __linux__							

#define FLING_LINUX
#include "FlingLinuxExports.h"

#else									

#   error "Unknown compiler"

#endif
#pragma once

// Windows上strings.h的替代实现
#ifndef _MSC_VER
#error This strings.h replacement is for Windows (MSVC) only
#endif

#include <string.h>

// strncasecmp的Windows替代
#define strncasecmp _strnicmp

// strcasecmp的Windows替代
#define strcasecmp _stricmp

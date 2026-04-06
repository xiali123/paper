#pragma once

// strings.h - platform compatibility
#ifdef _MSC_VER
// Windows MSVC doesn't have strings.h
#include <string.h>
#define strncasecmp _strnicmp
#define strcasecmp _stricmp
#else
// Linux/macOS have native strings.h
#include <strings.h>
#endif

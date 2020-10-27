#ifndef UTILS_H
#define UTILS_H

#ifdef _WIN32
#include <windows.h>
#endif
#include <string>

namespace utils {
    #ifdef __linux__
    extern unsigned int CP_UTF8;
    extern unsigned int CP_ACP;
    #endif
    std::string cpt(const char *str, unsigned int srcCP = CP_UTF8, unsigned int dstCP = CP_ACP);
    bool is_valid_utf8(const char * string);
}

#endif // UTILS_H
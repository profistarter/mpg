#include <windows.h>
#include <string>

std::string cpt(const char *str, unsigned int srcCP = CP_UTF8, unsigned int dstCP = CP_ACP);
bool is_valid_utf8(const char * string);
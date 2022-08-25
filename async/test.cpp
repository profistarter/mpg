#define DOCTEST_CONFIG_IMPLEMENT
#include "doctest/doctest.h"
#include "async.hpp"

int main(int argc, char** argv) {
    #ifdef _WIN32
    SetConsoleOutputCP(1251);
    #endif

    doctest::Context context;
    context.applyCommandLine(argc, argv);
    context.setOption("no-breaks", true); // don't break in the debugger when assertions fail

    int res = context.run(); 
    if(context.shouldExit()) // important - query flags (and --exit) rely on the user doing this
        return res;          
    int client_stuff_return_code = 0;   
    return res + client_stuff_return_code; // the result from doctest is propagated here as well
}
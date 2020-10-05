#include <windows.h>
#include <iostream>
#include <memory>
#include <string>
#include <libpq-fe.h>
#include "pg_connection.h"

int main()
{
    SetConsoleOutputCP(1251);
    PGConnection* conn = new PGConnection();
    std::cout << conn->exec("SELECT 1+1");
    delete conn;
}
#include "pg_connection.h"
#include <string>
#include <iostream>
#include <mutex>
#include <libpq-fe.h>

PGConnection::PGConnection()
{
    connection.reset(PQsetdbLogin(
                         dbhost.c_str(),
                         dbport.c_str(),
                         nullptr,
                         nullptr,
                         dbname.c_str(),
                         dbuser.c_str(),
                         dbpass.c_str()),
                     &PQfinish);

    if (PQstatus(connection.get()) != CONNECTION_OK)
    {
        std::cout << PQerrorMessage(connection.get()) << std::endl;
        throw std::runtime_error(PQerrorMessage(connection.get()));
    }
};

int PGConnection::exec(const char *query)
{
    PGresult *res = PQexec(connection.get(), query);
    if (PQresultStatus(res) == PGRES_FATAL_ERROR) {
        throw std::runtime_error(PQerrorMessage(connection.get()));
    }
    int result_n = PQntuples(res);
    PQclear(res);
    return result_n;
}
#include <windows.h>
#include <iostream>
#include <memory>
#include <string>
#include <libpq-fe.h>

class PGConnection
{
private:

    std::string dbhost = "localhost";
    std::string dbport = "5432";
    std::string dbname = "testdb";
    std::string dbuser = "postgres";
    std::string dbpass = "1";

    std::shared_ptr<PGconn> connection;

public:
    PGConnection()
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
    }

    int exec(const char *query)
    {
        PGresult *res = PQexec(connection.get(), query);
        if (PQresultStatus(res) == PGRES_FATAL_ERROR){
            throw std::runtime_error(PQerrorMessage(connection.get()));
        }
        int result_n = PQntuples(res);
        PQclear(res);
        return result_n;
    }
};

int main()
{
    SetConsoleOutputCP(1251);
    PGConnection* conn = new PGConnection();
    std::cout << conn->exec("SELECT 1+1");
    delete conn;
}

#ifndef PG_CONNECTION_H
#define PG_CONNECTION_H

#include <string>
#include <mutex>
#include <libpq-fe.h>

class PGConnection
{
private:
    std::string dbhost = "localhost";
    std::string dbport = "5432";
    std::string dbname = "testdb";
    std::string dbuser = "postgres";
    std::string dbpass = "1";

    std::shared_ptr<PGconn> connection; //Каждое соединение представляется объектом PGconn

public:
    PGConnection();
    int exec(const char *query);
};

#endif //PG_CONNECTION_H
#ifndef PG_CONNECTION_H
#define PG_CONNECTION_H

#include <string>
#include <mutex>
#include <vector>
#include <libpq-fe.h>

struct Connection_Params {
private:    
    std::vector<const char* > keys_ptr;
    std::vector<const char*> values_ptr;

public:
    Connection_Params();  
    ~Connection_Params();    
    void add_key(const char* key);
    void add_value(const char* key);
    std::vector<const char*> keys();
    std::vector<const char*> values();
};

class PGConnection {
private:
    std::shared_ptr<PGconn> connection; //Каждое соединение представляется объектом PGconn
    std::shared_ptr<std::string> load_params_to_str();
    std::shared_ptr<Connection_Params> parse_params_from_str(const char* str);

public:
    PGConnection();
    int exec(const char *query);
};

#endif //PG_CONNECTION_H
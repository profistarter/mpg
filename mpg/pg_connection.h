#ifndef PG_CONNECTION_H
#define PG_CONNECTION_H

#include <string>
#include <mutex>
#include <memory>
#include <vector>
#include <libpq-fe.h>
#include <functional>

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
public:
    typedef std::vector<std::shared_ptr<PGresult>> Results;
    typedef std::function<void(Results)> Callback;

private:
    bool sending;
    bool receiving;
    Callback async_handler;
    Results results;
    int sock;
    bool clear_send();
    void error();

    std::shared_ptr<PGconn> connection; //Каждое соединение представляется объектом PGconn
    std::shared_ptr<std::string> load_params_to_str();
    std::shared_ptr<Connection_Params> parse_params_from_str(const char* str);

public:
    PGConnection();
    virtual std::vector<std::shared_ptr<PGresult>> exec(const char *query);
    void send(const char* query, Callback fn);
    void receive();
    bool is_ready();
    const int *socket();
};

#endif //PG_CONNECTION_H
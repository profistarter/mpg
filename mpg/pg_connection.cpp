#include "pg_connection.h"
#include <string>
#include <iostream>
#include <mutex>
#include <libpq-fe.h>
#include "../utils/utils.h"

#include <filesystem>
#include <fstream>
#include <iostream>
#include <rapidjson/document.h>
#include <exception>

const std::string CONFIG_PATH = "../_config/config.json";
std::shared_ptr<Connection_Params> connection_params = nullptr;

/* --------------------------------------------------- */
/*                   PGConnection                      */
/* --------------------------------------------------- */

PGConnection::PGConnection()
    : async_handler(nullptr)
    , sock(-1)
    , results()
    , sending(false)
    , receiving(false)
{
    if (connection_params.get() == nullptr) {
        std::shared_ptr<std::string> str = this->load_params_to_str();
        connection_params = this->parse_params_from_str(str->c_str());
    }

    connection.reset(PQconnectdbParams(
                         connection_params.get()->keys().data(),
                         connection_params.get()->values().data(),
                         0),
        &PQfinish);

    if (PQstatus(connection.get()) != CONNECTION_OK || PQsetnonblocking(connection.get(), 1) == -1)
    {
        std::cout << PQerrorMessage(connection.get()) << std::endl;
        throw std::runtime_error(PQerrorMessage(connection.get()));
    }
    sock = PQsocket(connection.get());
};

std::vector<std::shared_ptr<PGresult>> PGConnection::exec(const char *query)
{
    std::shared_ptr<PGresult> res;
    res.reset(PQexec(connection.get(), query), &PQclear);
    if (PQresultStatus(res.get()) == PGRES_FATAL_ERROR){
        error();
    }
    return std::vector<std::shared_ptr<PGresult>>{res};
}

void PGConnection::send(const char* query, PGConnection::Callback fn)
{
    if(!clear_send() || receiving)
        return;
    int res = PQsendQuery(connection.get(), query);
    if(res == 0)
        error();
    async_handler = fn;
    sending = true;
    receiving = true;
};

void PGConnection::receive(){
    while(receiving){
        if(!PQconsumeInput(connection.get()))
            error();
        if(!clear_send())
            return;
        if(PQisBusy(connection.get()))
            return;
        std::shared_ptr<PGresult> res;
        res.reset(PQgetResult(connection.get()), &PQclear);
        int res_status = PQresultStatus(res.get());

        if(res.get() != NULL){
            results.push_back(std::move(res));
        }
        else{
            receiving = false;
            if(async_handler){
                async_handler(results);
                async_handler = nullptr;
            }
        }
    }
}

bool PGConnection::clear_send(){
    if(sending){
        int clr = PQflush(connection.get());
        if(clr == -1)
            error();
        else if(clr == 0){
            sending = false;
        }            
    }
    return !sending;
}
bool PGConnection::is_ready(){
    return !sending && !receiving;
}

const int *PGConnection::socket(){
    return &sock;
}

void PGConnection::error(){
    std::cout << utils::cpt(PQerrorMessage(connection.get())) << std::endl;
    throw std::runtime_error(PQerrorMessage(connection.get()));
}

std::shared_ptr<std::string> PGConnection::load_params_to_str()
{
    std::string params_str = "";
    std::string params_line;
    try {
        if (std::filesystem::exists(CONFIG_PATH)) {
            std::fstream config_file(CONFIG_PATH);
            while (std::getline(config_file, params_line)) {
                params_str += params_line;
            }
            config_file.close();
        } else {
            throw std::runtime_error("No config file. Add config file config/config.json");
        }
    } catch (std::exception& e) {
        throw;
    };
    return std::make_shared<std::string>(params_str);
}

std::shared_ptr<Connection_Params> PGConnection::parse_params_from_str(const char* str)
{
    std::shared_ptr<Connection_Params> conn_params = nullptr;
    rapidjson::Document d;
    d.Parse(str);
    if (d.IsObject() && d.HasMember("connection_params")) {
        rapidjson::Value& d_params = d["connection_params"];
        if (d_params.IsObject()) {
            conn_params = std::make_shared<Connection_Params>();
            rapidjson::Value::ConstMemberIterator iter = d_params.MemberBegin();
            while (iter != d_params.MemberEnd()) {
                conn_params->add_key(iter->name.GetString());
                conn_params->add_value(iter->value.IsNull() ? "" : iter->value.GetString());
                ++iter;
            }
            conn_params->add_key(NULL);
        }
        else{
            printf("В конфигурационном файле неверно задан раздел \"connection_params\"");
        }
    }
    else{
        printf("В конфигурационном файле отсутствует раздел \"connection_params\"");
    }
    return conn_params;
}

/* --------------------------------------------------- */
/*                Connection_Params                    */
/* --------------------------------------------------- */

Connection_Params::Connection_Params()
    : keys_ptr(std::vector<const char*>())
    , values_ptr(std::vector<const char*>())
{
}

void add(std::vector<const char*> *vec, const char* val) {
    if (val != NULL) {
        char* tmp = new char[std::strlen(val) + 1];
        std::strcpy(tmp, val);
        (*vec).push_back(tmp);
    } else {
        (*vec).push_back(nullptr);
    }
}

void Connection_Params::add_key(const char* key)
{
    add(&keys_ptr, key);
}

void Connection_Params::add_value(const char* value)
{
    add(&values_ptr, value);
}

std::vector<const char*> Connection_Params::keys()
{
    return keys_ptr;
}

std::vector<const char*> Connection_Params::values()
{
    return values_ptr;
}

Connection_Params::~Connection_Params()
{
    std::vector<const char*>::iterator itr = keys_ptr.begin();
    while (itr != keys_ptr.end()) {
        delete *itr;
        ++itr;
    }
    keys_ptr.clear();
    itr = values_ptr.begin();
    while (itr != values_ptr.end()) {
        delete *itr;
        ++itr;
    }
    values_ptr.clear();
}
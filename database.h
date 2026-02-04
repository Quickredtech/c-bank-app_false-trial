#ifndef DATABASE_H
#define DATABASE_H

#include <pqxx/pqxx>
#include <string>

class Database {
public:
    Database(const std::string& connStr);
    ~Database();

    void ensureSchema();
    pqxx::connection& getConnection();

private:
    pqxx::connection conn_;
};

#endif // DATABASE_H
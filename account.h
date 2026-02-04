#ifndef ACCOUNT_H
#define ACCOUNT_H

#include <string>
#include <pqxx/pqxx>

struct Account {
    int id = 0;
    std::string username;
    std::string pin_hash;
    std::string salt;
    double balance = 0.0;
    int failed_attempts = 0;
    long long locked_until = 0;
};

bool isValidUsername(const std::string& u);
bool isValidPin(const std::string& p);
std::string generateSalt();
std::string hashPin(const std::string& pin, const std::string& salt);
bool fetchAccountByUsername(pqxx::connection& conn, const std::string& username, Account& out);
bool fetchAccountById(pqxx::connection& conn, int id, Account& out);
void updateAccountAuth(pqxx::connection& conn, const Account& acc);
void updateAccountBalance(pqxx::connection& conn, int account_id, double new_balance);
void logLogin(pqxx::connection& conn, int account_id, bool success);

#endif // ACCOUNT_H
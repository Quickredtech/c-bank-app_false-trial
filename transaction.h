#ifndef TRANSACTION_H
#define TRANSACTION_H

#include <string>
#include <pqxx/pqxx>
#include "account.h"

void recordTransaction(pqxx::work& tx, int account_id, const std::string& type, double amount, const std::string& counterparty, const std::string& note);
void showHistory(pqxx::connection& conn, int account_id);
void exportHistory(pqxx::connection& conn, const Account& acc);
void generateMonthlyStatement(pqxx::connection& conn, const Account& acc);

#endif // TRANSACTION_H
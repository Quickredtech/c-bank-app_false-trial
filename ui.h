#ifndef UI_H
#define UI_H

#include <pqxx/pqxx>
#include "account.h"

void accountMenu(pqxx::connection& conn, Account& acc);
void mainMenu(pqxx::connection& conn);

#endif // UI_H
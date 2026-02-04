#include <iostream>
#include <cstdlib>
#include <pqxx/pqxx>
#include "database.h"
#include "ui.h"

int main() {
    const char* connStr = std::getenv("NEON_DATABASE_URL");
    if (!connStr || std::string(connStr).empty()) {
        std::cout << "Missing NEON_DATABASE_URL environment variable.\n";
        return 1;
    }

    try {
        Database db(connStr);
        db.ensureSchema();
        mainMenu(db.getConnection());
    } catch (const std::exception& ex) {
        std::cout << "Database error: " << ex.what() << "\n";
        return 1;
    }

    return 0;
}

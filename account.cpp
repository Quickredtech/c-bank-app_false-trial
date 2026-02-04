#include "account.h"
#include <random>
#include <iomanip>
#include <sstream>

bool isValidUsername(const std::string& u) {
    if (u.size() < 3 || u.size() > 20) return false;
    for (char c : u) {
        if (!std::isalnum(static_cast<unsigned char>(c)) && c != '_' && c != '-') return false;
    }
    return true;
}

bool isValidPin(const std::string& p) {
    if (p.size() < 4 || p.size() > 8) return false;
    for (char c : p) {
        if (!std::isdigit(static_cast<unsigned char>(c))) return false;
    }
    return true;
}

std::string toHex(uint64_t v) {
    std::ostringstream oss;
    oss << std::hex << std::setfill('0') << std::setw(16) << v;
    return oss.str();
}

uint64_t fnv1a64(const std::string& s) {
    const uint64_t fnv_offset = 14695981039346656037ull;
    const uint64_t fnv_prime = 1099511628211ull;
    uint64_t hash = fnv_offset;
    for (unsigned char c : s) {
        hash ^= static_cast<uint64_t>(c);
        hash *= fnv_prime;
    }
    return hash;
}

std::string generateSalt() {
    static const char* chars = "0123456789abcdef";
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<int> dist(0, 15);
    std::string salt;
    for (int i = 0; i < 16; ++i) {
        salt.push_back(chars[dist(gen)]);
    }
    return salt;
}

std::string hashPin(const std::string& pin, const std::string& salt) {
    return toHex(fnv1a64(salt + ":" + pin));
}

Account rowToAccount(const pqxx::row& r) {
    Account acc;
    acc.id = r["id"].as<int>();
    acc.username = r["username"].c_str();
    acc.pin_hash = r["pin_hash"].c_str();
    acc.salt = r["salt"].c_str();
    acc.balance = r["balance"].as<double>();
    acc.failed_attempts = r["failed_attempts"].as<int>();
    acc.locked_until = r["locked_until"].as<long long>();
    return acc;
}

bool fetchAccountByUsername(pqxx::connection& conn, const std::string& username, Account& out) {
    pqxx::work tx(conn);
    pqxx::result res = tx.exec_params(
        "SELECT id, username, pin_hash, salt, balance, failed_attempts, locked_until FROM accounts WHERE username = $1",
        username
    );
    if (res.empty()) return false;
    out = rowToAccount(res[0]);
    return true;
}

bool fetchAccountById(pqxx::connection& conn, int id, Account& out) {
    pqxx::work tx(conn);
    pqxx::result res = tx.exec_params(
        "SELECT id, username, pin_hash, salt, balance, failed_attempts, locked_until FROM accounts WHERE id = $1",
        id
    );
    if (res.empty()) return false;
    out = rowToAccount(res[0]);
    return true;
}

void updateAccountAuth(pqxx::connection& conn, const Account& acc) {
    pqxx::work tx(conn);
    tx.exec_params(
        "UPDATE accounts SET failed_attempts = $1, locked_until = $2 WHERE id = $3",
        acc.failed_attempts, acc.locked_until, acc.id
    );
    tx.commit();
}

void updateAccountBalance(pqxx::connection& conn, int account_id, double new_balance) {
    pqxx::work tx(conn);
    tx.exec_params(
        "UPDATE accounts SET balance = $1 WHERE id = $2",
        new_balance, account_id
    );
    tx.commit();
}

int createAccount(pqxx::connection& conn, const std::string& username, const std::string& pin_hash, const std::string& salt, double initial_balance) {
    pqxx::work tx(conn);
    pqxx::result res = tx.exec_params(
        "INSERT INTO accounts (username, pin_hash, salt, balance) VALUES ($1, $2, $3, $4) RETURNING id",
        username, pin_hash, salt, initial_balance
    );
    int new_id = res[0][0].as<int>();
    tx.commit();
    return new_id;
}

void logLogin(pqxx::connection& conn, int account_id, bool success) {
    pqxx::work tx(conn);
    tx.exec_params(
        "INSERT INTO login_logs (account_id, success) VALUES ($1, $2)",
        account_id, success
    );
    tx.commit();
}
#include "database.h"

Database::Database(const std::string& connStr) : conn_(connStr) {}

Database::~Database() {}

void Database::ensureSchema() {
    pqxx::work tx(conn_);
    tx.exec(R"SQL(
        CREATE TABLE IF NOT EXISTS accounts (
            id SERIAL PRIMARY KEY,
            username TEXT UNIQUE NOT NULL,
            pin_hash TEXT NOT NULL,
            salt TEXT NOT NULL,
            balance NUMERIC(12,2) NOT NULL DEFAULT 0,
            failed_attempts INT NOT NULL DEFAULT 0,
            locked_until BIGINT NOT NULL DEFAULT 0,
            created_at TIMESTAMPTZ NOT NULL DEFAULT NOW()
        );
    )SQL");

    tx.exec(R"SQL(
        CREATE TABLE IF NOT EXISTS transactions (
            id BIGSERIAL PRIMARY KEY,
            account_id INT NOT NULL REFERENCES accounts(id),
            type TEXT NOT NULL,
            amount NUMERIC(12,2) NOT NULL,
            counterparty TEXT,
            note TEXT,
            created_at TIMESTAMPTZ NOT NULL DEFAULT NOW()
        );
    )SQL");

    tx.exec(R"SQL(
        CREATE TABLE IF NOT EXISTS statements (
            id BIGSERIAL PRIMARY KEY,
            account_id INT NOT NULL REFERENCES accounts(id),
            statement_month DATE NOT NULL,
            generated_at TIMESTAMPTZ NOT NULL DEFAULT NOW(),
            total_in NUMERIC(12,2) NOT NULL,
            total_out NUMERIC(12,2) NOT NULL,
            ending_balance NUMERIC(12,2) NOT NULL,
            items_json TEXT NOT NULL
        );
    )SQL");

    tx.exec(R"SQL(
        CREATE UNIQUE INDEX IF NOT EXISTS statements_unique
        ON statements(account_id, statement_month);
    )SQL");

    tx.exec(R"SQL(
        CREATE TABLE IF NOT EXISTS login_logs (
            id BIGSERIAL PRIMARY KEY,
            account_id INT NOT NULL REFERENCES accounts(id),
            login_time TIMESTAMPTZ NOT NULL DEFAULT NOW(),
            success BOOLEAN NOT NULL,
            ip_address TEXT,
            user_agent TEXT
        );
    )SQL");

    tx.commit();
}

pqxx::connection& Database::getConnection() {
    return conn_;
}
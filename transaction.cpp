#include "transaction.h"
#include "utils.h"
#include <iostream>
#include <fstream>
#include <iomanip>

void recordTransaction(pqxx::work& tx, int account_id, const std::string& type, double amount, const std::string& counterparty, const std::string& note) {
    tx.exec_params(
        "INSERT INTO transactions (account_id, type, amount, counterparty, note) VALUES ($1, $2, $3, $4, $5)",
        account_id, type, amount, counterparty, note
    );
}

void showHistory(pqxx::connection& conn, int account_id) {
    pqxx::work tx(conn);
    pqxx::result res = tx.exec_params(
        "SELECT type, amount, counterparty, note, created_at::text AS created_at "
        "FROM transactions WHERE account_id = $1 ORDER BY id DESC",
        account_id
    );

    if (res.empty()) {
        std::cout << "No transactions yet.\n";
        return;
    }

    for (const auto& row : res) {
        std::string type = row["type"].c_str();
        std::string counterparty = row["counterparty"].is_null() ? "" : row["counterparty"].c_str();
        std::string note = row["note"].is_null() ? "" : row["note"].c_str();
        std::string created = row["created_at"].c_str();
        double amount = row["amount"].as<double>();

        std::cout << "- [" << created << "] " << type << " $" << formatMoney(amount);
        if (!counterparty.empty()) std::cout << " (" << counterparty << ")";
        if (!note.empty()) std::cout << " - " << note;
        std::cout << "\n";
    }
}

void exportHistory(pqxx::connection& conn, const Account& acc) {
    std::string csvName = "history_" + acc.username + ".csv";
    std::string jsonName = "history_" + acc.username + ".json";

    pqxx::work tx(conn);
    pqxx::result res = tx.exec_params(
        "SELECT type, amount, counterparty, note, created_at::text AS created_at "
        "FROM transactions WHERE account_id = $1 ORDER BY id ASC",
        acc.id
    );

    {
        std::ofstream csv(csvName, std::ios::trunc);
        csv << "index,created_at,type,amount,counterparty,note\n";
        for (size_t i = 0; i < res.size(); ++i) {
            std::string counterparty = res[i]["counterparty"].is_null() ? "" : res[i]["counterparty"].c_str();
            std::string note = res[i]["note"].is_null() ? "" : res[i]["note"].c_str();
            std::string cellNote = note;
            std::string cellParty = counterparty;

            size_t pos = 0;
            while ((pos = cellNote.find('"', pos)) != std::string::npos) {
                cellNote.insert(pos, "\"");
                pos += 2;
            }
            pos = 0;
            while ((pos = cellParty.find('"', pos)) != std::string::npos) {
                cellParty.insert(pos, "\"");
                pos += 2;
            }

            csv << (i + 1) << ",";
            csv << '"' << res[i]["created_at"].c_str() << '"' << ",";
            csv << '"' << res[i]["type"].c_str() << '"' << ",";
            csv << res[i]["amount"].as<double>() << ",";
            csv << '"' << cellParty << '"' << ",";
            csv << '"' << cellNote << '"' << "\n";
        }
    }

    {
        std::ofstream json(jsonName, std::ios::trunc);
        json << "[\n";
        for (size_t i = 0; i < res.size(); ++i) {
            std::string counterparty = res[i]["counterparty"].is_null() ? "" : res[i]["counterparty"].c_str();
            std::string note = res[i]["note"].is_null() ? "" : res[i]["note"].c_str();
            json << "  {\"index\": " << (i + 1)
                 << ", \"created_at\": \"" << escapeJson(res[i]["created_at"].c_str()) << "\""
                 << ", \"type\": \"" << escapeJson(res[i]["type"].c_str()) << "\""
                 << ", \"amount\": " << res[i]["amount"].as<double>()
                 << ", \"counterparty\": \"" << escapeJson(counterparty) << "\""
                 << ", \"note\": \"" << escapeJson(note) << "\"";
            json << "}";
            if (i + 1 < res.size()) json << ",";
            json << "\n";
        }
        json << "]\n";
    }

    std::cout << "Exported history to " << csvName << " and " << jsonName << ".\n";
}

void generateMonthlyStatement(pqxx::connection& conn, const Account& acc) {
    std::string input = prompt("Statement month (YYYY-MM, Enter for current): ");
    int year = 0;
    int month = 0;

    if (input.empty()) {
        std::time_t t = std::time(nullptr);
        std::tm local{};
#if defined(_WIN32)
        localtime_s(&local, &t);
#else
        local = *std::localtime(&t);
#endif
        year = local.tm_year + 1900;
        month = local.tm_mon + 1;
    } else if (!parseYearMonth(input, year, month)) {
        std::cout << "Invalid month format. Use YYYY-MM.\n";
        return;
    }

    std::string start = monthStartDate(year, month);
    std::string end = nextMonthStartDate(year, month);

    pqxx::work tx(conn);
    pqxx::result res = tx.exec_params(
        "SELECT type, amount, counterparty, note, created_at::text AS created_at "
        "FROM transactions WHERE account_id = $1 AND created_at >= $2 AND created_at < $3 "
        "ORDER BY created_at ASC",
        acc.id, start, end
    );

    double total_in = 0.0;
    double total_out = 0.0;

    std::ostringstream items;
    items << "[";
    for (size_t i = 0; i < res.size(); ++i) {
        std::string type = res[i]["type"].c_str();
        double amt = res[i]["amount"].as<double>();
        std::string counterparty = res[i]["counterparty"].is_null() ? "" : res[i]["counterparty"].c_str();
        std::string note = res[i]["note"].is_null() ? "" : res[i]["note"].c_str();
        std::string created = res[i]["created_at"].c_str();

        bool is_in = (type == "Deposit" || type == "InitialDeposit" || type == "TransferIn");
        bool is_out = (type == "Withdraw" || type == "TransferOut");
        if (is_in) total_in += amt;
        if (is_out) total_out += amt;

        items << "{"
              << "\"created_at\":\"" << escapeJson(created) << "\""
              << ",\"type\":\"" << escapeJson(type) << "\""
              << ",\"amount\":" << std::fixed << std::setprecision(2) << amt
              << ",\"counterparty\":\"" << escapeJson(counterparty) << "\""
              << ",\"note\":\"" << escapeJson(note) << "\"";
        items << "}";
        if (i + 1 < res.size()) items << ",";
    }
    items << "]";

    double ending_balance = acc.balance;

    tx.exec_params(
        "INSERT INTO statements (account_id, statement_month, total_in, total_out, ending_balance, items_json) "
        "VALUES ($1, $2, $3, $4, $5, $6) "
        "ON CONFLICT (account_id, statement_month) DO UPDATE SET "
        "total_in = EXCLUDED.total_in, total_out = EXCLUDED.total_out, ending_balance = EXCLUDED.ending_balance, items_json = EXCLUDED.items_json, generated_at = NOW()",
        acc.id, start, total_in, total_out, ending_balance, items.str()
    );

    tx.commit();

    std::cout << "Statement stored for " << start.substr(0, 7) << ".\n";
}
#include "ui.h"
#include "utils.h"
#include "account.h"
#include "transaction.h"
#include <iostream>
#include <thread>

void accountMenu(pqxx::connection& conn, Account& acc) {
    while (true) {
        std::cout << "\nLogged in as: " << acc.username << "\n";
        std::cout << "Balance: $" << formatMoney(acc.balance) << "\n";
        std::cout << "1. Deposit\n";
        std::cout << "2. Withdraw\n";
        std::cout << "3. Transfer (Real)\n";
        std::cout << "4. Fake Transfer\n";
        std::cout << "5. View History\n";
        std::cout << "6. Export History (CSV/JSON)\n";
        std::cout << "7. Generate Monthly Statement (store in Neon)\n";
        std::cout << "8. Logout\n";

        std::string choice = prompt("Select an option: ");
        if (choice == "1") {
            double amt = 0.0;
            std::string in = prompt("Deposit amount: ");
            if (!parseAmount(in, amt)) {
                std::cout << RED << "Invalid amount." << RESET << std::endl;
                continue;
            }

            pqxx::work tx(conn);
            acc.balance += amt;
            tx.exec_params("UPDATE accounts SET balance = $1 WHERE id = $2", acc.balance, acc.id);
            recordTransaction(tx, acc.id, "Deposit", amt, "", "");
            tx.commit();

            std::cout << GREEN << "Deposit complete." << RESET << std::endl;
        } else if (choice == "2") {
            double amt = 0.0;
            std::string in = prompt("Withdraw amount: ");
            if (!parseAmount(in, amt)) {
                std::cout << RED << "Invalid amount." << RESET << std::endl;
                continue;
            }
            if (amt > acc.balance) {
                std::cout << RED << "Insufficient funds." << RESET << std::endl;
                continue;
            }

            pqxx::work tx(conn);
            acc.balance -= amt;
            tx.exec_params("UPDATE accounts SET balance = $1 WHERE id = $2", acc.balance, acc.id);
            recordTransaction(tx, acc.id, "Withdraw", amt, "", "");
            tx.commit();

            std::cout << GREEN << "Withdrawal complete." << RESET << std::endl;
        } else if (choice == "3") {
            std::string toUser = prompt("Recipient username: ");
            if (toUser == acc.username) {
                std::cout << "Cannot transfer to yourself. Use a different account.\n";
                continue;
            }

            Account recipient;
            if (!fetchAccountByUsername(conn, toUser, recipient)) {
                std::cout << "Recipient not found.\n";
                continue;
            }

            double amt = 0.0;
            std::string in = prompt("Transfer amount: ");
            if (!parseAmount(in, amt)) {
                std::cout << RED << "Invalid amount." << RESET << std::endl;
                continue;
            }
            if (amt > acc.balance) {
                std::cout << RED << "Insufficient funds." << RESET << std::endl;
                continue;
            }

            pqxx::work tx(conn);
            acc.balance -= amt;
            double recipient_balance = recipient.balance + amt;

            tx.exec_params("UPDATE accounts SET balance = $1 WHERE id = $2", acc.balance, acc.id);
            tx.exec_params("UPDATE accounts SET balance = $1 WHERE id = $2", recipient_balance, recipient.id);

            recordTransaction(tx, acc.id, "TransferOut", amt, recipient.username, "");
            recordTransaction(tx, recipient.id, "TransferIn", amt, acc.username, "");
            tx.commit();

            std::cout << GREEN << "Transfer complete." << RESET << std::endl;
        } else if (choice == "4") {
            std::string toUser = prompt("Recipient username (simulated): ");
            double amt = 0.0;
            std::string in = prompt("Transfer amount (simulated): ");
            if (!parseAmount(in, amt)) {
                std::cout << RED << "Invalid amount." << RESET << std::endl;
                continue;
            }

            pqxx::work tx(conn);
            recordTransaction(tx, acc.id, "FakeTransfer", amt, toUser, "simulated only, no balance moved");
            tx.commit();

            std::cout << GREEN << "Fake transfer recorded. No balances were moved." << RESET << std::endl;
        } else if (choice == "5") {
            showHistory(conn, acc.id);
        } else if (choice == "6") {
            std::cout << YELLOW << "Exporting history..." << RESET << std::endl;
            beep();
            std::thread exportThread([conn, acc]() {
                exportHistory(conn, acc);
                beep(1000, 300); // success beep
            });
            exportThread.detach();
            std::cout << GREEN << "Export started in background." << RESET << std::endl;
        } else if (choice == "7") {
            std::cout << YELLOW << "Generating statement..." << RESET << std::endl;
            beep();
            std::thread stmtThread([conn, acc]() {
                generateMonthlyStatement(conn, acc);
                beep(1200, 300); // success beep
            });
            stmtThread.detach();
            std::cout << GREEN << "Statement generation started in background." << RESET << std::endl;
        } else if (choice == "8") {
            std::cout << "Logged out.\n";
            return;
        } else {
            std::cout << "Invalid option.\n";
        }

        Account refreshed;
        if (fetchAccountById(conn, acc.id, refreshed)) {
            acc = refreshed;
        }
    }
}

void mainMenu(pqxx::connection& conn) {
    std::cout << "=== CLI Bank App (Neon-backed) ===\n";

    while (true) {
        std::cout << "\n1. Create Account\n";
        std::cout << "2. Login\n";
        std::cout << "3. Exit\n";

        std::string choice = prompt("Select an option: ");
        if (choice == "1") {
            std::string username = prompt("Choose username (3-20, letters/numbers/_/-): ");
            if (!isValidUsername(username)) {
                std::cout << "Invalid username.\n";
                continue;
            }

            Account existing;
            if (fetchAccountByUsername(conn, username, existing)) {
                std::cout << "Username already exists.\n";
                continue;
            }

            std::string pin = prompt("Choose PIN (4-8 digits): ");
            if (!isValidPin(pin)) {
                std::cout << "Invalid PIN.\n";
                continue;
            }

            std::string salt = generateSalt();
            std::string pin_hash = hashPin(pin, salt);

            double initial_balance = 0.0;
            std::string initial = prompt("Initial deposit (optional, press Enter to skip): ");
            if (!initial.empty()) {
                if (!parseAmount(initial, initial_balance)) {
                    std::cout << "Invalid amount; starting with $0.00.\n";
                    initial_balance = 0.0;
                }
            }

            int new_id = createAccount(conn, username, pin_hash, salt, initial_balance);
            if (initial_balance > 0.0) {
                pqxx::work tx(conn);
                recordTransaction(tx, new_id, "InitialDeposit", initial_balance, "", "");
                tx.commit();
            }

            std::cout << "Account created. You can now log in.\n";
        } else if (choice == "2") {
            std::string username = prompt("Username: ");
            std::string pin = prompt("PIN: ");

            Account acc;
            if (!fetchAccountByUsername(conn, username, acc)) {
                std::cout << "Invalid login.\n";
                continue;
            }

            long long now = nowSeconds();
            if (acc.locked_until > now) {
                long long remaining = acc.locked_until - now;
                std::cout << "Account locked. Try again in " << remaining << " seconds.\n";
                logLogin(conn, acc.id, false);
                continue;
            }

            std::string expected = hashPin(pin, acc.salt);
            if (acc.pin_hash != expected) {
                acc.failed_attempts += 1;
                if (acc.failed_attempts >= 3) {  // kMaxFailedAttempts
                    acc.locked_until = now + 60;  // kLockSeconds
                    acc.failed_attempts = 0;
                    std::cout << "Too many failed attempts. Account locked for 60 seconds.\n";
                } else {
                    int remaining = 3 - acc.failed_attempts;
                    std::cout << "Invalid login. Attempts left: " << remaining << ".\n";
                }
                updateAccountAuth(conn, acc);
                logLogin(conn, acc.id, false);
                continue;
            }

            acc.failed_attempts = 0;
            acc.locked_until = 0;
            updateAccountAuth(conn, acc);
            logLogin(conn, acc.id, true);
            accountMenu(conn, acc);
        } else if (choice == "3") {
            std::cout << "Goodbye.\n";
            break;
        } else {
            std::cout << "Invalid option.\n";
        }
    }
}
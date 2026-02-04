DISCLAIMER: this application does not do real-world banking.
Transfers are either real (between local accounts) or simulated; no external systems are involved.
This application is made in C++.

Neon database:
- Set NEON_DATABASE_URL in your environment to your Neon Postgres connection string.
- Example (PowerShell session only):
  $env:NEON_DATABASE_URL = "postgresql://USER:PASSWORD@HOST/DB?sslmode=require"

Dependencies:
- Install libpqxx (C++ PostgreSQL library)
  On MSYS2: pacman -S mingw-w64-x86_64-libpqxx
  Or download from https://github.com/jtv/libpqxx and build/install.

Build (Windows, MSVC or MinGW):
  g++ -std=c++17 -O2 -o main.exe main.cpp database.cpp account.cpp transaction.cpp ui.cpp utils.cpp -lpqxx -lpq

Run:
  .\\main.exe

Schema:
- accounts: user login + balances
- transactions: all deposits/withdrawals/transfers/fake transfers
- statements: monthly statements stored as JSON

Security notes:
- PINs are stored as a salted hash (FNV-1a 64-bit). This is not cryptographically strong.
- Accounts lock for 60 seconds after 3 failed login attempts.

Exports:
- While logged in, use "Export History" to create:
  history_<username>.csv
  history_<username>.json

Monthly statements:
- Use "Generate Monthly Statement" to store statement data in Neon.

Migration:
- Run .\\migrate.ps1 to create Neon tables explicitly (requires psql).
- Or run: psql "$env:NEON_DATABASE_URL" -f migrations.sql

#include "utils.h"
#include <iostream>
#include <algorithm>
#include <ctime>

std::string trim(const std::string& s) {
    size_t start = s.find_first_not_of(" \t\r\n");
    size_t end = s.find_last_not_of(" \t\r\n");
    if (start == std::string::npos) return "";
    return s.substr(start, end - start + 1);
}

std::string prompt(const std::string& label) {
    std::cout << label;
    std::string line;
    std::getline(std::cin, line);
    return trim(line);
}

bool parseAmount(const std::string& s, double& out) {
    try {
        size_t idx = 0;
        double v = std::stod(s, &idx);
        if (idx != s.size()) return false;
        if (v <= 0.0) return false;
        out = v;
        return true;
    } catch (...) {
        return false;
    }
}

std::string formatMoney(double v) {
    std::ostringstream oss;
    oss << std::fixed << std::setprecision(2) << v;
    return oss.str();
}

long long nowSeconds() {
    return static_cast<long long>(std::time(nullptr));
}

std::string escapeJson(const std::string& s) {
    std::string out;
    out.reserve(s.size());
    for (char c : s) {
        if (c == '\\' || c == '"') {
            out.push_back('\\');
            out.push_back(c);
        } else if (c == '\n') {
            out += "\\n";
        } else if (c == '\r') {
            out += "\\r";
        } else if (c == '\t') {
            out += "\\t";
        } else {
            out.push_back(c);
        }
    }
    return out;
}

bool parseYearMonth(const std::string& input, int& year, int& month) {
    if (input.size() != 7 || input[4] != '-') return false;
    try {
        year = std::stoi(input.substr(0, 4));
        month = std::stoi(input.substr(5, 2));
    } catch (...) {
        return false;
    }
    return year >= 1970 && month >= 1 && month <= 12;
}

std::string monthStartDate(int year, int month) {
    std::ostringstream oss;
    oss << std::setw(4) << std::setfill('0') << year << "-";
    oss << std::setw(2) << std::setfill('0') << month << "-01";
    return oss.str();
}

#include <windows.h> // for Beep

std::string nextMonthStartDate(int year, int month) {
    int y = year;
    int m = month + 1;
    if (m == 13) {
        m = 1;
        y += 1;
    }
    return monthStartDate(y, m);
}

void beep(int frequency, int duration) {
    Beep(frequency, duration);
}
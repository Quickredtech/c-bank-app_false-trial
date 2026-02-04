#ifndef UTILS_H
#define UTILS_H

#include <string>
#include <iomanip>
#include <sstream>

std::string trim(const std::string& s);
std::string prompt(const std::string& label);
bool parseAmount(const std::string& s, double& out);
std::string formatMoney(double v);
long long nowSeconds();
std::string escapeJson(const std::string& s);
bool parseYearMonth(const std::string& input, int& year, int& month);
std::string monthStartDate(int year, int month);
std::string nextMonthStartDate(int year, int month);

// Colors
const std::string RESET = "\033[0m";
const std::string RED = "\033[31m";
const std::string GREEN = "\033[32m";
const std::string YELLOW = "\033[33m";
const std::string BLUE = "\033[34m";
const std::string MAGENTA = "\033[35m";
const std::string CYAN = "\033[36m";

void beep(int frequency = 800, int duration = 200);

#endif // UTILS_H
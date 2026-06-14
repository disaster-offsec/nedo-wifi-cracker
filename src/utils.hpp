#ifndef UTILS_HPP
#define UTILS_HPP

#include <string>

// Безопасное преобразование строки в int
int safe_stoi(const std::string& s, int default_value = 0);

// Удаление экранирующих обратных слешей перед двоеточиями
std::string unescape(const std::string& s);

// Выполнение команды и получение вывода
std::string execute_command(const char* cmd);

#endif // UTILS_HPP

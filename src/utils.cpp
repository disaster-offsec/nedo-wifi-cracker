#include "utils.hpp"
#include <iostream>
#include <memory>
#include <array>
#include <cctype>
#include <cstdio>

int safe_stoi(const std::string& s, int default_value) {
    if (s.empty()) return default_value;
    try {
        return std::stoi(s);
    } catch (...) {
        return default_value;
    }
}

std::string unescape(const std::string& s) {
    std::string result;
    for (size_t i = 0; i < s.length(); ++i) {
        if (s[i] == '\\' && i + 1 < s.length() && s[i + 1] == ':') {
            result += ':';
            i++;
        } else {
            result += s[i];
        }
    }
    return result;
}

std::string execute_command(const char* cmd) {
    std::array<char, 128> buffer;
    std::string result;
    std::unique_ptr<FILE, decltype(&pclose)> pipe(popen(cmd, "r"), pclose);
    if (!pipe) return "";
    while (fgets(buffer.data(), buffer.size(), pipe.get()) != nullptr) {
        result += buffer.data();
    }
    return result;
}

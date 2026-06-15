#include "interface.hpp"
#include "utils.hpp"
#include <iostream>
#include <regex>

std::vector<NetworkInterface> get_wireless_interfaces() {
    std::vector<NetworkInterface> interfaces;
    
    // Получаем список интерфейсов через iwconfig
    std::string output = execute_command("iwconfig 2>/dev/null | grep -E '^[a-zA-Z0-9]+' | awk '{print $1}'");
    if (output.empty()) return interfaces;
    
    size_t start = 0;
    size_t end;
    
    while ((end = output.find('\n', start)) != std::string::npos) {
        std::string iface_name = output.substr(start, end - start);
        start = end + 1;
        
        if (iface_name.empty()) continue;
        
        // Игнорируем docker, lo, eno и т.д.
        if (iface_name.find("lo") != std::string::npos ||
            iface_name.find("docker") != std::string::npos ||
            iface_name.find("en") == 0) {
            continue;
        }
        
        NetworkInterface iface;
        iface.name = iface_name;
        iface.is_wireless = true;
        
        // Пробуем получить описание через lspci или lsusb
        std::string desc_cmd = "lspci 2>/dev/null | grep -i 'network' | head -1";
        iface.description = execute_command(desc_cmd.c_str());
        if (iface.description.empty()) {
            iface.description = iface_name;
        }
        // Убираем лишние пробелы
        if (!iface.description.empty() && iface.description.back() == '\n') {
            iface.description.pop_back();
        }
        
        interfaces.push_back(iface);
    }
    
    return interfaces;
}

bool supports_monitor_mode(const std::string& interface) {
    // Пробуем создать виртуальный интерфейс в монитор-режиме
    std::string cmd = "sudo iw dev " + interface + " set type monitor 2>&1";
    std::string output = execute_command(cmd.c_str());
    
    // Возвращаем обратно в managed
    execute_command(("sudo iw dev " + interface + " set type managed 2>&1").c_str());
    
    // Если нет ошибки про unsupported, значит поддерживает
    return output.find("Operation not supported") == std::string::npos &&
           output.find("Invalid argument") == std::string::npos;
}

std::string select_interface() {
    auto interfaces = get_wireless_interfaces();
    
    if (interfaces.empty()) {
        std::cerr << "[-] No wireless interfaces found!" << std::endl;
        return "";
    }
    
    std::cout << "\n=== Detected Wireless Interfaces ===\n";
    for (size_t i = 0; i < interfaces.size(); ++i) {
        std::cout << "  " << i + 1 << ". " << interfaces[i].name;
        if (!interfaces[i].description.empty() && interfaces[i].description != interfaces[i].name) {
            std::cout << " (" << interfaces[i].description << ")";
        }
        std::cout << std::endl;
    }
    
    // Если только один интерфейс, используем его автоматически
    if (interfaces.size() == 1) {
        std::cout << "\n[*] Auto-selected interface: " << interfaces[0].name << std::endl;
        return interfaces[0].name;
    }
    
    // Если несколько — спрашиваем пользователя
    int choice;
    std::cout << "\nSelect interface (1-" << interfaces.size() << "): ";
    std::cin >> choice;
    
    if (choice < 1 || choice > static_cast<int>(interfaces.size())) {
        std::cerr << "[-] Invalid choice." << std::endl;
        return "";
    }
    
    return interfaces[choice - 1].name;
}

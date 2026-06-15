#ifndef INTERFACE_HPP
#define INTERFACE_HPP

#include <string>
#include <vector>

struct NetworkInterface {
    std::string name;        // wlan0, wlp0s20f3 и т.д.
    std::string description; // "Intel Wireless-AC 9260"
    bool is_wireless;        // true для Wi-Fi адаптеров
    bool supports_monitor;   // поддержка монитор-мода (проверяем драйвером)
};

// Получить список всех беспроводных интерфейсов
std::vector<NetworkInterface> get_wireless_interfaces();

// Выбрать интерфейс интерактивно или автоматически
std::string select_interface();

// Проверить, поддерживает ли интерфейс монитор-мод
bool supports_monitor_mode(const std::string& interface);

#endif // INTERFACE_HPP

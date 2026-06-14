#include "scanner.hpp"
#include "utils.hpp"
#include <iostream>
#include <cstdio>

bool WiFiNetwork::is_wpa2_personal() const {
    return security.find("WPA2") != std::string::npos && 
           security.find("802.1X") == std::string::npos;
}

std::vector<WiFiNetwork> scan_networks() {
    std::vector<WiFiNetwork> networks;
    const char* cmd = "nmcli --terse --fields SSID,BSSID,CHAN,SIGNAL,SECURITY dev wifi list";
    
    std::string output = execute_command(cmd);
    if (output.empty()) {
        std::cerr << "[-] No Wi-Fi networks found." << std::endl;
        return networks;
    }
    
    size_t start = 0;
    size_t end;
    
    while ((end = output.find('\n', start)) != std::string::npos) {
        std::string line = output.substr(start, end - start);
        start = end + 1;
        
        if (line.empty()) continue;
        
        std::vector<std::string> raw_fields;
        std::string current;
        bool escaped = false;
        
        for (char c : line) {
            if (escaped) {
                current += c;
                escaped = false;
            } else if (c == '\\') {
                escaped = true;
            } else if (c == ':') {
                raw_fields.push_back(current);
                current.clear();
            } else {
                current += c;
            }
        }
        raw_fields.push_back(current);
        
        if (raw_fields.size() < 4) continue;
        
        std::string ssid = raw_fields[0];
        if (ssid.empty() || ssid == "--") continue;
        
        std::string bssid;
        size_t idx = 1;
        while (idx < raw_fields.size()) {
            bool is_number = !raw_fields[idx].empty();
            for (char c : raw_fields[idx]) {
                if (!std::isdigit(c)) {
                    is_number = false;
                    break;
                }
            }
            
            if (is_number && !bssid.empty()) {
                break;
            }
            
            if (!bssid.empty()) bssid += ":";
            bssid += unescape(raw_fields[idx]);
            idx++;
        }
        
        if (idx >= raw_fields.size()) continue;
        int channel = safe_stoi(raw_fields[idx], 0);
        idx++;
        
        if (idx >= raw_fields.size()) continue;
        int signal = safe_stoi(raw_fields[idx], 0);
        idx++;
        
        std::string security = (idx < raw_fields.size()) ? raw_fields[idx] : "Unknown";
        bssid = unescape(bssid);
        
        if (bssid.length() < 10) continue;
        
        WiFiNetwork net;
        net.ssid = ssid;
        net.bssid = bssid;
        net.channel = channel;
        net.signal = signal;
        net.security = security;
        
        networks.push_back(net);
    }
    
    return networks;
}

void print_networks(const std::vector<WiFiNetwork>& networks) {
    std::cout << "\n=== Available Wi-Fi Networks ===\n";
    std::cout << " #  SSID                              BSSID               Ch  Sig  Security\n";
    std::cout << "--- --------------------------------- ------------------ --  ---  --------\n";
    
    for (size_t i = 0; i < networks.size(); ++i) {
        const auto& net = networks[i];
        std::string ssid_display = net.ssid;
        if (ssid_display.length() > 32) {
            ssid_display = ssid_display.substr(0, 29) + "...";
        }
        
        printf("%2zu  %-32s %-17s %2d  %3d  %s\n",
               i + 1,
               ssid_display.c_str(),
               net.bssid.c_str(),
               net.channel,
               net.signal,
               net.security.c_str());
    }
    std::cout << "--- --------------------------------- ------------------ --  ---  --------\n";
}

void save_target(const WiFiNetwork& target, const std::string& filename) {
    std::cout << "\n[+] Target saved: " << target.ssid << " (" << target.bssid << ")\n";
    std::cout << "[+] Channel: " << target.channel << ", Security: " << target.security << "\n";
    
    if (target.is_wpa2_personal()) {
        std::cout << "[+] This network is WPA2-Personal. Ready for handshake capture.\n";
    } else {
        std::cout << "[!] Warning: This network uses " << target.security 
                  << ". Attack may not work with standard deauth + handshake method.\n";
    }
    
    FILE* file = fopen(filename.c_str(), "w");
    if (file) {
        fprintf(file, "SSID=%s\n", target.ssid.c_str());
        fprintf(file, "BSSID=%s\n", target.bssid.c_str());
        fprintf(file, "CHANNEL=%d\n", target.channel);
        fprintf(file, "SIGNAL=%d\n", target.signal);
        fprintf(file, "SECURITY=%s\n", target.security.c_str());
        fclose(file);
        std::cout << "[+] Target data saved to: " << filename << "\n";
    } else {
        std::cerr << "[-] Failed to save target data.\n";
    }
}

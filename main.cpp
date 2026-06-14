#include <iostream>
#include <vector>
#include <string>
#include <memory>
#include <array>
#include <algorithm>
#include <cctype>
#include <cstdio>

// Безопасное преобразование строки в int
int safe_stoi(const std::string& s, int default_value = 0) {
    if (s.empty()) return default_value;
    try {
        return std::stoi(s);
    } catch (...) {
        return default_value;
    }
}

// Удаление экранирующих обратных слешей перед двоеточиями
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

// Структура для хранения информации о Wi-Fi сети
struct WiFiNetwork {
    std::string ssid;
    std::string bssid;
    int channel;
    int signal;
    std::string security;
    
    bool is_wpa2_personal() const {
        return security.find("WPA2") != std::string::npos && 
               security.find("802.1X") == std::string::npos;
    }
};

// Функция для выполнения команды и получения вывода
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

// Парсинг вывода nmcli в вектор WiFiNetwork
std::vector<WiFiNetwork> scan_networks() {
    std::vector<WiFiNetwork> networks;
    const char* cmd = "nmcli --terse --fields SSID,BSSID,CHAN,SIGNAL,SECURITY dev wifi list";
    
    std::string output = execute_command(cmd);
    if (output.empty()) {
        std::cerr << "[-] No Wi-Fi networks found." << std::endl;
        return networks;
    }
    
    // Включаем для отладки
    // std::cout << "[DEBUG] Raw output:\n" << output << std::endl;
    
    size_t start = 0;
    size_t end;
    
    while ((end = output.find('\n', start)) != std::string::npos) {
        std::string line = output.substr(start, end - start);
        start = end + 1;
        
        if (line.empty()) continue;
        
        // Парсим с учётом экранированных двоеточий
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
        
        // Должно быть как минимум 7 полей: [SSID, BSSID parts..., CHAN, SIGNAL, SECURITY]
        if (raw_fields.size() < 4) continue;
        
        // SSID — первое поле
        std::string ssid = raw_fields[0];
        if (ssid.empty() || ssid == "--") continue;
        
        // BSSID собираем из следующих полей, пока не встретим CHAN (число)
        std::string bssid;
        size_t idx = 1;
        while (idx < raw_fields.size()) {
            // Проверяем, является ли текущее поле числом (CHAN)
            bool is_number = !raw_fields[idx].empty();
            for (char c : raw_fields[idx]) {
                if (!std::isdigit(c)) {
                    is_number = false;
                    break;
                }
            }
            
            if (is_number && !bssid.empty()) {
                break; // дошли до CHAN
            }
            
            if (!bssid.empty()) bssid += ":";
            bssid += unescape(raw_fields[idx]);
            idx++;
        }
        
        // Теперь idx указывает на CHAN
        if (idx >= raw_fields.size()) continue;
        int channel = safe_stoi(raw_fields[idx], 0);
        idx++;
        
        // SIGNAL
        if (idx >= raw_fields.size()) continue;
        int signal = safe_stoi(raw_fields[idx], 0);
        idx++;
        
        // SECURITY (может быть пустым)
        std::string security = (idx < raw_fields.size()) ? raw_fields[idx] : "Unknown";
        
        // Очищаем BSSID от лишних обратных слешей
        bssid = unescape(bssid);
        
        // Пропускаем сети с невалидным BSSID
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

// Вывод списка сетей с нумерацией
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

// Сохранение данных выбранной сети в файл
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
    
    std::cout << "\n[*] Next steps (to be implemented):\n";
    std::cout << "    1. Switch adapter to channel " << target.channel << "\n";
    std::cout << "    2. Start sniffing for EAPOL packets (4-way handshake)\n";
    std::cout << "    3. Send deauth packet to " << target.bssid << "\n";
    std::cout << "    4. Capture handshake and save to .pcap\n";
    std::cout << "    5. Crack with hashcat\n";
    
    return 0;
}

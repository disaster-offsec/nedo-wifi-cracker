#ifndef SCANNER_HPP
#define SCANNER_HPP

#include <string>
#include <vector>

struct WiFiNetwork {
    std::string ssid;
    std::string bssid;
    int channel;
    int signal;
    std::string security;
    
    bool is_wpa2_personal() const;
};

std::vector<WiFiNetwork> scan_networks();
void print_networks(const std::vector<WiFiNetwork>& networks);
void save_target(const WiFiNetwork& target, const std::string& filename);

#endif // SCANNER_HPP

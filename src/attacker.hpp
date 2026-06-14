#ifndef ATTACKER_HPP
#define ATTACKER_HPP

#include "scanner.hpp"
#include <string>

bool setup_monitor_mode(const std::string& interface);
void cleanup_monitor_mode(const std::string& interface);
bool set_channel(const std::string& interface, int channel);
bool send_deauth(const std::string& interface, const std::string& bssid);
bool capture_handshake(const std::string& interface, const std::string& bssid, int channel, int timeout_seconds);
bool attack_network(const WiFiNetwork& target);

#endif // ATTACKER_HPP

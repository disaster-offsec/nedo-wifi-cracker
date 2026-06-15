#ifndef ATTACKER_HPP
#define ATTACKER_HPP

#include "scanner.hpp"
#include <string>

bool is_in_monitor_mode(const std::string& interface);
bool is_24ghz_channel(int channel);
bool is_5ghz_channel(int channel);
bool force_24ghz_mode(const std::string& interface);
bool force_5ghz_mode(const std::string& interface, int channel);
bool switch_to_band(const std::string& interface, int channel);
bool setup_monitor_mode(const std::string& interface, int target_channel = 0);
void cleanup_monitor_mode(const std::string& interface);
bool set_channel(const std::string& interface, int channel);
void deauth_sender(const std::string& interface, const std::string& bssid);
bool capture_handshake_with_timeout(const std::string& interface, const std::string& bssid, int channel, int timeout_seconds, const std::string& output_file);
int get_current_channel(const std::string& interface);
bool send_deauth(const std::string& interface, const std::string& bssid);
bool capture_handshake(const std::string& interface, const std::string& bssid, int channel, int timeout_seconds);
bool attack_network(const WiFiNetwork& target);

#endif // ATTACKER_HPP

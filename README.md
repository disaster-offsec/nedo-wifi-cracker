# NEDO-WIFI-CRACKER
# Wi-Fi Handshake Capture Tool

Инструмент для захвата 4-way handshake WPA/WPA2-Personal сетей. Автоматически сканирует доступные Wi-Fi сети, позволяет выбрать цель, отправляет deauth пакеты для переподключения клиента и сохраняет handshake в `.cap` файл для последующего взлома.

## 📋 Возможности

- Сканирование доступных Wi-Fi сетей через `nmcli`
- Автоматическое определение беспроводного интерфейса
- Поддержка 2.4 ГГц сетей (каналы 1-14)
- Захват 4-way handshake через `airodump-ng`
- Отправка deauth пакетов через `aireplay-ng`
- Многопоточная работа (отдельный поток для deauth)
- Автоматическое восстановление NetworkManager после атаки

## 🔧 Полный список зависимостей

### Обязательные пакеты

| Пакет | Назначение | Команда установки |
|-------|------------|-------------------|
| `cmake` | Система сборки проекта | `sudo pacman -S cmake` |
| `gcc` | Компилятор C/C++ | `sudo pacman -S gcc` |
| `g++` | Компилятор C++ | `sudo pacman -S gcc` (входит в gcc) |
| `make` | Утилита сборки | `sudo pacman -S make` |
| `libtins` | Библиотека для работы с сетевыми пакетами | `yay -S libtins` или сборка из исходников |
| `aircrack-ng` | Набор инструментов для Wi-Fi пентеста | `sudo pacman -S aircrack-ng` |
| `networkmanager` | Управление сетевыми подключениями (nmcli) | `sudo pacman -S networkmanager` |

### Пакеты aircrack-ng (устанавливаются вместе)

| Утилита | Назначение |
|---------|------------|
| `airmon-ng` | Включение/выключение монитор-режима на Wi-Fi адаптере |
| `aireplay-ng` | Отправка deauth пакетов (отключение клиентов) |
| `airodump-ng` | Захват и анализ Wi-Fi трафика, перехват handshake |
| `aircrack-ng` | Взлом WPA/WPA2 паролей по захваченному handshake |
| `packetforge-ng` | Создание зашифрованных пакетов |
| `airbase-ng` | Создание фальшивых точек доступа |

### Дополнительные пакеты (рекомендуются)

| Пакет | Назначение | Команда установки |
|-------|------------|-------------------|
| `wordlist` | Словари паролей (включая rockyou.txt) | `sudo pacman -S wordlist` |
| `hashcat` | Мощный инструмент для подбора паролей | `sudo pacman -S hashcat` |
| `pocl` | OpenCL для CPU (для hashcat) | `sudo pacman -S pocl` |
| `iw` | Управление беспроводными устройствами | `sudo pacman -S iw` |
| `wireless_tools` | iwconfig и другие утилиты | `sudo pacman -S wireless_tools` |

## 📦 Установка всех зависимостей (Arch Linux)

```bash
# 1. Базовые инструменты сборки
sudo pacman -S cmake gcc make base-devel

# 2. Сетевые утилиты
sudo pacman -S networkmanager iw wireless_tools

# 3. Wi-Fi пентест инструменты
sudo pacman -S aircrack-ng

# 4. Дополнительные инструменты для взлома
sudo pacman -S wordlist hashcat pocl

# 5. Библиотека libtins (из AUR)
yay -S libtins
# или сборка из исходников:
cd /tmp
git clone https://github.com/mfontanini/libtins.git
cd libtins
mkdir build && cd build
cmake .. -DCMAKE_INSTALL_PREFIX=/usr
make -j$(nproc)
sudo make install
```

# Структура проекта

```bash
wifi-cracker/
├── src/
│   ├── main.cpp          # Точка входа
│   ├── scanner.cpp/hpp   # Сканирование сетей
│   ├── attacker.cpp/hpp  # Захват handshake
│   ├── interface.cpp/hpp # Работа с интерфейсами
│   └── utils.cpp/hpp     # Вспомогательные функции
├── CMakeLists.txt        # Файл сборки
└── README.md             # Документация
```

# После захвата handshake
```bash
aircrack-ng handshake_*.cap -w /usr/share/wordlists/rockyou.txt
```

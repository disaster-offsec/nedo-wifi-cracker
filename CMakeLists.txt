cmake_minimum_required(VERSION 3.10)
project(wifi-cracker)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Находим библиотеку libtins
find_package(PkgConfig REQUIRED)
pkg_check_modules(TINS REQUIRED libtins)

# Добавляем исполняемый файл
add_executable(wifi-cracker
    src/main.cpp
    src/scanner.cpp
    src/attacker.cpp
    src/utils.cpp
)

# Подключаем библиотеки
target_include_directories(wifi-cracker PRIVATE ${TINS_INCLUDE_DIRS} src)
target_link_libraries(wifi-cracker ${TINS_LIBRARIES} pthread)

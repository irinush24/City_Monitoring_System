cmake_minimum_required(VERSION 3.10)
project(City_Monitoring_System C)

set(CMAKE_C_STANDARD 99)

add_executable(city_manager city_manager.c
        ctiy_hub.c)
add_executable(monitor_reports monitor_reports.c)
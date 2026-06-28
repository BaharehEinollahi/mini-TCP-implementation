#ifndef LOGGER_HPP
#define LOGGER_HPP

#include <iostream>
#include <string>
#include <chrono>
#include <iomanip>

inline void log_msg(const std::string& level, const std::string& message) {
    auto now = std::chrono::system_clock::now();
    auto time_t_now = std::chrono::system_clock::to_time_t(now);
    auto duration = now.time_since_epoch();
    auto millis = std::chrono::duration_cast<std::chrono::milliseconds>(duration).count() % 1000;

    std::cout << "[" << std::put_time(std::localtime(&time_t_now), "%H:%M:%S") 
              << "." << std::setfill('0') << std::setw(3) << millis << "] "
              << "[" << level << "] " << message << std::endl;
}

#endif
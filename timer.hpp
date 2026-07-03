#ifndef TIMER_HPP
#define TIMER_HPP

#include <chrono>

class Timer {
private:
    std::chrono::steady_clock::time_point start_time;
    bool running = false;
public:
    void start() {
        start_time = std::chrono::steady_clock::now();
        running = true;
    }

    bool is_running() const { return running; }
    
    double elapsed_ms() const {
        return std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - start_time).count();
    }
};

#endif
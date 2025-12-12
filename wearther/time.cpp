#include "time.h"
#include <iostream>



#include<chrono>
#include<ctime>
#include <string>

 std::string GetCurrentTimex()
{
    auto now = std::chrono::system_clock::now();
    std::time_t t = std::chrono::system_clock::to_time_t(now);

    std::tm local{};
    localtime_s(&local, &t);   // SAFE & warning-free

    char buffer[32];
    std::strftime(buffer, sizeof(buffer), "%H:%M:%S", &local);

    return std::string(buffer);
}
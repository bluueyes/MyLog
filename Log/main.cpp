#include <iostream>
#include "MyLog.h"
#include <chrono>
#include <thread>
int main()
{
    AsyncLog::ELog("hello {}", "world!");
    std::this_thread::sleep_for(std::chrono::seconds(10));
}

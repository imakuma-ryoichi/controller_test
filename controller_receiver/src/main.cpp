#include "bluetooth_receiver.hpp"
#include "wifi_receiver.hpp"

#include <thread>

int main()
{
    std::thread wifi_thread(receive_wifi);
    std::thread bluetooth_thread(receive_bluetooth);

    wifi_thread.join();
    bluetooth_thread.join();

    return 0;
}

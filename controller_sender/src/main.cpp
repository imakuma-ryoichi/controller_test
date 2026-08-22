#include <unistd.h>

#include "controller.hpp"
#include "wifi_sender.hpp"
#include "bluetooth_sender.hpp"

int main()
{
    const int controller_fd = open_controller();

    if (controller_fd < 0)
    {
        return 1;
    }

    const int wifi_fd = create_wifi_sender("192.168.1.100", 12345);

    if (wifi_fd < 0)
    {
        close(controller_fd);
        return 1;
    }

    const int bluetooth_fd = connect_bluetooth("XX:XX:XX:XX:XX:XX", 1);

    if (bluetooth_fd < 0)
    {
        close(wifi_fd);
        close(controller_fd);
        return 1;
    }

    while (true)
    {
        ControllerData data{};

        if (!read_controller(controller_fd, data))
        {
            break;
        }

        send_wifi(wifi_fd, data);
        send_bluetooth(bluetooth_fd, data);
    }

    close(bluetooth_fd);
    close(wifi_fd);
    close(controller_fd);

    return 0;
}

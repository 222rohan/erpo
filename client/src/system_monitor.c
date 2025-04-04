// system monitor 
// (using udev), enumarate all devices, and monitor changes in devices
// link using -ludev

#include "../include/system_monitor.h"
#include "../include/logger.h"
#include "../include/client.h"

#include <libudev.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>



static void print_device(struct udev_device* dev, int srv_event)
{
    const char* action = udev_device_get_action(dev);
    if (! action)
        action = "exists";

    const char* vendor = udev_device_get_sysattr_value(dev, "idVendor");
    if (! vendor)
        vendor = "0000";

    const char* product = udev_device_get_sysattr_value(dev, "idProduct");
    if (! product)
        product = "0000";

    log_eventf("[System Monitor] %s %s %6s %s:%s %s\n",
           udev_device_get_subsystem(dev),
           udev_device_get_devtype(dev),
           action,
           vendor,
           product,
           udev_device_get_devnode(dev));

    if(srv_event) {
        send_message_to_server("[System Monitor] Storage Device Event Detected");
    }
}

static void process_device(struct udev_device* dev, int srv_event)
{
    if (dev) {
        if (udev_device_get_devnode(dev))
            print_device(dev,srv_event);

        udev_device_unref(dev);
    }
}

static void enumerate_devices(struct udev* udev)
{
    log_event("[System Monitor] Enumerating Devices...");
    struct udev_enumerate* enumerate = udev_enumerate_new(udev);

    udev_enumerate_add_match_subsystem(enumerate, "block");
    udev_enumerate_scan_devices(enumerate);

    struct udev_list_entry* devices = udev_enumerate_get_list_entry(enumerate);
    struct udev_list_entry* entry;

    udev_list_entry_foreach(entry, devices) {
        const char* path = udev_list_entry_get_name(entry);
        struct udev_device* dev = udev_device_new_from_syspath(udev, path);
        process_device(dev, 0);
    }

    udev_enumerate_unref(enumerate);
}

static void monitor_devices(struct udev* udev)
{
    int srv_event = 0;
    struct udev_monitor* mon = udev_monitor_new_from_netlink(udev, "udev");

    udev_monitor_filter_add_match_subsystem_devtype(mon, "block", NULL);
    udev_monitor_enable_receiving(mon);

    int fd = udev_monitor_get_fd(mon);

    while (1) {
        srv_event = 0;
        fd_set fds;
        FD_ZERO(&fds);
        FD_SET(fd, &fds);

        int ret = select(fd+1, &fds, NULL, NULL, NULL);
        if (ret <= 0)
            break;

        if (FD_ISSET(fd, &fds)) {
            srv_event = 1;
            struct udev_device* dev = udev_monitor_receive_device(mon);
            process_device(dev, srv_event);
        }
    }
}

int monitor_system() {
    struct udev* udev = udev_new();
    if (!udev) {
        fprintf(stderr, "udev_new() failed\n");
        return -1;
    }

    enumerate_devices(udev);
    monitor_devices(udev);

    udev_unref(udev);
    return 0;
}
    
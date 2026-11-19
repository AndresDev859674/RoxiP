#ifndef ROXIP_DRIVERS_NETWORK_H
#define ROXIP_DRIVERS_NETWORK_H

#ifdef NETWORK_ENABLED

#include <stdint.h>
#include "kernel.h"

/* Network driver operations */
typedef struct net_driver_ops {
    int (*init)(void);
    int (*start)(void);
    int (*stop)(void);
    int (*send)(const uint8_t *buf, uint32_t len);
    int (*receive)(uint8_t *buf, uint32_t max_len, uint32_t *actual_len);
    void (*set_mac)(const uint8_t *mac);
    void (*get_mac)(uint8_t *mac);
} net_driver_ops_t;

/* Network device registration */
typedef struct net_device {
    char name[16];
    uint8_t mac[6];
    uint32_t ip;
    uint32_t netmask;
    uint32_t gateway;
    net_driver_ops_t *ops;
    int state;                  /* 0=down, 1=up, 2=running       */
    struct net_device *next;
} net_device_t;

void net_device_register(net_device_t *dev);
net_device_t* net_device_get_by_name(const char *name);
int net_device_send(net_device_t *dev, const uint8_t *buf, uint32_t len);

/* NE2000 Ethernet driver */
int ne2000_init_driver(void);
void ne2000_poll(void);

/* SMC91C111 Ethernet (Versatile/PB onboard NIC) */
int smc91c111_init_driver(void);
void smc91c111_poll(void);

#endif /* NETWORK_ENABLED */

#endif /* ROXIP_DRIVERS_NETWORK_H */

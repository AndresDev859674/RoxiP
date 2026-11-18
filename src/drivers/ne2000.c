#include "network.h"
#include "net.h"
#include "kernel.h"

#ifdef NETWORK_ENABLED

/* NE2000 register offsets */
#define NE_CMD        0x00  /* Command register              */
#define NE_DATAPORT   0x10  /* Data port                     */
#define NE_RESET      0x1F  /* Reset register                */

/* NE2000 commands */
#define NE_CMD_STOP   0x01
#define NE_CMD_START  0x02
#define NE_CMD_TXPKT  0x04
#define NE_CMD_RDMA   0x08
#define NE_CMD_WRMA   0x10
#define NE_CMD_PAGE0  0x00
#define NE_CMD_PAGE1  0x40
#define NE_CMD_PAGE2  0x80

/* NE2000 state */
static net_device_t ne2000_dev;
static uint16_t ne_io_base = 0x300;  /* Default I/O base */
static uint8_t ne_mac[6] = {0x52, 0x54, 0x00, 0x12, 0x34, 0x56};
static int ne_initialized = 0;

/* ----------------------------------------------------------------------
 * Low-level I/O (placeholder - would use inb/outb on real hardware)
 * ---------------------------------------------------------------------- */

static inline void ne_write_reg(uint16_t reg, uint8_t val) {
    /* Placeholder: actual implementation would use outb() */
    (void)reg;
    (void)val;
}

static inline uint8_t ne_read_reg(uint16_t reg) {
    /* Placeholder: actual implementation would use inb() */
    (void)reg;
    return 0;
}

/* ----------------------------------------------------------------------
 * NE2000 driver operations
 * ---------------------------------------------------------------------- */

static int ne2000_hw_init(void) {
    if (ne_initialized) return 0;

    (void)ne_io_base;  /* Reserved for future use */

    /* Reset the card */
    ne_write_reg(NE_RESET, 0);

    /* Set stop command */
    ne_write_reg(NE_CMD, NE_CMD_STOP | NE_CMD_PAGE1);

    /* Configure MAC address registers (page 1) */
    /* ... */

    /* Set start command */
    ne_write_reg(NE_CMD, NE_CMD_START | NE_CMD_PAGE0);

    ne_initialized = 1;
    log_boot_event("[NET] NE2000 Ethernet hardware initialized");
    return 0;
}

static int ne2000_start(void) {
    if (!ne_initialized) return -1;
    ne_write_reg(NE_CMD, NE_CMD_START | NE_CMD_PAGE0);
    log_boot_event("[NET] NE2000 Ethernet started");
    return 0;
}

static int ne2000_stop(void) {
    ne_write_reg(NE_CMD, NE_CMD_STOP | NE_CMD_PAGE0);
    return 0;
}

static int ne2000_send(const uint8_t *buf, uint32_t len) {
    if (!ne_initialized || len > ETH_FRAME_LEN) return -1;
    (void)buf;
    (void)len;
    return (int)len;
}

static int ne2000_receive(uint8_t *buf, uint32_t max_len, uint32_t *actual_len) {
    if (!ne_initialized) return -1;
    (void)buf;
    (void)max_len;
    (void)actual_len;
    return 0; /* No packets available */
}

static void ne2000_set_mac(const uint8_t *mac) {
    memcpy(ne_mac, mac, 6);
}

static void ne2000_get_mac(uint8_t *mac) {
    memcpy(mac, ne_mac, 6);
}

/* ----------------------------------------------------------------------
 * Driver registration
 * ---------------------------------------------------------------------- */

static net_driver_ops_t ne2000_ops = {
    .init = ne2000_hw_init,
    .start = ne2000_start,
    .stop = ne2000_stop,
    .send = ne2000_send,
    .receive = ne2000_receive,
    .set_mac = ne2000_set_mac,
    .get_mac = ne2000_get_mac,
};

int ne2000_init_driver(void) {
    memset(&ne2000_dev, 0, sizeof(ne2000_dev));
    const char *devname = "eth0";
    int i = 0;
    while (devname[i] && i < (int)sizeof(ne2000_dev.name) - 1) {
        ne2000_dev.name[i] = devname[i];
        i++;
    }
    ne2000_dev.name[i] = '\0';
    memcpy(ne2000_dev.mac, ne_mac, 6);
    ne2000_dev.ops = &ne2000_ops;
    ne2000_dev.state = 0;

    ne2000_hw_init();
    net_device_register(&ne2000_dev);

    log_boot_event("[NET] NE2000 device registered");
    return 0;
}

void ne2000_poll(void) {
    if (!ne_initialized) return;

    uint8_t buf[ETH_FRAME_LEN];
    uint32_t len = 0;

    while (ne2000_receive(buf, sizeof(buf), &len) == 0 && len > 0) {
        net_if_t *dev = net_if_get_by_name("eth0");
        if (dev) {
            net_rx(dev, buf, len);
        }
    }
}

#endif /* NETWORK_ENABLED */

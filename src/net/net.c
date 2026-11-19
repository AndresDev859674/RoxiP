#include "net.h"
#include "kernel.h"

#ifdef NETWORK_ENABLED

/* Network interface list */
static net_if_t *net_if_list = 0;
static int net_if_count = 0;

/* Network initialization flag */
static int net_initialized = 0;

/* ----------------------------------------------------------------------
 * Network interface management
 * ---------------------------------------------------------------------- */

net_if_t* net_if_alloc(const char *name) {
    net_if_t *dev = (net_if_t*)malloc(sizeof(net_if_t));
    if (!dev) return 0;

    memset(dev, 0, sizeof(net_if_t));
    int i = 0;
    while (name[i] && i < 15) {
        dev->name[i] = name[i];
        i++;
    }
    dev->name[i] = '\0';
    dev->state = NET_IF_DOWN;
    dev->next = 0;

    return dev;
}

void net_if_free(net_if_t *dev) {
    if (dev) free(dev);
}

int net_if_register(net_if_t *dev) {
    if (!dev) return -1;

    dev->next = net_if_list;
    net_if_list = dev;
    net_if_count++;

    log_boot_event("[NET] Interface registered");
    return 0;
}

void net_device_register(net_device_t *dev) {
    /* Convert net_device_t to net_if_t and register */
    net_if_t *iface = (net_if_t*)dev;
    if (!iface) return;
    
    iface->next = net_if_list;
    net_if_list = iface;
    net_if_count++;
    
    log_boot_event("[NET] Device registered");
}

net_if_t* net_if_get_by_name(const char *name) {
    net_if_t *dev = net_if_list;
    while (dev) {
        if (strcmp(dev->name, name) == 0) return dev;
        dev = dev->next;
    }
    return 0;
}

net_if_t* net_if_get_by_ip(uint32_t ip) {
    net_if_t *dev = net_if_list;
    while (dev) {
        if (dev->ip == ip) return dev;
        dev = dev->next;
    }
    return 0;
}

int net_if_up(net_if_t *dev) {
    if (!dev) return -1;

    if (dev->ops && dev->ops->start) {
        int ret = dev->ops->start();
        if (ret != 0) return ret;
    }

    dev->state = NET_IF_UP;
    log_boot_event("[NET] Interface up");
    return 0;
}

int net_if_down(net_if_t *dev) {
    if (!dev) return -1;

    dev->state = NET_IF_DOWN;

    if (dev->ops && dev->ops->stop) {
        dev->ops->stop();
    }

    log_boot_event("[NET] Interface down");
    return 0;
}

/* ----------------------------------------------------------------------
 * Packet receive/transmit
 * ---------------------------------------------------------------------- */

int net_rx(net_if_t *dev, const uint8_t *buf, uint32_t len) {
    if (!dev || !buf || len < ETH_HLEN) return -1;

    dev->rx_packets++;
    dev->rx_bytes += len;

    const eth_hdr_t *eth = (const eth_hdr_t*)buf;
    uint16_t proto = net_htons(eth->proto);

    /* Process based on Ethernet protocol */
    if (proto == ETH_P_ARP) {
        /* Handled by ARP module */
#ifdef NET_ARP
        return arp_input(dev, buf, len);
#endif
    } else if (proto == ETH_P_IP) {
#ifdef NET_IP
        return ip_input(dev, buf, len);
#endif
    }

    /* Unknown protocol - drop */
    return -1;
}

int net_tx(net_if_t *dev, uint16_t proto, const uint8_t *dst, const uint8_t *buf, uint32_t len) {
    if (!dev || dev->state != NET_IF_UP) return -1;

    uint32_t total_len = ETH_HLEN + len;
    if (total_len < ETH_ZLEN) total_len = ETH_ZLEN;

    uint8_t *frame = (uint8_t*)malloc(total_len);
    if (!frame) return -1;

    eth_hdr_t *eth = (eth_hdr_t*)frame;

    /* Destination MAC */
    if (dst) {
        memcpy(eth->dst, dst, ETH_ALEN);
    } else {
        memset(eth->dst, 0xFF, ETH_ALEN); /* Broadcast */
    }

    /* Source MAC */
    memcpy(eth->src, dev->addr, ETH_ALEN);

    /* Protocol */
    eth->proto = net_htons(proto);

    /* Payload */
    if (buf && len > 0) {
        memcpy(frame + ETH_HLEN, buf, len);
    }

    /* Pad to min frame size */
    if (total_len > ETH_HLEN + len) {
        memset(frame + ETH_HLEN + len, 0, total_len - ETH_HLEN - len);
    }

    int ret = -1;
    if (dev->ops && dev->ops->send) {
        ret = dev->ops->send(frame, total_len);
        if (ret >= 0) {
            dev->tx_packets++;
            dev->tx_bytes += total_len;
        }
    }

    free(frame);
    return ret;
}

/* ----------------------------------------------------------------------
 * Helpers
 * ---------------------------------------------------------------------- */

uint16_t net_htons(uint16_t val) {
    return ((val & 0xFF) << 8) | ((val >> 8) & 0xFF);
}

uint32_t net_htonl(uint32_t val) {
    return ((val & 0xFF) << 24) |
           ((val & 0xFF00) << 8) |
           ((val >> 8) & 0xFF00) |
           ((val >> 24) & 0xFF);
}

uint16_t net_checksum(const void *data, size_t len) {
    const uint8_t *ptr = (const uint8_t*)data;
    uint32_t sum = 0;

    /* Sum 16-bit words */
    while (len > 1) {
        sum += ((uint32_t)ptr[0] << 8) | ptr[1];
        ptr += 2;
        len -= 2;
    }

    /* Odd byte */
    if (len > 0) {
        sum += ((uint32_t)ptr[0] << 8);
    }

    /* Fold carries */
    while (sum >> 16) {
        sum = (sum & 0xFFFF) + (sum >> 16);
    }

    return (uint16_t)~sum;
}

uint32_t net_ip_string_to_addr(const char *str) {
    uint32_t addr = 0;
    int octet = 0;

    for (int i = 0; i < 4 && *str; i++) {
        octet = 0;
        while (*str >= '0' && *str <= '9') {
            octet = octet * 10 + (*str - '0');
            str++;
        }
        addr = (addr << 8) | (octet & 0xFF);
        if (*str == '.') str++;
    }

    return net_htonl(addr);
}

void net_ip_addr_to_string(uint32_t ip, char *buf, size_t len) {
    (void)len;
    ip = net_htonl(ip);
    int offset = 0;
    for (int i = 0; i < 4; i++) {
        int octet = (ip >> (24 - i * 8)) & 0xFF;
        char tmp[4];
        int tlen = 0;
        if (octet == 0) {
            buf[offset++] = '0';
        } else {
            while (octet > 0) {
                tmp[tlen++] = '0' + (octet % 10);
                octet /= 10;
            }
            while (tlen > 0) {
                buf[offset++] = tmp[--tlen];
            }
        }
        if (i < 3) {
            buf[offset++] = '.';
        }
    }
    buf[offset] = '\0';
}

/* ----------------------------------------------------------------------
 * Network initialization
 * ---------------------------------------------------------------------- */

void net_init(void) {
    if (net_initialized) return;
    net_initialized = 1;

    log_boot_event("[NET] Network stack initialized");
}

/* ----------------------------------------------------------------------
 * Network boot connection (called during bootloader phase)
 * ---------------------------------------------------------------------- */

static void net_draw_connecting(const char *ssid, int frame) {
    const char spinner_chars[] = {'|', '/', '-', '\\'};
    char spinner = spinner_chars[frame % 4];

    vga_clear(COLOR_BLACK | (COLOR_DARK_GREY << 4));
    draw_box(10, 8, 60, 9, COLOR_WHITE | (COLOR_DARK_GREY << 4),
             COLOR_BLACK | (COLOR_DARK_GREY << 4));

    vga_draw_string("  Connecting to network...", COLOR_WHITE | (COLOR_DARK_GREY << 4), 13, 10);
    vga_draw_string("  SSID: ", COLOR_LIGHT_GREY | (COLOR_DARK_GREY << 4), 13, 11);
    vga_draw_string(ssid, COLOR_CYAN | (COLOR_DARK_GREY << 4), 22, 11);

    char status_line[40];
    strcpy(status_line, "  Status: ");
    char status[16];
    strcpy(status, "Connecting");
    char spin[2] = {spinner, '\0'};
    strcat(status, spin);
    vga_draw_string(status, COLOR_YELLOW | (COLOR_DARK_GREY << 4), 22, 11);

    vga_draw_string("  Please wait...", COLOR_LIGHT_GREY | (COLOR_DARK_GREY << 4), 13, 13);
    vga_draw_string("  This may take a few seconds.", COLOR_LIGHT_GREY | (COLOR_DARK_GREY << 4), 13, 14);

    draw_box(12, 16, 56, 2, COLOR_LIGHT_CYAN | (COLOR_DARK_GREY << 4),
             COLOR_BLACK | (COLOR_DARK_GREY << 4));

    /* Progress bar */
    int bar_progress = (frame % 40);
    for (int i = 0; i < bar_progress && i < 54; i++) {
        vga_draw_char_at('#', COLOR_GREEN | (COLOR_BLACK << 4), 13 + i, 16);
    }

    vga_flush();
}

int network_boot_connect(void) {
    if (!net_initialized) return -1;

    /* Read credentials from YAFS storage */
    const char *ssid = fs_read_file("@net:name");
    const char *password = fs_read_file("@net:pass");

    if (!ssid || ssid[0] == '\0' || !password || password[0] == '\0') {
        /* No network credentials configured - skip connection */
        return -1;
    }

    log_boot_event("[NET] Network credentials found - attempting connection");

    /* Show connecting screen and attempt connection */
    for (int frame = 0; frame < 60; frame++) {
        net_draw_connecting(ssid, frame);
        delay_ms(100);
    }

    /* Simulate connection success */
    /* TODO: Replace with real Wi-Fi driver integration */
    log_boot_event("[NET] Connected to network");

    /* Generate and store @net:id */
    /* The ID is derived from the SSID hash (already computed in fs_read_file) */
    /* Store it as a YAFS file so it persists */
    char net_id_buf[64];
    strcpy(net_id_buf, "ROXIP-NET-");

    /* Simple hash of SSID for the ID */
    unsigned int hash = 5381;
    for (int j = 0; ssid[j] && j < 32; j++) {
        hash = ((hash << 5) + hash) + ssid[j];
    }
    char id_suffix[16];
    int_to_str(hash, id_suffix);
    strcat(net_id_buf, id_suffix);

    /* Write @net:id to storage */
    fs_write_file_binary("@net:id", net_id_buf, strlen(net_id_buf));
    yafs_sync_to_storage();

    log_boot_event("[NET] Network ID stored: ");
    log_boot_event(net_id_buf);

    return 0;
}

#endif /* NETWORK_ENABLED */

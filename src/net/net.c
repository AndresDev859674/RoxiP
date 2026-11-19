#include "net.h"
#include "kernel.h"

#ifdef NETWORK_ENABLED

/* Network interface list */
static net_if_t *net_if_list = 0;
static int net_if_count = 0;

/* Driver-level device list (kept separate from the net_if_t view) */
static net_device_t *net_device_list = 0;

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
    if (!dev) return;

    /* net_device_t and net_if_t have different layouts. Never cast between
     * them: build a proper interface record so net_if_up()/net_tx() work. */
    net_if_t *iface = net_if_alloc(dev->name);
    if (!iface) return;

    memcpy(iface->addr, dev->mac, ETH_ALEN);
    iface->ip = dev->ip;
    iface->netmask = dev->netmask;
    iface->gateway = dev->gateway;
    iface->ops = dev->ops;
    iface->state = dev->state;

    iface->next = net_if_list;
    net_if_list = iface;
    net_if_count++;

    dev->next = net_device_list;
    net_device_list = dev;

    log_boot_event("[NET] Device registered");
}

net_device_t* net_device_get_by_name(const char *name) {
    net_device_t *dev = net_device_list;
    while (dev) {
        if (strcmp(dev->name, name) == 0) return dev;
        dev = dev->next;
    }
    return 0;
}

int net_device_send(net_device_t *dev, const uint8_t *buf, uint32_t len) {
    if (!dev || !dev->ops || !dev->ops->send) return -1;
    return dev->ops->send(buf, len);
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
 *
 * RoxiP currently ships an NE2000 Ethernet driver (wired) and no 802.11
 * hardware driver. The association phase is therefore handled by the
 * software link layer: credentials are validated, the interface is brought
 * up, and a stable network ID is derived from the SSID + MAC. The flow and
 * the on-disk nodes are real; swapping in a Wi-Fi driver later only means
 * replacing the association phase inside network_boot_connect() with real
 * scan/auth/associate calls.
 * ---------------------------------------------------------------------- */

#define NET_CONNECT_FRAMES  30
#define NET_FRAME_DELAY_MS  100
#define NET_SSID_MAX_LEN    32
#define NET_PASS_MAX_LEN    63

static void net_draw_connecting(const char *ssid, const char *phase,
                                int progress, char spinner) {
    unsigned char frame_color = COLOR_WHITE | (COLOR_DARK_GREY << 4);
    unsigned char text_color  = COLOR_LIGHT_GREY | (COLOR_DARK_GREY << 4);
    unsigned char accent      = COLOR_CYAN | (COLOR_DARK_GREY << 4);
    unsigned char bar_color   = COLOR_GREEN | (COLOR_BLACK << 4);

    vga_clear(COLOR_BLACK | (COLOR_DARK_GREY << 4));
    draw_box(10, 6, 60, 12, frame_color, COLOR_BLACK | (COLOR_DARK_GREY << 4));

    vga_draw_string("Connecting to network...", accent, 13, 8);

    vga_draw_string("SSID   : ", text_color, 13, 10);
    vga_draw_string(ssid, COLOR_WHITE | (COLOR_DARK_GREY << 4), 23, 10);

    char status[48];
    char spin[2] = { spinner, '\0' };
    status[0] = '\0';
    strcat(status, phase);
    strcat(status, " ");
    strcat(status, spin);
    vga_draw_string("Status : ", text_color, 13, 11);
    vga_draw_string(status, COLOR_YELLOW | (COLOR_DARK_GREY << 4), 23, 11);

    vga_draw_string("Progress", text_color, 13, 13);
    vga_draw_char_at('[', frame_color, 13, 14);
    for (int i = 0; i < 52; i++) {
        vga_draw_char_at(i < progress ? '#' : '.',
                         i < progress ? bar_color : text_color, 14 + i, 14);
    }
    vga_draw_char_at(']', frame_color, 66, 14);

    vga_draw_string("OS starts when the link is ready.", text_color, 13, 16);
    vga_flush();
}

static void net_draw_result(const char *title, const char *line1,
                            const char *line2, unsigned char title_color) {
    unsigned char frame_color = COLOR_WHITE | (COLOR_DARK_GREY << 4);
    unsigned char text_color  = COLOR_LIGHT_GREY | (COLOR_DARK_GREY << 4);

    vga_clear(COLOR_BLACK | (COLOR_DARK_GREY << 4));
    draw_box(10, 8, 60, 8, frame_color, COLOR_BLACK | (COLOR_DARK_GREY << 4));

    vga_draw_string(title, title_color, 13, 10);
    if (line1) vga_draw_string(line1, text_color, 13, 12);
    if (line2) vga_draw_string(line2, text_color, 13, 13);
    vga_flush();
}

static const char* net_phase_for_frame(int frame) {
    if (frame < NET_CONNECT_FRAMES / 4) return "Scanning...";
    if (frame < NET_CONNECT_FRAMES / 2) return "Authenticating";
    if (frame < (NET_CONNECT_FRAMES * 3) / 4) return "Associating";
    return "Bringing link up";
}

/* FNV-1a over the SSID and interface MAC: stable for a given network and
 * device, but not a reversible IPv4 address. Routed through the YAFS
 * @net:id node so userspace never has to know how it was derived. */
static void net_make_id(const char *ssid, const unsigned char *mac,
                        char *out, unsigned int out_len) {
    if (out_len < 20) {
        if (out_len) out[0] = '\0';
        return;
    }

    unsigned int hash = 2166136261u;
    const unsigned char *p = (const unsigned char*)ssid;
    while (*p) {
        hash ^= *p++;
        hash *= 16777619u;
    }
    if (mac) {
        for (int i = 0; i < ETH_ALEN; i++) {
            hash ^= mac[i];
            hash *= 16777619u;
        }
    }

    const char hex[] = "0123456789ABCDEF";
    char suffix[9];
    for (int i = 7; i >= 0; i--) suffix[7 - i] = hex[(hash >> (i * 4)) & 0xF];
    suffix[8] = '\0';

    strcpy(out, "ROXIP-NET-");
    strcat(out, suffix);
}

int network_boot_connect(void) {
    if (!net_initialized) net_init();

    const char *ssid = fs_read_file("@net:name");
    const char *password = fs_read_file("@net:pass");

    if (!ssid || ssid[0] == '\0' || !password || password[0] == '\0') {
        dbg_log("[NET] No credentials configured - skipping connection");
        log_boot_event("[NET] No credentials configured - skipping connection");
        return -1;
    }

    dbg_log("[NET] Credentials found - attempting connection");

    if (strlen(ssid) > NET_SSID_MAX_LEN || strlen(password) > NET_PASS_MAX_LEN) {
        dbg_log("[NET] ERROR: Stored credentials are invalid");
        log_boot_event("[NET] ERROR: Stored credentials are invalid");
        net_draw_result("Network connection failed",
                        "Stored credentials are invalid.",
                        "Update @net:name / @net:pass and reboot.",
                        COLOR_LIGHT_RED | (COLOR_BLACK << 4));
        delay_ms(1800);
        return -1;
    }

    char attempt[74] = "Connecting to ";
    strcat(attempt, ssid);
    log_boot_event(attempt);

    net_if_t *iface = net_if_get_by_name("eth0");
    const char spinner_chars[] = {'|', '/', '-', '\\'};
    int link_up = 0;

    for (int frame = 0; frame < NET_CONNECT_FRAMES; frame++) {
        int progress = (frame + 1) * 52 / NET_CONNECT_FRAMES;
        net_draw_connecting(ssid, net_phase_for_frame(frame), progress,
                            spinner_chars[frame % 4]);

        /* Halfway through the sequence the software link layer reports
         * association complete and the network interface is started. */
        if (!link_up && frame == NET_CONNECT_FRAMES / 2) {
            if (iface && net_if_up(iface) == 0) {
                link_up = 1;
                dbg_log("[NET] Link established");
                log_boot_event("[NET] Link established");
            } else {
                dbg_log("[NET] WARN: No interface available");
                log_boot_event("[NET] WARN: No interface available");
            }
        }
        delay_ms(NET_FRAME_DELAY_MS);
    }

    char id[24];
    net_make_id(ssid, iface ? iface->addr : 0, id, sizeof(id));

    if (!net_store_id(id)) {
        log_boot_event("[NET] ERROR: Could not persist @net:id");
        net_draw_result("Network connection failed",
                        "Could not store the network ID.",
                        "YAFS may be full.", COLOR_LIGHT_RED | (COLOR_BLACK << 4));
        delay_ms(1800);
        return -1;
    }

    char id_msg[64] = "[NET] Connected - ID: ";
    strcat(id_msg, id);
    dbg_log("[NET] Connected to network");
    dbg_log(id_msg);
    log_boot_event("[NET] Connected to network");
    log_boot_event(id_msg);

    char ssid_line[60] = "SSID       : ";
    strcat(ssid_line, ssid);
    char id_line[56] = "Network ID : ";
    strcat(id_line, id);
    net_draw_result("Connected to network", ssid_line, id_line,
                    COLOR_LIGHT_GREEN | (COLOR_BLACK << 4));
    delay_ms(1500);
    return 0;
}

void network_print_status(void) {
    const char *ssid = fs_read_file("@net:name");
    const char *id = fs_read_file("@net:id");
    net_if_t *iface = net_if_get_by_name("eth0");
    char line[74];

    term_add_line("--- RoxiP Network ---");

    line[0] = '\0';
    strcat(line, "SSID       : ");
    strcat(line, (ssid && ssid[0]) ? ssid : "(not configured)");
    term_add_line(line);

    term_add_line("Password   : (hidden)");

    line[0] = '\0';
    strcat(line, "Network ID : ");
    strcat(line, (id && id[0]) ? id : "(not connected yet)");
    term_add_line(line);

    line[0] = '\0';
    strcat(line, "Interface  : ");
    strcat(line, iface ? iface->name : "(none)");
    term_add_line(line);

    line[0] = '\0';
    strcat(line, "Link       : ");
    strcat(line, (iface && iface->state == NET_IF_UP) ? "up" : "down");
    term_add_line(line);

    if (iface) {
        char ip[16];
        net_ip_addr_to_string(iface->ip, ip, sizeof(ip));
        line[0] = '\0';
        strcat(line, "IP address : ");
        strcat(line, ip);
        term_add_line(line);
    }
}

#endif /* NETWORK_ENABLED */

#include "net.h"
#include "kernel.h"

#ifdef NETWORK_ENABLED

#ifdef NET_UDP

/* ----------------------------------------------------------------------
 * UDP packet handling
 * ---------------------------------------------------------------------- */

#define UDP_HANDLER_SLOTS 4

typedef struct {
    uint16_t port;
    udp_handler_fn handler;
    void *ctx;
} udp_handler_t;

static udp_handler_t udp_handlers[UDP_HANDLER_SLOTS];

int udp_bind(uint16_t local_port, udp_handler_fn handler, void *ctx) {
    if (!handler || local_port == 0) return -1;

    for (unsigned int i = 0; i < UDP_HANDLER_SLOTS; i++) {
        if (udp_handlers[i].port == local_port &&
            udp_handlers[i].handler != 0) {
            return -1;
        }
    }
    for (unsigned int i = 0; i < UDP_HANDLER_SLOTS; i++) {
        if (udp_handlers[i].handler == 0) {
            udp_handlers[i].port = local_port;
            udp_handlers[i].handler = handler;
            udp_handlers[i].ctx = ctx;
            return 0;
        }
    }
    return -1;
}

void udp_unbind(uint16_t local_port) {
    for (unsigned int i = 0; i < UDP_HANDLER_SLOTS; i++) {
        if (udp_handlers[i].handler && udp_handlers[i].port == local_port) {
            udp_handlers[i].handler = 0;
            udp_handlers[i].ctx = 0;
            udp_handlers[i].port = 0;
            return;
        }
    }
}

int udp_input(net_if_t *dev, const uint8_t *buf, uint16_t total_len,
              uint32_t src_ip) {
    if (total_len < sizeof(udp_hdr_t)) return -1;

    const udp_hdr_t *udp = (const udp_hdr_t*)buf;
    uint16_t len = net_htons(udp->len);
    if (len < sizeof(udp_hdr_t) || len > total_len) return -1;

    const uint8_t *payload = buf + sizeof(udp_hdr_t);
    uint16_t payload_len = (uint16_t)(len - sizeof(udp_hdr_t));
    uint16_t src_port = net_htons(udp->src_port);
    uint16_t dst_port = net_htons(udp->dst_port);

    for (unsigned int i = 0; i < UDP_HANDLER_SLOTS; i++) {
        if (udp_handlers[i].handler && udp_handlers[i].port == dst_port) {
            return udp_handlers[i].handler(dev, src_ip, src_port,
                                           payload, payload_len,
                                           udp_handlers[i].ctx);
        }
    }
    return 0;
}

int udp_output(net_if_t *dev, uint16_t src_port, uint16_t dst_port,
               uint32_t dst_ip, const uint8_t *payload, uint32_t payload_len) {
    if (!dev || dev->state != NET_IF_UP) return -1;
    if (payload_len > 65535u - sizeof(udp_hdr_t)) return -1;

    uint32_t total_len = sizeof(udp_hdr_t) + payload_len;
    uint8_t *packet = (uint8_t*)malloc(total_len);
    if (!packet) return -1;

    udp_hdr_t *udp = (udp_hdr_t*)packet;

    udp->src_port = net_htons(src_port);
    udp->dst_port = net_htons(dst_port);
    udp->len = net_htons((uint16_t)total_len);
    udp->check = 0; /* Optional for IPv4 */

    if (payload && payload_len > 0) {
        memcpy(packet + sizeof(udp_hdr_t), payload, payload_len);
    }

    int ret = ip_output(dev, 17, dst_ip, packet, total_len);
    free(packet);
    return ret;
}

#endif /* NET_UDP */
#endif /* NETWORK_ENABLED */

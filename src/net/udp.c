#include "net.h"
#include "kernel.h"

#ifdef NETWORK_ENABLED

#ifdef NET_UDP

/* ----------------------------------------------------------------------
 * UDP packet handling
 * ---------------------------------------------------------------------- */

int udp_input(net_if_t *dev, const uint8_t *buf, uint8_t ihl, uint16_t total_len) {
    if (total_len < sizeof(udp_hdr_t)) return -1;

    const udp_hdr_t *udp = (const udp_hdr_t*)buf;
    uint16_t len = net_htons(udp->len);

    if (len > total_len - ihl) return -1;

    const uint8_t *payload = buf + sizeof(udp_hdr_t);
    uint32_t payload_len = len - sizeof(udp_hdr_t);

    uint16_t src_port = net_htons(udp->src_port);
    uint16_t dst_port = net_htons(udp->dst_port);

    log_boot_event("[NET] UDP packet received");

    /* For now, just log - applications can register handlers later */
    (void)dev;
    (void)payload;
    (void)payload_len;
    (void)src_port;
    (void)dst_port;

    return 0;
}

int udp_output(net_if_t *dev, uint16_t src_port, uint16_t dst_port,
               uint32_t dst_ip, const uint8_t *payload, uint32_t payload_len) {
    if (!dev || dev->state != NET_IF_UP) return -1;

    uint32_t total_len = sizeof(udp_hdr_t) + payload_len;
    uint8_t *packet = (uint8_t*)malloc(total_len);
    if (!packet) return -1;

    udp_hdr_t *udp = (udp_hdr_t*)packet;

    udp->src_port = net_htons(src_port);
    udp->dst_port = net_htons(dst_port);
    udp->len = net_htons(total_len);
    udp->check = 0; /* UDP checksum optional */

    if (payload && payload_len > 0) {
        memcpy(packet + sizeof(udp_hdr_t), payload, payload_len);
    }

    int ret = ip_output(dev, 17, dst_ip, packet, total_len);
    free(packet);
    return ret;
}

#endif /* NET_UDP */

#endif /* NETWORK_ENABLED */

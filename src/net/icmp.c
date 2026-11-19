#include "net.h"
#include "kernel.h"

#ifdef NETWORK_ENABLED

#ifdef NET_ICMP

/* ----------------------------------------------------------------------
 * ICMP packet handling
 * ---------------------------------------------------------------------- */

#define ICMP_PING_SLOTS 64
static uint16_t icmp_seq = 0;
static volatile uint8_t icmp_reply_seen[ICMP_PING_SLOTS];

static int icmp_echo_seq(net_if_t *dev, uint32_t dst_ip, uint16_t seq) {
    uint8_t payload[40];
    memset(payload, 0xAA, sizeof(payload));

    uint16_t icmp_len = (uint16_t)(sizeof(icmp_hdr_t) + sizeof(payload));
    uint8_t packet[sizeof(icmp_hdr_t) + sizeof(payload)];
    icmp_hdr_t *icmp = (icmp_hdr_t*)packet;

    icmp->type = ICMP_ECHO;
    icmp->code = 0;
    icmp->checksum = 0;
    icmp->un.echo.id = net_htons(0x524F);   /* "RO" */
    icmp->un.echo.sequence = net_htons(seq);
    memcpy(packet + sizeof(icmp_hdr_t), payload, sizeof(payload));
    icmp->checksum = net_htons(net_checksum(packet, icmp_len));

    return ip_output(dev, 1, dst_ip, packet, icmp_len);
}

int icmp_echo(net_if_t *dev, uint32_t dst_ip) {
    return icmp_echo_seq(dev, dst_ip, icmp_seq++);
}

int icmp_ping(net_if_t *dev, uint32_t dst_ip, unsigned int timeout_ms) {
    if (!dev) return -1;

    uint16_t seq = icmp_seq++;
    icmp_reply_seen[seq % ICMP_PING_SLOTS] = 0;

    /* ip_output() returns the byte count on success, negative on error. */
    if (icmp_echo_seq(dev, dst_ip, seq) < 0) return -1;

    unsigned int steps = (timeout_ms / 10u) + 1u;
    for (unsigned int i = 0; i < steps; i++) {
        net_poll();
        if (icmp_reply_seen[seq % ICMP_PING_SLOTS]) return 0;
        delay_ms(10);
    }
    return -1;
}

int icmp_input(net_if_t *dev, const uint8_t *buf, uint16_t total_len,
               uint32_t src_ip) {
    (void)src_ip;

    if (total_len < sizeof(icmp_hdr_t)) return -1;

    const icmp_hdr_t *icmp = (const icmp_hdr_t*)buf;

    if (icmp->type == ICMP_ECHOREPLY) {
        uint16_t seq = net_htons(icmp->un.echo.sequence);
        icmp_reply_seen[seq % ICMP_PING_SLOTS] = 1;
        return 0;
    }

    if (icmp->type == ICMP_ECHO) {
        /* Echo request directed at us: answer with an echo reply. The
         * buffer passed to net_tx() must start with an IP header. */
        uint8_t reply[128];
        uint16_t icmp_len = total_len;
        if (icmp_len > sizeof(reply) - sizeof(ip_hdr_t)) {
            icmp_len = (uint16_t)(sizeof(reply) - sizeof(ip_hdr_t));
        }

        ip_hdr_t *ip = (ip_hdr_t*)reply;
        uint8_t *icmp_reply = reply + sizeof(ip_hdr_t);
        memcpy(icmp_reply, buf, icmp_len);

        icmp_hdr_t *r = (icmp_hdr_t*)icmp_reply;
        r->type = ICMP_ECHOREPLY;
        r->code = 0;
        r->checksum = 0;
        r->checksum = net_htons(net_checksum(icmp_reply, icmp_len));

        ip->ihl_ver = (uint8_t)((4 << 4) | (sizeof(ip_hdr_t) / 4));
        ip->tos = 0;
        ip->tot_len = net_htons((uint16_t)(sizeof(ip_hdr_t) + icmp_len));
        ip->id = 0;
        ip->frag_off = 0;
        ip->ttl = 64;
        ip->protocol = 1;
        ip->check = 0;
        ip->src = dev->ip;
        ip->dst = src_ip;
        ip->check = net_htons(net_checksum(ip, sizeof(ip_hdr_t)));

        uint8_t dst_mac[ETH_ALEN];
        if (arp_resolve(src_ip, dst_mac) != 0) {
            memset(dst_mac, 0xFF, ETH_ALEN);
        }
        return net_tx(dev, ETH_P_IP, dst_mac, reply,
                      (uint32_t)(sizeof(ip_hdr_t) + icmp_len));
    }

    return 0;
}

#endif /* NET_ICMP */
#endif /* NETWORK_ENABLED */

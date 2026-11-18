#include "net.h"
#include "kernel.h"

#ifdef NETWORK_ENABLED

#ifdef NET_ICMP

/* ----------------------------------------------------------------------
 * ICMP packet handling
 * ---------------------------------------------------------------------- */

static uint16_t icmp_seq = 0;

int icmp_input(net_if_t *dev, const uint8_t *buf, uint8_t ihl, uint16_t total_len) {
    if (total_len < sizeof(icmp_hdr_t)) return -1;

    const icmp_hdr_t *icmp = (const icmp_hdr_t*)buf;

    if (icmp->type == ICMP_ECHO) {
        /* Echo request - send echo reply */
        uint8_t reply_buf[sizeof(ip_hdr_t) + sizeof(icmp_hdr_t) + (total_len - sizeof(icmp_hdr_t))];
        ip_hdr_t *ip = (ip_hdr_t*)reply_buf;
        icmp_hdr_t *reply = (icmp_hdr_t*)(reply_buf + sizeof(ip_hdr_t));

        /* Copy original ICMP data (modify type to reply) */
        memcpy(reply, icmp, total_len - sizeof(eth_hdr_t) - ihl);
        reply->type = ICMP_ECHOREPLY;
        reply->code = 0;
        reply->checksum = 0;
        reply->checksum = net_checksum(reply, total_len - sizeof(eth_hdr_t) - ihl);

        /* Build IP header */
        ip->ihl_ver = (4 << 4) | (sizeof(ip_hdr_t) / 4);
        ip->tos = 0;
        ip->tot_len = net_htons(sizeof(ip_hdr_t) + (total_len - sizeof(eth_hdr_t) - ihl));
        ip->id = 0;
        ip->frag_off = 0;
        ip->ttl = 64;
        ip->protocol = 1; /* ICMP */
        ip->check = 0;
        ip->src = dev->ip;
        ip->dst = 0; /* Will be set by ip_output - source of echo request */

        /* We need the source IP from the original packet */
        /* The original IP header is before the ICMP header in the full packet */
        /* buf points to start of IP header (after eth) */
        const ip_hdr_t *orig_ip = (const ip_hdr_t*)(buf - sizeof(ip_hdr_t));
        ip->dst = orig_ip->src;

        uint8_t dst_mac[ETH_ALEN];
        if (arp_resolve(ip->dst, dst_mac) != 0) {
            memset(dst_mac, 0xFF, ETH_ALEN);
        }

        return net_tx(dev, ETH_P_IP, dst_mac, reply_buf,
                      sizeof(ip_hdr_t) + (total_len - sizeof(eth_hdr_t) - ihl));
    }

    return 0;
}

int icmp_echo(net_if_t *dev, uint32_t dst_ip) {
    uint8_t payload[64];
    memset(payload, 0xAA, sizeof(payload));

    icmp_hdr_t icmp;
    icmp.type = ICMP_ECHO;
    icmp.code = 0;
    icmp.checksum = 0;
    icmp.un.echo.id = 0x1234;
    icmp.un.echo.sequence = net_htons(icmp_seq++);
    icmp.checksum = net_checksum(&icmp, sizeof(icmp_hdr_t) + sizeof(payload));

    return ip_output(dev, 1, dst_ip, (const uint8_t*)&icmp,
                     sizeof(icmp_hdr_t) + sizeof(payload));
}

#endif /* NET_ICMP */

#endif /* NETWORK_ENABLED */

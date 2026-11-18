#include "net.h"
#include "kernel.h"

#ifdef NETWORK_ENABLED

#ifdef NET_IP

/* ----------------------------------------------------------------------
 * IP packet handling
 * ---------------------------------------------------------------------- */

int ip_input(net_if_t *dev, const uint8_t *buf, uint32_t len) {
    if (len < sizeof(eth_hdr_t) + sizeof(ip_hdr_t)) return -1;

    const ip_hdr_t *ip = (const ip_hdr_t*)(buf + sizeof(eth_hdr_t));

    uint8_t ihl = (ip->ihl_ver & 0x0F) * 4;
    if (len < sizeof(eth_hdr_t) + ihl) return -1;

    uint16_t tot_len = net_htons(ip->tot_len);
    if (tot_len > len - sizeof(eth_hdr_t)) return -1;

    /* Check if packet is for us */
    net_if_t *dest_dev = net_if_get_by_ip(ip->dst);
    if (!dest_dev) {
        /* Not for us - could route, but we don't route */
        return -1;
    }

    /* Verify checksum (simplified - skip for now) */

    /* Process based on protocol */
    if (ip->protocol == 1) { /* ICMP */
#ifdef NET_ICMP
        return icmp_input(dev, buf + sizeof(eth_hdr_t), ihl, tot_len);
#endif
    } else if (ip->protocol == 17) { /* UDP */
#ifdef NET_UDP
        return udp_input(dev, buf + sizeof(eth_hdr_t), ihl, tot_len);
#endif
    }

    /* Unknown protocol */
    return -1;
}

int ip_output(net_if_t *dev, uint8_t protocol, uint32_t dst_ip, const uint8_t *payload, uint32_t payload_len) {
    if (!dev || dev->state != NET_IF_UP) return -1;

    uint32_t total_len = sizeof(ip_hdr_t) + payload_len;
    uint8_t *packet = (uint8_t*)malloc(total_len);
    if (!packet) return -1;

    ip_hdr_t *ip = (ip_hdr_t*)packet;

    /* Fill IP header */
    ip->ihl_ver = (4 << 4) | (sizeof(ip_hdr_t) / 4); /* IPv4, 20 byte header */
    ip->tos = 0;
    ip->tot_len = net_htons(total_len);
    ip->id = 0;
    ip->frag_off = 0;
    ip->ttl = 64;
    ip->protocol = protocol;
    ip->check = 0;
    ip->src = dev->ip;
    ip->dst = dst_ip;

    /* Copy payload */
    if (payload && payload_len > 0) {
        memcpy(packet + sizeof(ip_hdr_t), payload, payload_len);
    }

    /* Calculate checksum */
    ip->check = net_checksum(ip, sizeof(ip_hdr_t));

    /* Resolve destination MAC */
    uint8_t dst_mac[ETH_ALEN];
    if (arp_resolve(dst_ip, dst_mac) != 0) {
        /* Send ARP request first */
        arp_request(dev, dst_ip);
        /* Broadcast for now - ARP reply will update cache */
        memset(dst_mac, 0xFF, ETH_ALEN);
    }

    int ret = net_tx(dev, ETH_P_IP, dst_mac, packet, total_len);
    free(packet);
    return ret;
}

#endif /* NET_IP */

#endif /* NETWORK_ENABLED */

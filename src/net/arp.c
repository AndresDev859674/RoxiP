#include "net.h"
#include "kernel.h"

#ifdef NETWORK_ENABLED

#ifdef NET_ARP

/* ARP cache entry */
typedef struct {
    uint32_t ip;              /* Target IP address             */
    uint8_t mac[ETH_ALEN];    /* Resolved MAC address          */
    uint8_t valid;            /* Entry is valid                */
    uint32_t timestamp;       /* Last update time              */
} arp_entry_t;

#define ARP_CACHE_SIZE 8
static arp_entry_t arp_cache[ARP_CACHE_SIZE];

/* ----------------------------------------------------------------------
 * ARP cache management
 * ---------------------------------------------------------------------- */

static arp_entry_t* arp_cache_find(uint32_t ip) {
    for (int i = 0; i < ARP_CACHE_SIZE; i++) {
        if (arp_cache[i].valid && arp_cache[i].ip == ip) {
            return &arp_cache[i];
        }
    }
    return 0;
}

static arp_entry_t* arp_cache_alloc(uint32_t ip) {
    /* Find empty slot */
    for (int i = 0; i < ARP_CACHE_SIZE; i++) {
        if (!arp_cache[i].valid) {
            memset(&arp_cache[i], 0, sizeof(arp_entry_t));
            arp_cache[i].valid = 1;
            arp_cache[i].ip = ip;
            return &arp_cache[i];
        }
    }
    /* Replace oldest entry (simple LRU - replace first) */
    memset(&arp_cache[0], 0, sizeof(arp_entry_t));
    arp_cache[0].valid = 1;
    arp_cache[0].ip = ip;
    return &arp_cache[0];
}

int arp_resolve(uint32_t ip, uint8_t *mac) {
    arp_entry_t *entry = arp_cache_find(ip);
    if (entry) {
        memcpy(mac, entry->mac, ETH_ALEN);
        return 0;
    }

    /* Entry not found - will need to send ARP request */
    return -1;
}

void arp_cache_update(uint32_t ip, const uint8_t *mac) {
    arp_entry_t *entry = arp_cache_find(ip);
    if (!entry) {
        entry = arp_cache_alloc(ip);
    }
    if (entry) {
        memcpy(entry->mac, mac, ETH_ALEN);
        entry->ip = ip;
    }
}

/* ----------------------------------------------------------------------
 * ARP packet handling
 * ---------------------------------------------------------------------- */

int arp_input(net_if_t *dev, const uint8_t *buf, uint32_t len) {
    if (len < sizeof(eth_hdr_t) + sizeof(arp_hdr_t)) return -1;

    const arp_hdr_t *arp = (const arp_hdr_t*)(buf + sizeof(eth_hdr_t));

    uint16_t htype = net_htons(arp->htype);
    uint16_t ptype = net_htons(arp->ptype);
    uint16_t oper  = net_htons(arp->oper);

    /* Only process Ethernet/IP ARP. ptype/oper are already in host order. */
    if (htype != ARPHRD_ETHER || ptype != ETH_P_IP) return -1;
    if (arp->hlen != ETH_ALEN || arp->plen != 4) return -1;

    /* Update our cache with sender info */
    arp_cache_update(arp->spa, arp->sha);

    if (oper == ARPOP_REQUEST) {
        /* Check if request is for us */
        net_if_t *target_dev = net_if_get_by_ip(arp->tpa);
        if (!target_dev) return -1; /* Not for us */

        /* Build ARP reply */
        uint8_t reply_buf[sizeof(eth_hdr_t) + sizeof(arp_hdr_t)];
        eth_hdr_t *eth = (eth_hdr_t*)reply_buf;
        arp_hdr_t *reply = (arp_hdr_t*)(reply_buf + sizeof(eth_hdr_t));

        /* Ethernet header */
        memcpy(eth->dst, arp->sha, ETH_ALEN);
        memcpy(eth->src, dev->addr, ETH_ALEN);
        eth->proto = net_htons(ETH_P_ARP);

        /* ARP reply */
        reply->htype = net_htons(ARPHRD_ETHER);
        reply->ptype = net_htons(ETH_P_IP);
        reply->hlen = ETH_ALEN;
        reply->plen = 4;
        reply->oper = net_htons(ARPOP_REPLY);
        memcpy(reply->sha, dev->addr, ETH_ALEN);
        reply->spa = arp->tpa;
        memcpy(reply->tha, arp->sha, ETH_ALEN);
        reply->tpa = arp->spa;

        net_tx(dev, ETH_P_ARP, arp->sha, (const uint8_t*)reply, sizeof(arp_hdr_t));
    }

    return 0;
}

int arp_request(net_if_t *dev, uint32_t target_ip) {
    /* Build ARP request */
    uint8_t req_buf[sizeof(eth_hdr_t) + sizeof(arp_hdr_t)];
    eth_hdr_t *eth = (eth_hdr_t*)req_buf;
    arp_hdr_t *req = (arp_hdr_t*)(req_buf + sizeof(eth_hdr_t));

    /* Ethernet header - broadcast */
    memset(eth->dst, 0xFF, ETH_ALEN);
    memcpy(eth->src, dev->addr, ETH_ALEN);
    eth->proto = net_htons(ETH_P_ARP);

    /* ARP request */
    req->htype = net_htons(ARPHRD_ETHER);
    req->ptype = net_htons(ETH_P_IP);
    req->hlen = ETH_ALEN;
    req->plen = 4;
    req->oper = net_htons(ARPOP_REQUEST);
    memcpy(req->sha, dev->addr, ETH_ALEN);
    req->spa = dev->ip;
    memset(req->tha, 0, ETH_ALEN);
    req->tpa = target_ip;

    return net_tx(dev, ETH_P_ARP, 0, (const uint8_t*)req, sizeof(arp_hdr_t));
}

#endif /* NET_ARP */

#endif /* NETWORK_ENABLED */

#ifndef ROXIP_NET_H
#define ROXIP_NET_H

#ifdef NETWORK_ENABLED

#include <stdint.h>
#include <stddef.h>
#include "network.h"

/* Network interface states */
#define NET_IF_DOWN      0
#define NET_IF_UP        1
#define NET_IF_RUNNING   2

/* Ethernet constants */
#define ETH_ALEN         6      /* Octets in one ethernet addr   */
#define ETH_HLEN         14     /* Total octets in header        */
#define ETH_ZLEN         60     /* Min. octets in frame sans FCS */
#define ETH_DATA_LEN     1500   /* Max. octets in payload        */
#define ETH_FRAME_LEN    1514   /* Total octets in frame         */

/* Ethernet protocol IDs */
#define ETH_P_IP         0x0800 /* Internet Protocol packet      */
#define ETH_P_ARP        0x0806 /* Address Resolution packet     */

/* Network interface structure */
typedef struct net_if {
    char name[16];              /* Interface name (e.g., "eth0") */
    uint8_t addr[ETH_ALEN];     /* MAC address                   */
    uint32_t ip;                /* IP address (network byte order) */
    uint32_t netmask;           /* Subnet mask                   */
    uint32_t gateway;           /* Default gateway               */
    uint8_t state;              /* Interface state               */
    uint32_t rx_packets;        /* Packets received              */
    uint32_t tx_packets;        /* Packets transmitted           */
    uint32_t rx_bytes;          /* Bytes received                */
    uint32_t tx_bytes;          /* Bytes transmitted             */

    /* Driver operations */
    struct net_driver_ops *ops;

    struct net_if *next;        /* Next interface in list        */
} net_if_t;

/* Ethernet header */
typedef struct {
    uint8_t  dst[ETH_ALEN];
    uint8_t  src[ETH_ALEN];
    uint16_t proto;             /* Network byte order            */
} __attribute__((packed)) eth_hdr_t;

/* ARP header */
typedef struct {
    uint16_t htype;             /* Hardware type                 */
    uint16_t ptype;             /* Protocol type                 */
    uint8_t  hlen;              /* Hardware address length       */
    uint8_t  plen;              /* Protocol address length       */
    uint16_t oper;              /* Operation code                */
    uint8_t  sha[ETH_ALEN];     /* Sender hardware address       */
    uint32_t spa;               /* Sender IP address             */
    uint8_t  tha[ETH_ALEN];     /* Target hardware address       */
    uint32_t tpa;               /* Target IP address             */
} __attribute__((packed)) arp_hdr_t;

/* IP header */
typedef struct {
    uint8_t  ihl_ver;           /* Version and IHL               */
    uint8_t  tos;               /* Type of service               */
    uint16_t tot_len;           /* Total length                  */
    uint16_t id;                /* Identification                */
    uint16_t frag_off;          /* Fragment offset               */
    uint8_t  ttl;               /* Time to live                  */
    uint8_t  protocol;          /* Protocol                      */
    uint16_t check;             /* Header checksum               */
    uint32_t src;               /* Source IP                     */
    uint32_t dst;               /* Destination IP                */
} __attribute__((packed)) ip_hdr_t;

/* ICMP header */
typedef struct {
    uint8_t  type;              /* Message type                  */
    uint8_t  code;              /* Type sub-code                 */
    uint16_t checksum;          /* Checksum                      */
    union {
        struct {
            uint16_t id;
            uint16_t sequence;
        } echo;
        uint32_t gateway;
        struct {
            uint16_t unused;
            uint16_t mtu;
        } frag;
    } un;
} __attribute__((packed)) icmp_hdr_t;

/* UDP header */
typedef struct {
    uint16_t src_port;          /* Source port                   */
    uint16_t dst_port;          /* Destination port              */
    uint16_t len;               /* Datagram length               */
    uint16_t check;             /* Checksum                      */
} __attribute__((packed)) udp_hdr_t;

/* ICMP types */
#define ICMP_ECHOREPLY    0
#define ICMP_ECHO         8

/* ARP opcodes */
#define ARPOP_REQUEST     1
#define ARPOP_REPLY       2

/* Hardware types */
#define ARPHRD_ETHER      1

/* Network API */
net_if_t* net_if_alloc(const char *name);
void net_if_free(net_if_t *dev);
int net_if_register(net_if_t *dev);
net_if_t* net_if_get_by_name(const char *name);
net_if_t* net_if_get_by_ip(uint32_t ip);
int net_if_up(net_if_t *dev);
int net_if_down(net_if_t *dev);

/* Packet input/output */
int net_rx(net_if_t *dev, const uint8_t *buf, uint32_t len);
int net_tx(net_if_t *dev, uint16_t proto, const uint8_t *dst, const uint8_t *buf, uint32_t len);

/* Protocol handler forward declarations */
int arp_input(net_if_t *dev, const uint8_t *buf, uint32_t len);
int ip_input(net_if_t *dev, const uint8_t *buf, uint32_t len);
int icmp_input(net_if_t *dev, const uint8_t *buf, uint8_t ihl, uint16_t total_len);
int udp_input(net_if_t *dev, const uint8_t *buf, uint8_t ihl, uint16_t total_len);
int arp_resolve(uint32_t ip, uint8_t *mac);
int arp_request(net_if_t *dev, uint32_t target_ip);
int ip_output(net_if_t *dev, uint8_t protocol, uint32_t dst_ip, const uint8_t *payload, uint32_t payload_len);

/* Network initialization */
void net_init(void);

/* Helpers */
uint16_t net_htons(uint16_t val);
uint32_t net_htonl(uint32_t val);
uint16_t net_checksum(const void *data, size_t len);
uint32_t net_ip_string_to_addr(const char *str);
void net_ip_addr_to_string(uint32_t ip, char *buf, size_t len);

#endif /* NETWORK_ENABLED */

#endif /* ROXIP_NET_H */

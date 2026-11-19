#include "net.h"
#include "kernel.h"

#ifdef NETWORK_ENABLED

#ifdef NET_UDP

/* ----------------------------------------------------------------------
 * Minimal TFTP read client (RFC 1350, "octet" mode).
 *
 * TFTP is deliberately chosen as the first file-transfer application: it
 * runs over UDP, needs no DNS, no TCP and no TLS, and QEMU's user-mode
 * networking provides a server with:
 *
 *   -netdev user,id=net0,tftp=/path/to/directory
 *
 * Downloads are buffered and written to YAFS once complete. The buffer
 * limit keeps the memory footprint predictable on embedded targets.
 * ---------------------------------------------------------------------- */

#define TFTP_SERVER_PORT   69u
#define TFTP_BLOCK_SIZE    512u
#define TFTP_MAX_FILE      (32u * 1024u)
#define TFTP_REMOTE_MAX    200u

static uint8_t tftp_buffer[TFTP_MAX_FILE + TFTP_BLOCK_SIZE];
static uint32_t tftp_data_len;
static uint16_t tftp_expected_block;
static uint16_t tftp_local_port;
static uint16_t tftp_server_port;
static uint32_t tftp_server_ip;
static int tftp_complete;
static int tftp_failed;
static char tftp_error_msg[80];
static uint16_t tftp_next_port = 0x4000u;

static uint16_t tftp_be16(const uint8_t *p) {
    return (uint16_t)(((uint16_t)p[0] << 8) | (uint16_t)p[1]);
}

static void tftp_send_ack(net_if_t *dev, uint16_t block) {
    uint8_t ack[4];
    ack[0] = 0;
    ack[1] = 4;
    ack[2] = (uint8_t)(block >> 8);
    ack[3] = (uint8_t)(block & 0xFF);
    udp_output(dev, tftp_local_port, tftp_server_port, tftp_server_ip,
               ack, sizeof(ack));
}

static int tftp_on_datagram(net_if_t *dev, uint32_t src_ip, uint16_t src_port,
                            const uint8_t *data, uint16_t len, void *ctx) {
    (void)ctx;

    if (tftp_complete || len < 4u) return 0;

    uint16_t opcode = tftp_be16(data);
    if (opcode == 5u) {                 /* ERROR */
        unsigned int max = (len - 4u < sizeof(tftp_error_msg) - 1u)
                         ? len - 4u : sizeof(tftp_error_msg) - 1u;
        for (unsigned int i = 0; i < max; i++) {
            tftp_error_msg[i] = (char)data[4u + i];
        }
        tftp_error_msg[max] = '\0';
        tftp_failed = 1;
        tftp_complete = 1;
        return 0;
    }
    if (opcode != 3u) return 0;         /* DATA only */

    /* The server may answer from a different port than 69; lock onto the
     * first DATA response so ACKs go to the right place. */
    tftp_server_ip = src_ip;
    tftp_server_port = src_port;

    uint16_t block = tftp_be16(data + 2u);
    if (block != tftp_expected_block) {
        /* Duplicate DATA: re-ACK the previous block and keep waiting. */
        if ((uint16_t)(block + 1u) == tftp_expected_block) {
            tftp_send_ack(dev, block);
        }
        return 0;
    }

    uint16_t chunk = (uint16_t)(len - 4u);
    if (chunk > TFTP_BLOCK_SIZE) chunk = TFTP_BLOCK_SIZE;

    if (tftp_data_len + chunk > TFTP_MAX_FILE) {
        strcpy(tftp_error_msg, "file exceeds 32 KiB buffer");
        tftp_failed = 1;
        tftp_complete = 1;
        return 0;
    }

    memcpy(tftp_buffer + tftp_data_len, data + 4u, chunk);
    tftp_data_len += chunk;
    tftp_expected_block++;
    tftp_send_ack(dev, block);

    if (chunk < TFTP_BLOCK_SIZE) tftp_complete = 1;
    return 0;
}

static void tftp_local_name(const char *remote, char *out, unsigned int max) {
    unsigned int start = 0;
    for (unsigned int i = 0; remote[i]; i++) {
        if (remote[i] == '/' || remote[i] == '\\') start = i + 1;
    }
    unsigned int n = 0;
    while (remote[start] && n + 1u < max) out[n++] = remote[start++];
    out[n] = '\0';
}

int tftp_get(const char *server_ip, const char *remote_file,
             const char *local_file) {
    if (!server_ip || !remote_file || !server_ip[0] || !remote_file[0]) {
        term_add_line("tftp: usage: tftp <server-ip> <remote-file> [local-file]");
        return -1;
    }
    if ((unsigned int)strlen(remote_file) >= TFTP_REMOTE_MAX) {
        term_add_line("tftp: remote filename is too long");
        return -1;
    }
    if (local_file && strncmp(local_file, "@net:", 5) == 0) {
        term_add_line("tftp: refusing to write a @net node");
        return -1;
    }

    net_if_t *dev = net_if_get_by_name("eth0");
    if (!dev) {
        term_add_line("tftp: no network interface available");
        return -1;
    }
    if (dev->state != NET_IF_UP && net_if_up(dev) != 0) {
        term_add_line("tftp: could not bring the interface up");
        return -1;
    }

    tftp_data_len = 0;
    tftp_expected_block = 1;
    tftp_complete = 0;
    tftp_failed = 0;
    tftp_server_port = TFTP_SERVER_PORT;
    tftp_error_msg[0] = '\0';
    tftp_local_port = tftp_next_port;
    tftp_next_port++;

    char line[160];
    unsigned int dbg_pos = 0;
    const char *dbg_prefix = "[NET] tftp get ";
    while (*dbg_prefix && dbg_pos + 1u < sizeof(line)) line[dbg_pos++] = *dbg_prefix++;
    for (const char *p = server_ip; *p && dbg_pos + 1u < sizeof(line); ) line[dbg_pos++] = *p++;
    if (dbg_pos + 1u < sizeof(line)) line[dbg_pos++] = '/';
    for (const char *p = remote_file; *p && dbg_pos + 1u < sizeof(line); ) line[dbg_pos++] = *p++;
    line[dbg_pos] = '\0';
    dbg_log(line);

    if (udp_bind(tftp_local_port, tftp_on_datagram, 0) != 0) {
        term_add_line("tftp: no free UDP port");
        return -1;
    }

    uint32_t server = net_ip_string_to_addr(server_ip);
    tftp_server_ip = server;
    uint8_t mac[ETH_ALEN];
    if (net_arp_wait(dev, net_next_hop(dev, server), mac, 2000) != 0) {
        udp_unbind(tftp_local_port);
        term_add_line("tftp: ARP resolution failed");
        return -1;
    }

    uint8_t request[TFTP_REMOTE_MAX + 16];
    unsigned int pos = 0;
    request[pos++] = 0;
    request[pos++] = 1;                                 /* RRQ */
    for (unsigned int i = 0; remote_file[i]; i++) {
        request[pos++] = (uint8_t)remote_file[i];
    }
    request[pos++] = 0;
    request[pos++] = 'o'; request[pos++] = 'c';
    request[pos++] = 't'; request[pos++] = 'e';
    request[pos++] = 't'; request[pos++] = 0;

    if (udp_output(dev, tftp_local_port, TFTP_SERVER_PORT, server,
                   request, pos) < 0) {
        udp_unbind(tftp_local_port);
        term_add_line("tftp: could not send read request");
        return -1;
    }

    /* 15 seconds total, with retries for a lost RRQ or ACK. */
    for (unsigned int i = 0; i < 1500u && !tftp_complete; i++) {
        net_poll();
        if ((i % 150u) == 149u && !tftp_complete) {
            if (tftp_data_len == 0u) {
                udp_output(dev, tftp_local_port, TFTP_SERVER_PORT, server,
                           request, pos);
            } else {
                tftp_send_ack(dev, (uint16_t)(tftp_expected_block - 1u));
            }
        }
        delay_ms(10);
    }
    net_poll();
    udp_unbind(tftp_local_port);

    if (tftp_failed) {
        term_add_line("tftp: server error");
        if (tftp_error_msg[0]) term_add_line(tftp_error_msg);
        dbg_log("[NET] tftp: server error");
        return -1;
    }
    if (!tftp_complete) {
        term_add_line("tftp: transfer timed out");
        dbg_log("[NET] tftp: transfer timed out");
        return -1;
    }

    char name[MAX_FILENAME_LEN];
    if (local_file && local_file[0]) {
        unsigned int n = 0;
        while (local_file[n] && n + 1u < sizeof(name)) {
            name[n] = local_file[n];
            n++;
        }
        name[n] = '\0';
    } else {
        tftp_local_name(remote_file, name, sizeof(name));
    }

    if (!fs_write_file_binary(name, (const char*)tftp_buffer, tftp_data_len)) {
        term_add_line("tftp: could not save file to YAFS");
        dbg_log("[NET] tftp: could not save file to YAFS");
        return -1;
    }
    dbg_log("[NET] tftp: download complete");

    char msg[100];
    unsigned int n = 0;
    const char *prefix = "tftp: saved ";
    while (prefix[n] && n + 1u < sizeof(msg)) { msg[n] = prefix[n]; n++; }
    unsigned int j = 0;
    while (name[j] && n + 1u < sizeof(msg)) { msg[n++] = name[j++]; }
    j = 0;
    const char *suffix = " (";
    while (suffix[j] && n + 1u < sizeof(msg)) { msg[n++] = suffix[j++]; }
    /* Decimal byte count, no sprintf available. */
    unsigned int value = tftp_data_len;
    char digits[10];
    unsigned int d = 0;
    do { digits[d++] = (char)('0' + (value % 10u)); value /= 10u; } while (value);
    while (d > 0u && n + 1u < sizeof(msg)) msg[n++] = digits[--d];
    if (n + 1u < sizeof(msg)) msg[n++] = ')';
    msg[n] = '\0';
    term_add_line(msg);
    return 0;
}

#endif /* NET_UDP */
#endif /* NETWORK_ENABLED */

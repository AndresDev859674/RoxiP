#include "network.h"
#include "net.h"
#include "kernel.h"

#ifdef NETWORK_ENABLED

#ifdef NET_SMC91C111

/* ----------------------------------------------------------------------
 * SMSC LAN91C111 driver for the ARM Versatile/PB onboard Ethernet.
 *
 * QEMU maps the chip at a fixed address (versatilepb.c) and it is the
 * default NIC for that machine, so no PCI enumeration is required:
 *
 *   qemu-system-arm -M versatilepb ... -netdev user,id=net0
 *
 * The register file is a 16-byte window. Offset 14 selects the register
 * bank; the remaining offsets are bank-relative. All accesses are single
 * bytes, which keeps this driver bus-independent.
 * ---------------------------------------------------------------------- */

#define SMC_BASE_ADDR   0x10010000u

/* Bank 0 */
#define SMC0_TCR        0x00    /* Transmit control (16-bit)      */
#define SMC0_RCR        0x04    /* Receive control (16-bit)       */
#define SMC0_MEMINFO    0x08    /* Memory size / free pages       */

#define RCR_SOFT_RST    0x8000u
#define RCR_STRIP_CRC   0x0200u
#define RCR_RXEN        0x0100u
#define TCR_PAD_EN      0x0080u
#define TCR_TXEN        0x0001u

/* Bank 1 */
#define SMC1_CONFIG     0x00    /* Configuration (16-bit)         */
#define SMC1_IA         0x04    /* Individual address, 6 bytes    */
#define SMC1_CONTROL    0x0C    /* Control (16-bit)               */

#define CTR_AUTO_RELEASE 0x0800u

/* Bank 2 */
#define SMC2_MMUCR      0x00    /* MMU command                    */
#define SMC2_PACKET_NUM 0x02    /* Packet number                  */
#define SMC2_ALLOC_RESULT 0x03  /* Allocation result              */
#define SMC2_TX_FIFO    0x04    /* TX completion FIFO             */
#define SMC2_RX_FIFO    0x05    /* RX FIFO                        */
#define SMC2_POINTER    0x06    /* Data pointer (16-bit)          */
#define SMC2_DATA       0x08    /* Data port, 4 bytes             */
#define SMC2_INT_ACK    0x0C    /* Interrupt acknowledge          */
#define SMC2_INT_MASK   0x0D    /* Interrupt mask                 */

#define MMU_ALLOC_TX    (1u << 5)
#define MMU_RESET_MMU   (2u << 5)
#define MMU_POP_RX      (3u << 5)
#define MMU_POP_RELEASE (4u << 5)
#define MMU_RELEASE     (5u << 5)
#define MMU_QUEUE_TX    (6u << 5)
#define MMU_RESET_TX    (7u << 5)

#define PTR_AUTOINCR    0x4000u
#define PTR_RCV         0x8000u

/* Bank 3 */
#define SMC3_REVISION   0x0A    /* Revision registers: 0x91, 0x33 */

#define SMC_BANK_SELECT 0x0E

/* QEMU user-mode networking defaults. */
#define SMC_DEFAULT_IP      "10.0.2.15"
#define SMC_DEFAULT_MASK    "255.255.255.0"
#define SMC_DEFAULT_GATEWAY "10.0.2.2"

static net_device_t smc_dev;
static volatile uint8_t *smc_regs = (volatile uint8_t *)SMC_BASE_ADDR;
static uint8_t smc_mac[ETH_ALEN];
static int smc_initialized = 0;
static int smc_present = 0;

/* ----------------------------------------------------------------------
 * Low-level register access
 * ---------------------------------------------------------------------- */

static inline void smc_select_bank(uint8_t bank) {
    smc_regs[SMC_BANK_SELECT] = bank;
}

static inline void smc_write8(uint8_t reg, uint8_t value) {
    smc_regs[reg] = value;
}

static inline uint8_t smc_read8(uint8_t reg) {
    return smc_regs[reg];
}

static void smc_write16(uint8_t reg, uint16_t value) {
    smc_regs[reg]      = (uint8_t)(value & 0xFF);
    smc_regs[reg + 1]  = (uint8_t)(value >> 8);
}

static uint16_t smc_read16(uint8_t reg) {
    return (uint16_t)((uint16_t)smc_regs[reg] |
                      ((uint16_t)smc_regs[reg + 1] << 8));
}

static void smc_wait(void) {
    /* A handful of bus cycles is enough for the emulated chip. */
    for (volatile unsigned int i = 0; i < 64; i++) { }
}

/* ----------------------------------------------------------------------
 * Hardware operations
 * ---------------------------------------------------------------------- */

static int smc_hw_init(void) {
    if (smc_initialized) return 0;

    /* Detect the chip before touching anything else. */
    smc_select_bank(3);
    uint8_t rev0 = smc_read8(SMC3_REVISION);
    uint8_t rev1 = smc_read8(SMC3_REVISION + 1);
    if (rev0 != 0x91 || rev1 != 0x33) {
        smc_present = 0;
        return -1;
    }
    smc_present = 1;

    /* Soft reset. */
    smc_select_bank(0);
    smc_write16(SMC0_RCR, RCR_SOFT_RST);
    smc_wait();

    /* Reset the MMU and release any stale pages. */
    smc_select_bank(2);
    smc_write8(SMC2_MMUCR, (uint8_t)MMU_RESET_MMU);
    smc_wait();

    /* Read the MAC address programmed into the chip. QEMU fills this from
     * the netdev's mac= property (or a generated QEMU OUI), so the same
     * value can be used for ARP and for the derived @net:id. */
    smc_select_bank(1);
    for (unsigned int i = 0; i < ETH_ALEN; i++) {
        smc_mac[i] = smc_read8(SMC1_IA + i);
    }

    /* Auto-release transmitted pages so the four on-chip buffers cannot
     * be exhausted by a burst of ARP / UDP packets. */
    smc_write16(SMC1_CONTROL, (uint16_t)(smc_read16(SMC1_CONTROL) |
                                         CTR_AUTO_RELEASE));

    /* Mask all interrupts; this driver polls the status registers. */
    smc_select_bank(2);
    smc_write8(SMC2_INT_MASK, 0x00);
    smc_write8(SMC2_INT_ACK, 0xFF);

    smc_initialized = 1;
    return 0;
}

static int smc_start(void) {
    if (!smc_initialized) return -1;

    /* Enable receiver (and strip the FCS), then enable transmitter with
     * short-frame padding. */
    smc_select_bank(0);
    smc_write16(SMC0_RCR, RCR_RXEN | RCR_STRIP_CRC);
    smc_write16(SMC0_TCR, TCR_TXEN | TCR_PAD_EN);
    return 0;
}

static int smc_stop(void) {
    if (!smc_initialized) return -1;
    smc_select_bank(0);
    smc_write16(SMC0_RCR, 0);
    smc_write16(SMC0_TCR, 0);
    return 0;
}

static int smc_send(const uint8_t *buf, uint32_t len) {
    if (!smc_initialized || !buf || len == 0 || len > ETH_FRAME_LEN) return -1;

    /* The on-chip data frame length includes the 4-byte header and a
     * 2-byte trailer. Keep frames even so the simple control byte works. */
    unsigned int frame_len = len;
    uint8_t last_byte = buf[frame_len - 1];
    if (frame_len & 1u) frame_len++;

    smc_select_bank(2);
    smc_write8(SMC2_MMUCR, (uint8_t)MMU_ALLOC_TX);

    unsigned int timeout = 200000;
    uint8_t packet = smc_read8(SMC2_ALLOC_RESULT);
    while ((packet & 0x80u) && timeout--) {
        packet = smc_read8(SMC2_ALLOC_RESULT);
    }
    if (packet & 0x80u) return -1;

    smc_write8(SMC2_PACKET_NUM, packet);
    smc_write8(SMC2_POINTER, 0x00);
    smc_write8(SMC2_POINTER + 1, (uint8_t)(PTR_AUTOINCR >> 8));

    /* Status word (rewritten by the chip), then byte count. */
    smc_write8(SMC2_DATA, 0x00);
    smc_write8(SMC2_DATA, 0x00);
    uint16_t byte_count = (uint16_t)(frame_len + 6u);
    smc_write8(SMC2_DATA, (uint8_t)(byte_count & 0xFF));
    smc_write8(SMC2_DATA, (uint8_t)(byte_count >> 8));

    for (unsigned int i = 0; i < len; i++) {
        smc_write8(SMC2_DATA, buf[i]);
    }
    if (frame_len != len) {
        smc_write8(SMC2_DATA, last_byte);
    }

    /* Last data byte placeholder + control byte (0x40 = even frame). */
    smc_write8(SMC2_DATA, 0x00);
    smc_write8(SMC2_DATA, 0x40);

    smc_write8(SMC2_MMUCR, (uint8_t)MMU_QUEUE_TX);
    return (int)len;
}

static int smc_receive(uint8_t *buf, uint32_t max_len, uint32_t *actual_len) {
    if (!smc_initialized || !buf || !actual_len) return -1;

    smc_select_bank(2);
    uint8_t packet = smc_read8(SMC2_RX_FIFO);
    if (packet & 0x80u) return 1;   /* No packet available */

    smc_write8(SMC2_PACKET_NUM, packet);
    smc_write8(SMC2_POINTER, 0x00);
    smc_write8(SMC2_POINTER + 1, (uint8_t)((PTR_AUTOINCR | PTR_RCV) >> 8));

    /* Status word and byte count. */
    (void)smc_read8(SMC2_DATA);
    (void)smc_read8(SMC2_DATA);
    uint16_t byte_count = (uint16_t)((uint16_t)smc_read8(SMC2_DATA) |
                        ((uint16_t)smc_read8(SMC2_DATA) << 8));
    uint32_t payload = (byte_count >= 6u) ? (uint32_t)(byte_count - 6u) : 0u;
    if (payload > max_len) payload = max_len;

    for (uint32_t i = 0; i < payload; i++) {
        buf[i] = smc_read8(SMC2_DATA);
    }
    *actual_len = payload;

    /* Pop the RX FIFO and hand the on-chip page back. */
    smc_write8(SMC2_MMUCR, (uint8_t)MMU_POP_RELEASE);
    return 0;
}

static void smc_set_mac(const uint8_t *mac) {
    if (!mac) return;
    memcpy(smc_mac, mac, ETH_ALEN);
    smc_select_bank(1);
    for (unsigned int i = 0; i < ETH_ALEN; i++) {
        smc_write8(SMC1_IA + i, mac[i]);
    }
}

static void smc_get_mac(uint8_t *mac) {
    if (mac) memcpy(mac, smc_mac, ETH_ALEN);
}

/* ----------------------------------------------------------------------
 * Driver registration
 * ---------------------------------------------------------------------- */

static net_driver_ops_t smc_ops = {
    .init = smc_hw_init,
    .start = smc_start,
    .stop = smc_stop,
    .send = smc_send,
    .receive = smc_receive,
    .set_mac = smc_set_mac,
    .get_mac = smc_get_mac,
};

int smc91c111_init_driver(void) {
    memset(&smc_dev, 0, sizeof(smc_dev));
    memcpy(smc_dev.name, "eth0", 5);
    smc_dev.ops = &smc_ops;
    smc_dev.state = 0;

    if (smc_hw_init() != 0) {
        log_boot_event("[NET] No SMC91C111 Ethernet at 0x10010000");
        dbg_log("[NET] No SMC91C111 Ethernet at 0x10010000");
        return -1;
    }

    memcpy(smc_dev.mac, smc_mac, ETH_ALEN);
    smc_dev.ip = net_ip_string_to_addr(SMC_DEFAULT_IP);
    smc_dev.netmask = net_ip_string_to_addr(SMC_DEFAULT_MASK);
    smc_dev.gateway = net_ip_string_to_addr(SMC_DEFAULT_GATEWAY);

    net_device_register(&smc_dev);
    log_boot_event("[NET] SMC91C111 Ethernet initialized");
    dbg_log("[NET] SMC91C111 detected at 0x10010000");
    return 0;
}

void smc91c111_poll(void) {
    if (!smc_initialized || !smc_present) return;

    uint8_t frame[ETH_FRAME_LEN];
    uint32_t len = 0;
    net_if_t *iface = net_if_get_by_name("eth0");
    if (!iface) return;

    unsigned int budget = 8;
    while (budget-- && smc_receive(frame, sizeof(frame), &len) == 0 && len > 0) {
        net_rx(iface, frame, len);
    }
}

#endif /* NET_SMC91C111 */
#endif /* NETWORK_ENABLED */

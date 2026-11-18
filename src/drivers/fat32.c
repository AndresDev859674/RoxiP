#include "kernel.h"
#include "drivers.h"
#include "firmware.h"

/* ----------------------------------------------------------------------
 * FAT32 Read-Only Driver
 *
 * Reads files from a FAT32-formatted storage image or block device.
 * No write support — read-only for safety and simplicity.
 * ---------------------------------------------------------------------- */

/* FAT32 BPB (BIOS Parameter Block) fields, little-endian */
typedef struct {
    unsigned char  jmp_boot[3];
    unsigned char  oem_name[8];
    unsigned short bytes_per_sector;
    unsigned char  sectors_per_cluster;
    unsigned short reserved_sector_count;
    unsigned char  num_fats;
    unsigned short root_entry_count;      /* FAT12/16 only; 0 for FAT32 */
    unsigned short total_sectors_16;      /* FAT12/16 only; 0 for FAT32 */
    unsigned char  media;
    unsigned short fat_size_16;           /* FAT12/16 only; 0 for FAT32 */
    unsigned short sectors_per_track;
    unsigned short num_heads;
    unsigned int   hidden_sectors;
    unsigned int   total_sectors_32;
    unsigned int   fat_size_32;
    unsigned short ext_flags;
    unsigned short fs_version;
    unsigned int   root_cluster;
    unsigned int   fsinfo_sector;
    unsigned int   backup_boot_sector;
    unsigned char  reserved_0[12];
    unsigned char  drive_number;
    unsigned char  reserved_1;
    unsigned char  boot_signature;
    unsigned int   volume_id;
    unsigned char  volume_label[11];
    unsigned char  fat_type_label[8];
} __attribute__((packed)) fat32_bpb_t;

/* FAT32 directory entry */
typedef struct {
    unsigned char name[11];
    unsigned char attr;
    unsigned char nt_reserved;
    unsigned char create_time_tenths;
    unsigned short create_time;
    unsigned short create_date;
    unsigned short last_access_date;
    unsigned short first_cluster_hi;
    unsigned short write_time;
    unsigned short write_date;
    unsigned short first_cluster_lo;
    unsigned int   file_size;
} __attribute__((packed)) fat32_dirent_t;

#define FAT_ATTR_READ_ONLY  0x01
#define FAT_ATTR_HIDDEN     0x02
#define FAT_ATTR_SYSTEM     0x04
#define FAT_ATTR_VOLUME_ID  0x08
#define FAT_ATTR_DIRECTORY  0x10
#define FAT_ATTR_ARCHIVE    0x20
#define FAT_ATTR_LONG_NAME  0x0F

/* FAT32 context */
static fat32_bpb_t g_bpb;
static unsigned int g_fat_sector;
static unsigned int g_data_sector;
static unsigned int g_root_dir_sector;
static unsigned int g_cluster_size;
static int g_fat32_initialized = 0;

/* Simple sector cache — reads one sector at a time from storage base */
static unsigned char sector_cache[512];

/* Storage base address for the storage file image */
#define STORAGE_IMAGE_BASE STORAGE_FLASH_BASE

static int read_sector(unsigned int sector, unsigned char* buf) {
    volatile unsigned char* src = (volatile unsigned char*)((unsigned long)STORAGE_IMAGE_BASE + (unsigned long)sector * 512);
    for (int i = 0; i < 512; i++) buf[i] = src[i];
    return 0;
}

static int read_cluster(unsigned int cluster, unsigned char* buf) {
    unsigned int sector = g_data_sector + (cluster - 2) * g_bpb.sectors_per_cluster;
    for (unsigned int i = 0; i < g_bpb.sectors_per_cluster; i++) {
        read_sector(sector + i, buf + i * g_bpb.bytes_per_sector);
    }
    return 0;
}

static unsigned int read_fat_entry(unsigned int cluster) {
    unsigned int fat_offset = cluster * 4;
    unsigned int fat_sector = g_fat_sector + (fat_offset / g_bpb.bytes_per_sector);
    unsigned int fat_index  = fat_offset % g_bpb.bytes_per_sector;

    read_sector(fat_sector, sector_cache);
    unsigned int val = *(unsigned int*)(sector_cache + fat_index);
    return val & 0x0FFFFFFF;  /* Mask to 28 bits (FAT32) */
}

static void parse_8dot3(const unsigned char* name11, char* out, int out_len) {
    int i = 0;
    for (int j = 0; j < 8 && name11[j] != ' ' && i < out_len - 1; j++) {
        out[i++] = name11[j];
    }
    if (name11[8] != ' ' && i < out_len - 1) {
        out[i++] = '.';
        for (int j = 8; j < 11 && name11[j] != ' ' && i < out_len - 1; j++) {
            out[i++] = name11[j];
        }
    }
    out[i] = '\0';
}

int fat32_init(void) {
    read_sector(0, sector_cache);
    fat32_bpb_t* bpb = (fat32_bpb_t*)sector_cache;

    if (bpb->jmp_boot[0] != 0xEB && bpb->jmp_boot[0] != 0xE9) {
        log_boot_event("[FAT32] Not a valid FAT32 boot sector");
        return -1;
    }

    g_bpb = *bpb;

    g_fat_sector     = g_bpb.reserved_sector_count;
    g_root_dir_sector = g_bpb.reserved_sector_count + (g_bpb.num_fats * g_bpb.fat_size_32);
    g_data_sector     = g_root_dir_sector + ((g_bpb.root_entry_count * 32 + g_bpb.bytes_per_sector - 1) / g_bpb.bytes_per_sector);
    g_cluster_size    = g_bpb.bytes_per_sector * g_bpb.sectors_per_cluster;

    g_fat32_initialized = 1;
    log_boot_event("[OK] FAT32 read-only driver initialized");
    return 0;
}

/* Find a directory entry by name within a directory cluster chain.
 * dir_cluster: starting cluster (0 = root directory uses g_bpb.root_cluster)
 * name: 8.3 short name to match (case-insensitive)
 * Returns cluster of found entry, or 0 if not found.
 */
static unsigned int fat32_find_entry(unsigned int dir_cluster, const char* name) {
    unsigned char dir_buf[4096];  /* Enough for a few clusters */
    unsigned int cluster = dir_cluster ? dir_cluster : g_bpb.root_cluster;

    while (cluster >= 2 && cluster < 0x0FFFFFF8) {
        read_cluster(cluster, dir_buf);

        int entries_per_cluster = g_cluster_size / 32;
        for (int i = 0; i < entries_per_cluster; i++) {
            fat32_dirent_t* de = (fat32_dirent_t*)(dir_buf + i * 32);

            if (de->name[0] == 0x00) return 0;  /* End of directory */
            if (de->name[0] == 0xE5) continue;  /* Deleted entry */
            if (de->attr == FAT_ATTR_LONG_NAME) continue;  /* Skip LFN */

            char short_name[16];
            parse_8dot3(de->name, short_name, sizeof(short_name));

            /* Case-insensitive compare */
            int match = 1;
            for (int k = 0; short_name[k] && name[k]; k++) {
                char a = short_name[k] | 0x20;
                char b = name[k] | 0x20;
                if (a != b) { match = 0; break; }
            }
            if (match && short_name[strlen(short_name)] == '\0' && name[strlen(name)] == '\0') {
                unsigned int cluster_lo = de->first_cluster_lo;
                unsigned int cluster_hi = de->first_cluster_hi;
                return (cluster_hi << 16) | cluster_lo;
            }
        }

        cluster = read_fat_entry(cluster);
    }

    return 0;
}

int fat32_read_file(const char* fname, unsigned char* out_buf, unsigned int max_size) {
    if (!g_fat32_initialized) return -1;

    /* Parse path: support /dir/file.txt or just filename */
    char name[16];
    int len = strlen(fname);
    int start = 0;
    while (start < len && fname[start] == '/') start++;

    /* Find last component */
    int last_slash = start;
    for (int i = start; i < len; i++) {
        if (fname[i] == '/') last_slash = i + 1;
    }

    /* Build 8.3 name from last component */
    const char* comp = fname + last_slash;
    int comp_len = strlen(comp);
    if (comp_len > 11) comp_len = 11;

    /* Simple conversion: uppercase, strip dots beyond first */
    int ni = 0, dot_seen = 0;
    for (int i = 0; i < comp_len && ni < 11; i++) {
        char c = comp[i];
        if (c == '.') {
            if (dot_seen) continue;
            dot_seen = 1;
            /* Pad name with spaces to 8 chars */
            while (ni < 8) name[ni++] = ' ';
            continue;
        }
        if (c >= 'a' && c <= 'z') c = c - 'a' + 'A';
        name[ni++] = c;
    }
    while (ni < 11) name[ni++] = ' ';
    name[11] = '\0';

    unsigned int cluster = fat32_find_entry(g_bpb.root_cluster, name);
    if (cluster == 0) return -1;  /* Not found */

    /* Read file data following FAT chain */
    unsigned int total_read = 0;
    while (cluster >= 2 && cluster < 0x0FFFFFF8 && total_read < max_size) {
        unsigned char cluster_buf[4096];
        if (g_cluster_size > sizeof(cluster_buf)) break;
        read_cluster(cluster, cluster_buf);

        unsigned int to_copy = g_cluster_size;
        if (total_read + to_copy > max_size) to_copy = max_size - total_read;

        memcpy(out_buf + total_read, cluster_buf, to_copy);
        total_read += to_copy;

        cluster = read_fat_entry(cluster);
    }

    return (int)total_read;
}

/* List files in root directory */
int fat32_list_dir(void) {
    if (!g_fat32_initialized) return -1;

    term_add_line("FAT32 Root Directory:");
    term_add_line("-------------------");

    unsigned char dir_buf[4096];
    unsigned int cluster = g_bpb.root_cluster;
    int count = 0;

    while (cluster >= 2 && cluster < 0x0FFFFFF8 && count < 50) {
        read_cluster(cluster, dir_buf);

        int entries_per_cluster = g_cluster_size / 32;
        for (int i = 0; i < entries_per_cluster; i++) {
            fat32_dirent_t* de = (fat32_dirent_t*)(dir_buf + i * 32);

            if (de->name[0] == 0x00) return count;
            if (de->name[0] == 0xE5) continue;
            if (de->attr == FAT_ATTR_LONG_NAME) continue;

            char short_name[16];
            parse_8dot3(de->name, short_name, sizeof(short_name));

            char attr_str[8] = "";
            if (de->attr & FAT_ATTR_DIRECTORY) strcat(attr_str, "D");
            if (de->attr & FAT_ATTR_READ_ONLY) strcat(attr_str, "R");
            if (de->attr & FAT_ATTR_HIDDEN) strcat(attr_str, "H");
            if (de->attr & FAT_ATTR_SYSTEM) strcat(attr_str, "S");
            if (de->attr & FAT_ATTR_ARCHIVE) strcat(attr_str, "A");

            unsigned int size = de->file_size;
            char size_str[16];
            int_to_str(size, size_str);

            char line[80];
            strcpy(line, "  ");
            strcat(line, short_name);
            /* Pad to 16 chars */
            int slen = strlen(line);
            while (slen < 16) { line[slen++] = ' '; }
            line[slen] = '\0';
            strcat(line, attr_str);
            while (strlen(line) < 24) strcat(line, " ");
            strcat(line, size_str);
            term_add_line(line);
            count++;
        }

        cluster = read_fat_entry(cluster);
    }

    return count;
}

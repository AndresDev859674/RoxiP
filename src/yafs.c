#include "kernel.h"

file_record_t fs_table[MAX_YAFS_FILES];
int fs_file_count = 0;
static char synthetic_buf[2048];

typedef struct {
    char name[100];
    char mode[8];
    char uid[8];
    char gid[8];
    char size[12];
    char mtime[12];
    char chksum[8];
    char typeflag;
    char linkname[100];
    char magic[6];
    char version[2];
    char uname[32];
    char gname[32];
    char devmajor[8];
    char devminor[8];
    char prefix[155];
    char pad[12];
} tar_header_t;

static unsigned int parse_octal(const char* str, int max_len) {
    unsigned int val = 0;
    for (int i = 0; i < max_len && str[i] >= '0' && str[i] <= '7'; i++) {
        val = (val << 3) + (str[i] - '0');
    }
    return val;
}

static int unpack_tar_stream(const unsigned char* tar_bytes, unsigned int total_len) {
    unsigned int offset = 0;
    int files_extracted = 0;

    while (offset + 512 <= total_len) {
        const tar_header_t* header = (const tar_header_t*)(tar_bytes + offset);
        if (header->name[0] == '\0') break;

        unsigned int file_size = parse_octal(header->size, 12);
        offset += 512;

        if (header->typeflag == '0' || header->typeflag == '\0') {
            if (file_size < MAX_FILE_CONTENT) {
                fs_write_file_binary(header->name, (const char*)(tar_bytes + offset), file_size);
                files_extracted++;
            }
        } else if (header->typeflag == '5') {
            fs_mkdir(header->name);
        }

        offset += ((file_size + 511) & ~511);
    }

    return files_extracted;
}

static int inflate_deflate_stream(const unsigned char* src, unsigned int src_len, unsigned char* dst, unsigned int dst_len) {
    unsigned int src_pos = 0;
    unsigned int dst_pos = 0;

    while (src_pos < src_len && dst_pos < dst_len) {
        dst[dst_pos++] = src[src_pos++];
    }
    return (int)dst_pos;
}

int unpack_targz_in_ram(const unsigned char* src, unsigned int src_len) {
    if (src_len < 18) {
        term_add_line("tar ERR [E_TRUNCATED]: Payload size under GZIP minimum (18B).");
        return -1;
    }

    if (src[0] != 0x1F || src[1] != 0x8B) {
        char err_msg[80] = "tar ERR [E_BAD_MAGIC]: Header 0x";
        char h1[12], h2[12];
        hex_to_str(src[0], h1); hex_to_str(src[1], h2);
        strcpy(err_msg + strlen(err_msg), h1 + 6);
        strcpy(err_msg + strlen(err_msg), h2 + 6);
        strcpy(err_msg + strlen(err_msg), " != expected 0x1F8B");
        term_add_line(err_msg);
        return -2;
    }

    unsigned char flags = src[3];
    unsigned int gz_offset = 10;

    if (flags & 4) {
        unsigned int extra_len = src[gz_offset] | (src[gz_offset + 1] << 8);
        gz_offset += 2 + extra_len;
    }
    if (flags & 8) {
        while (gz_offset < src_len && src[gz_offset] != 0) gz_offset++;
        gz_offset++;
    }
    if (flags & 16) {
        while (gz_offset < src_len && src[gz_offset] != 0) gz_offset++;
        gz_offset++;
    }
    if (flags & 2) {
        gz_offset += 2;
    }

    if (gz_offset >= src_len) {
        term_add_line("tar ERR [E_CORRUPT_HEADER]: Dynamic header size exceeds file bounds.");
        return -3;
    }

    unsigned int uncompressed_len = src[src_len - 4] | (src[src_len - 3] << 8) | 
                                    (src[src_len - 2] << 16) | (src[src_len - 1] << 24);

    if (uncompressed_len == 0 || uncompressed_len > (1024 * 1024)) {
        uncompressed_len = 256 * 1024;
    }

    unsigned char* uncompressed_tar = (unsigned char*)malloc(uncompressed_len);
    if (!uncompressed_tar) {
        term_add_line("tar ERR [E_OOM]: Failed to allocate RAM extraction buffer.");
        return -4;
    }

    unsigned int payload_size = src_len - gz_offset - 8;
    inflate_deflate_stream(src + gz_offset, payload_size, uncompressed_tar, uncompressed_len);

    int count = unpack_tar_stream(uncompressed_tar, uncompressed_len);
    free(uncompressed_tar);
    return count;
}

void execute_tar(const char* args) {
    if (args[0] == '\0') {
        term_add_line("Usage: tar -xvf <archive.tar|archive.tar.gz>");
        return;
    }

    const char* filename = args;
    if (strncmp(args, "-xvf ", 5) == 0) filename = args + 5;
    else if (strncmp(args, "xvf ", 4) == 0) filename = args + 4;

    file_record_t* rec = 0;
    for (int i = 0; i < fs_file_count; i++) {
        if (fs_table[i].used && strcmp(fs_table[i].name, filename) == 0) {
            rec = &fs_table[i];
            break;
        }
    }

    if (!rec) {
        term_add_line("tar ERR [E_NOENT]: Target file not found in YAFS table.");
        return;
    }

    unsigned int data_len = rec->size;
    const unsigned char* bytes = (const unsigned char*)rec->content;

    char log_info[80] = "tar: Reading ";
    char sz_str[16];
    int_to_str(data_len, sz_str);
    strcpy(log_info + strlen(log_info), sz_str);
    strcpy(log_info + strlen(log_info), " Bytes from RAM payload...");
    term_add_line(log_info);

    if (data_len >= 2 && bytes[0] == 0x1F && bytes[1] == 0x8B) {
        term_add_line("tar: GZIP compression signature matches (0x1F8B)");
        int extracted = unpack_targz_in_ram(bytes, data_len);
        if (extracted >= 0) {
            char msg[64] = "tar: Extracted files count: ";
            char num[16];
            int_to_str(extracted, num);
            strcpy(msg + strlen(msg), num);
            term_add_line(msg);
        }
    } else {
        term_add_line("tar: Standard POSIX TAR stream detected");
        int extracted = unpack_tar_stream(bytes, data_len);
        char msg[64] = "tar: Extracted files count: ";
        char num[16];
        int_to_str(extracted, num);
        strcpy(msg + strlen(msg), num);
        term_add_line(msg);
    }
}

static void pflash_write_byte(unsigned int offset, unsigned char val) {
    volatile unsigned char* flash = (volatile unsigned char*)STORAGE_FLASH_BASE;
    flash[offset] = 0x40;
    flash[offset] = val;
    flash[offset] = 0xFF;
}

void vfs_sync_to_storage(void) {
    volatile unsigned char* flash = (volatile unsigned char*)STORAGE_FLASH_BASE;

    unsigned int magic = STORAGE_MAGIC;
    unsigned char* magic_ptr = (unsigned char*)&magic;
    for (unsigned int i = 0; i < sizeof(unsigned int); i++) {
        if (flash[i] != magic_ptr[i]) pflash_write_byte(i, magic_ptr[i]);
    }

    unsigned char* count_ptr = (unsigned char*)&fs_file_count;
    unsigned int count_offset = sizeof(unsigned int);
    for (unsigned int i = 0; i < sizeof(int); i++) {
        if (flash[count_offset + i] != count_ptr[i]) pflash_write_byte(count_offset + i, count_ptr[i]);
    }

    unsigned char* fs_ptr = (unsigned char*)fs_table;
    unsigned int table_offset = sizeof(unsigned int) + sizeof(int);
    unsigned int table_size = sizeof(file_record_t) * MAX_YAFS_FILES;

    for (unsigned int i = 0; i < table_size; i++) {
        if (flash[table_offset + i] != fs_ptr[i]) pflash_write_byte(table_offset + i, fs_ptr[i]);
    }
}

int vfs_load_from_storage(void) {
    volatile storage_header_t* flash = (volatile storage_header_t*)STORAGE_FLASH_BASE;
    if (flash->magic != STORAGE_MAGIC) return 0;

    fs_file_count = flash->file_count;
    for (int i = 0; i < MAX_YAFS_FILES; i++) fs_table[i] = flash->files[i];
    return 1;
}

void init_filesystems(void) {
    if (vfs_load_from_storage()) {
        log_boot_event("[OK] Persistent YAFS mounted from roxip.storage");
        return;
    }

    fs_file_count = 0;

    strcpy(fs_table[0].name, "motd.txt");
    strcpy(fs_table[0].content, "Welcome to RoxiP OS! comcow Compositor Engine Active.");
    fs_table[0].size = strlen(fs_table[0].content);
    fs_table[0].used = 1; fs_table[0].is_vfat = 0; fs_table[0].is_dir = 0;
    fs_file_count++;

    strcpy(fs_table[1].name, "LongFileName_VFAT.sys");
    strcpy(fs_table[1].content, "vfat_enabled=true\nfat32_clusters=65536\nlfn_support=1");
    fs_table[1].size = strlen(fs_table[1].content);
    fs_table[1].used = 1; fs_table[1].is_vfat = 1; fs_table[1].is_dir = 0;
    fs_file_count++;

    strcpy(fs_table[2].name, "history.txt");
    strcpy(fs_table[2].content, "");
    fs_table[2].size = 0;
    fs_table[2].used = 1; fs_table[2].is_vfat = 0; fs_table[2].is_dir = 0;
    fs_file_count++;

    log_boot_event("[OK] Persistent Flash formatted (YAFS1)");
    vfs_sync_to_storage();
}

const char* fs_read_file(const char* filename) {
    if (filename[0] == '@') {
        synthetic_buf[0] = '\0';
        if (strcmp(filename, "@cpu") == 0) {
            strcpy(synthetic_buf, "CPU: ");
            strcpy(synthetic_buf + strlen(synthetic_buf), cpu_model_string);
            strcpy(synthetic_buf + strlen(synthetic_buf), "\nArch: ARMv5TEJ (ARM926EJ-S)\nCore ID: 0x41069265");
            return synthetic_buf;
        } else if (strcmp(filename, "@ram") == 0) {
            char used[16], total[16];
            int_to_str(get_used_heap_bytes(), used);
            int_to_str(HEAP_SIZE, total);
            strcpy(synthetic_buf, "RAM Heap Used : ");
            strcpy(synthetic_buf + strlen(synthetic_buf), used);
            strcpy(synthetic_buf + strlen(synthetic_buf), " B\nRAM Heap Total: ");
            strcpy(synthetic_buf + strlen(synthetic_buf), total);
            strcpy(synthetic_buf + strlen(synthetic_buf), " B");
            return synthetic_buf;
        } else if (strcmp(filename, "@gpu") == 0) {
            strcpy(synthetic_buf, "Engine: comcow Compositor\nMode: ");
            strcpy(synthetic_buf + strlen(synthetic_buf), comcow.mode_name);
            strcpy(synthetic_buf + strlen(synthetic_buf), "\nBuffer: Double Buffered PL110");
            return synthetic_buf;
        } else if (strcmp(filename, "@fb0") == 0) {
            char hex[12];
            hex_to_str((unsigned int)fb_pixels, hex);
            strcpy(synthetic_buf, "FB Base: ");
            strcpy(synthetic_buf + strlen(synthetic_buf), hex);
            strcpy(synthetic_buf + strlen(synthetic_buf), "\nRes: 320x200 / 80x25\nSize: 614400 Bytes");
            return synthetic_buf;
        } else if (strcmp(filename, "@yafs") == 0 || strcmp(filename, "@vfs") == 0) {
            char num[16];
            int_to_str(fs_file_count, num);
            strcpy(synthetic_buf, "Type: RoxiP Synthetic/PFlash YAFS (Yet Another File System)\nFile Count: ");
            strcpy(synthetic_buf + strlen(synthetic_buf), num);
            strcpy(synthetic_buf + strlen(synthetic_buf), "\nMax Payload: 2 MB");
            return synthetic_buf;
        } else if (strcmp(filename, "@pipe") == 0) {
            return pipe_buffer;
        }
    }

    for (int i = 0; i < fs_file_count; i++) {
        if (fs_table[i].used && strcmp(fs_table[i].name, filename) == 0) {
            if (fs_table[i].is_dir) return 0;
            return fs_table[i].content;
        }
    }
    return 0;
}

int fs_write_file_binary(const char* filename, const char* content, unsigned int size) {
    if (size >= MAX_FILE_CONTENT) {
        term_add_line("YAFS ERR [E_OVERFLOW]: Size exceeds 2MB limit.");
        size = MAX_FILE_CONTENT - 1;
    }

    for (int i = 0; i < fs_file_count; i++) {
        if (fs_table[i].used && strcmp(fs_table[i].name, filename) == 0) {
            if (fs_table[i].is_dir) {
                term_add_line("YAFS ERR [E_ISDIR]: Target path is a directory.");
                return 0;
            }
            memcpy(fs_table[i].content, content, size);
            fs_table[i].content[size] = '\0';
            fs_table[i].size = size;
            vfs_sync_to_storage();
            return 1;
        }
    }

    if (fs_file_count < MAX_YAFS_FILES) {
        int idx = fs_file_count;
        strcpy(fs_table[idx].name, filename);
        memcpy(fs_table[idx].content, content, size);
        fs_table[idx].content[size] = '\0';
        fs_table[idx].size = size;
        fs_table[idx].used = 1;
        fs_table[idx].is_vfat = (strlen(filename) > 11) ? 1 : 0;
        fs_table[idx].is_dir = 0;
        fs_file_count++;
        vfs_sync_to_storage();
        return 1;
    }

    term_add_line("YAFS ERR [E_NOSPACE]: File table capacity reached (16 max).");
    return 0;
}

int fs_write_file(const char* filename, const char* content) {
    return fs_write_file_binary(filename, content, strlen(content));
}

int fs_mkdir(const char* dirname) {
    for (int i = 0; i < fs_file_count; i++) {
        if (fs_table[i].used && strcmp(fs_table[i].name, dirname) == 0) {
            term_add_line("YAFS ERR [E_EXIST]: Directory or file already exists.");
            return 0;
        }
    }
    if (fs_file_count < MAX_YAFS_FILES) {
        int idx = fs_file_count;
        strcpy(fs_table[idx].name, dirname);
        fs_table[idx].content[0] = '\0';
        fs_table[idx].size = 0;
        fs_table[idx].used = 1;
        fs_table[idx].is_vfat = 0;
        fs_table[idx].is_dir = 1;
        fs_file_count++;
        vfs_sync_to_storage();
        return 1;
    }
    term_add_line("YAFS ERR [E_NOSPACE]: File table capacity reached (16 max).");
    return 0;
}
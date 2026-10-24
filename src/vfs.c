#include "kernel.h"

file_record_t fs_table[MAX_VFS_FILES];
int fs_file_count = 0;

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
    unsigned int table_size = sizeof(file_record_t) * MAX_VFS_FILES;

    for (unsigned int i = 0; i < table_size; i++) {
        if (flash[table_offset + i] != fs_ptr[i]) pflash_write_byte(table_offset + i, fs_ptr[i]);
    }
}

int vfs_load_from_storage(void) {
    volatile storage_header_t* flash = (volatile storage_header_t*)STORAGE_FLASH_BASE;
    if (flash->magic != STORAGE_MAGIC) return 0;

    fs_file_count = flash->file_count;
    for (int i = 0; i < MAX_VFS_FILES; i++) fs_table[i] = flash->files[i];
    return 1;
}

void init_filesystems(void) {
    if (vfs_load_from_storage()) {
        log_boot_event("[OK] Persistent VFS mounted from roxip.storage");
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

    log_boot_event("[OK] Persistent Flash formatted (ROXIPFS1)");
    vfs_sync_to_storage();
}

/* Zero-copy Read API: returns direct pointer to memory */
const char* fs_read_file(const char* filename) {
    for (int i = 0; i < fs_file_count; i++) {
        if (fs_table[i].used && strcmp(fs_table[i].name, filename) == 0) {
            if (fs_table[i].is_dir) return 0;
            return fs_table[i].content;
        }
    }
    return 0;
}

/* Fast Write API using memcpy up to 2 MB */
int fs_write_file(const char* filename, const char* content) {
    int len = strlen(content);
    if (len >= MAX_FILE_CONTENT) len = MAX_FILE_CONTENT - 1;

    for (int i = 0; i < fs_file_count; i++) {
        if (fs_table[i].used && strcmp(fs_table[i].name, filename) == 0) {
            if (fs_table[i].is_dir) return 0;
            memcpy(fs_table[i].content, content, len);
            fs_table[i].content[len] = '\0';
            fs_table[i].size = len;
            vfs_sync_to_storage();
            return 1;
        }
    }

    if (fs_file_count < MAX_VFS_FILES) {
        int idx = fs_file_count;
        strcpy(fs_table[idx].name, filename);
        memcpy(fs_table[idx].content, content, len);
        fs_table[idx].content[len] = '\0';
        fs_table[idx].size = len;
        fs_table[idx].used = 1;
        fs_table[idx].is_vfat = (strlen(filename) > 11) ? 1 : 0;
        fs_table[idx].is_dir = 0;
        fs_file_count++;
        vfs_sync_to_storage();
        return 1;
    }
    return 0;
}

int fs_mkdir(const char* dirname) {
    for (int i = 0; i < fs_file_count; i++) {
        if (fs_table[i].used && strcmp(fs_table[i].name, dirname) == 0) return 0;
    }
    if (fs_file_count < MAX_VFS_FILES) {
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
    return 0;
}
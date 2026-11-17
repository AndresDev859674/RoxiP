# RoxiP YAFS — Yet Another File System

RoxiP does **not** use partitions. Instead, all data lives in a single storage file (`roxip.storage`, typically 64 MB), which is copied to `roxip.img` for QEMU.

## Format

YAFS is a simple container format stored at the beginning of the storage file:

```c
typedef struct {
    unsigned int magic;       // STORAGE_MAGIC = 0x53465053 ("SPFS")
    int file_count;
    file_record_t files[MAX_YAFS_FILES];  // max 16 files
} storage_header_t;

typedef struct {
    char name[MAX_FILENAME_LEN];   // 64 bytes
    char content[MAX_FILE_CONTENT]; // 2 MB max per file
    int size;
    int used;
    int is_vfat;
    int is_dir;
} file_record_t;
```

## API (`src/yafs.c`)

| Function | Description |
|----------|-------------|
| `init_filesystems()` | Mount YAFS from `roxip.storage` |
| `fs_read_file(filename)` | Read a file's content (returns pointer or NULL) |
| `fs_write_file(name, content)` | Write a text file |
| `fs_write_file_binary(name, content, size)` | Write a binary file |
| `fs_mkdir(dirname)` | Create a directory entry |
| `YAFS_load_from_storage()` | Load filesystem from storage file |
| `YAFS_sync_to_storage()` | Write filesystem back to storage |
| `execute_tar(args)` | Extract a TAR archive |
| `unpack_targz_in_ram(src, len)` | Unpack a .tar.gz in RAM |

## Synthetic Nodes

YAFS supports special "@" prefixed nodes that are generated on-the-fly:

| Node | Description |
|------|-------------|
| `@fb0` | Framebuffer info (base address, resolution, size) |
| `@yafs` / `@vfs` | YAFS filesystem info |
| `@history` | Shell command history |

## Storage Creation

Create a storage image:

```bash
# Via the host CLI tool
./roxip-cli create roxip.storage 64

# Or manually
truncate -s 64M roxip.storage
```

For QEMU, the storage file is passed as a pflash drive:
```
-drive file=roxip.storage,if=pflash,format=raw
```

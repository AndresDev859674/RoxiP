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
    int parent_dir;   /* -1 = root directory */
} file_record_t;
```

## Subdirectory Support

YAFS now supports hierarchical directories. Each file or directory record has a `parent_dir` field:
* `-1` = root directory
* `>= 0` = index of the parent directory in `fs_table`

Paths use `/` as a separator, e.g. `/docs/readme.txt` or `/subdir/nested/file.bin`.

Directories are created automatically when needed:
```c
fs_mkdir("/docs");           /* Creates /docs */
fs_mkdir("/docs/subdir");    /* Creates /docs and /docs/subdir if missing */
fs_write_file("/docs/readme.txt", "Hello");  /* Auto-creates /docs */
```

## FAT32 Read-Only Support

RoxiP includes a read-only FAT32 driver (`src/drivers/fat32.c`) for reading files from FAT32-formatted storage images or block devices.

### API

| Function | Description |
|----------|-------------|
| `fat32_init()` | Initialize FAT32 driver (reads boot sector/BPB) |
| `fat32_read_file(fname, buf, max_size)` | Read a file from FAT32 into buffer |
| `fat32_list_dir()` | List root directory contents |

FAT32 is initialized during kernel boot at stage "[08b] Initializing FAT32 Read-Only Driver...". It is **read-only** — no write support.

### Example

```c
fat32_init();
char buf[4096];
int len = fat32_read_file("CONFIG.TXT", buf, sizeof(buf));
if (len > 0) {
    /* buf now contains the file contents */
}
fat32_list_dir();
```

## Wear Leveling

YAFS now includes simple wear leveling for flash storage:

* The storage header is stored in two locations (primary and backup)
* Each `vfs_sync_to_storage()` call alternates between them based on a write counter
* This distributes erase cycles evenly across flash blocks
* On mount, the driver tries primary first, then backup

## API (`src/yafs.c`)

| Function | Description |
|----------|-------------|
| `init_filesystems()` | Mount YAFS from `roxip.storage` |
| `fs_read_file(filename)` | Read a file's content (supports paths like `/dir/file.txt`) |
| `fs_write_file(name, content)` | Write a text file |
| `fs_write_file_binary(name, content, size)` | Write a binary file |
| `fs_mkdir(dirname)` | Create a directory entry (supports paths) |
| `YAFS_load_from_storage()` | Load filesystem from storage file |
| `YAFS_sync_to_storage()` | Write filesystem back to storage (wear-leveled) |
| `execute_tar(args)` | Extract a TAR archive |
| `unpack_targz_in_ram(src, len)` | Unpack a .tar.gz in RAM |

## Synthetic Nodes

YAFS supports special "@" prefixed nodes that are generated on-the-fly:

| Node | Description |
|------|-------------|
| `@fb0` | Framebuffer info (base address, resolution, size) |
| `@yafs` / `@vfs` | YAFS filesystem info (includes wear leveling status) |
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

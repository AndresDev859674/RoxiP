# FAT32 Read-Only Driver (`src/drivers/fat32.c`)

## Overview

Read-only FAT32 filesystem driver for RoxiP. Reads files from FAT32-formatted storage images or block devices. No write support — read-only for safety and simplicity.

## Supported Features

* FAT32 BPB (BIOS Parameter Block) parsing
* FAT chain following for file clusters
* 8.3 short filename support (case-insensitive)
* Root directory and subdirectory traversal
* Read file contents into memory buffer

## Registers / Structures

The driver reads the standard FAT32 BPB from sector 0 of the storage image:

| Field | Offset | Description |
|-------|--------|-------------|
| `bytes_per_sector` | 11 | Usually 512 |
| `sectors_per_cluster` | 13 | Clusters per allocation unit |
| `reserved_sector_count` | 14 | Sectors before first FAT |
| `num_fats` | 16 | Usually 2 |
| `fat_size_32` | 36 | Sectors per FAT |
| `root_cluster` | 44 | Root directory cluster (usually 2) |

## API

| Function | Description |
|----------|-------------|
| `fat32_init()` | Initialize driver — reads boot sector, validates magic |
| `fat32_read_file(fname, buf, max_size)` | Read a file from FAT32 into buffer. Returns bytes read, or -1 on error |
| `fat32_list_dir()` | Print root directory listing to terminal |

## Boot Integration

`fat32_init()` is called from `kmain()` at stage **[08b] Initializing FAT32 Read-Only Driver...**, right after YAFS mounting.

## Usage Example

```c
fat32_init();

unsigned char buf[4096];
int len = fat32_read_file("CONFIG.TXT", buf, sizeof(buf));
if (len > 0) {
    /* Process file contents */
}

fat32_list_dir();
```

## Limitations

* Read-only — no file creation, deletion, or modification
* 8.3 short filenames only (no long filename support)
* Single sector cache — not optimized for random access
* No subdirectory path navigation in `fat32_read_file` (root directory only)

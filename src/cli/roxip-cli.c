/*
 * RoxiP OS Host CLI Utility (roxip-cli) - RoxiP Tools
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_FILENAME_LEN 64
#define MAX_FILE_CONTENT 2097152
#define MAX_VFS_FILES    16
#define STORAGE_MAGIC    0x53465053

typedef struct {
    char name[MAX_FILENAME_LEN];
    char content[MAX_FILE_CONTENT];
    int size;
    int used;
    int is_vfat;
    int is_dir;
} file_record_t;

typedef struct {
    unsigned int magic;
    int file_count;
    file_record_t files[MAX_VFS_FILES];
} storage_header_t;

static void print_usage(const char* prog_name) {
    printf("RoxiP Tools v0.1\n");
    printf("Usage:\n");
    printf("  %s create <storage_path> <size_mb>\n", prog_name);
    printf("  %s list   <storage_path>\n", prog_name);
    printf("  %s read   <storage_path> <vfs_name> [host_out_path]\n", prog_name);
    printf("  %s write  <storage_path> <vfs_name> <text_or_host_path> [-f|--file]\n", prog_name);
    printf("  %s mkdir  <storage_path> <dirname>\n", prog_name);
    printf("  %s rm     <storage_path> <vfs_name_or_dir>\n", prog_name);
}

static int load_header(const char* path, storage_header_t* header, FILE** out_fp) {
    FILE* fp = fopen(path, "r+b");
    if (!fp) {
        perror("Error opening storage image");
        return 0;
    }

    if (fread(header, sizeof(storage_header_t), 1, fp) != 1) {
        fprintf(stderr, "Error: Storage header is unreadable or truncated.\n");
        fclose(fp);
        return 0;
    }

    if (header->magic != STORAGE_MAGIC) {
        fprintf(stderr, "Error: Invalid RoxiP storage magic signature (0x%08X).\n", header->magic);
        fclose(fp);
        return 0;
    }

    if (out_fp) *out_fp = fp;
    else fclose(fp);

    return 1;
}

static int cmd_create(const char* path, int size_mb) {
    if (size_mb <= 0) {
        fprintf(stderr, "Error: Size must be greater than 0 MB.\n");
        return 1;
    }

    FILE* fp = fopen(path, "wb");
    if (!fp) {
        perror("Error creating storage image");
        return 1;
    }

    storage_header_t* header = (storage_header_t*)calloc(1, sizeof(storage_header_t));
    if (!header) {
        fprintf(stderr, "Error: Memory allocation failed.\n");
        fclose(fp);
        return 1;
    }

    header->magic = STORAGE_MAGIC;
    header->file_count = 0;

    fwrite(header, sizeof(storage_header_t), 1, fp);

    size_t total_bytes = (size_t)size_mb * 1024 * 1024;
    size_t remaining_bytes = total_bytes - sizeof(storage_header_t);

    char zero_buffer[4096];
    memset(zero_buffer, 0, sizeof(zero_buffer));

    while (remaining_bytes > 0) {
        size_t chunk = (remaining_bytes > sizeof(zero_buffer)) ? sizeof(zero_buffer) : remaining_bytes;
        fwrite(zero_buffer, 1, chunk, fp);
        remaining_bytes -= chunk;
    }

    free(header);
    fclose(fp);
    printf("Successfully created formatted image '%s' (%d MB)\n", path, size_mb);
    return 0;
}

static int cmd_list(const char* path) {
    storage_header_t* header = (storage_header_t*)malloc(sizeof(storage_header_t));
    if (!header) return 1;

    if (!load_header(path, header, NULL)) {
        free(header);
        return 1;
    }

    printf("Image: %s | Magic: 0x%08X | File Count: %d/%d\n", path, header->magic, header->file_count, MAX_VFS_FILES);
    printf("-------------------------------------------------------------------\n");
    printf("NAME                            SIZE        TYPE        ATTRIBUTES\n");
    printf("-------------------------------------------------------------------\n");

    for (int i = 0; i < MAX_VFS_FILES; i++) {
        if (header->files[i].used) {
            printf("%-30s  %-10d  %-10s  %s\n",
                   header->files[i].name,
                   header->files[i].size,
                   header->files[i].is_dir ? "<DIR>" : "FILE",
                   header->files[i].is_vfat ? "VFAT" : "VFS");
        }
    }
    free(header);
    return 0;
}

static int cmd_read(const char* path, const char* filename, const char* host_out_path) {
    storage_header_t* header = (storage_header_t*)malloc(sizeof(storage_header_t));
    if (!header) return 1;

    if (!load_header(path, header, NULL)) {
        free(header);
        return 1;
    }

    for (int i = 0; i < MAX_VFS_FILES; i++) {
        if (header->files[i].used && strcmp(header->files[i].name, filename) == 0) {
            if (header->files[i].is_dir) {
                fprintf(stderr, "Error: '%s' is a directory.\n", filename);
                free(header);
                return 1;
            }

            if (host_out_path) {
                FILE* out_file = fopen(host_out_path, "wb");
                if (!out_file) {
                    perror("Error creating host output file");
                    free(header);
                    return 1;
                }
                fwrite(header->files[i].content, 1, header->files[i].size, out_file);
                fclose(out_file);
                printf("Successfully extracted binary '%s' to host file '%s' (%d bytes)\n",
                       filename, host_out_path, header->files[i].size);
            } else {
                /* Dump raw bytes out to stdout safely */
                fwrite(header->files[i].content, 1, header->files[i].size, stdout);
                printf("\n");
            }

            free(header);
            return 0;
        }
    }

    fprintf(stderr, "Error: File '%s' not found.\n", filename);
    free(header);
    return 1;
}

static int cmd_write(const char* path, const char* vfs_name, const char* input_arg, int is_file_flag) {
    char* file_payload = (char*)calloc(1, MAX_FILE_CONTENT);
    if (!file_payload) return 1;

    size_t payload_size = 0;

    if (is_file_flag) {
        FILE* host_file = fopen(input_arg, "rb");
        if (!host_file) {
            perror("Error opening host file");
            free(file_payload);
            return 1;
        }

        /* Lectura de bytes crudos (Binarios/PNG/Executables) */
        payload_size = fread(file_payload, 1, MAX_FILE_CONTENT, host_file);

        fseek(host_file, 0, SEEK_END);
        long total_file_size = ftell(host_file);
        fclose(host_file);

        if (total_file_size > MAX_FILE_CONTENT) {
            printf("[Warning] Host file size (%ld bytes) exceeds VFS payload limit (%d bytes). Truncated!\n",
                   total_file_size, MAX_FILE_CONTENT);
        }
    } else {
        payload_size = strlen(input_arg);
        if (payload_size >= MAX_FILE_CONTENT) payload_size = MAX_FILE_CONTENT - 1;
        memcpy(file_payload, input_arg, payload_size);
    }

    storage_header_t* header = (storage_header_t*)malloc(sizeof(storage_header_t));
    FILE* fp = NULL;

    if (!header || !load_header(path, header, &fp)) {
        free(file_payload);
        if (header) free(header);
        return 1;
    }

    int target_idx = -1;

    for (int i = 0; i < MAX_VFS_FILES; i++) {
        if (header->files[i].used && strcmp(header->files[i].name, vfs_name) == 0) {
            if (header->files[i].is_dir) {
                fprintf(stderr, "Error: Cannot overwrite directory '%s'.\n", vfs_name);
                fclose(fp);
                free(file_payload);
                free(header);
                return 1;
            }
            target_idx = i;
            break;
        }
    }

    if (target_idx == -1) {
        if (header->file_count >= MAX_VFS_FILES) {
            fprintf(stderr, "Error: Storage full (Maximum %d files reached).\n", MAX_VFS_FILES);
            fclose(fp);
            free(file_payload);
            free(header);
            return 1;
        }
        for (int i = 0; i < MAX_VFS_FILES; i++) {
            if (!header->files[i].used) {
                target_idx = i;
                header->file_count++;
                break;
            }
        }
    }

    file_record_t* rec = &header->files[target_idx];
    snprintf(rec->name, MAX_FILENAME_LEN, "%s", vfs_name);
    memcpy(rec->content, file_payload, payload_size);
    rec->size = (int)payload_size;
    rec->used = 1;
    rec->is_vfat = (strlen(vfs_name) > 11) ? 1 : 0;
    rec->is_dir = 0;

    fseek(fp, 0, SEEK_SET);
    fwrite(header, sizeof(storage_header_t), 1, fp);
    fclose(fp);

    printf("Successfully wrote '%s' into %s (%d bytes payload)\n", vfs_name, path, rec->size);

    free(file_payload);
    free(header);
    return 0;
}

static int cmd_mkdir(const char* path, const char* dirname) {
    storage_header_t* header = (storage_header_t*)malloc(sizeof(storage_header_t));
    FILE* fp = NULL;

    if (!header || !load_header(path, header, &fp)) {
        if (header) free(header);
        return 1;
    }

    for (int i = 0; i < MAX_VFS_FILES; i++) {
        if (header->files[i].used && strcmp(header->files[i].name, dirname) == 0) {
            fprintf(stderr, "Error: '%s' already exists.\n", dirname);
            fclose(fp);
            free(header);
            return 1;
        }
    }

    if (header->file_count >= MAX_VFS_FILES) {
        fprintf(stderr, "Error: Storage full.\n");
        fclose(fp);
        free(header);
        return 1;
    }

    int target_idx = -1;
    for (int i = 0; i < MAX_VFS_FILES; i++) {
        if (!header->files[i].used) {
            target_idx = i;
            header->file_count++;
            break;
        }
    }

    file_record_t* rec = &header->files[target_idx];
    snprintf(rec->name, MAX_FILENAME_LEN, "%s", dirname);
    rec->content[0] = '\0';
    rec->size = 0;
    rec->used = 1;
    rec->is_vfat = 0;
    rec->is_dir = 1;

    fseek(fp, 0, SEEK_SET);
    fwrite(header, sizeof(storage_header_t), 1, fp);
    fclose(fp);

    printf("Created directory '%s' inside %s\n", dirname, path);
    free(header);
    return 0;
}

static int cmd_rm(const char* path, const char* target_name) {
    storage_header_t* header = (storage_header_t*)malloc(sizeof(storage_header_t));
    FILE* fp = NULL;

    if (!header || !load_header(path, header, &fp)) {
        if (header) free(header);
        return 1;
    }

    int found = 0;
    for (int i = 0; i < MAX_VFS_FILES; i++) {
        if (header->files[i].used && strcmp(header->files[i].name, target_name) == 0) {
            memset(&header->files[i], 0, sizeof(file_record_t));
            header->file_count--;
            found = 1;
            break;
        }
    }

    if (!found) {
        fprintf(stderr, "Error: Target '%s' not found in storage image.\n", target_name);
        fclose(fp);
        free(header);
        return 1;
    }

    fseek(fp, 0, SEEK_SET);
    fwrite(header, sizeof(storage_header_t), 1, fp);
    fclose(fp);

    printf("Successfully removed '%s' from %s\n", target_name, path);
    free(header);
    return 0;
}

int main(int argc, char* argv[]) {
    if (argc < 3) {
        print_usage(argv[0]);
        return 1;
    }

    const char* cmd = argv[1];
    const char* path = argv[2];

    if (strcmp(cmd, "create") == 0) {
        if (argc < 4) {
            fprintf(stderr, "Usage: %s create <storage_path> <size_mb>\n", argv[0]);
            return 1;
        }
        return cmd_create(path, atoi(argv[3]));
    }
    else if (strcmp(cmd, "list") == 0) {
        return cmd_list(path);
    }
    else if (strcmp(cmd, "read") == 0) {
        if (argc < 4) {
            fprintf(stderr, "Usage: %s read <storage_path> <vfs_name> [host_out_path]\n", argv[0]);
            return 1;
        }
        const char* host_out = (argc >= 5) ? argv[4] : NULL;
        return cmd_read(path, argv[3], host_out);
    }
    else if (strcmp(cmd, "write") == 0) {
        if (argc < 5) {
            fprintf(stderr, "Usage: %s write <storage_path> <vfs_name> <text_or_host_path> [-f|--file]\n", argv[0]);
            return 1;
        }

        const char* vfs_name = NULL;
        const char* input_arg = NULL;
        int is_file_flag = 0;

        for (int i = 3; i < argc; i++) {
            if (strcmp(argv[i], "-f") == 0 || strcmp(argv[i], "--file") == 0) {
                is_file_flag = 1;
                if (i + 1 < argc && input_arg == NULL) {
                    input_arg = argv[++i];
                }
            } else if (!vfs_name) {
                vfs_name = argv[i];
            } else if (!input_arg) {
                input_arg = argv[i];
            }
        }

        if (!vfs_name || !input_arg) {
            fprintf(stderr, "Error: Invalid arguments for write command.\n");
            return 1;
        }

        return cmd_write(path, vfs_name, input_arg, is_file_flag);
    }
    else if (strcmp(cmd, "mkdir") == 0) {
        if (argc < 4) {
            fprintf(stderr, "Usage: %s mkdir <storage_path> <dirname>\n", argv[0]);
            return 1;
        }
        return cmd_mkdir(path, argv[3]);
    }
    else if (strcmp(cmd, "rm") == 0 || strcmp(cmd, "delete") == 0) {
        if (argc < 4) {
            fprintf(stderr, "Usage: %s rm <storage_path> <vfs_name_or_dir>\n", argv[0]);
            return 1;
        }
        return cmd_rm(path, argv[3]);
    }

    print_usage(argv[0]);
    return 1;
}
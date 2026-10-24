#include "kernel.h"

block_header_t* free_list = 0;

__attribute__((used)) void* memset(void* dest, int c, unsigned int n) {
    unsigned char* p = (unsigned char*)dest;
    while (n--) *p++ = (unsigned char)c;
    return dest;
}

__attribute__((used)) void* memcpy(void* dest, const void* src, unsigned int n) {
    unsigned char* d = (unsigned char*)dest;
    const unsigned char* s = (const unsigned char*)src;
    while (n--) *d++ = *s++;
    return dest;
}

__attribute__((used)) void* memmove(void* dest, const void* src, unsigned int n) {
    unsigned char* d = (unsigned char*)dest;
    const unsigned char* s = (const unsigned char*)src;
    if (d < s) {
        while (n--) *d++ = *s++;
    } else {
        d += n;
        s += n;
        while (n--) *--d = *--s;
    }
    return dest;
}

__attribute__((used)) int memcmp(const void* s1, const void* s2, unsigned int n) {
    const unsigned char* p1 = (const unsigned char*)s1;
    const unsigned char* p2 = (const unsigned char*)s2;
    while (n--) {
        if (*p1 != *p2) return *p1 - *p2;
        p1++; p2++;
    }
    return 0;
}

int strcmp(const char* s1, const char* s2) {
    while (*s1 && (*s1 == *s2)) { s1++; s2++; }
    return *(unsigned char*)s1 - *(unsigned char*)s2;
}

int strncmp(const char* s1, const char* s2, int n) {
    while (n > 0 && *s1 && (*s1 == *s2)) { s1++; s2++; n--; }
    if (n == 0) return 0;
    return *(unsigned char*)s1 - *(unsigned char*)s2;
}

int strlen(const char* str) {
    int len = 0;
    while (str[len]) len++;
    return len;
}

void strcpy(char* dest, const char* src) {
    int i = 0;
    while (src[i]) { dest[i] = src[i]; i++; }
    dest[i] = '\0';
}

void int_to_str(int num, char* str) {
    int i = 0;
    if (num == 0) { str[i++] = '0'; str[i] = '\0'; return; }
    char temp[16]; int p = 0;
    while (num > 0) { temp[p++] = (num % 10) + '0'; num /= 10; }
    while (p > 0) { str[i++] = temp[--p]; }
    str[i] = '\0';
}

void hex_to_str(unsigned int val, char* buf) {
    const char hex_chars[] = "0123456789ABCDEF";
    buf[0] = '0'; buf[1] = 'x';
    for (int i = 7; i >= 0; i--) buf[2 + (7 - i)] = hex_chars[(val >> (i * 4)) & 0xF];
    buf[10] = '\0';
}

void init_heap(void) {
    dbg_log("    [HEAP 1/3] Initializing Memory Arena...");
    dbg_log("    [HEAP 2/3] Aligning heap after __bss_end...");

    unsigned int heap_start = ((unsigned int)__bss_end + 15) & ~15;
    if (heap_start < 0x00100000) heap_start = 0x00100000 + 0x20000;

    dbg_log("    [HEAP 3/3] Writing Free List Header...");
    free_list = (block_header_t*)heap_start;
    free_list->size = HEAP_SIZE - sizeof(block_header_t);
    free_list->is_free = 1;
    free_list->next = 0;

    log_boot_event("[OK] Dynamic Heap allocated safely in ARM RAM");
}

unsigned int get_used_heap_bytes(void) {
    unsigned int used = 0;
    block_header_t* curr = free_list;
    while (curr) {
        if (!curr->is_free) used += curr->size + sizeof(block_header_t);
        curr = curr->next;
    }
    return used;
}

void* malloc(unsigned int size) {
    if (size == 0) return 0;
    size = (size + 3) & ~3;

    block_header_t* curr = free_list;
    while (curr) {
        if (curr->is_free && curr->size >= size) {
            if (curr->size >= size + sizeof(block_header_t) + 16) {
                block_header_t* next_block = (block_header_t*)((unsigned char*)curr + sizeof(block_header_t) + size);
                next_block->size = curr->size - size - sizeof(block_header_t);
                next_block->is_free = 1;
                next_block->next = curr->next;

                curr->size = size;
                curr->next = next_block;
            }
            curr->is_free = 0;
            return (void*)((unsigned char*)curr + sizeof(block_header_t));
        }
        curr = curr->next;
    }
    return 0;
}

void free(void* ptr) {
    if (!ptr) return;
    block_header_t* block = (block_header_t*)((unsigned char*)ptr - sizeof(block_header_t));
    block->is_free = 1;

    block_header_t* curr = free_list;
    while (curr && curr->next) {
        if (curr->is_free && curr->next->is_free) {
            curr->size += sizeof(block_header_t) + curr->next->size;
            curr->next = curr->next->next;
        } else {
            curr = curr->next;
        }
    }
}
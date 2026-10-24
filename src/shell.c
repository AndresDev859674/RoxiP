#include "kernel.h"

char cmd_history[MAX_CMD_HIST][128];
int hist_count = 0;

env_var_t env_table[MAX_ENV_VARS];
char current_dir[MAX_FILENAME_LEN] = "/";

char pipe_buffer[4096];
int pipe_capture_active = 0;

static void save_history_to_vfs(void) {
    static char blob[4096];
    blob[0] = '\0';
    int pos = 0;

    for (int i = 0; i < hist_count && pos < 4000; i++) {
        int len = strlen(cmd_history[i]);
        for (int j = 0; j < len; j++) blob[pos++] = cmd_history[i][j];
        blob[pos++] = '\n';
    }
    blob[pos] = '\0';
    fs_write_file("history.txt", blob);
}

static void add_to_history(const char* cmd) {
    if (cmd[0] == '\0') return;
    if (hist_count > 0 && strcmp(cmd_history[hist_count - 1], cmd) == 0) return;

    if (hist_count < MAX_CMD_HIST) {
        strcpy(cmd_history[hist_count++], cmd);
    } else {
        for (int i = 0; i < MAX_CMD_HIST - 1; i++) strcpy(cmd_history[i], cmd_history[i + 1]);
        strcpy(cmd_history[MAX_CMD_HIST - 1], cmd);
    }
    save_history_to_vfs();
}

void init_env_vars(void) {
    for (int i = 0; i < MAX_ENV_VARS; i++) env_table[i].used = 0;
    strcpy(env_table[0].key, "OS"); strcpy(env_table[0].value, "RoxiP"); env_table[0].used = 1;
    strcpy(env_table[1].key, "SHELL"); strcpy(env_table[1].value, "perf"); env_table[1].used = 1;
}

int env_set(const char* key, const char* val) {
    for (int i = 0; i < MAX_ENV_VARS; i++) {
        if (env_table[i].used && strcmp(env_table[i].key, key) == 0) {
            strcpy(env_table[i].value, val); return 1;
        }
    }
    for (int i = 0; i < MAX_ENV_VARS; i++) {
        if (!env_table[i].used) {
            strcpy(env_table[i].key, key); strcpy(env_table[i].value, val);
            env_table[i].used = 1; return 1;
        }
    }
    return 0;
}

int env_unset(const char* key) {
    for (int i = 0; i < MAX_ENV_VARS; i++) {
        if (env_table[i].used && strcmp(env_table[i].key, key) == 0) {
            env_table[i].used = 0; return 1;
        }
    }
    return 0;
}

static inline unsigned int read_cp15_main_id(void) {
    unsigned int val; asm volatile("mrc p15, 0, %0, c0, c0, 0" : "=r"(val)); return val;
}

static inline unsigned int read_cp15_control(void) {
    unsigned int val; asm volatile("mrc p15, 0, %0, c1, c0, 0" : "=r"(val)); return val;
}

static void execute_whatweus(void) {
    term_add_line("  /\\        OS: RoxiP OS v0.1");
    term_add_line(" /  \\       Kernel: Momentanum v0.1");
    term_add_line("/ /\\ \\      Compositor: comcow Engine");

    char gpu_line[74] = "\\ \\/ /      GPU: "; int g_idx = 17; int p = 0;
    while (gpu_model_string[p] && g_idx < 72) gpu_line[g_idx++] = gpu_model_string[p++];
    gpu_line[g_idx] = '\0'; term_add_line(gpu_line);

    char audio_line[74] = " \\  /       Audio: "; int a_idx = 19; p = 0;
    while (audio_model_string[p] && a_idx < 72) audio_line[a_idx++] = audio_model_string[p++];
    audio_line[a_idx] = '\0'; term_add_line(audio_line);

    char cpu_line[74] = "  \\/        CPU: "; int c_idx = 17; p = 0;
    while (cpu_model_string[p] && c_idx < 72) cpu_line[c_idx++] = cpu_model_string[p++];
    cpu_line[c_idx] = '\0'; term_add_line(cpu_line);

    unsigned int total_system_ram_mb = 512;
    unsigned int used_kb = get_used_heap_bytes() / 1024;
    char ram_str[64] = "RAM: ", used_buf[16], total_buf[16];
    int_to_str(used_kb, used_buf); int_to_str(total_system_ram_mb, total_buf);

    int idx = 5; p = 0;
    while (used_buf[p]) ram_str[idx++] = used_buf[p++];
    ram_str[idx++] = 'K'; ram_str[idx++] = 'B'; ram_str[idx++] = ' ';
    ram_str[idx++] = 'U'; ram_str[idx++] = 's'; ram_str[idx++] = 'e'; ram_str[idx++] = 'd'; ram_str[idx++] = ' ';
    ram_str[idx++] = '/'; ram_str[idx++] = ' ';
    p = 0;
    while (total_buf[p]) ram_str[idx++] = total_buf[p++];
    ram_str[idx++] = 'M'; ram_str[idx++] = 'B'; ram_str[idx++] = ' ';
    ram_str[idx++] = '('; ram_str[idx++] = 'A'; ram_str[idx++] = 'R'; ram_str[idx++] = 'M'; ram_str[idx++] = ')';
    ram_str[idx] = '\0';

    char ram_line[74] = "            "; c_idx = 12; p = 0;
    while (ram_str[p] && c_idx < 72) ram_line[c_idx++] = ram_str[p++];
    ram_line[c_idx] = '\0'; term_add_line(ram_line);
}

static void execute_zcatcher(void) {
    char hex_buf[12], num_buf[16];
    term_add_line(" ");

    volatile unsigned int* sys_id_reg = (volatile unsigned int*)(0x10000000);
    hex_to_str(*sys_id_reg, hex_buf);
    char sys_line[80] = "[SYSCON ID]       : "; strcpy(sys_line + 20, hex_buf); term_add_line(sys_line);

    unsigned int main_id = read_cp15_main_id();
    hex_to_str(main_id, hex_buf);
    char cpu_line[80] = "[CP15 MAIN ID]    : "; strcpy(cpu_line + 20, hex_buf);
    if ((main_id & 0x000F0000) == 0x00020000) strcpy(cpu_line + 31, " (ARM9 Family / ARM926EJ-S)");
    term_add_line(cpu_line);

    unsigned int cp15_ctrl = read_cp15_control();
    hex_to_str(cp15_ctrl, hex_buf);
    char ctrl_line[80] = "[CP15 CTRL REG]   : "; strcpy(ctrl_line + 20, hex_buf); term_add_line(ctrl_line);

    char flags_line[80] = "  |-- System Bits : "; int flags_idx = 20;
    if (cp15_ctrl & (1 << 0)) { strcpy(flags_line + flags_idx, "MMU:ON "); flags_idx += 7; }
    else { strcpy(flags_line + flags_idx, "MMU:OFF "); flags_idx += 8; }

    if (cp15_ctrl & (1 << 2)) { strcpy(flags_line + flags_idx, "D-Cache:ON "); flags_idx += 11; }
    else { strcpy(flags_line + flags_idx, "D-Cache:OFF "); flags_idx += 12; }

    if (cp15_ctrl & (1 << 12)) strcpy(flags_line + flags_idx, "I-Cache:ON");
    else strcpy(flags_line + flags_idx, "I-Cache:OFF");
    term_add_line(flags_line);

    hex_to_str((unsigned int)fb_pixels, hex_buf);
    char fb_line[80] = "[PL110 FB BASE]   : "; strcpy(fb_line + 20, hex_buf); term_add_line(fb_line);

    hex_to_str(PL110_LCDCONTROL, hex_buf);
    char lcd_line[80] = "[PL110 CTRL REG]  : "; strcpy(lcd_line + 20, hex_buf); term_add_line(lcd_line);

    int_to_str(get_used_heap_bytes(), num_buf);
    char mem_line[80] = "[HEAP RAM USED]   : "; strcpy(mem_line + 20, num_buf);
    strcpy(mem_line + 20 + strlen(num_buf), " Bytes / 524288 Bytes"); term_add_line(mem_line);
    term_add_line(" ");
}

static void execute_lsblk(void) {
    char num_buf[16], hex_buf[12];
    term_add_line("NAME        MAJ:MIN  TYPE   BASE_ADDR   TOTAL_SIZE  USED_BYTES");
    term_add_line("-----------------------------------------------------------------");
    term_add_line("ram_vec0      1:0    ram    0x00000000  4096 B      1024 B (Vectors)");

    char heap_line[80] = "ram_heap0     1:1    ram    ";
    unsigned int heap_base = ((unsigned int)__bss_end + 15) & ~15;
    if (heap_base < 0x00100000) heap_base = 0x00100000 + 0x20000;
    hex_to_str(heap_base, hex_buf);
    strcpy(heap_line + 28, hex_buf); strcpy(heap_line + 38, "  512 KB      ");
    int_to_str(get_used_heap_bytes(), num_buf);
    strcpy(heap_line + 52, num_buf); strcpy(heap_line + 52 + strlen(num_buf), " B");
    term_add_line(heap_line);

    char fb_line[80] = "fb_pl1100    29:0    vram   ";
    hex_to_str((unsigned int)fb_pixels, hex_buf);
    strcpy(fb_line + 28, hex_buf); strcpy(fb_line + 38, "  614 KB      614400 B"); term_add_line(fb_line);

    char vfs_line[80] = "vfs_0     31:0    vfs    RAM         ";
    int total_vfs_bytes = 0;
    for (int i = 0; i < fs_file_count; i++) if (fs_table[i].used) total_vfs_bytes += fs_table[i].size;
    int_to_str(fs_file_count * sizeof(file_record_t), num_buf);
    strcpy(vfs_line + 40, num_buf); strcpy(vfs_line + 40 + strlen(num_buf), " B       ");
    int_to_str(total_vfs_bytes, num_buf);
    strcpy(vfs_line + 52, num_buf); strcpy(vfs_line + 52 + strlen(num_buf), " B (Data)");
    term_add_line(vfs_line);

    char flash_line[80] = "flash_nor0   31:1    flash  ";
    hex_to_str(STORAGE_FLASH_BASE, hex_buf);
    strcpy(flash_line + 28, hex_buf); strcpy(flash_line + 38, "  64 MB       ");
    int_to_str(sizeof(storage_header_t), num_buf);
    strcpy(flash_line + 52, num_buf); strcpy(flash_line + 52 + strlen(num_buf), " B (Header)");
    term_add_line(flash_line);
}

void execute_stat(const char* filename) {
    if (filename[0] == '\0') {
        term_add_line("Usage: stat <filename|@node>");
        return;
    }

    if (filename[0] == '@') {
        const char* node_data = fs_read_file(filename);
        if (node_data) {
            char line[80], num_buf[16];
            term_add_line("--- RoxiP Synthetic Node Stat ---");
            strcpy(line, "Node Name  : "); strcpy(line + 13, filename); term_add_line(line);
            int_to_str(strlen(node_data), num_buf);
            strcpy(line, "Data Size  : "); strcpy(line + 13, num_buf); strcpy(line + 13 + strlen(num_buf), " Bytes"); term_add_line(line);
            term_add_line("Attributes : Synthetic Callback Node");
            return;
        }
    }

    for (int i = 0; i < fs_file_count; i++) {
        if (fs_table[i].used && strcmp(fs_table[i].name, filename) == 0) {
            char line[80], num_buf[16], hex_buf[12];
            term_add_line("--- VFS File Stat Info ---");
            strcpy(line, "File Name  : "); strcpy(line + 13, fs_table[i].name); term_add_line(line);
            int_to_str(fs_table[i].size, num_buf);
            strcpy(line, "File Size  : "); strcpy(line + 13, num_buf); strcpy(line + 13 + strlen(num_buf), " Bytes"); term_add_line(line);
            hex_to_str((unsigned int)fs_table[i].content, hex_buf);
            strcpy(line, "RAM Addr   : "); strcpy(line + 13, hex_buf); term_add_line(line);
            strcpy(line, "Record Type: "); strcpy(line + 13, fs_table[i].is_dir ? "Directory" : "Regular File"); term_add_line(line);
            strcpy(line, "Attributes : "); strcpy(line + 13, fs_table[i].is_vfat ? "VFAT / LFN" : "Standard VFS"); term_add_line(line);
            return;
        }
    }
    term_add_line("stat: File or node not found");
}

void execute_xxd(const char* filename) {
    if (filename[0] == '\0') {
        term_add_line("Usage: xxd <filename|@node>");
        return;
    }

    const char* data = fs_read_file(filename);
    if (!data) {
        term_add_line("xxd: File or node not found");
        return;
    }

    const unsigned char* bytes = (const unsigned char*)data;
    int size = strlen(data);
    if (size == 0) {
        term_add_line("xxd: File is empty");
        return;
    }

    int dump_limit = (size > 128) ? 128 : size;
    for (int offset = 0; offset < dump_limit; offset += 8) {
        char line[80]; char hex_off[12];
        hex_to_str(offset, hex_off);
        int pos = 0;
        for (int k = 2; k < 6; k++) line[pos++] = hex_off[k];
        line[pos++] = ':'; line[pos++] = ' ';

        for (int b = 0; b < 8; b++) {
            if (offset + b < dump_limit) {
                const char hex_chars[] = "0123456789abcdef";
                unsigned char val = bytes[offset + b];
                line[pos++] = hex_chars[(val >> 4) & 0xF];
                line[pos++] = hex_chars[val & 0xF];
                line[pos++] = ' ';
            } else {
                line[pos++] = ' '; line[pos++] = ' '; line[pos++] = ' ';
            }
        }
        line[pos++] = ' ';

        for (int b = 0; b < 8; b++) {
            if (offset + b < dump_limit) {
                unsigned char val = bytes[offset + b];
                line[pos++] = (val >= 32 && val <= 126) ? (char)val : '.';
            }
        }
        line[pos] = '\0';
        term_add_line(line);
    }
    if (size > 128) term_add_line("[xxd: Truncated at 128 bytes]");
}

static void decode_arm32_inst(unsigned int addr, unsigned int inst, char* out_line) {
    const char* reg_names[] = {"R0","R1","R2","R3","R4","R5","R6","R7","R8","R9","R10","FP","IP","SP","LR","PC"};
    const char* cond_names[] = {"EQ","NE","CS","CC","MI","PL","VS","VC","HI","LS","GE","LT","GT","LE","",""};

    unsigned int cond = (inst >> 28) & 0xF;
    const char* c_str = cond_names[cond];

    char hex_addr[12], hex_inst[12];
    hex_to_str(addr, hex_addr);
    hex_to_str(inst, hex_inst);

    int pos = 0;
    for (int k = 2; k < 10; k++) out_line[pos++] = hex_addr[k];
    out_line[pos++] = ':'; out_line[pos++] = ' ';
    for (int k = 2; k < 10; k++) out_line[pos++] = hex_inst[k];
    out_line[pos++] = ' '; out_line[pos++] = ' ';

    if ((inst & 0x0FFFFFF0) == 0x012FFF10) {
        int rm = inst & 0xF;
        strcpy(out_line + pos, "BX"); pos += 2;
        if (c_str[0]) { strcpy(out_line + pos, c_str); pos += strlen(c_str); }
        while (pos < 28) out_line[pos++] = ' ';
        strcpy(out_line + pos, reg_names[rm]);
        return;
    }

    if (((inst >> 25) & 0x7) == 5) {
        int link = (inst >> 24) & 1;
        const char* b_op = link ? "BL" : "B";
        strcpy(out_line + pos, b_op); pos += strlen(b_op);
        if (c_str[0]) { strcpy(out_line + pos, c_str); pos += strlen(c_str); }
        while (pos < 28) out_line[pos++] = ' ';

        signed int imm24 = inst & 0xFFFFFF;
        if (imm24 & 0x800000) imm24 |= 0xFF000000;
        unsigned int target = addr + 8 + (imm24 << 2);
        char hex_target[12];
        hex_to_str(target, hex_target);
        strcpy(out_line + pos, hex_target);
        return;
    }

    if (((inst >> 26) & 0x3) == 0) {
        const char* dp_ops[] = {"AND","EOR","SUB","RSB","ADD","ADC","SBC","RSC","TST","TEQ","CMP","CMN","ORR","MOV","BIC","MVN"};
        int op = (inst >> 21) & 0xF;
        int rn = (inst >> 16) & 0xF;
        int rd = (inst >> 12) & 0xF;
        int is_imm = (inst >> 25) & 1;

        const char* op_name = dp_ops[op];
        strcpy(out_line + pos, op_name); pos += strlen(op_name);
        if (c_str[0]) { strcpy(out_line + pos, c_str); pos += strlen(c_str); }
        while (pos < 28) out_line[pos++] = ' ';

        if (op == 13 || op == 15) {
            strcpy(out_line + pos, reg_names[rd]); pos += strlen(reg_names[rd]);
            strcpy(out_line + pos, ", "); pos += 2;
        } else if (op >= 8 && op <= 11) {
            strcpy(out_line + pos, reg_names[rn]); pos += strlen(reg_names[rn]);
            strcpy(out_line + pos, ", "); pos += 2;
        } else {
            strcpy(out_line + pos, reg_names[rd]); pos += strlen(reg_names[rd]);
            strcpy(out_line + pos, ", "); pos += 2;
            strcpy(out_line + pos, reg_names[rn]); pos += strlen(reg_names[rn]);
            strcpy(out_line + pos, ", "); pos += 2;
        }

        if (is_imm) {
            unsigned int val = inst & 0xFF;
            char val_str[16];
            int_to_str(val, val_str);
            out_line[pos++] = '#';
            strcpy(out_line + pos, val_str);
        } else {
            int rm = inst & 0xF;
            strcpy(out_line + pos, reg_names[rm]);
        }
        return;
    }

    if (((inst >> 26) & 0x3) == 1) {
        int is_ldr = (inst >> 20) & 1;
        int rn = (inst >> 16) & 0xF;
        int rd = (inst >> 12) & 0xF;
        const char* l_op = is_ldr ? "LDR" : "STR";

        strcpy(out_line + pos, l_op); pos += 3;
        if (c_str[0]) { strcpy(out_line + pos, c_str); pos += strlen(c_str); }
        while (pos < 28) out_line[pos++] = ' ';

        strcpy(out_line + pos, reg_names[rd]); pos += strlen(reg_names[rd]);
        strcpy(out_line + pos, ", ["); pos += 3;
        strcpy(out_line + pos, reg_names[rn]); pos += strlen(reg_names[rn]);
        int offset = inst & 0xFFF;
        if (offset > 0) {
            char off_str[16];
            int_to_str(offset, off_str);
            strcpy(out_line + pos, ", #"); pos += 3;
            strcpy(out_line + pos, off_str); pos += strlen(off_str);
        }
        out_line[pos++] = ']';
        out_line[pos] = '\0';
        return;
    }

    strcpy(out_line + pos, "DATA / UNDEF");
}

void execute_disasm(const char* arg) {
    if (arg[0] == '\0') {
        term_add_line("Usage: disasm <filename|@node|0xADDR>");
        return;
    }

    term_add_line("--- RoxiP ARM32 Mini Disassembler ---");

    if (arg[0] == '0' && arg[1] == 'x') {
        unsigned int addr = 0;
        for (int i = 2; arg[i] != '\0'; i++) {
            addr <<= 4;
            if (arg[i] >= '0' && arg[i] <= '9') addr |= (arg[i] - '0');
            else if (arg[i] >= 'a' && arg[i] <= 'f') addr |= (arg[i] - 'a' + 10);
            else if (arg[i] >= 'A' && arg[i] <= 'F') addr |= (arg[i] - 'A' + 10);
        }

        const unsigned int* code_ptr = (const unsigned int*)addr;
        for (int k = 0; k < 8; k++) {
            char line[80];
            decode_arm32_inst(addr + (k * 4), code_ptr[k], line);
            term_add_line(line);
        }
        return;
    }

    const char* data = fs_read_file(arg);
    if (!data) {
        term_add_line("disasm: File or node not found");
        return;
    }

    const unsigned int* code_ptr = (const unsigned int*)data;
    for (int k = 0; k < 8; k++) {
        char line[80];
        decode_arm32_inst((unsigned int)(code_ptr + k), code_ptr[k], line);
        term_add_line(line);
    }
}

void execute_ecr_script(const char* filename) {
    if (filename[0] == '\0') {
        term_add_line("Usage: ecr <filename.ecr>");
        return;
    }

    const char* script = fs_read_file(filename);
    if (!script) {
        term_add_line("ecr: Script file not found or empty");
        return;
    }

    static char cmd_buf[128];
    int pos = 0;
    int i = 0;

    while (script[i] != '\0') {
        if (script[i] == '\n' || script[i] == '\r') {
            cmd_buf[pos] = '\0';
            while (pos > 0 && (cmd_buf[pos - 1] == ' ' || cmd_buf[pos - 1] == '\r')) {
                cmd_buf[--pos] = '\0';
            }
            if (pos > 0 && cmd_buf[0] != '#') {
                execute_command(cmd_buf);
            }
            pos = 0;
        } else if (pos < 127) {
            cmd_buf[pos++] = script[i];
        }
        i++;
    }

    if (pos > 0) {
        cmd_buf[pos] = '\0';
        if (cmd_buf[0] != '#') {
            execute_command(cmd_buf);
        }
    }
}

void execute_command(const char* cmd) {
    if (cmd[0] == '\0') { term_add_line("perf > "); return; }

    /* Parsing Pipes '|' */
    int pipe_pos = -1;
    for (int i = 0; cmd[i] != '\0'; i++) {
        if (cmd[i] == '|') { pipe_pos = i; break; }
    }

    if (pipe_pos != -1) {
        static char cmd1[64], cmd2[64];
        int c1 = 0, c2 = 0;
        for (int i = 0; i < pipe_pos && c1 < 63; i++) cmd1[c1++] = cmd[i];
        while (c1 > 0 && cmd1[c1 - 1] == ' ') c1--;
        cmd1[c1] = '\0';

        int start2 = pipe_pos + 1;
        while (cmd[start2] == ' ') start2++;
        while (cmd[start2] != '\0' && c2 < 63) cmd2[c2++] = cmd[start2++];
        while (c2 > 0 && cmd2[c2 - 1] == ' ') c2--;
        cmd2[c2] = '\0';

        pipe_buffer[0] = '\0';
        pipe_capture_active = 1;
        execute_command(cmd1);
        pipe_capture_active = 0;

        if (cmd2[0] != '\0') {
            int has_space = 0;
            for (int i = 0; cmd2[i] != '\0'; i++) if (cmd2[i] == ' ') { has_space = 1; break; }

            static char full_cmd2[128];
            if (!has_space) {
                strcpy(full_cmd2, cmd2);
                strcpy(full_cmd2 + strlen(full_cmd2), " @pipe");
            } else {
                strcpy(full_cmd2, cmd2);
            }
            execute_command(full_cmd2);
        }
        return;
    }

    add_to_history(cmd);

    char prompt_line[128] = "perf > "; int p = 7;
    for (int i = 0; cmd[i] && p < 120; i++) prompt_line[p++] = cmd[i];
    prompt_line[p] = '\0'; term_add_line(prompt_line);

    int cmd_len = strlen(cmd);

    if (cmd[0] == '@') {
        const char* node_data = fs_read_file(cmd);
        if (node_data) term_add_line(node_data);
        else term_add_line("VFS: Synthetic node not found");
    }
    else if (strcmp(cmd, "comcow") == 0 || strcmp(cmd, "comcow info") == 0) {
        term_add_line("--- comcow Display Compositor Engine ---");
        char line1[64] = "Mode: "; int idx = 6; p = 0;
        while (comcow.mode_name[p]) line1[idx++] = comcow.mode_name[p++];
        line1[idx] = '\0'; term_add_line(line1);
        term_add_line("Double Buffering: Enabled (PL110 Framebuffer)");
        term_add_line("Render Pipeline: Zero-Tear Atomic Flush");
        term_add_line("Commands of Comcow available :");
        term_add_line("modes, set");
    }
    else if (strcmp(cmd, "comcow modes") == 0) {
        term_add_line("Available Video Modes & Refresh Rates:");
        term_add_line(" 1. 80x25 @ 60Hz (Standard Serial Text)");
        term_add_line(" 2. 80x25 @ 70Hz (High Refresh Text Mode)");
        term_add_line(" 3. 80x50 @ 60Hz (High Density Text Mode)");
        term_add_line(" 4. 320x200 @ 60Hz (ARM Graphics Mode)");
        term_add_line("Usage: comcow set <mode_id>");
    }
    else if (strncmp(cmd, "comcow set ", 11) == 0) {
        int mode_id = cmd[11] - '0';
        if (mode_id >= 1 && mode_id <= 4) { comcow_set_mode(mode_id); term_add_line("comcow: Display mode switched successfully."); }
        else term_add_line("comcow: Invalid mode_id. Type 'comcow modes'");
    }
    else if (strcmp(cmd, "echo") == 0) term_add_line("Usage: echo <text>");
    else if (strncmp(cmd, "echo ", 5) == 0) {
        const char* echo_arg = cmd + 5; int redirect_pos = -1;
        for (int i = 0; echo_arg[i] != '\0'; i++) if (echo_arg[i] == '>') { redirect_pos = i; break; }
        if (redirect_pos != -1) {
            static char content[MAX_FILE_CONTENT];
            char filename[MAX_FILENAME_LEN];
            int c_idx = 0;

            for (int i = 0; i < redirect_pos && c_idx < MAX_FILE_CONTENT - 1; i++) {
                if (echo_arg[i] != '"') content[c_idx++] = echo_arg[i];
            }
            while (c_idx > 0 && content[c_idx - 1] == ' ') c_idx--;
            content[c_idx] = '\0';

            int f_idx = 0, start_f = redirect_pos + 1;
            while (echo_arg[start_f] == ' ') start_f++;
            while (echo_arg[start_f] != '\0' && f_idx < MAX_FILENAME_LEN - 1) {
                if (echo_arg[start_f] != ' ') filename[f_idx++] = echo_arg[start_f];
                start_f++;
            }
            filename[f_idx] = '\0';
            if (filename[0] != '\0') {
                if (fs_write_file(filename, content)) term_add_line("FS: File written successfully.");
                else term_add_line("FS Error: Storage full or target is directory.");
            } else term_add_line("echo: Syntax error near '>'");
            return;
        } else { term_add_line(echo_arg); return; }
    }
    else if (strcmp(cmd, "soundtest") == 0 || strcmp(cmd, "play") == 0) {
        if (audio_initialized) {
            term_add_line("ARM Audio: MMIO PCM Buffer configured.");
            term_add_line("ARM Audio: DMA Buffer Ring Active.");
            term_add_line("ARM Audio: Triggering PCM Stream... [Playback Simulation OK]");
        } else term_add_line("Audio Error: ARM Audio Controller not initialized.");
    }
    else if (strcmp(cmd, "env") == 0) {
        for (int i = 0; i < MAX_ENV_VARS; i++) {
            if (env_table[i].used) {
                char env_line[64]; int idx = 0; const char* k = env_table[i].key; const char* v = env_table[i].value;
                while (*k) env_line[idx++] = *k++;
                env_line[idx++] = '=';
                while (*v) env_line[idx++] = *v++;
                env_line[idx] = '\0';
                term_add_line(env_line);
            }
        }
    }
    else if (strcmp(cmd, "set") == 0) term_add_line("Usage: set KEY=VALUE");
    else if (strncmp(cmd, "set ", 4) == 0) {
        const char* args = cmd + 4;
        char key[MAX_ENV_KEY], val[MAX_ENV_VAL];
        int eq_pos = -1;
        for (int i = 0; args[i] != '\0'; i++) {
            if (args[i] == '=') {
                eq_pos = i;
                break;
            }
        }
        if (eq_pos > 0) {
            int k = 0;
            for (k = 0; k < eq_pos && k < MAX_ENV_KEY - 1; k++) {
                key[k] = args[k];
            }
            key[k] = '\0';

            int v = 0, src_v = eq_pos + 1;
            while (args[src_v] != '\0' && v < MAX_ENV_VAL - 1) {
                val[v++] = args[src_v++];
            }
            val[v] = '\0';

            if (env_set(key, val)) term_add_line("ENV: Variable set.");
            else term_add_line("ENV Error: Storage full.");
        } else term_add_line("Usage: set KEY=VALUE");
    }
    else if (strcmp(cmd, "unset") == 0) term_add_line("Usage: unset KEY");
    else if (strncmp(cmd, "unset ", 6) == 0) {
        if (env_unset(cmd + 6)) term_add_line("ENV: Variable removed.");
        else term_add_line("ENV: Variable not found.");
    }
    else if (strcmp(cmd, "mem") == 0) {
        term_add_line("--- RoxiP Dynamic Memory Allocator ---");
        term_add_line("Heap Size: 512 KB (524288 Bytes @ ARM RAM)");
        term_add_line("Status: malloc / free Engine Ready");
    }
    else if (strcmp(cmd, "rgbdemo") == 0) {
        rgb_framebuffer_t* fb = create_rgb_framebuffer(20, 4);
        if (fb) {
            rgb_color_t red = {255, 0, 0}, green = {0, 255, 0}, blue = {0, 0, 255};
            rgb_fill_rect(fb, 0, 0, 6, 4, red); rgb_fill_rect(fb, 6, 0, 7, 4, green); rgb_fill_rect(fb, 13, 0, 7, 4, blue);
            rgb_render_to_vga(fb, 2, 10);
            free_rgb_framebuffer(fb);
            term_add_line("RGB Software Buffer rendered cleanly.");
        } else term_add_line("RGB Software Error: Memory Allocation Failed.");
    }
    else if (strcmp(cmd, "ztop") == 0) current_mode = MODE_ZTOP;
    else if (strcmp(cmd, "mkdir") == 0) term_add_line("Usage: mkdir <dirname>");
    else if (strncmp(cmd, "mkdir ", 6) == 0) {
        if (fs_mkdir(cmd + 6)) term_add_line("FS: Directory created.");
        else term_add_line("FS Error: Directory exists or table full.");
    }
    else if (strcmp(cmd, "micro") == 0) term_add_line("Usage: micro <filename>");
    else if (strncmp(cmd, "micro ", 6) == 0) {
        const char* fname = cmd + 6;
        if (fname[0] != '\0') open_micro_editor(fname);
        else term_add_line("Usage: micro <filename>");
    }
    else if (strcmp(cmd, "stat") == 0) execute_stat("");
    else if (strncmp(cmd, "stat ", 5) == 0) execute_stat(cmd + 5);
    else if (strcmp(cmd, "xxd") == 0) execute_xxd("");
    else if (strncmp(cmd, "xxd ", 4) == 0) execute_xxd(cmd + 4);
    else if (strcmp(cmd, "disasm") == 0) execute_disasm("");
    else if (strncmp(cmd, "disasm ", 7) == 0) execute_disasm(cmd + 7);
    else if (strcmp(cmd, "ecr") == 0) execute_ecr_script("");
    else if (strncmp(cmd, "ecr ", 4) == 0) execute_ecr_script(cmd + 4);
    else if (cmd_len > 4 && strcmp(cmd + cmd_len - 4, ".ecr") == 0) execute_ecr_script(cmd);
    else if (strcmp(cmd, "whatweus") == 0 || strcmp(cmd, "fetch") == 0) execute_whatweus();
    else if (strcmp(cmd, "floatcube") == 0) {
        if (validate_3d_pipeline()) {
            vga_force_redraw();
            current_mode = MODE_FLOATCUBE;
            term_add_line("3D Engine: Environment check PASSED. Starting 3D pipeline...");
        } else {
            term_add_line("3D Engine Panic: Pre-flight checks failed! Launch aborted.");
        }
    }
    else if (strcmp(cmd, "dmesg") == 0) {
        term_add_line("--- RoxiP Kernel Diagnostics Log ---");
        for (int i = 0; i < boot_log_count; i++) term_add_line(boot_logs[i]);
    }
    else if (strcmp(cmd, "desk") == 0 || strcmp(cmd, "DESK") == 0) current_mode = MODE_DESK;
    else if (strcmp(cmd, "ls") == 0 || strncmp(cmd, "ls ", 3) == 0) {
        term_add_line("NAME                    SIZE        FS TYPE");
        term_add_line("-------------------------------------------");
        for (int f = 0; f < fs_file_count; f++) {
            if (fs_table[f].used) {
                char line[64]; int idx = 0; char display_name[24]; int n_len = strlen(fs_table[f].name);
                if (n_len > 20) {
                    for (int k = 0; k < 17; k++) display_name[k] = fs_table[f].name[k];
                    display_name[17] = '.'; display_name[18] = '.'; display_name[19] = '.'; display_name[20] = '\0';
                } else strcpy(display_name, fs_table[f].name);

                const char* n = display_name;
                while (*n) line[idx++] = *n++;
                while (idx < 24) line[idx++] = ' ';

                if (fs_table[f].is_dir) {
                    const char* d_str = "<DIR>"; while (*d_str) line[idx++] = *d_str++;
                    while (idx < 36) line[idx++] = ' ';
                } else {
                    char sz_buf[16]; int_to_str(fs_table[f].size, sz_buf); int s_ptr = 0;
                    while (sz_buf[s_ptr]) line[idx++] = sz_buf[s_ptr++];
                    line[idx++] = 'B';
                    while (idx < 36) line[idx++] = ' ';
                }

                const char* fs_tag = fs_table[f].is_dir ? "DIR" : (fs_table[f].is_vfat ? "VFAT/FAT32" : "VFS");
                while (*fs_tag) line[idx++] = *fs_tag++;
                line[idx] = '\0';
                term_add_line(line);
            }
        }
        term_add_line("@cpu, @ram, @gpu, @fb0, @vfs         SYNTHETIC NODES");
    }
    else if (strcmp(cmd, "cat") == 0) term_add_line("Usage: cat <filename|@node>");
    else if (strncmp(cmd, "cat ", 4) == 0) {
        const char* target = cmd + 4;
        const char* data = fs_read_file(target);
        if (data) {
            term_add_line(data);
        } else {
            term_add_line("cat: File not found or target is a directory");
        }
    }
    else if (strcmp(cmd, "uname") == 0) term_add_line("RoxiP 0.1-RELEASE armv7l monolithic ARM");
    else if (strcmp(cmd, "clear") == 0) term_clear_history();
    else if (strcmp(cmd, "panic") == 0) kernel_panic("Manual Panic Triggered from perf Terminal");
    else if (strcmp(cmd, "lsblk") == 0) execute_lsblk();
    else if (strcmp(cmd, "zcatcher") == 0) execute_zcatcher();
    else if (strcmp(cmd, "reboot") == 0) system_reboot();
    else if (strcmp(cmd, "shutdown") == 0 || strcmp(cmd, "poweroff") == 0) system_shutdown();
    else if (strcmp(cmd, "pwd") == 0) {
        char pwd_line[80] = "Path: "; strcpy(pwd_line + 6, current_dir); term_add_line(pwd_line);
    }
    else if (strcmp(cmd, "cd") == 0 || strncmp(cmd, "cd ", 3) == 0) {
        const char* target = (strncmp(cmd, "cd ", 3) == 0) ? cmd + 3 : "/";
        if (strcmp(target, "..") == 0 || strcmp(target, "/") == 0) {
            strcpy(current_dir, "/"); term_add_line("FS: Current directory set to [/]");
        } else {
            int found = 0;
            for (int f = 0; f < fs_file_count; f++) {
                if (fs_table[f].used && fs_table[f].is_dir && strcmp(fs_table[f].name, target) == 0) {
                    strcpy(current_dir, target); found = 1;
                    char msg[80] = "FS: Directory changed to /"; strcpy(msg + 26, target); term_add_line(msg); break;
                }
            }
            if (!found) term_add_line("cd: No such directory");
        }
    }
    else if (strcmp(cmd, "help") == 0) {
        term_add_line("desk, fetch, zcatcher, lsblk, comcow, ztop, soundtest, mem, rgbdemo, ls, cd, pwd, cat, micro, mkdir, stat, xxd, disasm, ecr, set, unset, env, clear, reboot, shutdown, panic");
    }
    else term_add_line("perf: command not found. Type 'help'");
}

void process_char_input(char c) {
    if (c == '\r') c = '\n';

    /* Handling input for micro Editor Mode */
    if (current_mode == MODE_MICRO_EDITOR) {
        /* Ctrl+X (24), Ctrl+Q (17), or ESC (27) -> Exit Editor */
        if (c == 24 || c == 17 || c == 27) {
            current_mode = MODE_PERF_CLI;
            term_add_line("micro: Exited text editor.");
            redraw_current_view();
            return;
        }
        /* Ctrl+S (19) or Ctrl+O (15) -> Save file to VFS */
        if (c == 19 || c == 15) {
            if (micro_filename[0] != '\0') {
                if (fs_write_file(micro_filename, micro_buffer)) {
                    term_add_line("micro: File saved successfully.");
                } else {
                    term_add_line("micro Error: Failed to save file.");
                }
            }
            redraw_current_view();
            return;
        }
        /* Backspace / Delete */
        if (c == '\b' || c == 127) {
            if (micro_pos > 0) {
                micro_pos--;
                micro_buffer[micro_pos] = '\0';
            }
            redraw_current_view();
            return;
        }
        /* Newline */
        if (c == '\n') {
            if (micro_pos < MAX_FILE_CONTENT - 1) {
                micro_buffer[micro_pos++] = '\n';
                micro_buffer[micro_pos] = '\0';
            }
            redraw_current_view();
            return;
        }
        /* Printable characters */
        if (c >= 32 && c <= 126) {
            if (micro_pos < MAX_FILE_CONTENT - 1) {
                micro_buffer[micro_pos++] = c;
                micro_buffer[micro_pos] = '\0';
            }
            redraw_current_view();
            return;
        }
        return;
    }

    /* Existing modes handling */
    if ((c == 27 || c == 'q' || c == 'Q') && (current_mode == MODE_FLOATCUBE || current_mode == MODE_ZTOP)) {
        current_mode = MODE_PERF_CLI; redraw_current_view(); return;
    }

    if (current_mode == MODE_DESK && active_tab == 1) {
        if (c == '1') { current_theme = THEME_DEFAULT; redraw_current_view(); return; }
        else if (c == '2') { current_theme = THEME_MAC_MONO; redraw_current_view(); return; }
    }

    if (c == KEY_F1) {
        if (current_mode == MODE_DESK) current_mode = MODE_PERF_CLI;
        else current_mode = MODE_DESK;
        redraw_current_view(); return;
    }

    if (c == KEY_UP) { if (mouse_y > 0) mouse_y--; redraw_current_view(); return; }
    if (c == KEY_DOWN) { if (mouse_y < VGA_HEIGHT - 1) mouse_y++; redraw_current_view(); return; }
    if (c == KEY_LEFT) { if (mouse_x > 0) mouse_x--; redraw_current_view(); return; }
    if (c == KEY_RIGHT) { if (mouse_x < VGA_WIDTH - 1) mouse_x++; redraw_current_view(); return; }

    if (c == '\t') { active_tab = (active_tab + 1) % 3; if (current_mode == MODE_DESK) render_desktop(); return; }

    if (c == '\n') {
        input_buffer[input_pos] = '\0';
        if (current_mode == MODE_DESK) {
            if (active_tab == 2) { current_mode = MODE_PERF_CLI; active_tab = 0; }
        } else execute_command(input_buffer);
        input_pos = 0; redraw_current_view();
    } else if (c == '\b' || c == 127) {
        if (input_pos > 0) { input_pos--; redraw_current_view(); }
    } else if (c >= 32 && c <= 126 && input_pos < 72) {
        input_buffer[input_pos++] = c; redraw_current_view();
    }
}
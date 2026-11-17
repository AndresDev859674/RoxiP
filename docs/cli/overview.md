# RoxiP CLI & Shell

## Shell (`src/shell.c`)

The shell runs in `MODE_PERF_CLI` and processes commands entered at the `perf >` prompt.

### Built-in Commands

| Command | Description |
|---------|-------------|
| `help` | Show available commands |
| `ps` | List running processes |
| `lsblk` | List block devices (ram, heap, fb, yafs, flash) |
| `stat <file>` | Show file statistics |
| `xxd <file>` | Hex dump a file |
| `disasm <addr\|file>` | ARM32 mini-disassembler (ARM only) |
| `sh <script.rsh>` / `rsh <script>` | Run a RoxiP shell script |
| `clear` | Clear the terminal |
| `reboot` | Reboot the system |
| `shutdown` | Power off the system |

### Environment Variables

The shell maintains an environment table (`env_table[MAX_ENV_VARS]`) with `env_set()` / `env_unset()`. Default variables:
- `OS` = "RoxiP"
- `SHELL` = "perf"

### History

Command history is saved to `history.txt` in YAFS via `save_history_to_yafs()`.

### Input Processing

`process_char_input()` handles:
- Character accumulation in `input_buffer`
- Enter key to execute commands
- Backspace for deletion
- Arrow keys for navigation

## Host CLI Tool (`src/cli/roxip-cli.c`)

A host-side (Linux/macOS) utility for creating YAFS storage images:

```bash
./roxip-cli create roxip.storage 64   # Create 64MB storage image
```

Built natively (not cross-compiled) with `-Dbuild-cli=true` in Meson or `make cli`.

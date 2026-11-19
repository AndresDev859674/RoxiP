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
| `echo <text>` | Print text; `echo text > file` writes a YAFS file |
| `echo @net:name "SSID"` | Set the Wi-Fi SSID node |
| `echo @net:pass "key"` | Set the Wi-Fi password node |
| `net` | Show network status (SSID, hidden password, ID, link state) — networking builds only |
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
./roxip-cli write roxip.storage @net:name "MyWiFi"
./roxip-cli write roxip.storage @net:pass "MyPassword123"
./roxip-cli read roxip.storage @net:id    # Written by RoxiP after boot connect
```

The host tool understands both YAFS header slots (primary and backup), and
`@net:id` is read-only: attempts to write or remove it are rejected. Its
`file_record_t` layout must stay in sync with `src/include/kernel.h`.

Built natively (not cross-compiled) with `-Dbuild-cli=true` in Meson or `make cli`.

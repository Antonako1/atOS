# BATSH – Better AT Shell for atOS

BATSH is a lightweight batch shell for the atOS operating system. It allows both interactive command execution and the running of `.SH` script files, providing familiar batch-like scripting capabilities, with some additional improvements and additions

---

## Features

- **Interactive shell commands**: Execute built-in commands directly in the shell.
- **Batch script execution**: Run `.SH` script files line by line.
- **Line parsing**:
  - Ignores comments (`#`, `;`, `rem`).
  - Supports `@ECHO ON/OFF`.
  - Supports `PAUSE` command.
- **Command echoing**: Control output visibility with `@ECHO`.
- **Directory navigation**: `cd`, `cd..`, `dir`.
- **File system manipulation**: `mkdir`, `rmdir`.
- **Audio control**: `tone`, `soundoff`.
- **Help system**: `help` lists all commands with descriptions.

---

## Built-in Commands

| Command     | Description |
|------------|-------------|
| help       | Show this help message |
| clear / cls | Clear the screen |
| version    | Show shell version |
| exit       | Exit the shell |
| echo       | Echo text to screen |
| tone       | Play a tone (`tone <freqHz> <ms> [amp] [rate]`) |
| soundoff   | Stop AC97 audio playback |
| cd         | Change directory |
| cd..       | Move back one directory |
| dir        | List directory contents |
| mkdir      | Create a directory |
| rmdir      | Remove a directory |

---

## Script Syntax

- **Comments**: Lines starting with `#`, `;`, or `rem`.
- **ECHO control**:  
```
@ECHO ON
@ECHO OFF
```

- **Pause execution**:  
```
PAUSE
```

- **Commands**: Any line not a comment or special instruction is treated as a shell command.

---

## Running a Script in shell

```c
#include <PROGRAMS/SHELL/BATSH.h>

RUN_BATSH_SCRIPT("myscript.sh");
```

* `.SH` files are parsed line by line.
* Lines are trimmed of CR/LF characters.
* Each line is processed through the BATSH parser.

---

## Integration

* BATSH is fully integrated with the atOS shell command system.
* Commands executed in BATSH scripts go through `HANDLE_COMMAND()` ensuring consistent behavior between interactive and scripted execution.

---

## Notes

* Scripts must have a `.SH` extension.
* `@ECHO OFF` disables line output during script execution.
* The shell supports basic command parsing, but does not currently support variables or complex control flow.

---

## Future Improvements

* Add variable substitution (e.g., `%USER%`, `%CD%`).
* Support for conditional statements and loops.
* Enhanced error reporting for commands and scripts.


# atOShell

A simple shell program for the atOS operating system.  
It provides a command-line interface for users to interact with the system, execute commands, and manage files.

## Features
- Command execution
- File management
- Basic scripting capabilities
- Command history (↑ / ↓)
- Cursor-based line editing
- Insert and overwrite mode
- Customizable prompt

## Example commands

- `help` — Lists available commands  
- `clear` / `cls` — Clears the screen  
- `echo <text>` — Prints the provided text  
- `tone <freqHz> <ms> [amp] [rate]` — Plays a square tone through AC'97  
  - Example: `tone 880 300 8000 48000`  
  - Returns an error if AC'97 is not available  
- `soundoff` — Stops AC'97 playback  
- `version` — Shows shell version  
- `exit` — Exits the shell (placeholder)

## Keyboard Shortcuts

| Key / Combo            | Action |
|-------------------------|--------|
| **Enter**               | Executes the current command |
| **Backspace**           | Deletes the character before the cursor |
| **Delete**              | Deletes the character under the cursor |
| **Insert**              | Toggles insert/overwrite mode |
| **Arrow Left / Right**  | Move cursor left or right by one character |
| **Arrow Up / Down**     | Cycle through command history |
| **Home**                | Move cursor to the beginning of the line |
| **End**                 | Move cursor to the end of the line |
| **Ctrl + A**            | Move cursor to the beginning of the line |
| **Ctrl + E**            | Move cursor to the end of the line |
| **Ctrl + U**            | Delete text from cursor to beginning of line |
| **Ctrl + K**            | Delete text from cursor to end of line |
| **Ctrl + W**            | Delete the previous word |
| **Ctrl + Y**            | Yank (paste) the last deleted text |
| **Ctrl + L**            | Clear the screen and redraw prompt |
| **Ctrl + Left / Right** | Move cursor one word left or right |
| **Ctrl + C**            | Cancel current input / running command |
| **Ctrl + Tab**          | Reserved for future use (e.g., tab completion) |
| **Shift + Alt + Del**   | Reserved (potential system interrupt or reset) |

## Important caveats
- Not an ordinary process — This shell is a **special process** that manages other processes and provides a user interface.
- Designed to run **alongside the kernel** and relies on kernel services.
- Intended for **educational and development purposes**, not a production-grade shell.
- Some operations may directly affect system stability or memory.
- Always use caution when executing system-level commands.

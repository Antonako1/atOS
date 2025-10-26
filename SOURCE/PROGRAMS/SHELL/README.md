# atOShell

A simple shell program for the atOS operating system.
It provides a command-line interface for users to interact with the system, execute commands, and manage files.

## Features
- Command execution
- File management
- Basic scripting capabilities
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

## Important caveats
- Not ordinary process — This shell is a special process that manages other processes and provides a user interface.
- This shell is designed to run alongside the kernel and relies on its services.
- The shell is intended for educational purposes and may not include all features of a full-fledged shell.
- Users should be cautious when executing commands, as some operations may affect system stability or security.


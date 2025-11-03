# BATSH – Better AT Shell for atOS

BATSH is a lightweight shell for the atOS operating system.
It is **batch-like, but not batch**—designed for both interactive command execution and script files (`.SH`), with simple syntax and added features.

---

## Features

* **Interactive commands**: Execute built-in commands directly in the shell.
* **Script execution**: Run `.SH` scripts line by line.
* **Comments**: Lines starting with `#` or `rem` are ignored.
* **Command stacking**: Separate multiple commands on a line with `;`.
* **Variables**: Define and use variables with `@var=value` or `@{var}`.
* **Arithmetic**: Supported only inside `IF` statements, `LOOP` statements, or variable assignments.
* **File system navigation and manipulation**: `cd`, `cd..`, `dir`, `mkdir`, `rmdir`.
* **Audio control**: `tone`, `soundoff`.
* **Help system**: `help` lists commands and descriptions.

---

## Built-in Commands

| Command     | Description                                     |
| ----------- | ----------------------------------------------- |
| help        | Show this help message                          |
| cls         | Clear the screen                                |
| version     | Show shell version                              |
| exit        | Exit the shell                                  |
| echo        | Echo text to screen                             |
| tone        | Play a tone (`tone <freqHz> <ms> [amp] [rate]`) |
| soundoff    | Stop AC97 audio playback                        |
| cd          | Change directory                                |
| cd..        | Move back one directory                         |
| dir         | List directory contents                         |
| mkdir       | Create a directory                              |
| rmdir       | Remove a directory                              |
| ...         | ...                                             |

---

## Script Syntax

### Comments

```bat
# This is a comment
rem This is also a comment
```

### Commands

```bat
dir
echo Hello World
```

* Commands can be stacked using `;`:

```bat
cd TEST; dir;
```

### Variables

```bat
@var=value
echo @{var}
```

* Advanced variable substitution (`@{CD}`, `@{PATH}`, `@{HOME}`, `@{DOCS}`).

### Arithmetic Expressions

* Only evaluated inside `IF`, `LOOP`, or variable assignments:

```bat
@count = 5 + 3

IF 1 EQU 2 THEN
    echo "Not equal"
FI

LOOP 1 TO 10
    echo "Iteration"
END
```

* Outside these contexts, `+`, `-`, `*`, `/` are treated as literal characters.

---

## Notes

* Scripts must have a `.SH` extension.
* Command stacking with `;` is supported.
* Reserved tokens for future: `|`, `>`, `<`.

---

## Future Improvements

* Full support for conditional statements and loops.
* Enhanced error reporting.
* Pipelines and redirection.

---

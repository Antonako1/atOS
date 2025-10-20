# atOShell

A simple shell program for the atOS operating system.
It provides a command-line interface for users to interact with the system, execute commands, and manage files.

## Features
- Command execution
- File management
- Basic scripting capabilities
- Customizable prompt

## Commands

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

## Rendering and Performance

The shell uses a software text renderer over the VBE framebuffer. As of this change:

- Batched spans: text output is batched by span instead of per-character pixels. A span redraws with a single background fill followed by merged horizontal line runs for glyph foreground bits.
- Fast path API:
  - `DRAW_STRING_AT(col, row, s, len, fg, bg)` — draws `len` characters from `s` starting at cell `(col,row)` using an 8x16 font.
  - `WRITE_SPAN_FAST(s, len)` — writes `len` bytes from `s` into the internal `text_buffer` starting at the current cursor, handling wrapping/scrolling and issuing a single redraw per chunk.
- Cursor and spacing: cell advance equals `CHAR_WIDTH + CHAR_SPACING`. All rendering uses this exact advance so the cursor aligns with text at any column.
- Edits and scrolling: insert/delete shift an entire line in the text buffer and redraw the affected region as one span.

Notes:

- `PUTS` now batches consecutive printable bytes and falls back to `PUTC` only for control characters (e.g., `\n`, `\r`, `\t`). Tabs are inserted as a batched group of spaces.
- `DRAW_TEXT_BUFFER` repaints per row using a single span.
- Keep `CHAR_SPACING` consistent with other console geometry; mismatches will misalign the cursor.

Troubleshooting:

- If the cursor appears to drift relative to text, verify both `COL_TO_PIX`/`ROW_TO_PIX` and the fast-path span logic use the same cell advance. The expected advance is `CHAR_WIDTH + CHAR_SPACING` horizontally and `CHAR_HEIGHT + CHAR_SPACING` vertically.

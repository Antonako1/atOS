# ASTRAC.BIN -A /HOME/SRC/BOOTLOADER/VBR.ASM -o /HOME/SRC/BOOTLOADER/VBR.BIN -v

# wait 3

# IF @{ERRORLEVEL} NEQ 0 THEN
    # echo "Error: Failed to build VBR.ASM. AstraC exited with @{ERRORLEVEL}."
# ELSE
    # echo "Successfully built VBR.ASM"
    # shuttle -p 2 -s -f /HOME/SRC/BOOTLOADER/VBR.BIN
# ENDIF
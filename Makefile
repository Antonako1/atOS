
ASSEMBLER     ?= nasm
SOURCE_DIR    ?= SOURCE
SOURCE_KERNEL_DIR ?= $(SOURCE_DIR)/KERNEL
SOURCE_BOOTLOADER_DIR ?= $(SOURCE_DIR)/BOOTLOADER
OUTPUT_DIR    ?= OUTPUT
OUTPUT_KERNEL_DIR ?= $(OUTPUT_DIR)/KERNEL
OUTPUT_BOOTLOADER_DIR ?= $(OUTPUT_DIR)/BOOTLOADER
OUTPUT_ISO_DIR ?= $(OUTPUT_DIR)/ISO
OUTPUT_USER_PROGRAMS_DIR ?= $(OUTPUT_DIR)/PROGRAMS
INPUT_ISO_DIR  ?= ISO_DIR
ISO_NAME      ?= atOS.iso
SOURCE_ROOT_USER_PROGRAMS_DIR := $(SOURCE_DIR)/PROGRAMS
PROGRAM_LIST_FILE := $(SOURCE_ROOT_USER_PROGRAMS_DIR)/USER_PROGRAMS.txt
KRNL_INCLUDES ?= \
	-I$(SOURCE_DIR)/ \
	-I$(SOURCE_KERNEL_DIR)/32RTOSKRNL/DRIVERS/VIDEO/ \
	-I$(SOURCE_KERNEL_DIR)/32RTOSKRNL/DRIVERS/DISK/ \
	-I$(SOURCE_KERNEL_DIR)/32RTOSKRNL/CPU/ \
	-I$(SOURCE_KERNEL_DIR)/32RTOSKRNL/CPU/GDT/ \
	-I$(SOURCE_KERNEL_DIR)/32RTOSKRNL/CPU/IDT/ \
	-I$(SOURCE_KERNEL_DIR)/32RTOSKRNL/CPU/ISR/ \
	-I$(SOURCE_KERNEL_DIR)/32RTOSKRNL/CPU/IRQ/ \
	-I$(SOURCE_KERNEL_DIR)/32RTOSKRNL/MEMORY/ \
	-I$(SOURCE_KERNEL_DIR)/32RTOSKRNL/CPU/PIT/ \

RTOSKRNL_INCLUDES ?= \
	-I$(SOURCE_DIR)/ \
	-I$(SOURCE_KERNEL_DIR)/32RTOSKRNL/DRIVERS/ \
	-I$(SOURCE_KERNEL_DIR)/32RTOSKRNL/ \
	-I$(SOURCE_KERNEL_DIR)/32RTOSKRNL/DRIVERS/ \
	-I$(SOURCE_KERNEL_DIR)/32RTOSKRNL/CPU/ \
	-I$(SOURCE_KERNEL_DIR)/32RTOSKRNL/MEMORY/ \
	-I$(SOURCE_KERNEL_DIR)/32RTOSKRNL/DRIVERS/ \
	-I$(SOURCE_KERNEL_DIR)/32RTOSKRNL/FS/ \
	-I$(SOURCE_KERNEL_DIR)/32RTOSKRNL/RTOSKRNL/


CComp         ?= gcc
CompArgs 	  ?= -Wno-comment -Wno-unused-variable -Wno-pointer-sign -Wno-comments -m32 -ffreestanding -fno-pic -fno-pie -nostdlib -O0 -Wall -Wextra -fno-stack-protector -fno-builtin -fno-inline
KRNLCompArgs  ?= $(CompArgs) $(KRNL_INCLUDES) -DKERNEL_ENTRY
RTOSKRNLCompArgs ?= $(CompArgs) $(RTOSKRNL_INCLUDES) -DRTOS_KERNEL -D__32RTOS__ -D__RTOS__ -D__KERNEL__


INPUT_ISO_DIR_SYSTEM ?= $(INPUT_ISO_DIR)/ATOS
INPUT_ISO_DIR_HOME ?= $(INPUT_ISO_DIR)/HOME
INPUT_ISO_DIR_PROGRAMS ?= $(INPUT_ISO_DIR)/PROGRAMS

.PHONY: all kernel bootloader iso clean run help programs diskvbr clean_tap runn setup_tap
.PHONY: run_user
.PHONY: run_user_gui

# Default target
all: iso

# Bootloader build (16-bit raw binary)
# Bootloader build (16-bit raw binary)
$(OUTPUT_BOOTLOADER_DIR)/BOOTLOADER.BIN: $(SOURCE_BOOTLOADER_DIR)/BOOTLOADER.asm
	@echo "Compiling bootloader (16-bit real mode)..."
	mkdir -p $(OUTPUT_BOOTLOADER_DIR)
	$(ASSEMBLER) -f bin -D__REAL_MODE__ -o $@ $<
	@echo "Bootloader compiled successfully."

bootloader: $(OUTPUT_BOOTLOADER_DIR)/BOOTLOADER.BIN


# Disk VBR build (Volume Boot Record for FAT)
$(OUTPUT_BOOTLOADER_DIR)/DISK_VBR.BIN: $(SOURCE_BOOTLOADER_DIR)/DISK_VBR.asm
	@echo "Compiling disk VBR (16-bit real mode)..."
	mkdir -p $(OUTPUT_BOOTLOADER_DIR)
	$(ASSEMBLER) -f bin -D__REAL_MODE__ -o $@ $<
	@echo "Disk VBR compiled successfully."

diskvbr: $(OUTPUT_BOOTLOADER_DIR)/DISK_VBR.BIN

kernel: 
	@echo "Compiling KERNEL.BIN (16-bit real mode second stage)..."
	mkdir -p $(OUTPUT_KERNEL_DIR)
	rm -rf $(OUTPUT_KERNEL_DIR)
	mkdir -p $(OUTPUT_KERNEL_DIR)
	$(ASSEMBLER) -f bin -D__REAL_PROTECTED_MODE__ \
		$(SOURCE_KERNEL_DIR)/KERNEL_ENTRY.asm \
		-o $(OUTPUT_KERNEL_DIR)/KERNEL.BIN
	@echo "KERNEL.BIN compiled successfully."
	@size=$$(stat -c%s "$(OUTPUT_KERNEL_DIR)/KERNEL.BIN"); \
	if [ $$size -gt 4095 ]; then \
		echo "\033[1;33mWARNING: KERNEL.BIN size is $$size bytes, which exceeds 4095 bytes!\033[0m"; \
		exit 1; \
	fi

	@echo "Compiling KRNL.BIN (32-bit protected mode C kernel)..."
	mkdir -p $(OUTPUT_KERNEL_DIR)
	$(CComp) $(KRNLCompArgs) -c $(SOURCE_KERNEL_DIR)/KERNEL.c -o $(OUTPUT_KERNEL_DIR)/KERNEL.o -m32
	$(CComp) $(KRNLCompArgs) -c $(SOURCE_KERNEL_DIR)/32RTOSKRNL/DRIVERS/VIDEO/VESA.c -o $(OUTPUT_KERNEL_DIR)/VESA.o
	$(CComp) $(KRNLCompArgs) -c $(SOURCE_KERNEL_DIR)/32RTOSKRNL/DRIVERS/VIDEO/VBE.c -o $(OUTPUT_KERNEL_DIR)/VBE.o
	$(CComp) $(KRNLCompArgs) -c $(SOURCE_KERNEL_DIR)/32RTOSKRNL/CPU/GDT/GDT.c -o $(OUTPUT_KERNEL_DIR)/GDT.o
	$(CComp) $(KRNLCompArgs) -c $(SOURCE_KERNEL_DIR)/32RTOSKRNL/CPU/IDT/IDT.c -o $(OUTPUT_KERNEL_DIR)/IDT.o
	$(CComp) $(KRNLCompArgs) -c $(SOURCE_KERNEL_DIR)/32RTOSKRNL/CPU/ISR/ISR.c -o $(OUTPUT_KERNEL_DIR)/ISR.o
	$(CComp) $(KRNLCompArgs) -c $(SOURCE_KERNEL_DIR)/32RTOSKRNL/CPU/IRQ/IRQ.c -o $(OUTPUT_KERNEL_DIR)/IRQ.o
	$(CComp) $(KRNLCompArgs) -c $(SOURCE_KERNEL_DIR)/32RTOSKRNL/CPU/INTERRUPTS/INTERRUPTS.c -o $(OUTPUT_KERNEL_DIR)/INTERRUPTS.o
	$(CComp) $(KRNLCompArgs) -c $(SOURCE_KERNEL_DIR)/32RTOSKRNL/DRIVERS/ATAPI/ATAPI.c -o $(OUTPUT_KERNEL_DIR)/ATA_ATAPI.o
	$(CComp) $(KRNLCompArgs) -c $(SOURCE_KERNEL_DIR)/32RTOSKRNL/CPU/PIC/PIC.c -o $(OUTPUT_KERNEL_DIR)/PIC.o

# Link all object files into KRNL.BIN
	
	$(CComp) -m32 -nostdlib -ffreestanding \
		-Wl,-T,$(SOURCE_KERNEL_DIR)/kernel.ld,-e,_start,--oformat=binary \
		-o $(OUTPUT_KERNEL_DIR)/KRNL.BIN \
		$(OUTPUT_KERNEL_DIR)/KERNEL.o \
		$(OUTPUT_KERNEL_DIR)/VESA.o \
		$(OUTPUT_KERNEL_DIR)/VBE.o \
		$(OUTPUT_KERNEL_DIR)/GDT.o \
		$(OUTPUT_KERNEL_DIR)/IDT.o \
		$(OUTPUT_KERNEL_DIR)/ISR.o \
		$(OUTPUT_KERNEL_DIR)/IRQ.o \
		$(OUTPUT_KERNEL_DIR)/INTERRUPTS.o \
		$(OUTPUT_KERNEL_DIR)/ATA_ATAPI.o \
		$(OUTPUT_KERNEL_DIR)/PIC.o \

	@echo "KRNL.BIN compiled successfully."
	@size=$$(stat -c%s "$(OUTPUT_KERNEL_DIR)/KRNL.BIN"); \
	if [ $$size -gt 16384 ]; then \
		echo "\033[1;33mWARNING: KRNL.BIN size is $$size bytes, which exceeds 16384 bytes!\033[0m"; \
		exit 1; \
	fi


	# Remove kernel entry object files
	rm -f $(OUTPUT_KERNEL_DIR)/*.o

	@echo "Compiling 32RTOSKRNL.BIN (32-bit asm kernel)..."
	$(CComp) $(RTOSKRNLCompArgs) -c $(SOURCE_KERNEL_DIR)/RTOSKRNL.c -o $(OUTPUT_KERNEL_DIR)/RTOSKRNL.o -m32
	$(CComp) $(RTOSKRNLCompArgs) -c $(SOURCE_KERNEL_DIR)/32RTOSKRNL/DRIVERS/VIDEO/VESA.c -o $(OUTPUT_KERNEL_DIR)/VESA.o
	$(CComp) $(RTOSKRNLCompArgs) -c $(SOURCE_KERNEL_DIR)/32RTOSKRNL/DRIVERS/VIDEO/VBE.c -o $(OUTPUT_KERNEL_DIR)/VBE.o
	$(CComp) $(RTOSKRNLCompArgs) -c $(SOURCE_KERNEL_DIR)/32RTOSKRNL/DRIVERS/PS2/KEYBOARD.c -o $(OUTPUT_KERNEL_DIR)/PS2_KEYBOARD.o
	$(CComp) $(RTOSKRNLCompArgs) -c $(SOURCE_KERNEL_DIR)/32RTOSKRNL/DRIVERS/ATAPI/ATAPI.c -o $(OUTPUT_KERNEL_DIR)/ATA_ATAPI.o
	$(CComp) $(RTOSKRNLCompArgs) -c $(SOURCE_KERNEL_DIR)/32RTOSKRNL/DRIVERS/ATA_PIIX3/ATA_PIIX3.c -o $(OUTPUT_KERNEL_DIR)/ATA_PIIX3.o
	$(CComp) $(RTOSKRNLCompArgs) -c $(SOURCE_KERNEL_DIR)/32RTOSKRNL/DRIVERS/ATA_PIO/ATA_PIO.c -o $(OUTPUT_KERNEL_DIR)/ATA_PIO.o
	$(CComp) $(RTOSKRNLCompArgs) -c $(SOURCE_KERNEL_DIR)/32RTOSKRNL/DRIVERS/CMOS/CMOS.c -o $(OUTPUT_KERNEL_DIR)/CMOS.o
	$(CComp) $(RTOSKRNLCompArgs) -c $(SOURCE_KERNEL_DIR)/32RTOSKRNL/DRIVERS/RTL8139/RTL8139.c -o $(OUTPUT_KERNEL_DIR)/RTL8139.o
	$(CComp) $(RTOSKRNLCompArgs) -c $(SOURCE_KERNEL_DIR)/32RTOSKRNL/DRIVERS/AC97/AC97.c -o $(OUTPUT_KERNEL_DIR)/AC97.o
	$(CComp) $(RTOSKRNLCompArgs) -c $(SOURCE_KERNEL_DIR)/32RTOSKRNL/DRIVERS/BEEPER/BEEPER.c -o $(OUTPUT_KERNEL_DIR)/BEEPER.o
	$(CComp) $(RTOSKRNLCompArgs) -c $(SOURCE_KERNEL_DIR)/32RTOSKRNL/DEBUG/KDEBUG.c -o $(OUTPUT_KERNEL_DIR)/KDEBUG.o
	$(CComp) $(RTOSKRNLCompArgs) -c $(SOURCE_KERNEL_DIR)/32RTOSKRNL/DRIVERS/PCI/PCI.c -o $(OUTPUT_KERNEL_DIR)/PCI.o
	$(CComp) $(RTOSKRNLCompArgs) -c $(SOURCE_KERNEL_DIR)/32RTOSKRNL/CPU/PIT/PIT.c -o $(OUTPUT_KERNEL_DIR)/PIT.o
	$(CComp) $(RTOSKRNLCompArgs) -c $(SOURCE_KERNEL_DIR)/32RTOSKRNL/CPU/GDT/GDT.c -o $(OUTPUT_KERNEL_DIR)/GDT.o
	$(CComp) $(RTOSKRNLCompArgs) -c $(SOURCE_KERNEL_DIR)/32RTOSKRNL/CPU/IDT/IDT.c -o $(OUTPUT_KERNEL_DIR)/IDT.o
	$(CComp) $(RTOSKRNLCompArgs) -c $(SOURCE_KERNEL_DIR)/32RTOSKRNL/CPU/ISR/ISR.c -o $(OUTPUT_KERNEL_DIR)/ISR.o
	$(CComp) $(RTOSKRNLCompArgs) -c $(SOURCE_KERNEL_DIR)/32RTOSKRNL/CPU/IRQ/IRQ.c -o $(OUTPUT_KERNEL_DIR)/IRQ.o
	$(CComp) $(RTOSKRNLCompArgs) -c $(SOURCE_KERNEL_DIR)/32RTOSKRNL/CPU/PIC/PIC.c -o $(OUTPUT_KERNEL_DIR)/PIC.o
	$(CComp) $(RTOSKRNLCompArgs) -c $(SOURCE_KERNEL_DIR)/32RTOSKRNL/CPU/FPU/FPU.c -o $(OUTPUT_KERNEL_DIR)/FPU.o
	$(CComp) $(RTOSKRNLCompArgs) -c $(SOURCE_KERNEL_DIR)/32RTOSKRNL/CPU/SYSCALL/SYSCALL.c -o $(OUTPUT_KERNEL_DIR)/SYSCALL.o
	$(CComp) $(RTOSKRNLCompArgs) -c $(SOURCE_KERNEL_DIR)/32RTOSKRNL/CPU/INTERRUPTS/INTERRUPTS.c -o $(OUTPUT_KERNEL_DIR)/INTERRUPTS.o
	$(CComp) $(RTOSKRNLCompArgs) -c $(SOURCE_KERNEL_DIR)/32RTOSKRNL/MEMORY/E820/E820.c -o $(OUTPUT_KERNEL_DIR)/E820.o
	$(CComp) $(RTOSKRNLCompArgs) -c $(SOURCE_KERNEL_DIR)/32RTOSKRNL/MEMORY/PAGING/PAGING.c -o $(OUTPUT_KERNEL_DIR)/PAGING.o
	$(CComp) $(RTOSKRNLCompArgs) -c $(SOURCE_KERNEL_DIR)/32RTOSKRNL/MEMORY/PAGEFRAME/PAGEFRAME.c -o $(OUTPUT_KERNEL_DIR)/PAGEFRAME.o
	$(CComp) $(RTOSKRNLCompArgs) -c $(SOURCE_KERNEL_DIR)/32RTOSKRNL/MEMORY/HEAP/KHEAP.c -o $(OUTPUT_KERNEL_DIR)/KHEAP.o
	$(CComp) $(RTOSKRNLCompArgs) -c $(SOURCE_KERNEL_DIR)/32RTOSKRNL/MEMORY/BYTEMAP/BYTEMAP.c -o $(OUTPUT_KERNEL_DIR)/BYTEMAP.o
	$(CComp) $(RTOSKRNLCompArgs) -c $(SOURCE_KERNEL_DIR)/32RTOSKRNL/FS/ISO9660/ISO9660.c -o $(OUTPUT_KERNEL_DIR)/ISO9660.o
	$(CComp) $(RTOSKRNLCompArgs) -c $(SOURCE_KERNEL_DIR)/32RTOSKRNL/FS/FAT/FAT.c -o $(OUTPUT_KERNEL_DIR)/FAT32.o
	$(CComp) $(RTOSKRNLCompArgs) -c $(SOURCE_DIR)/STD/MEM.c -o $(OUTPUT_KERNEL_DIR)/MEM.o
	$(CComp) $(RTOSKRNLCompArgs) -c $(SOURCE_DIR)/STD/STRING.c -o $(OUTPUT_KERNEL_DIR)/STRING.o
	$(CComp) $(RTOSKRNLCompArgs) -c $(SOURCE_DIR)/STD/MATH.c -o $(OUTPUT_KERNEL_DIR)/MATH.o
	$(CComp) $(RTOSKRNLCompArgs) -c $(SOURCE_KERNEL_DIR)/32RTOSKRNL/RTOSKRNL/ACPI/ACPI.c -o $(OUTPUT_KERNEL_DIR)/ACPI.o
	$(CComp) $(RTOSKRNLCompArgs) -c $(SOURCE_KERNEL_DIR)/32RTOSKRNL/RTOSKRNL/ERROR/ERROR.c -o $(OUTPUT_KERNEL_DIR)/ERROR.o	
	$(CComp) $(RTOSKRNLCompArgs) -c $(SOURCE_KERNEL_DIR)/32RTOSKRNL/RTOSKRNL/RTOSKRNL_INTERNAL.c -o $(OUTPUT_KERNEL_DIR)/RTOSKRNL_INTERNAL.o
	$(CComp) $(RTOSKRNLCompArgs) -c $(SOURCE_KERNEL_DIR)/32RTOSKRNL/RTOSKRNL/PROC/PROC.c -o $(OUTPUT_KERNEL_DIR)/PROC.o
	$(CComp) $(RTOSKRNLCompArgs) -c $(SOURCE_KERNEL_DIR)/32RTOSKRNL/NIC/NIC.c -o $(OUTPUT_KERNEL_DIR)/NIC.o
	$(CComp) $(RTOSKRNLCompArgs) -c $(SOURCE_KERNEL_DIR)/32RTOSKRNL/RTOSKRNL/BITMAP/BITMAP.c -o $(OUTPUT_KERNEL_DIR)/BITMAP.o


	$(CComp) -m32 -nostdlib -ffreestanding \
		-Wl,-T,$(SOURCE_KERNEL_DIR)/rtoskernel.ld,-e,_start,--oformat=binary \
		-o $(OUTPUT_KERNEL_DIR)/32RTOSKRNL.BIN \
		$(OUTPUT_KERNEL_DIR)/RTOSKRNL.o \
		$(OUTPUT_KERNEL_DIR)/VESA.o \
		$(OUTPUT_KERNEL_DIR)/VBE.o \
		$(OUTPUT_KERNEL_DIR)/PIC.o \
		$(OUTPUT_KERNEL_DIR)/STRING.o \
		$(OUTPUT_KERNEL_DIR)/PIT.o \
		$(OUTPUT_KERNEL_DIR)/PS2_KEYBOARD.o \
		$(OUTPUT_KERNEL_DIR)/CMOS.o \
		$(OUTPUT_KERNEL_DIR)/GDT.o \
		$(OUTPUT_KERNEL_DIR)/IDT.o \
		$(OUTPUT_KERNEL_DIR)/ISR.o \
		$(OUTPUT_KERNEL_DIR)/IRQ.o \
		$(OUTPUT_KERNEL_DIR)/INTERRUPTS.o \
		$(OUTPUT_KERNEL_DIR)/E820.o \
		$(OUTPUT_KERNEL_DIR)/PAGING.o \
		$(OUTPUT_KERNEL_DIR)/PAGEFRAME.o \
		$(OUTPUT_KERNEL_DIR)/MEM.o \
		$(OUTPUT_KERNEL_DIR)/BYTEMAP.o \
		$(OUTPUT_KERNEL_DIR)/RTOSKRNL_INTERNAL.o \
		$(OUTPUT_KERNEL_DIR)/ATA_ATAPI.o \
		$(OUTPUT_KERNEL_DIR)/ACPI.o \
		$(OUTPUT_KERNEL_DIR)/NIC.o \
		$(OUTPUT_KERNEL_DIR)/MATH.o \
		$(OUTPUT_KERNEL_DIR)/SYSCALL.o \
		$(OUTPUT_KERNEL_DIR)/ISO9660.o \
		$(OUTPUT_KERNEL_DIR)/FAT32.o \
		$(OUTPUT_KERNEL_DIR)/ERROR.o \
		$(OUTPUT_KERNEL_DIR)/KHEAP.o \
		$(OUTPUT_KERNEL_DIR)/BEEPER.o \
		$(OUTPUT_KERNEL_DIR)/PROC.o \
		$(OUTPUT_KERNEL_DIR)/FPU.o \
		$(OUTPUT_KERNEL_DIR)/RTL8139.o \
		$(OUTPUT_KERNEL_DIR)/AC97.o \
		$(OUTPUT_KERNEL_DIR)/KDEBUG.o \
		$(OUTPUT_KERNEL_DIR)/BITMAP.o \
		$(OUTPUT_KERNEL_DIR)/ATA_PIIX3.o \
		$(OUTPUT_KERNEL_DIR)/ATA_PIO.o \
		$(OUTPUT_KERNEL_DIR)/PCI.o \


	@echo "32RTOSKRNL.BIN compiled successfully."

	@echo "All kernel components compiled successfully."


programs:
	@echo "Building user programs..."
	@mkdir -p $(INPUT_ISO_DIR_PROGRAMS)
	@( \
		cat $(PROGRAM_LIST_FILE) | while read prog_dir prog_name; do \
			echo "Building $$prog_name from $$prog_dir..."; \
			cmake -S $$prog_dir -B $$prog_dir/build -G "Unix Makefiles" || exit 1; \
			cmake --build $$prog_dir/build || exit 1; \
		done \
	)
	cmake -S $(SOURCE_ROOT_USER_PROGRAMS_DIR)/SYS_PROGS -B $(SOURCE_ROOT_USER_PROGRAMS_DIR)/SYS_PROGS/build -G "Unix Makefiles"
	cmake --build $(SOURCE_ROOT_USER_PROGRAMS_DIR)/SYS_PROGS/build
	@echo "All user programs built successfully."

# ISO build
iso: bootloader kernel programs diskvbr
	@echo "Creating ISO directory structure..."
	mkdir -p $(INPUT_ISO_DIR_SYSTEM)/SYS_SRC
	mkdir -p $(INPUT_ISO_DIR_SYSTEM)
	mkdir -p $(INPUT_ISO_DIR_HOME)
	mkdir -p $(INPUT_ISO_DIR_HOME)/DOCS/
	mkdir -p $(INPUT_ISO_DIR_PROGRAMS)
	cp -f $(OUTPUT_BOOTLOADER_DIR)/BOOTLOADER.BIN $(INPUT_ISO_DIR)/BOOTLOADER.BIN
	cp -f $(OUTPUT_BOOTLOADER_DIR)/DISK_VBR.BIN $(INPUT_ISO_DIR_SYSTEM)/DISK_VBR.BIN

	cp -rf $(SOURCE_DIR)/../HOME/* $(INPUT_ISO_DIR_HOME)/
	cp -rf $(SOURCE_DIR)/FILES/* $(INPUT_ISO_DIR_HOME)/DOCS
	cp -rf $(SOURCE_DIR)/SYS_SRC/* $(INPUT_ISO_DIR_SYSTEM)/SYS_SRC
	cp -f $(OUTPUT_KERNEL_DIR)/KERNEL.BIN $(INPUT_ISO_DIR)/KERNEL.BIN
	cp -f $(OUTPUT_KERNEL_DIR)/32RTOSKRNL.BIN $(INPUT_ISO_DIR_SYSTEM)/32RTOSKRNL.BIN
	cp -f $(SOURCE_DIR)/ATOSHELL.SH $(INPUT_ISO_DIR_SYSTEM)/ATOSHELL.SH
	cp -f $(OUTPUT_KERNEL_DIR)/KRNL.BIN $(INPUT_ISO_DIR)/KRNL.BIN
	cp -r $(OUTPUT_USER_PROGRAMS_DIR)/ $(INPUT_ISO_DIR)/

	@echo "Building ISO..."
	mkdir -p $(OUTPUT_ISO_DIR)
	genisoimage -o $(OUTPUT_ISO_DIR)/$(ISO_NAME) -r -J -b BOOTLOADER.BIN -no-emul-boot $(INPUT_ISO_DIR)
	@echo "ISO created at $(OUTPUT_ISO_DIR)/$(ISO_NAME)"



# Run ISO in QEMU
run: 
	@echo "Running ISO in QEMU (user-mode net, GUI+audio)..."
	qemu-img create -f raw hdd.img 256M
	mkdir -p OUTPUT/DEBUG
	qemu-system-i386 -vga std \
	-m 1024 \
	-boot order=d \
	-cdrom $(OUTPUT_ISO_DIR)/$(ISO_NAME) \
	-drive id=cdrom,file=$(OUTPUT_ISO_DIR)/$(ISO_NAME),format=raw,if=none \
	-drive id=hd0,file=hdd.img,format=raw,if=none \
	-device piix3-ide,id=ide \
	-device ide-hd,drive=hd0,bus=ide.0 \
	-device ide-cd,drive=cdrom,bus=ide.1 \
	-device ac97,audiodev=snd0 \
	-debugcon file:OUTPUT/DEBUG/debug.log \
		-global isa-debugcon.iobase=0xe9 \
		-audiodev sdl,id=snd0 \
		-nic user,model=rtl8139,mac=52:54:00:12:34:56 \
	
# remove tap device
clean_tap:
	@if ip link show tap0 >/dev/null 2>&1; then \
	    echo "Removing tap0..."; \
	    sudo ip link set tap0 down; \
	    sudo ip tuntap del dev tap0 mode tap; \
	else \
	    echo "tap0 does not exist"; \
	fi


runn: clean_tap setup_tap run

setup_tap:
	$(MAKE) clean_tap
	# Create TAP device if it doesn't exist
	@if ! ip link show tap0 >/dev/null 2>&1; then \
	    echo "Creating TAP device tap0 owned by $(shell id -un)..."; \
	    sudo ip tuntap add dev tap0 mode tap user $(shell id -un); \
	else \
	    echo "TAP device tap0 already exists"; \
	fi

	# Bring TAP device up
	@if ! ip link show tap0 up | grep -q "UP"; then \
	    echo "Bringing tap0 up..."; \
	    sudo ip link set tap0 up; \
	else \
	    echo "tap0 is already up"; \
	fi

	@echo "TAP device ready. No IP assigned, communicate using MAC addresses only."
	qemu-system-i386 -vga std \
	-m 1024 \
	-boot order=d \
	-cdrom $(OUTPUT_ISO_DIR)/$(ISO_NAME) \
	-drive id=cdrom,file=$(OUTPUT_ISO_DIR)/$(ISO_NAME),format=raw,if=none \
	-drive id=hd0,file=hdd.img,format=raw,if=none \
	-device piix3-ide,id=ide \
	-device ide-hd,drive=hd0,bus=ide.0 \
	-device ide-cd,drive=cdrom,bus=ide.1 \
	-device ac97,audiodev=snd0 \
	-netdev tap,id=tap0,ifname=tap0,script=no,downscript=no \
	-device rtl8139,netdev=tap0,mac=52:54:00:12:34:56 \
	-audiodev sdl,id=snd0



runlh:
	@echo "Running ISO in QEMU..."
	qemu-img create -f raw hdd.img 256M
	qemu-system-i386 -vga std -boot d -cdrom \
	$(OUTPUT_ISO_DIR)/$(ISO_NAME) -m 512 \
	-drive file=hdd.img,format=raw,if=ide,index=0,media=disk \
	-monitor tcp:localhost:4444,server,nowait
	

# Clean
clean:
	@echo "Cleaning output directories..."
	rm -rf $(OUTPUT_DIR) $(INPUT_ISO_DIR)

# Help
help:
	@echo "Makefile usage:"
	@echo "  make kernel     - Compile kernel raw binaries"
	@echo "  make bootloader - Compile bootloader"
	@echo "  make iso        - Build bootable ISO (default)"
	@echo "  make run        - Run ISO in QEMU"
	@echo "  sudo make runn  - Run ISO in QEMU with ethernet driver"
	@echo "  make run_user   - Run headless QEMU (no sudo, user-net)"
	@echo "  make run_user_gui - Run QEMU with GUI+audio (no sudo, user-net)"
	@echo "  Logs: OUTPUT/DEBUG/debug.log (from I/O port 0xE9)"
	@echo "  sudo make setup_tap      - Setup tap ethernet config"
	@echo "  sudo make clean_tap      - Clean tap ethernet config"
	@echo "  make clean      - Clean build artifacts"

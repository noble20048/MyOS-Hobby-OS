# Makefile for MyOS

CC = gcc
AS = gcc
LD = ld
MKRESCUE = grub-mkrescue

CFLAGS = -ffreestanding -m32 -O2 -Wall -Wextra
ASFLAGS = -m32
LDFLAGS = -m elf_i386 -T src/linker.ld

SRC_DIR = src
BUILD_DIR = build
ISO_DIR = iso

OBJS = $(BUILD_DIR)/boot.o $(BUILD_DIR)/kernel.o $(BUILD_DIR)/vga.o $(BUILD_DIR)/fs.o

.PHONY: all clean iso run

all: $(BUILD_DIR)/kernel.elf

$(BUILD_DIR)/boot.o: $(SRC_DIR)/boot.S
	@mkdir -p $(BUILD_DIR)
	$(AS) $(ASFLAGS) -c $< -o $@

$(BUILD_DIR)/kernel.o: $(SRC_DIR)/kernel.c
	@mkdir -p $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/vga.o: $(SRC_DIR)/vga.c
	@mkdir -p $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/fs.o: $(SRC_DIR)/fs.c
	@mkdir -p $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@


$(BUILD_DIR)/kernel.elf: $(OBJS)
	$(LD) $(LDFLAGS) -o $@ $(OBJS)

iso: $(BUILD_DIR)/kernel.elf
	@mkdir -p $(ISO_DIR)/boot/grub
	cp $(BUILD_DIR)/kernel.elf $(ISO_DIR)/boot/kernel.elf
	$(MKRESCUE) -o $(BUILD_DIR)/myos.iso $(ISO_DIR)

clean:
	rm -rf $(BUILD_DIR) $(ISO_DIR)/boot/kernel.elf

run: $(BUILD_DIR)/kernel.elf
	qemu-system-i386 -kernel $(BUILD_DIR)/kernel.elf

CC = i686-elf-gcc
AS = i686-elf-gcc
GRUB_FILE = grub-file
GRUB_MKRESCUE = grub-mkrescue
QEMU = qemu-system-i386

BUILD_DIR = build
ISO_ROOT = $(BUILD_DIR)/isofiles
KERNEL = $(BUILD_DIR)/kernel.elf
ISO = $(BUILD_DIR)/enpy-os.iso

CFLAGS = -std=gnu11 -ffreestanding -O2 -Wall -Wextra -Werror -m32 -fno-pie -fno-stack-protector -ffreestanding
LDFLAGS = -T linker.ld -ffreestanding -O2 -nostdlib -m32 -no-pie

.PHONY: all iso run clean check-toolchain

all: iso

check-toolchain:
	@command -v $(CC) >/dev/null || { echo "Missing $(CC); install an i686-elf cross compiler."; exit 1; }
	@command -v $(GRUB_FILE) >/dev/null || { echo "Missing grub-file; install GRUB utilities."; exit 1; }
	@command -v $(GRUB_MKRESCUE) >/dev/null || { echo "Missing grub-mkrescue; install GRUB utilities."; exit 1; }

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

$(BUILD_DIR)/boot.o: kernel/boot.S | $(BUILD_DIR)
	$(AS) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/kernel.o: kernel/kernel.c kernel/interrupts.h | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/interrupts.o: kernel/interrupts.c kernel/interrupts.h | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/interrupts_asm.o: kernel/interrupts.S | $(BUILD_DIR)
	$(AS) $(CFLAGS) -c $< -o $@

$(KERNEL): $(BUILD_DIR)/boot.o $(BUILD_DIR)/kernel.o $(BUILD_DIR)/interrupts.o $(BUILD_DIR)/interrupts_asm.o linker.ld
	$(CC) $(LDFLAGS) -o $@ $(BUILD_DIR)/boot.o $(BUILD_DIR)/kernel.o $(BUILD_DIR)/interrupts.o $(BUILD_DIR)/interrupts_asm.o -lgcc
	$(GRUB_FILE) --is-x86-multiboot $@ || { echo "Kernel is missing a valid Multiboot header."; exit 1; }

iso: check-toolchain $(KERNEL) grub/grub.cfg
	mkdir -p $(ISO_ROOT)/boot/grub
	cp $(KERNEL) $(ISO_ROOT)/boot/kernel.elf
	cp grub/grub.cfg $(ISO_ROOT)/boot/grub/grub.cfg
	$(GRUB_MKRESCUE) -o $(ISO) $(ISO_ROOT)

run: $(ISO)
	$(QEMU) -boot order=d,strict=on -cdrom $(ISO) -m 128M

clean:
	rm -rf $(BUILD_DIR)

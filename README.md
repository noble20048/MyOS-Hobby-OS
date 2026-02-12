# MyOS-Hobby-OS
MyOS is a 32-bit multiboot hobby OS built with i686-elf GCC and GRUB. 

Currently its a minimal os built out of curiosity which just prints "hi" on the screen and boots using grub via Qemu
## Project Structure

osdev/
├── src/ # Source files
│ ├── boot.S
│ ├── kernel.c
│ └── linker.ld
├── iso/ # GRUB ISO folder
├── build/ # Compiled objects & binaries (ignored in Git)
└── README.md


---

## Build Instructions

Make sure you have **i686-elf cross-compiler** and **GRUB tools** installed.

```bash
# Compile
i686-elf-gcc -ffreestanding -m32 -c src/boot.S -o build/boot.o
i686-elf-gcc -ffreestanding -m32 -c src/kernel.c -o build/kernel.o

# Link
i686-elf-ld -m elf_i386 -T src/linker.ld -o build/kernel.elf build/boot.o build/kernel.o

# Copy kernel to ISO folder
cp build/kernel.elf iso/boot/

Run Instructions

We can run this OS using QEMU and connect via VNC:
(tigervnc or any other vnc client)

qemu-system-i386 -cdrom build/osdev.iso -vnc :0
vncviewer localhost:5900


GRUB menu will appear

Select MyOS

The screen should display Hi
Notes:
This is a very minimal os built out of curiosity, and its a learning+hobby projects,features are very basic at the moment.
In the future there will be more additions like keyboard input,basic shell and note typing, and will try to expand its functionality

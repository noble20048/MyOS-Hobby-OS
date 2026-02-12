# src/boot.S

# Multiboot header
.section .multiboot
.align 4
.long 0x1BADB002       # Multiboot magic
.long 0x0              # Flags
.long -(0x1BADB002+0x0) # Checksum

.global _start
_start:
    call kernel_main
    jmp .   # Infinite loop

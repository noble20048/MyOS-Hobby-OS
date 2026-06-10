# src/boot.S

# Multiboot header
.section .multiboot
.align 4
.long 0x1BADB002       # Multiboot magic
.long 0x0              # Flags
.long -(0x1BADB002+0x0) # Checksum

.section .bss
.align 16
stack_bottom:
.skip 16384 # 16 KiB
stack_top:

.section .text
.global _start
_start:
    mov $stack_top, %esp
    call kernel_main
    cli
1:  hlt
    jmp 1b


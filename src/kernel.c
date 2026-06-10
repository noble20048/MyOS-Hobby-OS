#include "vga.h"

// Read a byte from a hardware port
static inline unsigned char inb(unsigned short port) {
    unsigned char ret;
    __asm__ __volatile__ ("inb %1, %0" : "=a"(ret) : "Nd"(port));
    return ret;
}

unsigned char kbd_us[128] = {
    0,  27, '1', '2', '3', '4', '5', '6', '7', '8',	/* 9 */
  '9', '0', '-', '=', '\b',	/* Backspace */
  '\t',			/* Tab */
  'q', 'w', 'e', 'r',	/* 19 */
  't', 'y', 'u', 'i', 'o', 'p', '[', ']', '\n',	/* Enter key */
    0,			/* 29   - Control */
  'a', 's', 'd', 'f', 'g', 'h', 'j', 'k', 'l', ';',	/* 39 */
 '\'', '`',   0,		/* Left shift */
 '\\', 'z', 'x', 'c', 'v', 'b', 'n', 'm', ',', '.',	/* 49 */
  '/',   0,				/* Right shift */
  '*',
    0,	/* Alt */
  ' ',	/* Space bar */
};

void kernel_main(void) {
    terminal_initialize();

    // The "Vibe" Boot Splash
    terminal_setcolor(vga_entry_color(VGA_COLOR_CYAN, VGA_COLOR_BLACK));
    terminal_writestring("  __  __         ____   _____ \n");
    terminal_writestring(" |  \\/  |       / __ \\ / ____|\n");
    terminal_writestring(" | \\  / |_   _ | |  | | (___  \n");
    terminal_writestring(" | |\\/| | | | || |  | |\\___ \\ \n");
    terminal_writestring(" | |  | | |_| || |__| |____) |\n");
    terminal_writestring(" |_|  |_|\\__, | \\____/|_____/ \n");
    terminal_writestring("          __/ |               \n");
    terminal_writestring("         |___/                \n\n");

    terminal_setcolor(vga_entry_color(VGA_COLOR_GREEN, VGA_COLOR_BLACK));
    terminal_writestring("[ OK ] Kernel initialized.\n");
    terminal_writestring("[ OK ] Keyboard driver (polling) ready.\n");
    terminal_writestring("[ INFO ] Type something to test the vibes...\n\n");
    
    // Reset color for user input
    terminal_setcolor(vga_entry_color(VGA_COLOR_WHITE, VGA_COLOR_BLACK));

    while(1) {
        if (inb(0x64) & 1) {
            unsigned char scancode = inb(0x60);
            if (scancode & 0x80) continue;

            char c = kbd_us[scancode];
            
            // The Vibe Controller
            if (scancode == 0x3B) {        // F1
                terminal_setcolor(vga_entry_color(VGA_COLOR_LIGHT_CYAN, VGA_COLOR_BLACK));
                terminal_writestring("\n[ VIBE ] Electric Blue Mode!\n");
            } else if (scancode == 0x3C) { // F2
                terminal_setcolor(vga_entry_color(VGA_COLOR_LIGHT_MAGENTA, VGA_COLOR_BLACK));
                terminal_writestring("\n[ VIBE ] Pink Vibes!\n");
            } else if (scancode == 0x3D) { // F3
                terminal_setcolor(vga_entry_color(VGA_COLOR_LIGHT_RED, VGA_COLOR_BLACK));
                terminal_writestring("\n[ VIBE ] Hacker Red Mode!\n");
            } else if (c != 0) {
                terminal_putchar(c);
            }

        }
    }
}
#include "fs.h"
#include "vga.h"

// Command buffer to store what the user is typing
char buffer[256];
int buffer_index = 0;

// This function processes the command after Enter is pressed
void shell_execute(char *cmd) {
  if (str_compare(cmd, "help") == 0) {
    terminal_writestring("Commands: help, ls, cat <file>, touch <file>, write "
                         "<file> <text>, rm <file>, clear, vibe, whoami\n");
  } else if (str_compare(cmd, "ls") == 0) {
    fs_list();
  } else if (str_compare(cmd, "clear") == 0) {
    terminal_clear();
  } else if (cmd[0] == 'c' && cmd[1] == 'a' && cmd[2] == 't' && cmd[3] == ' ') {
    // Simple parser for "cat filename"
    fs_read(cmd + 4);
  } else if (str_compare(cmd, "vibe") == 0) {
    terminal_writestring("System Check: Everything's fine.\n");
  } else if (str_compare(cmd, "whoami") == 0) {
    terminal_writestring("You are Noble, Welcome Buddy!\n");
  } else if (cmd[0] == 't' && cmd[1] == 'o' && cmd[2] == 'u' && cmd[3] == 'c' &&
             cmd[4] == 'h' && cmd[5] == ' ') {
    fs_create(cmd + 6);
  } else if (cmd[0] == 'w' && cmd[1] == 'r' && cmd[2] == 'i' && cmd[3] == 't' &&
             cmd[4] == 'e' && cmd[5] == ' ') {
    char *filename = cmd + 6;
    char *content = 0;
    for (int i = 0; filename[i] != '\0'; i++) {
      if (filename[i] == ' ') {
        filename[i] = '\0';
        content = filename + i + 1;
        break;
      }
    }
    if (content) {
      fs_write(filename, content);
    } else {
      terminal_writestring("Usage: write <filename> <text>\n");
    }
  } else if (cmd[0] == 'r' && cmd[1] == 'm' && cmd[2] == ' ') {
    fs_delete(cmd + 3);
  } else if (cmd[0] != 0) {
    terminal_writestring("Unknown command: ");
    terminal_writestring(cmd);
    terminal_writestring("\n");
  }

  // Print the colorful prompt again
  terminal_setcolor(vga_entry_color(VGA_COLOR_LIGHT_GREEN, VGA_COLOR_BLACK));
  terminal_writestring("[Noble@MyOS]");
  terminal_setcolor(vga_entry_color(VGA_COLOR_LIGHT_CYAN, VGA_COLOR_BLACK));
  terminal_writestring(" ~> ");
}

// Read a byte from a hardware port
static inline unsigned char inb(unsigned short port) {
  unsigned char ret;
  __asm__ __volatile__("inb %1, %0" : "=a"(ret) : "Nd"(port));
  return ret;
}

unsigned char kbd_us[128] = {
    0,    27,  '1', '2', '3',  '4', '5', '6', '7',  '8', /* 9 */
    '9',  '0', '-', '=', '\b',                           /* Backspace */
    '\t',                                                /* Tab */
    'q',  'w', 'e', 'r',                                 /* 19 */
    't',  'y', 'u', 'i', 'o',  'p', '[', ']', '\n',      /* Enter key */
    0,                                                   /* 29   - Control */
    'a',  's', 'd', 'f', 'g',  'h', 'j', 'k', 'l',  ';', /* 39 */
    '\'', '`', 0,                                        /* Left shift */
    '\\', 'z', 'x', 'c', 'v',  'b', 'n', 'm', ',',  '.', /* 49 */
    '/',  0,                                             /* Right shift */
    '*',  0,                                             /* Alt */
    ' ',                                                 /* Space bar */
};

void kernel_main(void) {
  terminal_initialize();
  fs_init();

  // The "Enhanced" Boot Splash
  terminal_clear();
  terminal_setcolor(vga_entry_color(VGA_COLOR_MAGENTA, VGA_COLOR_BLACK));
  terminal_writestring("--------------------------------------------------------------------------------\n");
  terminal_setcolor(vga_entry_color(VGA_COLOR_CYAN, VGA_COLOR_BLACK));
  terminal_writestring("   __  __         ____   _____    [ VIBE EDITION - v1.2 ]\n");
  terminal_writestring("  |  \\/  |       / __ \\ / ____|   -----------------------\n");
  terminal_writestring("  | \\  / |_   _ | |  | | (___     Status: VIBES OPTIMAL\n");
  terminal_writestring("  | |\\/| | | | || |  | |\\___ \\    System: ACTIVE\n");
  terminal_writestring("  | |  | | |_| || |__| |____) |   User:   NOBLE\n");
  terminal_writestring("  |_|  |_|\\__, | \\____/|_____/    \n");
  terminal_writestring("           __/ |                  \n");
  terminal_writestring("          |___/                   \n");
  terminal_setcolor(vga_entry_color(VGA_COLOR_MAGENTA, VGA_COLOR_BLACK));
  terminal_writestring("--------------------------------------------------------------------------------\n\n");

  terminal_setcolor(vga_entry_color(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK));
  terminal_writestring("Welcome back, Noble. Type 'help' to explore.\n\n");
  
  // Reset color for user input
  terminal_setcolor(vga_entry_color(VGA_COLOR_LIGHT_GREEN, VGA_COLOR_BLACK));
  terminal_writestring("[Noble@MyOS]");
  terminal_setcolor(vga_entry_color(VGA_COLOR_LIGHT_CYAN, VGA_COLOR_BLACK));
  terminal_writestring(" ~> ");

  while (1) {
    if (inb(0x64) & 1) {
      unsigned char scancode = inb(0x60);
      if (scancode & 0x80)
        continue; // Ignore key release

      if (scancode == 0x3E) { // F4 Key (File Manager Button)
        terminal_clear();
        terminal_setcolor(
            vga_entry_color(VGA_COLOR_LIGHT_CYAN, VGA_COLOR_BLACK));
        terminal_writestring("--- [ QUICK FILE BROWSER ] ---\n");
        fs_list();
        terminal_writestring("------------------------------\n");
        terminal_setcolor(vga_entry_color(VGA_COLOR_LIGHT_GREEN, VGA_COLOR_BLACK));
        terminal_writestring("[Noble@MyOS]");
        terminal_setcolor(vga_entry_color(VGA_COLOR_LIGHT_CYAN, VGA_COLOR_BLACK));
        terminal_writestring(" ~> ");
        buffer_index = 0;
        continue;
      }

      if (scancode == 0x1C) { // ENTER KEY
        terminal_putchar('\n');
        buffer[buffer_index] = '\0'; // End the string
        shell_execute(buffer);       // Run the command
        buffer_index = 0;            // Reset buffer
      } else if (scancode == 0x0E) { // BACKSPACE
        if (buffer_index > 0) {
          buffer_index--;
          terminal_putchar('\b');
        }
      } else {
        char c = kbd_us[scancode];
        if (c != 0 && buffer_index < 255) {
          buffer[buffer_index++] = c;
          terminal_putchar(c);
        }
      }
    }
  }
}
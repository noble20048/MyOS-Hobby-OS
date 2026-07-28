#include "fs.h"
#include "vga.h"
void show_time();
void itoa(int n, char *s);
void terminal_print_int(int n);

static inline unsigned char inb(unsigned short port) {
  unsigned char ret;
  __asm__ __volatile__("inb %1, %0" : "=a"(ret) : "Nd"(port));
  return ret;
}

static inline void outb(unsigned short port, unsigned char val) {
  __asm__ __volatile__("outb %0, %1" : : "a"(val), "Nd"(port));
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
void draw_editor(const char *filename, const char *buf, uint32_t len, uint32_t cursor_pos) {
  // A. Draw top bar (row 0) in cyan background, black text
  uint8_t top_color = vga_entry_color(VGA_COLOR_BLACK, VGA_COLOR_LIGHT_CYAN);
  for (int x = 0; x < 80; x++) {
    terminal_putentryat(' ', top_color, x, 0);
  }
  const char *title = "MyOS Text Editor | Editing: ";
  int tx = 0;
  while (title[tx]) {
    terminal_putentryat(title[tx], top_color, tx, 0);
    tx++;
  }
  int fx = 0;
  while (filename[fx]) {
    terminal_putentryat(filename[fx], top_color, tx + fx, 0);
    fx++;
  }

  // B. Draw bottom bar (row 24) in dark grey background white text
  uint8_t bottom_color = vga_entry_color(VGA_COLOR_WHITE, VGA_COLOR_DARK_GREY);
  for (int x = 0; x < 80; x++) {
    terminal_putentryat(' ', bottom_color, x, 24);
  }
  const char *help = " [F2/F10] Save & Exit  |   [ESC] Exit without Saving";
  int hx = 0;
  while (help[hx]) {
    terminal_putentryat(help[hx], bottom_color, hx, 24);
    hx++;
  }

  // C. Clear the main text editing area (rows 1 to 23) to black
  uint8_t text_color = vga_entry_color(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK);
  for (int y = 1; y < 24; y++) {
    for (int x = 0; x < 80; x++) {
      terminal_putentryat(' ', text_color, x, y);
    }
  }

  // D. Print the buffer contents character by character
  int cur_x = 0;
  int cur_y = 1;
  int target_cursor_x = 0;
  int target_cursor_y = 1;

  for (uint32_t i = 0; i <= len; i++) {
    // Record where the cursor should blink when we reach the cursor's index
    if (i == cursor_pos) {
      target_cursor_x = cur_x;
      target_cursor_y = cur_y;
    }

    if (i == len)
      break;

    char c = buf[i];
    if (c == '\n') {
      cur_x = 0;
      cur_y++;
      if (cur_y >= 24)
        break;
    } else {
      terminal_putentryat(c, text_color, cur_x, cur_y);
      cur_x++;
      if (cur_x >= 80) {
        cur_x = 0;
        cur_y++;
        if (cur_y >= 24)
          break;
      }
    }
  }

  // E. Place the hardware blinking cursor at the cursor location
  terminal_move_cursor(target_cursor_x, target_cursor_y);
}

void start_editor(char *filename) {
  // A. Check if the file exists. If not, create it.
  int file_idx = -1;
  for (int i = 0; i < MAX_FILES; i++) {
    if (files[i].exists && str_compare(files[i].name, filename) == 0) {
      file_idx = i;
      break;
    }
  }

  if (file_idx == -1) {
    fs_create(filename);
    for (int i = 0; i < MAX_FILES; i++) {
      if (files[i].exists && str_compare(files[i].name, filename) == 0) {
        file_idx = i;
        break;
      }
    }
  }

  if (file_idx == -1) {
    terminal_writestring("Error: Filesystem is full!\n");
    return;
  }

  // B. Load current content into a temporary editor buffer
  char edit_buffer[MAX_FILE_SIZE];
  uint32_t buffer_len = files[file_idx].size;
  for (uint32_t i = 0; i < buffer_len; i++) {
    edit_buffer[i] = files[file_idx].content[i];
  }
  edit_buffer[buffer_len] = '\0';

  uint32_t edit_cursor = buffer_len; // Start cursor at end of text
  int editing = 1;

  // Initial draw
  draw_editor(filename, edit_buffer, buffer_len, edit_cursor);

  // C. Editor keyboard event loop
  while (editing) {
    // Wait for keyboard input
    if (inb(0x64) & 1) {
      unsigned char scancode = inb(0x60);
      if (scancode & 0x80)
        continue; // Ignore key release events

      if (scancode == 0x01) { // ESC Key - Exit without saving
        editing = 0;
      } else if (scancode == 0x44 || scancode == 0x3C) { // F10 (0x44) or F2 (0x3C) Key - Save & Exit
        fs_write(filename, edit_buffer);
        editing = 0;
      } else if (scancode == 0x4B) { // Left Arrow
        if (edit_cursor > 0)
          edit_cursor--;
      } else if (scancode == 0x4D) { // Right Arrow
        if (edit_cursor < buffer_len)
          edit_cursor++;
      } else if (scancode == 0x48) { // Up Arrow
        if (edit_cursor > 0) {
          int cur_line_start = edit_cursor;
          while (cur_line_start > 0 && edit_buffer[cur_line_start - 1] != '\n') {
            cur_line_start--;
          }
          int col = edit_cursor - cur_line_start;
          if (cur_line_start > 0) {
            int prev_line_end = cur_line_start - 1;
            int prev_line_start = prev_line_end;
            while (prev_line_start > 0 &&
                   edit_buffer[prev_line_start - 1] != '\n') {
              prev_line_start--;
            }
            int prev_line_len = prev_line_end - prev_line_start;
            if (col > prev_line_len) {
              edit_cursor = prev_line_start + prev_line_len;
            } else {
              edit_cursor = prev_line_start + col;
            }
          }
        }
      } else if (scancode == 0x50) { // Down Arrow
        int cur_line_start = edit_cursor;
        while (cur_line_start > 0 && edit_buffer[cur_line_start - 1] != '\n') {
          cur_line_start--;
        }
        int col = edit_cursor - cur_line_start;

        int cur_line_end = edit_cursor;
        while (cur_line_end < (int)buffer_len &&
               edit_buffer[cur_line_end] != '\n') {
          cur_line_end++;
        }

        if (cur_line_end < (int)buffer_len) {
          int next_line_start = cur_line_end + 1;
          int next_line_end = next_line_start;
          while (next_line_end < (int)buffer_len &&
                 edit_buffer[next_line_end] != '\n') {
            next_line_end++;
          }
          int next_line_len = next_line_end - next_line_start;
          if (col > next_line_len) {
            edit_cursor = next_line_start + next_line_len;
          } else {
            edit_cursor = next_line_start + col;
          }
        }
      } else if (scancode == 0x0E) { // Backspace
        if (edit_cursor > 0) {
          for (uint32_t i = edit_cursor - 1; i < buffer_len - 1; i++) {
            edit_buffer[i] = edit_buffer[i + 1];
          }
          buffer_len--;
          edit_cursor--;
          edit_buffer[buffer_len] = '\0';
        }
      } else if (scancode == 0x1C) { // Enter Key (New line)
        if (buffer_len < MAX_FILE_SIZE - 1) {
          for (int i = buffer_len; i > (int)edit_cursor; i--) {
            edit_buffer[i] = edit_buffer[i - 1];
          }
          edit_buffer[edit_cursor] = '\n';
          buffer_len++;
          edit_cursor++;
          edit_buffer[buffer_len] = '\0';
        }
      } else { // Normal Character Typing
        char c = kbd_us[scancode];
        if (c != 0 && buffer_len < MAX_FILE_SIZE - 1) {
          for (int i = buffer_len; i > (int)edit_cursor; i--) {
            edit_buffer[i] = edit_buffer[i - 1];
          }
          edit_buffer[edit_cursor] = c;
          buffer_len++;
          edit_cursor++;
          edit_buffer[buffer_len] = '\0';
        }
      }
      draw_editor(filename, edit_buffer, buffer_len, edit_cursor);
    }
  }

  // D. Exit back to the shell cleanly
  terminal_clear();
}

/*
 * ============================================================================
 * INTERACTIVE TUI FILE MANAGER
 * ============================================================================
 */

// Draw double-lined bordered panels with centering titles in VGA Text Mode
void draw_box(uint32_t start_x, uint32_t start_y, uint32_t width, uint32_t height, const char *title, uint8_t color) {
  // Corners (ASCII: 201 = ╔, 187 = ╗, 200 = ╚, 188 = ╝)
  terminal_putentryat((char)201, color, start_x, start_y);
  terminal_putentryat((char)187, color, start_x + width - 1, start_y);
  terminal_putentryat((char)200, color, start_x, start_y + height - 1);
  terminal_putentryat((char)188, color, start_x + width - 1, start_y + height - 1);

  // Horizontal double borders (ASCII: 205 = ═)
  for (uint32_t x = start_x + 1; x < start_x + width - 1; x++) {
    terminal_putentryat((char)205, color, x, start_y);
    terminal_putentryat((char)205, color, x, start_y + height - 1);
  }

  // Vertical double borders (ASCII: 186 = ║) and background fill
  for (uint32_t y = start_y + 1; y < start_y + height - 1; y++) {
    terminal_putentryat((char)186, color, start_x, y);
    terminal_putentryat((char)186, color, start_x + width - 1, y);
    for (uint32_t x = start_x + 1; x < start_x + width - 1; x++) {
      terminal_putentryat(' ', color, x, y);
    }
  }

  // Center and draw the title on the top border
  if (title) {
    int len = 0;
    while (title[len]) len++;
    int title_x = start_x + (width - len) / 2;
    for (int i = 0; i < len; i++) {
      terminal_putentryat(title[i], color, title_x + i, start_y);
    }
  }
}

// Draw the main File Manager dashboard
void draw_file_manager(int selected_index) {
  uint8_t border_color = vga_entry_color(VGA_COLOR_WHITE, VGA_COLOR_BLUE);
  uint8_t top_bar_color = vga_entry_color(VGA_COLOR_BLACK, VGA_COLOR_LIGHT_CYAN);
  uint8_t bottom_bar_color = vga_entry_color(VGA_COLOR_WHITE, VGA_COLOR_DARK_GREY);

  // A. Top info bar
  for (int x = 0; x < 80; x++) {
    terminal_putentryat(' ', top_bar_color, x, 0);
  }
  const char *title = "MyOS File Manager | [ESC/Q] Exit back to Shell";
  int tx = 0;
  while (title[tx]) {
    terminal_putentryat(title[tx], top_bar_color, tx + 2, 0);
    tx++;
  }

  // B. Bottom key shortcuts legend
  for (int x = 0; x < 80; x++) {
    terminal_putentryat(' ', bottom_bar_color, x, 24);
  }
  const char *help = " [UP/DOWN] Select  [V] View  [E] Edit  [C] Create  [D] Delete  [R] Rename";
  int hx = 0;
  while (help[hx]) {
    terminal_putentryat(help[hx], bottom_bar_color, hx + 2, 24);
    hx++;
  }

  // C. Fill outer desktop background (Blue)
  uint8_t bg_outer = vga_entry_color(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLUE);
  for (int y = 1; y < 24; y++) {
    for (int x = 0; x < 80; x++) {
      terminal_putentryat(' ', bg_outer, x, y);
    }
  }

  // D. Draw the double-lined main container panel (X: 2, Y: 2, W: 76, H: 20)
  draw_box(2, 2, 76, 20, " FILE MANAGER ", border_color);

  // E. Column Titles (Row 3)
  const char* headers = "   Name                             Size (Bytes)        Status";
  int hx_offset = 5;
  while (headers[hx_offset - 5]) {
    terminal_putentryat(headers[hx_offset - 5], border_color, hx_offset, 3);
    hx_offset++;
  }

  // Header separator line (ASCII: 204 = ╠, 185 = ╣, 205 = ═)
  terminal_putentryat((char)204, border_color, 2, 4);
  terminal_putentryat((char)185, border_color, 77, 4);
  for (int x = 3; x < 77; x++) {
    terminal_putentryat((char)205, border_color, x, 4);
  }

  // F. Populate file rows (Rows 5 to 20)
  int row = 5;
  for (int i = 0; i < MAX_FILES; i++) {
    uint8_t item_color;
    if (i == selected_index) {
      item_color = vga_entry_color(VGA_COLOR_BLACK, VGA_COLOR_LIGHT_CYAN); // Highlighted active row
    } else {
      item_color = vga_entry_color(VGA_COLOR_WHITE, VGA_COLOR_BLUE); // Regular row
    }

    // Fill file row background
    for (int x = 3; x < 77; x++) {
      terminal_putentryat(' ', item_color, x, row);
    }

    if (files[i].exists) {
      // Draw indicator arrow
      if (i == selected_index) {
        terminal_putentryat('>', item_color, 4, row);
      }

      // Draw Filename
      int fx = 0;
      while (files[i].name[fx] && fx < MAX_FILENAME) {
        terminal_putentryat(files[i].name[fx], item_color, 6 + fx, row);
        fx++;
      }

      // Draw File Size
      char size_str[16];
      itoa(files[i].size, size_str);
      int sx = 0;
      while (size_str[sx]) {
        terminal_putentryat(size_str[sx], item_color, 42 + sx, row);
        sx++;
      }

      // Draw Status info
      const char *status = "Saved (Disk)";
      int stx = 0;
      while (status[stx]) {
        terminal_putentryat(status[stx], item_color, 62 + stx, row);
        stx++;
      }
    } else {
      // Draw placeholder for empty drive slot
      const char *empty_text = "- [Empty Slot] -";
      int ex = 0;
      while (empty_text[ex]) {
        terminal_putentryat(empty_text[ex], item_color, 6 + ex, row);
        ex++;
      }
    }
    row++;
  }
}

// Sub-screen for viewing a file's contents directly
void view_file_content_tui(const char* name) {
  int file_idx = -1;
  for (int i = 0; i < MAX_FILES; i++) {
    if (files[i].exists && str_compare(files[i].name, name) == 0) {
      file_idx = i;
      break;
    }
  }
  if (file_idx == -1) return;

  uint8_t border_color = vga_entry_color(VGA_COLOR_WHITE, VGA_COLOR_BLUE);
  draw_box(4, 4, 72, 16, " FILE CONTENT VIEW ", border_color);

  int cur_x = 6;
  int cur_y = 6;
  uint8_t text_color = vga_entry_color(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLUE);

  // Loop through file characters and write to screen, wrapping at margins
  for (uint32_t i = 0; i < files[file_idx].size; i++) {
    char c = files[file_idx].content[i];
    if (c == '\n') {
      cur_x = 6;
      cur_y++;
      if (cur_y >= 19) break;
    } else {
      terminal_putentryat(c, text_color, cur_x, cur_y);
      cur_x++;
      if (cur_x >= 70) {
        cur_x = 6;
        cur_y++;
        if (cur_y >= 19) break;
      }
    }
  }

  // Draw navigation help inside viewer
  const char *instr = "Press [ESC] or [Q] to return to File Manager";
  int inst_len = 0;
  while (instr[inst_len]) inst_len++;
  int inst_x = 4 + (72 - inst_len) / 2;
  uint8_t inst_color = vga_entry_color(VGA_COLOR_LIGHT_CYAN, VGA_COLOR_BLUE);
  for (int i = 0; i < inst_len; i++) {
    terminal_putentryat(instr[i], inst_color, inst_x + i, 18);
  }

  // Stay inside reader loop
  while (1) {
    if (inb(0x64) & 1) {
      unsigned char scancode = inb(0x60);
      if (scancode & 0x80) continue;
      if (scancode == 0x01 || scancode == 0x10) { // ESC or Q
        break;
      }
    }
  }
}

// Modal dialog for entering new filename
void prompt_create_file_tui(void) {
  uint8_t border_color = vga_entry_color(VGA_COLOR_WHITE, VGA_COLOR_BLUE);
  uint8_t input_color = vga_entry_color(VGA_COLOR_WHITE, VGA_COLOR_BLACK);

  draw_box(15, 8, 50, 8, " CREATE NEW FILE ", border_color);

  const char *prompt = "Enter new filename:";
  int px = 0;
  while (prompt[px]) {
    terminal_putentryat(prompt[px], border_color, 18 + px, 10);
    px++;
  }

  // Clear input field background
  for (int x = 18; x < 62; x++) {
    terminal_putentryat(' ', input_color, x, 12);
  }

  char name_buf[MAX_FILENAME];
  int name_len = 0;
  name_buf[0] = '\0';

  terminal_move_cursor(18, 12);
  terminal_enable_cursor(14, 15);

  int entering = 1;
  while (entering) {
    if (inb(0x64) & 1) {
      unsigned char scancode = inb(0x60);
      if (scancode & 0x80) continue;

      if (scancode == 0x01) { // ESC
        entering = 0;
      } else if (scancode == 0x1C) { // ENTER
        if (name_len > 0) {
          name_buf[name_len] = '\0';
          fs_create(name_buf);
        }
        entering = 0;
      } else if (scancode == 0x0E) { // Backspace
        if (name_len > 0) {
          name_len--;
          name_buf[name_len] = '\0';
          terminal_putentryat(' ', input_color, 18 + name_len, 12);
          terminal_move_cursor(18 + name_len, 12);
        }
      } else {
        char c = kbd_us[scancode];
        if (c != 0 && c != '\n' && c != '\b' && name_len < MAX_FILENAME - 1) {
          name_buf[name_len++] = c;
          name_buf[name_len] = '\0';
          terminal_putentryat(c, input_color, 18 + name_len - 1, 12);
          terminal_move_cursor(18 + name_len, 12);
        }
      }
    }
  }
  terminal_disable_cursor();
}

// Modal dialog warning for file deletion (Red border layout)
void prompt_delete_file_tui(const char* name) {
  uint8_t border_color = vga_entry_color(VGA_COLOR_WHITE, VGA_COLOR_RED);

  draw_box(15, 8, 50, 8, " DELETE FILE ", border_color);

  const char *msg1 = "Are you sure you want to delete:";
  int mx1 = 0;
  while (msg1[mx1]) {
    terminal_putentryat(msg1[mx1], border_color, 18 + mx1, 10);
    mx1++;
  }

  int nx = 0;
  while (name[nx] && nx < MAX_FILENAME) {
    terminal_putentryat(name[nx], vga_entry_color(VGA_COLOR_LIGHT_RED, VGA_COLOR_RED), 18 + nx, 11);
    nx++;
  }

  const char *msg2 = "Press [Y] to Confirm | [N] to Cancel";
  int mx2 = 0;
  while (msg2[mx2]) {
    terminal_putentryat(msg2[mx2], border_color, 18 + mx2, 13);
    mx2++;
  }

  while (1) {
    if (inb(0x64) & 1) {
      unsigned char scancode = inb(0x60);
      if (scancode & 0x80) continue;

      char c = kbd_us[scancode];
      if (c == 'y' || c == 'Y') {
        fs_delete(name);
        break;
      } else if (c == 'n' || c == 'N' || scancode == 0x01) {
        break;
      }
    }
  }
}

// Modal dialog for renaming files
void prompt_rename_file_tui(const char* name) {
  uint8_t border_color = vga_entry_color(VGA_COLOR_WHITE, VGA_COLOR_BLUE);
  uint8_t input_color = vga_entry_color(VGA_COLOR_WHITE, VGA_COLOR_BLACK);

  draw_box(15, 8, 50, 8, " RENAME FILE ", border_color);

  const char *prompt = "Enter new filename:";
  int px = 0;
  while (prompt[px]) {
    terminal_putentryat(prompt[px], border_color, 18 + px, 10);
    px++;
  }

  // Pre-fill the input with the old name
  for (int x = 18; x < 62; x++) {
    terminal_putentryat(' ', input_color, x, 12);
  }

  char name_buf[MAX_FILENAME];
  int name_len = 0;
  while (name[name_len] && name_len < MAX_FILENAME - 1) {
    name_buf[name_len] = name[name_len];
    terminal_putentryat(name_buf[name_len], input_color, 18 + name_len, 12);
    name_len++;
  }
  name_buf[name_len] = '\0';

  terminal_move_cursor(18 + name_len, 12);
  terminal_enable_cursor(14, 15);

  int entering = 1;
  while (entering) {
    if (inb(0x64) & 1) {
      unsigned char scancode = inb(0x60);
      if (scancode & 0x80) continue;

      if (scancode == 0x01) { // ESC
        entering = 0;
      } else if (scancode == 0x1C) { // ENTER
        if (name_len > 0) {
          name_buf[name_len] = '\0';
          // Find and update filename in database
          for (int i = 0; i < MAX_FILES; i++) {
            if (files[i].exists && str_compare(files[i].name, name) == 0) {
              int j = 0;
              while (name_buf[j] != '\0' && j < MAX_FILENAME - 1) {
                files[i].name[j] = name_buf[j];
                j++;
              }
              files[i].name[j] = '\0';
              fs_save_to_disk();
              break;
            }
          }
        }
        entering = 0;
      } else if (scancode == 0x0E) { // Backspace
        if (name_len > 0) {
          name_len--;
          name_buf[name_len] = '\0';
          terminal_putentryat(' ', input_color, 18 + name_len, 12);
          terminal_move_cursor(18 + name_len, 12);
        }
      } else {
        char c = kbd_us[scancode];
        if (c != 0 && c != '\n' && c != '\b' && name_len < MAX_FILENAME - 1) {
          name_buf[name_len++] = c;
          name_buf[name_len] = '\0';
          terminal_putentryat(c, input_color, 18 + name_len - 1, 12);
          terminal_move_cursor(18 + name_len, 12);
        }
      }
    }
  }
  terminal_disable_cursor();
}

// File Manager event loop & entry point
void start_file_manager(void) {
  int selected = 0;
  int running = 1;
  int dirty = 1; // 1 means we need to redraw the interface

  terminal_disable_cursor();

  // Highlight first active file slot on start
  for (int i = 0; i < MAX_FILES; i++) {
    if (files[i].exists) {
      selected = i;
      break;
    }
  }

  while (running) {
    if (dirty) {
      draw_file_manager(selected);
      dirty = 0; // Reset flag after drawing
    }

    if (inb(0x64) & 1) {
      unsigned char scancode = inb(0x60);
      if (scancode & 0x80) continue; // Key release ignore

      if (scancode == 0x01 || scancode == 0x10) { // ESC or Q (Quit)
        running = 0;
      } else if (scancode == 0x48) { // Up Arrow
        int prev = selected;
        do {
          prev--;
          if (prev < 0) prev = MAX_FILES - 1;
        } while (!files[prev].exists && prev != selected);
        if (files[prev].exists && prev != selected) {
          selected = prev;
          dirty = 1;
        }
      } else if (scancode == 0x50) { // Down Arrow
        int next = selected;
        do {
          next++;
          if (next >= MAX_FILES) next = 0;
        } while (!files[next].exists && next != selected);
        if (files[next].exists && next != selected) {
          selected = next;
          dirty = 1;
        }
      } else if (scancode == 0x2F) { // V key (View file)
        if (files[selected].exists) {
          view_file_content_tui(files[selected].name);
          dirty = 1; // Mark dirty to redraw the dashboard after viewport closes
        }
      } else if (scancode == 0x12) { // E key (Edit file)
        if (files[selected].exists) {
          terminal_enable_cursor(14, 15);
          start_editor(files[selected].name);
          terminal_disable_cursor();
          dirty = 1; // Mark dirty to redraw dashboard after editor closes
        }
      } else if (scancode == 0x2E) { // C key (Create file)
        prompt_create_file_tui();
        for (int i = 0; i < MAX_FILES; i++) {
          if (files[i].exists && files[i].size == 0) {
            selected = i;
          }
        }
        dirty = 1; // Redraw dashboard after creating a new file
      } else if (scancode == 0x20) { // D key (Delete file)
        if (files[selected].exists) {
          prompt_delete_file_tui(files[selected].name);
          selected = 0;
          for (int i = 0; i < MAX_FILES; i++) {
            if (files[i].exists) {
              selected = i;
              break;
            }
          }
          dirty = 1; // Redraw dashboard after deleting
        }
      } else if (scancode == 0x13) { // R key (Rename file)
        if (files[selected].exists) {
          prompt_rename_file_tui(files[selected].name);
          dirty = 1; // Redraw dashboard after renaming
        }
      }
    }
  }

  terminal_enable_cursor(14, 15);
  terminal_clear();
}

// Command buffer to store what the user is typing
char buffer[256];
int buffer_index = 0;

// This function processes the command after Enter is pressed
void shell_execute(char *cmd) {
  if (str_compare(cmd, "help") == 0) {
    terminal_writestring(
        "Commands: help, ls, cat <file>, touch <file>, write <file> <text>,\n"
        "          edit <file>, rm <file>, fm, clear, vibe, whoami, time\n");
  } else if (str_compare(cmd, "ls") == 0) {
    fs_list();
  } else if (str_compare(cmd, "fm") == 0) {
    start_file_manager();
  } else if (str_compare(cmd, "clear") == 0) {
    terminal_clear();
  } else if (str_compare(cmd, "time") == 0) {
    show_time();
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
  } else if (cmd[0] == 'e' && cmd[1] == 'd' && cmd[2] == 'i' && cmd[3] == 't' &&
             cmd[4] == ' ') {
    start_editor(cmd + 5);
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


// convert an integer to a string (very basic method)
void itoa(int n, char *s) {
  int i, sign;
  if ((sign = n) < 0)
    n = -n;
  i = 0;
  do {
    s[i++] = n % 10 + '0';
  } while ((n /= 10) > 0);
  if (sign < 0)
    s[i++] = '-';
  s[i] = '\0';
  // reverse string
  for (int j = 0, k = i - 1; j < k; j++, k--) {
    char temp = s[j];
    s[j] = s[k];
    s[k] = temp;
  }
}

int bcd_to_bin(int bcd) { return ((bcd / 16) * 10) + (bcd % 16); }

void terminal_print_int(int n) {
  char str[16];
  itoa(n, str);
  terminal_writestring(str);
}

void show_time() {
  // 0x70 is the address port, 0x71 is the data port
  outb(0x70, 0x04); // Register 0x04 is Hours
  int hours = bcd_to_bin(inb(0x71));

  outb(0x70, 0x02); // Register 0x02 is Minutes
  int mins = bcd_to_bin(inb(0x71));

  terminal_writestring("Current Time (UTC): ");
  terminal_print_int(hours);
  terminal_writestring(":");
  if (mins < 10)
    terminal_writestring("0"); // Leading zero
  terminal_print_int(mins);
  terminal_writestring("\n");
}



void kernel_main(void) {
  terminal_initialize();
  fs_load_from_disk();

  // The "Enhanced" Boot Splash
  terminal_clear();
  terminal_setcolor(vga_entry_color(VGA_COLOR_MAGENTA, VGA_COLOR_BLACK));
  terminal_writestring("-------------------------------------------------------"
                       "-------------------------\n");
  terminal_setcolor(vga_entry_color(VGA_COLOR_CYAN, VGA_COLOR_BLACK));
  terminal_writestring(
      "   __  __         ____   _____    [ VIBE EDITION - v1.2 ]\n");
  terminal_writestring(
      "  |  \\/  |       / __ \\ / ____|   -----------------------\n");
  terminal_writestring(
      "  | \\  / |_   _ | |  | | (___     Status: VIBES OPTIMAL\n");
  terminal_writestring("  | |\\/| | | | || |  | |\\___ \\    System: ACTIVE\n");
  terminal_writestring("  | |  | | |_| || |__| |____) |   User:   NOBLE\n");
  terminal_writestring("  |_|  |_|\\__, | \\____/|_____/    \n");
  terminal_writestring("           __/ |                  \n");
  terminal_writestring("          |___/                   \n");
  terminal_setcolor(vga_entry_color(VGA_COLOR_MAGENTA, VGA_COLOR_BLACK));
  terminal_writestring("-------------------------------------------------------"
                       "-------------------------\n\n");

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
        start_file_manager();
        // Reset and draw prompt
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
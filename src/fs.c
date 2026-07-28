#include "fs.h"
#include "vga.h"
#include "ata.h"

// Our "Hard Drive" in RAM
File files[MAX_FILES];

// Function to compare strings (since we don't have a full C library yet)
int str_compare(const char *s1, const char *s2) {
  while (*s1 && (*s1 == *s2)) {
    s1++;
    s2++;
  }
  return *(unsigned char *)s1 - *(unsigned char *)s2;
}

// Initialize the file system with some "factory" files
void fs_init() {
  for (int i = 0; i < MAX_FILES; i++)
    files[i].exists = 0;

  // Create a default README file
  files[0].exists = 1;
  const char *name0 = "readme.txt";
  int i = 0;
  while (name0[i]) {
    files[0].name[i] = name0[i];
    i++;
  }
  files[0].name[i] = '\0';

  const char *msg = "Welcome to MyOS!\nThis is a file stored in RAM.";
  int j = 0;
  while (msg[j]) {
    files[0].content[j] = msg[j];
    j++;
  }
  files[0].content[j] = '\0';
  files[0].size = j;

  // Create a Vibe file
  files[1].exists = 1;
  const char *name1 = "vibe.txt";
  i = 0;
  while (name1[i]) {
    files[1].name[i] = name1[i];
    i++;
  }
  files[1].name[i] = '\0';

  const char *vibe_msg = "Vibe's goood.";
  j = 0;
  while (vibe_msg[j]) {
    files[1].content[j] = vibe_msg[j];
    j++;
  }
  files[1].content[j] = '\0';
  files[1].size = j;
}

// A simple function to list files (The "ls" command logic)
void fs_list() {
  terminal_writestring("Directory of /\n");
  for (int i = 0; i < MAX_FILES; i++) {
    if (files[i].exists) {
      terminal_setcolor(vga_entry_color(VGA_COLOR_LIGHT_CYAN, VGA_COLOR_BLACK));
      terminal_writestring("  > ");
      terminal_writestring(files[i].name);
      
      // Show size too!
      terminal_setcolor(vga_entry_color(VGA_COLOR_DARK_GREY, VGA_COLOR_BLACK));
      terminal_writestring(" (");
      // Note: We don't have itoa yet, but we can print a simple char if size is small
      // For now let's just use text
      terminal_writestring("file");
      terminal_writestring(")\n");
    }
  }
}

// Read and print file content (The "cat" command logic)
int fs_read(const char *name) {
  for (int i = 0; i < MAX_FILES; i++) {
    if (files[i].exists && str_compare(files[i].name, name) == 0) {
      terminal_writestring(files[i].content);
      terminal_writestring("\n");
      return 1;
    }
  }
  terminal_writestring("File not found: ");
  terminal_writestring(name);
  terminal_writestring("\n");
  return 0;
}

// Function to create a new empty file
void fs_create(const char *name) {
  for (int i = 0; i < MAX_FILES; i++) {
    // Look for an empty slot
    if (files[i].exists == 0) {
      files[i].exists = 1;
      files[i].size = 0;

      // Copy the filename
      int j = 0;
      while (name[j] != '\0' && j < MAX_FILENAME - 1) {
        files[i].name[j] = name[j];
        j++;
      }
      files[i].name[j] = '\0';

      // Initialize content to empty
      files[i].content[0] = '\0';

      terminal_writestring("Created file: ");
      terminal_writestring(name);
      terminal_writestring("\n");
      fs_save_to_disk(); // Auto-save filesystem changes to virtual hard disk
      return;
    }
  }
  terminal_writestring("Error: Filesystem full!\n");
}

// Function to write content to an existing file
void fs_write(const char *name, const char *content) {
  for (int i = 0; i < MAX_FILES; i++) {
    if (files[i].exists && str_compare(files[i].name, name) == 0) {
      int j = 0;
      while (content[j] != '\0' && j < MAX_FILE_SIZE - 1) {
        files[i].content[j] = content[j];
        j++;
      }
      files[i].content[j] = '\0';
      files[i].size = j;

      terminal_writestring("Saved data to: ");
      terminal_writestring(name);
      terminal_writestring("\n");
      fs_save_to_disk(); // Auto-save filesystem changes to virtual hard disk
      return;
    }
  }
  terminal_writestring("Error: File not found!\n");
}

// Function to delete a file
void fs_delete(const char *name) {
  for (int i = 0; i < MAX_FILES; i++) {
    if (files[i].exists && str_compare(files[i].name, name) == 0) {
      files[i].exists = 0;
      terminal_setcolor(vga_entry_color(VGA_COLOR_LIGHT_CYAN, VGA_COLOR_BLACK));
      terminal_writestring("Deleted file: ");
      terminal_writestring(name);
      terminal_writestring("\n");
      fs_save_to_disk(); // Auto-save filesystem changes to virtual hard disk
      return;
    }
  }
  terminal_writestring("Error: File not found!\n");
}

// Load files from the hard disk
void fs_load_from_disk(void) {
  uint16_t sector_buf[256];
  
  // Sector 0 contains the File System Magic Number (0x4D594F53 = "MYOS")
  ata_read_sector(0, sector_buf);
  uint32_t magic = (uint32_t)sector_buf[0] | ((uint32_t)sector_buf[1] << 16);
  
  if (magic == 0x4D594F53) {
    // Magic matches! Read sectors 1 to 34 sequentially into the files array.
    uint8_t *dest = (uint8_t*)files;
    uint32_t bytes_to_read = sizeof(files);
    uint32_t current_lba = 1;
    
    while (bytes_to_read > 0) {
      ata_read_sector(current_lba, sector_buf);
      uint32_t chunk = (bytes_to_read > 512) ? 512 : bytes_to_read;
      
      uint8_t *src = (uint8_t*)sector_buf;
      for (uint32_t i = 0; i < chunk; i++) {
        dest[i] = src[i];
      }
      
      dest += chunk;
      bytes_to_read -= chunk;
      current_lba++;
    }
    terminal_writestring("Disk storage detected. Loaded filesystem.\n");
  } else {
    // Disk uninitialized or magic check failed. 
    // Format the disk with defaults.
    terminal_writestring("Hard drive uninitialized. Formatting storage...\n");
    fs_init();
    fs_save_to_disk();
  }
}

// Save files to the hard disk
void fs_save_to_disk(void) {
  uint16_t sector_buf[256];
  
  // Prepare sector 0 with the filesystem magic number
  for (int i = 0; i < 256; i++) {
    sector_buf[i] = 0;
  }
  sector_buf[0] = 0x4F53; // "SO" in little endian
  sector_buf[1] = 0x4D59; // "MY" in little endian
  ata_write_sector(0, sector_buf);
  
  // Write the files array to sectors 1 to 34
  uint8_t *src = (uint8_t*)files;
  uint32_t bytes_to_write = sizeof(files);
  uint32_t current_lba = 1;
  
  while (bytes_to_write > 0) {
    uint32_t chunk = (bytes_to_write > 512) ? 512 : bytes_to_write;
    
    // Zero-initialize the temporary sector buffer
    uint8_t *dest = (uint8_t*)sector_buf;
    for (uint32_t i = 0; i < 512; i++) {
      dest[i] = 0;
    }
    // Copy the files chunk data into the sector buffer
    for (uint32_t i = 0; i < chunk; i++) {
      dest[i] = src[i];
    }
    
    ata_write_sector(current_lba, sector_buf);
    
    src += chunk;
    bytes_to_write -= chunk;
    current_lba++;
  }
}

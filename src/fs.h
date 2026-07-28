#ifndef FS_H
#define FS_H

#include <stdint.h>

#define MAX_FILES 16
#define MAX_FILENAME 32
#define MAX_FILE_SIZE 1024

// This structure defines what a file is in our RAM
typedef struct {
    char name[MAX_FILENAME];    // The name of the file (e.g., "hello.txt")
    char content[MAX_FILE_SIZE]; // The actual text inside the file
    uint32_t size;              // How many characters are in the file
    uint8_t exists;             // A simple flag: 1 if file exists, 0 if empty slot
} File;

extern File files[MAX_FILES];

// Functions we'll use to interact with files
void fs_init();
void fs_list();
int fs_read(const char* name);
void fs_create(const char* name);
void fs_write(const char* name, const char* content);
void fs_delete(const char* name);
int str_compare(const char* s1, const char* s2);

// Disk persistence functions
void fs_load_from_disk(void);
void fs_save_to_disk(void);

#endif

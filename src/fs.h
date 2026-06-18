#ifndef FS_H
#define FS_H

#include <stdint.h>

#define MAX_FILES 16
#define MAX_FILENAME 32
#define MAX_FILE_SIZE 256

// This structure defines what a file is in our RAM
typedef struct {
    char name[MAX_FILENAME];    // The name of the file (e.g., "hello.txt")
    char content[MAX_FILE_SIZE]; // The actual text inside the file
    uint32_t size;              // How many characters are in the file
    uint8_t exists;             // A simple flag: 1 if file exists, 0 if empty slot
} File;

// Functions we'll use to interact with files
void fs_init();
void fs_list();
int fs_read(const char* name);
int str_compare(const char* s1, const char* s2);

#endif

#include "fs.h"
#include "vga.h"

// Our "Hard Drive" in RAM
File files[MAX_FILES];

// Function to compare strings (since we don't have a full C library yet)
int str_compare(const char* s1, const char* s2) {
    while (*s1 && (*s1 == *s2)) {
        s1++;
        s2++;
    }
    return *(unsigned char*)s1 - *(unsigned char*)s2;
}

// Initialize the file system with some "factory" files
void fs_init() {
    for (int i = 0; i < MAX_FILES; i++) files[i].exists = 0;

    // Create a default README file
    files[0].exists = 1;
    const char* name0 = "readme.txt";
    int i = 0;
    while(name0[i]) { files[0].name[i] = name0[i]; i++; }
    files[0].name[i] = '\0';

    const char* msg = "Welcome to MyOS!\nThis is a file stored in RAM.";
    int j = 0;
    while(msg[j]) {
        files[0].content[j] = msg[j];
        j++;
    }
    files[0].content[j] = '\0';
    files[0].size = j;

    // Create a Vibe file
    files[1].exists = 1;
    const char* name1 = "vibe.txt";
    i = 0;
    while(name1[i]) { files[1].name[i] = name1[i]; i++; }
    files[1].name[i] = '\0';

    const char* vibe_msg = "Stay chill and keep coding.";
    j = 0;
    while(vibe_msg[j]) {
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
            terminal_writestring(" - ");
            terminal_writestring(files[i].name);
            terminal_writestring("\n");
        }
    }
}

// Read and print file content (The "cat" command logic)
int fs_read(const char* name) {
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

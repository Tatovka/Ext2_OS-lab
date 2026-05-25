#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>

int main(int argc, char** argv) {
    char* filePath = argv[1];
    
    FILE* file = fopen(filePath, "rb");
    if (file == NULL) {
        perror("unable to open file");
        return 1;
    }

    fseek(file, 0, SEEK_END);
    int filesz = ftell(file);
    fseek(file, 0, SEEK_SET);

    char* buf = malloc(filesz);
    if (buf == NULL) {
        perror("failed to allocate buffer");
        exit(1);
    }
    
    fread(buf, 1, filesz, file);

    int entryHead = 0;

    const char* types[] = {
        "Unknown type",
        "Regular file",
        "Directory",
        "Character device",
        "Block device",
        "FIFO",
        "Socket",
        "Symbolic link"
    };

    while(entryHead < filesz) {
        char* entry = buf + entryHead;

        int inodeNumber = le32toh(((int*) entry) [0]);
        short entryLen = le16toh(((short*) entry) [2]);
        if (entryLen == 0) return 0;
        char nameLen = entry[6];
        char fileType = entry[7];

        memcpy(entry, entry + 8, nameLen); //reuse read space
        entry[nameLen] = 0;
        
        printf("%i %s - %s\n", inodeNumber, entry, types[fileType]);

        entryHead += entryLen;
    }
    fclose(file);
    free(buf);
    return 0;
}
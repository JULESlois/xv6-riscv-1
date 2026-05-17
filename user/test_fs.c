// test_fs.c - Demo for File System
// Task 6: File System - Structured Parsing of Persistent Data

#include "kernel/types.h"
#include "kernel/stat.h"
#include "kernel/spinlock.h"
#include "kernel/sleeplock.h"
#include "kernel/fs.h"
#include "kernel/file.h"
#include "user/user.h"

void test_filesystem(void) {
    printf("\n=== Task 6: File System Demo ===\n\n");

    printf("1. Log-structured File System (log.c):\n");
    printf("   - Write-ahead logging: write log before actual operation\n");
    printf("   - begin_op()/end_op(): mark transaction boundaries\n");
    printf("   - Log commit ensures crash recovery\n\n");

    printf("2. Superblock (fs.c:30-45):\n");
    printf("   - superblock contains filesystem metadata\n");
    printf("   - s_magic, s_size, s_nblocks, s_ninodes\n\n");

    printf("3. Inode Structure (fs.h:25-50):\n");
    printf("   - inode is core data structure for files/directories\n");
    printf("   - addrs[13]: data block address array\n");
    printf("   - dinode: on-disk inode format\n\n");

    printf("4. Directory Parsing (fs.c:200-250):\n");
    printf("   - namei(path): resolve path to inode\n");
    printf("   - dirlookup(dp, name): look up directory entry\n");
    printf("   - dirlink(dp, name, inum): create directory entry\n\n");

    printf("5. File Operations:\n");
    printf("   - open(): open file, get file descriptor\n");
    printf("   - read(fd, buf, n): read from file\n");
    printf("   - write(fd, buf, n): write to file\n\n");

    printf("6. File Read Test:\n");
    int fd = open("README", 0);
    if (fd >= 0) {
        char buf[128];
        int n = read(fd, buf, 64);
        if (n > 0) {
            buf[n] = '\0';
            printf("   - Successfully read %d bytes\n", n);
            printf("   - First 64 bytes: %.64s...\n", buf);
        }
        close(fd);
    } else {
        printf("   - Failed to open README\n");
    }

    printf("\n7. Directory Operation Test:\n");
    printf("   - Use 'ls' command to view file list\n\n");

    printf("=== Demo Complete ===\n");
    exit(0);
}

int main(void) {
    test_filesystem();
    return 0;
}
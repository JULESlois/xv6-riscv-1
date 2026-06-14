#include "kernel/types.h"
#include "kernel/stat.h"
#include "kernel/spinlock.h"
#include "kernel/sleeplock.h"
#include "kernel/fs.h"
#include "kernel/file.h"
#include "kernel/fcntl.h"
#include "user/user.h"

int
main(void)
{
  int pid;                         // Child process id returned by fork.
  int status;                      // Exit status collected from the test process.
  char *argv[] = { "test_fs", 0 }; // exec argument vector terminated by a null pointer.

  if(open("console", O_RDWR) < 0){ // Open the console device; create it if needed.
    mknod("console", CONSOLE, 0);  // Create the console device node in the root directory.
    open("console", O_RDWR);       // Reopen console as file descriptor 0.
  }
  dup(0);                          // Duplicate stdin to stdout.
  dup(0);                          // Duplicate stdin to stderr.

  printf("init: starting test_fs\n"); // Announce the automatic validation program.
  pid = fork();                    // Create a real user process for the test program.
  if(pid < 0){                     // A negative return means fork failed.
    printf("init: fork failed\n"); // Print the failure before exiting.
    exit(1);                       // Return a failing status to the kernel.
  }
  if(pid == 0){                    // The child process sees fork return zero.
    exec("test_fs", argv);          // Replace the child image with the test ELF from fs.img.
    printf("init: exec test_fs failed\n"); // exec returns only on failure.
    exit(1);                       // Report the exec failure.
  }

  wait(&status);                   // Wait for the test process to finish.
  printf("init: test_fs exited with status %d\n", status); // Print the test exit status.
  for(;;){                         // init must stay alive after the test exits.
    pause(100);                    // Sleep to avoid a busy loop.
  }
}

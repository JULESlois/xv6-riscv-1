#include "kernel/types.h"
#include "kernel/stat.h"
#include "kernel/spinlock.h"
#include "kernel/sleeplock.h"
#include "kernel/fs.h"
#include "kernel/file.h"
#include "kernel/fcntl.h"
#include "user/user.h"

static void
run(char *name)
{
  int pid;                         // Child process id returned by fork.
  int status;                      // Exit status collected from the child process.
  char *argv[] = { name, 0 };      // exec argument vector terminated by a null pointer.

  printf("init: starting %s\n", name); // Announce the user program being launched.
  pid = fork();                    // Create a real user process for the target program.
  if(pid < 0){                     // A negative return means fork failed.
    printf("init: fork failed\n"); // Print the failure before returning to the caller.
    return;                        // Keep init alive even if one fork fails.
  }
  if(pid == 0){                    // The child process sees fork return zero.
    exec(name, argv);              // Replace the child image with the named ELF from fs.img.
    printf("init: exec %s failed\n", name); // exec returns only on failure.
    exit(1);                       // Report the exec failure.
  }
  wait(&status);                   // Wait for the launched program to finish.
  printf("init: %s exited with status %d\n", name, status); // Print the program exit status.
}

static void
shell_loop(void)
{
  int pid;                         // Process id of the current shell.
  int wpid;                        // Process id returned by wait.
  int status;                      // Exit status collected from wait.
  char *argv[] = { "sh", 0 };      // Argument vector used to start the xv6 shell.

  for(;;){                         // Keep a command loop available forever.
    printf("init: starting shell\n"); // Announce each shell start.
    pid = fork();                  // Create a child that will become the shell.
    if(pid < 0){                   // A negative return means fork failed.
      printf("init: fork shell failed\n"); // Report shell creation failure.
      pause(100);                  // Sleep before retrying to avoid a busy loop.
      continue;                    // Retry shell creation.
    }
    if(pid == 0){                  // The shell child sees fork return zero.
      exec("sh", argv);            // Load the real xv6 shell from fs.img.
      printf("init: exec sh failed\n"); // exec returns only if the shell is missing.
      exit(1);                     // Report shell exec failure.
    }
    while((wpid = wait(&status)) >= 0 && wpid != pid) // Reap any orphan before the shell exits.
      printf("init: reaped orphan pid %d status %d\n", wpid, status); // Show orphan cleanup.
    printf("init: shell exited with status %d\n", status); // Report shell exit status.
  }
}

int
main(void)
{
  if(open("console", O_RDWR) < 0){ // Open the console device; create it if needed.
    mknod("console", CONSOLE, 0);  // Create the console device node in the root directory.
    open("console", O_RDWR);       // Reopen console as file descriptor 0.
  }
  dup(0);                          // Duplicate stdin to stdout.
  dup(0);                          // Duplicate stdin to stderr.

  run("test_fs");                  // Run the file-system validation once at boot.
  shell_loop();                    // Enter the interactive user-program launcher.
}

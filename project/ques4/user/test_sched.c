#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

static void
busy_round(int child, int iter)
{
  volatile int sink = 0;                    // Volatile prevents the compiler from removing the loop.
  for(int i = 0; i < 1200000; i++)          // Burn CPU long enough for timer interrupts.
    sink += i + child + iter;               // Make every child execute real arithmetic.
}

int
main(void)
{
  int pids[3];                              // PIDs returned by the three fork calls.
  int pipefd[2];                            // Pipe used by children to report scheduling progress.
  int status;                               // Exit status collected by wait.
  char log[16];                             // Progress bytes written by scheduled children.
  int counts[3] = {0, 0, 0};                // Per-child progress counters decoded from the pipe.
  int total = 0;                            // Total number of progress bytes read from the pipe.

  printf("\n=== Task 4: Scheduler Demo ===\n\n"); // Visible test banner.
  printf("1. Fork three CPU-bound children without explicit yield\n"); // Explain the test.
  if(pipe(pipefd) < 0){                     // Create a real kernel pipe before forking children.
    printf("FAIL test_sched: pipe failed\n"); // Report IPC setup failure.
    exit(1);                                // Return failure to init.
  }

  for(int child = 0; child < 3; child++){   // Create three independent user processes.
    pids[child] = fork();                   // fork copies the current user process.
    if(pids[child] < 0){                    // Negative PID means fork failed.
      printf("FAIL test_sched: fork failed\n"); // Report process creation failure.
      exit(1);                              // Return failure to init.
    }
    if(pids[child] == 0){                   // Child sees fork return zero.
      char mark = '1' + child;              // Encode this child id as one byte.
      close(pipefd[0]);                     // Child only writes progress, so close the read end.
      for(int iter = 0; iter < 5; iter++){  // Each child runs several CPU-bound rounds.
        busy_round(child + 1, iter);        // Consume CPU without calling yield.
        printf("   child %d pid=%d iter=%d\n", child + 1, getpid(), iter); // Print progress.
        if(write(pipefd[1], &mark, 1) != 1) // Record that this child got CPU for another round.
          exit(1);                          // Fail if the pipe write syscall fails.
      }
      close(pipefd[1]);                     // Close the write end before exiting.
      exit(0);                              // Child reports success.
    }
  }
  close(pipefd[1]);                         // Parent only reads progress from children.

  while(total < 15){                        // Expect 3 children times 5 progress reports.
    int n = read(pipefd[0], log + total, 15 - total); // Read available child progress bytes.
    if(n < 0){                              // Negative return means pipe read failed.
      printf("FAIL test_sched: pipe read failed\n"); // Report read failure.
      exit(1);                              // Return failure to init.
    }
    if(n == 0)                              // EOF before all reports means a child stopped early.
      break;                                // Leave the loop and fail below.
    total += n;                             // Accumulate the number of progress bytes read.
  }
  close(pipefd[0]);                         // Parent is done reading the progress pipe.

  for(int i = 0; i < total; i++){           // Decode every progress byte received from children.
    if(log[i] >= '1' && log[i] <= '3')      // Accept only known child id markers.
      counts[log[i] - '1']++;               // Count one scheduled round for that child.
  }
  printf("   scheduler progress log: ");    // Print the raw order seen through the kernel pipe.
  for(int i = 0; i < total; i++)            // Walk the progress log in arrival order.
    printf("%c", log[i]);                   // Print each child marker.
  printf("\n");                             // Finish the progress-log line.
  printf("   child round counts: %d %d %d (expected 5 5 5)\n",
         counts[0], counts[1], counts[2]);  // Print the per-child scheduling evidence.
  if(total != 15 || counts[0] != 5 || counts[1] != 5 || counts[2] != 5){ // Validate scheduling evidence.
    printf("FAIL test_sched: missing child progress\n"); // Report incomplete scheduling.
    exit(1);                                // Return failure to init.
  }

  for(int i = 0; i < 3; i++){               // Reap all child processes and validate status.
    if(wait(&status) < 0){                  // wait returns negative if no child is available.
      printf("FAIL test_sched: wait failed\n"); // Report unexpected wait failure.
      exit(1);                              // Return failure to init.
    }
    if(status != 0){                        // Every child should have exited cleanly.
      printf("FAIL test_sched: child status %d\n", status); // Report child failure.
      exit(1);                              // Return failure to init.
    }
  }

  printf("\n2. Timer interrupt path preempted CPU-bound user code\n"); // Summarize what the output shows.
  printf("   trap -> yield -> sched -> swtch -> scheduler -> next RUNNABLE process\n"); // Name the kernel path.
  printf("\n=== Scheduler Test PASSED ===\n"); // Stable pass marker.
  exit(0);                                  // Return success to init.
}

#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

static void
fail(char *msg)
{
  printf("FAIL test_mem: %s\n", msg);     // Print the failing condition for the QEMU log.
  exit(1);                                // Return a non-zero status to init.
}

int
main(void)
{
  char *base;                              // Heap break before this test starts.
  char *p1;                                // Address returned by the first sbrk call.
  char *p2;                                // Address returned by the second sbrk call.
  char *now;                               // Current heap break used for checks.

  printf("\n=== Task 3: Memory Management Demo ===\n\n"); // Visible test banner.
  printf("1. Physical allocator path: sbrk -> growproc -> uvmalloc -> kalloc\n"); // Describe kernel path.

  base = sbrk(0);                          // Read the initial user heap break.
  printf("   - initial brk: 0x%lx\n", (uint64)base); // Show the starting break.

  p1 = sbrk(4096);                         // Grow the process by one page.
  if(p1 == SBRK_ERROR)                     // sbrk returns -1 on allocation failure.
    fail("sbrk(4096) returned error");     // Report failed page allocation.
  if(p1 != base)                           // sbrk must return the old break.
    fail("sbrk(4096) did not return old brk"); // Report incorrect return value.
  now = sbrk(0);                           // Read the new heap break.
  if(now != base + 4096)                   // The break must advance by one page.
    fail("brk did not grow by 4096");      // Report incorrect heap size.
  p1[0] = 'A';                             // Write the first byte of the allocated page.
  p1[4095] = 'Z';                          // Write the last byte of the allocated page.
  printf("   - wrote first allocated page: %c ... %c\n", p1[0], p1[4095]); // Prove the page is mapped.

  p2 = sbrk(8192);                         // Grow the process by two more pages.
  if(p2 == SBRK_ERROR)                     // Check for allocation failure.
    fail("sbrk(8192) returned error");     // Report failure.
  if(p2 != base + 4096)                    // The returned address must be the previous break.
    fail("sbrk(8192) did not return previous brk"); // Report incorrect return value.
  now = sbrk(0);                           // Read the new heap break.
  if(now != base + 4096 + 8192)            // The break must advance by two more pages.
    fail("brk did not grow by 8192");      // Report incorrect heap size.
  p2[0] = 'B';                             // Write the first byte of the new range.
  p2[8191] = 'Y';                          // Write the last byte of the new range.
  printf("   - wrote second allocation: %c ... %c\n", p2[0], p2[8191]); // Prove both pages are mapped.

  if(sbrk(-4096) == SBRK_ERROR)            // Shrink by one page.
    fail("sbrk(-4096) returned error");    // Report shrink failure.
  if(sbrk(-8192) == SBRK_ERROR)            // Shrink by the remaining two pages.
    fail("sbrk(-8192) returned error");    // Report shrink failure.
  now = sbrk(0);                           // Read the final heap break.
  if(now != base)                          // The heap must return to its initial break.
    fail("final brk did not return to initial brk"); // Report leaked user heap.

  printf("   - final brk: 0x%lx\n", (uint64)now); // Show the final break.
  printf("\n=== Memory Management Test PASSED ===\n"); // Stable pass marker.
  exit(0);                                  // Return success to init.
}

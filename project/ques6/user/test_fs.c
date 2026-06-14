#include "kernel/types.h"
#include "kernel/stat.h"
#include "kernel/elf.h"
#include "user/user.h"

static uchar *image;                        // Buffer holding a complete file read from fs.img.
static int image_size;                      // Number of bytes currently stored in image.

static uint
le32(const uchar *p)
{
  return ((uint)p[0]) | ((uint)p[1] << 8) | ((uint)p[2] << 16) | ((uint)p[3] << 24); // Decode little-endian u32.
}

static uint64
le64(const uchar *p)
{
  return ((uint64)le32(p)) | ((uint64)le32(p + 4) << 32); // Decode little-endian u64.
}

static ushort
le16(const uchar *p)
{
  return ((ushort)p[0]) | ((ushort)p[1] << 8); // Decode little-endian u16.
}

static int
read_file(char *name)
{
  int fd;                                    // File descriptor returned by open.
  struct stat st;                            // File metadata returned by fstat.
  int n;                                     // Number of bytes read by one read syscall.
  int off = 0;                               // Current buffer offset while reading the file.

  fd = open(name, 0);                        // Open the named file from the real xv6 file system.
  if(fd < 0){                                // Negative fd means open failed.
    printf("FAIL test_fs: open %s failed\n", name); // Report the missing file.
    exit(1);                                 // Return failure to init.
  }
  if(fstat(fd, &st) < 0){                    // Ask the xv6 file system for the file size.
    printf("FAIL test_fs: fstat %s failed\n", name); // Report metadata failure.
    close(fd);                               // Close the descriptor before failing.
    exit(1);                                 // Return failure to init.
  }
  if(st.size <= 0){                          // A useful positive or negative test file must have bytes.
    printf("FAIL test_fs: %s is empty\n", name); // Report an unusable file.
    close(fd);                               // Close the descriptor before failing.
    exit(1);                                 // Return failure to init.
  }
  if(st.size > 65536){                       // Keep the user buffer small enough for xv6 test memory.
    printf("FAIL test_fs: %s too large: %lu\n", name, st.size); // Report an oversized test file.
    close(fd);                               // Close the descriptor before failing.
    exit(1);                                 // Return failure to init.
  }
  if(image)                                  // If a previous file buffer exists.
    free(image);                             // Release it before allocating the next one.
  image_size = (int)st.size;                 // Convert the checked file size to an int for read calls.
  image = malloc(image_size);                // Allocate enough user memory for the complete file.
  if(image == 0){                            // Null means the user heap allocation failed.
    printf("FAIL test_fs: malloc %d bytes failed\n", image_size); // Report allocation failure.
    close(fd);                               // Close the descriptor before failing.
    exit(1);                                 // Return failure to init.
  }
  while(off < image_size){                   // Read until the whole file has been copied.
    n = read(fd, image + off, image_size - off); // Read the remaining file bytes into the buffer.
    if(n <= 0){                              // EOF or negative error before full size is unexpected.
      printf("FAIL test_fs: short read %s at %d/%d\n", name, off, image_size); // Report truncated read.
      close(fd);                             // Close the descriptor before failing.
      exit(1);                               // Return failure to init.
    }
    off += n;                                // Advance past the bytes just read.
  }
  close(fd);                                 // Close the descriptor after reading.
  return image_size;                         // Return the full number of bytes available for parsing.
}

static int
parse_elf(char *name, int expect_ok)
{
  int n;                                     // Number of bytes read from the file.
  uint magic;                                // ELF magic decoded from file bytes.
  uint64 entry;                              // ELF entry address.
  uint64 phoff;                              // Program header table file offset.
  ushort phentsize;                          // Program header entry size.
  ushort phnum;                              // Program header count.
  int loads = 0;                             // Number of PT_LOAD entries found.

  n = read_file(name);                       // Read real file bytes from fs.img.
  if(n < (int)sizeof(struct elfhdr)){        // The ELF header must fit in the read buffer.
    if(expect_ok)                            // A real ELF file should not be this short.
      return -1;                             // Report parse failure.
    if(n < 4)                                // Even a negative test should prove real bytes were read.
      return -1;                             // Report an invalid negative-test file.
    printf("   - %s rejected as short non-ELF file, size=%d\n", name, n); // Show short-file rejection.
    return 0;                                // A short non-ELF file is an expected rejection.
  }

  magic = le32(image);                       // Decode the ELF magic from byte zero.
  if(magic != ELF_MAGIC){                    // Reject files that are not ELF executables.
    if(expect_ok)                            // The caller expected an executable.
      return -1;                             // Report parse failure.
    printf("   - %s rejected as non-ELF, magic=0x%x\n", name, magic); // Show negative test.
    return 0;                                // Non-ELF rejection succeeded.
  }

  entry = le64(image + 24);                  // Decode e_entry.
  phoff = le64(image + 32);                  // Decode e_phoff.
  phentsize = le16(image + 54);              // Decode e_phentsize.
  phnum = le16(image + 56);                  // Decode e_phnum.
  printf("   - %s ELF entry=0x%lx phoff=%lu phnum=%d\n", name, entry, phoff, phnum); // Print header summary.

  if(phentsize < sizeof(struct proghdr))     // Program header entries must hold a full proghdr.
    return -1;                               // Reject malformed table layout.
  for(int i = 0; i < phnum; i++){            // Walk each program header in the table.
    uint64 off = phoff + (uint64)i * phentsize; // Compute this entry's byte offset.
    if(off + sizeof(struct proghdr) > (uint64)n) // Ensure the entry is inside the bytes read.
      return -1;                             // Report truncated program header table.
    uint type = le32(image + off);           // Decode p_type.
    uint flags = le32(image + off + 4);      // Decode p_flags.
    uint64 seg_off = le64(image + off + 8);  // Decode p_offset.
    uint64 vaddr = le64(image + off + 16);   // Decode p_vaddr.
    uint64 filesz = le64(image + off + 32);  // Decode p_filesz.
    uint64 memsz = le64(image + off + 40);   // Decode p_memsz.
    if(type == ELF_PROG_LOAD){               // Count loadable program segments.
      loads++;                               // Remember that at least one loadable segment exists.
      printf("   - PH[%d] LOAD off=0x%lx vaddr=0x%lx filesz=0x%lx memsz=0x%lx flags=0x%x\n",
             i, seg_off, vaddr, filesz, memsz, flags); // Print the real segment fields.
    }
  }
  return loads > 0 ? 0 : -1;                 // Succeed only if a loadable segment was found.
}

int
main(void)
{
  printf("\n=== Task 6: File System / ELF Parsing Demo ===\n\n"); // Visible test banner.
  printf("1. Read a real ELF executable from fs.img and parse Program Headers\n"); // Describe positive test.
  if(parse_elf("test_fs", 1) < 0){           // Parse this running test's executable file.
    printf("FAIL test_fs: ELF parse failed\n"); // Report parse failure.
    exit(1);                                 // Return failure to init.
  }

  printf("\n2. Read a real non-ELF file and reject it\n"); // Describe negative test.
  if(parse_elf("README", 0) < 0){            // README exists in fs.img but is not ELF.
    printf("FAIL test_fs: README rejection failed\n"); // Report negative-test failure.
    exit(1);                                 // Return failure to init.
  }

  printf("\n=== File System Parser Test PASSED ===\n"); // Stable pass marker.
  exit(0);                                  // Return success to init.
}

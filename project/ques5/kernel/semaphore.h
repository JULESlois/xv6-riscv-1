#define NSEM 16

struct semaphore {
  struct spinlock lock;  // Protects used, value, and all sleepers on this object.
  int used;              // Non-zero after sem_init publishes this slot.
  int value;             // Number of permits currently available.
  char name[16];         // Stable lock name storage for diagnostics.
};

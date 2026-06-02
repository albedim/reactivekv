/**
 * @file kvdb_store.c
 * @brief In-memory key-value store implementation
 * @details Manages the core storage operations and thread synchronization
 * 
 * @author albedim <Alberto Di Maio> <https://github.com/albedim> <https://albertodimaio.com>
 * @version 1.0
 * @date 2026
 */

#include "kvdb.h"

/** 
 * @var store 
 * Global key-value store array (statically allocated)
 * Maximum 100 key-value pairs can be stored simultaneously.
 * Grows incrementally as new keys are added.
 * @see size for tracking current entry count
 */
Entry store[100];

/** 
 * @var size 
 * Current number of entries in the store.
 * Incremented when a new key is added, not decremented on updates.
 * Used to find the next free slot when adding new entries.
 * Range: 0 to 100 (prevents overflow)
 * @see set() for how size is managed
 */
int size = 0;

/**
 * @var lock
 * Mutex lock for thread-safe access to the global store.
 *
 * ============================================================================
 * WHAT ARE THREADS IN THIS PROJECT?
 * ============================================================================
 *
 * A thread is a lightweight execution unit within a single process. Multiple
 * threads can run simultaneously (concurrently), all sharing the same memory
 * space and global variables.
 *
 * In this database server:
 * - Each client connection is handled by a separate thread
 * - Thread 1 handles Client A (executes SET/GET commands)
 * - Thread 2 handles Client B (executes SET/GET commands)
 * - Thread 3 handles Client C (executes SET/GET commands)
 * - etc.
 *
 * All these threads execute at the SAME TIME (not sequentially), and they all
 * read and write to the SAME shared store[] array.
 *
 * ============================================================================
 * WHY DO WE NEED THREADS?
 * ============================================================================
 *
 * Without threads, the server would be single-threaded:
 * - Accept connection from Client A
 * - Handle ALL requests from Client A (read/write commands)
 * - Close Client A connection
 * - THEN accept Client B
 * - Handle ALL requests from Client B
 * - Result: Only one client at a time. Other clients must wait.
 *
 * With threads, the server is multi-threaded:
 * - Accept connection from Client A -> spawn Thread 1
 * - Accept connection from Client B -> spawn Thread 2
 * - Accept connection from Client C -> spawn Thread 3
 * - All three threads handle requests simultaneously
 * - Result: Many clients can use the server at the same time (high concurrency)
 *
 * ============================================================================
 * WHAT IS THE PTHREAD LIBRARY?
 * ============================================================================
 *
 * "pthread" = POSIX Threads
 * - POSIX = Portable Operating System Interface (standard API)
 * - Threads = concurrent execution units
 *
 * pthread is a library provided by Unix-like systems (Linux, macOS, WSL) that
 * provides functions to create, manage, and synchronize threads in C programs.
 *
 * Key pthread functions used in this project:
 * - pthread_mutex_init()   : Initialize a mutex
 * - pthread_mutex_lock()   : Acquire lock (block if another thread holds it)
 * - pthread_mutex_unlock() : Release lock
 *
 * ============================================================================
 * WHY DO WE NEED THE LOCK VARIABLE?
 * ============================================================================
 *
 * Problem: Race Condition
 *
 * Without lock, multiple threads can access store[] simultaneously:
 *
 * Thread A: reads size = 5
 * Thread B: reads size = 5
 * Thread A: writes new key at index 5, then increments size to 6
 * Thread B: also writes new key at index 5 (OVERWRITES Thread A's data!)
 * Thread B: increments size to 6
 *
 * Result:
 * - One key-value pair is LOST (overwritten)
 * - store_size is inconsistent
 * - Database is corrupted
 *
 * Solution: Mutex Lock
 *
 * A mutex (mutual exclusion) ensures only ONE thread can access store[] at
 * a time. This prevents simultaneous read-write operations:
 *
 * Thread A: locks mutex (acquires exclusive access)
 * Thread A: reads size = 5, writes at index 5, increments size to 6
 * Thread A: unlocks mutex
 * Thread B: blocks waiting for mutex to unlock
 * Thread B: acquires lock
 * Thread B: reads size = 6, writes at index 6, increments size to 7
 * Thread B: unlocks mutex
 *
 * Result:
 * - Both operations complete safely, one after another
 * - No data loss or corruption
 *
 * Critical Usage Rules:
 *
 * 1. ALWAYS lock before modifying store[]:
 *    pthread_mutex_lock(&lock);
 *    // modify store
 *    pthread_mutex_unlock(&lock);
 *
 * 2. Forgetting to lock -> race condition, corruption (CRITICAL BUG)
 *
 * 3. Forgetting to unlock -> deadlock (server freezes forever)
 *
 * 4. Lock in correct order -> protect ALL access to store[]
 *
 * @warning This lock is REQUIRED. Every access to store[] must be protected
 * by lock/unlock, or the database will corrupt under concurrent load.
 *
 * @see set(), get() for correct locking patterns
 */
pthread_mutex_t lock;

/**
 * @brief Store or update a key-value pair in the database.
 * @param k The key to set (max 64 characters)
 * @param v The value to store (max 64 characters)
 * @details If key exists, updates the value. Otherwise, creates new entry.
 *          Thread-safe with mutex locking.
 *
 * @note TWO UNLOCK CALLS - WHY?
 *
 * This function has two separate unlock calls because it has two different
 * execution paths that require unlocking before returning in both cases.
 *
 * CRITICAL: Both paths must unlock before exiting, or other threads will
 * deadlock waiting for the lock forever.
 *
 * If we forgot either unlock:
 * - Lock never released
 * - Other threads block indefinitely trying to lock
 * - Server becomes unresponsive
 *
 * The early return in PATH 1 is why we need the first unlock - otherwise
 * the second unlock would never be reached, leaving the lock held.
 */
void set(const char* k, const char* v) {
  pthread_mutex_lock(&lock);

  for (int i = 0; i < size; i++) {
    if (strcmp(store[i].key, k) == 0) {
      strcpy(store[i].value, v);
      pthread_mutex_unlock(&lock);
      return;
    }
  }

  strcpy(store[size].key, k);
  strcpy(store[size].value, v);
  size++;

  pthread_mutex_unlock(&lock);
}

/**
 * @brief Retrieve a value from the database by key.
 * @param k The key to look up
 * @return Pointer to the value string, or NULL if key not found
 * @note Caller should not modify returned pointer
 */
char* get(const char* k) {
  for (int i = 0; i < size; i++) {
    if (strcmp(store[i].key, k) == 0)
      return store[i].value;
  }
  return NULL;
}

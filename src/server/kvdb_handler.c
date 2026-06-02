/**
 * @file kvdb_handler.c
 * @brief Client connection handler implementation
 * @details Per-client thread logic for reading commands and sending responses
 * 
 * @author albedim <Alberto Di Maio> <https://github.com/albedim> <https://albertodimaio.com>
 * @version 1.0
 * @date 2026
 */

#include "kvdb.h"

/**
 * @brief Handle a single client connection in a dedicated thread.
 * 
 * This function is executed in its own thread for each connected client.
 * It reads commands from the client socket, processes them, and sends responses.
 * The function runs until the client disconnects or an error occurs.
 * 
 * @param arg Opaque pointer to an allocated int* containing the client socket FD.
 *            Will be freed within this function using free().
 * @return Always returns NULL (void pointer requirement for pthread_create)
 * 
 * @details
 *   Command Processing:
 *   1. Read raw client input into buf[]
 *   2. Parse command string using sscanf(buf, "%s %s %s", cmd, key, val)
 *   3. Execute command and write response back to client:
 *      - SET <key> <value>  -> Calls set(), responds "OK\n"
 *      - GET <key>          -> Calls get(), responds <value>\n or NULL\n
 *      - PING               -> Responds "PONG\n" + prints debug msg
 *      - <anything else>    -> Responds "UNKNOWN\n"
 * 
 *   Connection Lifecycle:
 *   - Receives client fd via arg pointer
 *   - Immediately frees arg pointer
 *   - Loops until read() returns <= 0 (client disconnect)
 *   - Closes socket and thread exits naturally
 * 
 *   Buffer Handling:
 *   - Input buffer: BUF=1024 bytes
 *   - Command, key, value: 16, 64, 64 bytes respectively
 *   - Null-terminates all strings for safe processing
 * 
 * @thread_safety YES - Each thread has its own stack variables and socket fd.
 *                All shared store access is protected by mutex in set().
 * 
 * @note Thread is created DETACHED, so it cleans up automatically on exit.
 * @warning Resource leak if client.fd not properly closed (handled here)
 * 
 * @example
 *   // Client sends: "SET mykey myvalue\n"
 *   // Thread reads and parses: cmd="SET", key="mykey", val="myvalue"
 *   // Thread calls: set("mykey", "myvalue")
 *   // Thread responds: "OK\n" back to client
 * 
 * @see set() and get() for command implementations
 * @see main() for thread creation
 */
void* client_handler(void* arg) {
    // The 'arg' pointer holds a temporary memory address sent by the main thread.
    // Inside it, there is the unique ID (fd) assigned to this specific client.
    // We convert 'arg' back to an integer pointer, read the ID, and save it in 'fd'.
    int fd = *((int *) arg);

    // We no longer need the 'arg' pointer after extracting the fd, so we free the allocated memory.
    free(arg);

    // CREATE A PRIVATE INPUT BUFFER
    // This 1024-byte array lives on this thread's private stack.
    // It will hold the raw text data sent by this specific client.
    // Because it is local, other client threads cannot see or overwrite it.
    char buf[BUF];

    // MAIN LOOP - KEEP TALKING TO THE CLIENT
    // This loop runs forever until the client decides to close the connection 
    // or a network error occurs.
    //
    while (1) {
        /* BLOCKING CALL: The thread completely freezes and goes to sleep right here.
         * It consumes 0% CPU while waiting. The Operating System will wake it up ONLY when:
         * a) The client sends data (r > 0 bytes received)
         * b) The client disconnects gracefully (r == 0)
         * c) The connection drops due to an error (r < 0)
         * We read up to BUF-1 (1023 bytes) to guarantee space for the string terminator.
         */
        int r = read(fd, buf, BUF - 1);
        
        if (r <= 0) break;

        buf[r] = '\0';

        printf("%s", buf);

        // We need memset to clear the command, key, and value buffers before parsing new input.
        char cmd[16], key[64], val[64];
        memset(cmd, 0, sizeof(cmd));
        memset(key, 0, sizeof(key));
        memset(val, 0, sizeof(val));

        // Split the text from the buffer into up to 3 separate words: e.g., "SET" "mykey" "myval" */
        sscanf(buf, "%s %s %s", cmd, key, val);

        if (strcmp(cmd, "SET") == 0) {
            set(key, val);
            write(fd, "OK\n", 3);
        }
        else if (strcmp(cmd, "GET") == 0) {
            char *v = get(key);
            if (v) write(fd, v, strlen(v));
            else write(fd, "NULL", 4);
            write(fd, "\n", 1);
        }
        else if (strcmp(cmd, "PING") == 0) {
            printf("Received PING\n");
            write(fd, "PONG\n", 5);
        }
        else {
            write(fd, "UNKNOWN\n", 8);
        }
    }

    // The loop broke because the client disconnected or an error happened.
    // We close the connection ID (socket) to tell the OS to release the network resources,
    // then the thread finishes its execution naturally.
    close(fd);
    
    return NULL;
}
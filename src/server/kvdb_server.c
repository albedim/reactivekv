/**
 * @file kvdb_server.c
 * @brief Main server entry point and socket management
 * @details Initializes the server and handles the accept loop for client connections
 * 
 * @author albedim <Alberto Di Maio> <https://github.com/albedim> <https://albertodimaio.com>
 * @version 1.0
 * @date 2026
 */

#include "kvdb.h"

/**
 * @brief Main server entry point - initializes and runs the key-value database server.
 * 
 * This function initializes the server infrastructure and enters an infinite loop
 * accepting client connections. Each client is handled by a dedicated thread.
 * 
 * @return int Program exit code (NEVER RETURNS - runs indefinitely)
 * 
 * @details
 *   Initialization Steps:
 *   1. Initialize mutex lock with pthread_mutex_init()
 *   2. Create server socket using socket(AF_INET, SOCK_STREAM, 0)
 *   3. Configure address structure for IPv4 on all interfaces (INADDR_ANY)
 *   4. Bind socket to PORT (6379) using bind()
 *   5. Listen for incoming connections with listen(queue_size=10)
 *   6. Print status message
 * 
 *   Accept Loop:
 *   - Infinite while(1) loop that:
 *     a) Allocates memory for client socket fd
 *     b) Calls accept() to wait for client connection (BLOCKING)
 *     c) Creates new pthread with client_handler as entry point
 *     d) Immediately detaches thread (auto-cleanup)
 *   - Accepts up to 10 pending connections in backlog
 * 
 *   Network Setup:
 *   - Protocol: TCP (AF_INET + SOCK_STREAM)
 *   - Address Family: IPv4
 *   - Binding Address: INADDR_ANY (0.0.0.0, listens on all interfaces)
 *   - Port: 6379 (network byte order via htons())
 *   - Backlog: 10 (kernel queues up to 10 pending accepts)
 * 
 * @thread_safety 
 *   - Main thread: Accept loop
 *   - Worker threads: One per client (created by this function)
 *   - Shared data: Protected by mutex in set()/get()
 * 
 * @note This function never returns - server runs until process is killed (SIGTERM/SIGKILL)
 * @warning No signal handlers, no graceful shutdown mechanism
 * @error_handling Minimal - does not validate bind()/listen() return codes
 * 
 * @example
 *   $ ./server
 *   Server running on 6379
 *   // (now listens for clients indefinitely)
 *   
 *   // In another terminal:
 *   $ nc localhost 6379
 *   > SET foo bar
 *   OK
 *   > GET foo
 *   bar
 *   > PING
 *   PONG
 * 
 * @see client_handler() for per-client thread logic
 * @see set() and get() for command implementations
 * @see store and lock for global data
 */
int main() {
  // pthread_mutex_init() prepares the lock for use
  // Parameters: (&lock, NULL) - address of lock, NULL means default attributes
  // This MUST be called before any thread uses the lock
  // If not initialized: undefined behavior, potential crash
  pthread_mutex_init(&lock, NULL);

  // socket(AF_INET, SOCK_STREAM, 0) creates a TCP socket for IPv4
  // Parameters:
  //   AF_INET: Address Family = Internet (IPv4)
  //   SOCK_STREAM: Socket type = TCP (reliable, ordered, connection-based)
  //   0: Protocol = default for TCP (IPPROTO_TCP is assumed)
  // Returns: file descriptor (fd) of the socket, or -1 on error
  int server_fd = socket(AF_INET, SOCK_STREAM, 0);

  // struct sockaddr_in defines the server's network address configuration
  struct sockaddr_in addr;
  // sin_family: Address family (AF_INET = IPv4)
  addr.sin_family = AF_INET;
  // sin_addr.s_addr: IP address (INADDR_ANY = 0.0.0.0 = listen on ALL interfaces)
  //   - INADDR_ANY allows connections from any network interface/IP on this machine
  //   - Useful for servers that should be accessible from multiple IPs
  addr.sin_addr.s_addr = INADDR_ANY;
  // sin_port: Port number (must be in network byte order - htons converts from host to network)
  //   - htons(PORT) converts 6379 (host byte order) to network byte order
  //   - Network byte order is "big-endian" (most significant byte first)
  //   - On little-endian systems (x86, ARM): htons() swaps the bytes
  addr.sin_port = htons(PORT);

  // bind() associates the socket with the address and port we want to listen on
  // Parameters: (socket_fd, address_ptr, address_len)
  //   - server_fd: the socket we created with socket()
  //   - (struct sockaddr*)&addr: pointer to the address structure (cast to generic sockaddr)
  //   - sizeof(addr): size of the address structure
  // After bind(), the OS knows this socket "owns" 127.0.0.1:6379 (or all IPs:6379 with INADDR_ANY)
  bind(server_fd, (struct sockaddr*) &addr, sizeof(addr));

  // listen(socket_fd, backlog) marks the socket as a listening socket
  // Parameters:
  //   server_fd: the socket to listen on (must have been bind()'d first)
  //   10: backlog = max pending connections kernel queues before accept() is called
  //       If 11 clients try to connect before we accept(), the 11th is rejected
  listen(server_fd, 10);

  printf("Server running on %d\n", PORT);

  while (1) {
    // malloc(sizeof(int)) reserves heap memory to store ONE integer (4 bytes on most systems)
    // WHY? Each thread needs its OWN copy of the client_fd, not a stack variable that disappears
    int* client_fd = malloc(sizeof(int));

    // accept() is the BLOCKING function that waits for a client to connect
    // Returns the socket file descriptor (fd) of the new client connection
    // This fd is used to read() client commands and write() responses back
    // It blocks here until a new client tries to connect - doesn't create threads yet
    // Parameters: (server_fd, client_addr_ptr, client_addr_len_ptr)
    //   - server_fd: the listening socket we created with socket() and bind()
    //   - NULL, NULL: we don't care about the client's IP address, so pass NULL pointers
    //     (if we wanted the IP, we'd create sockaddr_in and pass its address here)
    // Returns: a NEW file descriptor (different from server_fd) for THIS client connection
    *client_fd = accept(server_fd, NULL, NULL);

    // CREATE PTHREAD THREAD STRUCTURE
    // pthread_t is just a typedef for the thread ID (a handle to track the thread)
    // We create this variable to store the thread's ID when pthread_create returns
    // pthread_t is implementation-dependent (could be an int, struct, etc depending on OS)
    pthread_t t;

    // CREATE A NEW THREAD TO HANDLE THIS SPECIFIC CLIENT
    // pthread_create() creates a brand new thread of execution that runs in parallel
    // The new thread starts executing at client_handler() immediately (concurrently with main thread)
    // 
    // Parameters explained:
    //   &t                 -> Address of pthread_t variable (pthread_create fills in the thread ID)
    //   NULL               -> Thread attributes (NULL = use default: joinable, normal priority, etc)
    //   client_handler     -> Function pointer: this function will run in the new thread
    //   client_fd          -> Void pointer argument passed to client_handler()
    int res = pthread_create(&t, NULL, client_handler, client_fd);

    // DETACH THE THREAD (TELL KERNEL TO AUTO-CLEANUP WHEN THREAD EXITS)
    // pthread_detach() sets the thread as "detached" - not "joinable"
    //
    // Thread states:
    //   JOINABLE (default): Thread stays in memory until you call pthread_join()
    //                       Used when main thread needs to wait for worker threads to finish
    //                       Useful for collecting return values or synchronizing completion
    //   DETACHED: Thread automatically cleans up when it finishes, no pthread_join() needed
    //            Main thread doesn't wait for it - both run independently
    //            When detached thread exits, kernel frees its resources automatically
    //
    // WHY detach here?
    //   - The main thread wants to immediately go back to accept() and handle the NEXT client
    //   - We don't need pthread_join() to wait for this client thread to finish
    //   - Detaching lets the kernel free the thread's resources automatically when it exits
    //   - Without detach: resources leak as threads accumulate in memory (zombie threads)
    //     Each un-joined thread takes up memory for its thread descriptor and status
    //
    // Flow WITHOUT detach (WRONG - SERIALIZED):
    //   Main Thread:
    //     accept() -> create thread T1 -> somehow wait for T1 to finish -> accept() BLOCKED
    //   This serializes clients! Only handles one at a time!
    //   Performance is terrible: while T1 is running, other clients wait in the backlog
    //
    // Flow WITH detach (CORRECT - CONCURRENT):
    //   Main Thread:                    Client Thread 1:              Client Thread 2:
    //     accept() ------------------>   client_handler()            
    //     create thread T1                  read/write/process         
    //     detach T1                          (client commands)          
    //     accept() (READY FOR NEXT) -----> (still running)          
    //                 ---------> create thread T2 --------> client_handler()
    //                              detach T2                  read/write/process
    //                              accept() (READY)           (client commands)
    //   
    //   Both T1 and T2 run concurrently, each handling a different client independently
    //   Main thread immediately goes back to accept(), not blocked waiting for clients
    // 
    // Additional context:
    //   - pthread_detach() must be called AFTER pthread_create() (thread already exists)
    //   - Some systems allow pthread_attr_setdetachstate() BEFORE pthread_create() to avoid explicit detach
    //   - Once detached, a thread CANNOT be re-attached
    pthread_detach(t);
  }

  return 0;
}

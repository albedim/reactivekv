/**
 * @file kvdb.h
 * @brief Header file for Key-Value Database Server
 * @details Central header containing all type definitions, constants, and function declarations
 * 
 * @author albedim <Alberto Di Maio> <https://github.com/albedim> <https://albertodimaio.com>
 * @version 1.0
 * @date 2026
 */

#ifndef KVDB_H
#define KVDB_H

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <pthread.h>

/* Server Configuration Constants */
#define SERVER_NAME "reactivekv"                    /**< Server application name */
#define SERVER_VERSION "1.0"                        /**< Server version */
#define PORT 6379                                   /**< Server listening port (Redis default) */
#define BUF 1024                                    /**< Buffer size for client messages (1KB max message) */
#define MAX_KEY_LEN 64                              /**< Maximum key length (matches Entry struct) */
#define MAX_VALUE_LEN 64                            /**< Maximum value length (matches Entry struct) */
#define MAX_STORE_ENTRIES 100                       /**< Maximum key-value pairs in store */
#define MAX_CONNECTIONS 1024                        /**< Maximum concurrent client connections */

/* Error Messages */
#define ERR_SOCKET_CREATION "socket creation failed, server is not running"
#define ERR_CONNECTION_FAILED "connection failed"

/**
 * @struct Entry
 * @brief Key-value pair structure for in-memory storage.
 * 
 * This structure represents a single entry in the key-value store.
 * Both key and value are null-terminated strings with a maximum length of 63 characters.
 * 
 * @member key   Unique identifier for the entry (max 63 chars + \0 terminator)
 * @member value Associated value/data (max 63 chars + \0 terminator)
 * 
 * @note Keys and values must be unique and non-empty for meaningful operations.
 * @warning String overflow can occur if input exceeds 63 characters - validate input!
 */
typedef struct {
  char key[64];
  char value[64];
} Entry;

/* Global variables - declared in kvdb_store.c */
extern Entry store[100];
extern int size;
extern pthread_mutex_t lock;

/* Function declarations - database operations (kvdb_store.c) */
void set(const char* k, const char* v);
char* get(const char* k);

/* Function declarations - client handling (kvdb_handler.c) */
void* client_handler(void* arg);

#endif

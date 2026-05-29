reactivekv
Overview

reactivekv is an in-memory key-value database written in C. It is designed as a lightweight, Redis-like system that provides fast data access through a network interface while adding additional features such as typed values, event subscriptions, and command hooks.

The goal of reactivekv is to explore how modern in-memory databases work internally while extending the basic KV model with reactive and programmable behavior.

Core Concept

At its core, reactivekv is:

an in-memory key-value store
accessible through a socket-based client-server architecture
designed for low-latency operations
extended with a reactive system that allows side effects on data operations

Unlike traditional key-value stores, reactivekv introduces mechanisms to react to changes in data and enforce behavior at runtime.

Features
1. In-Memory Key-Value Store

All data is stored in RAM for fast access.

Supported operations:

SET key value
GET key
DEL key

Each key maps to a value stored in a hash table.

2. Typed Values

Values are not limited to raw strings. Each value has an explicit type.

Supported types:

INT
STRING
BOOL

Example usage:

SET age 20 INT
SET name alberto STRING
SET active true BOOL

Type information allows safer and more meaningful operations.

Example:

INCR age (valid)
INCR name (invalid)
3. Event Subscription System

Clients can subscribe to specific keys to receive updates when values change.

Example:

SUBSCRIBE user:1

When a value changes:

SET user:1 online

All subscribed clients receive an event notification:

EVENT user:1 changed to online

This enables real-time reactive behavior similar to pub-sub systems.

4. Command Hooks

reactivekv supports programmable hooks that execute automatically when commands are run.

Hooks are defined using the syntax:

ON <COMMAND> <PATTERN> <ACTION>

Examples:

ON SET user:* LOG
ON GET * COUNT
ON SET age VALIDATE INT
ON SET password HASH

Supported hook types:

LOG: logs operations
COUNT: tracks command usage statistics
VALIDATE: enforces type constraints before execution
NOTIFY: triggers subscription events
TRANSFORM: modifies values before storing

Hooks allow behavior to be extended without modifying core database logic.

Architecture

reactivekv is composed of the following components:

1. Server Layer

Handles socket connections, client communication, and request dispatching.

2. Command Parser

Parses incoming text commands and converts them into internal operations.

3. Storage Engine

Implements the in-memory key-value store using hash tables.

4. Type System

Manages value types and ensures type-safe operations.

5. Subscription Engine

Tracks clients subscribed to keys and dispatches real-time updates.

6. Hook Engine

Evaluates registered hooks and executes side effects during command execution.

Execution Flow
Client sends command via socket
Server receives and parses command
Type system validates input (if applicable)
Storage engine executes operation in memory
Hook engine runs any matching hooks
Subscription engine sends events if required
Response is returned to client
Design Goals

reactivekv is designed with the following principles:

simplicity over completeness
clarity of internal database mechanics
extensibility through hooks and subscriptions
predictable in-memory performance
minimal but expressive command set
Non-Goals

reactivekv is not intended to:

replace production databases
provide full SQL support
implement distributed clustering
guarantee persistence or durability as a primary feature
Possible Future Extensions
persistence layer (AOF-style logging)
TTL expiration system
multi-threaded execution
more advanced query capabilities
replication system
Summary

reactivekv is a minimal in-memory database system inspired by Redis, extended with typed values, event subscriptions, and command hooks to explore reactive and programmable data behavior in a low-level C implementation.
# Emergency Response Simulator

University systems-programming project that simulates dispatching limited rescue
resources to emergencies on a two-dimensional grid. A C client submits requests
through a local POSIX message queue; a multithreaded server assigns responders,
simulates travel and on-site work, and logs state changes.

## Technologies and design

- C11, POSIX message queues (`mqueue.h`), GCC and GNU Make.
- C11 threads, mutexes, condition variables and atomics coordinate emergency
  handlers and one thread per assigned responder. A per-emergency barrier waits
  for all responders to arrive before on-site work begins.
- Configuration files define the grid, responder counts, speeds and bases, and
  each emergency type's resource requirements, service times and priority.
- Travel time is Manhattan distance divided by speed, using integer division.
  Travel, service and return trips use real `sleep()` calls in seconds.
- Priorities affect arrival-time checks and resource-wait timeouts: priority 2
  uses 10 seconds, priority 1 uses 30 seconds, and priority 0 has no timeout.
  Wake-ups favor higher waiting priorities; active work is not preempted.

## Prerequisites and build

Use Linux with POSIX message queues enabled, GCC with C11 thread and atomic
support, GNU Make, and the system libraries required by `-lrt`.

Run commands from the repository root. Configuration is loaded from `config/`
using paths relative to the working directory. Create `log_file.log` before
starting either executable: the logger appends to an existing file and does not
create it.

```sh
touch log_file.log
make all
```

This builds `server` and `client`. The default `make` target builds and runs only
the server, so use `make all` to build both executables.

## Minimal example

In one terminal:

```sh
./server
```

In a second terminal, from the same repository root:

```sh
./client Theft 120 220 0
```

Arguments are `emergency_name x y delay_seconds`. This example requests one
police responder at its configured base, with two seconds of on-site work.
Names must match the configuration. With the supplied grid, coordinates must
satisfy `1 <= x <= 400` and `1 <= y <= 300`.

Wait for `emergenza COMPLETED` and the responder's return to `IDLE` in
`log_file.log`, then request shutdown from the second terminal:

```sh
./client shutdown_server
```

The client also supports a user-supplied request file. Create `requests.txt`
with one `emergency_name x y delay_seconds` entry per line, for example
`Theft 120 220 0`. This mode has been checked against the source but
**has not been run during this review**:

```sh
./client -f requests.txt
```

Each nonempty line contains `emergency_name x y delay_seconds`. Requests are
sent sequentially, waiting the specified delay before each send.

**Verification:** `touch log_file.log`, `make all`, `./server`,
`./client Theft 120 220 0` and
`./client shutdown_server` were run successfully in a temporary copy on Linux
x86_64 with GCC 16.2.1 and GNU Make 4.4.1. The log recorded completion and return
to base, and the server exited successfully and removed its queue. Compilation
emits header-guard warnings. File input, concurrent workloads and Arm builds
remain unverified.

## Repository layout

| Path | Purpose |
| --- | --- |
| `server.c`, `client.c` | Server startup and request submission |
| `threads/` | Emergency handling, responder lifecycle and arrival barrier |
| `parsers/`, `include/` | Parsers, shared types, constants and interfaces |
| `include/simulation_types.h` | Simulation states and shared data structures |
| `config/` | Environment, responder and emergency-type configuration |
| `write_log.c`, `log_file.log` | Event logger and runtime log; create the log before startup |

## Limitations

- Emergency handlers run in batches of ten. The next batch starts only after
  all current handlers finish, which can delay queued requests.
- Responder allocation uses an idle count as an array index without checking
  individual responder states. Overlapping emergencies may reuse busy instances.
- Priority-0 resource waits are unbounded. Shutdown messages do not interrupt
  active work or resource waits, so shutdown can remain blocked.
- Input validation is limited. Use existing type names, positive responder
  counts and speeds, and nonnegative delays; negative delays in file mode can
  cause very long waits. Timing checks do not provide real-time guarantees.

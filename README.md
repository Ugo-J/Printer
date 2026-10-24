# Locklog

A header-only C++ logging library that keeps printing off your hot path. The producer thread copies values into a lock-free ring buffer. A separate thread formats them and writes them out in large batches.

## Overview

Printing from a latency-sensitive thread is expensive. `std::cout` builds a sentry object, goes through locale machinery, and often ends in a syscall per line. Locklog avoids all of that on the producer side. The hot thread does a bounds check, a `memcpy` or a single store, and moves on. Formatting and I/O happen on whichever thread calls `print()`.

## Key Features

- **Single-producer, single-consumer, lock-free.** Synchronisation uses acquire/release atomics on two unmasked counters, with no mutexes.
- **Values are copied at call time.** Strings are copied into an internal ring buffer, and numbers are stored by value inside the entry. Changing or destroying your variable after the call is safe.
- **Fast formatting.** Numbers are formatted with `std::to_chars` straight into the output buffer, with no locale work and no allocation.
- **Batched output.** Everything is assembled in a 1 MB scratchpad (configurable) and sent with a single `write` call per batch.
- **Never blocks the producer.** When a ring is full, the new entry is dropped and counted rather than stalling your thread.
- **Optional prelude.** A fixed string (a tag or timestamp label, for example) is placed at the start of every output chunk.
- **Stream-style syntax.** Every `print_shift` overload has a matching `operator<<`, so `log << "x = " << x << Lock_logger::endl` reads like `std::cout`. Both forms return `*this`, so they chain.
- **Header-only.** All out-of-class definitions are `inline`, so the header can be included from several translation units without link errors.

## Supported Types

- `const char*` (null-terminated C strings)
- `int`
- `unsigned int`
- `int64_t`
- `uint64_t`
- `double`
- `Lock_logger::endl`, a tag that appends a newline, publishes everything queued so far to the consumer, and causes the scratchpad to be written out

## Requirements

- C++17 or later (`std::to_chars`)
- GCC 11 or later, or a recent Clang with a standard library that supports floating-point `std::to_chars`
- A POSIX system (Linux or macOS), since output goes through `write(2)` and `<unistd.h>`

```bash
g++ -std=c++17 -O2 -pthread your_program.cpp -o your_program
```

## How It Works

Locklog uses two rings and one staging buffer.

1. **Pointer array.** A ring of fixed-size entries. Each entry holds a type tag and either the value itself (numbers) or a position in the data buffer (strings).
2. **Data buffer.** A ring of bytes that holds string contents, each stored with its null terminator. A string is always stored contiguously, so if it would straddle the end of the buffer the producer skips to the start and the skipped tail counts as used space.
3. **Scratchpad.** A linear buffer owned by the consumer. `print()` formats entries into it and writes it out when it fills, when an `endl` entry arrives, or when a string is too large to stage.

Both rings use unmasked, ever-increasing 64-bit counters. A counter is masked with `& (SIZE - 1)` only when an actual position is needed, so sizes must be powers of two. Because the counters are never masked, an empty ring and a full ring are always distinguishable.

### Producer and consumer

| Thread | Calls | Does |
|---|---|---|
| Producer | `print_shift(...)`, `flush()` | Copies the value into the rings, publishes with `flush()` |
| Consumer | `print()` | Formats pending entries and writes them out |

Entries are not visible to the consumer until they are published with a release store to `write_index`. `flush()` does this explicitly, and `Lock_logger::endl` does it implicitly, so a line that ends in `endl` needs no separate `flush()` call.

### Memory ordering

- The producer fills an entry, then publishes `write_index` with `memory_order_release`.
- The consumer reads `write_index` with `memory_order_acquire`, processes entries, then publishes `read_index` (and `data_read` for string entries) with `memory_order_release`.
- The producer reads those two with `memory_order_acquire` before reusing any slot or bytes.

A stale view on either side can only make a ring look fuller than it is, so unread data is never overwritten.

## Usage

### Basic example

```cpp
#include "locklog.hpp"

int main() {
    Lock_logger log("[log] ", /* TODO: constructor arguments */);

    // stream style
    log << "Price: " << 101.5 << Lock_logger::endl;

    // equivalent function-call style
    log.print_shift("Qty: ").print_shift(10).print_shift(Lock_logger::endl);

    // endl already published everything queued above, so no flush() is needed

    log.print();     // formats and writes the pending entries

    return 0;
}
```

### Multi-threaded example

```cpp
#include "locklog.hpp"
#include <thread>

Lock_logger log("[log] ", /* TODO: constructor arguments */);
std::atomic<bool> running{true};

void producer() {
    for (int i = 0; i < 1000; ++i) {
        log << "iteration " << i << Lock_logger::endl;   // endl publishes the line
        // latency-sensitive work continues here
    }
    running = false;
}

void consumer() {
    while (running.load()) {
        log.print();
    }
    log.print();   // drain whatever was published last
}

int main() {
    std::thread p(producer), c(consumer);
    p.join();
    c.join();
}
```

Only one thread may call `print_shift` and `flush`, and only one thread may call `print`. Calling them from more than one thread each is undefined behaviour.

## API Reference

### `Lock_logger`

#### `print_shift(const char* data)`

Copies the string, including its terminator, into the data buffer and adds an entry to the pointer array. Returns `*this`.

The entry is dropped and `dropped_data` is incremented if the pointer array is full, if the data buffer does not have enough free bytes (padding included), or if the string is longer than the whole data buffer.

#### `print_shift(int)`, `print_shift(unsigned int)`, `print_shift(int64_t)`, `print_shift(uint64_t)`, `print_shift(double)`

Stores the value directly in a pointer array entry. No data buffer space is used. The entry is dropped and counted if the pointer array is full.

#### `print_shift(Lock_logger::endl)`

Adds an entry telling the consumer to append `'\n'` and write the scratchpad out immediately. It also publishes every entry queued up to and including itself, so no separate `flush()` call is needed after it.

#### `operator<<`

```cpp
Lock_logger& operator<<(const char* data);
Lock_logger& operator<<(int x);
Lock_logger& operator<<(unsigned int x);
Lock_logger& operator<<(int64_t x);
Lock_logger& operator<<(uint64_t x);
Lock_logger& operator<<(double x);
Lock_logger& operator<<(/* endl tag type */);
```

Each overload forwards to the matching `print_shift`, so it behaves identically, including the drop rules and the counters. It returns a reference to the logger, which is what makes chains like `log << "a" << 1 << 2.0 << Lock_logger::endl` work. The chain is parsed left to right, one call per value.

A few things to know:

- **Chaining is not atomic.** Each `<<` is a separate entry. Since there is a single producer thread this does not matter for ordering, but a drop can happen in the middle of a chain if a ring fills up, leaving a partial line.
- **No iostream manipulators.** `std::endl`, `std::hex`, `std::setw` and similar are not supported. Use `Lock_logger::endl` for a newline.
- **Wrap ternaries in brackets.** `?:` has lower precedence than `<<`, so write `log << (ok ? "yes" : "no")`.
- **Publishing.** Like `print_shift`, `<<` only queues the entry. Entries become visible to the consumer when `Lock_logger::endl` is queued or when you call `flush()`. A line that does not end in `endl` needs an explicit `flush()`.

#### `flush()`

Publishes every entry added since the last flush by storing the producer's counter into `write_index` with release ordering. Until this runs, the consumer cannot see those entries. You only need to call it yourself for output that does not end in `Lock_logger::endl`, since `endl` publishes on its own.

#### `print()`

Consumer side. Processes every published entry:

- Strings are copied into the scratchpad.
- Numbers are formatted into the scratchpad with `std::to_chars`.
- `endl` appends a newline and writes the scratchpad out.
- When the scratchpad cannot hold the next item, it is written out first and the item goes into the emptied scratchpad.
- A string that cannot fit in an empty scratchpad (after the prelude) is written straight to the output descriptor.

The scratchpad is also written out at the end of the call if it is completely full. Otherwise, partially filled data stays staged until the next `endl`, a full scratchpad, or the destructor.

#### Counters

| Member | Meaning |
|---|---|
| `dropped_data` | Entries discarded on the producer side because a ring was full or a string was too large |
| `failed_writes` | `write` calls that failed with an error other than `EINTR`. The affected entry is treated as consumed |

## Configuration

Sizes are set at compile time in `locklog.hpp`. The ring sizes must be powers of two.

| Constant | Purpose | Default |
|---|---|---|
| `SIZE_OF_SCRATCHPAD` | Staging buffer for output batches | 1 MB |
| `SIZE_OF_BUFFER` | Ring buffer holding string data | TODO |
| `SIZE_OF_POINTER_ARRAY` | Number of ring entries | TODO |
| `MIN_NUM_SPACE` | Minimum free scratchpad bytes required before formatting a number | 30 |

A bigger scratchpad means fewer syscalls but longer individual writes and more output sitting unflushed. Sizes in the 128 KB to 1 MB range are worth benchmarking against each other, since smaller buffers can stay cache-warm.

## Prelude

If a prelude is set, it is copied to the start of the scratchpad whenever the scratchpad is empty. That means one prelude per output chunk, so a long line that triggers a mid-line flush will carry a fresh prelude on its continuation. Set `prelude_len` to 0 when there is no prelude.

## Behaviour to Be Aware Of

- **Output can lag.** Data stays in the scratchpad until an `endl`, a full scratchpad, or the destructor. A crash, `_exit`, `abort` or kill signal loses whatever was staged.
- **Drops, not stalls.** If the consumer falls behind, new entries are discarded and `dropped_data` goes up. Check it after a run.
- **Unflushed entries cannot free space.** The consumer only releases ring space for entries it has seen, so end lines with `Lock_logger::endl` or call `flush()` regularly. The library flushes automatically when it finds the pointer array full.
- **Failed writes lose data.** A write that fails is counted in `failed_writes` and the entry is not retried. With a closed pipe, `SIGPIPE` will kill the process unless you ignore it.
- **Interleaving.** Locklog writes to the file descriptor directly. Do not mix it with `std::cout` or `printf` on the same stream unless you flush those first, because the two buffer separately.
- **Number formatting.** Numbers use the shortest round-trip representation from `std::to_chars`. There are no width, precision or base options.

## Limitations

- Single producer, single consumer only
- Fixed buffer sizes, with no dynamic growth
- Output to standard output only
- POSIX only
- Strings longer than the data buffer are dropped on the producer side

## Performance Notes

- The producer path has no syscalls, no locks and no allocation. A string costs a `strlen`, a capacity check and a `memcpy`. A number costs a capacity check and a store.
- Place `write_index` and `read_index` on separate cache lines to avoid false sharing between the two threads.
- For best results, keep the consumer on its own thread so that `write` latency never reaches the producer.
- Call sites inline fully when the header is included directly, so keep hot-path definitions in the header.

## Building

Locklog is header-only. Include `locklog.hpp` from as many source files as you like.

```cpp
#include "locklog.hpp"
```

## License

MIT License

Copyright (c) 2026

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.

## Contributing

Contributions are subject to manual review. Please submit pull requests or issues through the repository, and they will be reviewed as time permits.

## Contact

For questions or feedback, contact: ujezeorah@gmail.com
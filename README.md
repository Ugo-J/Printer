# PrintShift

A high-performance C++ library that moves printing operations off the hot path by buffering output and deferring actual I/O to a separate thread.

## Overview

PrintShift is a header-only library designed to minimize the performance impact of logging and debugging output in performance-critical applications. Instead of blocking your main thread with I/O operations, PrintShift buffers data in memory and allows a separate thread to handle the actual printing asynchronously.

## Key Features

- **Lock-free design**: Uses atomic operations for thread synchronization
- **Zero-copy buffering**: Stores data directly in an internal buffer
- **Type-safe**: Supports multiple data types with proper alignment
- **Minimal overhead**: Defers expensive I/O operations to a separate thread
- **Large data handling**: Automatically handles strings larger than the internal buffer
- **Header-only**: Easy integration into existing projects

## Supported Data Types

- `const char*` (C-strings)
- `int`
- `unsigned int`
- `int64_t`
- `uint64_t`
- `double`

## How It Works

PrintShift uses a dual-buffer architecture:

1. **Internal Buffer** (64 KB): Stores the actual data (strings, numbers, etc.)
2. **Pointer Array** (32K entries): Stores metadata about each buffered item (type and location)

The writer thread calls `print_shift()` to buffer data, then `flush()` to make it visible. A separate reader thread calls `print()` to output the buffered data to `std::cout`.

### Thread Safety

- The `write_index` is atomic, ensuring visibility across threads
- The writer updates `p_array_index` locally and atomically publishes via `flush()`
- The reader uses `read_index` to track its position
- The `wrap` flag handles circular buffer wraparound

## Usage

### Basic Example

```cpp
#include "printshift.hpp"

int main() {
    printer p;
    
    // Buffer some data (writer thread)
    p.print_shift("Hello, ", 7);
    p.print_shift("World!", 6);
    p.print_shift(42);
    p.flush();  // Make data visible to reader
    
    // Print buffered data (reader thread)
    p.print();  // Outputs: Hello, World!42
    
    return 0;
}
```

### Multi-threaded Example

```cpp
#include "printshift.hpp"
#include <thread>
#include <chrono>

printer p;

void writer_thread() {
    for (int i = 0; i < 1000; i++) {
        p.print_shift("Iteration: ", 11);
        p.print_shift(i);
        p.print_shift("\n", 1);
        p.flush();
        
        // Do performance-critical work here
        // without blocking on I/O
    }
}

void reader_thread() {
    while (true) {
        p.print();
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
}

int main() {
    std::thread writer(writer_thread);
    std::thread reader(reader_thread);
    
    writer.join();
    reader.join();
    
    return 0;
}
```

## API Reference

### Class: `printer`

#### Methods

##### `void print_shift(const char* data, int length_of_data)`

Buffers a C-string for later printing.

**Parameters:**
- `data`: Pointer to the string data
- `length_of_data`: Length of the string (excluding null terminator)

**Behavior:**
- Copies the string into the internal buffer
- Automatically handles strings larger than the buffer size
- Wraps around when the buffer is full

**Example:**
```cpp
const char* msg = "Hello";
p.print_shift(msg, 5);
```

##### `void print_shift(int x)`
##### `void print_shift(unsigned int x)`
##### `void print_shift(int64_t x)`
##### `void print_shift(uint64_t x)`
##### `void print_shift(double x)`

Buffers a numeric value for later printing.

**Parameters:**
- `x`: The numeric value to buffer

**Behavior:**
- Stores the value with proper alignment in the internal buffer
- Automatically wraps to the beginning if insufficient space remains

**Example:**
```cpp
p.print_shift(42);
p.print_shift(3.14159);
p.print_shift((uint64_t)1234567890);
```

##### `void flush()`

Makes all buffered data visible to the reader thread.

**Behavior:**
- Atomically updates `write_index` to match `p_array_index`
- Must be called after `print_shift()` operations to make data visible
- Automatically called during buffer wraparound

**Example:**
```cpp
p.print_shift("Data", 4);
p.flush();  // Now visible to print()
```

##### `void print()`

Outputs all visible buffered data to `std::cout`.

**Behavior:**
- Reads from `read_index` up to `write_index`
- Handles circular buffer wraparound
- Type-casts and prints each item according to its stored type
- Non-blocking: only prints what's currently visible

**Example:**
```cpp
p.print();  // Outputs all flushed data
```

## Configuration

### Buffer Sizes

The library uses compile-time constants that can be modified in `printshift.hpp`:

```cpp
inline static const int SIZE_OF_BUFFER = 64 * 1024;  // 64 KB data buffer
inline static const int SIZE_OF_POINTER_ARRAY = SIZE_OF_BUFFER/2;  // 32K entries
```

**Considerations:**
- Larger buffers reduce wraparound frequency but use more memory
- The pointer array size is set to `SIZE_OF_BUFFER/2` to ensure it can hold metadata for the worst case (all single-character strings)

## Performance Characteristics

### Writer Thread (Hot Path)

- **Time Complexity**: O(n) where n is the data size
- **Operations**: Memory copy + pointer arithmetic + occasional atomic store
- **No I/O blocking**: All expensive operations deferred

### Reader Thread

- **Time Complexity**: O(m) where m is the number of buffered items
- **Operations**: Type casting + `std::cout` operations
- **Blocking**: May block on I/O, but doesn't affect writer

## Memory Layout

```
Internal Buffer (64 KB):
[string1\0][int][string2\0][double][...]
     ^      ^       ^         ^
     |      |       |         |
Pointer Array:
[{CHAR, ptr1}, {INT, ptr2}, {CHAR, ptr3}, {DOUBLE, ptr4}, ...]
```

## Thread Synchronization

The library uses a lock-free synchronization mechanism:

1. Writer increments `p_array_index` locally
2. Writer calls `flush()` to atomically publish `write_index`
3. Reader reads `write_index` atomically
4. Reader processes items from `read_index` to `write_index`

The `wrap` flag ensures proper handling when the circular buffer wraps around.

## Limitations

- **Single writer, single reader**: Not designed for multiple concurrent writers
- **No backpressure**: If the reader is too slow, old data may be overwritten
- **Fixed buffer size**: Cannot dynamically grow
- **No formatting**: Numbers are printed with default `std::cout` formatting
- **Output to stdout only**: Currently hardcoded to use `std::cout`

## Best Practices

1. **Call flush() regularly**: Data isn't visible until flushed
2. **Size your buffer appropriately**: Consider your data volume and reader frequency
3. **Dedicate a thread to printing**: Run `print()` in a loop on a separate thread
4. **Handle wraparound**: Ensure your reader keeps up to avoid data loss
5. **Measure overhead**: Profile to ensure the library provides benefit for your use case

## Use Cases

- High-frequency trading systems
- Real-time game engines
- Performance-critical simulations
- Low-latency network applications
- Any scenario where logging overhead impacts performance

## Building

This is a header-only library. Simply include the headers:

```cpp
#include "printshift.hpp"
```

**Requirements:**
- C++11 or later (for `std::atomic` and `std::align`)
- Standard library headers: `<memory>`, `<cstring>`, `<iostream>`, `<atomic>`

**Compilation:**
```bash
g++ -std=c++11 -pthread your_program.cpp -o your_program
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

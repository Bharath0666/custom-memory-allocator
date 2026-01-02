# Custom Memory Allocator in C++

A **from-scratch educational implementation** of a dynamic memory allocator in C++, illustrating the inner workings of `malloc`, `free`, `realloc`, and `calloc`.  

This allocator manages a **single contiguous heap** (one initial OS allocation), uses **hidden metadata** for block management, implements **splitting** and **coalescing** to combat fragmentation, and includes tools for visualization and statistics.  

Perfect for learning systems programming, operating systems concepts, pointer arithmetic, and preparing for technical interviews.

---

## ✨ Key Features

- Single fixed-size heap (one OS allocation via `new[]` or similar)
- Full implementation of `my_malloc`, `my_free`, `my_realloc`, `my_calloc`
- **First-Fit** allocation strategy
- **Block splitting** to reduce internal fragmentation
- **Immediate coalescing** (forward and backward) to reduce external fragmentation
- **Doubly-linked explicit free list** for efficient traversal and merging
- Heap visualization and detailed statistics
- Robust pointer validation and error checking
- Optional global drop-in replacement for standard allocator

---

## 🧠 Memory Model

```
┌────────────────────┬────────────────────┐
│     METADATA       │   USER PAYLOAD     │
│  (MemoryBlock)     │  (returned ptr)    │
└────────────────────┴────────────────────┘
```

- Metadata is **hidden** immediately before the user pointer
- Includes size, status, and prev/next pointers for the **doubly-linked free list**
- All blocks (allocated and free) are chained via prev/next for easy coalescing

---

## ⚙️ Internal Design

### MemoryBlock Header

```cpp
struct MemoryBlock {
		size_t size;             // Usable payload size (excludes header)
		bool   is_free;          // true if block is available
		MemoryBlock* prev;        // Previous block in heap (physical order)
		MemoryBlock* next;        // Next block in heap (physical order)
		// Additional: prev_free / next_free for explicit free list (optional enhancement)
};
```

- **Doubly-linked physical list**: Enables immediate neighbor coalescing
- **Explicit free list** (recommended): Separate doubly-linked list of only free blocks for faster search

---

## 🧩 Public API

### Core Functions

- `void* my_malloc(size_t size)`  
	Allocates and returns a pointer (or throws/returns nullptr on failure)

- `void my_free(void* ptr)`  
	Marks block free and coalesces with adjacent free blocks

- `void* my_realloc(void* ptr, size_t new_size)`  
	Resizes in-place if possible; otherwise allocates new block and copies data

- `void* my_calloc(size_t count, size_t size)`  
	Allocates and zero-initializes `count * size` bytes

### Debug & Utility

- `printStats()` → Heap usage, fragmentation percentage, allocation count
- `printHeapLayout()` → Visual table of blocks (address, size, status)
- `isValidPointer(void* ptr)` → Checks if pointer belongs to this heap
- `reset()` → Resets heap to initial empty state (for testing)

---

## 🌍 Optional Global Allocator

```cpp
void initGlobalAllocator(size_t heap_size);
void destroyGlobalAllocator();

// Drop-in replacements
void* malloc(size_t size) { return custom_malloc(size); }
void* calloc(size_t n, size_t size) { return custom_calloc(n, size); }
// etc.
```

Allows overriding standard functions (via macros or linking) for testing.

---

## 🚀 Usage Examples

### Instance-Based Usage

```cpp
#include "memory_allocator.hpp"

int main() {
		CustomAllocator::MemoryAllocator alloc(8192);  // 8KB heap

		int* ints = static_cast<int*>(alloc.my_malloc(10 * sizeof(int)));
		for (int i = 0; i < 10; ++i) ints[i] = i * i;

		alloc.printHeapLayout();
		alloc.printStats();

		alloc.my_free(ints);
		return 0;
}
```

### Realloc & Calloc

```cpp
char* buf = static_cast<char*>(alloc.my_malloc(32));
strcpy(buf, "Hello");

buf = static_cast<char*>(alloc.my_realloc(buf, 128));  // Preserves content
strcat(buf, ", expanded world!");

int* zeros = static_cast<int*>(alloc.my_calloc(50, sizeof(int)));
// All 50 elements guaranteed zero
```

---

## 📊 Visualization Examples

**Stats Output**

```
╔═══════════════════════════════════╗
║   CUSTOM ALLOCATOR STATISTICS     ║
╠═══════════════════════════════════╣
║ Total Heap Size :   8192 bytes    ║
║ Used Memory     :   1240 bytes    ║
║ Free Memory     :   6952 bytes    ║
║ Active Blocks   :      5          ║
║ Fragmentation   :    8.7%         ║
╚═══════════════════════════════════╝
```

**Heap Layout**

```
Address       | Size (bytes) | Status
---------------------------------------
0x12340000    |   128        | ALLOCATED
0x123400A0    |   512        | FREE
0x123402C0    |   256        | ALLOCATED
0x123403E0    |  6796        | FREE (top)
```

---

## 🔄 How It Works: Key Mechanisms

- **First-Fit**: Scan free list for first block large enough
- **Splitting**: If remainder ≥ minimum block size + header, split
- **Coalescing**: On free, merge with prev/next if free (immediate, no delay)
- **Alignment**: All allocations aligned to 16 bytes (or platform max) for performance/safety

---

## 🎯 Educational Value

This project demystifies:
- Hidden metadata and pointer arithmetic
- Internal vs. external fragmentation
- Why real allocators (dlmalloc, jemalloc, tcmalloc) are complex
- Trade-offs in allocation strategies (first-fit vs. best-fit vs. segregated lists)

Great for OS courses, embedded programming, and interview prep.

---

## 🚧 Possible Extensions & Improvements

- **Segregated free lists** → Separate lists by size class for O(1) small allocations
- **Best-Fit / Next-Fit** strategies
- **Thread safety** with mutexes or lock-free structures
- **Memory alignment** guarantees (e.g., `aligned_alloc`)
- **Boundary tags / footers** for faster coalescing
- **Leak detection** and use-after-free guards
- **Integration with C++ `operator new/delete`**

---

## 🛠️ Build & Run

```bash
g++ -std=c++17 -O2 -Wall -Wextra demo.cpp memory_allocator.cpp -o demo
./demo
```

Test with various allocation patterns to observe fragmentation and coalescing.

---

## 📜 License

MIT License – free to use, study, modify, and share.

---



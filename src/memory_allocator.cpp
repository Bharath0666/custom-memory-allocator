/**
 * @file memory_allocator.cpp
 * @brief Custom Memory Allocator - Implementation
 *
 * Implementation of custom malloc(), free(), and supporting functions
 * with block coalescing and splitting algorithms.
 */

#include "memory_allocator.hpp"
#include <algorithm>
#include <cassert>
#include <cstring>


namespace CustomAllocator {

// Global allocator instance
MemoryAllocator *g_allocator = nullptr;

//=============================================================================
// MemoryAllocator - Constructors and Destructor
//=============================================================================

MemoryAllocator::MemoryAllocator(size_t heap_size)
    : heap_size_(heap_size), free_list_(nullptr), stats_{}, owns_memory_(true) {
  // Allocate the heap using system malloc
  heap_start_ = static_cast<char *>(std::malloc(heap_size));
  if (!heap_start_) {
    throw std::bad_alloc();
  }
  heap_end_ = heap_start_ + heap_size;

  initializeHeap();
}

MemoryAllocator::MemoryAllocator(void *memory, size_t size)
    : heap_start_(static_cast<char *>(memory)),
      heap_end_(static_cast<char *>(memory) + size), heap_size_(size),
      free_list_(nullptr), stats_{}, owns_memory_(false) {
  if (!memory || size < sizeof(MemoryBlock) + MIN_BLOCK_SIZE) {
    throw std::invalid_argument("Invalid memory region");
  }

  initializeHeap();
}

MemoryAllocator::~MemoryAllocator() {
  if (owns_memory_ && heap_start_) {
    std::free(heap_start_);
  }
  heap_start_ = nullptr;
  heap_end_ = nullptr;
  free_list_ = nullptr;
}

MemoryAllocator::MemoryAllocator(MemoryAllocator &&other) noexcept
    : heap_start_(other.heap_start_), heap_end_(other.heap_end_),
      heap_size_(other.heap_size_), free_list_(other.free_list_),
      stats_(other.stats_), owns_memory_(other.owns_memory_) {
  other.heap_start_ = nullptr;
  other.heap_end_ = nullptr;
  other.free_list_ = nullptr;
  other.owns_memory_ = false;
}

MemoryAllocator &MemoryAllocator::operator=(MemoryAllocator &&other) noexcept {
  if (this != &other) {
    if (owns_memory_ && heap_start_) {
      std::free(heap_start_);
    }

    heap_start_ = other.heap_start_;
    heap_end_ = other.heap_end_;
    heap_size_ = other.heap_size_;
    free_list_ = other.free_list_;
    stats_ = other.stats_;
    owns_memory_ = other.owns_memory_;

    other.heap_start_ = nullptr;
    other.heap_end_ = nullptr;
    other.free_list_ = nullptr;
    other.owns_memory_ = false;
  }
  return *this;
}

//=============================================================================
// Initialization
//=============================================================================

void MemoryAllocator::initializeHeap() {
  // Create initial free block spanning the entire heap
  free_list_ = reinterpret_cast<MemoryBlock *>(heap_start_);
  free_list_->size = heap_size_ - sizeof(MemoryBlock);
  free_list_->is_free = true;
  free_list_->next = nullptr;
  free_list_->prev = nullptr;

  // Initialize statistics
  stats_.total_heap_size = heap_size_;
  stats_.used_memory = 0;
  stats_.free_memory = free_list_->size;
  stats_.total_allocations = 0;
  stats_.total_frees = 0;
  stats_.block_count = 1;
  stats_.free_block_count = 1;
  stats_.coalesce_count = 0;
  stats_.split_count = 0;
}

void MemoryAllocator::reset() { initializeHeap(); }

//=============================================================================
// Core Allocation Functions
//=============================================================================

void *MemoryAllocator::my_malloc(size_t size) {
  if (size == 0) {
    return nullptr;
  }

  // Align the requested size
  size = alignSize(size);

  // Ensure minimum block size
  if (size < MIN_BLOCK_SIZE) {
    size = MIN_BLOCK_SIZE;
  }

  // Find a suitable free block using first-fit strategy
  MemoryBlock *block = findFreeBlock(size);

  if (!block) {
    // No suitable block found
    std::cerr << "[my_malloc] ERROR: Out of memory! Requested: " << size
              << " bytes\n";
    return nullptr;
  }

  // Try to split the block if it's too large
  splitBlock(block, size);

  // Mark as allocated
  block->is_free = false;

  // Update statistics
  updateStatsAfterAlloc(block->size);

  // Return pointer to data portion (after metadata)
  return block->getData();
}

void MemoryAllocator::my_free(void *ptr) {
  if (!ptr) {
    return; // free(nullptr) is valid and does nothing
  }

  // Validate pointer
  if (!isValidPointer(ptr)) {
    std::cerr << "[my_free] ERROR: Invalid pointer!\n";
    return;
  }

  // Get the block metadata
  MemoryBlock *block = MemoryBlock::fromData(ptr);

  // Check if already free (double-free detection)
  if (block->is_free) {
    std::cerr << "[my_free] WARNING: Double free detected!\n";
    return;
  }

  // Update statistics before coalescing
  updateStatsAfterFree(block->size);

  // Mark as free
  block->is_free = true;

  // Coalesce with adjacent free blocks to reduce fragmentation
  coalesceBlock(block);
}

void *MemoryAllocator::my_realloc(void *ptr, size_t new_size) {
  // realloc(nullptr, size) is equivalent to malloc(size)
  if (!ptr) {
    return my_malloc(new_size);
  }

  // realloc(ptr, 0) is equivalent to free(ptr)
  if (new_size == 0) {
    my_free(ptr);
    return nullptr;
  }

  if (!isValidPointer(ptr)) {
    std::cerr << "[my_realloc] ERROR: Invalid pointer!\n";
    return nullptr;
  }

  MemoryBlock *block = MemoryBlock::fromData(ptr);
  size_t old_size = block->size;
  new_size = alignSize(new_size);

  // If new size fits in current block, just return the same pointer
  if (new_size <= old_size) {
    // Could potentially split the block here
    return ptr;
  }

  // Check if we can expand into the next block
  if (block->next && block->next->is_free) {
    size_t combined_size = old_size + sizeof(MemoryBlock) + block->next->size;
    if (combined_size >= new_size) {
      // Absorb the next block
      MemoryBlock *next = block->next;
      block->size = combined_size;
      block->next = next->next;
      if (block->next) {
        block->next->prev = block;
      }
      stats_.free_block_count--;
      stats_.block_count--;
      return ptr;
    }
  }

  // Allocate new block and copy data
  void *new_ptr = my_malloc(new_size);
  if (!new_ptr) {
    return nullptr;
  }

  std::memcpy(new_ptr, ptr, old_size);
  my_free(ptr);

  return new_ptr;
}

void *MemoryAllocator::my_calloc(size_t count, size_t size) {
  size_t total_size = count * size;

  // Check for overflow
  if (count != 0 && total_size / count != size) {
    std::cerr << "[my_calloc] ERROR: Size overflow!\n";
    return nullptr;
  }

  void *ptr = my_malloc(total_size);
  if (ptr) {
    std::memset(ptr, 0, total_size);
  }

  return ptr;
}

//=============================================================================
// Block Management Algorithms
//=============================================================================

MemoryBlock *MemoryAllocator::findFreeBlock(size_t size) {
  // First-fit strategy: find first block that fits
  MemoryBlock *current = free_list_;

  while (current) {
    if (current->is_free && current->size >= size) {
      return current;
    }
    current = current->next;
  }

  return nullptr; // No suitable block found
}

bool MemoryAllocator::splitBlock(MemoryBlock *block, size_t size) {
  // Calculate remaining space after allocation
  size_t remaining = block->size - size;

  // Only split if remaining space can hold metadata + minimum data
  size_t min_split_size = sizeof(MemoryBlock) + MIN_BLOCK_SIZE;

  if (remaining < min_split_size) {
    return false; // Not worth splitting
  }

  // Create new block in the remaining space
  char *new_block_addr =
      reinterpret_cast<char *>(block) + sizeof(MemoryBlock) + size;
  MemoryBlock *new_block = reinterpret_cast<MemoryBlock *>(new_block_addr);

  // Initialize the new block
  new_block->size = remaining - sizeof(MemoryBlock);
  new_block->is_free = true;
  new_block->next = block->next;
  new_block->prev = block;

  // Update the next block's prev pointer
  if (new_block->next) {
    new_block->next->prev = new_block;
  }

  // Update original block
  block->size = size;
  block->next = new_block;

  // Update statistics
  stats_.block_count++;
  stats_.free_block_count++;
  stats_.split_count++;

  return true;
}

MemoryBlock *MemoryAllocator::coalesceBlock(MemoryBlock *block) {
  bool coalesced = false;

  // Try to coalesce with next block
  if (block->next && block->next->is_free) {
    // Absorb the next block
    MemoryBlock *next = block->next;
    block->size += sizeof(MemoryBlock) + next->size;
    block->next = next->next;

    if (block->next) {
      block->next->prev = block;
    }

    stats_.block_count--;
    stats_.free_block_count--;
    stats_.coalesce_count++;
    coalesced = true;
  }

  // Try to coalesce with previous block
  if (block->prev && block->prev->is_free) {
    // Previous block absorbs current block
    MemoryBlock *prev = block->prev;
    prev->size += sizeof(MemoryBlock) + block->size;
    prev->next = block->next;

    if (prev->next) {
      prev->next->prev = prev;
    }

    stats_.block_count--;
    stats_.free_block_count--;
    stats_.coalesce_count++;
    block = prev;
    coalesced = true;
  }

  return block;
}

//=============================================================================
// Utility Functions
//=============================================================================

size_t MemoryAllocator::alignSize(size_t size) {
  return (size + ALIGNMENT - 1) & ~(ALIGNMENT - 1);
}

bool MemoryAllocator::isValidPointer(void *ptr) const {
  char *p = static_cast<char *>(ptr);
  return p >= heap_start_ + sizeof(MemoryBlock) && p < heap_end_;
}

void MemoryAllocator::updateStatsAfterAlloc(size_t size) {
  stats_.used_memory += size;
  stats_.free_memory -= size;
  stats_.total_allocations++;
  stats_.free_block_count--;
  recalculateBlockCounts();
}

void MemoryAllocator::updateStatsAfterFree(size_t size) {
  stats_.used_memory -= size;
  stats_.free_memory += size;
  stats_.total_frees++;
  stats_.free_block_count++;
}

void MemoryAllocator::recalculateBlockCounts() {
  size_t total = 0;
  size_t free_count = 0;

  MemoryBlock *current = free_list_;
  while (current) {
    total++;
    if (current->is_free) {
      free_count++;
    }
    current = current->next;
  }

  stats_.block_count = total;
  stats_.free_block_count = free_count;
}

//=============================================================================
// Statistics and Debugging
//=============================================================================

void MemoryAllocator::printStats() const {
  std::cout << "\n";
  std::cout
      << "╔══════════════════════════════════════════════════════════════╗\n";
  std::cout
      << "║           CUSTOM MEMORY ALLOCATOR - STATISTICS               ║\n";
  std::cout
      << "╠══════════════════════════════════════════════════════════════╣\n";
  std::cout << "║  Heap Size:          " << std::setw(12)
            << stats_.total_heap_size << " bytes                    ║\n";
  std::cout << "║  Used Memory:        " << std::setw(12) << stats_.used_memory
            << " bytes                    ║\n";
  std::cout << "║  Free Memory:        " << std::setw(12) << stats_.free_memory
            << " bytes                    ║\n";
  std::cout
      << "╠══════════════════════════════════════════════════════════════╣\n";
  std::cout << "║  Total Allocations:  " << std::setw(12)
            << stats_.total_allocations << "                          ║\n";
  std::cout << "║  Total Frees:        " << std::setw(12) << stats_.total_frees
            << "                          ║\n";
  std::cout << "║  Active Allocations: " << std::setw(12)
            << (stats_.total_allocations - stats_.total_frees)
            << "                          ║\n";
  std::cout
      << "╠══════════════════════════════════════════════════════════════╣\n";
  std::cout << "║  Total Blocks:       " << std::setw(12) << stats_.block_count
            << "                          ║\n";
  std::cout << "║  Free Blocks:        " << std::setw(12)
            << stats_.free_block_count << "                          ║\n";
  std::cout << "║  Split Operations:   " << std::setw(12) << stats_.split_count
            << "                          ║\n";
  std::cout << "║  Coalesce Operations:" << std::setw(12)
            << stats_.coalesce_count << "                          ║\n";
  std::cout
      << "╠══════════════════════════════════════════════════════════════╣\n";
  std::cout << "║  Fragmentation:      " << std::setw(11) << std::fixed
            << std::setprecision(2) << stats_.getFragmentationRatio()
            << "%                         ║\n";
  std::cout
      << "╚══════════════════════════════════════════════════════════════╝\n";
  std::cout << "\n";
}

void MemoryAllocator::printHeapLayout() const {
  std::cout << "\n";
  std::cout
      << "═══════════════════════════════════════════════════════════════\n";
  std::cout
      << "                    HEAP MEMORY LAYOUT                         \n";
  std::cout
      << "═══════════════════════════════════════════════════════════════\n";
  std::cout << "  Address          Size        Status      Block #\n";
  std::cout
      << "───────────────────────────────────────────────────────────────\n";

  MemoryBlock *current = free_list_;
  int block_num = 0;

  while (current) {
    char *addr = reinterpret_cast<char *>(current);
    size_t offset = addr - heap_start_;

    std::cout << "  0x" << std::hex << std::setw(8) << std::setfill('0')
              << offset << std::dec << std::setfill(' ') << "    "
              << std::setw(10) << current->size << " B"
              << "    " << (current->is_free ? "[FREE]    " : "[USED]    ")
              << "    #" << block_num++ << "\n";

    current = current->next;
  }

  std::cout
      << "───────────────────────────────────────────────────────────────\n";
  std::cout << "  Legend: [FREE] = Available   [USED] = Allocated\n";
  std::cout
      << "═══════════════════════════════════════════════════════════════\n";
  std::cout << "\n";
}

//=============================================================================
// Global Functions
//=============================================================================

void initGlobalAllocator(size_t heap_size) {
  if (g_allocator) {
    delete g_allocator;
  }
  g_allocator = new MemoryAllocator(heap_size);
}

void destroyGlobalAllocator() {
  delete g_allocator;
  g_allocator = nullptr;
}

void *custom_malloc(size_t size) {
  if (!g_allocator) {
    initGlobalAllocator();
  }
  return g_allocator->my_malloc(size);
}

void custom_free(void *ptr) {
  if (g_allocator) {
    g_allocator->my_free(ptr);
  }
}

void *custom_realloc(void *ptr, size_t size) {
  if (!g_allocator) {
    initGlobalAllocator();
  }
  return g_allocator->my_realloc(ptr, size);
}

void *custom_calloc(size_t count, size_t size) {
  if (!g_allocator) {
    initGlobalAllocator();
  }
  return g_allocator->my_calloc(count, size);
}

} // namespace CustomAllocator

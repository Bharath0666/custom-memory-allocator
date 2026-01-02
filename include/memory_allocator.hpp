/**
 * @file memory_allocator.hpp
 * @brief Custom Memory Allocator - Header File
 * 
 * A low-level memory allocator implementation demonstrating:
 * - Custom malloc() and free() functions
 * - Memory fragmentation handling
 * - Block coalescing and splitting algorithms
 * 
 * @author Custom Memory Allocator Project
 * @date 2025
 */

#ifndef MEMORY_ALLOCATOR_HPP
#define MEMORY_ALLOCATOR_HPP

#include <cstddef>
#include <cstdint>
#include <iostream>
#include <iomanip>

namespace CustomAllocator {

/**
 * @struct MemoryBlock
 * @brief Metadata structure for each memory block
 * 
 * This structure sits at the beginning of each allocation,
 * storing essential information for memory management.
 */
struct MemoryBlock {
    size_t size;            ///< Size of the data portion (excluding metadata)
    bool is_free;           ///< Flag indicating if block is available
    MemoryBlock* next;      ///< Pointer to the next block in the list
    MemoryBlock* prev;      ///< Pointer to the previous block (for coalescing)
    
    /**
     * @brief Get pointer to the data portion of this block
     * @return Pointer to usable memory after metadata
     */
    void* getData() {
        return reinterpret_cast<void*>(reinterpret_cast<char*>(this) + sizeof(MemoryBlock));
    }
    
    /**
     * @brief Get block from data pointer
     * @param data Pointer to the data portion
     * @return Pointer to the MemoryBlock metadata
     */
    static MemoryBlock* fromData(void* data) {
        return reinterpret_cast<MemoryBlock*>(
            reinterpret_cast<char*>(data) - sizeof(MemoryBlock)
        );
    }
};

/**
 * @struct MemoryStats
 * @brief Statistics about memory usage and fragmentation
 */
struct MemoryStats {
    size_t total_heap_size;      ///< Total heap size in bytes
    size_t used_memory;          ///< Currently allocated memory
    size_t free_memory;          ///< Currently available memory
    size_t total_allocations;    ///< Number of successful allocations
    size_t total_frees;          ///< Number of successful deallocations
    size_t block_count;          ///< Total number of blocks
    size_t free_block_count;     ///< Number of free blocks
    size_t coalesce_count;       ///< Number of coalescing operations
    size_t split_count;          ///< Number of split operations
    
    /**
     * @brief Calculate fragmentation ratio
     * @return Fragmentation as percentage (0-100)
     */
    double getFragmentationRatio() const {
        if (free_memory == 0) return 0.0;
        // Fragmentation = (free blocks - 1) / free blocks * 100
        // More free blocks = more fragmentation
        if (free_block_count <= 1) return 0.0;
        return (static_cast<double>(free_block_count - 1) / free_block_count) * 100.0;
    }
};

/**
 * @class MemoryAllocator
 * @brief Custom memory allocator with malloc/free implementation
 * 
 * This allocator uses a first-fit strategy with a free list.
 * Features include:
 * - Block splitting for efficient memory usage
 * - Block coalescing to reduce fragmentation
 * - Memory statistics tracking
 */
class MemoryAllocator {
public:
    /// Minimum block size to prevent excessive fragmentation
    static constexpr size_t MIN_BLOCK_SIZE = 16;
    
    /// Default heap size (1 MB)
    static constexpr size_t DEFAULT_HEAP_SIZE = 1024 * 1024;
    
    /// Alignment requirement (8 bytes for 64-bit systems)
    static constexpr size_t ALIGNMENT = 8;

private:
    char* heap_start_;          ///< Start of the managed heap
    char* heap_end_;            ///< End of the managed heap
    size_t heap_size_;          ///< Total heap size
    MemoryBlock* free_list_;    ///< Head of the free block list
    MemoryStats stats_;         ///< Memory statistics
    bool owns_memory_;          ///< Whether allocator owns the heap memory

public:
    /**
     * @brief Construct a new Memory Allocator
     * @param heap_size Size of the heap to manage (default: 1MB)
     */
    explicit MemoryAllocator(size_t heap_size = DEFAULT_HEAP_SIZE);
    
    /**
     * @brief Construct allocator with external memory
     * @param memory Pointer to pre-allocated memory
     * @param size Size of the memory region
     */
    MemoryAllocator(void* memory, size_t size);
    
    /**
     * @brief Destructor - frees managed heap if owned
     */
    ~MemoryAllocator();
    
    // Disable copy operations
    MemoryAllocator(const MemoryAllocator&) = delete;
    MemoryAllocator& operator=(const MemoryAllocator&) = delete;
    
    // Enable move operations
    MemoryAllocator(MemoryAllocator&& other) noexcept;
    MemoryAllocator& operator=(MemoryAllocator&& other) noexcept;

    /**
     * @brief Allocate memory (custom malloc)
     * @param size Number of bytes to allocate
     * @return Pointer to allocated memory, or nullptr on failure
     */
    void* my_malloc(size_t size);
    
    /**
     * @brief Deallocate memory (custom free)
     * @param ptr Pointer to memory previously allocated by my_malloc
     */
    void my_free(void* ptr);
    
    /**
     * @brief Reallocate memory to a new size
     * @param ptr Pointer to existing allocation (can be nullptr)
     * @param new_size New size in bytes
     * @return Pointer to reallocated memory, or nullptr on failure
     */
    void* my_realloc(void* ptr, size_t new_size);
    
    /**
     * @brief Allocate and zero-initialize memory (custom calloc)
     * @param count Number of elements
     * @param size Size of each element
     * @return Pointer to allocated and zeroed memory
     */
    void* my_calloc(size_t count, size_t size);
    
    /**
     * @brief Get current memory statistics
     * @return MemoryStats structure with current state
     */
    MemoryStats getStats() const { return stats_; }
    
    /**
     * @brief Print detailed memory statistics to stdout
     */
    void printStats() const;
    
    /**
     * @brief Print a visual representation of the heap
     */
    void printHeapLayout() const;
    
    /**
     * @brief Reset the allocator to initial state
     */
    void reset();
    
    /**
     * @brief Check if a pointer is valid (within heap bounds)
     * @param ptr Pointer to check
     * @return true if pointer is within managed heap
     */
    bool isValidPointer(void* ptr) const;

private:
    /**
     * @brief Initialize the heap with a single free block
     */
    void initializeHeap();
    
    /**
     * @brief Find a suitable free block (first-fit strategy)
     * @param size Required size
     * @return Pointer to suitable block, or nullptr if none found
     */
    MemoryBlock* findFreeBlock(size_t size);
    
    /**
     * @brief Split a block if it's larger than needed
     * @param block Block to potentially split
     * @param size Required size for the first part
     * @return true if split occurred
     */
    bool splitBlock(MemoryBlock* block, size_t size);
    
    /**
     * @brief Coalesce a block with adjacent free blocks
     * @param block Block to coalesce
     * @return Pointer to the resulting (possibly larger) block
     */
    MemoryBlock* coalesceBlock(MemoryBlock* block);
    
    /**
     * @brief Align size to ALIGNMENT boundary
     * @param size Size to align
     * @return Aligned size
     */
    static size_t alignSize(size_t size);
    
    /**
     * @brief Update statistics after allocation
     * @param size Allocated size
     */
    void updateStatsAfterAlloc(size_t size);
    
    /**
     * @brief Update statistics after deallocation
     * @param size Freed size
     */
    void updateStatsAfterFree(size_t size);
    
    /**
     * @brief Recalculate block counts for statistics
     */
    void recalculateBlockCounts();
};

/**
 * @brief Global allocator instance for convenience functions
 */
extern MemoryAllocator* g_allocator;

/**
 * @brief Initialize the global allocator
 * @param heap_size Size of the heap
 */
void initGlobalAllocator(size_t heap_size = MemoryAllocator::DEFAULT_HEAP_SIZE);

/**
 * @brief Destroy the global allocator
 */
void destroyGlobalAllocator();

/**
 * @brief Global malloc function using global allocator
 * @param size Number of bytes to allocate
 * @return Pointer to allocated memory
 */
void* custom_malloc(size_t size);

/**
 * @brief Global free function using global allocator
 * @param ptr Pointer to free
 */
void custom_free(void* ptr);

/**
 * @brief Global realloc function using global allocator
 * @param ptr Existing pointer
 * @param size New size
 * @return Pointer to reallocated memory
 */
void* custom_realloc(void* ptr, size_t size);

/**
 * @brief Global calloc function using global allocator
 * @param count Number of elements
 * @param size Size of each element
 * @return Pointer to allocated and zeroed memory
 */
void* custom_calloc(size_t count, size_t size);

} // namespace CustomAllocator

#endif // MEMORY_ALLOCATOR_HPP

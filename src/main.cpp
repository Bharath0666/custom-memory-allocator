/**
 * @file main.cpp
 * @brief Custom Memory Allocator - Test Suite and Demonstration
 *
 * Comprehensive tests demonstrating the memory allocator's capabilities:
 * - Basic allocation/deallocation
 * - Block splitting and coalescing
 * - Fragmentation handling
 * - Edge cases and error handling
 */

#include "memory_allocator.hpp"
#include <cassert>
#include <cstring>
#include <iomanip>
#include <iostream>
#include <string>
#include <vector>


using namespace CustomAllocator;

//=============================================================================
// Test Utilities
//=============================================================================

#define TEST_PASSED() std::cout << "  ✓ PASSED\n"
#define TEST_FAILED(msg) std::cout << "  ✗ FAILED: " << msg << "\n"

void printTestHeader(const std::string &test_name) {
  std::cout << "\n";
  std::cout
      << "┌─────────────────────────────────────────────────────────────┐\n";
  std::cout << "│ TEST: " << std::left << std::setw(54) << test_name << "│\n";
  std::cout
      << "└─────────────────────────────────────────────────────────────┘\n";
}

void printSectionHeader(const std::string &section_name) {
  std::cout << "\n▶ " << section_name << "\n";
  std::cout << "  ─────────────────────────────────────────────────\n";
}

//=============================================================================
// Test Cases
//=============================================================================

/**
 * Test 1: Basic Allocation and Deallocation
 */
bool testBasicAllocation() {
  printTestHeader("Basic Allocation and Deallocation");

  MemoryAllocator allocator(4096); // 4KB heap

  printSectionHeader("Allocating 100 bytes");
  int *ptr = static_cast<int *>(allocator.my_malloc(100));

  if (!ptr) {
    TEST_FAILED("Allocation returned nullptr");
    return false;
  }

  // Write and read data to verify memory is usable
  for (int i = 0; i < 25; i++) {
    ptr[i] = i * 10;
  }

  bool data_valid = true;
  for (int i = 0; i < 25; i++) {
    if (ptr[i] != i * 10) {
      data_valid = false;
      break;
    }
  }

  if (!data_valid) {
    TEST_FAILED("Data verification failed");
    return false;
  }

  std::cout << "  Data written and verified successfully\n";

  printSectionHeader("Freeing memory");
  allocator.my_free(ptr);

  allocator.printStats();

  TEST_PASSED();
  return true;
}

/**
 * Test 2: Multiple Allocations
 */
bool testMultipleAllocations() {
  printTestHeader("Multiple Allocations of Varying Sizes");

  MemoryAllocator allocator(8192); // 8KB heap

  struct Allocation {
    void *ptr;
    size_t size;
    const char *name;
  };

  std::vector<Allocation> allocations;

  // Allocate various sizes
  size_t sizes[] = {32, 64, 128, 256, 512, 1024};
  const char *names[] = {"Small (32B)",   "Medium-Small (64B)",
                         "Medium (128B)", "Medium-Large (256B)",
                         "Large (512B)",  "Extra-Large (1KB)"};

  printSectionHeader("Allocating multiple blocks");

  for (size_t i = 0; i < 6; i++) {
    void *ptr = allocator.my_malloc(sizes[i]);
    if (!ptr) {
      TEST_FAILED("Allocation failed for size " + std::to_string(sizes[i]));
      return false;
    }
    allocations.push_back({ptr, sizes[i], names[i]});
    std::cout << "  Allocated: " << names[i] << " at " << ptr << "\n";
  }

  allocator.printHeapLayout();
  allocator.printStats();

  printSectionHeader("Freeing all blocks");
  for (auto &alloc : allocations) {
    allocator.my_free(alloc.ptr);
    std::cout << "  Freed: " << alloc.name << "\n";
  }

  allocator.printStats();

  TEST_PASSED();
  return true;
}

/**
 * Test 3: Block Splitting Demonstration
 */
bool testBlockSplitting() {
  printTestHeader("Block Splitting Algorithm");

  MemoryAllocator allocator(4096); // 4KB heap

  printSectionHeader("Initial heap state (single large block)");
  allocator.printHeapLayout();

  printSectionHeader("Allocating 100 bytes (should split the block)");
  void *ptr1 = allocator.my_malloc(100);
  allocator.printHeapLayout();

  printSectionHeader("Allocating another 200 bytes (should split again)");
  void *ptr2 = allocator.my_malloc(200);
  allocator.printHeapLayout();

  printSectionHeader("Allocating 50 bytes");
  void *ptr3 = allocator.my_malloc(50);
  allocator.printHeapLayout();

  auto stats = allocator.getStats();
  std::cout << "\n  Split operations performed: " << stats.split_count << "\n";

  if (stats.split_count < 3) {
    TEST_FAILED("Expected at least 3 split operations");
    return false;
  }

  // Cleanup
  allocator.my_free(ptr1);
  allocator.my_free(ptr2);
  allocator.my_free(ptr3);

  TEST_PASSED();
  return true;
}

/**
 * Test 4: Block Coalescing Demonstration
 */
bool testBlockCoalescing() {
  printTestHeader("Block Coalescing Algorithm");

  MemoryAllocator allocator(4096); // 4KB heap

  // Allocate three adjacent blocks
  printSectionHeader("Allocating three adjacent blocks");
  void *ptr1 = allocator.my_malloc(100);
  void *ptr2 = allocator.my_malloc(100);
  void *ptr3 = allocator.my_malloc(100);

  allocator.printHeapLayout();

  // Free the middle block first
  printSectionHeader("Freeing middle block (ptr2)");
  allocator.my_free(ptr2);
  allocator.printHeapLayout();

  // Free the first block - should coalesce with middle
  printSectionHeader(
      "Freeing first block (ptr1) - should coalesce with middle");
  allocator.my_free(ptr1);
  allocator.printHeapLayout();

  // Free the last block - should coalesce all three
  printSectionHeader("Freeing last block (ptr3) - should coalesce all");
  allocator.my_free(ptr3);
  allocator.printHeapLayout();

  auto stats = allocator.getStats();
  std::cout << "\n  Coalesce operations performed: " << stats.coalesce_count
            << "\n";

  allocator.printStats();

  if (stats.coalesce_count < 2) {
    TEST_FAILED("Expected at least 2 coalesce operations");
    return false;
  }

  TEST_PASSED();
  return true;
}

/**
 * Test 5: Fragmentation Scenario
 */
bool testFragmentation() {
  printTestHeader("Memory Fragmentation Scenario");

  MemoryAllocator allocator(4096); // 4KB heap

  printSectionHeader("Creating fragmentation pattern");
  std::cout << "  Allocating blocks: A, B, C, D, E\n";

  void *a = allocator.my_malloc(100);
  void *b = allocator.my_malloc(100);
  void *c = allocator.my_malloc(100);
  void *d = allocator.my_malloc(100);
  void *e = allocator.my_malloc(100);

  allocator.printHeapLayout();

  // Free alternating blocks to create fragmentation
  printSectionHeader("Freeing alternate blocks (B, D) - creates fragmentation");
  allocator.my_free(b);
  allocator.my_free(d);

  allocator.printHeapLayout();
  allocator.printStats();

  auto stats = allocator.getStats();
  std::cout << "  Fragmentation ratio: " << std::fixed << std::setprecision(2)
            << stats.getFragmentationRatio() << "%\n";

  // Try to allocate a larger block that doesn't fit in fragments
  printSectionHeader(
      "Attempting to allocate 300 bytes (larger than fragments)");
  void *large = allocator.my_malloc(300);

  if (large) {
    std::cout
        << "  Allocation succeeded - found space after allocated blocks\n";
  } else {
    std::cout << "  Allocation failed - demonstrates fragmentation effect\n";
  }

  allocator.printHeapLayout();

  // Cleanup
  allocator.my_free(a);
  allocator.my_free(c);
  allocator.my_free(e);
  if (large)
    allocator.my_free(large);

  TEST_PASSED();
  return true;
}

/**
 * Test 6: Realloc Functionality
 */
bool testRealloc() {
  printTestHeader("Realloc Functionality");

  MemoryAllocator allocator(4096);

  printSectionHeader("Initial allocation of 50 bytes");
  char *ptr = static_cast<char *>(allocator.my_malloc(50));
  std::strcpy(ptr, "Hello, Custom Allocator!");
  std::cout << "  Data: " << ptr << "\n";

  printSectionHeader("Reallocating to 100 bytes");
  ptr = static_cast<char *>(allocator.my_realloc(ptr, 100));
  std::cout << "  Data after realloc: " << ptr << "\n";

  // Verify data is preserved
  if (std::strcmp(ptr, "Hello, Custom Allocator!") != 0) {
    TEST_FAILED("Data not preserved after realloc");
    return false;
  }

  // Extend the string
  std::strcat(ptr, " Now with more space!");
  std::cout << "  Extended data: " << ptr << "\n";

  allocator.my_free(ptr);
  allocator.printStats();

  TEST_PASSED();
  return true;
}

/**
 * Test 7: Calloc Functionality
 */
bool testCalloc() {
  printTestHeader("Calloc Functionality (Zero-Initialized Memory)");

  MemoryAllocator allocator(4096);

  printSectionHeader("Allocating 10 integers with calloc");
  int *arr = static_cast<int *>(allocator.my_calloc(10, sizeof(int)));

  if (!arr) {
    TEST_FAILED("Calloc returned nullptr");
    return false;
  }

  // Verify all values are zero
  bool all_zero = true;
  for (int i = 0; i < 10; i++) {
    if (arr[i] != 0) {
      all_zero = false;
      break;
    }
  }

  if (!all_zero) {
    TEST_FAILED("Memory not zero-initialized");
    return false;
  }

  std::cout << "  All 10 integers verified as zero-initialized\n";

  // Use the array
  for (int i = 0; i < 10; i++) {
    arr[i] = i * i;
  }

  std::cout << "  Array contents after modification: ";
  for (int i = 0; i < 10; i++) {
    std::cout << arr[i] << " ";
  }
  std::cout << "\n";

  allocator.my_free(arr);

  TEST_PASSED();
  return true;
}

/**
 * Test 8: Edge Cases
 */
bool testEdgeCases() {
  printTestHeader("Edge Cases and Error Handling");

  MemoryAllocator allocator(1024);

  printSectionHeader("Test: malloc(0)");
  void *ptr0 = allocator.my_malloc(0);
  if (ptr0 == nullptr) {
    std::cout << "  Correctly returned nullptr for size 0\n";
  }

  printSectionHeader("Test: free(nullptr)");
  allocator.my_free(nullptr); // Should not crash
  std::cout << "  Correctly handled free(nullptr)\n";

  printSectionHeader("Test: Double free detection");
  void *ptr = allocator.my_malloc(50);
  allocator.my_free(ptr);
  allocator.my_free(ptr); // Should warn about double free

  printSectionHeader("Test: Large allocation (larger than heap)");
  void *large = allocator.my_malloc(2000); // Larger than 1KB heap
  if (large == nullptr) {
    std::cout << "  Correctly returned nullptr for oversized allocation\n";
  }

  printSectionHeader("Test: Many small allocations");
  std::vector<void *> small_allocs;
  for (int i = 0; i < 20; i++) {
    void *p = allocator.my_malloc(16);
    if (p) {
      small_allocs.push_back(p);
    } else {
      std::cout << "  Ran out of memory after " << i << " allocations\n";
      break;
    }
  }
  std::cout << "  Successfully allocated " << small_allocs.size()
            << " small blocks\n";

  // Cleanup
  for (void *p : small_allocs) {
    allocator.my_free(p);
  }

  allocator.printStats();

  TEST_PASSED();
  return true;
}

/**
 * Test 9: Memory Reuse After Free
 */
bool testMemoryReuse() {
  printTestHeader("Memory Reuse After Free");

  MemoryAllocator allocator(2048);

  printSectionHeader("Allocate -> Free -> Reallocate pattern");

  // First allocation
  void *ptr1 = allocator.my_malloc(200);
  std::cout << "  First allocation at: " << ptr1 << "\n";

  allocator.my_free(ptr1);

  // Second allocation (should reuse the freed block)
  void *ptr2 = allocator.my_malloc(150);
  std::cout << "  Second allocation at: " << ptr2 << "\n";

  if (ptr1 == ptr2) {
    std::cout << "  ✓ Memory block was successfully reused!\n";
  } else {
    std::cout << "  Memory was allocated from a different location\n";
  }

  allocator.my_free(ptr2);
  allocator.printStats();

  TEST_PASSED();
  return true;
}

/**
 * Test 10: Global Allocator Functions
 */
bool testGlobalAllocator() {
  printTestHeader("Global Allocator Functions");

  printSectionHeader("Initializing global allocator");
  initGlobalAllocator(4096);

  printSectionHeader("Using custom_malloc()");
  int *arr = static_cast<int *>(custom_malloc(sizeof(int) * 5));
  for (int i = 0; i < 5; i++) {
    arr[i] = i + 1;
  }
  std::cout << "  Array: ";
  for (int i = 0; i < 5; i++) {
    std::cout << arr[i] << " ";
  }
  std::cout << "\n";

  printSectionHeader("Using custom_realloc()");
  arr = static_cast<int *>(custom_realloc(arr, sizeof(int) * 10));
  for (int i = 5; i < 10; i++) {
    arr[i] = i + 1;
  }
  std::cout << "  Extended array: ";
  for (int i = 0; i < 10; i++) {
    std::cout << arr[i] << " ";
  }
  std::cout << "\n";

  printSectionHeader("Using custom_free()");
  custom_free(arr);

  g_allocator->printStats();

  destroyGlobalAllocator();

  TEST_PASSED();
  return true;
}

//=============================================================================
// Main Entry Point
//=============================================================================

int main() {
  std::cout << "\n";
  std::cout << "╔══════════════════════════════════════════════════════════════"
               "════╗\n";
  std::cout << "║                                                              "
               "    ║\n";
  std::cout << "║            CUSTOM MEMORY ALLOCATOR - C++ IMPLEMENTATION      "
               "    ║\n";
  std::cout << "║                                                              "
               "    ║\n";
  std::cout << "║  Features:                                                   "
               "    ║\n";
  std::cout << "║    • Custom malloc() and free() functions                    "
               "    ║\n";
  std::cout << "║    • Block splitting for efficient memory usage              "
               "    ║\n";
  std::cout << "║    • Block coalescing to minimize fragmentation              "
               "    ║\n";
  std::cout << "║    • Memory statistics and heap visualization                "
               "    ║\n";
  std::cout << "║                                                              "
               "    ║\n";
  std::cout << "╚══════════════════════════════════════════════════════════════"
               "════╝\n";

  int passed = 0;
  int failed = 0;

  // Run all tests
  if (testBasicAllocation())
    passed++;
  else
    failed++;
  if (testMultipleAllocations())
    passed++;
  else
    failed++;
  if (testBlockSplitting())
    passed++;
  else
    failed++;
  if (testBlockCoalescing())
    passed++;
  else
    failed++;
  if (testFragmentation())
    passed++;
  else
    failed++;
  if (testRealloc())
    passed++;
  else
    failed++;
  if (testCalloc())
    passed++;
  else
    failed++;
  if (testEdgeCases())
    passed++;
  else
    failed++;
  if (testMemoryReuse())
    passed++;
  else
    failed++;
  if (testGlobalAllocator())
    passed++;
  else
    failed++;

  // Print summary
  std::cout << "\n";
  std::cout << "╔══════════════════════════════════════════════════════════════"
               "════╗\n";
  std::cout << "║                        TEST SUMMARY                          "
               "    ║\n";
  std::cout << "╠══════════════════════════════════════════════════════════════"
               "════╣\n";
  std::cout << "║  Tests Passed: " << std::setw(3) << passed
            << "                                              ║\n";
  std::cout << "║  Tests Failed: " << std::setw(3) << failed
            << "                                              ║\n";
  std::cout << "║  Total Tests:  " << std::setw(3) << (passed + failed)
            << "                                              ║\n";
  std::cout << "╠══════════════════════════════════════════════════════════════"
               "════╣\n";

  if (failed == 0) {
    std::cout << "║                    ✓ ALL TESTS PASSED!                     "
                 "     ║\n";
  } else {
    std::cout << "║                    ✗ SOME TESTS FAILED                     "
                 "     ║\n";
  }

  std::cout << "╚══════════════════════════════════════════════════════════════"
               "════╝\n";
  std::cout << "\n";

  return failed > 0 ? 1 : 0;
}

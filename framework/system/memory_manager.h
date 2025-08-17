#ifndef MEMORY_MANAGER_H
#define MEMORY_MANAGER_H

#include "os_config.h"
#include <vector>
#include <memory>

/**
 * @file memory_manager.h
 * @brief Memory management system for M5Stack Tab5
 * 
 * Provides memory allocation, tracking, and pool management
 * with debugging and leak detection capabilities.
 */

struct MemoryBlock {
    void* ptr;
    size_t size;
    uint32_t timestamp;
    const char* file;
    int line;
    bool inUse;
};

class MemoryPool {
public:
    MemoryPool(size_t blockSize, size_t blockCount);
    ~MemoryPool();

    void* allocate();
    bool deallocate(void* ptr);
    size_t getBlockSize() const { return m_blockSize; }
    size_t getFreeBlocks() const { return m_freeBlocks; }
    size_t getTotalBlocks() const { return m_totalBlocks; }

private:
    size_t m_blockSize;
    size_t m_totalBlocks;
    size_t m_freeBlocks;
    void* m_memory;
    std::vector<bool> m_blockUsed;
};

class MemoryManager {
public:
    MemoryManager() = default;
    ~MemoryManager();

    /**
     * @brief Initialize the memory manager
     * @return OS_OK on success, error code on failure
     */
    os_error_t initialize();

    /**
     * @brief Allocate memory with tracking
     * @param size Size in bytes to allocate
     * @param file Source file for debugging (use __FILE__)
     * @param line Source line for debugging (use __LINE__)
     * @return Pointer to allocated memory or nullptr on failure
     */
    void* allocate(size_t size, const char* file = nullptr, int line = 0);

    /**
     * @brief Deallocate tracked memory
     * @param ptr Pointer to memory to deallocate
     * @return true on success, false on failure
     */
    bool deallocate(void* ptr);

    /**
     * @brief Allocate from a specific memory pool
     * @param poolIndex Pool index (0-based)
     * @return Pointer to allocated memory or nullptr on failure
     */
    void* allocateFromPool(size_t poolIndex);

    /**
     * @brief Deallocate from a specific memory pool
     * @param ptr Pointer to memory to deallocate
     * @param poolIndex Pool index (0-based)
     * @return true on success, false on failure
     */
    bool deallocateFromPool(void* ptr, size_t poolIndex);

    /**
     * @brief Get total allocated memory
     * @return Total allocated memory in bytes
     */
    size_t getTotalAllocated() const { return m_totalAllocated; }

    /**
     * @brief Get peak allocated memory
     * @return Peak allocated memory in bytes
     */
    size_t getPeakAllocated() const { return m_peakAllocated; }

    /**
     * @brief Get number of active allocations
     * @return Number of active allocations
     */
    size_t getActiveAllocations() const { return m_activeBlocks.size(); }

    /**
     * @brief Check for memory leaks
     * @return Number of leaked blocks
     */
    size_t checkLeaks();

    /**
     * @brief Print memory statistics
     */
    void printStats();

    /**
     * @brief Force garbage collection
     */
    void garbageCollect();

    /**
     * @brief Get free heap memory
     * @return Free heap memory in bytes
     */
    size_t getFreeHeap() const;

    /**
     * @brief Get largest free block
     * @return Size of largest free block in bytes
     */
    size_t getLargestFreeBlock() const;

private:
    /**
     * @brief Find memory block by pointer
     * @param ptr Pointer to find
     * @return Iterator to block or end() if not found
     */
    std::vector<MemoryBlock>::iterator findBlock(void* ptr);

    /**
     * @brief Update memory statistics
     */
    void updateStats();

    // Memory tracking
    std::vector<MemoryBlock> m_activeBlocks;
    size_t m_totalAllocated = 0;
    size_t m_peakAllocated = 0;
    uint32_t m_allocationCount = 0;
    uint32_t m_deallocationCount = 0;

    // Memory pools for common sizes
    std::vector<std::unique_ptr<MemoryPool>> m_pools;
    bool m_initialized = false;
};

// Memory allocation macros for debugging
#ifdef OS_DEBUG_ENABLED
#define OS_MALLOC(size) OS().getMemoryManager().allocate(size, __FILE__, __LINE__)
#define OS_FREE(ptr) OS().getMemoryManager().deallocate(ptr)
#else
#define OS_MALLOC(size) OS().getMemoryManager().allocate(size)
#define OS_FREE(ptr) OS().getMemoryManager().deallocate(ptr)
#endif

#endif // MEMORY_MANAGER_H
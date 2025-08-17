#include "memory_manager.h"
#include <esp_log.h>
#include <esp_heap_caps.h>

static const char* TAG = "MemoryManager";

// MemoryPool Implementation
MemoryPool::MemoryPool(size_t blockSize, size_t blockCount) 
    : m_blockSize(blockSize), m_totalBlocks(blockCount), m_freeBlocks(blockCount) {
    
    size_t totalSize = blockSize * blockCount;
    m_memory = heap_caps_malloc(totalSize, MALLOC_CAP_DEFAULT);
    
    if (m_memory) {
        m_blockUsed.resize(blockCount, false);
        ESP_LOGI(TAG, "Created memory pool: %d blocks of %d bytes", blockCount, blockSize);
    } else {
        ESP_LOGE(TAG, "Failed to allocate memory pool");
        m_totalBlocks = 0;
        m_freeBlocks = 0;
    }
}

MemoryPool::~MemoryPool() {
    if (m_memory) {
        heap_caps_free(m_memory);
    }
}

void* MemoryPool::allocate() {
    if (m_freeBlocks == 0) {
        return nullptr;
    }

    for (size_t i = 0; i < m_totalBlocks; i++) {
        if (!m_blockUsed[i]) {
            m_blockUsed[i] = true;
            m_freeBlocks--;
            return static_cast<char*>(m_memory) + (i * m_blockSize);
        }
    }

    return nullptr;
}

bool MemoryPool::deallocate(void* ptr) {
    if (!ptr || !m_memory) {
        return false;
    }

    char* charPtr = static_cast<char*>(ptr);
    char* basePtr = static_cast<char*>(m_memory);
    
    if (charPtr < basePtr) {
        return false;
    }

    size_t offset = charPtr - basePtr;
    if (offset % m_blockSize != 0) {
        return false;
    }

    size_t blockIndex = offset / m_blockSize;
    if (blockIndex >= m_totalBlocks) {
        return false;
    }

    if (!m_blockUsed[blockIndex]) {
        return false; // Double free
    }

    m_blockUsed[blockIndex] = false;
    m_freeBlocks++;
    return true;
}

// MemoryManager Implementation
MemoryManager::~MemoryManager() {
    if (m_initialized) {
        checkLeaks();
    }
}

os_error_t MemoryManager::initialize() {
    if (m_initialized) {
        return OS_OK;
    }

    ESP_LOGI(TAG, "Initializing Memory Manager");

    // Create memory pools for common allocation sizes
    m_pools.push_back(std::make_unique<MemoryPool>(16, 64));    // Small objects
    m_pools.push_back(std::make_unique<MemoryPool>(64, 32));    // Medium objects
    m_pools.push_back(std::make_unique<MemoryPool>(256, 16));   // Large objects
    m_pools.push_back(std::make_unique<MemoryPool>(1024, 8));   // Very large objects

    // Reserve space for tracking allocations
    m_activeBlocks.reserve(256);

    m_initialized = true;
    ESP_LOGI(TAG, "Memory Manager initialized with %d pools", m_pools.size());
    
    return OS_OK;
}

void* MemoryManager::allocate(size_t size, const char* file, int line) {
    if (!m_initialized || size == 0) {
        return nullptr;
    }

    void* ptr = nullptr;

    // Try to allocate from pools first for small allocations
    if (size <= 1024) {
        for (auto& pool : m_pools) {
            if (size <= pool->getBlockSize() && pool->getFreeBlocks() > 0) {
                ptr = pool->allocate();
                if (ptr) {
                    break;
                }
            }
        }
    }

    // Fall back to heap allocation
    if (!ptr) {
        ptr = heap_caps_malloc(size, MALLOC_CAP_DEFAULT);
    }

    if (ptr) {
        // Track the allocation
        MemoryBlock block;
        block.ptr = ptr;
        block.size = size;
        block.timestamp = millis();
        block.file = file;
        block.line = line;
        block.inUse = true;

        m_activeBlocks.push_back(block);
        m_totalAllocated += size;
        m_allocationCount++;

        if (m_totalAllocated > m_peakAllocated) {
            m_peakAllocated = m_totalAllocated;
        }

        #if OS_DEBUG_ENABLED >= 3
        ESP_LOGD(TAG, "Allocated %d bytes at %p (%s:%d)", size, ptr, 
                file ? file : "unknown", line);
        #endif
    } else {
        ESP_LOGW(TAG, "Failed to allocate %d bytes", size);
    }

    return ptr;
}

bool MemoryManager::deallocate(void* ptr) {
    if (!ptr || !m_initialized) {
        return false;
    }

    auto it = findBlock(ptr);
    if (it == m_activeBlocks.end()) {
        ESP_LOGW(TAG, "Attempt to free untracked pointer %p", ptr);
        return false;
    }

    size_t size = it->size;
    bool freedFromPool = false;

    // Try to free from pools first
    for (auto& pool : m_pools) {
        if (pool->deallocate(ptr)) {
            freedFromPool = true;
            break;
        }
    }

    // Fall back to heap deallocation
    if (!freedFromPool) {
        heap_caps_free(ptr);
    }

    // Update tracking
    m_totalAllocated -= size;
    m_deallocationCount++;
    m_activeBlocks.erase(it);

    #if OS_DEBUG_ENABLED >= 3
    ESP_LOGD(TAG, "Deallocated %d bytes at %p", size, ptr);
    #endif

    return true;
}

void* MemoryManager::allocateFromPool(size_t poolIndex) {
    if (!m_initialized || poolIndex >= m_pools.size()) {
        return nullptr;
    }

    return m_pools[poolIndex]->allocate();
}

bool MemoryManager::deallocateFromPool(void* ptr, size_t poolIndex) {
    if (!ptr || !m_initialized || poolIndex >= m_pools.size()) {
        return false;
    }

    return m_pools[poolIndex]->deallocate(ptr);
}

size_t MemoryManager::checkLeaks() {
    size_t leakCount = 0;
    uint32_t currentTime = millis();

    for (const auto& block : m_activeBlocks) {
        if (block.inUse) {
            leakCount++;
            ESP_LOGW(TAG, "Memory leak: %d bytes at %p, allocated %d ms ago (%s:%d)",
                    block.size, block.ptr, currentTime - block.timestamp,
                    block.file ? block.file : "unknown", block.line);
        }
    }

    if (leakCount > 0) {
        ESP_LOGW(TAG, "Found %d memory leaks", leakCount);
    }

    return leakCount;
}

void MemoryManager::printStats() {
    ESP_LOGI(TAG, "=== Memory Statistics ===");
    ESP_LOGI(TAG, "Total allocated: %d bytes", m_totalAllocated);
    ESP_LOGI(TAG, "Peak allocated: %d bytes", m_peakAllocated);
    ESP_LOGI(TAG, "Active allocations: %d", m_activeBlocks.size());
    ESP_LOGI(TAG, "Total allocations: %d", m_allocationCount);
    ESP_LOGI(TAG, "Total deallocations: %d", m_deallocationCount);
    ESP_LOGI(TAG, "Free heap: %d bytes", getFreeHeap());
    ESP_LOGI(TAG, "Largest free block: %d bytes", getLargestFreeBlock());

    ESP_LOGI(TAG, "=== Pool Statistics ===");
    for (size_t i = 0; i < m_pools.size(); i++) {
        auto& pool = m_pools[i];
        ESP_LOGI(TAG, "Pool %d: %d/%d blocks free (%d bytes each)", 
                i, pool->getFreeBlocks(), pool->getTotalBlocks(), pool->getBlockSize());
    }
}

void MemoryManager::garbageCollect() {
    // For now, just check for leaks and log them
    // In a more advanced implementation, this could free unused resources
    checkLeaks();
}

size_t MemoryManager::getFreeHeap() const {
    return heap_caps_get_free_size(MALLOC_CAP_DEFAULT);
}

size_t MemoryManager::getLargestFreeBlock() const {
    return heap_caps_get_largest_free_block(MALLOC_CAP_DEFAULT);
}

std::vector<MemoryBlock>::iterator MemoryManager::findBlock(void* ptr) {
    return std::find_if(m_activeBlocks.begin(), m_activeBlocks.end(),
                       [ptr](const MemoryBlock& block) {
                           return block.ptr == ptr;
                       });
}
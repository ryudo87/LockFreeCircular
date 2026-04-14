#ifndef LOCK_FREE_CIRCULAR_QUEUE_H
#define LOCK_FREE_CIRCULAR_QUEUE_H

#include <atomic>
#include <memory>
#include <cstring>

template <typename T>
class MultiProducerCircularQueue {
private:
    struct Node {
        T value;
        std::atomic<size_t> sequence;
        
        Node() : sequence(0) {}
    };
    
    static constexpr size_t CACHE_LINE_SIZE = 64;
    
    alignas(CACHE_LINE_SIZE) std::atomic<size_t> enqueue_pos;
    alignas(CACHE_LINE_SIZE) std::atomic<size_t> dequeue_pos;
    
    std::unique_ptr<Node[]> buffer;
    size_t capacity;
    size_t capacity_mask;
    
public:
    explicit MultiProducerCircularQueue(size_t size) 
        : enqueue_pos(0), dequeue_pos(0), capacity(size), capacity_mask(size - 1) {
        // Ensure size is a power of 2
        if ((size & capacity_mask) != 0) {
            throw std::invalid_argument("Queue size must be a power of 2");
        }
        buffer = std::make_unique<Node[]>(size);
        for (size_t i = 0; i < size; ++i) {
            buffer[i].sequence = i;
        }
    }
    
    ~MultiProducerCircularQueue() = default;
    
    // Delete copy operations
    MultiProducerCircularQueue(const MultiProducerCircularQueue&) = delete;
    MultiProducerCircularQueue& operator=(const MultiProducerCircularQueue&) = delete;
    
    bool enqueue(const T& value) {
        size_t pos = enqueue_pos.load(std::memory_order_acquire);
        
        while (true) {
            Node& node = buffer[pos & capacity_mask];
            size_t seq = node.sequence.load(std::memory_order_acquire);
            
            intptr_t diff = static_cast<intptr_t>(seq) - static_cast<intptr_t>(pos);
            
            if (diff == 0) {
                if (enqueue_pos.compare_exchange_strong(pos, pos + 1, 
                        std::memory_order_release, std::memory_order_acquire)) {
                    node.value = value;
                    node.sequence.store(pos + 1, std::memory_order_release);
                    return true;
                }
            } else if (diff < 0) {
                // Queue is full
                return false;
            } else {
                pos = enqueue_pos.load(std::memory_order_acquire);
            }
        }
    }
    
    bool enqueue(T&& value) {
        size_t pos = enqueue_pos.load(std::memory_order_acquire);
        
        while (true) {
            Node& node = buffer[pos & capacity_mask];
            size_t seq = node.sequence.load(std::memory_order_acquire);
            
            intptr_t diff = static_cast<intptr_t>(seq) - static_cast<intptr_t>(pos);
            
            if (diff == 0) {
                if (enqueue_pos.compare_exchange_strong(pos, pos + 1, 
                        std::memory_order_release, std::memory_order_acquire)) {
                    node.value = std::move(value);
                    node.sequence.store(pos + 1, std::memory_order_release);
                    return true;
                }
            } else if (diff < 0) {
                // Queue is full
                return false;
            } else {
                pos = enqueue_pos.load(std::memory_order_acquire);
            }
        }
    }
    
    bool dequeue(T& value) {
        size_t pos = dequeue_pos.load(std::memory_order_acquire);
        
        while (true) {
            Node& node = buffer[pos & capacity_mask];
            size_t seq = node.sequence.load(std::memory_order_acquire);
            
            intptr_t diff = static_cast<intptr_t>(seq) - static_cast<intptr_t>(pos + 1);
            
            if (diff == 0) {
                if (dequeue_pos.compare_exchange_strong(pos, pos + 1, 
                        std::memory_order_release, std::memory_order_acquire)) {
                    value = node.value;
                    node.sequence.store(pos + capacity_mask + 1, std::memory_order_release);
                    return true;
                }
            } else if (diff < 0) {
                // Queue is empty
                return false;
            } else {
                pos = dequeue_pos.load(std::memory_order_acquire);
            }
        }
    }
    
    size_t size() const {
        size_t enq = enqueue_pos.load(std::memory_order_acquire);
        size_t deq = dequeue_pos.load(std::memory_order_acquire);
        return (enq >= deq) ? (enq - deq) : 0;
    }
    
    bool empty() const {
        return enqueue_pos.load(std::memory_order_acquire) == 
               dequeue_pos.load(std::memory_order_acquire);
    }
    
    size_t get_capacity() const {
        return capacity;
    }
};

#endif // LOCK_FREE_CIRCULAR_QUEUE_H

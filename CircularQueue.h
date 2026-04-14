#include <vector>
#include <atomic>

template <typename T>
class CircularQueue {
    std::vector<T> data;
    const size_t capacity; //capacity is power of 2
    size_t Size;   // capacity is Size + 1, so we can use bitwise AND for indexing, which is faster than modulus.
    //优化方法：使用 alignas(64) 强制让 head 和 tail 分布在不同的缓存行。
    alignas(64) std::atomic<size_t> head;      // 队首索引（由消费者更新）
    alignas(64) std::atomic<size_t> tail;      // 队尾索引（由生产者更新）
    //关键点：使用 std::atomic 包装索引。这保证了对 head 和 tail 的读写是原子性的，不会出现“只改了一半”的情况。
public:

    // 初始化：由于循环队列判满逻辑，我们需要多申请一个空间（capacity = k + 1）
    CircularQueue(size_t k) : capacity(k+1), head(0), tail(0) {
        data.resize(capacity);
        Size = k;//Size is 2,4,8..
    }
    bool enQueuePowerOfTwo(const T& value) {
        size_t current_tail = tail.load(std::memory_order_relaxed);
        size_t next_tail = (current_tail + 1); // Note: We mask only when indexing
        if (next_tail - head.load(std::memory_order_acquire) > Size) {
            return false; // Buffer Full
        }
        data[current_tail & (Size - 1)] = value; // Masking for power of two capacity
        tail.store(next_tail, std::memory_order_release);

        std::cout << "---- enQueuePowerOfTwo " << value << std::endl;
        return true;
    }

    bool enQueue(T value) {//入队操作 (enQueue) —— 生产者视角
        // 1. 读取当前的 tail 索引（使用 relaxed 语义，因为只有生产者自己会改 tail，不需要同步）
        size_t current_tail = tail.load(std::memory_order_relaxed);

        size_t next_tail = (current_tail + 1) % capacity;
        if (next_tail == head.load(std::memory_order_acquire)) {
            return false; // 队列已满
        }
        // 3. 将 value 放入当前 tail 索引位置
        data[current_tail] = value;
        // 核心：release 保证了上面的 data[current_tail] = value 必须先于这一步完成
        tail.store(next_tail, std::memory_order_release); // 使用 release 语义更新 tail，确保之前的写入对消费者可见
    
        std::cout << "---- enQueue " << value << std::endl;
        return true;
    }

    bool deQueue() {//出队操作 (deQueue) —— 消费者视角
        // 1. 读取当前的 head 索引（使用 relaxed 语义，因为只有消费者自己会改 head，不需要同步）
        size_t current_head = head.load(std::memory_order_relaxed);
        // 2. 检查队列是否为空 ，如果head 索引等于 tail 索引，说明队列为空，返回 false
        if (current_head == tail.load(std::memory_order_acquire)) {
            return false; // 队列为空
        }

        T value = data[current_head];
        std::cout << "---- deQueue " << value << std::endl;
 
        head.store((current_head + 1) % capacity, std::memory_order_release); // 使用 release 语义更新 head，确保之前的读取对生产者可见
        
        return true;
    }

    bool isEmpty() const {
        return head.load(std::memory_order_acquire) == tail.load(std::memory_order_acquire);
    }

    bool isFull() const {
        size_t next_tail = (tail.load(std::memory_order_acquire) + 1) % capacity;
        return next_tail == head.load(std::memory_order_acquire);
    }




    void print() {
        std::cout << "---CircularQueue capacity : " << capacity 
        << "  alignas(64) sizeof(head) " << sizeof(head)
        << "  sizeof(size_t) " << sizeof(size_t)
        << "  head is " << head.load(std::memory_order_acquire) 
        << "  tail is " << tail.load(std::memory_order_acquire) << std::endl;
    }
};
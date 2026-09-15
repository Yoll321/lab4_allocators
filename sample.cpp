#include <iostream>
#include <map>
#include <memory>
#include <vector>
#include <algorithm>
#include <utility>
#include <cstdint>

// ============================================================
// 1. Собственный аллокатор памяти (PoolAllocator)
// ============================================================
//
// - параметризуется количеством элементов, выделяемых за раз (ChunkSize)
// - stateful: хранит общий пул через shared_ptr, копии аллокатора
//   (порождённые копирующим конструктором) разделяют один пул
// - расширяемость: при нехватке места в текущем блоке выделяется новый блок
// - поэлементное освобождение: через free-list для n == 1
// - полное освобождение: автоматически в деструкторе Pool (RAII)
//
template <typename T, std::size_t ChunkSize>
class PoolAllocator {
private:
    struct Chunk {
        std::unique_ptr<char[]> memory;
        std::size_t capacity;
        std::size_t used = 0;

        explicit Chunk(std::size_t count)
            : memory(new char[count * sizeof(T)]), capacity(count) {}
    };

    struct Pool {
        std::vector<std::unique_ptr<Chunk>> chunks;
        std::vector<T*> freeList;
    };

    std::shared_ptr<Pool> pool_;

    template <typename U, std::size_t N>
    friend class PoolAllocator;

    Chunk* findChunkWithSpace(std::size_t n) {
        if (!pool_->chunks.empty()) {
            Chunk* last = pool_->chunks.back().get();
            if (last->used + n <= last->capacity) {
                return last;
            }
        }
        return nullptr;
    }

public:
    using value_type = T;

    template <typename U>
    struct rebind {
        using other = PoolAllocator<U, ChunkSize>;
    };

    PoolAllocator() : pool_(std::make_shared<Pool>()) {
        std::cout << "[PoolAllocator<" << sizeof(T)
                  << " byte элементы, chunk=" << ChunkSize << ">] создан новый пул\n";
    }

    template <typename U>
    PoolAllocator(const PoolAllocator<U, ChunkSize>& other) noexcept
        : pool_(other.pool_) {}

    PoolAllocator(const PoolAllocator&) noexcept = default;
    PoolAllocator& operator=(const PoolAllocator&) noexcept = default;

    T* allocate(std::size_t n) {
        if (n == 0) return nullptr;

        if (n == 1 && !pool_->freeList.empty()) {
            T* p = pool_->freeList.back();
            pool_->freeList.pop_back();
            return p;
        }

        Chunk* chunk = findChunkWithSpace(n);
        if (!chunk) {
            std::size_t newChunkSize = std::max(n, ChunkSize);
            std::cout << "  [allocate] расширяем пул: новый блок на "
                      << newChunkSize << " элементов\n";
            pool_->chunks.push_back(std::make_unique<Chunk>(newChunkSize));
            chunk = pool_->chunks.back().get();
        }

        T* result = reinterpret_cast<T*>(chunk->memory.get()) + chunk->used;
        chunk->used += n;
        return result;
    }
    // !!!!
    void deallocate(T* p, std::size_t n) noexcept {
        if (n == 1) {
            pool_->freeList.push_back(p);
        }
    }

    template <typename U, std::size_t N>
    bool operator==(const PoolAllocator<U, N>& other) const noexcept {
        return pool_ == other.pool_;
    }

    // !!!!!
    template <typename U, std::size_t N>
    bool operator!=(const PoolAllocator<U, N>& other) const noexcept {
        return !(*this == other);
    }
};

// ============================================================
// 2. Собственный контейнер (односвязный список)
// ============================================================
//
// - параметризуется аллокатором (по умолчанию std::allocator<T>)
// - push_back для добавления, forward-итератор для обхода
// - size(), empty(), begin()/end() — совместимость со стилем STL
//
template <typename T, typename Allocator = std::allocator<T>>
class ForwardList {
private:
    struct Node {
        T value;
        Node* next;
        Node(const T& v, Node* n) : value(v), next(n) {}
    };

    using NodeAllocator =
        typename std::allocator_traits<Allocator>::template rebind_alloc<Node>;
    using NodeAllocTraits = std::allocator_traits<NodeAllocator>;

    NodeAllocator alloc_;
    Node* head_ = nullptr;
    Node* tail_ = nullptr;
    std::size_t size_ = 0;

public:
    ForwardList() = default;
    ForwardList(const ForwardList&) = delete;
    ForwardList& operator=(const ForwardList&) = delete;

    ~ForwardList() { clear(); }

    class iterator {
    public:
        using iterator_category = std::forward_iterator_tag;
        using value_type = T;
        using difference_type = std::ptrdiff_t;
        using pointer = T*;
        using reference = T&;

        explicit iterator(Node* node = nullptr) : node_(node) {}

        reference operator*() const { return node_->value; }
        pointer operator->() const { return &node_->value; }

        iterator& operator++() { node_ = node_->next; return *this; }
        iterator operator++(int) { iterator tmp = *this; ++(*this); return tmp; }

        bool operator==(const iterator& other) const { return node_ == other.node_; }
        bool operator!=(const iterator& other) const { return node_ != other.node_; }

    private:
        Node* node_;
    };

    iterator begin() { return iterator(head_); }
    iterator end() { return iterator(nullptr); }

    void push_back(const T& value) {
        Node* newNode = NodeAllocTraits::allocate(alloc_, 1);
        NodeAllocTraits::construct(alloc_, newNode, value, nullptr);

        if (tail_) tail_->next = newNode;
        else head_ = newNode;
        tail_ = newNode;
        ++size_;
    }

    std::size_t size() const noexcept { return size_; }
    bool empty() const noexcept { return size_ == 0; }

    void clear() {
        Node* cur = head_;
        while (cur) {
            Node* next = cur->next;
            NodeAllocTraits::destroy(alloc_, cur);
            NodeAllocTraits::deallocate(alloc_, cur, 1);
            cur = next;
        }
        head_ = tail_ = nullptr;
        size_ = 0;
    }
};

// ============================================================
// 3. Прикладной код
// ============================================================

std::uint64_t factorial(int n) {
    std::uint64_t result = 1;
    for (int i = 2; i <= n; ++i) result *= static_cast<std::uint64_t>(i);
    return result;
}

int main() {
    // --- 1) создание std::map со стандартным аллокатором ---
    std::map<int, int> m1;

    // --- 2) заполнение 10 элементами (ключ / факториал ключа) ---
    for (int i = 0; i < 10; ++i) {
        m1[i] = static_cast<int>(factorial(i));
    }

    // --- 3) std::map со своим аллокатором, ограниченным 10 элементами ---
    using MapValueType = std::pair<const int, int>;
    // std::map<int, int, std::less<int>, PoolAllocator<MapValueType, 10>> m2;
    std::map<int, int, PoolAllocator<MapValueType, 10>> m2;

    // --- 4) заполнение тем же способом ---
    for (int i = 0; i < 10; ++i) {
        m2[i] = static_cast<int>(factorial(i));
    }

    // --- 5) вывод обоих map ---
    std::cout << "\n=== std::map (std::allocator) ===\n";
    for (const auto& kv : m1) {
        std::cout << kv.first << " " << kv.second << "\n";
    }

    std::cout << "\n=== std::map (PoolAllocator) ===\n";
    for (const auto& kv : m2) {
        std::cout << kv.first << " " << kv.second << "\n";
    }

    // --- 6) собственный контейнер со стандартным аллокатором ---
    ForwardList<int> c1;

    // --- 7) заполнение 10 элементами от 0 до 9 ---
    for (int i = 0; i < 10; ++i) {
        c1.push_back(i);
    }

    // --- 8) собственный контейнер со своим аллокатором, ограниченным 10 ---
    ForwardList<int, PoolAllocator<int, 10>> c2;

    // --- 9) заполнение 10 элементами от 0 до 9 ---
    for (int i = 0; i < 10; ++i) {
        c2.push_back(i);
    }

    // --- 10) вывод обоих контейнеров ---
    std::cout << "\n=== ForwardList (std::allocator), size=" << c1.size() << " ===\n";
    for (int v : c1) std::cout << v << " ";
    std::cout << "\n";

    std::cout << "\n=== ForwardList (PoolAllocator), size=" << c2.size() << " ===\n";
    for (int v : c2) std::cout << v << " ";
    std::cout << "\n";

    return 0;
}
#pragma once

#include <iostream>
#include <vector>
#include <memory>
#include <cstddef>
#include <algorithm>
#include <map>

template <typename T, std::size_t ChunkSize>
class MyAlloc {
private:
    struct Chunk {
        void* memory;
        std::size_t capacity;
        std::size_t used = 0;

        explicit Chunk(std::size_t count)
            : memory(::operator new(count * sizeof(T))), capacity(count) {}

        ~Chunk() {
            ::operator delete(memory);
        }
    };

    struct Pool {
        std::vector<std::unique_ptr<Chunk>> chunks;
        std::vector<T*> freeList;
    };

    std::shared_ptr<Pool> pool_;

    template <typename U, std::size_t N>
    friend class MyAlloc;

    Chunk *findChunkWithSpace(std::size_t n) {
        for (const auto& chunkPtr : pool_->chunks) {
            Chunk *chunk = chunkPtr.get();
            if (chunk->used + n <= chunk->capacity)
                return chunk;
        }
        return nullptr;
    }

public:
    using value_type = T;

    template <typename U>
    struct rebind {
        using other = MyAlloc<U, ChunkSize>;
    };

    MyAlloc()
        : pool_(std::make_shared<Pool>()) {
        std::cout << "Created new MyAlloc with chunk size: " << ChunkSize
                  << " and element size: " << sizeof(T) << '\n';
    }

    template <typename U>
    MyAlloc(const MyAlloc<U, ChunkSize> &other) noexcept
        : pool_(other.pool_) {
        std::cout << "Created new MyAlloc with chunk size: " << ChunkSize
                  << " and element size: " << sizeof(T)
                  << "\nfrom MyAlloc with chunk size: " << ChunkSize
                  << " and element size: " << sizeof(U) << '\n';
    }

    MyAlloc(const MyAlloc&) noexcept = default;
    MyAlloc& operator=(const MyAlloc&) noexcept = default;

    T* allocate(std::size_t n) {
        if (n == 0) return nullptr;
        if (n == 1 && !(pool_->freeList.empty())) {
            T* p = pool_->freeList.back();
            pool_->freeList.pop_back();
            return p;
        }

        Chunk *chunk = findChunkWithSpace(n);
        if (!chunk) {
            std::size_t newChunkSize = std::max(ChunkSize, n);
            std::cout << "Creating new chunk with the size of: "
                      << newChunkSize << '\n';
            pool_->chunks.push_back(std::make_unique<Chunk>(newChunkSize));
            chunk = pool_->chunks.back().get();
        }

        T* result = reinterpret_cast<T*>(chunk->memory) + chunk->used;
        chunk->used += n;
        
        return result;
    }

    void deallocate(T* p, std::size_t n) {
        if (n == 1)
         pool_->freeList.push_back(p);
    }

    template <typename U, std::size_t N>
    bool operator==(const MyAlloc<U, N>& other) const noexcept {
        return pool_ == other.pool_;
    }

    template <typename U, std::size_t N>
    bool operator!=(const MyAlloc<U, N>& other) const noexcept {
        return !(*(this) == other);
    }
};

template <typename Alloc>
class AllocTester {
public:
    void testAlloc() {
        std::map<int, int, std::less<int>, Alloc> m;

        m.insert(std::make_pair(1, 1));
        m.insert(std::make_pair(2, 2));
    }
};
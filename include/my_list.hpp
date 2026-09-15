#pragma once

#include <utility>
#include <memory>

#include "my_allocator.hpp"

template <typename T, typename Allocator = std::allocator<T>>
class MyList {
private:
    struct Node {
        Node *next;
        T val;

        Node(Node* n, const T& v) 
            : next(n), val(v){
        }
    };

    using NodeAllocator = 
        typename std::allocator_traits<Allocator>::template rebind_alloc<Node>;
    using NodeAllocTraits = std::allocator_traits<NodeAllocator>;
    
    NodeAllocator  alloc_;
    std::size_t size_ = 0;
    Node *head_ = nullptr;
    Node *tail_ = nullptr;

public:
    MyList() = default;
    MyList(const MyList&) = delete;
    MyList& operator=(const MyList&) = delete;

    ~MyList() { clear(); };

    class Iterator {
    public:
        using iterator_category = std::forward_iterator_tag;
        using value_type = T;
        using difference_type = std::ptrdiff_t;
        using pointer = T*;
        using reference = T&;

        explicit Iterator(Node *node = nullptr) : node_(node) {}
        reference operator*() const { return node_->val; }
        pointer operator->() const { return &node_->val; }

        Iterator& operator++() {
            this->node_ = node_->next;
            return *this;
        }

        Iterator operator++(int) {
            Iterator it = *this;
            ++(*this);
            return it;
        }

        bool operator==(Iterator& other) { return node_ == other.node_; }
        bool operator!=(Iterator& other) { return node_ != other.node_; }

    private:
        Node *node_;
    };

    Iterator begin() { return Iterator(head_); };
    Iterator end()   { return Iterator(nullptr); };

    void push_back(const T& val) {
        Node *newNode = NodeAllocTraits::allocate(alloc_, 1);
        NodeAllocTraits::construct(alloc_, newNode, nullptr, val);
        
        if (tail_) tail_->next = newNode;
        else head_ = newNode;
        tail_ = newNode;
        
        return;
    }

    std::size_t size() const noexcept { return size_; }
    bool empty() const noexcept { return size_ == 0; }

    T& pop_back() {
        if (empty()) return T();
        T retval = tail_->val;
        Node *cur = head_;
        while (cur->next != tail_) cur = cur->next;
        cur->next = nullptr;
        NodeAllocTraits::destroy(tail_);
        NodeAllocTraits::deallocate(tail_);
        tail_ = cur;
        size_--;
        return retval;
    }

    void clear() {
        Node *current = head_;
        while (current != nullptr) {
            Node *nextNode = current->next;
            NodeAllocTraits::destroy(alloc_, current);
            NodeAllocTraits::deallocate(alloc_, current, 1);
            current = nextNode;
        }
        head_ = tail_ = nullptr;
        size_ = 0;
    }
};
#pragma once
#include <stdexcept>
#include <type_traits>

template<typename T>
class SimpleList {
private:
    T* data_{nullptr};
    size_t size_{0};
    size_t capacity_{0};

    void ensure_capacity(size_t newCap) {
        if (newCap <= capacity_) return;
        auto* newData = new T[newCap];
        for (size_t i = 0; i < size_; ++i)
            newData[i] = data_[i];
        delete[] data_;
        data_ = newData;
        capacity_ = newCap;
    }

public:
    using value_type = T;

    SimpleList() = default;
    ~SimpleList() { delete[] data_; }
    
    SimpleList(const SimpleList& other) : size_(other.size_), capacity_(other.capacity_) {
        if (capacity_ > 0) {
            data_ = new T[capacity_];
            for (size_t i = 0; i < size_; ++i)
                data_[i] = other.data_[i];
        }
    }
    
    SimpleList& operator=(const SimpleList& other) {
        if (this != &other) {
            delete[] data_;
            size_ = other.size_;
            capacity_ = other.capacity_;
            if (capacity_ > 0) {
                data_ = new T[capacity_];
                for (size_t i = 0; i < size_; ++i)
                    data_[i] = other.data_[i];
            } else {
                data_ = nullptr;
            }
        }
        return *this;
    }
    
    SimpleList(SimpleList&& other) noexcept : data_(other.data_), size_(other.size_), capacity_(other.capacity_) {
        other.data_ = nullptr;
        other.size_ = 0;
        other.capacity_ = 0;
    }
    
    SimpleList& operator=(SimpleList&& other) noexcept {
        if (this != &other) {
            delete[] data_;
            data_ = other.data_;
            size_ = other.size_;
            capacity_ = other.capacity_;
            other.data_ = nullptr;
            other.size_ = 0;
            other.capacity_ = 0;
        }
        return *this;
    }

    void push_back(const T& value) {
        if (size_ == capacity_)
            ensure_capacity(capacity_ == 0 ? 2 : capacity_ * 2);
        data_[size_++] = value;
    }

    size_t size() const { return size_; }

    T& operator[](size_t idx) {
        if (idx >= size_) throw std::out_of_range("SimpleList index out of range");
        return data_[idx];
    }

    const T& operator[](size_t idx) const {
        if (idx >= size_) throw std::out_of_range("SimpleList index out of range");
        return data_[idx];
    }

    T* begin() { return data_; }
    T* end() { return data_ + size_; }
    const T* begin() const { return data_; }
    const T* end() const { return data_ + size_; }

    void clear() { size_ = 0; }
};

template<typename Container, typename Predicate>
auto* find_if(Container& c, Predicate pred) {
    using Elem = typename std::remove_reference_t<decltype(*c.begin())>;
    for (auto& x : c)
        if (pred(x)) return &x;
    return static_cast<Elem*>(nullptr);
}

#pragma once
#include <stdexcept>
#include <type_traits>

template<typename T>
class SimpleList {
private:
    T* data_;
    size_t size_;
    size_t capacity_;

    void ensure_capacity(size_t newCap) {
        if (newCap <= capacity_) return;
        T* newData = new T[newCap];
        for (size_t i = 0; i < size_; ++i)
            newData[i] = data_[i];
        delete[] data_;
        data_ = newData;
        capacity_ = newCap;
    }

public:
    using value_type = T;

    SimpleList() : data_(nullptr), size_(0), capacity_(0) {}
    ~SimpleList() { delete[] data_; }

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

#ifndef WHALE_BASE_ARRAY_REF_H_
#define WHALE_BASE_ARRAY_REF_H_

#include <cstddef>
#include <iterator>
#include "base/logging.h"

namespace whale {


/**
 * @brief A container that references an array.
 *
 * @details The template class ArrayRef provides a container that references
 * an external array. This external array must remain alive while the ArrayRef
 * object is in use. The external array may be a std::vector<>-backed storage
 * or any other contiguous chunk of memory but that memory must remain valid,
 * i.e. the std::vector<> must not be resized for example.
 *
 * Except for copy/assign and insert/erase/capacity functions, the interface
 * is essentially the same as std::vector<>. Since we don't want to throw
 * exceptions, at() is also excluded.
 */
template<typename T>
class ArrayRef {
 public:
    using value_type = T;
    using reference = T &;
    using const_reference = const T &;
    using pointer = T *;
    using const_pointer = const T *;
    using iterator = T *;
    using const_iterator = const T *;
    using reverse_iterator = std::reverse_iterator<iterator>;
    using const_reverse_iterator = std::reverse_iterator<const_iterator>;
    using difference_type = ptrdiff_t;
    using size_type = size_t;

    // Constructors.

    constexpr ArrayRef()
            : array_(nullptr), size_(0u) {
    }

    template<size_t size>
    explicit constexpr ArrayRef(T (&array)[size])
            : array_(array), size_((size_t) size) {
    }

    template<typename U,
            size_t size,
            typename = typename std::enable_if<std::is_same<T, const U>::value>::type>
    explicit constexpr ArrayRef(U (&array)[size])
            : array_(array), size_((size_t) size) {
    }

    constexpr ArrayRef(T *array_in, size_t size_in)
            : array_(array_in), size_(size_in) {
    }

    template<typename Vector,
            typename = typename std::enable_if<
                    std::is_same<typename Vector::value_type, value_type>::value>::type>
    explicit ArrayRef(Vector &v)
            : array_(v.data()), size_(v.size()) {
    }

    template<typename Vector,
            typename = typename std::enable_if<
                    std::is_same<
                            typename std::add_const<typename Vector::value_type>::type,
                            value_type>::value>::type>
    explicit ArrayRef(const Vector &v)
            : array_(v.data()), size_(v.size()) {
    }

    ArrayRef(const ArrayRef &) = default;

    // Assignment operators.

    ArrayRef &operator=(const ArrayRef &other) {
        array_ = other.array_;
        size_ = other.size_;
        return *this;
    }

    template<typename U>
    typename std::enable_if<std::is_same<T, const U>::value, ArrayRef>::type &
    operator=(const ArrayRef<U> &other) {
        return *this = ArrayRef(other);
    }

    template<typename U>
    static ArrayRef Cast(const ArrayRef<U> &src) {
        return ArrayRef(reinterpret_cast<const T *>(src.data()),
                        src.size() * sizeof(T) / sizeof(U));
    }

    // Destructor.
    ~ArrayRef() = default;

    // Iterators.
    iterator begin() { return array_; }

    const_iterator begin() const { return array_; }

    const_iterator cbegin() const { return array_; }

    iterator end() { return array_ + size_; }

    const_iterator end() const { return array_ + size_; }

    const_iterator cend() const { return array_ + size_; }

    reverse_iterator rbegin() { return reverse_iterator(end()); }

    const_reverse_iterator rbegin() const { return const_reverse_iterator(end()); }

    const_reverse_iterator crbegin() const { return const_reverse_iterator(cend()); }

    reverse_iterator rend() { return reverse_iterator(begin()); }

    const_reverse_iterator rend() const { return const_reverse_iterator(begin()); }

    const_reverse_iterator crend() const { return const_reverse_iterator(cbegin()); }

    // Size.
    size_type size() const { return size_; }

    bool empty() const { return size() == 0u; }

    // Element access. NOTE: Not providing at().

    reference operator[](size_type n) {
        return array_[n];
    }

    const_reference operator[](size_type n) const {
        return array_[n];
    }

    reference front() {
        return array_[0];
    }

    const_reference front() const {
        return array_[0];
    }

    reference back() {
        return array_[size_ - 1u];
    }

    const_reference back() const {
        return array_[size_ - 1u];
    }

    value_type *data() { return array_; }

    const value_type *data() const { return array_; }

    ArrayRef SubArray(size_type pos) {
        return SubArray(pos, size() - pos);
    }

    ArrayRef<const T> SubArray(size_type pos) const {
        return SubArray(pos, size() - pos);
    }

    ArrayRef SubArray(size_type pos, size_type length) {
        return ArrayRef(data() + pos, length);
    }

    ArrayRef<const T> SubArray(size_type pos, size_type length) const {
        return ArrayRef<const T>(data() + pos, length);
    }

 private:
    T *array_;
    size_t size_;
};

template<typename T>
bool operator==(const ArrayRef<T> &lhs, const ArrayRef<T> &rhs) {
    return lhs.size() == rhs.size() && std::equal(lhs.begin(), lhs.end(), rhs.begin());
}

template<typename T>
bool operator!=(const ArrayRef<T> &lhs, const ArrayRef<T> &rhs) {
    return !(lhs == rhs);
}

}  // namespace whale

#endif  // WHALE_BASE_ARRAY_REF_H_

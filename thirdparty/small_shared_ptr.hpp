#ifndef small_shared_ptr_hpp
#define small_shared_ptr_hpp

#include <utility>
#include <atomic>
#include <stdlib.h>
#include <new>

template <class T>
struct small_shared_ptr {
private:
    static const uint64_t strong_incr = (1ull << 32) + 1ull;

    struct buffer {
        union {
            std::atomic<uint64_t> state;
            struct {
                std::atomic<uint32_t> strong;
                std::atomic<uint32_t> weak;
            } b;
        };
        T value;
        template<typename... Args>
        buffer(Args&&... args) : state(strong_incr), value(std::forward<Args>(args)...) {
        }
        virtual void cleanup() {
            (&value)->~T();
        }
    };
    T* ptr;
    template<typename... Args>
    small_shared_ptr(buffer* buf) noexcept : ptr(&buf->value) {
    }

    buffer* get_buffer() const {
        return (buffer*)((char*)ptr - (size_t)&(reinterpret_cast<buffer*>(0)->value));
    }

public:
    small_shared_ptr() noexcept : ptr(nullptr) {
    }
    small_shared_ptr(std::nullptr_t null) noexcept : ptr(nullptr) {
    }
    small_shared_ptr(small_shared_ptr<T>&& other) noexcept : ptr(other.ptr) {
        other.ptr = nullptr;
    }
    small_shared_ptr(const small_shared_ptr<T>& other) noexcept : ptr(other.ptr) {
        if (ptr) {
            std::atomic_fetch_add_explicit(&get_buffer()->state, strong_incr, std::memory_order_relaxed);
        }
    }
    template <class U>
    small_shared_ptr(small_shared_ptr<U>&& other) noexcept : ptr(other.ptr) {
        other.ptr = nullptr;
    }
    template <class U>
    small_shared_ptr(const small_shared_ptr<U>& other) noexcept : ptr(other.ptr) {
        if (ptr) {
            std::atomic_fetch_add_explicit(&get_buffer()->state, strong_incr, std::memory_order_relaxed);
        }
    }
    ~small_shared_ptr() {
        *this = nullptr;
    }
    small_shared_ptr<T>& operator=(const small_shared_ptr<T>& other) {
        *this = nullptr;
        ptr = other.ptr;
        if (ptr) {
            std::atomic_fetch_add_explicit(&get_buffer()->state, strong_incr, std::memory_order_relaxed);
        }
        return *this;
    }
    small_shared_ptr<T>& operator=(small_shared_ptr<T>&& other) {
        *this = nullptr;
        ptr = other.ptr;
        other.ptr = nullptr;
        return *this;
    }
    small_shared_ptr<T>& operator=(std::nullptr_t null) {
        if (ptr != nullptr) {
            auto ref = get_buffer();
            ptr = nullptr;
            auto new_state = std::atomic_fetch_sub_explicit(&ref->state, strong_incr, std::memory_order_acq_rel);
            if ((new_state >> 32) == 0) {
                try {
                    ref->cleanup();
                } catch (...) {
                    if (new_state == 0) {
                        free(ref);
                    }
                    throw;
                }
            }
            if (new_state == 0) {
                free(ref);
            }
        }
        return *this;
    }
    T& operator*() const noexcept {
        return *get();
    }
    T* operator->() const noexcept {
        return get();
    }
    T* get() const noexcept {
        return static_cast<T*>(ptr);
    }
    long use_count() const noexcept {
        return ptr ? get_buffer()->b.strong : 0;
    }
    explicit operator bool() const noexcept {
        return ptr != nullptr;
    }
    template<typename U, typename... Args>
    friend small_shared_ptr<U> make_small_shared(Args&&... args);
};

template<class T, class U> inline bool operator==(const small_shared_ptr<T>& l, const small_shared_ptr<U>& r) noexcept {
    return l.get() == r.get();
}
template<class T, class U> inline bool operator!=(const small_shared_ptr<T>& l, const small_shared_ptr<U>& r) noexcept {
    return l.get() != r.get();
}
template<class T, class U> inline bool operator<=(const small_shared_ptr<T>& l, const small_shared_ptr<U>& r) noexcept {
    return l.get() <= r.get();
}
template<class T, class U> inline bool operator<(const small_shared_ptr<T>& l, const small_shared_ptr<U>& r) noexcept {
    return l.get() < r.get();
}
template<class T, class U> inline bool operator>=(const small_shared_ptr<T>& l, const small_shared_ptr<U>& r) noexcept {
    return l.get() >= r.get();
}
template<class T, class U> inline bool operator>(const small_shared_ptr<T>& l, const small_shared_ptr<U>& r) noexcept {
    return l.get() > r.get();
}

template<typename T, typename... Args>
small_shared_ptr<T> make_small_shared(Args&&... args) {
    typename small_shared_ptr<T>::buffer* result = static_cast<typename small_shared_ptr<T>::buffer*>(malloc(sizeof(typename small_shared_ptr<T>::buffer)));
    if (!result) {
        throw std::bad_alloc();
    }
    try {
        new (result) typename small_shared_ptr<T>::buffer(std::forward<Args>(args)...);
    } catch(...) {
        free(result);
        throw;
    }
    return small_shared_ptr<T>(result);
}

#endif
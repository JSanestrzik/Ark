#ifndef function_view_hpp
#define function_view_hpp

#include <type_traits>
#include <functional>

template <typename TReturn, typename TA1, typename TA2>
class function_view final
{
private:
    using signature_type = TReturn(TA1, TA2);

    void* _ptr;
    TReturn (*_erased_fn)(void*, TA1, TA2);

public:
    function_view(signature_type x) noexcept :
        _ptr(static_cast<void*>(x))
    {
        _erased_fn = [](void* ptr, TA1 xa, TA2 xb) -> TReturn {
            return (*reinterpret_cast<std::add_pointer_t<TReturn>>(ptr))(std::forward<TA1>(xa), std::forward<TA2>(xb));
        };
    }

    function_view(const function_view<TReturn, TA1, TA2>& fv) noexcept :
        _ptr(fv._ptr), _erased_fn(fv._erased_fn)
    {}

    function_view(function_view<TReturn, TA1, TA2>&& fv) noexcept :
        _ptr(fv._ptr), _erased_fn(fv._erased_fn)
    {}

    function_view<TReturn, TA1, TA2>& operator=(const function_view<TReturn, TA1, TA2>& fv) noexcept
    {
        _ptr = fv._ptr;
        _erased_fn = fv._erased_fn;
        return *this;
    }

    decltype(auto) operator()(TA1 xa, TA2 xb) const noexcept(noexcept(_erased_fn(_ptr, std::forward<TA1>(xa), std::forward<TA2>(xb))))
    {
        return _erased_fn(_ptr, std::forward<TA1>(xa), std::forward<TA2>(xb));
    }
};

#endif
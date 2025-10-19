#pragma once

namespace derivecpp {
template <typename T> struct member_pointer_class;

template <typename C, typename M> struct member_pointer_class<M C::*> {
    using type = C;
};

template <typename T> using member_pointer_class_t = typename member_pointer_class<T>::type;
} // namespace derivecpp

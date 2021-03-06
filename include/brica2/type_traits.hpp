#ifndef __BRICA2_TYPE_TRAITS_H__
#define __BRICA2_TYPE_TRAITS_H__

namespace brica2 {

template <class Q, std::size_t I, class T, class... Ts>
constexpr std::enable_if_t<std::is_same<Q, T>::value, std::size_t> find_type() {
  return I;
}

template <class Q, std::size_t I, class T, class... Ts>
constexpr std::enable_if_t<!std::is_same<Q, T>::value, std::size_t>
find_type() {
  static_assert(sizeof...(Ts) != 0, "type not found");
  return find_type<Q, I + 1, Ts...>();
}

template <class Q, class... Ts> constexpr std::size_t find_type() {
  static_assert(sizeof...(Ts) != 0, "type not found");
  return find_type<Q, 0, Ts...>();
}

}  // namespace brica2

#endif  // __BRICA2_TYPE_TRAITS_H__

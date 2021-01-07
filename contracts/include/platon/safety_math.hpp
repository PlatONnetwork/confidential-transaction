#pragma once

#include <limits>
#include <type_traits>
namespace platon {
template <typename T, class = typename std::enable_if<
                          !std::numeric_limits<std::decay_t<T>>::is_signed>>
std::pair<T, bool> SafeSub(T x, T y) {
  return std::make_pair(x - y, x < y);
}

// SafeAdd returns the result and whether overflow occurred.
template <typename T, class = typename std::enable_if<
                          !std::numeric_limits<std::decay_t<T>>::is_signed>>
std::pair<T, bool> SafeAdd(T x, T y) {
  return std::make_pair(x + y, y > std::numeric_limits<T>::max() - x);
}

// SafeMul returns multiplication result and whether overflow occurred.
template <typename T, class = typename std::enable_if<
                          !std::numeric_limits<std::decay_t<T>>::is_signed>>
std::pair<T, bool> SafeMul(T x, T y) {
  if (x == 0 || y == 0) {
    return std::make_pair(0, false);
  }
  DEBUG("x:", x, " y:", y, " max:", std::numeric_limits<T>::max())
  return std::make_pair(x * y, y > std::numeric_limits<T>::max() / x);
}
}  // namespace platon
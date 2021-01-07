#pragma once
#include <platon/common.h>
#include "platon/print.hpp"
namespace platon {
class VectorWrapper {
#define MUTABLE_FUNC_WRAPPER(NAME)                     \
  template <class... Args>                             \
  auto NAME(Args&&... _Args) {                         \
    if (!alloc_) {                                     \
      buffer_ = bytes_ref_.toVector();                 \
      alloc_ = true;                                   \
    }                                                  \
    return buffer_.NAME(std::forward<Args>(_Args)...); \
  }
#define IMMUTABLE_FUNC_WRAPPER(NAME)                      \
  template <class... Args>                                \
  auto NAME(Args&&... _Args) const {                      \
    if (!alloc_) {                                        \
      buffer_.NAME(std::forward<Args>(_Args)...);         \
    }                                                     \
    return bytes_ref_.NAME(std::forward<Args>(_Args)...); \
  }

 public:
  VectorWrapper() : alloc_(false) {}
  explicit VectorWrapper(const bytesConstRef& ref)
      : alloc_(false), bytes_ref_(ref) {}
  explicit VectorWrapper(const bytes& ref)
      : alloc_(false), bytes_ref_(bytesConstRef(ref.data(), ref.size())) {}
  VectorWrapper(const VectorWrapper&& v) {
    alloc_ = v.alloc_;
    buffer_ = std::move(v.buffer_);
    bytes_ref_ = std::move(v.bytes_ref_);
  }

  VectorWrapper(const VectorWrapper& v) {
    alloc_ = v.alloc_;
    buffer_ = v.buffer_;
    bytes_ref_ = v.bytes_ref_;
  }

  IMMUTABLE_FUNC_WRAPPER(data)
  IMMUTABLE_FUNC_WRAPPER(empty)
  IMMUTABLE_FUNC_WRAPPER(size)
  VectorWrapper& operator=(const VectorWrapper& v) {
    alloc_ = v.alloc_;
    buffer_ = v.buffer_;
    bytes_ref_ = v.bytes_ref_;
    return *this;
  }

  VectorWrapper& operator=(const VectorWrapper&& v) {
    alloc_ = v.alloc_;
    buffer_ = std::move(v.buffer_);
    bytes_ref_ = std::move(v.bytes_ref_);
    return *this;
  }

  VectorWrapper& operator=(const bytesConstRef& ref) {
    alloc_ = false;
    bytes_ref_ = ref;
    return *this;
  }

  bool Alloc() { return alloc_; }

  bytes ToBytes() const {
    if (alloc_) {
      return buffer_;
    } else {
      return bytes_ref_.toBytes();
    }
  }

  bytesConstRef ToBytesConstRef() const {
    if (alloc_) {
      return bytesConstRef(buffer_.data(), buffer_.size());
    } else {
      return bytes_ref_;
    }
  }

  friend RLPStream& operator<<(RLPStream& rlp, const VectorWrapper& t) {
    if (t.alloc_) {
      rlp << t.buffer_;
    } else {
      rlp << t.bytes_ref_;
    }
    return rlp;
  }

  friend void fetch(const platon::RLP& rlp, VectorWrapper& t) {
    t.bytes_ref_ = rlp.toBytesConstRef();
  }

  friend RLPSize& operator<<(RLPSize& rlp, const VectorWrapper& t) {
    if (t.alloc_) {
      rlp << t.buffer_;
    } else {
      rlp << t.bytes_ref_;
    }
    return rlp;
  }

 private:
  bool alloc_;
  bytes buffer_;
  bytesConstRef bytes_ref_;
};
}  // namespace platon
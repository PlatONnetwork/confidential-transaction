#pragma once

#include <platon/platon.h>

namespace privacy {
template <Name::Raw ProxyName, typename T>
class AdminProxy : public T {
 public:
  AdminProxy(const Address &owner, const Address &proxy) {
    privacy_assert(owner != Address(), "illegal owner address");
    privacy_assert(proxy != Address(), "illegal proxy address");
    setOwner(owner);
    SetProxy(proxy);
    this->SetContractProxy(proxy);
  }

  AdminProxy(const Address &owner) {
    setOwner(owner);
    this->SetContractProxy(GetProxy());
  }

  void UpdateProxy(const Address &owner, const Address &new_proxy) {
    privacy_assert(owner == getOwner(), "illegal owner address");
    privacy_assert(new_proxy != Address(), "illegal proxy address");
    SetProxy(new_proxy);
  }

  Address GetProxy() {
    SetProxyKey();
    Address proxy;
    platon_get_state(owner_.data(), owner_.size(), proxy.data(), proxy.size);
    return proxy;
  }

 private:
  void setOwner(const Address &owner) {
    std::copy(owner.data(), owner.data() + owner.size, owner_.data());
  }

  Address getOwner() {
    Address result;
    std::copy(owner_.data(), owner_.data() + Address::size, result.data());
    return result;
  }

  void SetProxy(const Address &proxy) {
    SetProxyKey();
    platon_set_state(owner_.data(), owner_.size(), proxy.data(), proxy.size);
  }

  void SetProxyKey() {
    std::copy((const uint8_t *)&kProxyAddrKey,
              (const uint8_t *)&kProxyAddrKey + sizeof(kProxyAddrKey),
              owner_.data() + Address::size);
  }

 private:
  std::array<byte, 36> owner_{};
  const uint64_t kProxyAddrKey[2] = {uint64_t(ProxyName),
                                     uint64_t(Name::Raw("proxy"_n))};
};
}  // namespace privacy

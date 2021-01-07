#include <platon/platon.h>
#include "privacy/common.hpp"
#include "privacy/debug/gas/stack_helper.h"
#include "token/arc20_proxy.hpp"

class TokenManager : public Contract {
 public:
  /**
   * @brief TokenManager contract initialization
   *
   * @param owner Administrator address, a value of 0 means that no
   * administrator is set.
   */
  ACTION void init(const platon::Address &owner) {
    if (owner != Address()) {
      platon_set_state((const byte *)&kOwnerKey, sizeof(kOwnerKey),
                       owner.data(), owner.size);
      DEBUG("owner address:", owner.toString());
    }
  }

 public:
  /**
   * @brief Set the owner of the token manager (only the owner has the authority
   * to set a new owner)
   *
   * @param owner owner address
   * @return Setting success or failure, failure triggers revert operation.
   */
  ACTION bool SetOwner(const platon::Address &owner) {
    Address old_owner_address;
    platon_get_state((const byte *)&kOwnerKey, sizeof(kOwnerKey),
                     old_owner_address.data(), old_owner_address.size);
    DEBUG("old owner address:", old_owner_address.toString(),
          "new owner address:", owner.toString());
    privacy_assert(old_owner_address == platon_caller(), "no permission");
    platon_set_state((const byte *)&kOwnerKey, sizeof(kOwnerKey), owner.data(),
                     owner.size);

    return true;
  }

  /**
   * @brief Get the owner of the token manager
   *
   * @return owner of token manager
   */
  CONST platon::Address GetOwner() {
    Address owner_address;
    platon_get_state((const byte *)&kOwnerKey, sizeof(kOwnerKey),
                     owner_address.data(), owner_address.size);
    return owner_address;
  }

  /**
   * @brief Recharge, recharge arc20 tokens to the privacy token contract (only
   * the owner has permission to call this interface)
   *
   * @param token_address token contract address
   * @param public_owner Transfer account address, transfer token from this
   * account to tokenManager contract
   * @param value Transfer amount after scaling factor processing
   * @return Failure to trigger revert operation
   */
  ACTION bool Deposit(const platon::Address &token_address,
                      const platon::Address &public_owner, platon::u128 value) {
    privacy_assert(GetOwner() == platon_caller(), "no permission");
    Arc20Proxy arc20(token_address);
    DEBUG("token address:", token_address.toString(),
          "public owner:", public_owner.toString(), "deposit value", value);
    return arc20.TransferFrom(public_owner, platon_address(), value);
  }

  /**
   * @brief Withdraw money, withdraw arc20 tokens from the privacy token
   * contract to the designated account (only the owner has permission to call
   * this interface)
   *
   * @param token_address token contract address
   * @param public_owner Transfer account address, transfer token from
   * tokenManager to this address
   * @param value 经过缩放因子处理后的转账金额
   * @return Failure to trigger revert operation
   */
  ACTION bool Withdraw(const platon::Address &token_address,
                       const platon::Address &public_owner,
                       platon::u128 value) {
    privacy_assert(GetOwner() == platon_caller(), "no permission");
    Arc20Proxy arc20(token_address);
    DEBUG("token address:", token_address.toString(),
          "public owner:", public_owner.toString(), "withdraw value", value);
    return arc20.Transfer(public_owner, value);
  }

 private:
  const uint64_t kOwnerKey = uint64_t(Name::Raw("owner"_n));
};

PLATON_DISPATCH(TokenManager, (init)(SetOwner)(GetOwner)(Deposit)(Withdraw))
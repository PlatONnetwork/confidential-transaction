#pragma once
#include <platon/platon.h>
#include "platon/escape_event.hpp"

namespace privacy {
class IARC20 {
 public:
  // @return total issuance of tokens
  virtual u128 GetTotalSupply() = 0;

  // @param _owner The address from which the balance will be retrieved
  // @return The balance.
  virtual u128 BalanceOf(const Address& _owner) = 0;

  // @param _owner The address of the account owning tokens
  // @param _spender The address of the account able to transfer the tokens
  // @return Amount of remaining tokens allowed to spent.
  virtual u128 Allowance(const Address& _owner, const Address& _spender) = 0;

  // @notice send '_value' token to `_to` from `msg.sender`
  // @param _to THe address of the recipient.
  // @param _value The amount of token to be transferred.
  // @return Whether the transfer was successful or not.
  virtual bool Transfer(const Address& _to, u128 _value) = 0;

  // @notice send `_value` token to `_to` from `_from` on the condition it is
  // approved by `_from`
  // @param _from The address of the sender.
  // @param _to The address of the recepient.
  // @param _value The amount of token to be transferred.
  // @return Whether the transfer was successful or not.
  virtual bool TransferFrom(const Address& _from, const Address& _to,
                            u128 _value) = 0;

  // @notice `msg.sender` approves `_spender` to spend `_value` tokens
  // @param _spender The address of the account able to transfer the tokens
  // @param _value The amount of tokens to be approved for transfer
  // @return Whether thee approval was successful or not.
  virtual bool Approve(const Address& _spender, u128 _value) = 0;

 protected:
  // define: _from, _to, _value
  PLATON_EVENT2(TransferEvent, const Address&, const Address&, u128);
  // define: _owner, _spender, _value
  PLATON_EVENT2(ApprovalEvent, const Address&, const Address&, u128);
};
}  // namespace privacy
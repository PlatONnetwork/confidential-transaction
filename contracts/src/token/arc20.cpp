
#include <string>
#include "privacy/common.hpp"
#include "privacy/debug/gas/stack_helper.h"
#include "token/iarc20.h"

CONTRACT ARC20 : public IARC20, public Contract {
 public:
  ACTION void init(const std::string &token_name,
                   const std::string &token_symbol, u128 initial_amount,
                   uint8_t decimal_units) {
    privacy_assert(token_name.length() != 0,
                   "PlatON ARC20: token name is empty");
    privacy_assert(token_symbol.length() != 0,
                   "PlatON ARC20: token symbol is empty");
    privacy_assert(initial_amount > 0, "PlatON ARC20: amount is invalid");
    privacy_assert(decimal_units > 0, "PlatON ARC20: decimal is invalid");

    Address sender = platon_caller();
    SetOwner(sender);
    SetName(token_name);
    SetSymbol(token_symbol);
    SetBalance(sender, initial_amount);
    SetTotalSupply(initial_amount);
    SetDecimals(decimal_units);  // Amount of decimals for display purposes

    DEBUG("init", "name:", token_name, "Symbol:", token_symbol,
          "total:", initial_amount, "_Decimal:", decimal_units,
          "owner:", sender.toString());
  }

  CONST std::string GetName() {
    size_t len =
        ::platon_get_state_length((const uint8_t *)&kNameKey, sizeof(kNameKey));
    privacy_assert(len != 0, "PlatON ARC20: get token name failed");
    std::string name;
    name.resize(len);
    ::platon_get_state((const uint8_t *)&kNameKey, sizeof(kNameKey),
                       (byte *)name.data(), name.length());
    DEBUG("name:", name);
    return name;
  }

  CONST std::string GetSymbol() {
    size_t len =
        ::platon_get_state_length((const uint8_t *)&kSymbol, sizeof(kSymbol));
    privacy_assert(len != 0, "PlatON ARC20: get token symbol failed");

    std::string symbol;
    symbol.resize(len);
    ::platon_get_state((const uint8_t *)&kSymbol, sizeof(kSymbol),
                       (byte *)symbol.data(), symbol.length());
    DEBUG("symbol:", symbol);
    return symbol;
  }

  CONST u128 GetTotalSupply() override { return TotalSupply(); }

  CONST uint8_t GetDecimals() {
    uint8_t decimals;
    ::platon_get_state((const uint8_t *)&kDecimals, sizeof(kDecimals),
                       (byte *)&decimals, sizeof(decimals));
    DEBUG("decimals:", decimals);
    return decimals;
  }

  CONST u128 BalanceOf(const Address &owner) override {
    return GetBalance(owner);
  }

  CONST u128 Allowance(const Address &owner, const Address &spender) override {
    FixedHash<40> combine_addr;
    ConcatAddress(owner, spender, combine_addr);

    u128 balance;
    platon_get_state(combine_addr.data(), combine_addr.size, (byte *)&balance,
                     sizeof(balance));

    DEBUG("allowance", "owner:", owner.toString(),
          "spender:", spender.toString(), "balance:", balance);
    return balance;
  }

 public:
  ACTION bool Transfer(const Address &to, u128 value) override {
    // Default assumes totalSupply can't be over max(2^64 - 1)
    // If your token leaves out totalSupply and can issue more tokens as time
    // goes on, you need to check if it doesn't wrap. Replace the if with this
    // on instead.
    Address sender = platon_caller();
    u128 sender_balance = GetBalance(sender);
    u128 to_balance = GetBalance(to);
    privacy_assert(sender_balance >= value && value > 0,
                   "PlatON ARC20: transfer amount exceeds balance");
    SetBalance(sender, sender_balance - value);
    SetBalance(to, to_balance + value);
    PLATON_EMIT_EVENT2(TransferEvent, sender, to, value);
    DEBUG("transfer", "sender:", sender.toString(), "to:", to.toString(),
          "value:", value);
    return true;
  }

  ACTION bool TransferFrom(const Address &from, const Address &to, u128 value)
      override {
    // same as above. Replace this line with the following if you want to
    // protect against wrapping uints.
    Address sender = platon_caller();
    u128 from_balance = GetBalance(from);
    u128 to_balance = GetBalance(to);
    u128 from_sender_allowance = GetAllowance(from, sender);
    privacy_assert(
        from_balance >= value && from_sender_allowance >= value && value > 0,
        "PlatON ARC20: transfer amount exceeds balance");
    SetBalance(to, to_balance + value);
    SetBalance(from, from_balance - value);
    SetAllowance(from, sender, from_sender_allowance - value);

    PLATON_EMIT_EVENT2(TransferEvent, from, to, value);
    DEBUG("transfer from", "sender:", sender.toString(),
          "from:", from.toString(), "to:", to.toString(), "value:", value);
    return true;
  }

  ACTION bool Approve(const Address &spender, u128 value) override {
    Address sender = platon_caller();
    privacy_assert(value > 0, "PlatON ARC20: approve amount illegal");
    SetAllowance(sender, spender, value);
    PLATON_EMIT_EVENT2(ApprovalEvent, sender, spender, value);
    DEBUG("approve", "sender:", sender.toString(),
          "spender:", spender.toString(), "value:", value);
    return true;
  }

  ACTION bool IncreaseApprove(const Address &spender, u128 value) {
    Address sender = platon_caller();
    u128 old_val = GetAllowance(sender, spender);
    privacy_assert(
        value > 0,
        "PlatON ARC20: can't increase approval, approve amount illegal");

    u128 new_val = old_val + value;
    SetAllowance(sender, spender, new_val);

    PLATON_EMIT_EVENT2(ApprovalEvent, sender, spender, new_val);
    DEBUG("increase approve", "sender:", sender.toString(),
          "spender:", spender.toString(), "oldVal:", old_val, "value:", value,
          "new_val:", new_val);
    return true;
  }

  ACTION bool DecreaseApprove(const Address &spender, u128 value) {
    Address sender = platon_caller();
    u128 old_val = GetAllowance(sender, spender);
    privacy_assert(old_val > 0 && old_val >= value,
                   "PlatON ARC20: can't decrease approval");

    u128 new_val = old_val - value;
    SetAllowance(sender, spender, new_val);

    PLATON_EMIT_EVENT2(ApprovalEvent, sender, spender, new_val);
    DEBUG("decrease approve", "sender:", sender.toString(),
          "spender:", spender.toString(), "oldVal:", old_val, "value:", value,
          "new_val:", new_val);
    return true;
  }

  ACTION bool Mint(const Address &account, u128 value) {
    Address sender = platon_caller();
    privacy_assert(GetOwner() == sender,
                   "PlatON ARC20: only owner can do mint");
    privacy_assert(account != Address(0),
                   "PlatON ARC20: mint to the zero address");
    privacy_assert(value > 0, "PlatON ARC20: mint value illegal");

    u128 total_supply = TotalSupply();
    SetTotalSupply(total_supply + value);
    SetBalance(account, GetBalance(account) + value);
    PLATON_EMIT_EVENT1(MintEvent, account, value);
    DEBUG("mint", "sender:", sender.toString(), "account:", account.toString(),
          "value:", value);
    return true;
  }

  ACTION bool Burn(const Address &account, u128 value) {
    DEBUG("burn", "sender:", platon_caller().toString(),
          "account:", account.toString(), "value:", value);
    privacy_assert(GetOwner() == platon_caller(),
                   "PlatON ARC20: only owner can do burn");
    privacy_assert(account != Address(0),
                   "PlatON ARC20: burn from the zero address");
    u128 account_balance = GetBalance(account);
    privacy_assert(account_balance >= value,
                   "PlatON ARC20: transfer amount exceeds balance");
    SetTotalSupply(TotalSupply() - value);
    SetBalance(account, account_balance - value);
    PLATON_EMIT_EVENT1(BurnEvent, account, value);
    return true;
  }

  //  ACTION bool Suicide(Address receiver) {
  //    privacy_assert(GetOwner() == platon_caller(), "PlatON ARC20: Only owner
  //    can do suicide");
  //
  //    if (sender == GetOwner()) {
  //      platon_destroy(receiver);
  //      DEBUG("suicide", "receiver:", receiver.toString());
  //      return true;
  //    }
  //    return false;
  //  }

 protected:
  // define:  _to, _value
  PLATON_EVENT1(MintEvent, const Address &, u128);
  // define: _from, _value
  PLATON_EVENT1(BurnEvent, const Address &, u128);

 private:
  void SetName(const std::string &name) {
    ::platon_set_state((const uint8_t *)&kNameKey, sizeof(kNameKey),
                       (const byte *)name.data(), name.length());
  }

  void SetSymbol(const std::string &symbol) {
    ::platon_set_state((const uint8_t *)&kSymbol, sizeof(kSymbol),
                       (const byte *)symbol.data(), symbol.length());
  }

  void SetTotalSupply(u128 total_supply) {
    ::platon_set_state((const uint8_t *)&kTotalSupply, sizeof(kTotalSupply),
                       (const byte *)&total_supply, sizeof(total_supply));
  }

  void SetDecimals(uint8_t & decimals) {
    ::platon_set_state((const uint8_t *)&kDecimals, sizeof(kDecimals),
                       (const byte *)&decimals, sizeof(decimals));
  }

  void SetOwner(const Address &owner) {
    ::platon_set_state((const uint8_t *)&kOwner, sizeof(kOwner),
                       (const byte *)owner.data(), owner.size);
  }

  Address GetOwner() {
    Address addr;
    ::platon_get_state((const uint8_t *)&kOwner, sizeof(kOwner), addr.data(),
                       addr.size);
    return addr;
  }

  void ConcatAddress(const Address &a1, const Address &a2, FixedHash<40> &res) {
    memcpy(res.data(), a1.data(), a1.size);
    memcpy(res.data() + a2.size, a2.data(), a2.size);
  }

  u128 GetBalance(const Address &owner) {
    u128 balance = 0;
    platon_get_state(owner.data(), owner.size, (byte *)&balance,
                     sizeof(balance));

    DEBUG("balance", "owner:", owner.toString(), "balance:", balance);
    return balance;
  }

  u128 SetBalance(const Address &owner, u128 balance) {
    platon_set_state(owner.data(), owner.size, (const byte *)&balance,
                     sizeof(balance));

    DEBUG("set balance", "owner:", owner.toString(), "balance:", balance);
    return balance;
  }

  u128 GetAllowance(const Address &owner, const Address &spender) {
    FixedHash<40> combine_addr;
    ConcatAddress(owner, spender, combine_addr);

    u128 balance = 0;
    platon_get_state(combine_addr.data(), combine_addr.size, (byte *)&balance,
                     sizeof(balance));

    DEBUG("allowance", "owner:", owner.toString(),
          "spender:", spender.toString(), "balance:", balance);
    return balance;
  }

  void SetAllowance(const Address &owner, const Address &spender, u128 value) {
    FixedHash<40> combine_addr;
    ConcatAddress(owner, spender, combine_addr);

    platon_set_state(combine_addr.data(), combine_addr.size,
                     (const byte *)&value, sizeof(value));

    DEBUG("set allowance", "owner:", owner.toString(),
          "spender:", spender.toString(), "balance:", value);
  }

  u128 TotalSupply() {
    size_t len = ::platon_get_state_length((const uint8_t *)&kTotalSupply,
                                           sizeof(kTotalSupply));
    privacy_assert(len != 0, "total supply does not exist");
    u128 supply;
    ::platon_get_state((const uint8_t *)&kTotalSupply, sizeof(kTotalSupply),
                       (byte *)&supply, sizeof(supply));

    DEBUG("total supply:", supply);
    return supply;
  }

 private:
  const uint64_t kNameKey = uint64_t(Name::Raw("name"_n));
  const uint64_t kSymbol = uint64_t(Name::Raw("symbol"_n));
  const uint64_t kTotalSupply = uint64_t(Name::Raw("total_supply"_n));
  const uint64_t kDecimals = uint64_t(Name::Raw("decimals"_n));
  const uint64_t kOwner = uint64_t(Name::Raw("owner"_n));

 private:
};

PLATON_DISPATCH(ARC20,
                (init)(GetName)(GetSymbol)(GetTotalSupply)(GetDecimals)(
                    BalanceOf)(Allowance)(Transfer)(TransferFrom)(Approve)(
                    IncreaseApprove)(DecreaseApprove)(Mint)(Burn))

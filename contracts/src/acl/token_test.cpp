
// #include "token/privacy_token.hpp"
// #include "privacy/validator_interface.hpp"

#include <platon/platon.hpp>
using namespace platon;
struct XX {
  platon::Address public_owner;

  PLATON_SERIALIZE(XX, (public_owner))
};
CONTRACT PrivacyToken : public platon::Contract {
 public:
  ACTION void init(const std::string &name, const std::string &symbol,
                   const platon::Address &acl, uint32_t validator_version,
                   uint32_t storage_version, u128 scaling_factor,
                   const platon::Address &token_address) {
    XX xx;
    fetch(platon::RLP(), xx);
    // privacy::BasePrivacyToken::init(name, symbol, acl, validator_version,
    // storage_version, scaling_factor, token_address);
  }
};

PLATON_DISPATCH(PrivacyToken, (init))
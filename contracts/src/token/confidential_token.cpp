
#include "token/confidential_token.hpp"

CONTRACT ConfidentialToken : public privacy::MintBurnConfidentialToken,
                          public Contract {
 public:
  ACTION void init(const std::string &name, const std::string &symbol,
                   const Address &registry_address, uint32_t validator_version,
                   uint32_t storage_version, u128 scaling_factor,
                   const Address &token_address) {
    privacy::BaseConfidentialToken::init(name, symbol, registry_address,
                                         validator_version, storage_version,
                                         scaling_factor, token_address);
  }
};

PLATON_DISPATCH(ConfidentialToken,
                (init)(Transfer)(Approve)(GetApproval)(GetAcl)(Name)(Symbol)(
                    ScalingFactor)(TotalSupply)(UpdateMetaData)(Mint)(Burn)(
                    UpdateValidator)(UpdateStorage)(SupportProof))
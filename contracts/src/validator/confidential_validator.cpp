#include <platon/platon.h>
#include <privacy/confidential_utxo.h>
#include <privacy/validator_interface.hpp>
#include "platon/call.hpp"
#include "privacy/debug/gas/stack_helper.h"
#include "platon/escape_event.hpp"

CONTRACT ConfidentialValidator : public ValidatorInterface, public Contract {
 public:
  // note hash, note owner, note cipher value, note meta data
  PLATON_EVENT1(CreateNoteDetailEvent, const h256 &, const bytesConstRef &,
                const bytesConstRef &, const bytesConstRef &)
 public:
  ACTION void init() {}

  /**
   * @brief Verify that the proof is legal
   *
   * @param proof Proof byte stream
   * @return Return a serialized byte stream of authentication information
   */
  virtual CONST bytesConstRef ValidateProof(const bytesConstRef &proof) override {
    // fetch confidential proof
    ConfidentialProof confidential_proof;
    fetch(RLP(proof), confidential_proof);

    ConfidentialData confidential_data;
    ConfidentialUTXO utxo;
    privacy_assert(PreCheck(confidential_proof, confidential_data, utxo) == 0,
                   "precheck failed");

    switch (ProofType(utxo.tx_type)) {
      case ProofType::kTransfer:
      case ProofType::kDeposit:
      case ProofType::kWithdraw:
        return ValidateTransfer(utxo, confidential_data.confidential_tx,
                                confidential_data.extra_data);
      case ProofType::kMint:
        return ValidateMint(utxo, confidential_data.extra_data,
                            confidential_proof.data);
      case ProofType::kBurn:
        return ValidateBurn(utxo, confidential_data.extra_data,
                            confidential_proof.data);
      case ProofType::kApprove:
        return ValidateApprove(utxo, confidential_data.extra_data);
      default:
        privacy_assert(false, "unknow proof type");
    }

    return bytesConstRef();
  }

  /**
   * @brief Verify the legality of the signature
   *
   * @param note_sender note sender's address
   * @param hash The hash value of the signature data
   * @param signature Signature information
   * @return Return true on success, false on failure
   */
  CONST bool ValidateSignature(const bytes &note_sender, const h256 &hash,
                               const bytesConstRef &signature) override {
    Address result;
    platon_ecrecover(hash.data(), signature.data(), signature.size(),
                     result.data());
    DEBUG("result", result.toString(), "sender",
          Address(note_sender).toString(), "hash", hash.toString(), "signature",
          toHex(signature));
    return result == Address(note_sender.data(), note_sender.size());
  }

  /**
   * @brief Determine whether the plug-in supports the specified version
   * certificate
   *
   * @param version version number
   * @return Support return true, fail return false
   */
  CONST bool SupportProof(uint32_t version) override {
    return MatchVersion(version);
  }

  /**
   * @brief Migration upgrade
   *
   * @param address Template contract address
   * @return Contract address after upgrade
   */
  ACTION Address Migrate(const Address &address) override {
    DEBUG("migrate address", address.toString());

    bytesConstRef init_rlp = escape::cross_call_args_ref("init");
    bytes value_bytes = value_to_bytes(u128(0));
    bytes gas_bytes = value_to_bytes(::platon_gas());

    Address new_address;
    bool success = ::platon_clone_migrate(
                       address.data(), new_address.data(), init_rlp.data(),
                       init_rlp.size(), value_bytes.data(), value_bytes.size(),
                       gas_bytes.data(), gas_bytes.size()) == 0;
    privacy_assert(success, "migrate failed");
    PLATON_EMIT_EVENT2(ValidatorMigrateEvent, platon_address(), new_address);
    return new_address;
  }

 private:
  void MakeInputNote(const ConfidentialInputNote &one_note, InputNote &input) {
    input.owner = rlp_encode(one_note.ephemeral_pk, one_note.sign_pk);

    ::platon_sha3(one_note.note_id.data(), one_note.note_id.size(),
                  input.hash.data(), input.hash.size);
  }

  OutputNote MakeOutputNote(const ConfidentialInputNote &one_note,
                            const platon::VectorWrapper &meta_data) {
    OutputNote output_note;
    MakeInputNote(one_note, output_note);
    output_note.meta_data = meta_data;
    return output_note;
  }

  bytesConstRef ValidateTransfer(const ConfidentialUTXO &utxo,
                                 const platon::VectorWrapper &confidential_tx,
                                 const platon::VectorWrapper &extra_data) {
    TransferExtra transfer_extra_data;
    fetch(RLP(extra_data.ToBytesConstRef()), transfer_extra_data);

    InputNotes inputs;
    for (const ConfidentialInputNote &one_note : utxo.inputs) {
      InputNote one_input;
      MakeInputNote(one_note, one_input);
      inputs.emplace_back(std::move(one_input));
    }

    OutputNotes outputs;
    const std::vector<platon::VectorWrapper> &vect_meta_data =
        transfer_extra_data.meta_data;
    auto iter_meta = vect_meta_data.cbegin();
    for (const auto &one : utxo.outputs) {
      platon::VectorWrapper meta_data;
      if (iter_meta != vect_meta_data.cend()) {
        meta_data = *iter_meta++;
      }

      OutputNote one_output = MakeOutputNote(one, meta_data);
      PLATON_EMIT_EVENT1(CreateNoteDetailEvent, one_output.hash,
                         one_output.owner.ToBytesConstRef(),
                         one.cipher_value.ToBytesConstRef(),
                         one_output.meta_data.ToBytesConstRef());
      outputs.emplace_back(std::move(one_output));
    }

    // check withdraw target address
    if (ProofType(utxo.tx_type) == ProofType::kWithdraw) {
      privacy_assert(transfer_extra_data.public_owner != Address(),
                     "invalid withdraw target address");
    }

    // check deposit signature
    if (ProofType(utxo.tx_type) == ProofType::kDeposit) {
      h256 hash;
      ::platon_sha3(confidential_tx.data(), confidential_tx.size(), hash.data(),
                    hash.size);
      Address result;
      platon_ecrecover(
          hash.data(), transfer_extra_data.deposit_signature.data(),
          transfer_extra_data.deposit_signature.size(), result.data());
      privacy_assert(transfer_extra_data.public_owner == result,
                     "invalid deposit signature");
    }
    DEBUG("deposit signature success");

    i128 public_value;
    switch (ProofType(utxo.tx_type)) {
      case ProofType::kTransfer:
        public_value = 0;
        break;
      case ProofType::kDeposit:
        public_value = -utxo.public_value;
        break;
      case ProofType::kWithdraw:
        public_value = utxo.public_value;
        break;
      default:
        privacy_assert(false, "unknow proof type");
    }

    DEBUG("validate transfer success.", "tx_type:", utxo.tx_type,
          "public_value:", public_value);

    return SerializeResultRef(
        TransferResult{std::move(inputs), std::move(outputs),
                       transfer_extra_data.public_owner, public_value,
                       bytesConstRef(utxo.authorized_address.data(),
                                     utxo.authorized_address.size)});
  }

  bytesConstRef ValidateApprove(const ConfidentialUTXO &utxo,
                                const platon::VectorWrapper &extra_data) {
    privacy_assert(false, "unknow proof type");

    return bytesConstRef();
  }

  bytesConstRef ValidateMint(const ConfidentialUTXO &utxo,
                             const platon::VectorWrapper &extra_data,
                             const platon::VectorWrapper &confidential_data) {
    MintExtra mint_extra_data;
    fetch(RLP(extra_data.ToBytesConstRef()), mint_extra_data);

    OutputNotes outputs;
    const std::vector<platon::VectorWrapper> &vect_meta_data =
        mint_extra_data.meta_data;
    auto iter_meta = mint_extra_data.meta_data.cbegin();
    for (const auto &one : utxo.outputs) {
      platon::VectorWrapper meta_data;
      if (iter_meta != vect_meta_data.cend()) {
        meta_data = *iter_meta++;
      }

      OutputNote one_output = MakeOutputNote(one, meta_data);
      PLATON_EMIT_EVENT1(CreateNoteDetailEvent, one_output.hash,
                         one_output.owner.ToBytesConstRef(),
                         one.cipher_value.ToBytesConstRef(),
                         one_output.meta_data.ToBytesConstRef());
      outputs.emplace_back(std::move(one_output));
    }

    u128 total_mint = utxo.public_value;

    DEBUG("validate mint success");

    h256 sha3_data;
    ::platon_sha3(confidential_data.data(), confidential_data.size(),
                  sha3_data.data(), sha3_data.size);

    return SerializeResultRef(
        MintResult{mint_extra_data.old_mint_hash,
                   platon_sha3(confidential_data), total_mint, outputs,
                   bytesConstRef(utxo.authorized_address.data(),
                                 utxo.authorized_address.size)});
  }

  CONST bytesConstRef ValidateBurn(
      const ConfidentialUTXO &utxo, const platon::VectorWrapper &extra_data,
      const platon::VectorWrapper &confidential_data) {
    BurnExtra burn_extra_data;
    fetch(RLP(extra_data.ToBytesConstRef()), burn_extra_data);

    InputNotes inputs;
    for (const ConfidentialInputNote &one_note : utxo.inputs) {
      InputNote one_input;
      MakeInputNote(one_note, one_input);
      inputs.emplace_back(std::move(one_input));
    }

    u128 total_burn = utxo.public_value;

    DEBUG("validate burn success");

    h256 sha3_data;
    ::platon_sha3(confidential_data.data(), confidential_data.size(),
                  sha3_data.data(), sha3_data.size);

    return SerializeResultRef(
        BurnResult{burn_extra_data.old_burn_hash,
                   platon_sha3(confidential_data), total_burn, inputs,
                   bytesConstRef(utxo.authorized_address.data(),
                                 utxo.authorized_address.size)});
  }

  int PreCheck(const ConfidentialProof &confidential_proof,
               ConfidentialData &confidential_data, ConfidentialUTXO &utxo) {
    // check version
    fetch(RLP(confidential_proof.data.ToBytesConstRef()), confidential_data);

    if (!MatchVersion(confidential_data.version)) {
      DEBUG("check version failed");
      return -1;
    }
    DEBUG("check version success");

    // Cryptographic verification
    int32_t result_len =
        platon_confidential_tx_verify(confidential_data.confidential_tx.data(),
                                      confidential_data.confidential_tx.size());
    if (result_len <= 0) {
      DEBUG("cryptographic verification failed");
      return -1;
    }
    DEBUG("cryptographic verification success");

    // get cryptographic verification results
    platon::bytes result(result_len);
    int32_t variable_result =
        platon_variable_length_result(result.data(), result.size());
    if (result_len <= 0) {
      DEBUG("get cryptographic verification results failed");
      return -1;
    }

    fetch(RLP(result), utxo);
    DEBUG("fetch cryptographic verification results success");

    // check signature
    h256 sha3_data;
    ::platon_sha3(confidential_proof.data.data(),
                  confidential_proof.data.size(), sha3_data.data(),
                  sha3_data.size);

    Address signature_address;
    platon_ecrecover(sha3_data.data(), confidential_proof.signature.data(),
                     confidential_proof.signature.size(),
                     signature_address.data());

    if (signature_address != utxo.authorized_address) {
      DEBUG("check proof signature failed")
      return -1;
    }

    return 0;
  }

  /**
   * @brief Verify the version number
   *
   * @param version version number
   *
   * @return Return true on success, false on failure
   */
  bool MatchVersion(uint32_t version) {
    auto major = static_cast<uint8_t>((version & 0x00FF0000) >> 16);
    auto minor = static_cast<uint8_t>((version & 0x0000FF00) >> 8);
    auto type = static_cast<ProofType>((version & 0x000000FF));
    return (kMajor > major ||
            (major == kMajor && kMinor >= minor && minor > 0)) &&
           type >= ProofType::kTransfer && type <= ProofType::kApprove;
  }

 private:
  const uint16_t kMajor = 1;
  const uint16_t kMinor = 1;

 private:
  PLATON_EVENT2(ValidatorMigrateEvent, const Address &, const Address &);
};

PLATON_DISPATCH(ConfidentialValidator,
                (init)(ValidateProof)(ValidateSignature)(SupportProof)(Migrate))
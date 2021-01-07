#include <platon/platon.h>
#include <privacy/plaintext_utxo.h>
#include <privacy/validator_interface.hpp>
#include "platon/call.hpp"
#include "privacy/debug/gas/stack_helper.h"
#include "platon/escape_event.hpp"

CONTRACT PlaintextValidator : public ValidatorInterface, public Contract {
 public:
  PLATON_EVENT2(CreateNoteDetailEvent, const h256 &, const bytes &,
                const bytes &, u128, const bytes &)
 public:
  ACTION void init() {}
  /**
   * @brief Verify the plaintext UTXO certificate, the order of verification is:
   * verify signature, verify version number, verify input, verify whether the
   * amount is balanced
   *
   * @param proof Proof byte stream
   *
   * @return Return a serialized byte stream of authentication information
   */
  virtual CONST bytesConstRef
  ValidateProof(const bytesConstRef &proof) override {
    Address spender;
    PlaintextData plaintext_data;
    privacy_assert(PreCheck(proof, spender, plaintext_data) == 0,
                   "precheck failed");

    switch (static_cast<ProofType>(plaintext_data.version & 0x000000FF)) {
      case ProofType::kTransfer:
        return ValidateTransfer(spender, plaintext_data);
      case ProofType::kApprove:
        return ValidateApprove(spender, plaintext_data);
      case ProofType::kMint:
        return ValidateMint(spender, plaintext_data);
      case ProofType::kBurn:
        return ValidateBurn(spender, plaintext_data);
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
  bytesConstRef ValidateTransfer(const Address &spender,
                                 const PlaintextData &plaintext_data) {
    PlaintextUTXO utxo;
    fetch(RLP(plaintext_data.data), utxo);

    InputNotes inputs;
    u128 input_total = 0;
    privacy_assert(CheckInputs(utxo.inputs, inputs, spender, input_total),
                   "checkinput failed");
    DEBUG("check inputs success");

    OutputNotes outputs;
    privacy_assert(CheckBalance(utxo, outputs, input_total),
                   "check balance failed");

    DEBUG("check balance success");
    return SerializeResultRef(
        TransferResult{inputs, outputs, utxo.public_owner, utxo.public_value,
                       bytesConstRef(spender.data(), spender.size)});
  }

  bytesConstRef ValidateApprove(const Address &note_owner,
                                const PlaintextData &plaintext_data) {
    PlaintextApprove approve;
    fetch(RLP(plaintext_data.data), approve);

    DEBUG("note owner", note_owner.toString(), "approve",
          Address(approve.owner).toString());
    privacy_assert(Address(approve.owner) == Address(note_owner),
                   "recover address mismatch with note owner");

    RLPStream stream;
    stream << *reinterpret_cast<const PlaintextNote *>(&approve);
    h256 hash;
    ::platon_sha3(stream.out().data(), stream.out().size(), hash.data(),
                  hash.size);
    DEBUG("validate approve success:", hash.toString(),
          "shared_sign:", toHex(approve.shared_sign))

    return SerializeResultRef(ApproveResult{
        hash,
        bytesConstRef(approve.shared_sign.data(), approve.shared_sign.size()),
        bytesConstRef(note_owner.data(), note_owner.size)});
  }

  bytesConstRef ValidateMint(const Address &note_owner,
                             const PlaintextData &plaintext_data) {
    PlaintextMint mint;
    fetch(RLP(plaintext_data.data), mint);
    OutputNotes outputs;
    u128 total_mint = 0;
    for (const auto &i : mint.outputs) {
      privacy_assert(note_owner == Address(i.owner), "illegal note owner");
      total_mint += i.value;
      RLPStream stream;
      stream << *reinterpret_cast<const PlaintextNote *>(&i);
      h256 hash;
      platon_sha3(stream.out().data(), stream.out().size(), hash.data(),
                  hash.size);
      outputs.push_back(
          OutputNote{VectorWrapper(i.owner), hash, VectorWrapper(i.meta_data)});
      PLATON_EMIT_EVENT2(CreateNoteDetailEvent, hash, i.owner, i.owner, i.value,
                         i.random);
    }

    h256 hash;
    platon_sha3(plaintext_data.data.data(), plaintext_data.data.size(),
                hash.data(), hash.size);
    return SerializeResultRef(
        MintResult{mint.old_mint_hash, hash, total_mint, outputs,
                   bytesConstRef(note_owner.data(), note_owner.size)});
  }

  CONST bytesConstRef ValidateBurn(const Address &note_owner,
                                   const PlaintextData &plaintext_data) {
    PlaintextBurn burn;
    fetch(RLP(plaintext_data.data), burn);
    InputNotes input_notes;
    u128 total_burn = 0;
    privacy_assert(
        CheckInputs(burn.inputs, input_notes, note_owner, total_burn),
        "check input notes failed");

    h256 hash;
    platon_sha3(plaintext_data.data.data(), plaintext_data.data.size(),
                hash.data(), hash.size);

    return SerializeResultRef(
        BurnResult{burn.old_burn_hash, hash, total_burn, input_notes,
                   bytesConstRef(note_owner.data(), note_owner.size)});
  }

  int PreCheck(const bytesConstRef &proof, Address &spender,
               PlaintextData &plaintext) {
    // check signature
    PlaintextProof plain_proof;
    fetch(RLP(proof), plain_proof);

    auto sha3 = platon_sha3(plain_proof.data);
    if (0 != platon_ecrecover(sha3.data(), plain_proof.signature.data(),
                              plain_proof.signature.size(), spender.data())) {
      DEBUG("check proof signature failed")
      return -1;
    }

    DEBUG("check proof signature success")
    fetch(RLP(plain_proof.data), plaintext);

    if (!MatchVersion(plaintext.version)) {
      DEBUG("check version failed");
      return -1;
    }
    DEBUG("check version success");
    return 0;
  }
  
  /**
   * @brief Verify that the version number is supported
   *
   * @param version version number
   *
   * @return true succeeds, false fails
   */
  bool MatchVersion(uint32_t version) {
    auto major = static_cast<uint8_t>((version & 0x00FF0000) >> 16);
    auto minor = static_cast<uint8_t>((version & 0x0000FF00) >> 8);
    auto type = static_cast<ProofType>((version & 0x000000FF));
    return (kMajor > major ||
            (major == kMajor && kMinor >= minor && minor > 0)) &&
           type >= ProofType::kTransfer && type <= ProofType::kApprove;
  }

  bool CheckInputs(const std::vector<PlaintextInputNote> &inputs,
                   InputNotes &input_notes, const Address &spender,
                   u128 &input_total) {
    input_total = 0;
    for (const auto &i : inputs) {
      PlaintextInputNote &one_input = const_cast<PlaintextInputNote &>(i);
      one_input.spender = spender;

      RLPStream stream;
      stream << *reinterpret_cast<const SpenderNote *>(&i);

      h256 hash;
      ::platon_sha3(stream.out().data(), stream.out().size(), hash.data(),
                    hash.size);

      Address result;
      platon_ecrecover(hash.data(), i.signature.data(), i.signature.size(),
                       result.data());

      DEBUG("stream:", toHex(stream.out()), "note hash", hash.toString(),
            "spender", Address(i.spender).toString(), "result",
            result.toString(), "owner", Address(i.owner).toString());
      Address owner(i.owner);
      if (result == owner) {
        stream.clear();
        stream << *reinterpret_cast<const PlaintextNote *>(&i);
        h256 hash;
        ::platon_sha3(stream.out().data(), stream.out().size(), hash.data(),
                      hash.size);
        input_notes.push_back(InputNote{VectorWrapper(i.owner), hash});
      } else {
        return false;
      }
      input_total += i.value;
    }
    return true;
  }

  /**
   * @brief Verify balance
   *
   * @param utxo Plaintext utxo
   * @param outputs Plaintext utxo
   * @param public_value Transfer amount
   * @return Return true on success, false on failure
   */
  bool CheckBalance(const PlaintextUTXO &utxo, OutputNotes &outputs,
                    u128 input_total) {
    u128 output_total = 0;

    for (const auto &i : utxo.outputs) {
      output_total += i.value;
      RLPStream stream;
      stream << *reinterpret_cast<const PlaintextNote *>(&i);
      h256 hash;
      platon_sha3(stream.out().data(), stream.out().size(), hash.data(),
                  hash.size);
      DEBUG("create outputs note", toHex(stream.out()),
            "hash:", hash.toString());

      outputs.push_back(
          OutputNote{VectorWrapper(i.owner), hash, VectorWrapper(i.meta_data)});
      PLATON_EMIT_EVENT2(CreateNoteDetailEvent, hash, i.owner, i.owner, i.value,
                         i.random);
    }

    if (utxo.public_value < 0) {
      if (input_total != 0) {
        return false;
      }
      if (output_total != -utxo.public_value) {
        DEBUG("balance error:", "output:", output_total,
              "public:", utxo.public_value);
        return false;
      }
    } else if (input_total != output_total + utxo.public_value) {
      DEBUG("balance error:", "input:", input_total, "output:", output_total,
            "public:", utxo.public_value);
      return false;
    }
    return true;
  }

 private:
  const uint16_t kMajor = 1;
  const uint16_t kMinor = 1;

 private:
  PLATON_EVENT2(ValidatorMigrateEvent, const Address &, const Address &);
};

PLATON_DISPATCH(PlaintextValidator,
                (init)(ValidateProof)(ValidateSignature)(SupportProof)(Migrate))
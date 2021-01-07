#include <privacy/plaintext_utxo.h>
#include <platon/platon.hpp>
#include <privacy/validator_interface.hpp>
using namespace platon;

using namespace privacy;

CONTRACT PlaintextValidatorUpgradeTest : public ValidatorInterface,
                                         public platon::Contract {
 public:
  PLATON_EVENT2(CreateNoteDetailEvent, const h256 &, const platon::bytes &,
                const platon::bytes &, u128, const platon::bytes &)
 public:
  ACTION void init() {}

  ACTION void SetVersion(uint16_t major, uint16_t minor) {
    platon_set_state((const byte *)&kMajor, sizeof(kMajor),
                     (const byte *)&major, sizeof(major));
    platon_set_state((const byte *)&kMinor, sizeof(kMinor),
                     (const byte *)&minor, sizeof(minor));
  }

  CONST uint16_t Major() {
    uint16_t major;
    platon_get_state((const byte *)&kMajor, sizeof(kMajor), (byte *)&major,
                     sizeof(major));
    return major;
  }

  CONST uint16_t Minor() {
    uint16_t minor;

    platon_get_state((const byte *)&kMinor, sizeof(kMinor), (byte *)&minor,
                     sizeof(minor));
    return minor;
  }
  /**
   * @brief 验证明文UTXO证明，验证顺序依次是 验证签名，验证版本号，验证input,
   * 验证金额是否平衡
   *
   * @param proof 证明字节流
   *
   * @return 返回验证结果
   */
  virtual CONST platon::bytes ValidateProof(const platon::bytes &proof)
      override {
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

    return platon::bytes{};
  }

  /**
   * @brief 验证签名合法性
   *
   * @param owner 公钥
   * @param hash 数据hash
   * @param signature 签名
   * @return 成功返回 true, 失败返回 false
   */
  CONST bool ValidateSignature(const platon::bytes &owner,
                               const platon::h256 &hash,
                               const platon::bytes &signature) override {
    Address result;
    platon_ecrecover(hash, signature, result);
    DEBUG("result", result.toString(), "owner", Address(owner).toString(),
          "hash", hash.toString(), "signature", toHex(signature));
    return result == Address(owner);
  }

  /**
   * @brief 判断插件是否支持指定版本证明
   *
   * @param version 版本好
   * @return 支持返回 true, 失败返回 false
   */
  CONST bool SupportProof(uint32_t version) override {
    return MatchVersion(version);
  }

  ACTION platon::Address Migrate(const platon::Address &address) {
    DEBUG("migrate address", address.toString());
    bytes init_rlp = platon::cross_call_args("init");
    platon::Address new_address;
    bool success = platon::platon_migrate_contract(
        new_address, address, init_rlp, platon::u128(0), ::platon_gas());
    privacy_assert(success, "migrate failed");
    PLATON_EMIT_EVENT2(ValidatorMigrateEvent, platon::platon_address(),
                       new_address);
    return new_address;
  }

 private:
  platon::bytes ValidateTransfer(const Address &spender,
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
    return SerializeResult(
        TransferResult{inputs, outputs, utxo.public_owner, utxo.public_value});
  }

  platon::bytes ValidateApprove(const Address &note_owner,
                                const PlaintextData &plaintext_data) {
    PlaintextApprove approve;
    fetch(RLP(plaintext_data.data), approve);

    DEBUG("note owner", note_owner.toString(), "approve",
          Address(approve.owner).toString());
    privacy_assert(Address(approve.owner) == note_owner,
                   "recover address mismatch with note owner");

    RLPStream stream;
    stream << *reinterpret_cast<const PlaintextNote *>(&approve);
    h256 hash;
    ::platon_sha3(stream.out().data(), stream.out().size(), hash.data(),
                  hash.size);
    DEBUG("validate approve success:", hash.toString(),
          "shared_sign:", toHex(approve.shared_sign))

    return SerializeResult(ApproveResult{hash, approve.shared_sign});
  }

  platon::bytes ValidateMint(const Address &note_owner,
                             const PlaintextData &plaintext_data) {
    PlaintextMint mint;
    fetch(RLP(plaintext_data.data), mint);
    OutputNotes outputs;
    platon::u128 total_mint = 0;
    for (const auto &i : mint.outputs) {
      privacy_assert(note_owner == Address(i.owner), "illegal note owner");
      total_mint += i.value;
      RLPStream stream;
      stream << *reinterpret_cast<const PlaintextNote *>(&i);
      h256 hash;
      platon_sha3(stream.out().data(), stream.out().size(), hash.data(),
                  hash.size);
      outputs.push_back(OutputNote{i.owner, hash});
      PLATON_EMIT_EVENT2(CreateNoteDetailEvent, hash, i.owner, i.owner, i.value,
                         i.random);
    }

    return SerializeResult(MintResult{mint.old_mint_hash, total_mint, outputs});
  }

  CONST platon::bytes ValidateBurn(const Address &note_owner,
                                   const PlaintextData &plaintext_data) {
    PlaintextBurn burn;
    fetch(RLP(plaintext_data.data), burn);
    InputNotes input_notes;
    u128 total_burn = 0;
    privacy_assert(
        CheckInputs(burn.inputs, input_notes, note_owner, total_burn),
        "check input notes failed");

    return SerializeResult(
        BurnResult{burn.old_burn_hash, total_burn, input_notes});
  }

  template <typename T>
  platon::bytes SerializeResult(const T &result) {
    RLPStream stream;
    stream << result;
    platon::bytes out;
    out.resize(stream.out().size());
    memcpy(out.data(), stream.out().data(), stream.out().size());
    return out;
  }

  int PreCheck(const bytes &proof, Address &spender, PlaintextData &plaintext) {
    // check signature
    PlaintextProof plain_proof;
    fetch(RLP(proof), plain_proof);
    auto sha3 = platon_sha3(plain_proof.data);
    if (platon_ecrecover(sha3, plain_proof.signature, spender) != 0) {
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
   * @brief 验证版本号
   *
   * @param version 版本号
   *
   * @return true 成功， false 失败
   */
  bool MatchVersion(uint32_t version) {
    uint16_t major;
    uint16_t minor;
    platon_get_state((const byte *)&kMajor, sizeof(kMajor), (byte *)&major,
                     sizeof(major));
    platon_get_state((const byte *)&kMinor, sizeof(kMinor), (byte *)&minor,
                     sizeof(minor));
    return (kMajor > major ||
            (major == kMajor && kMinor >= minor && minor > 0));
  }

  /**
   * @brief 验证Input, 验证Input 签名， 生成input{owner, hash}
   *
   * @param utxo 明文utxo
   * @param inputs 输出input
   *
   * @return true 成功， false 失败
   */
  bool CheckInputs(const std::vector<PlaintextInputNote> &inputs,
                   InputNotes &input_notes, const Address &spender,
                   u128 &input_total) {
    input_total = 0;
    for (const auto &i : inputs) {
      ::memcpy(const_cast<byte *>(i.spender.data()), spender.data(),
               spender.size);
      RLPStream stream;
      stream << *reinterpret_cast<const SpenderNote *>(&i);
      h256 hash;
      ::platon_sha3(stream.out().data(), stream.out().size(), hash.data(),
                    hash.size);
      Address result;
      platon_ecrecover(hash, i.signature, result);
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
        DEBUG("create input note", toHex(stream.out()),
              "hash:", hash.toString());
        input_notes.push_back(InputNote{i.owner, hash});
      } else {
        return false;
      }
      input_total += i.value;
    }
    return true;
  }
  /**
   * @brief 验证金额平衡
   *
   * @param utxo 明文utxo
   * @param outputs 输出output
   * @param public_value 转出金额
   * @return true 成功， false 失败
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

      outputs.push_back(OutputNote{i.owner, hash});
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
  const uint64_t kMajor = uint64_t(platon::Name::Raw("major"_n));
  const uint64_t kMinor = uint64_t(platon::Name::Raw("minor"_n));

 private:
  PLATON_EVENT2(ValidatorMigrateEvent, const platon::Address &,
                const platon::Address &);
};

PLATON_DISPATCH(PlaintextValidatorUpgradeTest,
                (init)(ValidateProof)(ValidateSignature)(SupportProof)(Migrate)(
                    SetVersion)(Major)(Minor))
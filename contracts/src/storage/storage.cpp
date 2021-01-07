#include <platon/platon.h>
#include "platon/call.hpp"
#include "privacy/common.hpp"
#include "privacy/debug/gas/stack_helper.h"
#include "privacy/storage_interface.h"
#include "privacy/utxo.h"
#include "privacy/validator_interface.hpp"
#include "platon/escape_event.hpp"

class Storage : public StorageInterface, public Contract {
 public:
  ACTION void init() {}

  /**
   * @brief Create a Registry object
   * @param  can_mint_burn  Whether to allow minting and destruction operations
   */
  ACTION virtual void CreateRegistry(bool can_mint_burn) override {
    privacy_assert(!Initialize(), "had initialized");

    SetInitialize(true);
    SetMintBurn(can_mint_burn);
  }

  /**
   * @brief Query Note information
   *
   * @param hash hash value of note
   * @return There is a return NoteStatus, there is no trigger revert operation
   */
  CONST virtual NoteStatus GetNote(const h256 &hash) override {
    privacy_assert(Initialize(), "uninitialized");
    NoteStatus status;
    GetNoteStatus(hash, status);
    return status;
  }

  /**
   * @brief Update Note, if the Note corresponding to the inputs does not exist,
   * the revert operation will be triggered.
   *
   * @param proof_result Transfer result information
   */
  ACTION virtual void UpdateNotes(const bytesConstRef &proof_result) override {
    privacy_assert(Initialize(), "uninitialized");

    TransferResult result;
    RLP rlp(proof_result);
    fetch(rlp[0], result.inputs);
    fetch(rlp[1], result.outputs);
    fetch(rlp[4], result.sender);

    UpdateInputNotes(result.inputs);
    UpdateOutputNotes(result.outputs, result.sender);
  }

  /**
   * @brief Store approve information.
   * @param  outputs  approve information
   * @return ACTION
   */
  ACTION virtual void Approve(const bytesConstRef &outputs) override {
    DEBUG("approve storage outputs", toHex(outputs));
    privacy_assert(Initialize(), "uninitialized");
    ApproveResult result;
    fetch(RLP(outputs), result);
    privacy_assert(HasNote(result.note_hash), "note is invalid");
    std::array<byte, 40> key = GetApproveKey(result.note_hash);
    platon_set_state(key.data(), key.size(), result.shared_sign.data(),
                     result.shared_sign.size());
    DEBUG("approve storage", result.note_hash.toString());
  }

  /**
   * @brief Get the Approval object.
   * @param  note_hash        output hash
   * @return Return the approve information, if there is no note hash, trigger
   * the revert operation.
   */
  CONST virtual bytesConstRef GetApproval(const h256 &note_hash) override {
    privacy_assert(Initialize(), "uninitialized");

    std::array<byte, 40> key = GetApproveKey(note_hash);
    size_t length = platon_get_state_length(key.data(), key.size());
    privacy_assert(length != 0, "invalid note hash");
    bytes &shared_secret = *new bytes();
    shared_secret.resize(length);
    platon_get_state(key.data(), key.size(), shared_secret.data(), length);
    return bytesConstRef(shared_secret.data(), shared_secret.size());
  }

  /**
   * @brief Store coin information
   * @param  outputs  Serialized byte stream of coin information
   * @return ACTION
   */
  ACTION virtual void Mint(const bytesConstRef &outputs) override {
    privacy_assert(MintBurn(), "this asset is not mintable");
    MintResult result;
    fetch(RLP(outputs), result);
    UpdateOutputNotes(result.outputs, result.sender);
  }

  /**
   * @brief Destroy tokens
   * @param  outputs   Destroy the information byte stream
   * @return ACTION
   */
  ACTION virtual void Burn(const bytesConstRef &outputs) override {
    privacy_assert(MintBurn(), "this asset is not burnable");
    BurnResult result;
    fetch(RLP(outputs), result);
    UpdateInputNotes(result.inputs);
  }

  /**
   * @brief Migration upgrade
   *
   * @param address Template contract address
   * @return Contract address after upgrade
   */
  ACTION Address Migrate(const Address &address) override {
    bytesConstRef init_rlp = escape::cross_call_args_ref("init");
    bytes value_bytes = value_to_bytes(u128(0));
    bytes gas_bytes = value_to_bytes(::platon_gas());

    Address new_address;
    bool success = ::platon_clone_migrate(
                       address.data(), new_address.data(), init_rlp.data(),
                       init_rlp.size(), value_bytes.data(), value_bytes.size(),
                       gas_bytes.data(), gas_bytes.size()) == 0;

    privacy_assert(success, "migrate failed");
    PLATON_EMIT_EVENT2(StorageMigrateEvent, platon_address(), new_address);
    return new_address;
  }

 private:
  void DelApproval(const h256 &note_hash) {
    privacy_assert(Initialize(), "uninitialized");
    std::array<byte, 40> key = GetApproveKey(note_hash);
    byte del = 0;
    platon_set_state(key.data(), key.size(), &del, 0);
  }

  std::array<byte, 40> GetApproveKey(const h256 &note_hash) {
    std::array<byte, 40> key;
    memcpy(key.data(), (const byte *)&kApproveKeyPrefix,
           sizeof(kApproveKeyPrefix));
    memcpy(key.data() + sizeof(kApproveKeyPrefix), note_hash.data(),
           note_hash.size);
    DEBUG("key:", toHex(key))
    return key;
  }

  bool HasNote(const h256 &hash) {
    size_t len = ::platon_get_state_length(hash.data(), hash.size);
    DEBUG("has note len:", len);
    return len != 0;
  }

  void GetNoteStatus(const h256 &hash, NoteStatus &status) {
    size_t len = ::platon_get_state_length(hash.data(), hash.size);
    privacy_assert(len != 0, "illegal note hash");
    byte *buf = (byte *)malloc(len);
    platon_get_state(hash.data(), hash.size, buf, len);
    fetch(RLP(buf, len), status);
  }

  void DestroyNote(const h256 &hash) {
    platon_set_state(hash.data(), hash.size, nullptr, 0);
  }

  void CreateNote(const NoteStatus &status) {
    RLPStream stream;
    stream.reserve(70);
    stream << status;
    DEBUG("create note status:", status.hash.toString());
    platon_set_state(status.hash.data(), status.hash.size, stream.out().data(),
                     stream.out().size());
  }

  void UpdateInputNotes(const InputNotes &inputs) {
    DEBUG("update note");
    for (auto &input : inputs) {
      DEBUG("destroy note:", input.hash.toString());

      privacy_assert(HasNote(input.hash), "input note does not exist");
      DestroyNote(input.hash);
      DelApproval(input.hash);
    }
    DEBUG("update input success");
  }

  void UpdateOutputNotes(const OutputNotes &outputs,
                         const platon::bytesConstRef &sender) {
    for (auto &output : outputs) {
      DEBUG("create note:", output.hash.toString());
      privacy_assert(!HasNote(output.hash), output.hash.toString(),
                     "output note already exists");
      NoteStatus status{std::move(output.owner.ToBytes()),
                        std::move(output.hash), sender.toBytes()};
      CreateNote(status);
    }
    DEBUG("update output success");
  }

  void SetInitialize(bool init) {
    byte status = static_cast<byte>(init);
    platon_set_state((const byte *)&kInitialize, sizeof(kInitialize), &status,
                     sizeof(status));
  }

  bool Initialize() {
    if (platon_get_state_length((const byte *)&kInitialize,
                                sizeof(kInitialize)) == 0) {
      return false;
    }

    byte status = 0;
    platon_get_state((const byte *)&kInitialize, sizeof(kInitialize), &status,
                     sizeof(status));
    return static_cast<bool>(status);
  }

  void SetMintBurn(bool mint_burn) {
    byte status = static_cast<byte>(mint_burn);
    platon_set_state((const byte *)&kMintBurn, sizeof(kMintBurn), &status,
                     sizeof(status));
  }

  bool MintBurn() {
    byte status;
    platon_get_state((const byte *)&kMintBurn, sizeof(kMintBurn), &status,
                     sizeof(status));
    return static_cast<bool>(status);
  }

  const uint64_t kApproveKeyPrefix = uint64_t(Name::Raw("approve"_n));
  const uint64_t kInitialize = uint64_t(Name::Raw("initialize"_n));
  const uint64_t kMintBurn = uint64_t(Name::Raw("mint_burn"_n));

 private:
  PLATON_EVENT2(StorageMigrateEvent, const Address &, const Address &);
};

PLATON_DISPATCH(Storage, (init)(Approve)(GetApproval)(Mint)(Burn)(
                             CreateRegistry)(GetNote)(UpdateNotes)(Migrate))
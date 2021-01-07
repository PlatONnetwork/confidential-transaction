
#include <platon/platon.h>
#include "platon/call.hpp"
#include "privacy/common.hpp"
#include "privacy/debug/gas/stack_helper.h"
#include "privacy/storage_interface.h"
#include "privacy/utxo.h"
#include "privacy/validator_interface.hpp"
#include "platon/escape_event.hpp"

class ConfidentialStorage : public StorageInterface, public Contract {
 public:
  ACTION void init() {}

  /**
   * @brief Create a Registry object
   * @param can_mint_burn  Whether to allow minting and destruction operations
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
   * @brief update storage
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
   * @brief Store approve information, this interface is invalid in this
   * version.
   * @param  outputs  approve information
   * @return ACTION
   */
  ACTION virtual void Approve(const bytesConstRef &outputs) override {
    privacy_assert(false, "Unsupported operation");
  }

  /**
   * @brief Get the Approval object, this interface is invalid in this version.
   * @param  note_hash  output hash
   * @return Return the approve information, if there is no note hash, trigger
   * the revert operation.
   */
  CONST virtual bytesConstRef GetApproval(const h256 &note_hash) override {
    privacy_assert(false, "Unsupported operation");

    return bytesConstRef();
  }

  /**
   * @brief Store coin information
   * @param  outputs  Serialized byte stream of coin information
   * @return ACTION
   */
  ACTION virtual void Mint(const bytesConstRef &outputs) override {
    privacy_assert(MintBurn(), "this asset is not mintable");

    MintResult result;
    RLP rlp(outputs);
    fetch(rlp[3], result.outputs);
    fetch(rlp[4], result.sender);

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
    RLP rlp(outputs);
    fetch(rlp[3], result.inputs);

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

  void CreateNote(const NoteStatusConst &status) {
    DEBUG("note status owner:", toHex(status.owner), "hash",
          status.hash.toString(), "sender",
          Address(status.sender.toBytes()).toString());
    RLPStream stream;
    stream.reserve(140);

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
    }
    DEBUG("update input success");
  }

  void UpdateOutputNotes(const OutputNotes &outputs,
                         const platon::bytesConstRef &sender) {
    for (auto &output : outputs) {
      DEBUG("create note:", output.hash.toString());
      privacy_assert(!HasNote(output.hash), "output note already exists");
      NoteStatusConst status{output.owner.ToBytesConstRef(),
                             std::move(output.hash), sender};
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

  const uint64_t kInitialize = uint64_t(Name::Raw("initialize"_n));
  const uint64_t kMintBurn = uint64_t(Name::Raw("mint_burn"_n));

 private:
  PLATON_EVENT2(StorageMigrateEvent, const Address &, const Address &);
};

PLATON_DISPATCH(ConfidentialStorage,
                (init)(Approve)(GetApproval)(Mint)(Burn)(CreateRegistry)(
                    GetNote)(UpdateNotes)(Migrate))
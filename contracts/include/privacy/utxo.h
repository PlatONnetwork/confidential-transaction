#pragma once
#include <platon/platon.h>
#include "platon/vector_wrapper.hpp"
namespace privacy {

struct EncryptedOwner {
  platon::VectorWrapper
      ephemeral_pk;               // Temporary public key created by the sender
  platon::VectorWrapper sign_pk;  // Encrypted owner
  PLATON_SERIALIZE(EncryptedOwner, (ephemeral_pk)(sign_pk))
};

struct InputNote {
  platon::VectorWrapper owner;
  platon::h256 hash;  // note hash
  PLATON_SERIALIZE(InputNote, (owner)(hash))
};

struct OutputNote : public InputNote {
  platon::VectorWrapper meta_data;  // Remarks information
  PLATON_SERIALIZE(OutputNote, (owner)(hash)(meta_data))
};

typedef std::vector<InputNote> InputNotes;
typedef std::vector<OutputNote> OutputNotes;

struct NoteStatus {
  platon::bytes owner;
  platon::h256 hash;
  platon::bytes sender;
  PLATON_SERIALIZE(NoteStatus, (owner)(hash)(sender))
};

struct NoteStatusConst {
  platon::bytesConstRef owner;
  platon::h256 hash;
  platon::bytesConstRef sender;
  PLATON_SERIALIZE(NoteStatusConst, (owner)(hash)(sender))
};

}  // namespace privacy
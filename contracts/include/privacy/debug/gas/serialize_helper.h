
#define PLATON_REFLECT_MEMBER_NUMBER(r, OP, elem) items_number++;

#define PLATON_REFLECT_MEMBER_OP_INPUT(r, OP, elem) OP t.elem

#define PLATON_REFLECT_MEMBER_OP_OUTPUT(r, OP, elem) \
  gas.Reset(BOOST_PP_STRINGIZE(elem), __FUNCTION__, ""); \
  OP(rlp[vect_index], t.elem);                       \
  vect_index++;

/**
 *  Defines serialization and deserialization for a class
 *
 *  @brief Defines serialization and deserialization for a class
 *
 *  @param TYPE - the class to have its serialization and deserialization
 * defined
 *  @param MEMBERS - a sequence of member names.  (field1)(field2)(field3)
 */
#define PLATON_SERIALIZE(TYPE, MEMBERS)                                      \
  friend platon::RLPStream& operator<<(platon::RLPStream& rlp,               \
                                       const TYPE& t) {                      \
    size_t items_number = 0;                                                 \
    BOOST_PP_SEQ_FOR_EACH(PLATON_REFLECT_MEMBER_NUMBER, <<, MEMBERS)         \
    rlp.appendList(items_number);                                            \
    platon::RLPSize rlps;                                                    \
    rlps BOOST_PP_SEQ_FOR_EACH(PLATON_REFLECT_MEMBER_OP_INPUT, <<, MEMBERS); \
    rlp.reserve(rlps.size());                                                \
    return rlp BOOST_PP_SEQ_FOR_EACH(PLATON_REFLECT_MEMBER_OP_INPUT, <<,     \
                                     MEMBERS);                               \
  }                                                                          \
  friend void fetch(const platon::RLP& rlp, TYPE& t) {                       \
    Gas gas("fetch", "", ""); \
    size_t vect_index = 0;                                                   \
    BOOST_PP_SEQ_FOR_EACH(PLATON_REFLECT_MEMBER_OP_OUTPUT, fetch, MEMBERS)   \
  }                                                                          \
  friend platon::RLPSize& operator<<(platon::RLPSize& rlps, const TYPE& t) { \
    rlps << platon::RLPSize::list_start();                                   \
    rlps BOOST_PP_SEQ_FOR_EACH(PLATON_REFLECT_MEMBER_OP_INPUT, <<, MEMBERS); \
    return rlps << platon::RLPSize::list_end();                              \
  }
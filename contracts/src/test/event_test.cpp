#include <memory>
#include <platon/platon.hpp>
#include <stack>
#include "privacy/common.hpp"

struct Demo {
  Demo(){}
  Demo(const platon::h256 &a):old_burn_hash(a){}
  platon::h256 old_burn_hash; // 上一次销毁证明hash
  PLATON_SERIALIZE(Demo, (old_burn_hash));
};

// template<typename T, 
// class = typename std::enable_if<std::is_same<T, Demo>::value>::type>
RLPStream & operator << (RLPStream &stream, Demo &demo) {
  DEBUG("stream  new demo")
  stream << std::as_const(demo);
  return stream;
}

#include "privacy/debug/gas/stack_helper.h"

#define EVENT int i = 10;
#define EVENT int i = 11;
// using namespace platon;

// #ifdef DEBUG
// #define PLATON(x) (funcName.push(__FUNCTION__),platon::x)

// #else
// #define PLATON(x) (platon::x)
// #endif

  template <typename T>
  size_t Serialize(const T &t) {
    EVENT
    DEBUG("i----:", i);
    DEBUG("class serialize");
    RLPStream stream;
    Demo demo;
    stream << demo;
    return stream.out().size();
  }
  template <typename T>
  size_t Serialize(const T &&t) {
    DEBUG("class out serialize");
    T tt  = std::move(t);
    DEBUG("tt:", tt.old_burn_hash.toString());
    return Serialize(tt);
  }

  

class EventTest : public platon::Contract {
 public:
  void init() {}
  ACTION void TestEvent(const std::string &note) {
    platon::Address owner;
    // platon_assert(owner == platon_caller(), "");
    // int *p;
    // p = (int *)__builtin_frame_address(0);
    // printf("f    return address: %p\n", p);
    // p = (int *)__builtin_frame_address(1);
    // printf("f    return address: %p\n", p);
    // PLATON_EMIT_EVENT2(CreateEvent, note, note, note, note);
    h256 a{1,2,3};
  Serialize(Demo(a));
    DEBUG("Test event end")
  }



  void TestStack() {
    int *p;
    p = (int *)__builtin_frame_address(0);
    printf("f    stack return address: %p\n", p);
    p = (int *)__builtin_frame_address(1);
    printf("f    stack return address: %p\n", p);
  }
  PLATON_EVENT2(CreateEvent, const std::string &, const std::string &,
                const std::string &, const std::string &);
};

PLATON_DISPATCH(EventTest, (init)(TestEvent))
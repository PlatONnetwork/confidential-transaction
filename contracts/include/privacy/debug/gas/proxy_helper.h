#define PA_F_HELPER(...)                     \
  M_CAT(params_help, ARG_COUNT(__VA_ARGS__)) \
  (1, ARG_COUNT(__VA_ARGS__), __VA_ARGS__)

#define params_help0(N, total, a)
#define params_help1(N, total, a) M_CAT(arg, N)
#define params_help2(N, total, a, ...) \
  M_CAT(arg, N)                        \
  , params_help1(M_CAT(ARG_POS, total)(__VA_ARGS__), total, __VA_ARGS__)
#define params_help3(N, total, a, ...) \
  M_CAT(arg, N)                        \
  , params_help2(M_CAT(ARG_POS, total)(__VA_ARGS__), total, __VA_ARGS__)
#define params_help4(N, total, a, ...) \
  M_CAT(arg, N)                        \
  , params_help3(M_CAT(ARG_POS, total)(__VA_ARGS__), total, __VA_ARGS__)
#define params_help5(N, total, a, ...) \
  M_CAT(arg, N)                        \
  , params_help4(M_CAT(ARG_POS, total)(__VA_ARGS__), total, __VA_ARGS__)
#define params_help6(N, total, a, ...) \
  M_CAT(arg, N)                        \
  , params_help5(M_CAT(ARG_POS, total)(__VA_ARGS__), total, __VA_ARGS__)
#define params_help7(N, total, a, ...) \
  M_CAT(arg, N)                        \
  , params_help6(M_CAT(ARG_POS, total)(__VA_ARGS__), total, __VA_ARGS__)
#define params_help8(N, total, a, ...) \
  M_CAT(arg, N)                        \
  , params_help7(M_CAT(ARG_POS, total)(__VA_ARGS__), total, __VA_ARGS__)
#define params_help9(N, total, a, ...) \
  M_CAT(arg, N)                        \
  , params_help8(M_CAT(ARG_POS, total)(__VA_ARGS__), total, __VA_ARGS__)
#define params_help10(N, total, a, ...) \
  M_CAT(arg, N)                         \
  , params_help9(M_CAT(ARG_POS, total)(__VA_ARGS__), total, __VA_ARGS__)

#define PROXY_INTERFACE_HELPER(NAME, RETURN, ...) \
  RETURN NAME(VA_F(__VA_ARGS__)) {                \
    Gas gas(__FUNCTION__, "proxy_call", "");      \
    return proxy_.NAME(PA_F_HELPER(__VA_ARGS__)); \
  }

#define PROXY_INTERFACE_VOID_HELPER(NAME, ...) \
  void NAME(VA_F(__VA_ARGS__)) {               \
    Gas gas(__FUNCTION__, "proxy_call", "");   \
    proxy_.NAME(PA_F_HELPER(__VA_ARGS__));     \
  }

template <typename T>
class ProxyHelper {
 public:
  ProxyHelper() = default;
  explicit ProxyHelper(const platon::Address &addr) : proxy_(addr) {}
  void SetContractProxy(const Address &proxy) {
    proxy_.SetContractProxy(proxy);
  }
  void GetContractProxy(const Address &proxy) {
    proxy_.GetContractProxy(proxy);
  }
  T proxy_;
};

class ValidatorProxyHelper : public ProxyHelper<ValidatorProxy> {
 public:
  ValidatorProxyHelper() = default;
  explicit ValidatorProxyHelper(const platon::Address &addr)
      : ProxyHelper(addr) {}

 public:
  PROXY_INTERFACE_HELPER(ValidateProof, platon::bytes, const platon::bytes &)
  PROXY_INTERFACE_HELPER(ValidateSignature, bool, const platon::bytes &,
                         const platon::h256 &, const platon::bytes &)
  PROXY_INTERFACE_HELPER(SupportProof, bool, uint32_t)

  PROXY_INTERFACE_HELPER(Migrate, platon::Address, const platon::Address &)
};

static constexpr platon::Name::Raw kValidatorName = "validator"_n;

#define ValidatorAdminProxy AdminProxy<kValidatorName, ValidatorProxyHelper>

class StorageProxyHelper : public ProxyHelper<StorageProxy> {
 public:
  StorageProxyHelper() = default;
  explicit StorageProxyHelper(const Address &addr) : ProxyHelper(addr) {}

  PROXY_INTERFACE_VOID_HELPER(CreateRegistry, bool)
  PROXY_INTERFACE_HELPER(GetNote, NoteStatus, const h256 &)

  PROXY_INTERFACE_VOID_HELPER(UpdateNotes, const bytes &)
  PROXY_INTERFACE_HELPER(Approve, bytes, const bytes &)

  PROXY_INTERFACE_HELPER(GetApproval, bytes, const h256 &)

  PROXY_INTERFACE_VOID_HELPER(Mint, const bytes &)
  PROXY_INTERFACE_VOID_HELPER(Burn, const bytes &)

  PROXY_INTERFACE_HELPER(Migrate, Address, const Address &)
};

static constexpr platon::Name::Raw kStorageName = "storage"_n;

#define StorageAdminProxy AdminProxy<kStorageName, StorageProxyHelper>

class AclProxyHelper : public ProxyHelper<AclProxy> {
 public:
  AclProxyHelper() = default;
  explicit AclProxyHelper(const platon::Address &addr) : ProxyHelper(addr) {}

 public:
  PROXY_INTERFACE_HELPER(CreateRegistry, bool, uint32_t, uint32_t, u128,
                         const Address &, bool)
  PROXY_INTERFACE_HELPER(ValidateProof, bytes, const bytes &)
  PROXY_INTERFACE_HELPER(Transfer, bytes, const bytes &)
  PROXY_INTERFACE_VOID_HELPER(UpdateNotes, const bytes &)
  PROXY_INTERFACE_HELPER(Approve, bytes, const bytes &)
  PROXY_INTERFACE_HELPER(GetApproval, bytes, const h256 &)
  PROXY_INTERFACE_HELPER(Mint, bytes, const bytes &)
  PROXY_INTERFACE_HELPER(Burn, bytes, const bytes &)
  PROXY_INTERFACE_HELPER(GetNote, NoteStatus, const h256 &)
  PROXY_INTERFACE_HELPER(ValidateSignature, bool, const bytes &, const h256 &,
                         const bytes &)
  PROXY_INTERFACE_HELPER(GetRegistry, Registry, const Address &)
  PROXY_INTERFACE_HELPER(UpdateValidator, bool, uint32_t)
  PROXY_INTERFACE_HELPER(UpdateStorage, bool, uint32_t)
  PROXY_INTERFACE_HELPER(SupportProof, bool, uint32_t)
};

#define AclProxy AclProxyHelper

class Arc20ProxyHelper : public ProxyHelper<Arc20Proxy> {
 public:
  explicit Arc20ProxyHelper(const Address &addr) : ProxyHelper(addr) {}

  PROXY_INTERFACE_HELPER(GetTotalSupply, u128)

  PROXY_INTERFACE_HELPER(BalanceOf, u128, const Address &)

  PROXY_INTERFACE_HELPER(Allowance, u128, const Address &, const Address &)

  PROXY_INTERFACE_HELPER(Transfer, bool, const Address &, u128)

  PROXY_INTERFACE_HELPER(TransferFrom, bool, const Address &, const Address &,
                         u128)

  PROXY_INTERFACE_HELPER(Approve, bool, const Address &, u128)
};

#define Arc20Proxy Arc20ProxyHelper

class TokenManagerProxyHelper : public ProxyHelper<TokenManagerProxy> {
 public:
  explicit TokenManagerProxyHelper(const platon::Address &addr) : ProxyHelper(addr) {}

 public:
  PROXY_INTERFACE_HELPER(SetOwner, bool, const platon::Address &)
  PROXY_INTERFACE_HELPER(GetOwner, platon::Address)
  PROXY_INTERFACE_HELPER(Deposit, bool, const platon::Address &, const platon::Address &, platon::u128)
  PROXY_INTERFACE_HELPER(Withdraw, bool, const platon::Address &, const platon::Address &, platon::u128)
};

#define TokenManagerProxy TokenManagerProxyHelper

class RegistryProxyHelper : public ProxyHelper<RegistryProxy> {
 public:
  explicit RegistryProxyHelper(const platon::Address &addr) : ProxyHelper(addr) {}

 public:
  PROXY_INTERFACE_HELPER(GetContractAddress, platon::Address, const std::string &)
  PROXY_INTERFACE_HELPER(SetContractAddress, bool, const std::string &, const platon::Address &)
  PROXY_INTERFACE_HELPER(GetManager, platon::Address, const std::string &)
  PROXY_INTERFACE_HELPER(SetManager, bool, const std::string &, const platon::Address &)
};

#define RegistryProxy RegistryProxyHelper
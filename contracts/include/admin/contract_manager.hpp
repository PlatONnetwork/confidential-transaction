#pragma once

#include <platon/platon.h>
#include "platon/call.hpp"
#include "privacy/common.hpp"
#include "platon/escape_event.hpp"
using namespace privacy;

/*
 * Contract version management
 *
 // Name + major version + minor version + type
 */
class ContractManager {
 protected:
  PLATON_EVENT0(UpgradeEvent, uint32_t, const Address&, const std::string&)

  PLATON_EVENT0(CreateEvent, uint32_t, const Address&, const std::string&)

 public:
  /**
   * @brief Get the highest version number of the algorithm
   *
   * @param algorithm_name Algorithm type name
   * @return The specific version information is returned successfully, 0 is
   * returned on failure
   */
  virtual uint32_t Latest(uint8_t algorithm_name) = 0;

  /**
   * @brief Get the latest minor version number of a major version of the
   * algorithm
   *
   * @param algorithm_name Algorithm type name
   * @param major_version Major version number
   * @return The specific version information is returned successfully, 0 is
   * returned on failure
   */
  virtual uint32_t LatestMinor(uint8_t algorithm_name,
                               uint8_t major_version) = 0;
  /**
   * @brief Deploy the specified version of the contract
   *
   * @param version version number
   * @return Successfully return to the deployed address, fail to trigger revert
   */
  virtual Address Deploy(uint32_t version) = 0;

  /**
   * @brief Get the address of the template contract corresponding to the
   * version number
   *
   * @param version version number
   * @return Successfully returns the template contract address, failure to
   * trigger revert
   */
  virtual Address VersionAddress(uint32_t version) = 0;

  /**
   * @brief Create a new type of template contract
   *
   * @param version version number
   * @param address Template contract address
   * @param description Contract description information
   * @return Return true on success, false on failure
   */
  virtual bool Create(uint32_t version, const Address& address,
                      const std::string& description) = 0;
  /**
   * @brief Create the same type of upgraded version contract
   *
   * @param version version number
   * @param address Template contract address
   * @param description Contract description information
   * @return Return true on success, false on failure
   */
  virtual bool Update(uint32_t version, const Address& address,
                      const std::string& description) = 0;
};

template <Name::Raw ManagerName>
class DefaultContractManager : public ContractManager {
 private:
  struct VersionInfo {
    uint8_t name_ = 0;
    uint8_t major_ = 0;
    uint8_t minor_ = 0;
    uint8_t extra_ = 0;
    uint32_t version_ = 0;
    Address address_;
    std::string description_;
    uint8_t GetName() const { return name_; }
    uint8_t GetMajor() const { return major_; }
    uint8_t GetMinor() const { return minor_; }
    uint32_t GetVersion() const { return version_; }
    PLATON_SERIALIZE(VersionInfo, (name_)(major_)(minor_)(extra_)(version_)(
                                      address_)(description_))
  };

  using MultiVersionInfo = db::MultiIndex<
      ManagerName, VersionInfo,
      db::IndexedBy<"name"_n, db::IndexMemberFun<VersionInfo, uint8_t,
                                                 &VersionInfo::GetName,
                                                 db::IndexType::NormalIndex>>,
      db::IndexedBy<"major"_n, db::IndexMemberFun<VersionInfo, uint8_t,
                                                  &VersionInfo::GetMajor,
                                                  db::IndexType::NormalIndex>>,
      db::IndexedBy<"monor"_n, db::IndexMemberFun<VersionInfo, uint8_t,
                                                  &VersionInfo::GetMinor,
                                                  db::IndexType::NormalIndex>>,
      db::IndexedBy<
          "version"_n,
          db::IndexMemberFun<VersionInfo, uint32_t, &VersionInfo::GetVersion,
                             db::IndexType::UniqueIndex>>>;

 public:
  DefaultContractManager() = default;

  /**
   * @brief Get the highest version number of the algorithm
   *
   * @param algorithm_name Algorithm type name
   * @return The specific version information is returned successfully, 0 is
   * returned on failure
   */
  virtual uint32_t Latest(uint8_t algorithm_name) {
    auto index = versionInfo_.template get_index<"name"_n>();
    auto index_iter = index.cbegin(algorithm_name);
    privacy_assert(index_iter != index.cend(algorithm_name),
                   "non-existent version number");

    auto max_iter = index_iter;
    for (; index_iter != index.cend(algorithm_name); ++index_iter) {
      if (index_iter->version_ > max_iter->version_) max_iter = index_iter;
    }
    return max_iter->version_;
  }

  /**
   * @brief Get the latest minor version number of a major version of the
   * algorithm
   *
   * @param algorithm_name Algorithm type name
   * @param major_version Major version number
   * @return The specific version information is returned successfully, 0 is
   * returned on failure
   */
  virtual uint32_t LatestMinor(uint8_t algorithm_name, uint8_t major_version) {
    auto index = versionInfo_.template get_index<"name"_n>();
    auto index_iter = index.cbegin(algorithm_name);
    privacy_assert(index_iter != index.cend(algorithm_name),
                   "non-existent version number");

    auto max_iter = index_iter;
    for (; index_iter != index.cend(algorithm_name); ++index_iter) {
      if (index_iter->major_ == major_version &&
          index_iter->minor_ > max_iter->minor_)
        max_iter = index_iter;
    }

    privacy_assert(max_iter->major_ == major_version,
                   "invalid version minor number");
    return max_iter->version_;
  }

  /**
   * @brief Deploy the specified version of the contract
   *
   * @param version version number
   * @return Successfully return to the deployed address, fail to trigger revert
   */
  virtual Address Deploy(uint32_t version) {
    auto iter = versionInfo_.template find<"version"_n>(version);
    privacy_assert(iter != versionInfo_.cend(), "non-existent version number");

    auto deploy_info =
        escape::platon_create_contract(iter->address_, u128(0), ::platon_gas());

    privacy_assert(deploy_info.second, "deploy contract failed");
    return deploy_info.first;
  }

  /**
   * @brief Get the address of the template contract corresponding to the
   * version number
   *
   * @param version version number
   * @return Successfully returns the template contract address, failure to
   * trigger revert
   */
  virtual Address VersionAddress(uint32_t version) {
    auto iter = versionInfo_.template find<"version"_n>(version);
    privacy_assert(iter != versionInfo_.cend(), "non-existent version number");
    return iter->address_;
  }

  /**
   * @brief Create a new type of template contract
   *
   * @param version version number
   * @param address Template contract address
   * @param description Contract description information
   * @return Return true on success, false on failure
   */
  virtual bool Create(uint32_t version, const Address& address,
                      const std::string& description) {
    auto iter = versionInfo_.template find<"version"_n>(version);
    privacy_assert(iter == versionInfo_.cend(), "existing version");

    VersionInfo one_version{
        .version_ = version, .address_ = address, .description_ = description};
    one_version.name_ = uint8_t(version >> 24);
    one_version.major_ = uint8_t(version >> 16);
    one_version.minor_ = uint8_t(version >> 8);

    auto constructor_func = [&](auto& m) { m = one_version; };
    auto result = versionInfo_.emplace(constructor_func);
    privacy_assert(result.second, "insert failed");
    PLATON_EMIT_EVENT0(CreateEvent, version, address, description);
    DEBUG("manage name", static_cast<uint64_t>(ManagerName), "create",
          "algorithm name", uint8_t(version >> 24), "major version",
          uint8_t(version >> 16), "minor version", uint8_t(version >> 8),
          "address", address.toString(), "description", description)
    return true;
  }

  /**
   * @brief Create the same type of upgraded version contract
   *
   * @param version version number
   * @param address Template contract address
   * @param description Contract description information
   * @return Return true on success, false on failure
   */
  virtual bool Update(uint32_t version, const Address& address,
                      const std::string& description) {
    uint8_t name = uint8_t(version >> 24);
    auto index = versionInfo_.template get_index<"name"_n>();
    auto index_iter = index.cbegin(name);
    privacy_assert(index_iter != index.cend(name),
                   "non-existent version number");

    std::map<uint8_t, uint8_t> major2minor;
    for (; index_iter != index.cend(name); ++index_iter) {
      if (index_iter->minor_ > major2minor[index_iter->major_])
        major2minor[index_iter->major_] = index_iter->minor_;
    }

    uint8_t major = uint8_t(version >> 16);
    uint8_t minor = uint8_t(version >> 8);
    auto map_iter = major2minor.find(major);
    if (map_iter == major2minor.end()) {
      map_iter--;
      privacy_assert(major > map_iter->first, "invalid version minor number");
    } else {
      privacy_assert(minor > map_iter->second, "invalid version minor number");
    }

    auto constructor_func = [&](auto& m) {
      m.name_ = name;
      m.major_ = major;
      m.minor_ = minor;
      m.version_ = version;
      m.address_ = address;
      m.description_ = description;
    };
    auto result = versionInfo_.emplace(constructor_func);
    privacy_assert(result.second, "insert failed");
    PLATON_EMIT_EVENT0(UpgradeEvent, version, address, description);
    DEBUG("manage name", static_cast<uint64_t>(ManagerName), "update",
          "algorithm name", uint8_t(version >> 24), "major version",
          uint8_t(version >> 16), "minor version", uint8_t(version >> 8),
          "address", address.toString(), "description", description)
    return true;
  }

 private:
  MultiVersionInfo versionInfo_;
};

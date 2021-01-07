#pragma once
#include <platon/platon.h>
#include "admin/admin_proxy.hpp"
#include "privacy/storage_proxy.hpp"
namespace privacy {
namespace internal {
static const Name::Raw kStorageName = "storage"_n;
}
using StorageAdminProxy = AdminProxy<internal::kStorageName, StorageProxy>;
}  // namespace privacy

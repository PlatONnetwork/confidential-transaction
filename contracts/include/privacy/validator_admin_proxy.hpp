#pragma once

#include <platon/platon.h>
#include "admin/admin_proxy.hpp"
#include "privacy/validator_proxy.hpp"
namespace privacy {
namespace internal {
static constexpr platon::Name::Raw kValidatorName = "validator"_n;
}
using ValidatorAdminProxy =
    AdminProxy<internal::kValidatorName, ValidatorProxy>;
}  // namespace privacy
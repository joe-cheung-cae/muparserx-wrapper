#include "tfp/utility/expression/expression_error.h"

namespace tfp
{
namespace utility
{

ExpressionError::ExpressionError(ExpressionErrorCode code, const std::string& message)
    : std::runtime_error(message),
      code_(code)
{
}

ExpressionErrorCode ExpressionError::Code() const noexcept
{
    return code_;
}

} // namespace utility
} // namespace tfp

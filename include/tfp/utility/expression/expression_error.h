#ifndef TFP_UTILITY_EXPRESSION_ERROR_H
#define TFP_UTILITY_EXPRESSION_ERROR_H

#include <stdexcept>
#include <string>

namespace tfp
{
namespace utility
{

// ExpressionErrorCode identifies which phase of the runtime pipeline reported
// the failure so callers can distinguish configuration problems from runtime
// lookup, compile, or evaluation failures.
enum class ExpressionErrorCode
{
    ConfigError,
    TableError,
    ConstantError,
    CompileError,
    EvaluationError,
    NotFound,
    InvalidArgument
};

class ExpressionError : public std::runtime_error
{
public:
    ExpressionError(ExpressionErrorCode code, const std::string& message);

    ExpressionErrorCode Code() const noexcept;

private:
    ExpressionErrorCode code_;
};

} // namespace utility
} // namespace tfp

#endif

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
    // JSON or in-memory configuration schema validation failed.
    ConfigError,
    // Table configuration failed validation during construction.
    TableError,
    // Constant dependency resolution or constant expression evaluation failed.
    ConstantError,
    // Parser binding, symbol definition, or expression compilation failed.
    CompileError,
    // Evaluation failed after an expression or table had been compiled.
    EvaluationError,
    // A requested expression or symbol name was not present.
    NotFound,
    // A caller supplied the wrong handle state, arity, variable set, or index.
    InvalidArgument
};

// Exception type thrown by the expression runtime. The inherited what() message
// is human-readable, while Code() provides stable phase-level classification.
class ExpressionError : public std::runtime_error
{
public:
    // Stores the supplied classification and message. The message is also
    // passed to std::runtime_error and remains available through what().
    ExpressionError(ExpressionErrorCode code, const std::string& message);

    // Returns the classification associated with this exception without
    // allocating or throwing.
    ExpressionErrorCode Code() const noexcept;

private:
    ExpressionErrorCode code_;
};

} // namespace utility
} // namespace tfp

#endif

#ifndef TFP_UTILITY_EXPRESSION_ERROR_H
#define TFP_UTILITY_EXPRESSION_ERROR_H

#include <stdexcept>
#include <string>

namespace tfp
{
namespace utility
{

/// @brief Classifies expression runtime failures by pipeline stage.
///
/// Use this code to distinguish configuration, compilation, lookup, and
/// evaluation failures without parsing the error string.
enum class ExpressionErrorCode
{
    /// JSON or in-memory configuration schema validation failed.
    ConfigError,
    /// Table configuration failed validation during construction.
    TableError,
    /// Constant dependency resolution or constant expression evaluation failed.
    ConstantError,
    /// Parser binding, symbol definition, or expression compilation failed.
    CompileError,
    /// Evaluation failed after an expression or table had been compiled.
    EvaluationError,
    /// A requested expression or symbol name was not present.
    NotFound,
    /// A caller supplied the wrong handle state, arity, variable set, or index.
    InvalidArgument
};

/// @brief Exception type thrown by the expression runtime.
///
/// The inherited what() message is human-readable, while Code() provides
/// stable phase-level classification for programmatic handling.
class ExpressionError : public std::runtime_error
{
public:
    /// @brief Stores the supplied classification and message.
    /// @param code Stable phase-level error classification.
    /// @param message Human-readable failure description exposed through what().
    ExpressionError(ExpressionErrorCode code, const std::string& message);

    /// @brief Returns the classification associated with this exception.
    ExpressionErrorCode Code() const noexcept;

private:
    ExpressionErrorCode code_;
};

} // namespace utility
} // namespace tfp

#endif

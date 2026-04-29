#ifndef TFP_UTILITY_EXPRESSION_RUNTIME_H
#define TFP_UTILITY_EXPRESSION_RUNTIME_H

#include "tfp/utility/expression/expression_config.h"
#include "tfp/utility/expression/expression_handle.h"

#include <memory>
#include <string>
#include <unordered_map>

namespace tfp
{
namespace utility
{
namespace detail
{
class ExpressionRuntimeImpl;
}

// ExpressionRuntime owns the compiled expression graph created from a JSON
// configuration and serves as the entry point for loading and evaluating it.
class ExpressionRuntime
{
public:
    // Evaluate mutates parser-bound variable storage. Use one runtime instance
    // per thread when evaluating expressions concurrently.
    ExpressionRuntime();
    ~ExpressionRuntime();

    ExpressionRuntime(ExpressionRuntime&&) noexcept;
    ExpressionRuntime& operator=(ExpressionRuntime&&) noexcept;

    ExpressionRuntime(const ExpressionRuntime&) = delete;
    ExpressionRuntime& operator=(const ExpressionRuntime&) = delete;

    // Loads constants, tables, and expressions from an in-memory normalized
    // configuration object and replaces any previously compiled expressions
    // owned by this runtime.
    void LoadFromConfig(const ExpressionRuntimeConfig& config);
    // Loads constants, tables, and expressions from a JSON document string and
    // replaces any previously compiled expressions owned by this runtime.
    void LoadFromJsonString(const std::string& json_text);
    // Loads the same JSON model from disk before compiling it into runtime
    // state. File and JSON validation failures are reported as ExpressionError.
    void LoadFromJsonFile(const std::string& path);

    // Returns a handle for ordered argument evaluation. The handle shares the
    // compiled expression state owned by this runtime and remains valid after
    // the runtime is moved, but concurrent evaluation still follows the runtime
    // thread-safety rules because both use the same bound parser state.
    ExpressionHandle GetExpression(const std::string& name) const;
    // Returns a handle specialized for expressions that declare exactly one
    // runtime variable in wordable order. Requesting a non-unary expression is
    // treated as an invalid argument error rather than a compile-time error.
    UnaryExpressionHandle GetUnaryExpression(const std::string& name) const;

    // Evaluates an expression by variable name for convenience. This path is
    // useful for low-frequency or debugging use, while repeated evaluation is
    // better served by the typed handle APIs that avoid per-call name lookup.
    double Evaluate(const std::string& name, const std::unordered_map<std::string, double>& variables) const;

private:
    std::unique_ptr<detail::ExpressionRuntimeImpl> impl_;
};

} // namespace utility
} // namespace tfp

#endif

#ifndef TFP_UTILITY_EXPRESSION_RUNTIME_H
#define TFP_UTILITY_EXPRESSION_RUNTIME_H

#include "tfp/utility/expression/expression_config.h"
#include "tfp/utility/expression/expression_handle.h"

#include <memory>
#include <nlohmann/json.hpp>
#include <string>
#include <unordered_map>

namespace tfp
{
namespace utility
{
namespace detail
{
// Private implementation that owns compiled expressions and parser state.
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
    // Releases compiled expressions and parser resources owned by this runtime.
    ~ExpressionRuntime();

    // Transfers ownership of compiled runtime state. Existing handles remain
    // valid because they share compiled expressions independently.
    ExpressionRuntime(ExpressionRuntime&&) noexcept;
    // Replaces this runtime with another runtime's compiled state.
    ExpressionRuntime& operator=(ExpressionRuntime&&) noexcept;

    // Runtime state is unique because it owns mutable parser bindings.
    ExpressionRuntime(const ExpressionRuntime&) = delete;
    // Copy assignment is disabled for the same ownership and thread-safety
    // reasons as copy construction.
    ExpressionRuntime& operator=(const ExpressionRuntime&) = delete;

    // Constructs a runtime and loads an in-memory config before returning it.
    // Throws ExpressionError for the same failures as LoadFromConfig().
    [[nodiscard]] static ExpressionRuntime CreateFromConfig(const ExpressionRuntimeConfig& config);
    // Constructs a runtime from JSON text. Throws ExpressionError when parsing,
    // validation, or compilation fails.
    [[nodiscard]] static ExpressionRuntime CreateFromJsonString(const std::string& json_text);
    // Constructs a runtime from a parsed JSON object. Throws ExpressionError
    // when schema validation or compilation fails.
    [[nodiscard]] static ExpressionRuntime CreateFromJsonObject(const nlohmann::json& json_object);
    // Constructs a runtime from a JSON file path. Throws ExpressionError when
    // file loading, parsing, validation, or compilation fails.
    [[nodiscard]] static ExpressionRuntime CreateFromJsonFile(const std::string& path);

    // Loads constants, tables, and expressions from an in-memory normalized
    // configuration object and replaces any previously compiled expressions
    // owned by this runtime.
    void LoadFromConfig(const ExpressionRuntimeConfig& config);
    // Loads constants, tables, and expressions from a JSON document string and
    // replaces any previously compiled expressions owned by this runtime.
    void LoadFromJsonString(const std::string& json_text);
    // Loads constants, tables, and expressions from a parsed JSON object and
    // replaces any previously compiled expressions owned by this runtime.
    // Schema validation and JSON conversion failures are reported as
    // ExpressionError.
    void LoadFromJsonObject(const nlohmann::json& json_object);
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

    // Returns the ordered runtime argument names for a named item loaded into
    // this runtime. Constants return an empty list, tables return {"x"}, and
    // expressions return their declared wordable order. Throws ExpressionError
    // if the name does not exist.
    std::vector<std::string> GetArgumentNames(const std::string& name) const;

    // Evaluates a loaded runtime item by name. Expressions are evaluated with
    // the supplied variable map, tables use variable "x" or the single supplied
    // variable value, and constants require an empty variable map.
    double Evaluate(const std::string& name, const std::unordered_map<std::string, double>& variables) const;

    // Evaluates a loaded one-dimensional runtime item by name. Constants ignore
    // x, tables evaluate at x, and expressions must declare exactly one runtime
    // variable.
    double EvaluateUnary(const std::string& name, double x) const;

private:
    std::unique_ptr<detail::ExpressionRuntimeImpl> impl_;
};

} // namespace utility
} // namespace tfp

#endif

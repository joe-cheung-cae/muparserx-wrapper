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
/// Private implementation that owns compiled expressions and parser state.
class ExpressionRuntimeImpl;
}

/// @brief Owns the compiled expression graph for a loaded configuration.
///
/// Use ExpressionRuntime as the entry point for loading JSON/config objects and
/// evaluating constants, tables, and expressions by name.
class ExpressionRuntime
{
public:
    /// @brief Constructs an empty runtime.
    ///
    /// Evaluation mutates parser-bound variable storage, so use one runtime
    /// instance per thread when evaluating expressions concurrently.
    ExpressionRuntime();

    /// @brief Releases compiled expressions and parser resources.
    ~ExpressionRuntime();

    /// @brief Transfers ownership of compiled runtime state.
    ///
    /// Existing handles remain valid because they share compiled expressions
    /// independently.
    ExpressionRuntime(ExpressionRuntime&&) noexcept;

    /// @brief Replaces this runtime with another runtime's compiled state.
    ExpressionRuntime& operator=(ExpressionRuntime&&) noexcept;

    /// Copy construction is disabled because runtime state owns mutable parser
    /// bindings.
    ExpressionRuntime(const ExpressionRuntime&) = delete;

    /// Copy assignment is disabled for the same ownership and thread-safety
    /// reasons as copy construction.
    ExpressionRuntime& operator=(const ExpressionRuntime&) = delete;

    /// @brief Constructs a runtime and loads an in-memory config.
    /// @param config Normalized runtime configuration.
    /// @return A loaded runtime instance.
    ///
    /// Throws ExpressionError for the same failures as LoadFromConfig().
    [[nodiscard]] static ExpressionRuntime CreateFromConfig(const ExpressionRuntimeConfig& config);
    /// @brief Constructs a runtime from JSON text.
    /// @param json_text JSON document text.
    /// @return A loaded runtime instance.
    ///
    /// Throws ExpressionError when parsing, validation, or compilation fails.
    [[nodiscard]] static ExpressionRuntime CreateFromJsonString(const std::string& json_text);
    /// @brief Constructs a runtime from a parsed JSON object.
    /// @param json_object Parsed JSON document.
    /// @return A loaded runtime instance.
    ///
    /// Throws ExpressionError when schema validation or compilation fails.
    [[nodiscard]] static ExpressionRuntime CreateFromJsonObject(const nlohmann::json& json_object);
    /// @brief Constructs a runtime from a JSON file path.
    /// @param path File system path to a JSON document.
    /// @return A loaded runtime instance.
    ///
    /// Throws ExpressionError when file loading, parsing, validation, or
    /// compilation fails.
    [[nodiscard]] static ExpressionRuntime CreateFromJsonFile(const std::string& path);

    /// @brief Loads constants, tables, and expressions from a normalized config.
    /// @param config Normalized runtime configuration.
    ///
    /// Replaces any previously compiled expressions owned by this runtime.
    void LoadFromConfig(const ExpressionRuntimeConfig& config);
    /// @brief Loads constants, tables, and expressions from JSON text.
    /// @param json_text JSON document text.
    ///
    /// Replaces any previously compiled expressions owned by this runtime.
    void LoadFromJsonString(const std::string& json_text);
    /// @brief Loads constants, tables, and expressions from a parsed JSON object.
    /// @param json_object Parsed JSON document.
    ///
    /// Schema validation and JSON conversion failures are reported as
    /// ExpressionError.
    void LoadFromJsonObject(const nlohmann::json& json_object);
    /// @brief Loads constants, tables, and expressions from a JSON file.
    /// @param path File system path to a JSON document.
    ///
    /// File and JSON validation failures are reported as ExpressionError.
    void LoadFromJsonFile(const std::string& path);

    /// @brief Returns a snapshot handle for ordered expression evaluation.
    ///
    /// The handle shares compiled expression state and remains valid after
    /// this runtime is moved or reloaded. After a reload it continues to
    /// evaluate the old compiled expression.
    ExpressionHandle GetExpression(const std::string& name) const;
    /// @brief Returns a snapshot handle for expressions with one runtime variable.
    ///
    /// This is expression-only; use EvaluateUnary() for unified
    /// constant/table/expression evaluation.
    UnaryExpressionHandle GetUnaryExpression(const std::string& name) const;

    /// @brief Returns the ordered runtime argument names for a named item.
    ///
    /// Constants return an empty list, tables return {"x"}, and expressions
    /// return their declared wordable order. Throws ExpressionError if the
    /// name does not exist.
    std::vector<std::string> GetArgumentNames(const std::string& name) const;

    /// @brief Evaluates a loaded runtime item by name.
    ///
    /// Expressions require the supplied variable map to match wordable
    /// exactly, tables use the single supplied variable value, and constants
    /// require an empty map.
    double Evaluate(const std::string& name, const std::unordered_map<std::string, double>& variables) const;

    /// @brief Evaluates a loaded one-dimensional runtime item by name.
    ///
    /// Constants ignore x, tables evaluate at x, and expressions must declare
    /// exactly one runtime variable.
    double EvaluateUnary(const std::string& name, double x) const;

private:
    std::unique_ptr<detail::ExpressionRuntimeImpl> impl_;
};

} // namespace utility
} // namespace tfp

#endif

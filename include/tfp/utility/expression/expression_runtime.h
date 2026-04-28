#ifndef TFP_UTILITY_EXPRESSION_RUNTIME_H
#define TFP_UTILITY_EXPRESSION_RUNTIME_H

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

    void LoadFromJsonString(const std::string& json_text);
    void LoadFromJsonFile(const std::string& path);

    ExpressionHandle GetExpression(const std::string& name) const;
    UnaryExpressionHandle GetUnaryExpression(const std::string& name) const;

    double Evaluate(const std::string& name, const std::unordered_map<std::string, double>& variables) const;

private:
    std::unique_ptr<detail::ExpressionRuntimeImpl> impl_;
};

} // namespace utility
} // namespace tfp

#endif

#ifndef TFP_UTILITY_EXPRESSION_HANDLE_H
#define TFP_UTILITY_EXPRESSION_HANDLE_H

#include <memory>
#include <vector>

namespace tfp
{
namespace utility
{
namespace detail
{
class RuntimeExpression;
class ExpressionRuntimeImpl;
}

class ExpressionHandle
{
public:
    ExpressionHandle();

    double Evaluate(const std::vector<double>& args) const;
    std::size_t Arity() const;

private:
    friend class ExpressionRuntime;
    friend class detail::ExpressionRuntimeImpl;
    explicit ExpressionHandle(std::shared_ptr<detail::RuntimeExpression> expression);

    std::shared_ptr<detail::RuntimeExpression> expression_;
};

class UnaryExpressionHandle
{
public:
    UnaryExpressionHandle();

    double Evaluate(double x) const;

private:
    friend class ExpressionRuntime;
    friend class detail::ExpressionRuntimeImpl;
    explicit UnaryExpressionHandle(std::shared_ptr<detail::RuntimeExpression> expression);

    std::shared_ptr<detail::RuntimeExpression> expression_;
};

} // namespace utility
} // namespace tfp

#endif

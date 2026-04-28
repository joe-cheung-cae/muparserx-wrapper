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

// ExpressionHandle is a lightweight reference to a compiled expression that
// evaluates arguments in the order declared by ExpressionConfig::wordable.
class ExpressionHandle
{
public:
    // A default-constructed handle is empty and throws ExpressionError when it
    // is evaluated or queried for arity.
    ExpressionHandle();

    // Evaluates the compiled expression with positional arguments matching the
    // configured wordable order exactly.
    double Evaluate(const std::vector<double>& args) const;
    // Returns the number of positional arguments required by Evaluate().
    std::size_t Arity() const;

private:
    friend class ExpressionRuntime;
    friend class detail::ExpressionRuntimeImpl;
    explicit ExpressionHandle(std::shared_ptr<detail::RuntimeExpression> expression);

    std::shared_ptr<detail::RuntimeExpression> expression_;
};

// UnaryExpressionHandle is the specialized fast path for expressions with one
// runtime variable, avoiding vector construction at each evaluation.
class UnaryExpressionHandle
{
public:
    // A default-constructed unary handle is empty and throws ExpressionError
    // if Evaluate() is called before it is bound to a compiled expression.
    UnaryExpressionHandle();

    // Evaluates the compiled unary expression with a single runtime variable.
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

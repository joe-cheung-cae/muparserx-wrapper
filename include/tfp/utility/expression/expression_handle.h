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
// Internal compiled expression representation shared by public handles.
class RuntimeExpression;
// Internal pimpl type that creates handles for ExpressionRuntime.
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
    // configured wordable order exactly. Throws ExpressionError if the handle
    // is empty, the argument count is wrong, or parser evaluation fails.
    double Evaluate(const std::vector<double>& args) const;
    // Returns the number of positional arguments required by Evaluate(). Throws
    // ExpressionError if the handle is empty.
    std::size_t Arity() const;

private:
    friend class ExpressionRuntime;
    friend class detail::ExpressionRuntimeImpl;
    // Binds the handle to compiled shared state. Runtime factory methods use
    // this constructor so handles can outlive moved or reloaded runtime objects
    // as snapshots of the expression state at lookup time.
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
    // Throws ExpressionError if the handle is empty or evaluation fails.
    double Evaluate(double x) const;

private:
    friend class ExpressionRuntime;
    friend class detail::ExpressionRuntimeImpl;
    // Binds the handle to a compiled expression already checked for arity 1 by
    // ExpressionRuntime::GetUnaryExpression(). The handle is a snapshot of the
    // expression state at lookup time.
    explicit UnaryExpressionHandle(std::shared_ptr<detail::RuntimeExpression> expression);

    std::shared_ptr<detail::RuntimeExpression> expression_;
};

} // namespace utility
} // namespace tfp

#endif

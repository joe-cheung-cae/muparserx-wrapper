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
/// Internal compiled expression representation shared by public handles.
class RuntimeExpression;
/// Internal pimpl type that creates handles for ExpressionRuntime.
class ExpressionRuntimeImpl;
}

/// @brief Lightweight reference to a compiled expression.
///
/// Positional arguments are evaluated in the order declared by
/// ExpressionConfig::wordable.
class ExpressionHandle
{
public:
    /// @brief Constructs an empty handle.
    ///
    /// An empty handle throws ExpressionError when evaluated or queried for
    /// arity.
    ExpressionHandle();

    /// @brief Evaluates the compiled expression with positional arguments.
    /// @param args Positional argument values in wordable order.
    /// @return The evaluated scalar result.
    ///
    /// Throws ExpressionError if the handle is empty, the argument count is
    /// wrong, or parser evaluation fails.
    double Evaluate(const std::vector<double>& args) const;

    /// @brief Returns the number of positional arguments required by Evaluate().
    ///
    /// Throws ExpressionError if the handle is empty.
    std::size_t Arity() const;

private:
    friend class ExpressionRuntime;
    friend class detail::ExpressionRuntimeImpl;
    /// Binds the handle to compiled shared state.
    ///
    /// Runtime factory methods use this constructor so handles can outlive
    /// moved or reloaded runtime objects as snapshots of the expression state
    /// at lookup time.
    explicit ExpressionHandle(std::shared_ptr<detail::RuntimeExpression> expression);

    std::shared_ptr<detail::RuntimeExpression> expression_;
};

/// @brief Specialized fast path for unary expressions.
///
/// This avoids vector construction at each evaluation for expressions with one
/// runtime variable.
class UnaryExpressionHandle
{
public:
    /// @brief Constructs an empty unary handle.
    ///
    /// An empty handle throws ExpressionError if Evaluate() is called before it
    /// is bound to a compiled expression.
    UnaryExpressionHandle();

    /// @brief Evaluates the compiled unary expression.
    /// @param x Value supplied for the single runtime variable.
    /// @return The evaluated scalar result.
    ///
    /// Throws ExpressionError if the handle is empty or evaluation fails.
    double Evaluate(double x) const;

private:
    friend class ExpressionRuntime;
    friend class detail::ExpressionRuntimeImpl;
    /// Binds the handle to compiled shared state already checked for arity 1.
    ///
    /// ExpressionRuntime::GetUnaryExpression() uses this constructor so the
    /// handle remains a snapshot of the expression state at lookup time.
    explicit UnaryExpressionHandle(std::shared_ptr<detail::RuntimeExpression> expression);

    std::shared_ptr<detail::RuntimeExpression> expression_;
};

} // namespace utility
} // namespace tfp

#endif

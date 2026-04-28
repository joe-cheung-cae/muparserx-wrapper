#include "tfp/utility/expression/expression_handle.h"

#include "runtime_expression.h"
#include "tfp/utility/expression/expression_error.h"

namespace tfp
{
namespace utility
{

ExpressionHandle::ExpressionHandle() = default;

ExpressionHandle::ExpressionHandle(std::shared_ptr<detail::RuntimeExpression> expression)
    : expression_(std::move(expression))
{
}

double ExpressionHandle::Evaluate(const std::vector<double>& args) const
{
    if (!expression_)
    {
        throw ExpressionError(ExpressionErrorCode::InvalidArgument, "expression handle is empty");
    }
    return expression_->Evaluate(args);
}

std::size_t ExpressionHandle::Arity() const
{
    if (!expression_)
    {
        throw ExpressionError(ExpressionErrorCode::InvalidArgument, "expression handle is empty");
    }
    return expression_->Arity();
}

UnaryExpressionHandle::UnaryExpressionHandle() = default;

UnaryExpressionHandle::UnaryExpressionHandle(std::shared_ptr<detail::RuntimeExpression> expression)
    : expression_(std::move(expression))
{
}

double UnaryExpressionHandle::Evaluate(double x) const
{
    if (!expression_)
    {
        throw ExpressionError(ExpressionErrorCode::InvalidArgument, "unary expression handle is empty");
    }
    return expression_->EvaluateUnary(x);
}

} // namespace utility
} // namespace tfp

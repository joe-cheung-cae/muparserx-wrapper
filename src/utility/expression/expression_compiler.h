#ifndef TFP_UTILITY_EXPRESSION_COMPILER_H
#define TFP_UTILITY_EXPRESSION_COMPILER_H

#include "runtime_expression.h"

#include <memory>
#include <string>
#include <unordered_map>

namespace tfp
{
namespace utility
{
namespace detail
{

std::shared_ptr<RuntimeExpression> CompileRuntimeExpression(
    const ExpressionConfig& config,
    const std::unordered_map<std::string, double>& constants,
    const std::unordered_map<std::string, std::shared_ptr<const TableFunction> >& tables);

} // namespace detail
} // namespace utility
} // namespace tfp

#endif

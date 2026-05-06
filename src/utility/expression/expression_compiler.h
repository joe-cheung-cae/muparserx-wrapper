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

// Implementation detail used by ExpressionRuntimeImpl to compile one expression
// config against already-resolved constants and immutable table functions.
// Returns shared compiled state so public handles can extend expression
// lifetime beyond runtime moves. Throws ExpressionError on parser binding or
// validation failures.
std::shared_ptr<RuntimeExpression> CompileRuntimeExpression(
    const ExpressionConfig& config,
    const std::unordered_map<std::string, double>& constants,
    const std::unordered_map<std::string, std::shared_ptr<const TableFunction> >& tables);

} // namespace detail
} // namespace utility
} // namespace tfp

#endif

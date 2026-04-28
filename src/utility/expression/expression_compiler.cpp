#include "expression_compiler.h"

namespace tfp
{
namespace utility
{
namespace detail
{

std::shared_ptr<RuntimeExpression> CompileRuntimeExpression(
    const ExpressionConfig& config,
    const std::unordered_map<std::string, double>& constants,
    const std::unordered_map<std::string, std::shared_ptr<const TableFunction> >& tables)
{
    return std::make_shared<RuntimeExpression>(config, constants, tables);
}

} // namespace detail
} // namespace utility
} // namespace tfp

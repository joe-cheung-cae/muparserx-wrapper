#ifndef TFP_UTILITY_EXPRESSION_CONSTANT_RESOLVER_H
#define TFP_UTILITY_EXPRESSION_CONSTANT_RESOLVER_H

#include <string>
#include <unordered_map>

namespace tfp
{
namespace utility
{

// Resolves raw named constant expressions into double values before runtime
// expressions are compiled. Constants may refer to constants already resolved
// in dependency order, but unknown symbols and cycles are reported as
// ExpressionError with ConstantError.
std::unordered_map<std::string, double> ResolveConstants(
    const std::unordered_map<std::string, std::string>& raw_constants);

} // namespace utility
} // namespace tfp

#endif

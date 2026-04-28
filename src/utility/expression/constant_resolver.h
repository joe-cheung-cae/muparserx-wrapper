#ifndef TFP_UTILITY_EXPRESSION_CONSTANT_RESOLVER_H
#define TFP_UTILITY_EXPRESSION_CONSTANT_RESOLVER_H

#include <string>
#include <unordered_map>

namespace tfp
{
namespace utility
{

std::unordered_map<std::string, double> ResolveConstants(
    const std::unordered_map<std::string, std::string>& raw_constants);

} // namespace utility
} // namespace tfp

#endif

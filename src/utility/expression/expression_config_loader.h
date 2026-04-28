#ifndef TFP_UTILITY_EXPRESSION_CONFIG_LOADER_H
#define TFP_UTILITY_EXPRESSION_CONFIG_LOADER_H

#include "tfp/utility/expression/expression_config.h"

#include <string>

namespace tfp
{
namespace utility
{

ExpressionRuntimeConfig LoadExpressionRuntimeConfigFromJsonString(const std::string& json_text);
ExpressionRuntimeConfig LoadExpressionRuntimeConfigFromJsonFile(const std::string& path);

} // namespace utility
} // namespace tfp

#endif

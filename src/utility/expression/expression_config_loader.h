#ifndef TFP_UTILITY_EXPRESSION_CONFIG_LOADER_H
#define TFP_UTILITY_EXPRESSION_CONFIG_LOADER_H

#include "tfp/utility/expression/expression_config.h"

#include <string>

namespace tfp
{
namespace utility
{

// Parses and validates the supported JSON schema into the normalized in-memory
// configuration used by the compiler. Accepts both current top-level sections
// and the legacy functions array, and throws ExpressionError with ConfigError
// for malformed JSON or schema violations.
ExpressionRuntimeConfig LoadExpressionRuntimeConfigFromJsonString(const std::string& json_text);
// Reads a JSON document from path and delegates to
// LoadExpressionRuntimeConfigFromJsonString(). File-open, parse, and schema
// failures are reported as ExpressionError with ConfigError.
ExpressionRuntimeConfig LoadExpressionRuntimeConfigFromJsonFile(const std::string& path);

} // namespace utility
} // namespace tfp

#endif

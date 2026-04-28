#include "constant_resolver.h"

#include "muparserx_backend.h"
#include "tfp/utility/expression/expression_error.h"

#include <set>

namespace tfp
{
namespace utility
{

std::unordered_map<std::string, double> ResolveConstants(
    const std::unordered_map<std::string, std::string>& raw_constants)
{
    std::unordered_map<std::string, double> resolved;
    std::set<std::string> unresolved;
    for (std::unordered_map<std::string, std::string>::const_iterator it = raw_constants.begin(); it != raw_constants.end(); ++it)
    {
        unresolved.insert(it->first);
    }

    while (!unresolved.empty())
    {
        bool made_progress = false;
        std::vector<std::string> resolved_this_pass;

        for (std::set<std::string>::const_iterator it = unresolved.begin(); it != unresolved.end(); ++it)
        {
            const std::string& name = *it;
            try
            {
                resolved[name] = detail::EvaluateConstantExpression(name, raw_constants.at(name), resolved);
                resolved_this_pass.push_back(name);
                made_progress = true;
            }
            catch (const ExpressionError&)
            {
            }
        }

        for (std::vector<std::string>::const_iterator it = resolved_this_pass.begin(); it != resolved_this_pass.end(); ++it)
        {
            unresolved.erase(*it);
        }

        if (!made_progress)
        {
            throw ExpressionError(ExpressionErrorCode::ConstantError,
                                  "constants contain an unknown symbol or circular dependency near '" + *unresolved.begin() + "'");
        }
    }

    return resolved;
}

} // namespace utility
} // namespace tfp

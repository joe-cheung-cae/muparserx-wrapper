#include "constant_resolver.h"

#include "muparserx_backend.h"
#include "tfp/utility/expression/expression_error.h"

#include <set>

namespace tfp
{
namespace utility
{

std::unordered_map<std::string, double> ResolveConstants(
    const std::unordered_map<std::string, std::string>& raw_constants,
    const std::unordered_map<std::string, double>& seed_constants)
{
    std::unordered_map<std::string, double> resolved = seed_constants;
    std::set<std::string> unresolved;
    for (std::unordered_map<std::string, std::string>::const_iterator it = raw_constants.begin(); it != raw_constants.end(); ++it)
    {
        if (resolved.find(it->first) != resolved.end())
        {
            throw ExpressionError(ExpressionErrorCode::ConstantError,
                                  "duplicate constant '" + it->first + "'");
        }
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
                // A constant can be resolved as soon as every referenced symbol
                // has already been resolved in an earlier pass.
                resolved[name] = detail::EvaluateConstantExpression(name, raw_constants.at(name), resolved);
                resolved_this_pass.push_back(name);
                made_progress = true;
            }
            catch (const ExpressionError&)
            {
                // Failures are deferred until the end of the pass so other
                // constants that no longer have unresolved dependencies can
                // still make progress.
            }
        }

        for (std::vector<std::string>::const_iterator it = resolved_this_pass.begin(); it != resolved_this_pass.end(); ++it)
        {
            unresolved.erase(*it);
        }

        if (!made_progress)
        {
            // If an entire pass cannot resolve anything, the remaining names
            // must contain either an unknown symbol reference or a dependency
            // cycle between constants.
            throw ExpressionError(ExpressionErrorCode::ConstantError,
                                  "constants contain an unknown symbol or circular dependency near '" + *unresolved.begin() + "'");
        }
    }

    return resolved;
}

} // namespace utility
} // namespace tfp

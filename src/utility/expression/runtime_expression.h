#ifndef TFP_UTILITY_EXPRESSION_RUNTIME_EXPRESSION_H
#define TFP_UTILITY_EXPRESSION_RUNTIME_EXPRESSION_H

#include "muparserx_backend.h"
#include "tfp/utility/expression/expression_config.h"

#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

namespace tfp
{
namespace utility
{
namespace detail
{

class RuntimeExpression
{
public:
    RuntimeExpression(const ExpressionConfig& config,
                      const std::unordered_map<std::string, double>& constants,
                      const std::unordered_map<std::string, std::shared_ptr<const TableFunction> >& tables);

    const std::string& Name() const noexcept;
    const std::vector<std::string>& Wordable() const noexcept;
    std::size_t Arity() const noexcept;

    double Evaluate(const std::vector<double>& args);
    double EvaluateUnary(double x);
    double EvaluateMap(const std::unordered_map<std::string, double>& variables);

private:
    std::string name_;
    std::string expression_;
    std::vector<std::string> wordable_;
    std::unordered_map<std::string, std::size_t> variable_indices_;
    MuParserXBackend backend_;
};

} // namespace detail
} // namespace utility
} // namespace tfp

#endif

#ifndef TFP_UTILITY_EXPRESSION_MUPARSERX_BACKEND_H
#define TFP_UTILITY_EXPRESSION_MUPARSERX_BACKEND_H

#include "tfp/utility/expression/table_function.h"

#include "mpParser.h"
#include "mpValue.h"

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

class MuParserXBackend
{
public:
    MuParserXBackend();

    void DefineConstant(const std::string& name, double value);
    void DefineVariables(const std::vector<std::string>& names);
    void SetVariable(std::size_t index, double value);
    void DefineTable(const std::shared_ptr<const TableFunction>& table);
    void SetExpression(const std::string& expression);

    std::vector<std::string> GetReferencedVariables() const;
    double EvalAsDouble() const;

private:
    mup::ParserX parser_;
    std::vector<mup::Value> variable_storage_;
};

double EvaluateConstantExpression(const std::string& constant_name,
                                  const std::string& expression,
                                  const std::unordered_map<std::string, double>& constants);

} // namespace detail
} // namespace utility
} // namespace tfp

#endif

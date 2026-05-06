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

// Thin implementation detail around muparserx that owns parser-bound variable
// storage and converts muparserx errors into ExpressionError. Methods are not
// synchronized; each RuntimeExpression owns the backend it mutates.
class MuParserXBackend
{
public:
    // Creates an empty parser backend with no constants, variables, tables, or
    // expression text registered.
    MuParserXBackend();

    // Defines or replaces a numeric constant in the parser symbol table.
    void DefineConstant(const std::string& name, double value);
    // Creates mutable parser variables in the supplied order. The backing
    // storage remains owned by this backend for the backend lifetime.
    void DefineVariables(const std::vector<std::string>& names);
    // Updates one parser-bound variable by positional index.
    void SetVariable(std::size_t index, double value);
    // Defines or replaces a table function callback. The shared_ptr keeps the
    // immutable TableFunction alive for callback use.
    void DefineTable(const std::shared_ptr<const TableFunction>& table);
    // Sets parser expression text and performs muparserx parsing/validation.
    void SetExpression(const std::string& expression);

    // Returns variable symbols referenced by the current expression. Constants
    // and table callback names are excluded by muparserx.
    std::vector<std::string> GetReferencedVariables() const;
    // Evaluates the current expression and converts the parser value to double.
    double EvalAsDouble() const;

private:
    mup::ParserX parser_;
    std::vector<mup::Value> variable_storage_;
};

// Evaluates a constant expression with only already-resolved constants in
// scope. Unknown symbols, parser errors, and evaluation errors are wrapped as
// ExpressionError so the resolver can classify constant failures consistently.
double EvaluateConstantExpression(const std::string& constant_name,
                                  const std::string& expression,
                                  const std::unordered_map<std::string, double>& constants);

} // namespace detail
} // namespace utility
} // namespace tfp

#endif

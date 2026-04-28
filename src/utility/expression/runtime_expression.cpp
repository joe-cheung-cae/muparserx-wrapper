#include "runtime_expression.h"

#include "tfp/utility/expression/expression_error.h"

#include <set>

namespace tfp
{
namespace utility
{
namespace detail
{

RuntimeExpression::RuntimeExpression(
    const ExpressionConfig& config,
    const std::unordered_map<std::string, double>& constants,
    const std::unordered_map<std::string, std::shared_ptr<const TableFunction> >& tables)
    : name_(config.name),
      expression_(config.expression),
      wordable_(config.wordable)
{
    // Constants and tables are registered before runtime variables so the
    // parser sees the same symbol set during validation and later evaluation.
    for (std::unordered_map<std::string, double>::const_iterator it = constants.begin(); it != constants.end(); ++it)
    {
        backend_.DefineConstant(it->first, it->second);
    }

    for (std::unordered_map<std::string, std::shared_ptr<const TableFunction> >::const_iterator it = tables.begin();
         it != tables.end(); ++it)
    {
        backend_.DefineTable(it->second);
    }

    // wordable defines the ordered runtime-variable contract exposed through
    // ExpressionHandle::Evaluate(std::vector<double>) and EvaluateMap().
    for (std::size_t i = 0; i < wordable_.size(); ++i)
    {
        variable_indices_[wordable_[i]] = i;
    }
    backend_.DefineVariables(wordable_);

    backend_.SetExpression(expression_);

    const std::vector<std::string> referenced_variables = backend_.GetReferencedVariables();
    for (std::vector<std::string>::const_iterator it = referenced_variables.begin(); it != referenced_variables.end(); ++it)
    {
        if (variable_indices_.find(*it) == variable_indices_.end())
        {
            throw ExpressionError(ExpressionErrorCode::CompileError,
                                  "expression '" + name_ + "' references undeclared runtime variable '" + *it + "'");
        }
    }

    try
    {
        // A dry run forces parser-side validation while all symbols are bound,
        // so malformed expressions fail during load instead of on first use.
        backend_.EvalAsDouble();
    }
    catch (const ExpressionError& error)
    {
        throw ExpressionError(ExpressionErrorCode::CompileError,
                              "expression '" + name_ + "' failed to compile: " + error.what());
    }
}

const std::string& RuntimeExpression::Name() const noexcept
{
    return name_;
}

const std::vector<std::string>& RuntimeExpression::Wordable() const noexcept
{
    return wordable_;
}

std::size_t RuntimeExpression::Arity() const noexcept
{
    return wordable_.size();
}

double RuntimeExpression::Evaluate(const std::vector<double>& args)
{
    if (args.size() != wordable_.size())
    {
        throw ExpressionError(ExpressionErrorCode::InvalidArgument,
                              "expression '" + name_ + "' expected " + std::to_string(wordable_.size()) +
                                  " arguments, got " + std::to_string(args.size()));
    }

    for (std::size_t i = 0; i < args.size(); ++i)
    {
        backend_.SetVariable(i, args[i]);
    }

    try
    {
        return backend_.EvalAsDouble();
    }
    catch (const ExpressionError& error)
    {
        throw ExpressionError(ExpressionErrorCode::EvaluationError,
                              "expression '" + name_ + "' evaluation failed: " + error.what());
    }
}

double RuntimeExpression::EvaluateUnary(double x)
{
    if (wordable_.size() != 1)
    {
        throw ExpressionError(ExpressionErrorCode::InvalidArgument,
                              "expression '" + name_ + "' expected 1 argument, got " + std::to_string(wordable_.size()));
    }

    backend_.SetVariable(0, x);
    try
    {
        return backend_.EvalAsDouble();
    }
    catch (const ExpressionError& error)
    {
        throw ExpressionError(ExpressionErrorCode::EvaluationError,
                              "expression '" + name_ + "' evaluation failed: " + error.what());
    }
}

double RuntimeExpression::EvaluateMap(const std::unordered_map<std::string, double>& variables)
{
    // The map-based API preserves the same evaluation semantics as positional
    // handles by translating names into the canonical wordable argument order.
    std::vector<double> args(wordable_.size());
    for (std::size_t i = 0; i < wordable_.size(); ++i)
    {
        std::unordered_map<std::string, double>::const_iterator value = variables.find(wordable_[i]);
        if (value == variables.end())
        {
            throw ExpressionError(ExpressionErrorCode::InvalidArgument,
                                  "expression '" + name_ + "' missing variable '" + wordable_[i] + "'");
        }
        args[i] = value->second;
    }

    return Evaluate(args);
}

} // namespace detail
} // namespace utility
} // namespace tfp

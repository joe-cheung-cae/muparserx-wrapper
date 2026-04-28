#include "muparserx_backend.h"

#include "tfp/utility/expression/expression_error.h"

#include "mpError.h"
#include "mpICallback.h"
#include "mpVariable.h"

#include <sstream>

namespace tfp
{
namespace utility
{
namespace detail
{

mup::ICallback* CreateTableCallback(const std::shared_ptr<const TableFunction>& table);

namespace
{

std::string ToMessage(const mup::ParserError& error)
{
    return error.GetMsg();
}

} // namespace

MuParserXBackend::MuParserXBackend()
    : parser_()
{
}

void MuParserXBackend::DefineConstant(const std::string& name, double value)
{
    try
    {
        if (parser_.IsConstDefined(name))
        {
            parser_.RemoveConst(name);
        }
        parser_.DefineConst(name, mup::Value(value));
    }
    catch (const mup::ParserError& error)
    {
        throw ExpressionError(ExpressionErrorCode::CompileError,
                              "failed to define constant '" + name + "': " + ToMessage(error));
    }
}

void MuParserXBackend::DefineVariables(const std::vector<std::string>& names)
{
    // muparserx variables are bound to mutable Value objects, so the storage
    // vector must stay alive for the full lifetime of the compiled expression.
    variable_storage_.assign(names.size(), mup::Value(0.0));
    for (std::size_t i = 0; i < names.size(); ++i)
    {
        try
        {
            parser_.DefineVar(names[i], mup::Variable(&variable_storage_[i]));
        }
        catch (const mup::ParserError& error)
        {
            throw ExpressionError(ExpressionErrorCode::CompileError,
                                  "failed to define variable '" + names[i] + "': " + ToMessage(error));
        }
    }
}

void MuParserXBackend::SetVariable(std::size_t index, double value)
{
    if (index >= variable_storage_.size())
    {
        throw ExpressionError(ExpressionErrorCode::InvalidArgument, "variable index out of range");
    }
    variable_storage_[index] = value;
}

void MuParserXBackend::DefineTable(const std::shared_ptr<const TableFunction>& table)
{
    try
    {
        if (parser_.IsFunDefined(table->Name()))
        {
            parser_.RemoveFun(table->Name());
        }
        parser_.DefineFun(mup::ptr_cal_type(CreateTableCallback(table)));
    }
    catch (const mup::ParserError& error)
    {
        throw ExpressionError(ExpressionErrorCode::CompileError,
                              "failed to define table function '" + table->Name() + "': " + ToMessage(error));
    }
}

void MuParserXBackend::SetExpression(const std::string& expression)
{
    try
    {
        parser_.SetExpr(expression);
    }
    catch (const mup::ParserError& error)
    {
        throw ExpressionError(ExpressionErrorCode::CompileError, "failed to set expression: " + ToMessage(error));
    }
}

std::vector<std::string> MuParserXBackend::GetReferencedVariables() const
{
    try
    {
        std::vector<std::string> variables;
        // muparserx reports only variable symbols here, so constants and table
        // callbacks can already be registered without appearing in this list.
        const mup::var_maptype& referenced = parser_.GetExprVar();
        for (mup::var_maptype::const_iterator it = referenced.begin(); it != referenced.end(); ++it)
        {
            variables.push_back(it->first);
        }
        return variables;
    }
    catch (const mup::ParserError& error)
    {
        throw ExpressionError(ExpressionErrorCode::CompileError,
                              "failed to inspect expression variables: " + ToMessage(error));
    }
}

double MuParserXBackend::EvalAsDouble() const
{
    try
    {
        return parser_.Eval().GetFloat();
    }
    catch (const ExpressionError&)
    {
        throw;
    }
    catch (const mup::ParserError& error)
    {
        throw ExpressionError(ExpressionErrorCode::EvaluationError, "expression evaluation failed: " + ToMessage(error));
    }
    catch (const std::exception& error)
    {
        throw ExpressionError(ExpressionErrorCode::EvaluationError, std::string("expression evaluation failed: ") + error.what());
    }
}

double EvaluateConstantExpression(const std::string& constant_name,
                                  const std::string& expression,
                                  const std::unordered_map<std::string, double>& constants)
{
    // Constant evaluation uses a fresh backend with only already-resolved
    // constants so runtime variables and table functions are never in scope.
    MuParserXBackend backend;
    for (std::unordered_map<std::string, double>::const_iterator it = constants.begin(); it != constants.end(); ++it)
    {
        backend.DefineConstant(it->first, it->second);
    }
    backend.SetExpression(expression);

    const std::vector<std::string> referenced_variables = backend.GetReferencedVariables();
    if (!referenced_variables.empty())
    {
        throw ExpressionError(ExpressionErrorCode::ConstantError,
                              "constant '" + constant_name + "' references unknown symbol '" + referenced_variables.front() + "'");
    }

    try
    {
        return backend.EvalAsDouble();
    }
    catch (const ExpressionError& error)
    {
        throw ExpressionError(ExpressionErrorCode::ConstantError,
                              "constant '" + constant_name + "' failed to evaluate: " + error.what());
    }
}

} // namespace detail
} // namespace utility
} // namespace tfp

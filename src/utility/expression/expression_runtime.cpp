#include "tfp/utility/expression/expression_runtime.h"

#include "expression_compiler.h"
#include "expression_config_loader.h"
#include "tfp/utility/expression/expression_config.h"
#include "tfp/utility/expression/expression_error.h"

#include <set>
#include <utility>

namespace tfp
{
namespace utility
{
namespace detail
{

class ExpressionRuntimeImpl
{
public:
    void Load(const ExpressionRuntimeConfig& config)
    {
        // All top-level symbol names share one parser namespace, so conflicts
        // must be rejected before any constants, tables, or expressions are
        // compiled into partially initialized runtime state.
        CheckNameConflicts(config);

        // ExpressionRuntimeConfig stores normalized constants, so compilation
        // can register them directly for tables and runtime expressions.
        std::unordered_map<std::string, double> constants = config.constants;

        std::unordered_map<std::string, std::shared_ptr<const TableFunction> > tables;
        std::unordered_map<std::string, std::vector<std::string> > argument_names;
        for (std::vector<TableConfig>::const_iterator it = config.tables.begin(); it != config.tables.end(); ++it)
        {
            tables[it->name] = std::make_shared<TableFunction>(it->name, it->data, it->extrapolation);
            argument_names[it->name] = std::vector<std::string>{"x"};
        }

        std::unordered_map<std::string, std::shared_ptr<RuntimeExpression> > expressions;
        for (std::vector<ExpressionConfig>::const_iterator it = config.expressions.begin(); it != config.expressions.end(); ++it)
        {
            expressions[it->name] = CompileRuntimeExpression(*it, constants, tables);
            argument_names[it->name] = it->wordable;
        }

        for (std::unordered_map<std::string, double>::const_iterator it = config.constants.begin();
             it != config.constants.end(); ++it)
        {
            argument_names[it->first] = std::vector<std::string>();
        }

        constants_ = std::move(constants);
        tables_ = std::move(tables);
        expressions_ = std::move(expressions);
        argument_names_ = std::move(argument_names);
    }

    ExpressionHandle GetExpression(const std::string& name) const
    {
        return ExpressionHandle(FindExpression(name));
    }

    UnaryExpressionHandle GetUnaryExpression(const std::string& name) const
    {
        std::shared_ptr<RuntimeExpression> expression = FindExpression(name);
        // The same compiled expression representation supports both generic and
        // unary handles, so arity is enforced when the specialized handle is
        // requested rather than during the shared compile pipeline.
        if (expression->Arity() != 1)
        {
            throw ExpressionError(ExpressionErrorCode::InvalidArgument,
                                  "expression '" + name + "' is not unary");
        }
        return UnaryExpressionHandle(expression);
    }

    std::vector<std::string> GetArgumentNames(const std::string& name) const
    {
        return FindArgumentNames(name);
    }

    double Evaluate(const std::string& name, const std::unordered_map<std::string, double>& variables) const
    {
        std::unordered_map<std::string, std::shared_ptr<RuntimeExpression> >::const_iterator expression =
            expressions_.find(name);
        if (expression != expressions_.end())
        {
            return expression->second->EvaluateMap(variables);
        }

        std::unordered_map<std::string, std::shared_ptr<const TableFunction> >::const_iterator table =
            tables_.find(name);
        if (table != tables_.end())
        {
            if (variables.empty())
            {
                throw ExpressionError(ExpressionErrorCode::InvalidArgument,
                                      "table '" + name + "' requires variable 'x'");
            }
            if (variables.size() != 1)
            {
                throw ExpressionError(ExpressionErrorCode::InvalidArgument,
                                      "table '" + name + "' expects exactly one variable");
            }

            return table->second->Evaluate(variables.begin()->second);
        }

        std::unordered_map<std::string, double>::const_iterator constant = constants_.find(name);
        if (constant != constants_.end())
        {
            if (!variables.empty())
            {
                throw ExpressionError(ExpressionErrorCode::InvalidArgument,
                                      "constant '" + name + "' expects no variables");
            }
            return constant->second;
        }

        throw ExpressionError(ExpressionErrorCode::NotFound, "runtime item '" + name + "' does not exist");
    }

    double EvaluateUnary(const std::string& name, double x) const
    {
        std::unordered_map<std::string, std::shared_ptr<RuntimeExpression> >::const_iterator expression =
            expressions_.find(name);
        if (expression != expressions_.end())
        {
            if (expression->second->Arity() != 1)
            {
                throw ExpressionError(ExpressionErrorCode::InvalidArgument,
                                      "expression '" + name + "' is not unary");
            }
            return expression->second->EvaluateUnary(x);
        }

        std::unordered_map<std::string, std::shared_ptr<const TableFunction> >::const_iterator table =
            tables_.find(name);
        if (table != tables_.end())
        {
            return table->second->Evaluate(x);
        }

        std::unordered_map<std::string, double>::const_iterator constant = constants_.find(name);
        if (constant != constants_.end())
        {
            return constant->second;
        }

        throw ExpressionError(ExpressionErrorCode::NotFound, "runtime item '" + name + "' does not exist");
    }

private:
    static void CheckNameConflicts(const ExpressionRuntimeConfig& config)
    {
        // Constants, tables, and expressions are all registered into one
        // global symbol namespace, so duplicate names would become ambiguous
        // long before evaluation.
        std::set<std::string> names;
        for (std::unordered_map<std::string, double>::const_iterator it = config.constants.begin();
             it != config.constants.end(); ++it)
        {
            if (it->first.empty())
            {
                throw ExpressionError(ExpressionErrorCode::ConfigError, "runtime item name must not be empty");
            }
            if (!names.insert(it->first).second)
            {
                throw ExpressionError(ExpressionErrorCode::ConfigError, "duplicate global symbol '" + it->first + "'");
            }
        }
        for (std::vector<TableConfig>::const_iterator it = config.tables.begin(); it != config.tables.end(); ++it)
        {
            if (it->name.empty())
            {
                throw ExpressionError(ExpressionErrorCode::ConfigError, "runtime item name must not be empty");
            }
            if (!names.insert(it->name).second)
            {
                throw ExpressionError(ExpressionErrorCode::ConfigError, "duplicate global symbol '" + it->name + "'");
            }
        }
        for (std::vector<ExpressionConfig>::const_iterator it = config.expressions.begin(); it != config.expressions.end(); ++it)
        {
            if (it->name.empty())
            {
                throw ExpressionError(ExpressionErrorCode::ConfigError, "runtime item name must not be empty");
            }
            if (!names.insert(it->name).second)
            {
                throw ExpressionError(ExpressionErrorCode::ConfigError, "duplicate global symbol '" + it->name + "'");
            }

            std::set<std::string> wordable_names;
            for (std::vector<std::string>::const_iterator wordable = it->wordable.begin();
                 wordable != it->wordable.end(); ++wordable)
            {
                if (wordable->empty())
                {
                    throw ExpressionError(ExpressionErrorCode::ConfigError,
                                          "expression '" + it->name + "' wordable variable name must not be empty");
                }
                if (!wordable_names.insert(*wordable).second)
                {
                    throw ExpressionError(ExpressionErrorCode::ConfigError,
                                          "expression '" + it->name + "' has duplicate wordable variable '" + *wordable + "'");
                }
            }
        }
    }

    std::shared_ptr<RuntimeExpression> FindExpression(const std::string& name) const
    {
        std::unordered_map<std::string, std::shared_ptr<RuntimeExpression> >::const_iterator it = expressions_.find(name);
        if (it == expressions_.end())
        {
            throw ExpressionError(ExpressionErrorCode::NotFound, "expression '" + name + "' does not exist");
        }
        return it->second;
    }

    std::vector<std::string> FindArgumentNames(const std::string& name) const
    {
        std::unordered_map<std::string, std::vector<std::string> >::const_iterator it = argument_names_.find(name);
        if (it == argument_names_.end())
        {
            throw ExpressionError(ExpressionErrorCode::NotFound, "item '" + name + "' does not exist");
        }
        return it->second;
    }

    std::unordered_map<std::string, double> constants_;
    std::unordered_map<std::string, std::shared_ptr<const TableFunction> > tables_;
    std::unordered_map<std::string, std::shared_ptr<RuntimeExpression> > expressions_;
    std::unordered_map<std::string, std::vector<std::string> > argument_names_;
};

} // namespace detail

ExpressionRuntime::ExpressionRuntime()
    : impl_(new detail::ExpressionRuntimeImpl())
{
}

ExpressionRuntime::~ExpressionRuntime() = default;
ExpressionRuntime::ExpressionRuntime(ExpressionRuntime&&) noexcept = default;
ExpressionRuntime& ExpressionRuntime::operator=(ExpressionRuntime&&) noexcept = default;

ExpressionRuntime ExpressionRuntime::CreateFromConfig(const ExpressionRuntimeConfig& config)
{
    ExpressionRuntime runtime;
    runtime.LoadFromConfig(config);
    return runtime;
}

ExpressionRuntime ExpressionRuntime::CreateFromJsonString(const std::string& json_text)
{
    ExpressionRuntime runtime;
    runtime.LoadFromJsonString(json_text);
    return runtime;
}

ExpressionRuntime ExpressionRuntime::CreateFromJsonObject(const nlohmann::json& json_object)
{
    ExpressionRuntime runtime;
    runtime.LoadFromJsonObject(json_object);
    return runtime;
}

ExpressionRuntime ExpressionRuntime::CreateFromJsonFile(const std::string& path)
{
    ExpressionRuntime runtime;
    runtime.LoadFromJsonFile(path);
    return runtime;
}

void ExpressionRuntime::LoadFromConfig(const ExpressionRuntimeConfig& config)
{
    impl_->Load(config);
}

void ExpressionRuntime::LoadFromJsonString(const std::string& json_text)
{
    LoadFromConfig(LoadExpressionRuntimeConfigFromJsonString(json_text));
}

void ExpressionRuntime::LoadFromJsonObject(const nlohmann::json& json_object)
{
    LoadFromConfig(LoadExpressionRuntimeConfigFromJsonObject(json_object));
}

void ExpressionRuntime::LoadFromJsonFile(const std::string& path)
{
    LoadFromConfig(LoadExpressionRuntimeConfigFromJsonFile(path));
}

ExpressionHandle ExpressionRuntime::GetExpression(const std::string& name) const
{
    return impl_->GetExpression(name);
}

UnaryExpressionHandle ExpressionRuntime::GetUnaryExpression(const std::string& name) const
{
    return impl_->GetUnaryExpression(name);
}

std::vector<std::string> ExpressionRuntime::GetArgumentNames(const std::string& name) const
{
    return impl_->GetArgumentNames(name);
}

double ExpressionRuntime::Evaluate(const std::string& name, const std::unordered_map<std::string, double>& variables) const
{
    return impl_->Evaluate(name, variables);
}

double ExpressionRuntime::EvaluateUnary(const std::string& name, double x) const
{
    return impl_->EvaluateUnary(name, x);
}

} // namespace utility
} // namespace tfp

#include "tfp/utility/expression/expression_runtime.h"

#include "constant_resolver.h"
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
        CheckNameConflicts(config);

        std::unordered_map<std::string, double> constants = ResolveConstants(config.constants);

        std::unordered_map<std::string, std::shared_ptr<const TableFunction> > tables;
        for (std::vector<TableConfig>::const_iterator it = config.tables.begin(); it != config.tables.end(); ++it)
        {
            tables[it->name] = std::make_shared<TableFunction>(it->name, it->data, it->extrapolation);
        }

        std::unordered_map<std::string, std::shared_ptr<RuntimeExpression> > expressions;
        for (std::vector<ExpressionConfig>::const_iterator it = config.expressions.begin(); it != config.expressions.end(); ++it)
        {
            expressions[it->name] = CompileRuntimeExpression(*it, constants, tables);
        }

        expressions_ = std::move(expressions);
    }

    ExpressionHandle GetExpression(const std::string& name) const
    {
        return ExpressionHandle(FindExpression(name));
    }

    UnaryExpressionHandle GetUnaryExpression(const std::string& name) const
    {
        std::shared_ptr<RuntimeExpression> expression = FindExpression(name);
        if (expression->Arity() != 1)
        {
            throw ExpressionError(ExpressionErrorCode::InvalidArgument,
                                  "expression '" + name + "' is not unary");
        }
        return UnaryExpressionHandle(expression);
    }

    double Evaluate(const std::string& name, const std::unordered_map<std::string, double>& variables) const
    {
        return FindExpression(name)->EvaluateMap(variables);
    }

private:
    static void CheckNameConflicts(const ExpressionRuntimeConfig& config)
    {
        std::set<std::string> names;
        for (std::unordered_map<std::string, std::string>::const_iterator it = config.constants.begin();
             it != config.constants.end(); ++it)
        {
            if (!names.insert(it->first).second)
            {
                throw ExpressionError(ExpressionErrorCode::ConfigError, "duplicate global symbol '" + it->first + "'");
            }
        }
        for (std::vector<TableConfig>::const_iterator it = config.tables.begin(); it != config.tables.end(); ++it)
        {
            if (!names.insert(it->name).second)
            {
                throw ExpressionError(ExpressionErrorCode::ConfigError, "duplicate global symbol '" + it->name + "'");
            }
        }
        for (std::vector<ExpressionConfig>::const_iterator it = config.expressions.begin(); it != config.expressions.end(); ++it)
        {
            if (!names.insert(it->name).second)
            {
                throw ExpressionError(ExpressionErrorCode::ConfigError, "duplicate global symbol '" + it->name + "'");
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

    std::unordered_map<std::string, std::shared_ptr<RuntimeExpression> > expressions_;
};

} // namespace detail

ExpressionRuntime::ExpressionRuntime()
    : impl_(new detail::ExpressionRuntimeImpl())
{
}

ExpressionRuntime::~ExpressionRuntime() = default;
ExpressionRuntime::ExpressionRuntime(ExpressionRuntime&&) noexcept = default;
ExpressionRuntime& ExpressionRuntime::operator=(ExpressionRuntime&&) noexcept = default;

void ExpressionRuntime::LoadFromJsonString(const std::string& json_text)
{
    impl_->Load(LoadExpressionRuntimeConfigFromJsonString(json_text));
}

void ExpressionRuntime::LoadFromJsonFile(const std::string& path)
{
    impl_->Load(LoadExpressionRuntimeConfigFromJsonFile(path));
}

ExpressionHandle ExpressionRuntime::GetExpression(const std::string& name) const
{
    return impl_->GetExpression(name);
}

UnaryExpressionHandle ExpressionRuntime::GetUnaryExpression(const std::string& name) const
{
    return impl_->GetUnaryExpression(name);
}

double ExpressionRuntime::Evaluate(const std::string& name, const std::unordered_map<std::string, double>& variables) const
{
    return impl_->Evaluate(name, variables);
}

} // namespace utility
} // namespace tfp

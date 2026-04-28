#include "expression_config_loader.h"

#include "tfp/utility/expression/expression_error.h"

#include <fstream>
#include <cmath>
#include <nlohmann/json.hpp>
#include <set>
#include <sstream>

namespace tfp
{
namespace utility
{
namespace
{

using Json = nlohmann::json;

ExpressionError ConfigError(const std::string& message)
{
    return ExpressionError(ExpressionErrorCode::ConfigError, message);
}

ExtrapolationMode ParseExtrapolation(const std::string& table_name, const Json& table)
{
    if (!table.contains("extrapolation"))
    {
        return ExtrapolationMode::Clamp;
    }

    if (!table["extrapolation"].is_string())
    {
        throw ConfigError("table '" + table_name + "': extrapolation must be a string");
    }

    const std::string mode = table["extrapolation"].get<std::string>();
    if (mode == "clamp")
    {
        return ExtrapolationMode::Clamp;
    }
    if (mode == "error")
    {
        return ExtrapolationMode::Error;
    }
    if (mode == "linear")
    {
        return ExtrapolationMode::Linear;
    }

    throw ConfigError("table '" + table_name + "': unsupported extrapolation '" + mode + "'");
}

std::vector<std::array<double, 2> > ParseTableData(const std::string& table_name, const Json& table)
{
    if (!table.contains("data"))
    {
        throw ConfigError("table '" + table_name + "': missing data");
    }
    if (!table["data"].is_array())
    {
        throw ConfigError("table '" + table_name + "': data must be an array");
    }

    std::vector<std::array<double, 2> > rows;
    for (std::size_t i = 0; i < table["data"].size(); ++i)
    {
        const Json& row = table["data"][i];
        if (!row.is_array() || row.size() != 2)
        {
            throw ConfigError("table '" + table_name + "': data row " + std::to_string(i) + " must be [x, y]");
        }
        if (!row[0].is_number() || !row[1].is_number())
        {
            throw ConfigError("table '" + table_name + "': data row " + std::to_string(i) + " values must be numeric");
        }

        double x = 0.0;
        double y = 0.0;
        try
        {
            x = row[0].get<double>();
            y = row[1].get<double>();
        }
        catch (const std::exception& error)
        {
            throw ConfigError("table '" + table_name + "': data row " + std::to_string(i) +
                              " values cannot be represented as double: " + error.what());
        }

        if (!std::isfinite(x) || !std::isfinite(y))
        {
            throw ConfigError("table '" + table_name + "': data row " + std::to_string(i) +
                              " contains non-finite value");
        }

        rows.push_back(std::array<double, 2>{{x, y}});
    }
    return rows;
}

void ParseConstants(const Json& root, ExpressionRuntimeConfig& config)
{
    if (!root.contains("constants"))
    {
        return;
    }
    if (!root["constants"].is_object())
    {
        throw ConfigError("constants must be an object");
    }

    for (Json::const_iterator it = root["constants"].begin(); it != root["constants"].end(); ++it)
    {
        if (!it.value().is_string())
        {
            throw ConfigError("constant '" + it.key() + "' must be a string expression");
        }
        config.constants[it.key()] = it.value().get<std::string>();
    }
}

void ParseTables(const Json& root, ExpressionRuntimeConfig& config)
{
    if (!root.contains("tables"))
    {
        return;
    }
    if (!root["tables"].is_object())
    {
        throw ConfigError("tables must be an object");
    }

    for (Json::const_iterator it = root["tables"].begin(); it != root["tables"].end(); ++it)
    {
        if (!it.value().is_object())
        {
            throw ConfigError("table '" + it.key() + "' must be an object");
        }

        TableConfig table;
        table.name = it.key();
        table.extrapolation = ParseExtrapolation(table.name, it.value());
        table.data = ParseTableData(table.name, it.value());
        config.tables.push_back(table);
    }
}

void ParseExpressions(const Json& root, ExpressionRuntimeConfig& config)
{
    if (!root.contains("expressions"))
    {
        return;
    }
    if (!root["expressions"].is_object())
    {
        throw ConfigError("expressions must be an object");
    }

    for (Json::const_iterator it = root["expressions"].begin(); it != root["expressions"].end(); ++it)
    {
        if (!it.value().is_object())
        {
            throw ConfigError("expression '" + it.key() + "' must be an object");
        }
        if (!it.value().contains("expression") || !it.value()["expression"].is_string())
        {
            throw ConfigError("expression '" + it.key() + "': expression must be a string");
        }
        if (!it.value().contains("wordable") || !it.value()["wordable"].is_array())
        {
            throw ConfigError("expression '" + it.key() + "': wordable must be a string array");
        }

        ExpressionConfig expression;
        expression.name = it.key();
        expression.expression = it.value()["expression"].get<std::string>();

        std::set<std::string> seen;
        for (std::size_t i = 0; i < it.value()["wordable"].size(); ++i)
        {
            const Json& variable = it.value()["wordable"][i];
            if (!variable.is_string())
            {
                throw ConfigError("expression '" + it.key() + "': wordable values must be strings");
            }
            const std::string variable_name = variable.get<std::string>();
            if (!seen.insert(variable_name).second)
            {
                throw ConfigError("expression '" + it.key() + "': duplicate wordable variable '" + variable_name + "'");
            }
            expression.wordable.push_back(variable_name);
        }
        config.expressions.push_back(expression);
    }
}

} // namespace

ExpressionRuntimeConfig LoadExpressionRuntimeConfigFromJsonString(const std::string& json_text)
{
    Json root;
    try
    {
        root = Json::parse(json_text);
    }
    catch (const std::exception& error)
    {
        throw ConfigError(std::string("invalid JSON: ") + error.what());
    }

    if (!root.is_object())
    {
        throw ConfigError("root JSON value must be an object");
    }

    ExpressionRuntimeConfig config;
    ParseConstants(root, config);
    ParseTables(root, config);
    ParseExpressions(root, config);
    return config;
}

ExpressionRuntimeConfig LoadExpressionRuntimeConfigFromJsonFile(const std::string& path)
{
    std::ifstream input(path.c_str());
    if (!input)
    {
        throw ConfigError("failed to open JSON file '" + path + "'");
    }

    std::ostringstream buffer;
    buffer << input.rdbuf();
    return LoadExpressionRuntimeConfigFromJsonString(buffer.str());
}

} // namespace utility
} // namespace tfp

#include "expression_config_loader.h"

#include "constant_resolver.h"
#include "tfp/utility/expression/expression_error.h"

#include <fstream>
#include <cmath>
#include <cstdint>
#include <nlohmann/json.hpp>
#include <set>
#include <sstream>
#include <unordered_map>

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

void AddConstant(const std::string& name,
                 const std::string& value,
                 const ExpressionRuntimeConfig& config,
                 std::unordered_map<std::string, std::string>& raw_constants)
{
    if (name.empty())
    {
        throw ConfigError("name must not be empty");
    }
    if (raw_constants.find(name) != raw_constants.end() || config.constants.find(name) != config.constants.end())
    {
        throw ConfigError("duplicate global symbol '" + name + "'");
    }
    raw_constants[name] = value;
}

void AddResolvedConstant(const std::string& name,
                         double value,
                         ExpressionRuntimeConfig& config,
                         const std::unordered_map<std::string, std::string>& raw_constants)
{
    if (name.empty())
    {
        throw ConfigError("name must not be empty");
    }
    if (!std::isfinite(value))
    {
        throw ConfigError("constant '" + name + "': value must be finite");
    }
    if (raw_constants.find(name) != raw_constants.end() || config.constants.find(name) != config.constants.end())
    {
        throw ConfigError("duplicate global symbol '" + name + "'");
    }
    config.constants[name] = value;
}

void RequireNonEmptyName(const std::string& context, const std::string& name)
{
    if (name.empty())
    {
        throw ConfigError(context + ": name must not be empty");
    }
}

ExtrapolationMode ParseExtrapolation(const std::string& table_name, const Json& table)
{
    // Tables default to clamp extrapolation so callers can omit the field when
    // boundary values should simply stick to the nearest endpoint.
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
    // The loader accepts raw JSON rows and normalizes them into the fixed
    // numeric table representation expected by TableFunction.
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

TableConfig ParseTableObject(const std::string& table_name, const Json& table_json)
{
    if (!table_json.is_object())
    {
        throw ConfigError("table '" + table_name + "' must be an object");
    }

    TableConfig table;
    table.name = table_name;
    table.extrapolation = ParseExtrapolation(table.name, table_json);
    table.data = ParseTableData(table.name, table_json);
    return table;
}

void ParseConstants(const Json& root,
                    ExpressionRuntimeConfig& config,
                    std::unordered_map<std::string, std::string>& raw_constants)
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
        AddConstant(it.key(), it.value().get<std::string>(), config, raw_constants);
    }
}

void ParseTables(const Json& root, ExpressionRuntimeConfig& config)
{
    if (!root.contains("tables"))
    {
        return;
    }

    if (root["tables"].is_array())
    {
        for (std::size_t i = 0; i < root["tables"].size(); ++i)
        {
            const Json& table_json = root["tables"][i];
            if (!table_json.is_object())
            {
                throw ConfigError("tables[" + std::to_string(i) + "] must be an object");
            }
            if (!table_json.contains("name") || !table_json["name"].is_string())
            {
                throw ConfigError("tables[" + std::to_string(i) + "]: name must be a string");
            }

            const std::string name = table_json["name"].get<std::string>();
            RequireNonEmptyName("tables[" + std::to_string(i) + "]", name);
            config.tables.push_back(ParseTableObject(name, table_json));
        }
        return;
    }

    if (!root["tables"].is_object())
    {
        throw ConfigError("tables must be an array or object");
    }

    for (Json::const_iterator it = root["tables"].begin(); it != root["tables"].end(); ++it)
    {
        RequireNonEmptyName("table '" + it.key() + "'", it.key());
        config.tables.push_back(ParseTableObject(it.key(), it.value()));
    }
}

ExpressionConfig ParseExpressionObject(const std::string& expression_name, const Json& expression_json)
{
    if (!expression_json.is_object())
    {
        throw ConfigError("expression '" + expression_name + "' must be an object");
    }
    if (!expression_json.contains("expression") || !expression_json["expression"].is_string())
    {
        throw ConfigError("expression '" + expression_name + "': expression must be a string");
    }
    if (!expression_json.contains("wordable") || !expression_json["wordable"].is_array())
    {
        throw ConfigError("expression '" + expression_name + "': wordable must be a string array");
    }

    ExpressionConfig expression;
    expression.name = expression_name;
    expression.expression = expression_json["expression"].get<std::string>();

    std::set<std::string> seen;
    for (std::size_t i = 0; i < expression_json["wordable"].size(); ++i)
    {
        const Json& variable = expression_json["wordable"][i];
        if (!variable.is_string())
        {
            throw ConfigError("expression '" + expression_name + "': wordable values must be strings");
        }
        const std::string variable_name = variable.get<std::string>();
        if (variable_name.empty())
        {
            throw ConfigError("expression '" + expression_name + "': wordable variable name must not be empty");
        }
        if (!seen.insert(variable_name).second)
        {
            throw ConfigError("expression '" + expression_name + "': duplicate wordable variable '" + variable_name + "'");
        }
        expression.wordable.push_back(variable_name);
    }

    return expression;
}

void ParseExpressions(const Json& root, ExpressionRuntimeConfig& config)
{
    if (!root.contains("expressions"))
    {
        return;
    }

    // The preferred representation is an array of named objects, but the
    // loader still accepts the older object form for compatibility.
    if (root["expressions"].is_array())
    {
        for (std::size_t i = 0; i < root["expressions"].size(); ++i)
        {
            const Json& expression_json = root["expressions"][i];
            if (!expression_json.is_object())
            {
                throw ConfigError("expressions[" + std::to_string(i) + "] must be an object");
            }
            if (!expression_json.contains("name") || !expression_json["name"].is_string())
            {
                throw ConfigError("expressions[" + std::to_string(i) + "]: name must be a string");
            }

            const std::string name = expression_json["name"].get<std::string>();
            RequireNonEmptyName("expressions[" + std::to_string(i) + "]", name);
            config.expressions.push_back(ParseExpressionObject(name, expression_json));
        }
        return;
    }

    if (!root["expressions"].is_object())
    {
        throw ConfigError("expressions must be an array or object");
    }

    for (Json::const_iterator it = root["expressions"].begin(); it != root["expressions"].end(); ++it)
    {
        RequireNonEmptyName("expression '" + it.key() + "'", it.key());
        config.expressions.push_back(ParseExpressionObject(it.key(), it.value()));
    }
}

int ParseFunctionType(const Json& function_json, std::size_t index)
{
    if (!function_json.contains("function_type") ||
        (!function_json["function_type"].is_number_integer() && !function_json["function_type"].is_number_unsigned()))
    {
        throw ConfigError("functions[" + std::to_string(index) + "]: function_type must be an integer");
    }

    if (function_json["function_type"].is_number_unsigned())
    {
        const std::uint64_t value = function_json["function_type"].get<std::uint64_t>();
        if (value > 2)
        {
            throw ConfigError("functions[" + std::to_string(index) + "]: function_type unsupported '" +
                              std::to_string(value) + "'");
        }
        return static_cast<int>(value);
    }

    const std::int64_t value = function_json["function_type"].get<std::int64_t>();
    if (value < 0 || value > 2)
    {
        throw ConfigError("functions[" + std::to_string(index) + "]: function_type unsupported '" +
                          std::to_string(value) + "'");
    }
    return static_cast<int>(value);
}

void ParseFunctionDefinition(const Json& function_json,
                             std::size_t index,
                             ExpressionRuntimeConfig& config,
                             std::unordered_map<std::string, std::string>& raw_constants)
{
    if (!function_json.is_object())
    {
        throw ConfigError("functions[" + std::to_string(index) + "] must be an object");
    }
    if (!function_json.contains("name") || !function_json["name"].is_string())
    {
        throw ConfigError("functions[" + std::to_string(index) + "]: name must be a string");
    }
    const std::string name = function_json["name"].get<std::string>();
    RequireNonEmptyName("functions[" + std::to_string(index) + "]", name);
    const int function_type = ParseFunctionType(function_json, index);

    if (function_type == 0)
    {
        if (!function_json.contains("value") || (!function_json["value"].is_string() && !function_json["value"].is_number()))
        {
            throw ConfigError("function '" + name + "': value must be a string expression or number");
        }
        if (function_json["value"].is_number())
        {
            AddResolvedConstant(name, function_json["value"].get<double>(), config, raw_constants);
            return;
        }
        AddConstant(name, function_json["value"].get<std::string>(), config, raw_constants);
        return;
    }

    if (function_type == 1)
    {
        config.tables.push_back(ParseTableObject(name, function_json));
        return;
    }

    if (function_type == 2)
    {
        config.expressions.push_back(ParseExpressionObject(name, function_json));
        return;
    }

    throw ConfigError("functions[" + std::to_string(index) + "]: function_type unsupported '" +
                      std::to_string(function_type) + "'");
}

void ParseFunctions(const Json& root,
                    ExpressionRuntimeConfig& config,
                    std::unordered_map<std::string, std::string>& raw_constants)
{
    if (!root.contains("functions"))
    {
        return;
    }
    if (!root["functions"].is_array())
    {
        throw ConfigError("functions must be an array");
    }

    for (std::size_t i = 0; i < root["functions"].size(); ++i)
    {
        ParseFunctionDefinition(root["functions"][i], i, config, raw_constants);
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

    return LoadExpressionRuntimeConfigFromJsonObject(root);
}

ExpressionRuntimeConfig LoadExpressionRuntimeConfigFromJsonObject(const nlohmann::json& json_object)
{
    if (!json_object.is_object())
    {
        throw ConfigError("root JSON value must be an object");
    }

    // The loader normalizes each top-level section into one internal config
    // object so later compilation stages can stay independent from JSON types.
    ExpressionRuntimeConfig config;
    std::unordered_map<std::string, std::string> raw_constants;
    try
    {
        ParseConstants(json_object, config, raw_constants);
        ParseTables(json_object, config);
        ParseExpressions(json_object, config);
        ParseFunctions(json_object, config, raw_constants);
        config.constants = ResolveConstants(raw_constants, config.constants);
    }
    catch (const ExpressionError&)
    {
        throw;
    }
    catch (const nlohmann::json::exception& error)
    {
        throw ConfigError(std::string("invalid JSON config value: ") + error.what());
    }
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

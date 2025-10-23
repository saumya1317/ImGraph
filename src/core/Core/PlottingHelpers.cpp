#include "PlottingHelpers.hpp"
#include <cmath>

namespace App {

void PlottingHelpers::plotCartesian(const Expression& expr, ImDrawList* draw_list, 
                                   const ImVec2& origin, float zoom, 
                                   const ImVec2& canvas_p0, const ImVec2& canvas_sz) {
    double x;
    
    exprtk::symbol_table<double> symbolTable;
    symbolTable.add_constants();
    addConstants(symbolTable);
    symbolTable.add_variable("x", x);
    
    exprtk::expression<double> expression;
    expression.register_symbol_table(symbolTable);
    
    exprtk::parser<double> parser;
    if (!parser.compile(expr.function, expression)) {
        return; // Failed to compile expression
    }
    
    std::vector<ImVec2> points;
    const float lineThickness = 6.0f;
    
    for (x = -canvas_sz.x / (2 * zoom); x < canvas_sz.x / (2 * zoom); x += 0.05) {
        const double y = expression.value();
        
        ImVec2 screen_pos(origin.x + x * zoom, origin.y - y * zoom);
        points.push_back(screen_pos);
    }
    
    if (!points.empty()) {
        draw_list->AddPolyline(points.data(), points.size(),
                             colorToImU32(expr.color), ImDrawFlags_None, lineThickness);
    }
}

void PlottingHelpers::plotPolar(const Expression& expr, ImDrawList* draw_list,
                               const ImVec2& origin, float zoom) {
    double theta;
    
    exprtk::symbol_table<double> symbolTable;
    symbolTable.add_constants();
    addConstants(symbolTable);
    symbolTable.add_variable("theta", theta);
    
    exprtk::expression<double> expression;
    expression.register_symbol_table(symbolTable);
    
    std::string polar_function = extractPolarFunction(expr.function);
    if (polar_function.empty()) {
        return;
    }
    
    exprtk::parser<double> parser;
    if (!parser.compile(polar_function, expression)) {
        return; // Failed to compile expression
    }
    
    std::vector<ImVec2> points;
    const float lineThickness = 6.0f;
    
    const double theta_min = 0.0;
    const double theta_max = 4.0 * M_PI;
    const double theta_step = 0.02;
    
    for (theta = theta_min; theta <= theta_max; theta += theta_step) {
        const double r = expression.value();
        
        const double x = r * cos(theta);
        const double y = r * sin(theta);
        
        ImVec2 screen_pos(origin.x + static_cast<float>(x * zoom),
                         origin.y - static_cast<float>(y * zoom));
        points.push_back(screen_pos);
    }
    
    if (!points.empty()) {
        draw_list->AddPolyline(points.data(), points.size(),
                             colorToImU32(expr.color), ImDrawFlags_None, lineThickness);
    }
}

void PlottingHelpers::plotParametric(const Expression& expr, ImDrawList* draw_list,
                                    const ImVec2& origin, float zoom) {
    auto [fx, gx] = extractParametricFunctions(expr.function);
    if (fx.empty() || gx.empty()) {
        return;
    }
    
    double t = 0.0;
    exprtk::symbol_table<double> sym_t;
    sym_t.add_constants();
    addConstants(sym_t);
    sym_t.add_variable("t", t);
    
    exprtk::expression<double> expr_fx;
    expr_fx.register_symbol_table(sym_t);
    exprtk::expression<double> expr_gx;
    expr_gx.register_symbol_table(sym_t);
    
    exprtk::parser<double> parser;
    bool ok_fx = parser.compile(fx, expr_fx);
    bool ok_gx = parser.compile(gx, expr_gx);
    
    if (!ok_fx || !ok_gx) {
        return; // Failed to compile expressions
    }
    
    std::vector<ImVec2> points;
    const float lineThickness = 6.0f;
    
    const double t_min = -10.0;
    const double t_max = 10.0;
    const double t_step = 0.02;
    
    for (t = t_min; t <= t_max; t += t_step) {
        const double vx = expr_fx.value();
        const double vy = expr_gx.value();
        
        ImVec2 screen_pos(origin.x + static_cast<float>(vx * zoom),
                         origin.y - static_cast<float>(vy * zoom));
        points.push_back(screen_pos);
    }
    
    if (!points.empty()) {
        draw_list->AddPolyline(points.data(), points.size(),
                             colorToImU32(expr.color), ImDrawFlags_None, lineThickness);
    }
}

ImU32 PlottingHelpers::colorToImU32(const ImVec4& color) {
    return IM_COL32(
        static_cast<int>(color.x * 255),
        static_cast<int>(color.y * 255),
        static_cast<int>(color.z * 255),
        static_cast<int>(color.w * 255)
    );
}

ExpressionType PlottingHelpers::detectExpressionType(const std::string& function) {
    if (function.find("r=") != std::string::npos || function.find("r =") != std::string::npos) {
        return ExpressionType::POLAR;
    }
    
    if (!function.empty() && function.front() == '(' && function.back() == ')') {
        return ExpressionType::PARAMETRIC;
    }
    
    return ExpressionType::CARTESIAN;
}

std::string PlottingHelpers::extractPolarFunction(const std::string& func_str) {
    size_t eq_pos = func_str.find("r=");
    if (eq_pos == std::string::npos) {
        eq_pos = func_str.find("r =");
    }
    
    if (eq_pos != std::string::npos) {
        size_t start_pos = func_str.find("=", eq_pos) + 1;
        std::string polar_function = func_str.substr(start_pos);
        polar_function.erase(0, polar_function.find_first_not_of(" \t"));
        return polar_function;
    }
    
    return "";
}

std::pair<std::string, std::string> PlottingHelpers::extractParametricFunctions(const std::string& func_str) {
    if (func_str.empty() || func_str.front() != '(' || func_str.back() != ')') {
        return {"", ""};
    }
    
    const std::string inner = func_str.substr(1, func_str.size() - 2);
    int depth = 0;
    size_t split_pos = std::string::npos;
    
    for (size_t i = 0; i < inner.size(); ++i) {
        char c = inner[i];
        if (c == '(') {
            ++depth;
        } else if (c == ')') {
            --depth;
        } else if (c == ',' && depth == 0) {
            split_pos = i;
            break;
        }
    }
    
    if (split_pos != std::string::npos) {
        std::string fx = trim(inner.substr(0, split_pos));
        std::string gx = trim(inner.substr(split_pos + 1));
        return {fx, gx};
    }
    
    return {"", ""};
}

} // namespace App

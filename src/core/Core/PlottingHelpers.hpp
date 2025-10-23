#pragma once

#include "Expression.hpp"
#include "funcs.hpp"
#include "exprtk.hpp"
#include <imgui.h>
#include <vector>

namespace App {

class PlottingHelpers {
public:
    static void plotCartesian(const Expression& expr, ImDrawList* draw_list, 
                             const ImVec2& origin, float zoom, 
                             const ImVec2& canvas_p0, const ImVec2& canvas_sz);
    
    static void plotPolar(const Expression& expr, ImDrawList* draw_list,
                         const ImVec2& origin, float zoom);
    
    static void plotParametric(const Expression& expr, ImDrawList* draw_list,
                              const ImVec2& origin, float zoom);
    
    static ImU32 colorToImU32(const ImVec4& color);
    static ExpressionType detectExpressionType(const std::string& function);
    static std::string extractPolarFunction(const std::string& func_str);
    static std::pair<std::string, std::string> extractParametricFunctions(const std::string& func_str);
};

} // namespace App

#pragma once

#include <string>
#include <vector>
#include <imgui.h>

namespace App {

enum class ExpressionType {
    CARTESIAN,  // y = f(x)
    POLAR,      // r = f(θ)
    PARAMETRIC  // (f(t), g(t))
};

struct Expression {
    std::string function;
    ExpressionType type;
    ImVec4 color;
    bool enabled;
    std::string name;
    
    Expression() : type(ExpressionType::CARTESIAN), color(ImVec4(1.0f, 0.0f, 0.0f, 1.0f)), enabled(true), name("Expression") {}
    
    Expression(const std::string& func, ExpressionType t, const ImVec4& col, const std::string& n = "Expression")
        : function(func), type(t), color(col), enabled(true), name(n) {}
};

class ExpressionManager {
public:
    ExpressionManager();
    ~ExpressionManager() = default;
    
    void addExpression(const Expression& expr);
    void removeExpression(size_t index);
    void updateExpression(size_t index, const Expression& expr);
    Expression& getExpression(size_t index);
    const Expression& getExpression(size_t index) const;
    size_t getExpressionCount() const;
    void clearExpressions();
    
    // Default colors for new expressions
    static ImVec4 getDefaultColor(size_t index);
    
private:
    std::vector<Expression> m_expressions;
    static const std::vector<ImVec4> s_defaultColors;
};

} // namespace App

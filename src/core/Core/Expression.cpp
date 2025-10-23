#include "Expression.hpp"

namespace App {

const std::vector<ImVec4> ExpressionManager::s_defaultColors = {
    ImVec4(1.0f, 0.0f, 0.0f, 1.0f),     // Red
    ImVec4(0.0f, 0.0f, 1.0f, 1.0f),     // Blue
    ImVec4(0.0f, 0.8f, 0.0f, 1.0f),     // Green
    ImVec4(1.0f, 0.0f, 1.0f, 1.0f),     // Magenta
    ImVec4(1.0f, 0.5f, 0.0f, 1.0f),     // Orange
    ImVec4(0.0f, 0.8f, 0.8f, 1.0f),     // Cyan
    ImVec4(0.5f, 0.0f, 0.5f, 1.0f),     // Purple
    ImVec4(0.8f, 0.8f, 0.0f, 1.0f),     // Yellow
    ImVec4(0.8f, 0.4f, 0.4f, 1.0f),     // Pink
    ImVec4(0.4f, 0.4f, 0.4f, 1.0f)      // Gray
};

ExpressionManager::ExpressionManager() {
    // Add a default expression
    addExpression(Expression("sin(x)", ExpressionType::CARTESIAN, s_defaultColors[0], "sin(x)"));
}

void ExpressionManager::addExpression(const Expression& expr) {
    m_expressions.push_back(expr);
}

void ExpressionManager::removeExpression(size_t index) {
    if (index < m_expressions.size()) {
        m_expressions.erase(m_expressions.begin() + index);
    }
}

void ExpressionManager::updateExpression(size_t index, const Expression& expr) {
    if (index < m_expressions.size()) {
        m_expressions[index] = expr;
    }
}

Expression& ExpressionManager::getExpression(size_t index) {
    return m_expressions[index];
}

const Expression& ExpressionManager::getExpression(size_t index) const {
    return m_expressions[index];
}

size_t ExpressionManager::getExpressionCount() const {
    return m_expressions.size();
}

void ExpressionManager::clearExpressions() {
    m_expressions.clear();
}

ImVec4 ExpressionManager::getDefaultColor(size_t index) {
    return s_defaultColors[index % s_defaultColors.size()];
}

} // namespace App

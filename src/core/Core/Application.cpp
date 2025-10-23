#include "Application.hpp"

#include <SDL2/SDL.h>
#include <backends/imgui_impl_sdl2.h>
#include <backends/imgui_impl_sdlrenderer2.h>
#include <imgui.h>

#include <cmath>
#include <memory>
#include <string>
#include <vector>

#include "Core/DPIHandler.hpp"
#include "Core/Debug/Instrumentor.hpp"
#include "Core/Log.hpp"
#include "Core/Resources.hpp"
#include "Core/Window.hpp"
#include "Core/PlottingHelpers.hpp"
#include "Settings/Project.hpp"
#include "exprtk.hpp"
#include "funcs.hpp"

namespace App {

Application::Application(const std::string& title) {
  APP_PROFILE_FUNCTION();

  const unsigned int init_flags{SDL_INIT_VIDEO | SDL_INIT_TIMER | SDL_INIT_GAMECONTROLLER};
  if (SDL_Init(init_flags) != 0) {
    APP_ERROR("Error: %s\n", SDL_GetError());
    m_exit_status = ExitStatus::FAILURE;
  }

  m_window = std::make_unique<Window>(Window::Settings{title});
}

Application::~Application() {
  APP_PROFILE_FUNCTION();

  ImGui_ImplSDLRenderer2_Shutdown();
  ImGui_ImplSDL2_Shutdown();
  ImGui::DestroyContext();

  SDL_Quit();
}

ExitStatus App::Application::run() {
  APP_PROFILE_FUNCTION();

  if (m_exit_status == ExitStatus::FAILURE) {
    return m_exit_status;
  }

  // Setup Dear ImGui context
  IMGUI_CHECKVERSION();
  ImGui::CreateContext();
  ImGuiIO& io{ImGui::GetIO()};

  io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard | ImGuiConfigFlags_DockingEnable |
                    ImGuiConfigFlags_ViewportsEnable;

  const std::string user_config_path{SDL_GetPrefPath(COMPANY_NAMESPACE.c_str(), APP_NAME.c_str())};
  APP_DEBUG("User config path: {}", user_config_path);

  // Absolute imgui.ini path to preserve settings independent of app location.
  static const std::string imgui_ini_filename{user_config_path + "imgui.ini"};
  io.IniFilename = imgui_ini_filename.c_str();

  // ImGUI font
  const float font_scaling_factor{DPIHandler::get_scale()};
  const float font_size{18.0F * font_scaling_factor};
  const std::string font_path{Resources::font_path("Manrope.ttf").generic_string()};

  if (Resources::exists(font_path)) {
    io.Fonts->AddFontFromFileTTF(font_path.c_str(), font_size);
    io.FontDefault = io.Fonts->AddFontFromFileTTF(font_path.c_str(), font_size);
  } else {
    APP_WARN("Could not find font file under: {}", font_path.c_str());
  }

  DPIHandler::set_global_font_scaling(&io);

  // Setup Platform/Renderer backends
  ImGui_ImplSDL2_InitForSDLRenderer(m_window->get_native_window(), m_window->get_native_renderer());
  ImGui_ImplSDLRenderer2_Init(m_window->get_native_renderer());

  m_running = true;
  while (m_running) {
    APP_PROFILE_SCOPE("MainLoop");

    SDL_Event event{};
    while (SDL_PollEvent(&event) == 1) {
      APP_PROFILE_SCOPE("EventPolling");

      ImGui_ImplSDL2_ProcessEvent(&event);

      if (event.type == SDL_QUIT) {
        stop();
      }

      if (event.type == SDL_WINDOWEVENT &&
          event.window.windowID == SDL_GetWindowID(m_window->get_native_window())) {
        on_event(event.window);
      }
    }

    // Start the Dear ImGui frame
    ImGui_ImplSDLRenderer2_NewFrame();
    ImGui_ImplSDL2_NewFrame();
    ImGui::NewFrame();

    if (!m_minimized) {
      const ImGuiViewport* viewport = ImGui::GetMainViewport();
      const ImVec2 base_pos = viewport->Pos;
      const ImVec2 base_size = viewport->Size;

      static float zoom = 100.0f;
      static char newExpression[1024] = "sin(x)";
      static int selectedExpressionType = 0;
      const char* expressionTypes[] = {"Cartesian (y=f(x))", "Polar (r=f(θ))", "Parametric ((f(t),g(t)))"};

      // Left Pane (expressions)
      {
        ImGui::SetNextWindowPos(base_pos);
        ImGui::SetNextWindowSize(ImVec2(base_size.x * 0.25f, base_size.y));
        ImGui::Begin("Expressions", nullptr, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoTitleBar);
        
        // Add new expression
        ImGui::Text("Add New Expression:");
        ImGui::InputTextMultiline("##new_expr", newExpression, sizeof(newExpression), ImVec2(-FLT_MIN, ImGui::GetTextLineHeight() * 2));
        ImGui::Combo("Type", &selectedExpressionType, expressionTypes, 3);
        
        if (ImGui::Button("Add Expression")) {
          ExpressionType type = static_cast<ExpressionType>(selectedExpressionType);
          ImVec4 color = ExpressionManager::getDefaultColor(m_expressionManager.getExpressionCount());
          m_expressionManager.addExpression(Expression(newExpression, type, color, "Expression " + std::to_string(m_expressionManager.getExpressionCount() + 1)));
          strcpy(newExpression, ""); // Clear the input
        }
        
        ImGui::Separator();
        ImGui::Text("Expressions:");
        
        // List existing expressions
        for (size_t i = 0; i < m_expressionManager.getExpressionCount(); ++i) {
          Expression& expr = m_expressionManager.getExpression(i);
          
          ImGui::PushID(static_cast<int>(i));
          
          // Enable/disable checkbox
          ImGui::Checkbox("##enabled", &expr.enabled);
          ImGui::SameLine();
          
          // Color picker
          ImGui::ColorEdit4("##color", &expr.color.x, ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_NoLabel);
          ImGui::SameLine();
          
          // Expression name and function
          ImGui::Text("%s: %s", expr.name.c_str(), expr.function.c_str());
          
          // Remove button
          ImGui::SameLine();
          if (ImGui::Button("Remove")) {
            m_expressionManager.removeExpression(i);
            --i; // Adjust index since we removed an element
          }
          
          ImGui::PopID();
        }
        
        ImGui::Separator();
        ImGui::SliderFloat("Graph Scale", &zoom, 10.0f, 500.0f, "%.1f");
        ImGui::End();
      }

      // Right Pane (Graphing Area)
      {
        ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(1.0f, 1.0f, 1.0f, 1.0f));
        ImGui::SetNextWindowPos(ImVec2(base_pos.x + base_size.x * 0.25f, base_pos.y));
        ImGui::SetNextWindowSize(ImVec2(base_size.x * 0.75f, base_size.y));
        ImGui::Begin("Right Pane", nullptr, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoTitleBar);
        ImDrawList* draw_list = ImGui::GetWindowDrawList();
        const ImVec2 canvas_p0 = ImGui::GetCursorScreenPos();
        const ImVec2 canvas_sz = ImGui::GetContentRegionAvail();
        const auto canvas_p1 = ImVec2(canvas_p0.x + canvas_sz.x, canvas_p0.y + canvas_sz.y);
        const ImVec2 origin(canvas_p0.x + canvas_sz.x * 0.5f, canvas_p0.y + canvas_sz.y * 0.5f);
        const float lineThickness = 6.0f;
        draw_list->AddLine(ImVec2(canvas_p0.x, origin.y), ImVec2(canvas_p1.x, origin.y), IM_COL32(0, 0, 0, 255), lineThickness);
        draw_list->AddLine(ImVec2(origin.x, canvas_p0.y), ImVec2(origin.x, canvas_p1.y), IM_COL32(0, 0, 0, 255), lineThickness);

        // Plot all enabled expressions
        for (size_t i = 0; i < m_expressionManager.getExpressionCount(); ++i) {
          const Expression& expr = m_expressionManager.getExpression(i);
          
          if (!expr.enabled) {
            continue; // Skip disabled expressions
          }
          
          // Auto-detect expression type if not set correctly
          ExpressionType detectedType = PlottingHelpers::detectExpressionType(expr.function);
          if (detectedType != expr.type) {
            // Update the expression type
            const_cast<Expression&>(expr).type = detectedType;
          }
          
          // Plot based on expression type
          switch (expr.type) {
            case ExpressionType::CARTESIAN:
              PlottingHelpers::plotCartesian(expr, draw_list, origin, zoom, canvas_p0, canvas_sz);
              break;
            case ExpressionType::POLAR:
              PlottingHelpers::plotPolar(expr, draw_list, origin, zoom);
              break;
            case ExpressionType::PARAMETRIC:
              PlottingHelpers::plotParametric(expr, draw_list, origin, zoom);
              break;
          }
        }


        ImGui::End();
        ImGui::PopStyleColor();
      }
    }

    // Rendering
    ImGui::Render();

    SDL_RenderSetScale(m_window->get_native_renderer(),
        io.DisplayFramebufferScale.x,
        io.DisplayFramebufferScale.y);
    SDL_SetRenderDrawColor(m_window->get_native_renderer(), 100, 100, 100, 255);
    SDL_RenderClear(m_window->get_native_renderer());
    ImGui_ImplSDLRenderer2_RenderDrawData(ImGui::GetDrawData(), m_window->get_native_renderer());
    SDL_RenderPresent(m_window->get_native_renderer());
  }

  return m_exit_status;
}

void App::Application::stop() {
  APP_PROFILE_FUNCTION();

  m_running = false;
}

void Application::on_event(const SDL_WindowEvent& event) {
  APP_PROFILE_FUNCTION();

  switch (event.event) {
    case SDL_WINDOWEVENT_CLOSE:
      return on_close();
    case SDL_WINDOWEVENT_MINIMIZED:
      return on_minimize();
    case SDL_WINDOWEVENT_SHOWN:
      return on_shown();
    default:
      // Do nothing otherwise
      return;
  }
}

void Application::on_minimize() {
  APP_PROFILE_FUNCTION();

  m_minimized = true;
}

void Application::on_shown() {
  APP_PROFILE_FUNCTION();

  m_minimized = false;
}

void Application::on_close() {
  APP_PROFILE_FUNCTION();

  stop();
}

}  // namespace App

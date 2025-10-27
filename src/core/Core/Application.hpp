#pragma once

#include <SDL2/SDL.h>

#include <memory>
#include <string>
#include <vector>

#include "Core/Window.hpp"

namespace App {

enum class ExitStatus : int { SUCCESS = 0, FAILURE = 1 };

class Application {
 public:
  explicit Application(const std::string& title);
  ~Application();

  Application(const Application&) = delete;
  Application(Application&&) = delete;
  Application& operator=(Application other) = delete;
  Application& operator=(Application&& other) = delete;

  ExitStatus run();
  void stop();

  void on_event(const SDL_WindowEvent& event);
  void on_minimize();
  void on_shown();
  void on_close();

 private:
  struct Expression {
    char text[1024];
    float color[3];
  };

  ExitStatus m_exit_status{ExitStatus::SUCCESS};
  std::unique_ptr<Window> m_window{nullptr};

  bool m_running{true};
  bool m_minimized{false};
  bool m_show_some_panel{true};
  bool m_show_debug_panel{false};
  bool m_show_demo_panel{false};
  
  std::vector<Expression> m_expressions{{{"r = 1 + 0.5*cos(theta)"}, {0.25f, 0.5f, 0.78f}}};
};

}  // namespace App

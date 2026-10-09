#include "log.hpp"
#include "serial_interface.hpp"

#include <GLFW/glfw3.h>
#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>

#include <array>
#include <chrono>
#include <cstddef>
#include <exception>
#include <future>
#include <iostream>
#include <string>

static void glfw_error_callback(int error, const char *desc) {
  std::cerr << "glfw error: " << error << " " << desc << "\n";
}

int main() {
  Serial::Log log("main");
  Serial::Port port;

  std::future<void> connection_task,
      stepper_task; // Prevent a port from being destroyed while connecting
  std::string connection_status = "Not Connected";

  glfwSetErrorCallback(glfw_error_callback);
  if (!glfwInit()) {
    std::cerr << "glfw failed to initialize\n";
    return 1;
  }

  log.add_entry("glfw initialized");

  /* IMGUI setup
   * (https://github.com/ocornut/imgui/blob/master/examples/example_glfw_opengl3/main.cpp)
   */
  // Select GL version + let the backend select a GLSL version
  const char *glsl_version = nullptr;
#if defined(IMGUI_IMPL_OPENGL_ES2)
  // GL ES 2.0 + GLSL 100 (WebGL 1.0)
  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 2);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);
  glfwWindowHint(GLFW_CLIENT_API, GLFW_OPENGL_ES_API);
#elif defined(IMGUI_IMPL_OPENGL_ES3)
  // GL ES 3.0 + GLSL 300 es (WebGL 2.0)
  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);
  glfwWindowHint(GLFW_CLIENT_API, GLFW_OPENGL_ES_API);
#elif defined(__APPLE__)
  // GL 3.2 + generally GLSL 150
  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 2);
  glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE); // 3.2+ only
  glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE); // Required on Mac
#else
  // GL 3.0 + generally GLSL 130
  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);
  // glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);  // 3.2+
  // only glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE); // 3.0+ only
#endif

  float main_scale =
      ImGui_ImplGlfw_GetContentScaleForMonitor(glfwGetPrimaryMonitor());
  GLFWwindow *window =
      glfwCreateWindow((int)(1280 * main_scale), (int)(800 * main_scale),
                       "Telescope Tracker", nullptr, nullptr);
  if (window == nullptr)
    return 1;
  glfwMakeContextCurrent(window);
  glfwSwapInterval(1); // Enable vsync

  IMGUI_CHECKVERSION();
  ImGui::CreateContext();
  ImGuiIO &io = ImGui::GetIO();
  (void)io;
  io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;

  ImGui::StyleColorsDark();

  ImGuiStyle &style = ImGui::GetStyle();
  style.ScaleAllSizes(
      main_scale); // Bake a fixed style scale. (until we have a solution for
                   // dynamic style scaling, changing this requires resetting
                   // Style + calling this again)
  style.FontScaleDpi =
      main_scale; // Set initial font scale. (in docking branch: using
                  // io.ConfigDpiScaleFonts=true automatically overrides this
                  // for every window depending on the current monitor)

  // Setup Platform/Renderer backends
  ImGui_ImplGlfw_InitForOpenGL(window, true);
  ImGui_ImplOpenGL3_Init(glsl_version);

  // Load Fonts
  // - If fonts are not explicitly loaded, Dear ImGui will select an embedded
  // font: either AddFontDefaultVector() or AddFontDefaultBitmap().
  //   This selection is based on (style.FontSizeBase * style.FontScaleMain *
  //   style.FontScaleDpi) reaching a small threshold.
  // - You can load multiple fonts and use ImGui::PushFont()/PopFont() to select
  // them.
  // - If a file cannot be loaded, AddFont functions will return a nullptr.
  // Please handle those errors in your code (e.g. use an assertion, display an
  // error and quit).
  // - Read 'docs/FONTS.md' for more instructions and details.
  // - Use '#define IMGUI_ENABLE_FREETYPE' in your imconfig file to use FreeType
  // for higher quality font rendering.
  // - Remember that in C/C++ if you want to include a backslash \ in a string
  // literal you need to write a double backslash \\ !
  // - Our Emscripten build process allows embedding fonts to be accessible at
  // runtime from the "fonts/" folder. See Makefile.emscripten for details.
  // style.FontSizeBase = 20.0f;
  // io.Fonts->AddFontDefaultVector();
  // io.Fonts->AddFontDefaultBitmap();
  // io.Fonts->AddFontFromFileTTF("c:\\Windows\\Fonts\\segoeui.ttf");
  // io.Fonts->AddFontFromFileTTF("../../misc/fonts/DroidSans.ttf");
  // io.Fonts->AddFontFromFileTTF("../../misc/fonts/Roboto-Medium.ttf");
  // io.Fonts->AddFontFromFileTTF("../../misc/fonts/Cousine-Regular.ttf");
  // ImFont* font =
  // io.Fonts->AddFontFromFileTTF("c:\\Windows\\Fonts\\ArialUni.ttf");
  // IM_ASSERT(font != nullptr);

  // Our state
  bool show_demo_window = true;
  bool show_another_window = false;
  ImVec4 clear_color = ImVec4(0.0f, 0.0f, 0.0f, 1.00f);

  log.add_entry("imgui initialized");

  while (!glfwWindowShouldClose(window)) {
    if (connection_task.valid() &&
        connection_task.wait_for(std::chrono::milliseconds(0)) ==
            std::future_status::ready) {
      try {
        connection_task.get();
        connection_status = port.is_ok()
                                ? "Connected to " + port.get_name()
                                : "Connection failed (status " +
                                      std::to_string(port.get_status()) + ")";
      } catch (const std::exception &err) {
        connection_status = "Connection failed";
        log.add_entry("port initialization failed: " + std::string(err.what()));
      }
    }

    if (stepper_task.valid() && stepper_task.wait_for(std::chrono::milliseconds(
                                    0)) == std::future_status::ready) {
      try {
        stepper_task.get();
      } catch (const std::exception &err) {
        log.add_entry("stepper command failed: " + std::string(err.what()));
      }
    }

    // Poll and handle events (inputs, window resize, etc.)
    // You can read the io.WantCaptureMouse, io.WantCaptureKeyboard flags to
    // tell if dear imgui wants to use your inputs.
    // - When io.WantCaptureMouse is true, do not dispatch mouse input data to
    // your main application, or clear/overwrite your copy of the mouse data.
    // - When io.WantCaptureKeyboard is true, do not dispatch keyboard input
    // data to your main application, or clear/overwrite your copy of the
    // keyboard data. Generally you may always pass all inputs to dear imgui,
    // and hide them from your application based on those two flags.
    glfwPollEvents();
    if (glfwGetWindowAttrib(window, GLFW_ICONIFIED) != 0) {
      ImGui_ImplGlfw_Sleep(10);
      continue;
    }

    // Start the Dear ImGui frame
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();

    // Docking
    const ImGuiID dockspace = ImGui::DockSpaceOverViewport();

    // Log panel
    {
      ImGui::SetNextWindowDockID(dockspace, ImGuiCond_FirstUseEver);
      ImGui::Begin("Log");
      if (ImGui::Button("Clear"))
        log.clear();

      ImGui::Separator();
      ImGui::BeginChild("Log Output", ImVec2(0, 0), 0,
                        ImGuiWindowFlags_HorizontalScrollbar);

      const std::string text = log.text();
      ImGui::TextUnformatted(text.data(), text.data() + text.size());
      ImGui::EndChild();
      ImGui::End();
    }

    // Connection panel
    {
      static Serial::port_list_t ports = {};
      static int selection = 0;
      static bool ports_loaded = false;

      ImGui::SetNextWindowDockID(dockspace, ImGuiCond_FirstUseEver);
      ImGui::Begin("Connection");

      ImGui::BeginDisabled(connection_task.valid() || stepper_task.valid());
      const bool refresh_ports = ImGui::Button("Refresh Ports");
      if (!ports_loaded || refresh_ports) {
        const std::string selected_name =
            selection > 0 && static_cast<std::size_t>(selection) <= ports.size()
                ? ports[selection - 1].name
                : "";
        ports_loaded = true;
        try {
          ports = Serial::get_ports();
          selection = 0;
          for (std::size_t i = 0; i < ports.size(); ++i) {
            if (ports[i].name == selected_name) {
              selection = static_cast<int>(i + 1);
              break;
            }
          }
        } catch (const std::exception &err) {
          log.add_entry("failed to get ports: " + std::string(err.what()));
        }
      }

      ImGui::SameLine();
      ImGui::BeginDisabled(selection <= 0 ||
                           static_cast<std::size_t>(selection) > ports.size());
      const bool retry_connection = ImGui::Button("Retry Connection");
      ImGui::EndDisabled();

      std::vector<const char *> port_names = {"none"};
      port_names.resize(ports.size() + 1);
      for (std::size_t i = 0; i < ports.size(); i++) {
        port_names[i + 1] = ports[i].name.c_str();
      }

      const std::string status =
          connection_status + " | " + std::to_string(ports.size()) + " found";
      ImGui::TextUnformatted(status.c_str());

      const bool selection_changed =
          ImGui::Combo("##ports", &selection, port_names.data(),
                       static_cast<int>(port_names.size()));

      if ((selection_changed || retry_connection) && selection > 0 &&
          static_cast<std::size_t>(selection) <= ports.size() &&
          !connection_task.valid()) {
        const std::string port_name = ports[selection - 1].name;
        connection_status = "Connecting to " + port_name;

        try {
          connection_task = std::async(std::launch::async, [&port, port_name] {
            port.initialize(port_name, 115200);
          });
        } catch (const std::exception &err) {
          connection_status = "Connection failed";
          log.add_entry("connection failed: " + std::string(err.what()));
        }
      }

      ImGui::EndDisabled();
      ImGui::End();
    }

    // Stepper control panel
    {
      const std::array<const char *, 2> cmds = {"step", "revolve"};
      static int selection = 0, steps = 0;
      static float deg = 0;

      ImGui::SetNextWindowDockID(dockspace, ImGuiCond_FirstUseEver);
      ImGui::Begin("Stepper Control");

      ImGui::Combo("##commands", &selection, cmds.data(),
                   static_cast<int>(cmds.size()));

      if (selection == 0)
        ImGui::InputInt("steps", &steps);
      else
        ImGui::SliderFloat("deg", &deg, -360.0f, 360.0f);

      ImGui::BeginDisabled(stepper_task.valid());
      if (ImGui::Button("Submit") && !stepper_task.valid()) {
        const std::string cmd = selection == 0
                                    ? "step " + std::to_string(steps)
                                    : "revolve " + std::to_string(deg);
        try {
          stepper_task = std::async(std::launch::async, [&port, cmd] {
            std::string pending;
            port.send_and_wait(cmd, pending, "finish", 5000);
          });
        } catch (const std::exception &err) {
          log.add_entry("failed to send command");
        }
      }

      ImGui::EndDisabled();
      ImGui::End();
    }

    // Rendering
    ImGui::Render();
    int display_w, display_h;
    glfwGetFramebufferSize(window, &display_w, &display_h);
    glViewport(0, 0, display_w, display_h);
    glClearColor(clear_color.x * clear_color.w, clear_color.y * clear_color.w,
                 clear_color.z * clear_color.w, clear_color.w);
    glClear(GL_COLOR_BUFFER_BIT);
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

    glfwSwapBuffers(window);
  }

  // Cleanup
  ImGui_ImplOpenGL3_Shutdown();
  ImGui_ImplGlfw_Shutdown();
  ImGui::DestroyContext();

  glfwDestroyWindow(window);
  glfwTerminate();

  return 0;
}

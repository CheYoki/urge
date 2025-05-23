// Copyright 2018-2025 Admenri.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/worker/content_runner.h"

#include "imgui/backends/imgui_impl_sdl3.h"
#include "imgui/imgui.h"

#include "content/context/execution_context.h"
#include "content/profile/command_ids.h"

namespace content {

namespace {

void DrawEngineInfoGUI(I18NProfile* i18n_profile) {
  if (ImGui::CollapsingHeader(
          i18n_profile->GetI18NString(IDS_SETTINGS_ABOUT, "About").c_str())) {
    ImGui::Text("Universal Ruby Game Engine (URGE) Runtime");
    ImGui::Separator();
    ImGui::Text("Copyright (C) 2018-2025 Admenri Adev.");
    ImGui::TextWrapped(
        "The URGE is licensed under the BSD-2-Clause License, see LICENSE for "
        "more information.");

    if (ImGui::Button("Github"))
      SDL_OpenURL("https://github.com/Admenri/urge");
  }
}

}  // namespace

ContentRunner::ContentRunner(std::unique_ptr<ContentProfile> profile,
                             std::unique_ptr<filesystem::IOService> io_service,
                             std::unique_ptr<base::ThreadWorker> render_worker,
                             std::unique_ptr<EngineBindingBase> binding,
                             base::WeakPtr<ui::Widget> window)
    : profile_(std::move(profile)),
      render_worker_(std::move(render_worker)),
      window_(window),
      exit_code_(0),
      binding_quit_flag_(0),
      binding_reset_flag_(0),
      binding_(std::move(binding)),
      io_service_(std::move(io_service)),
      disable_gui_input_(false),
      show_settings_menu_(false),
      show_fps_monitor_(false),
      last_tick_(SDL_GetPerformanceCounter()),
      total_delta_(0),
      frame_count_(0) {}

ContentRunner::~ContentRunner() {
  base::ThreadWorker::PostTask(
      render_worker_.get(),
      base::BindOnce(&ContentRunner::DestroyIMGUIContextInternal,
                     base::Unretained(this)));
  base::ThreadWorker::WaitWorkerSynchronize(render_worker_.get());
}

bool ContentRunner::RunMainLoop() {
  // Poll event queue
  SDL_Event queued_event;
  while (SDL_PollEvent(&queued_event)) {
    // Quit event
    if (queued_event.type == SDL_EVENT_QUIT)
      binding_quit_flag_.store(1);

    // GUI event process
    if (!disable_gui_input_)
      ImGui_ImplSDL3_ProcessEvent(&queued_event);

    // Shortcut
    if (queued_event.type == SDL_EVENT_KEY_UP &&
        queued_event.key.windowID == window_->GetWindowID()) {
      if (queued_event.key.scancode == SDL_SCANCODE_F1) {
        show_settings_menu_ = !show_settings_menu_;
      } else if (queued_event.key.scancode == SDL_SCANCODE_F2) {
        show_fps_monitor_ = !show_fps_monitor_;
      } else if (queued_event.key.scancode == SDL_SCANCODE_F12) {
        binding_reset_flag_.store(1);
      }
    }
  }

  // Reset input state
  input_impl_->SetUpdateEnable(true);
  mouse_impl_->SetUpdateEnable(true);

  // Update fps
  UpdateDisplayFPSInternal();

  // Render GUI if need
  RenderGUIInternal();

  // Present screen buffer
  graphics_impl_->PresentScreenBuffer(imgui_.get());

  return exit_code_.load();
}

std::unique_ptr<ContentRunner> ContentRunner::Create(InitParams params) {
  std::unique_ptr<base::ThreadWorker> render_worker =
      base::ThreadWorker::Create();

  auto* runner = new ContentRunner(
      std::move(params.profile), std::move(params.io_service),
      std::move(render_worker), std::move(params.entry), params.window);
  runner->InitializeContentInternal();

  return std::unique_ptr<ContentRunner>(runner);
}

void ContentRunner::InitializeContentInternal() {
  // Initialize CC
  cc_.reset(new CoroutineContext);
  cc_->primary_fiber = fiber_create(nullptr, 0, nullptr, nullptr);
  cc_->main_loop_fiber =
      fiber_create(cc_->primary_fiber, 0, EngineEntryFunctionInternal, this);

  // Graphics initialize settings
  int32_t frame_rate = 40;
  if (profile_->api_version >= ContentProfile::APIVersion::RGSS2)
    frame_rate = 60;

  // Create components instance
  auto* i18n_xml_stream =
      io_service_->OpenReadRaw(profile_->i18n_xml_path, nullptr);
  i18n_profile_ = I18NProfile::MakeForStream(i18n_xml_stream);
  scoped_font_.reset(
      new ScopedFontData(io_service_.get(), profile_->default_font_path));
  graphics_impl_.reset(new RenderScreenImpl(
      window_, render_worker_.get(), cc_.get(), profile_.get(),
      i18n_profile_.get(), io_service_.get(), scoped_font_.get(),
      profile_->resolution, frame_rate));
  input_impl_.reset(new KeyboardControllerImpl(
      window_->AsWeakPtr(), profile_.get(), i18n_profile_.get()));
  audio_impl_.reset(
      new AudioImpl(profile_.get(), io_service_.get(), i18n_profile_.get()));
  mouse_impl_.reset(new MouseImpl(window_->AsWeakPtr(), profile_.get()));

  // Create imgui context
  base::ThreadWorker::PostTask(
      render_worker_.get(),
      base::BindOnce(&ContentRunner::CreateIMGUIContextInternal,
                     base::Unretained(this)));
  base::ThreadWorker::WaitWorkerSynchronize(render_worker_.get());

  // Hook graphics event loop
  tick_observer_ = graphics_impl_->AddTickObserver(base::BindRepeating(
      &ContentRunner::TickHandlerInternal, base::Unretained(this)));
}

void ContentRunner::TickHandlerInternal() {
  frame_count_++;

  if (binding_quit_flag_.load()) {
    binding_quit_flag_.store(0);
    binding_->ExitSignalRequired();
  } else if (binding_reset_flag_.load()) {
    binding_reset_flag_.store(0);
    binding_->ResetSignalRequired();
  }
}

void ContentRunner::UpdateDisplayFPSInternal() {
  const uint64_t current_tick = SDL_GetPerformanceCounter();
  const int64_t delta_tick = current_tick - last_tick_;
  last_tick_ = current_tick;

  total_delta_ += delta_tick;
  const uint64_t timer_freq = SDL_GetPerformanceFrequency();
  if (total_delta_ >= timer_freq) {
    const float fps_scale = static_cast<float>(
        SDL_GetPerformanceFrequency() / static_cast<float>(total_delta_));
    const float current_fps = static_cast<float>(frame_count_) * fps_scale;

    total_delta_ = 0;
    frame_count_ = 0;

    fps_history_.push_back(current_fps);
    if (fps_history_.size() > 20)
      fps_history_.erase(fps_history_.begin());
  }
}

void ContentRunner::RenderGUIInternal() {
  // Setup renderer new frame
  const Diligent::SwapChainDesc& swapchain_desc =
      graphics_impl_->GetDevice()->GetSwapchain()->GetDesc();
  imgui_->NewFrame(swapchain_desc.Width, swapchain_desc.Height,
                   swapchain_desc.PreTransform);

  // Setup sdl new frame
  ImGui_ImplSDL3_NewFrame();

  // Setup imgui new frame
  ImGui::NewFrame();

  // Render settings menu
  if (show_settings_menu_)
    RenderSettingsGUIInternal();

  // Render fps monitor
  if (show_fps_monitor_)
    RenderFPSMonitorGUIInternal();

  // Final gui render
  ImGui::Render();
}

void ContentRunner::RenderSettingsGUIInternal() {
  ImGui::SetNextWindowPos(ImVec2(), ImGuiCond_FirstUseEver);
  ImGui::SetNextWindowSize(ImVec2(350, 400), ImGuiCond_FirstUseEver);

  if (ImGui::Begin(i18n_profile_->GetI18NString(IDS_MENU_SETTINGS, "Settings")
                       .c_str())) {
    // Reset input state
    input_impl_->SetUpdateEnable(!ImGui::IsWindowFocused());
    mouse_impl_->SetUpdateEnable(!ImGui::IsWindowFocused());

    // Button settings
    disable_gui_input_ = input_impl_->CreateButtonGUISettings();

    // Graphics settings
    graphics_impl_->CreateButtonGUISettings();

    // Engine Info
    DrawEngineInfoGUI(i18n_profile_.get());
  }

  // End window create
  ImGui::End();
}

void ContentRunner::RenderFPSMonitorGUIInternal() {
  if (ImGui::Begin("FPS")) {
    // Draw plot for fps monitor
    ImGui::PlotHistogram("##FPSDisplay", fps_history_.data(),
                         static_cast<int>(fps_history_.size()), 0, nullptr,
                         0.0f, FLT_MAX, ImVec2(300, 80));
  }

  ImGui::End();
}

void ContentRunner::CreateIMGUIContextInternal() {
  auto* render_device = graphics_impl_->GetDevice();

  // Setup context
  ImGui::CreateContext();
  ImGuiIO& io = ImGui::GetIO();
  io.IniFilename = nullptr;
  io.ConfigFlags |=
      ImGuiConfigFlags_NavEnableKeyboard | ImGuiConfigFlags_NavEnableGamepad;

  // Apply DPI Settings
  int display_w, display_h;
  SDL_GetWindowSizeInPixels(window_->AsSDLWindow(), &display_w, &display_h);
  io.DisplaySize =
      ImVec2(static_cast<float>(display_w), static_cast<float>(display_h));
  io.DisplayFramebufferScale = ImVec2(1.0f, 1.0f);
  float window_scale = SDL_GetWindowDisplayScale(window_->AsSDLWindow());
  ImGui::GetStyle().ScaleAllSizes(window_scale);

  // Apply default font
  int64_t font_data_size;
  auto* font_data = scoped_font_->GetUIDefaultFont(&font_data_size);
  ImFontConfig font_config;
  font_config.FontDataOwnedByAtlas = false;
  io.Fonts->AddFontFromMemoryTTF(const_cast<void*>(font_data), font_data_size,
                                 16.0f * window_scale, &font_config,
                                 io.Fonts->GetGlyphRangesChineseFull());

  // Setup Dear ImGui style
  ImGui::StyleColorsDark();

  // Setup imgui platform backends
  ImGui_ImplSDL3_InitForOther(window_->AsSDLWindow());

  // Setup renderer backend
  Diligent::ImGuiDiligentCreateInfo imgui_create_info(
      **render_device, render_device->GetSwapchain()->GetDesc());
  imgui_.reset(new Diligent::ImGuiDiligentRenderer(imgui_create_info));
}

void ContentRunner::DestroyIMGUIContextInternal() {
  imgui_.reset();

  ImGui_ImplSDL3_Shutdown();
  ImGui::DestroyContext();
}

void ContentRunner::EngineEntryFunctionInternal(fiber_t* fiber) {
  auto* self = static_cast<ContentRunner*>(fiber->userdata);

  // Before running loop handler
  self->binding_->PreEarlyInitialization(self->profile_.get(),
                                         self->io_service_.get());

  // Make script binding execution context
  // Call binding boot handler before running loop handler
  ExecutionContext execution_context;
  execution_context.font_context = self->scoped_font_.get();
  execution_context.canvas_scheduler =
      self->graphics_impl_->GetCanvasScheduler();
  execution_context.graphics = self->graphics_impl_.get();
  execution_context.input = self->input_impl_.get();

  // Make module context
  EngineBindingBase::ScopedModuleContext module_context;
  module_context.graphics = self->graphics_impl_.get();
  module_context.input = self->input_impl_.get();
  module_context.audio = self->audio_impl_.get();
  module_context.mouse = self->mouse_impl_.get();

  // Execute main loop
  self->binding_->OnMainMessageLoopRun(&execution_context, &module_context);

  // End of running
  self->binding_->PostMainLoopRunning();
  self->exit_code_.store(1);

  // Loop switch context to primary fiber
  for (;;)
    fiber_switch(self->cc_->primary_fiber);
}

}  // namespace content

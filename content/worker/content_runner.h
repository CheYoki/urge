// Copyright 2018-2025 Admenri.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_WORKER_CONTENT_RUNNER_H_
#define CONTENT_WORKER_CONTENT_RUNNER_H_

#include "imgui/backends/ImGuiDiligentRenderer.hpp"

#include "base/worker/thread_worker.h"
#include "components/filesystem/io_service.h"
#include "content/components/font_context.h"
#include "content/input/keyboard_controller.h"
#include "content/input/mouse_controller.h"
#include "content/media/audio_impl.h"
#include "content/profile/content_profile.h"
#include "content/profile/i18n_profile.h"
#include "content/screen/renderscreen_impl.h"
#include "content/worker/coroutine_context.h"
#include "content/worker/engine_binding.h"
#include "ui/widget/widget.h"

namespace content {

class ContentRunner {
 public:
  struct InitParams {
    // Engine profile configure,
    // require a configure unique object.
    std::unique_ptr<ContentProfile> profile;

    // Binding boot entry,
    // require an unique one.
    std::unique_ptr<EngineBindingBase> entry;

    // Graphics renderer target.
    base::WeakPtr<ui::Widget> window;

    // Runner filesystem service.
    std::unique_ptr<filesystem::IOService> io_service;
  };

  ~ContentRunner();

  ContentRunner(const ContentRunner&) = delete;
  ContentRunner& operator=(const ContentRunner&) = delete;

  static std::unique_ptr<ContentRunner> Create(InitParams params);

  bool RunMainLoop();

 private:
  ContentRunner(std::unique_ptr<ContentProfile> profile,
                std::unique_ptr<filesystem::IOService> io_service,
                std::unique_ptr<base::ThreadWorker> render_worker,
                std::unique_ptr<EngineBindingBase> binding,
                base::WeakPtr<ui::Widget> window);
  void InitializeContentInternal();
  void TickHandlerInternal();
  void UpdateDisplayFPSInternal();
  void RenderGUIInternal();
  void RenderSettingsGUIInternal();
  void RenderFPSMonitorGUIInternal();

  void CreateIMGUIContextInternal();
  void DestroyIMGUIContextInternal();

  static void EngineEntryFunctionInternal(fiber_t* fiber);

  std::unique_ptr<ContentProfile> profile_;
  std::unique_ptr<CoroutineContext> cc_;
  std::unique_ptr<base::ThreadWorker> render_worker_;
  base::WeakPtr<ui::Widget> window_;
  std::atomic<int32_t> exit_code_;
  base::CallbackListSubscription tick_observer_;
  std::atomic<int32_t> binding_quit_flag_;
  std::atomic<int32_t> binding_reset_flag_;

  std::unique_ptr<EngineBindingBase> binding_;
  std::unique_ptr<ExecutionContext> execution_context_;

  std::unique_ptr<RenderScreenImpl> graphics_impl_;
  std::unique_ptr<KeyboardControllerImpl> input_impl_;
  std::unique_ptr<AudioImpl> audio_impl_;
  std::unique_ptr<MouseImpl> mouse_impl_;

  std::unique_ptr<filesystem::IOService> io_service_;
  std::unique_ptr<ScopedFontData> scoped_font_;
  std::unique_ptr<I18NProfile> i18n_profile_;
  std::unique_ptr<Diligent::ImGuiDiligentRenderer> imgui_;

  bool disable_gui_input_;
  bool show_settings_menu_;
  bool show_fps_monitor_;

  uint64_t last_tick_;
  int64_t total_delta_;
  int32_t frame_count_;
  std::vector<float> fps_history_;
};

}  // namespace content

#endif  //! CONTENT_WORKER_CONTENT_RUNNER_H_

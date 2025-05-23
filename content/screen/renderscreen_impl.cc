// Copyright 2018-2025 Admenri.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/screen/renderscreen_impl.h"

#include <unordered_map>

#include "SDL3/SDL_events.h"
#include "SDL3/SDL_timer.h"
#include "magic_enum/magic_enum.hpp"

#include "Graphics/GraphicsAccessories/interface/GraphicsAccessories.hpp"

#include "content/canvas/canvas_scheduler.h"
#include "content/profile/command_ids.h"
#include "renderer/utils/texture_utils.h"

namespace content {

namespace {

void GPUResetScreenBufferInternal(RenderGraphicsAgent* agent,
                                  const base::Vec2i& resolution) {
  constexpr Diligent::BIND_FLAGS bind_flags =
      Diligent::BIND_RENDER_TARGET | Diligent::BIND_SHADER_RESOURCE;

  agent->screen_buffer.Release();
  agent->frozen_buffer.Release();
  agent->transition_buffer.Release();

  renderer::CreateTexture2D(**agent->device, &agent->screen_buffer,
                            "screen.main.buffer", resolution,
                            Diligent::USAGE_DEFAULT, bind_flags);
  renderer::CreateTexture2D(**agent->device, &agent->frozen_buffer,
                            "screen.frozen.buffer", resolution,
                            Diligent::USAGE_DEFAULT, bind_flags);
  renderer::CreateTexture2D(**agent->device, &agent->transition_buffer,
                            "screen.transition.buffer", resolution,
                            Diligent::USAGE_DEFAULT, bind_flags);

  renderer::WorldTransform world_transform;
  renderer::MakeIdentityMatrix(world_transform.transform);
  renderer::MakeProjectionMatrix(world_transform.projection, resolution,
                                 agent->device->IsUVFlip());

  agent->world_transform.Release();
  Diligent::CreateUniformBuffer(
      **agent->device, sizeof(world_transform), "graphics.world.transform",
      &agent->world_transform, Diligent::USAGE_IMMUTABLE,
      Diligent::BIND_UNIFORM_BUFFER, Diligent::CPU_ACCESS_NONE,
      &world_transform);
}

void GPUCreateGraphicsHostInternal(RenderGraphicsAgent* agent,
                                   base::WeakPtr<ui::Widget> window,
                                   ContentProfile* profile,
                                   base::ThreadWorker* render_worker,
                                   filesystem::IOService* io_service,
                                   const base::Vec2i& resolution) {
  // Create primary device on window widget
  agent->device = renderer::RenderDevice::Create(
      window,
      magic_enum::enum_cast<renderer::DriverType>(profile->driver_backend)
          .value_or(renderer::DriverType::UNDEFINED));

  // Create global canvas scheduler
  agent->canvas_scheduler = CanvasScheduler::MakeInstance(
      render_worker, agent->device.get(), io_service);

  // Create global sprite batch scheduler
  agent->sprite_batch = SpriteBatch::Make(agent->device.get());

  // Get pipeline manager
  auto* pipelines = agent->device->GetPipelines();

  // Create generic quads batch
  agent->transition_quads = renderer::QuadBatch::Make(**agent->device);
  agent->effect_quads = renderer::QuadBatch::Make(**agent->device);

  // Create generic shader binding
  agent->transition_binding_alpha =
      pipelines->alphatrans.CreateBinding<renderer::Binding_AlphaTrans>();
  agent->transition_binding_vague =
      pipelines->mappedtrans.CreateBinding<renderer::Binding_VagueTrans>();
  agent->effect_binding =
      pipelines->color.CreateBinding<renderer::Binding_Color>();

  // If the swap chain color buffer format is a non-sRGB UNORM format,
  // we need to manually convert pixel shader output to gamma space.
  auto* swapchain = agent->device->GetSwapchain();
  const auto& swapchain_desc = swapchain->GetDesc();
  bool convert_gamma_to_output = (swapchain_desc.ColorBufferFormat ==
                                      Diligent::TEX_FORMAT_RGBA8_UNORM_SRGB ||
                                  swapchain_desc.ColorBufferFormat ==
                                      Diligent::TEX_FORMAT_BGRA8_UNORM_SRGB);

  // Create screen present pipeline
  agent->present_pipeline.reset(new renderer::Pipeline_Present(
      **agent->device, swapchain_desc.ColorBufferFormat,
      convert_gamma_to_output));

  // Get renderer info
  Diligent::GraphicsAdapterInfo adapter_info =
      (*agent->device)->GetAdapterInfo();
  agent->renderer_info.device = Diligent::GetRenderDeviceTypeString(
      (*agent->device)->GetDeviceInfo().Type);
  agent->renderer_info.vendor = magic_enum::enum_name(
      Diligent::VendorIdToAdapterVendor(adapter_info.VendorId));
  agent->renderer_info.description = adapter_info.Description;

  struct {
    std::string device;
    std::string vendor;
    std::string description;
  } renderer_info_;

  // Create screen buffer
  GPUResetScreenBufferInternal(agent, resolution);
}

void GPUDestroyGraphicsHostInternal(RenderGraphicsAgent* agent) {
  delete agent;
}

void GPUPresentScreenBufferInternal(
    RenderGraphicsAgent* agent,
    const base::Rect& display_viewport,
    const base::Vec2i& resolution,
    bool keep_ratio,
    bool smooth,
    Diligent::ImGuiDiligentRenderer* gui_renderer) {
  // Initial device attribute
  Diligent::IDeviceContext* context = agent->device->GetContext();
  Diligent::ISwapChain* swapchain = agent->device->GetSwapchain();

  // Process mouse coordinate and viewport rect
  base::Rect target_rect;
  base::WeakPtr<ui::Widget> window = agent->device->GetWindow();
  window->GetMouseState().resolution = resolution;
  if (keep_ratio) {
    target_rect = display_viewport;
    window->GetMouseState().screen_offset = display_viewport.Position();
    window->GetMouseState().screen = display_viewport.Size();
  } else {
    target_rect = window->GetSize();
    window->GetMouseState().screen_offset = base::Vec2i();
    window->GetMouseState().screen = window->GetSize();
  }

  // Setup render params
  auto* render_target_view = swapchain->GetCurrentBackBufferRTV();
  auto& pipeline_set = *agent->present_pipeline;
  auto* pipeline = pipeline_set.GetPipeline(renderer::BlendType::NO_BLEND);
  std::unique_ptr<renderer::QuadBatch> present_quads =
      renderer::QuadBatch::Make(**agent->device);
  std::unique_ptr<renderer::Binding_Base> present_binding =
      pipeline_set.CreateBinding<renderer::Binding_Base>();
  RRefPtr<Diligent::IBuffer> present_uniform;
  RRefPtr<Diligent::ISampler> present_sampler;

  if (agent->present_target) {
    // Update vertex
    renderer::Quad transient_quad;
    renderer::Quad::SetPositionRect(&transient_quad, target_rect);
    renderer::Quad::SetTexCoordRectNorm(
        &transient_quad, agent->device->IsUVFlip() ? base::Rect(0, 1, 1, -1)
                                                   : base::Rect(0, 0, 1, 1));
    present_quads->QueueWrite(context, &transient_quad);

    // Update window screen transform
    renderer::WorldTransform world_matrix;
    renderer::MakeProjectionMatrix(world_matrix.projection, window->GetSize(),
                                   agent->device->IsUVFlip());
    renderer::MakeIdentityMatrix(world_matrix.transform);

    Diligent::CreateUniformBuffer(**agent->device, sizeof(world_matrix),
                                  "present.world.uniform", &present_uniform,
                                  Diligent::USAGE_IMMUTABLE,
                                  Diligent::BIND_UNIFORM_BUFFER,
                                  Diligent::CPU_ACCESS_NONE, &world_matrix);

    // Create sampler
    Diligent::SamplerDesc sampler_desc;
    sampler_desc.Name = "present.sampler";
    sampler_desc.MinFilter =
        smooth ? Diligent::FILTER_TYPE_LINEAR : Diligent::FILTER_TYPE_POINT;
    sampler_desc.MagFilter = sampler_desc.MinFilter;
    sampler_desc.MipFilter = sampler_desc.MinFilter;
    (*agent->device)->CreateSampler(sampler_desc, &present_sampler);
  }

  // Update gui device objects if need
  if (gui_renderer) {
    gui_renderer->CheckDeviceObjects();
  }

  // Prepare for rendering
  float clear_color[] = {0, 0, 0, 1};
  context->SetRenderTargets(
      1, &render_target_view, nullptr,
      Diligent::RESOURCE_STATE_TRANSITION_MODE_TRANSITION);
  context->ClearRenderTarget(
      render_target_view, clear_color,
      Diligent::RESOURCE_STATE_TRANSITION_MODE_TRANSITION);

  Diligent::Rect scissor;
  scissor.right = window->GetSize().x;
  scissor.bottom = window->GetSize().y;
  context->SetScissorRects(1, &scissor, 1, scissor.bottom + scissor.top);

  // Start screen render
  if (agent->present_target) {
    auto* render_source =
        (*agent->present_target)
            ->GetDefaultView(Diligent::TEXTURE_VIEW_SHADER_RESOURCE);
    render_source->SetSampler(present_sampler);

    // Set present uniform
    present_binding->u_transform->Set(present_uniform);
    present_binding->u_texture->Set(render_source);

    // Apply pipeline state
    context->SetPipelineState(pipeline);
    context->CommitShaderResources(
        **present_binding, Diligent::RESOURCE_STATE_TRANSITION_MODE_TRANSITION);

    // Apply vertex index
    Diligent::IBuffer* const vertex_buffer = **present_quads;
    context->SetVertexBuffers(
        0, 1, &vertex_buffer, nullptr,
        Diligent::RESOURCE_STATE_TRANSITION_MODE_TRANSITION);
    context->SetIndexBuffer(
        **agent->device->GetQuadIndex(), 0,
        Diligent::RESOURCE_STATE_TRANSITION_MODE_TRANSITION);

    // Execute render command
    Diligent::DrawIndexedAttribs draw_indexed_attribs;
    draw_indexed_attribs.NumIndices = 6;
    draw_indexed_attribs.IndexType = agent->device->GetQuadIndex()->format();
    context->DrawIndexed(draw_indexed_attribs);
  }

  // Render GUI if need
  if (gui_renderer) {
    gui_renderer->RenderDrawData(context, ImGui::GetDrawData());
  }

  // Flush command buffer and present GPU surface
  swapchain->Present();
}

void GPUFrameBeginRenderPassInternal(RenderGraphicsAgent* agent,
                                     Diligent::ITexture** render_target,
                                     uint32_t brightness) {
  auto* context = agent->device->GetContext();

  // Setup render target
  Diligent::ITexture* const render_target_ptr = *render_target;
  const base::Vec2i target_size(render_target_ptr->GetDesc().Width,
                                render_target_ptr->GetDesc().Height);

  // Setup screen effect params
  if (brightness < 255) {
    renderer::Quad effect_quad;
    renderer::Quad::SetPositionRect(&effect_quad, base::Rect(target_size));
    renderer::Quad::SetColor(&effect_quad,
                             base::Vec4(0, 0, 0, (255 - brightness) / 255.0f));
    agent->effect_quads->QueueWrite(context, &effect_quad);
  }

  // Setup render pass
  auto render_target_view =
      render_target_ptr->GetDefaultView(Diligent::TEXTURE_VIEW_RENDER_TARGET);
  const float clear_color[] = {0, 0, 0, 1};
  context->SetRenderTargets(
      1, &render_target_view, nullptr,
      Diligent::RESOURCE_STATE_TRANSITION_MODE_TRANSITION);
  context->ClearRenderTarget(
      render_target_view, clear_color,
      Diligent::RESOURCE_STATE_TRANSITION_MODE_TRANSITION);

  // Restore scissor region
  Diligent::Rect render_scissor;
  render_scissor.right = target_size.x;
  render_scissor.bottom = target_size.y;
  context->SetScissorRects(1, &render_scissor, 1,
                           render_scissor.bottom + render_scissor.top);
}

void GPUFrameEndRenderPassInternal(RenderGraphicsAgent* agent,
                                   uint32_t brightness) {
  auto* context = agent->device->GetContext();

  // Render screen effect if need
  if (brightness < 255) {
    // Apply brightness effect
    auto& pipeline_set = agent->device->GetPipelines()->color;
    auto* pipeline = pipeline_set.GetPipeline(renderer::BlendType::NORMAL);

    // Set world transform
    agent->effect_binding->u_transform->Set(agent->world_transform);

    // Apply pipeline state
    context->SetPipelineState(pipeline);
    context->CommitShaderResources(
        **agent->effect_binding,
        Diligent::RESOURCE_STATE_TRANSITION_MODE_TRANSITION);

    // Apply vertex index
    Diligent::IBuffer* const vertex_buffer = **agent->effect_quads;
    context->SetVertexBuffers(
        0, 1, &vertex_buffer, nullptr,
        Diligent::RESOURCE_STATE_TRANSITION_MODE_TRANSITION);
    context->SetIndexBuffer(
        **agent->device->GetQuadIndex(), 0,
        Diligent::RESOURCE_STATE_TRANSITION_MODE_TRANSITION);

    // Execute render command
    Diligent::DrawIndexedAttribs draw_indexed_attribs;
    draw_indexed_attribs.NumIndices = 6;
    draw_indexed_attribs.IndexType = agent->device->GetQuadIndex()->format();
    context->DrawIndexed(draw_indexed_attribs);
  }
}

void GPURenderAlphaTransitionFrameInternal(RenderGraphicsAgent* agent,
                                           float progress) {
  // Initial context
  Diligent::IDeviceContext* context = agent->device->GetContext();

  // Target UV
  const bool flip_uv = agent->device->IsUVFlip();
  const base::RectF uv_rect = flip_uv ? base::RectF(0.0f, 1.0f, 1.0f, -1.0f)
                                      : base::RectF(0.0f, 0.0f, 1.0f, 1.0f);

  // Update transition uniform
  renderer::Quad transient_quad;
  renderer::Quad::SetPositionRect(&transient_quad,
                                  base::RectF(-1.0f, 1.0f, 2.0f, -2.0f));
  renderer::Quad::SetTexCoordRectNorm(&transient_quad, uv_rect);
  renderer::Quad::SetColor(&transient_quad, base::Vec4(progress));
  agent->transition_quads->QueueWrite(context, &transient_quad);

  // Composite transition frame
  auto render_target_view = agent->screen_buffer->GetDefaultView(
      Diligent::TEXTURE_VIEW_RENDER_TARGET);
  const float clear_color[] = {0, 0, 0, 1};
  context->SetRenderTargets(
      1, &render_target_view, nullptr,
      Diligent::RESOURCE_STATE_TRANSITION_MODE_TRANSITION);
  context->ClearRenderTarget(
      render_target_view, clear_color,
      Diligent::RESOURCE_STATE_TRANSITION_MODE_TRANSITION);

  Diligent::Rect render_scissor;
  render_scissor.right = agent->screen_buffer->GetDesc().Width;
  render_scissor.bottom = agent->screen_buffer->GetDesc().Height;
  context->SetScissorRects(1, &render_scissor, 1,
                           render_scissor.bottom + render_scissor.top);

  // Apply brightness effect
  auto& pipeline_set = agent->device->GetPipelines()->alphatrans;
  auto* pipeline = pipeline_set.GetPipeline(renderer::BlendType::NO_BLEND);

  // Set uniform texture
  static_cast<renderer::Binding_AlphaTrans*>(
      agent->transition_binding_alpha.get())
      ->u_current_texture->Set(agent->transition_buffer->GetDefaultView(
          Diligent::TEXTURE_VIEW_SHADER_RESOURCE));
  static_cast<renderer::Binding_AlphaTrans*>(
      agent->transition_binding_alpha.get())
      ->u_frozen_texture->Set(agent->frozen_buffer->GetDefaultView(
          Diligent::TEXTURE_VIEW_SHADER_RESOURCE));

  // Apply pipeline state
  context->SetPipelineState(pipeline);
  context->CommitShaderResources(
      **agent->transition_binding_alpha,
      Diligent::RESOURCE_STATE_TRANSITION_MODE_TRANSITION);

  // Apply vertex index
  Diligent::IBuffer* const vertex_buffer = **agent->transition_quads;
  context->SetVertexBuffers(
      0, 1, &vertex_buffer, nullptr,
      Diligent::RESOURCE_STATE_TRANSITION_MODE_TRANSITION);
  context->SetIndexBuffer(**agent->device->GetQuadIndex(), 0,
                          Diligent::RESOURCE_STATE_TRANSITION_MODE_TRANSITION);

  // Execute render command
  Diligent::DrawIndexedAttribs draw_indexed_attribs;
  draw_indexed_attribs.NumIndices = 6;
  draw_indexed_attribs.IndexType = agent->device->GetQuadIndex()->format();
  context->DrawIndexed(draw_indexed_attribs);
}

void GPURenderVagueTransitionFrameInternal(
    RenderGraphicsAgent* agent,
    float progress,
    float vague,
    Diligent::ITextureView** trans_mapping) {
  // Initial context
  Diligent::IDeviceContext* context = agent->device->GetContext();

  // Target UV
  const bool flip_uv = agent->device->IsUVFlip();
  const base::RectF uv_rect = flip_uv ? base::RectF(0.0f, 1.0f, 1.0f, -1.0f)
                                      : base::RectF(0.0f, 0.0f, 1.0f, 1.0f);

  // Update transition uniform
  renderer::Quad transient_quad;
  renderer::Quad::SetPositionRect(&transient_quad,
                                  base::RectF(-1.0f, 1.0f, 2.0f, -2.0f));
  renderer::Quad::SetTexCoordRectNorm(&transient_quad, uv_rect);
  renderer::Quad::SetColor(&transient_quad, base::Vec4(vague, 0, 0, progress));
  agent->transition_quads->QueueWrite(context, &transient_quad);

  // Composite transition frame
  auto render_target_view = agent->screen_buffer->GetDefaultView(
      Diligent::TEXTURE_VIEW_RENDER_TARGET);
  const float clear_color[] = {0, 0, 0, 1};
  context->SetRenderTargets(
      1, &render_target_view, nullptr,
      Diligent::RESOURCE_STATE_TRANSITION_MODE_TRANSITION);
  context->ClearRenderTarget(
      render_target_view, clear_color,
      Diligent::RESOURCE_STATE_TRANSITION_MODE_TRANSITION);

  Diligent::Rect render_scissor;
  render_scissor.right = agent->screen_buffer->GetDesc().Width;
  render_scissor.bottom = agent->screen_buffer->GetDesc().Height;
  context->SetScissorRects(1, &render_scissor, 1,
                           render_scissor.bottom + render_scissor.top);

  // Apply brightness effect
  auto& pipeline_set = agent->device->GetPipelines()->mappedtrans;
  auto* pipeline = pipeline_set.GetPipeline(renderer::BlendType::NO_BLEND);

  // Set uniform texture
  static_cast<renderer::Binding_VagueTrans*>(
      agent->transition_binding_vague.get())
      ->u_current_texture->Set(agent->transition_buffer->GetDefaultView(
          Diligent::TEXTURE_VIEW_SHADER_RESOURCE));
  static_cast<renderer::Binding_VagueTrans*>(
      agent->transition_binding_vague.get())
      ->u_frozen_texture->Set(agent->frozen_buffer->GetDefaultView(
          Diligent::TEXTURE_VIEW_SHADER_RESOURCE));
  static_cast<renderer::Binding_VagueTrans*>(
      agent->transition_binding_vague.get())
      ->u_trans_texture->Set(*trans_mapping);

  // Apply pipeline state
  context->SetPipelineState(pipeline);
  context->CommitShaderResources(
      **agent->transition_binding_vague,
      Diligent::RESOURCE_STATE_TRANSITION_MODE_TRANSITION);

  // Apply vertex index
  Diligent::IBuffer* const vertex_buffer = **agent->transition_quads;
  context->SetVertexBuffers(
      0, 1, &vertex_buffer, nullptr,
      Diligent::RESOURCE_STATE_TRANSITION_MODE_TRANSITION);
  context->SetIndexBuffer(**agent->device->GetQuadIndex(), 0,
                          Diligent::RESOURCE_STATE_TRANSITION_MODE_TRANSITION);

  // Execute render command
  Diligent::DrawIndexedAttribs draw_indexed_attribs;
  draw_indexed_attribs.NumIndices = 6;
  draw_indexed_attribs.IndexType = agent->device->GetQuadIndex()->format();
  context->DrawIndexed(draw_indexed_attribs);
}

void GPUResizeSwapchainInternal(RenderGraphicsAgent* agent,
                                const base::Vec2i& size) {
  auto* swapchain = agent->device->GetSwapchain();
  swapchain->Resize(size.x, size.y);
}

}  // namespace

RenderScreenImpl::RenderScreenImpl(base::WeakPtr<ui::Widget> window,
                                   base::ThreadWorker* render_worker,
                                   CoroutineContext* cc,
                                   ContentProfile* profile,
                                   I18NProfile* i18n_profile,
                                   filesystem::IOService* io_service,
                                   ScopedFontData* scoped_font,
                                   const base::Vec2i& resolution,
                                   int frame_rate)
    : cc_(cc),
      window_(window),
      profile_(profile),
      i18n_profile_(i18n_profile),
      io_service_(io_service),
      render_worker_(render_worker),
      scoped_font_(scoped_font),
      agent_(nullptr),
      frozen_(false),
      resolution_(resolution),
      brightness_(255),
      frame_count_(0),
      frame_rate_(frame_rate),
      elapsed_time_(0.0),
      smooth_delta_time_(1.0),
      last_count_time_(SDL_GetPerformanceCounter()),
      desired_delta_time_(SDL_GetPerformanceFrequency() / frame_rate_),
      frame_skip_required_(false),
      keep_ratio_(true),
      smooth_scale_(profile->smooth_scale),
      allow_skip_frame_(profile->allow_skip_frame) {
  // Setup render device on render thread if possible
  agent_ = new RenderGraphicsAgent;

  base::ThreadWorker::PostTask(
      render_worker,
      base::BindOnce(&GPUCreateGraphicsHostInternal, agent_, window, profile,
                     render_worker, io_service, resolution));
  base::ThreadWorker::WaitWorkerSynchronize(render_worker);
}

RenderScreenImpl::~RenderScreenImpl() {
  base::ThreadWorker::PostTask(
      render_worker_, base::BindOnce(&GPUDestroyGraphicsHostInternal, agent_));
  base::ThreadWorker::WaitWorkerSynchronize(render_worker_);
}

void RenderScreenImpl::PresentScreenBuffer(
    Diligent::ImGuiDiligentRenderer* gui_renderer) {
  // Determine update repeat time
  const uint64_t now_time = SDL_GetPerformanceCounter();
  const uint64_t delta_time = now_time - last_count_time_;
  last_count_time_ = now_time;

  // Calculate smooth frame rate
  const double delta_rate =
      delta_time / static_cast<double>(desired_delta_time_);
  const int repeat_time = DetermineRepeatNumberInternal(delta_rate);

  // Switch to runner coroutine
  for (int i = 0; i < repeat_time; ++i) {
    frame_skip_required_ = (i != 0);
    fiber_switch(cc_->main_loop_fiber);
  }

  // Update drawing viewport
  UpdateWindowViewportInternal();

  // Present to screen surface
  base::ThreadWorker::PostTask(
      render_worker_,
      base::BindOnce(&GPUPresentScreenBufferInternal, agent_, display_viewport_,
                     resolution_, keep_ratio_, smooth_scale_, gui_renderer));
  base::ThreadWorker::WaitWorkerSynchronize(render_worker_);
}

void RenderScreenImpl::CreateButtonGUISettings() {
  if (ImGui::CollapsingHeader(
          i18n_profile_->GetI18NString(IDS_SETTINGS_GRAPHICS, "Graphics")
              .c_str())) {
    ImGui::TextWrapped(
        "%s: %s",
        i18n_profile_->GetI18NString(IDS_GRAPHICS_RENDERER, "Renderer").c_str(),
        agent_->renderer_info.device.c_str());
    ImGui::Separator();
    ImGui::TextWrapped(
        "%s: %s",
        i18n_profile_->GetI18NString(IDS_GRAPHICS_VENDOR, "Vendor").c_str(),
        agent_->renderer_info.vendor.c_str());
    ImGui::Separator();
    ImGui::TextWrapped(
        "%s: %s",
        i18n_profile_->GetI18NString(IDS_GRAPHICS_DESCRIPTION, "Description")
            .c_str(),
        agent_->renderer_info.description.c_str());
    ImGui::Separator();

    // Keep Ratio
    ImGui::Checkbox(
        i18n_profile_->GetI18NString(IDS_GRAPHICS_KEEP_RATIO, "Keep Ratio")
            .c_str(),
        &keep_ratio_);

    // Smooth Scale
    ImGui::Checkbox(
        i18n_profile_->GetI18NString(IDS_GRAPHICS_SMOOTH_SCALE, "Smooth Scale")
            .c_str(),
        &smooth_scale_);

    // Skip Frame
    ImGui::Checkbox(
        i18n_profile_->GetI18NString(IDS_GRAPHICS_SKIP_FRAME, "Skip Frame")
            .c_str(),
        &allow_skip_frame_);

    // Fullscreen
    bool is_fullscreen = GetDevice()->GetWindow()->IsFullscreen(),
         last_fullscreen = is_fullscreen;
    ImGui::Checkbox(
        i18n_profile_->GetI18NString(IDS_GRAPHICS_FULLSCREEN, "Fullscreen")
            .c_str(),
        &is_fullscreen);
    if (last_fullscreen != is_fullscreen)
      GetDevice()->GetWindow()->SetFullscreen(is_fullscreen);
  }
}

base::CallbackListSubscription RenderScreenImpl::AddTickObserver(
    const base::RepeatingClosure& handler) {
  return tick_observers_.Add(handler);
}

void RenderScreenImpl::AddDisposable(Disposable* disp) {
  disposable_elements_.Append(disp);
}

void RenderScreenImpl::PostTask(base::OnceClosure task) {
  base::ThreadWorker::PostTask(render_worker_, std::move(task));
}

void RenderScreenImpl::WaitWorkerSynchronize() {
  base::ThreadWorker::WaitWorkerSynchronize(render_worker_);
}

void RenderScreenImpl::Update(ExceptionState& exception_state) {
  // Skip render if freeze screen or allow skip frames
  if (!frozen_) {
    if (!(allow_skip_frame_ && frame_skip_required_)) {
      // Render a frame and push into render queue
      // This function only encodes the render commands
      RenderFrameInternal(agent_->screen_buffer.RawDblPtr());
    }
  }

  // Process frame delay
  // This calling will yield to event coroutine and present
  FrameProcessInternal(agent_->screen_buffer.RawDblPtr());
}

void RenderScreenImpl::Wait(uint32_t duration,
                            ExceptionState& exception_state) {
  for (int32_t i = 0; i < duration; ++i)
    Update(exception_state);
}

void RenderScreenImpl::FadeOut(uint32_t duration,
                               ExceptionState& exception_state) {
  duration = std::max(duration, 1u);

  float current_brightness = static_cast<float>(brightness_);
  for (int i = 0; i < duration; ++i) {
    brightness_ = current_brightness -
                  current_brightness * (i / static_cast<float>(duration));
    if (frozen_) {
      FrameProcessInternal(agent_->frozen_buffer.RawDblPtr());
    } else {
      Update(exception_state);
    }
  }

  // Set final brightness
  brightness_ = 0;
  Update(exception_state);
}

void RenderScreenImpl::FadeIn(uint32_t duration,
                              ExceptionState& exception_state) {
  duration = std::max(duration, 1u);

  float current_brightness = static_cast<float>(brightness_);
  float diff = 255.0f - current_brightness;
  for (int i = 0; i < duration; ++i) {
    brightness_ =
        current_brightness + diff * (i / static_cast<float>(duration));

    if (frozen_) {
      FrameProcessInternal(agent_->frozen_buffer.RawDblPtr());
    } else {
      Update(exception_state);
    }
  }

  // Set final brightness
  brightness_ = 255;
  Update(exception_state);
}

void RenderScreenImpl::Freeze(ExceptionState& exception_state) {
  if (!frozen_) {
    // Get frozen scene snapshot for transition
    RenderFrameInternal(agent_->frozen_buffer.RawDblPtr());

    // Set forzen flag for blocking frame update
    frozen_ = true;
  }
}

void RenderScreenImpl::Transition(ExceptionState& exception_state) {
  Transition(10, std::string(), 40, exception_state);
}

void RenderScreenImpl::Transition(uint32_t duration,
                                  ExceptionState& exception_state) {
  Transition(duration, std::string(), 40, exception_state);
}

void RenderScreenImpl::Transition(uint32_t duration,
                                  const std::string& filename,
                                  ExceptionState& exception_state) {
  Transition(duration, filename, 40, exception_state);
}

void RenderScreenImpl::Transition(uint32_t duration,
                                  const std::string& filename,
                                  uint32_t vague,
                                  ExceptionState& exception_state) {
  scoped_refptr<CanvasImpl> transition_mapping = nullptr;
  if (!filename.empty())
    transition_mapping = CanvasImpl::Create(
        GetCanvasScheduler(), this, scoped_font_, filename, exception_state);
  if (exception_state.HadException())
    return;

  TransitionWithBitmap(duration, transition_mapping, vague, exception_state);
}

void RenderScreenImpl::TransitionWithBitmap(uint32_t duration,
                                            scoped_refptr<Bitmap> bitmap,
                                            uint32_t vague,
                                            ExceptionState& exception_state) {
  if (!frozen_)
    return;

  // Fetch screen attribute
  Put_Brightness(255, exception_state);
  vague = std::clamp<int>(vague, 1, 256);
  float vague_norm = vague / 255.0f;

  // Fetch transmapping if available
  scoped_refptr<CanvasImpl> mapping_bitmap = CanvasImpl::FromBitmap(bitmap);
  TextureAgent* texture_agent =
      mapping_bitmap ? mapping_bitmap->GetAgent() : nullptr;
  Diligent::ITextureView** transition_mapping =
      texture_agent ? texture_agent->view.RawDblPtr() : nullptr;

  // Get current scene snapshot for transition
  RenderFrameInternal(agent_->transition_buffer.RawDblPtr());

  // Transition render loop
  for (int i = 0; i < duration; ++i) {
    // Norm transition progress
    float progress = i * (1.0f / duration);

    // Render per transition frame
    if (transition_mapping)
      base::ThreadWorker::PostTask(
          render_worker_,
          base::BindOnce(&GPURenderVagueTransitionFrameInternal, agent_,
                         progress, vague_norm, transition_mapping));
    else
      base::ThreadWorker::PostTask(
          render_worker_, base::BindOnce(&GPURenderAlphaTransitionFrameInternal,
                                         agent_, progress));

    // Present to screen
    FrameProcessInternal(agent_->screen_buffer.RawDblPtr());
  }

  // Transition process complete
  frozen_ = false;
}

scoped_refptr<Bitmap> RenderScreenImpl::SnapToBitmap(
    ExceptionState& exception_state) {
  scoped_refptr<CanvasImpl> target = CanvasImpl::Create(
      GetCanvasScheduler(), this, scoped_font_, resolution_, exception_state);

  if (target) {
    RenderFrameInternal(target->GetAgent()->data.RawDblPtr());
    base::ThreadWorker::WaitWorkerSynchronize(render_worker_);
  }

  return target;
}

void RenderScreenImpl::FrameReset(ExceptionState& exception_state) {
  elapsed_time_ = 0.0;
  smooth_delta_time_ = 1.0;
  last_count_time_ = SDL_GetPerformanceCounter();
  desired_delta_time_ = SDL_GetPerformanceFrequency() / frame_rate_;
}

uint32_t RenderScreenImpl::Width(ExceptionState& exception_state) {
  return resolution_.x;
}

uint32_t RenderScreenImpl::Height(ExceptionState& exception_state) {
  return resolution_.y;
}

void RenderScreenImpl::ResizeScreen(uint32_t width,
                                    uint32_t height,
                                    ExceptionState& exception_state) {
  resolution_ = base::Vec2i(width, height);
  base::ThreadWorker::PostTask(
      render_worker_,
      base::BindOnce(&GPUResetScreenBufferInternal, agent_, resolution_));
  base::ThreadWorker::WaitWorkerSynchronize(render_worker_);
}

void RenderScreenImpl::Reset(ExceptionState& exception_state) {
  /* Reset freeze */
  frozen_ = false;

  /* Disposed all elements */
  for (auto it = disposable_elements_.tail(); it != disposable_elements_.end();
       it = it->previous()) {
    it->value()->Dispose(exception_state);
  }

  /* Reset attribute */
  frame_rate_ =
      profile_->api_version == ContentProfile::APIVersion::RGSS1 ? 40 : 60;
  brightness_ = 255;
  FrameReset(exception_state);
}

void RenderScreenImpl::PlayMovie(const std::string& filename,
                                 ExceptionState& exception_state) {
  exception_state.ThrowContentError(ExceptionCode::CONTENT_ERROR,
                                    "unimplement Graphics.play_movie");
}

uint32_t RenderScreenImpl::Get_FrameRate(ExceptionState& exception_state) {
  return frame_rate_;
}

void RenderScreenImpl::Put_FrameRate(const uint32_t& rate,
                                     ExceptionState& exception_state) {
  frame_rate_ = rate;
  FrameReset(exception_state);
}

uint32_t RenderScreenImpl::Get_FrameCount(ExceptionState& exception_state) {
  return frame_count_;
}

void RenderScreenImpl::Put_FrameCount(const uint32_t& count,
                                      ExceptionState& exception_state) {
  frame_count_ = count;
}

uint32_t RenderScreenImpl::Get_Brightness(ExceptionState& exception_state) {
  return brightness_;
}

void RenderScreenImpl::Put_Brightness(const uint32_t& value,
                                      ExceptionState& exception_state) {
  brightness_ = std::clamp<uint32_t>(value, 0, 255);
}

void RenderScreenImpl::FrameProcessInternal(
    Diligent::ITexture** present_target) {
  // Setup target
  agent_->present_target = present_target;

  // Increase frame render count
  ++frame_count_;

  // Tick callback
  tick_observers_.Notify();

  // Switch to primary fiber
  fiber_switch(cc_->primary_fiber);
}

int RenderScreenImpl::DetermineRepeatNumberInternal(double delta_rate) {
  smooth_delta_time_ *= 0.8;
  smooth_delta_time_ += std::fmin(delta_rate, 2) * 0.2;

  if (smooth_delta_time_ >= 0.9) {
    elapsed_time_ = 0;
    return std::round(smooth_delta_time_);
  } else {
    elapsed_time_ += delta_rate;
    if (elapsed_time_ >= 1) {
      elapsed_time_ -= 1;
      return 1;
    }
  }

  return 0;
}

void RenderScreenImpl::UpdateWindowViewportInternal() {
  auto window_size = window_->GetSize();

  if (!(window_size == window_size_)) {
    window_size_ = window_size;

    // Resize screen surface
    base::ThreadWorker::PostTask(
        render_worker_,
        base::BindOnce(&GPUResizeSwapchainInternal, agent_, window_size_));
  }

  float window_ratio = static_cast<float>(window_size.x) / window_size.y;
  float screen_ratio = static_cast<float>(resolution_.x) / resolution_.y;

  display_viewport_.width = window_size.x;
  display_viewport_.height = window_size.y;

  if (screen_ratio > window_ratio)
    display_viewport_.height = display_viewport_.width / screen_ratio;
  else if (screen_ratio < window_ratio)
    display_viewport_.width = display_viewport_.height * screen_ratio;

  display_viewport_.x = (window_size.x - display_viewport_.width) / 2.0f;
  display_viewport_.y = (window_size.y - display_viewport_.height) / 2.0f;
}

void RenderScreenImpl::RenderFrameInternal(Diligent::ITexture** render_target) {
  // Submit pending canvas commands
  GetCanvasScheduler()->SubmitPendingPaintCommands();

  // Prepare for rendering context
  DrawableNode::RenderControllerParams controller_params;
  controller_params.device = GetDevice();
  controller_params.screen_buffer = render_target;
  controller_params.screen_size = resolution_;
  controller_params.viewport = resolution_;
  controller_params.origin = base::Vec2i();

  // 1) Execute pre-composite handler
  controller_.BroadCastNotification(DrawableNode::BEFORE_RENDER,
                                    &controller_params);

  // 1.5) Update sprite batch
  base::ThreadWorker::PostTask(
      render_worker_, base::BindOnce(&SpriteBatch::SubmitBatchDataAndResetCache,
                                     base::Unretained(GetSpriteBatch()),
                                     controller_params.device));

  // 2) Setup renderpass
  base::ThreadWorker::PostTask(
      render_worker_, base::BindOnce(&GPUFrameBeginRenderPassInternal, agent_,
                                     render_target, brightness_));

  // 3) Notify render a frame
  controller_params.root_world = agent_->world_transform.RawDblPtr();
  controller_params.world_binding = controller_params.root_world;
  controller_.BroadCastNotification(DrawableNode::ON_RENDERING,
                                    &controller_params);

  // 4) End render pass and process after-render effect
  base::ThreadWorker::PostTask(
      render_worker_,
      base::BindOnce(&GPUFrameEndRenderPassInternal, agent_, brightness_));
}

}  // namespace content

// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <memory>

#include "base/at_exit.h"
#include "base/callback.h"
#include "base/command_line.h"
#include "base/i18n/icu_util.h"
#include "base/macros.h"
#include "base/memory/ptr_util.h"
#include "base/message_loop/message_pump_type.h"
#include "base/power_monitor/power_monitor.h"
#include "base/power_monitor/power_monitor_device_source.h"
#include "base/run_loop.h"
#include "base/task/single_thread_task_executor.h"
#include "base/task/thread_pool/thread_pool_instance.h"
#include "build/build_config.h"
#include "components/viz/host/host_frame_sink_manager.h"
#include "components/viz/service/display_embedder/server_shared_bitmap_manager.h"
#include "components/viz/service/frame_sinks/frame_sink_manager_impl.h"
#include "mojo/core/embedder/embedder.h"
#include "third_party/skia/include/core/SkBlendMode.h"
#include "ui/aura/client/default_capture_client.h"
#include "ui/aura/client/focus_change_observer.h"
#include "ui/aura/client/focus_client.h"
#include "ui/aura/client/window_parenting_client.h"
#include "ui/aura/env.h"
#include "ui/aura/test/test_focus_client.h"
#include "ui/aura/test/test_screen.h"
#include "ui/aura/window.h"
#include "ui/aura/window_delegate.h"
#include "ui/aura/window_observer.h"
#include "ui/aura/window_tree_host.h"
#include "ui/aura/window_tree_host_observer.h"
#include "ui/base/hit_test.h"
#include "ui/base/ime/init/input_method_initializer.h"
#include "ui/compositor/paint_recorder.h"
#include "ui/compositor/test/in_process_context_factory.h"
#include "ui/events/event.h"
#include "ui/gfx/canvas.h"
#include "ui/gfx/geometry/rect.h"
#include "ui/gfx/skia_util.h"
#include "ui/gfx/transform.h"
#include "ui/gl/gl_switches.h"
#include "ui/gl/init/gl_factory.h"

#include "base/feature_list.h"
#include "ui/base/ui_base_features.h"
#include "ui/ozone/public/ozone_platform.h"

#if defined(OS_WIN)
#include "ui/display/win/dpi.h"
#endif

namespace {

class WindowParentingClient : public aura::client::WindowParentingClient {
 public:
  explicit WindowParentingClient(aura::Window* window) : window_(window) {
    aura::client::SetWindowParentingClient(window_, this);
  }

  ~WindowParentingClient() override {
    aura::client::SetWindowParentingClient(window_, nullptr);
  }

  // Overridden from aura::client::WindowParentingClient:
  aura::Window* GetDefaultParent(aura::Window* window,
                                 const gfx::Rect& bounds) override {
    if (!capture_client_) {
      capture_client_ = std::make_unique<aura::client::DefaultCaptureClient>(
          window_->GetRootWindow());
    }
    return window_;
  }

 private:
  aura::Window* window_;

  std::unique_ptr<aura::client::DefaultCaptureClient> capture_client_;

  DISALLOW_COPY_AND_ASSIGN(WindowParentingClient);
};

class WindowDelegate : public aura::WindowDelegate {
 public:
  explicit WindowDelegate(SkColor color) : color_(color) {}

  // Overridden from WindowDelegate:
  gfx::Size GetMinimumSize() const override { return gfx::Size(); }

  gfx::Size GetMaximumSize() const override { return gfx::Size(); }

  void OnBoundsChanged(const gfx::Rect& old_bounds,
                       const gfx::Rect& new_bounds) override {
    window_bounds_ = new_bounds;
  }
  gfx::NativeCursor GetCursor(const gfx::Point& point) override {
    return gfx::kNullCursor;
  }
  int GetNonClientComponent(const gfx::Point& point) const override {
    return HTCAPTION;
  }
  bool ShouldDescendIntoChildForEventHandling(
      aura::Window* child,
      const gfx::Point& location) override {
    return true;
  }
  bool CanFocus() override { return true; }
  void OnCaptureLost() override {}
  void OnPaint(const ui::PaintContext& context) override {
    ui::PaintRecorder recorder(context, window_bounds_.size());
    recorder.canvas()->DrawColor(color_, SkBlendMode::kSrc);
    gfx::Rect r;
    recorder.canvas()->GetClipBounds(&r);
    // Fill with a non-solid color so that the compositor will exercise its
    // texture upload path.
    while (!r.IsEmpty()) {
      r.Inset(2, 2);
      recorder.canvas()->FillRect(r, color_, SkBlendMode::kXor);
    }
  }
  void OnDeviceScaleFactorChanged(float old_device_scale_factor,
                                  float new_device_scale_factor) override {}
  void OnWindowDestroying(aura::Window* window) override {}
  void OnWindowDestroyed(aura::Window* window) override {}
  void OnWindowTargetVisibilityChanged(bool visible) override {}
  bool HasHitTestMask() const override { return false; }
  void GetHitTestMask(SkPath* mask) const override {}

 private:
  SkColor color_;
  gfx::Rect window_bounds_;

  DISALLOW_COPY_AND_ASSIGN(WindowDelegate);
};

class Observer : public aura::WindowTreeHostObserver {
 public:
  explicit Observer(base::OnceClosure quit_closure)
      : quit_closure_(std::move(quit_closure)) {}

  void OnHostCloseRequested(aura::WindowTreeHost* host) override {
    std::move(quit_closure_).Run();
  }

 private:
  base::OnceClosure quit_closure_;
  DISALLOW_COPY_AND_ASSIGN(Observer);
};

class EventHandler : public ui::EventHandler {
 public:
  EventHandler(aura::Window* window, aura::client::FocusClient* focus_client)
      : window_(window), focus_client_(focus_client) {}
  void OnMouseEvent(ui::MouseEvent* event) override {
    if (event->IsLeftMouseButton()) {
      focus_client_->FocusWindow(window_);
    }
  }

  void OnTouchEvent(ui::TouchEvent* event) override {}

  void OnKeyEvent(ui::KeyEvent* event) override { LOG(INFO) << "key event"; }

 private:
  aura::Window* window_;
  aura::client::FocusClient* focus_client_;
};

}  // namespace

int main(int argc, char** argv) {
  base::CommandLine::Init(argc, argv);

  base::AtExitManager exit_manager;

  if (features::IsUsingOzonePlatform()) {
    ui::OzonePlatform::InitParams params;
    params.single_process = true;
    ui::OzonePlatform::InitializeForUI(params);
    ui::OzonePlatform::InitializeForGPU(params);
  }

  mojo::core::Init();

  base::i18n::InitializeICU();

  gl::init::InitializeGLOneOff();

#if defined(OS_WIN)
  display::win::SetDefaultDeviceScaleFactor(1.0f);
#endif

  base::SingleThreadTaskExecutor main_task_executor(base::MessagePumpType::UI);

  base::ThreadPoolInstance::CreateAndStartWithDefaultParams("learn_aura");

  ui::InitializeInputMethodForTesting();

  viz::ServerSharedBitmapManager server_shared_bitmap_manager;
  viz::FrameSinkManagerImpl frame_sink_manager(&server_shared_bitmap_manager);
  viz::HostFrameSinkManager host_frame_sink_manager;

  host_frame_sink_manager.SetLocalManager(&frame_sink_manager);
  frame_sink_manager.SetLocalClient(&host_frame_sink_manager);

  auto context_factory = std::make_unique<ui::InProcessContextFactory>(
      &host_frame_sink_manager, &frame_sink_manager);

  context_factory->set_use_test_surface(false);

  std::unique_ptr<aura::Env> env = aura::Env::CreateInstance();
  env->set_context_factory(context_factory.get());

  base::PowerMonitor::Initialize(
      std::make_unique<base::PowerMonitorDeviceSource>());

  std::unique_ptr<aura::TestScreen> test_screen(
      aura::TestScreen::Create(gfx::Size(600, 600)));
  display::Screen::SetScreenInstance(test_screen.get());

  std::unique_ptr<aura::WindowTreeHost> host(
      test_screen->CreateHostForPrimaryDisplay());

  aura::test::TestFocusClient focus_client(host->window());

  class FocusChangeObserver : public aura::client::FocusChangeObserver {
   public:
    void OnWindowFocused(aura::Window* gained_focus,
                         aura::Window* lost_focus) override {
      if (gained_focus) {
        LOG(INFO) << gained_focus->GetName();
      }
    }
  };
  FocusChangeObserver fo;

  focus_client.AddObserver(&fo);

  WindowParentingClient window_parenting_client(host->window());

  gfx::Rect window1_bounds(100, 100, 400, 400);
  WindowDelegate window_delegate1(SK_ColorBLUE);
  aura::Window window1(&window_delegate1);
  window1.set_id(1);
  window1.SetName("w1");
  window1.Init(ui::LAYER_TEXTURED);
  window1.SetBounds(window1_bounds);
  window1.Show();
  EventHandler eh1(&window1, &focus_client);
  window1.AddPostTargetHandler(&eh1);
  aura::client::ParentWindowWithContext(&window1, host->window(), gfx::Rect());

  gfx::Rect window2_bounds(200, 200, 350, 350);
  WindowDelegate window_delegate2(SK_ColorRED);
  aura::Window window2(&window_delegate2);
  window2.set_id(2);
  window2.SetName("w2");
  window2.Init(ui::LAYER_TEXTURED);
  window2.SetBounds(window2_bounds);
  window2.Show();
  aura::client::ParentWindowWithContext(&window2, host->window(), gfx::Rect());
  // window1.AddChild(&window2);

  gfx::Transform xform;
  xform.Rotate(45);
  window2.SetTransform(xform);

  EventHandler eh2(&window2, &focus_client);
  window2.AddPostTargetHandler(&eh2);

  // gfx::Rect window3_bounds(10, 10, 50, 50);
  // WindowDelegate window_delegate3(SK_ColorGREEN);
  // aura::Window window3(&window_delegate3);
  // window3.set_id(3);
  // window3.Init(ui::LAYER_TEXTURED);
  // window3.SetBounds(window3_bounds);
  // window3.Show();
  // window2.AddChild(&window3);

  host->Show();

  base::RunLoop run_loop;
  Observer observer(run_loop.QuitClosure());
  host->AddObserver(&observer);
  run_loop.Run();
  host->RemoveObserver(&observer);

  ui::ShutdownInputMethodForTesting();

  return 0;
}

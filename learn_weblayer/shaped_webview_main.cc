#include <math.h>
#include "base/at_exit.h"
#include "base/bind.h"
#include "base/strings/stringprintf.h"
#include "base/task/post_task.h"
#include "base/threading/thread_restrictions.h"
#include "base/time/time.h"
#include "include/core/SkColor.h"
#include "ui/display/screen.h"
#include "ui/gfx/animation/animation.h"
#include "ui/gfx/animation/animation_delegate.h"
#include "ui/gfx/animation/linear_animation.h"
#include "ui/gfx/animation/tween.h"
#include "ui/gfx/scoped_canvas.h"
#include "ui/views/background.h"
#include "ui/views/controls/webview/webview.h"
#include "ui/views/layout/fill_layout.h"
#include "ui/views/painter.h"
#include "ui/views/test/desktop_test_views_delegate.h"
#include "ui/views/widget/desktop_aura/desktop_screen.h"
#include "ui/views/widget/widget.h"
#include "ui/views/widget/widget_delegate.h"
#include "ui/wm/core/wm_state.h"
#include "url/gurl.h"
#include "weblayer/browser/browser_context_impl.h"
#include "weblayer/browser/profile_impl.h"
#include "weblayer/browser/tab_impl.h"
#include "weblayer/public/main.h"
#include "weblayer/public/profile.h"
#include "weblayer/public/tab.h"

namespace {

class ColoredView : public views::WidgetDelegateView,
                    public gfx::AnimationDelegate {
 public:
  explicit ColoredView(SkColor color) {
    color_animation_ = std::make_unique<gfx::LinearAnimation>(
        base::TimeDelta::FromSeconds(100), 30, this);

    color_animation_->Start();
  }

  void WindowClosing() override {
    if (close_callback_) {
      std::move(close_callback_).Run();
    }
  }

  void SetOnCloseCallback(base::OnceClosure callback) {
    close_callback_ = std::move(callback);
  }

 private:
  void AnimationProgressed(const gfx::Animation* animation) override {
    double value = animation->GetCurrentValue();
    double rate = sin(value * 100);
    auto c1 = SkColorSetARGB(0xFF, 0xDE, 0xFF, 0x57);
    auto c2 = SkColorSetARGB(0xD8, 0x83, 0xFC, 0xFF);
    auto color = gfx::Tween::ColorValueBetween(rate, c1, c2);
    SetBackground(views::CreateSolidBackground(color));
  }

  base::OnceClosure close_callback_;
  std::unique_ptr<weblayer::Tab> tab_;
  std::unique_ptr<gfx::Animation> color_animation_;
};

class WebBasedView : public views::WidgetDelegateView {
 public:
  explicit WebBasedView(weblayer::Profile* profile) : tab_(CreateTab(profile)) {
    web_view_ = AddChildView(std::make_unique<views::WebView>(nullptr));
    tab_->AttachToView(web_view_);
    SetLayoutManager(std::make_unique<views::FillLayout>());
  }

  void LoadUrl(GURL url) { web_view_->LoadInitialURL(url); }

  views::WebView* GetWebView() { return web_view_; }

 private:
  std::unique_ptr<weblayer::Tab> CreateTab(weblayer::Profile* profile) {
    auto* profileImpl = static_cast<weblayer::ProfileImpl*>(profile);

    content::WebContents::CreateParams create_params(
        profileImpl->GetBrowserContext());
    auto web_contents = content::WebContents::Create(create_params);

    return std::make_unique<weblayer::TabImpl>(profileImpl,
                                               std::move(web_contents));
  }

  views::WebView* web_view_;
  std::unique_ptr<weblayer::Tab> tab_;
};

class WebviewPainter : public views::Painter {
 public:
  static std::unique_ptr<views::Painter> Create() {
    return std::make_unique<WebviewPainter>();
  }

  ~WebviewPainter() override = default;

  gfx::Size GetMinimumSize() const override { return gfx::Size(); }
  void Paint(gfx::Canvas* canvas, const gfx::Size& size) override {
    gfx::ScopedCanvas scoped_canvas(canvas);

    cc::PaintFlags flags;
    flags.setBlendMode(SkBlendMode::kSrc);
    flags.setStyle(cc::PaintFlags::kFill_Style);
    flags.setAntiAlias(true);
    flags.setColor(SK_ColorWHITE);
    canvas->DrawCircle(gfx::Rect(size).CenterPoint(), size.width() / 2, flags);
  }

  static void ApplyTo(views::WebView* web_view) {
    views::NativeViewHost* view_host = web_view->holder();
    if (view_host) {
      view_host->SetCustomMask(
          views::Painter::CreatePaintedLayer(WebviewPainter::Create()));
    }
  }
};

views::Widget* CreateWidget(views::WidgetDelegate* delegate,
                            views::Widget* parent,
                            const gfx::Rect& bounds) {
  if (!parent) {
    return views::Widget::CreateWindowWithContext(delegate, nullptr, bounds);
  }

  views::Widget::InitParams params;
  params.delegate = delegate;
  params.parent = parent->GetNativeView();
  params.context = parent->GetNativeWindow();
  params.type = views::Widget::InitParams::TYPE_POPUP;
  views::Widget* widget = new views::Widget(std::move(params));

  auto parent_origin = parent->GetClientAreaBoundsInScreen().origin();
  gfx::Point pos(parent_origin.x() + bounds.origin().x(),
                 parent_origin.y() + bounds.origin().y());
  widget->SetBounds(gfx::Rect(pos, bounds.size()));
  return widget;
}

class MainDelegateImpl : public weblayer::MainDelegate {
 public:
  void PreMainMessageLoopRun() override {
    wm_state_ = std::make_unique<wm::WMState>();

    views_delegate_ = std::make_unique<views::DesktopTestViewsDelegate>();

    if (!display::Screen::GetScreen()) {
      screen_ = views::CreateDesktopScreen();
      display::Screen::SetScreenInstance(screen_.get());
    }

    profile_ = weblayer::Profile::Create("test_widgets", false);

    root_widget_delegate_ = SetUpWidgets();
  }

  void PostMainMessageLoopRun() override {
    screen_.reset();
    profile_.reset();
    views_delegate_.reset();
    wm_state_.reset();
  }

  void SetMainMessageLoopQuitClosure(base::OnceClosure quit_closure) override {
    DCHECK(root_widget_delegate_);
    root_widget_delegate_->SetOnCloseCallback(std::move(quit_closure));
  }

 private:
  ColoredView* SetUpWidgets() {
    ColoredView* delegate1 = new ColoredView(SK_ColorLTGRAY);
    auto rect1 = gfx::Rect(100, 100, 500, 500);
    views::Widget* widget1 = CreateWidget(delegate1, nullptr, rect1);
    widget1->Show();

    WebBasedView* delegate2 = new WebBasedView(profile_.get());
    auto rect2 = gfx::Rect(50, 50, 300, 300);
    views::Widget* widget2 = CreateWidget(delegate2, widget1, rect2);
    delegate2->LoadUrl(GURL("https://m.naver.com"));
    WebviewPainter::ApplyTo(delegate2->GetWebView());
    widget2->Show();

    WebBasedView* delegate3 = new WebBasedView(profile_.get());
    auto rect3 = gfx::Rect(150, 150, 300, 300);
    views::Widget* widget3 = CreateWidget(delegate3, widget1, rect3);
    delegate3->LoadUrl(GURL("https://deview.kr/2021"));
    WebviewPainter::ApplyTo(delegate3->GetWebView());
    widget3->Show();

    return delegate1;
  }

  std::unique_ptr<display::Screen> screen_;
  std::unique_ptr<wm::WMState> wm_state_;
  std::unique_ptr<views::ViewsDelegate> views_delegate_;
  std::unique_ptr<weblayer::Profile> profile_;
  ColoredView* root_widget_delegate_{nullptr};
};

weblayer::MainParams CreateMainParams() {
  static const base::NoDestructor<MainDelegateImpl> weblayer_delegate;
  weblayer::MainParams params;
  params.delegate = const_cast<MainDelegateImpl*>(&(*weblayer_delegate));
  params.pak_name = "qlabs_common.pak";
  return params;
}

}  // namespace

#if defined(OS_WIN)
int APIENTRY wWinMain(HINSTANCE instance, HINSTANCE, wchar_t*, int) {
  base::AtExitManager exit_manager;
  return weblayer::Main(CreateMainParams(), instance);
}
#else
int main(int argc, const char** argv) {
  base::AtExitManager exit_manager;
  return weblayer::Main(CreateMainParams(), argc, argv);
}
#endif

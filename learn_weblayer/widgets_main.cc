#include "base/at_exit.h"
#include "base/strings/stringprintf.h"
#include "ui/display/screen.h"
#include "ui/views/background.h"
#include "ui/views/test/desktop_test_views_delegate.h"
#include "ui/views/widget/desktop_aura/desktop_screen.h"
#include "ui/views/widget/widget.h"
#include "ui/views/widget/widget_delegate.h"
#include "ui/wm/core/wm_state.h"
#include "weblayer/public/main.h"
#include "weblayer/public/profile.h"

namespace {

class MyWidgetDelegateView : public views::WidgetDelegateView {
 public:
  explicit MyWidgetDelegateView(SkColor color) {
    SetBackground(views::CreateSolidBackground(color));
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
  base::OnceClosure close_callback_;
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
  MyWidgetDelegateView* SetUpWidgets() {
    MyWidgetDelegateView* delegate1 = new MyWidgetDelegateView(SK_ColorRED);
    auto rect1 = gfx::Rect(100, 100, 300, 300);
    views::Widget* widget1 = CreateWidget(delegate1, nullptr, rect1);
    widget1->Show();

    MyWidgetDelegateView* delegate2 = new MyWidgetDelegateView(SK_ColorYELLOW);
    auto rect2 = gfx::Rect(50, 50, 200, 200);
    views::Widget* widget2 = CreateWidget(delegate2, widget1, rect2);
    widget2->Show();

    return delegate1;
  }

  std::unique_ptr<display::Screen> screen_;
  std::unique_ptr<wm::WMState> wm_state_;
  std::unique_ptr<views::ViewsDelegate> views_delegate_;
  std::unique_ptr<weblayer::Profile> profile_;
  MyWidgetDelegateView* root_widget_delegate_{nullptr};
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

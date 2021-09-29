#include "base/at_exit.h"
#include "base/no_destructor.h"
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

using namespace views;

// Null until/unless the default main message loop is running.
base::NoDestructor<base::OnceClosure> g_quit_main_message_loop;

std::ostream& operator<<(std::ostream& o, const gfx::Rect& r) {
  return o << "(" << r.x() << ", " << r.y() << ", " << r.width() << ", "
           << r.height() << ")";
}

class MyView : public views::WidgetDelegateView {
 public:
  explicit MyView(SkColor color) {
    SetBackground(CreateSolidBackground(color));
  }
};

Widget* CreateWidget(WidgetDelegate* delegate,
                     Widget* parent,
                     const gfx::Rect& bounds) {
  Widget::InitParams params;
  params.delegate = delegate;
  params.parent = parent->GetNativeView();
  params.context = parent->GetNativeWindow();
  params.type = views::Widget::InitParams::TYPE_POPUP;
  // 동작 안 함
  // params.bounds = bounds;
  Widget* widget = new Widget(std::move(params));

  // 직접 origin 계산
  auto parent_origin = parent->GetClientAreaBoundsInScreen().origin();
  gfx::Point pos(parent_origin.x() + bounds.origin().x(),
                 parent_origin.y() + bounds.origin().y());
  widget->SetBounds(gfx::Rect(pos, bounds.size()));

  return widget;
}

void SetUp() {
  MyView* v1 = new MyView(SK_ColorRED);
  Widget* w1 = Widget::CreateWindowWithContext(v1, nullptr,
                                               gfx::Rect(100, 100, 300, 300));
  w1->Show();
  LOG(INFO) << "v1: " << v1->GetLocalBounds();

  MyView* v2 = new MyView(SK_ColorYELLOW);
  Widget* w2 = CreateWidget(v2, w1, gfx::Rect(50, 50, 200, 200));
  w2->Show();
  LOG(INFO) << "v2: " << v2->GetLocalBounds();
}

class MainDelegateImpl : public weblayer::MainDelegate {
 public:
  void PreMainMessageLoopRun() override {
    wm_state_ = new wm::WMState;

    if (!display::Screen::GetScreen())
      display::Screen::SetScreenInstance(
          views::CreateDesktopScreen().release());

    views_delegate_ = new views::DesktopTestViewsDelegate();

    InitializeProfile();
    SetUp();
  }

  void PostMainMessageLoopRun() override {
    DestroyProfile();

    delete views_delegate_;
    views_delegate_ = nullptr;

    delete wm_state_;
    wm_state_ = nullptr;
  }

  void SetMainMessageLoopQuitClosure(base::OnceClosure quit_closure) override {
    *g_quit_main_message_loop = std::move(quit_closure);
  }

 private:
  void InitializeProfile() {
    profile_ = weblayer::Profile::Create("test_widgets", false);
  }

  void DestroyProfile() { profile_.reset(); }

  wm::WMState* wm_state_{nullptr};
  views::ViewsDelegate* views_delegate_{nullptr};
  std::unique_ptr<weblayer::Profile> profile_;
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

#include "base/at_exit.h"
#include "base/bind.h"
#include "base/command_line.h"
#include "base/debug/stack_trace.h"
#include "base/location.h"
#include "base/logging.h"
#include "base/message_loop/message_pump_type.h"
#include "base/run_loop.h"
#include "base/task/single_thread_task_executor.h"
#include "base/task/thread_pool/thread_pool_instance.h"
#include "base/threading/thread_task_runner_handle.h"

namespace {

void Async(int n) {
  for (int i = 0; i < 10; ++i) {
    LOG(INFO) << "Async " << 10 * n + 1 + i;
  }
}

void PostTasks() {
  base::ThreadTaskRunnerHandle::Get()->PostTask(FROM_HERE,
                                                base::BindOnce(&Async, 1));
  base::ThreadTaskRunnerHandle::Get()->PostTask(FROM_HERE,
                                                base::BindOnce(&Async, 2));
  base::ThreadTaskRunnerHandle::Get()->PostTask(FROM_HERE,
                                                base::BindOnce(&Async, 3));
}

void PostCallbackTwice(base::RepeatingCallback<void(int)> cb) {
  base::ThreadTaskRunnerHandle::Get()->PostTask(FROM_HERE,
                                                base::BindOnce(cb, 1));
  base::ThreadTaskRunnerHandle::Get()->PostTask(
      FROM_HERE, base::BindOnce(std::move(cb), 1));
  LOG(INFO) << (cb.is_null() ? "cb is null" : "cb is not null");
}

class A : public base::RefCounted<A> {
 public:
  int Func1() {
    LOG(INFO) << "Func1";
    return 1;
  }

  void Func2(int x) { LOG(INFO) << "Func2: " << x; }

  void Func3() { LOG(INFO) << "Func3"; }

 private:
  friend class base::RefCounted<A>;
  ~A() = default;
};

void PostWithResultCallback() {
  LOG(INFO) << "[PostWithResultCallback]";
  auto a = base::MakeRefCounted<A>();
  base::ThreadTaskRunnerHandle::Get()->PostTaskAndReplyWithResult(
      FROM_HERE, base::BindOnce(&A::Func1, a), base::BindOnce(&A::Func2, a));
}

void PastTaskWithMemberMethod() {
  LOG(INFO) << "[PastTaskWithMemberMethod]";
  auto a = base::MakeRefCounted<A>();
  base::ThreadTaskRunnerHandle::Get()->PostTask(
      FROM_HERE, base::BindOnce(&A::Func3, std::move(a)));
}

void Test(base::RepeatingClosure quit) {
  PostTasks();
  PostCallbackTwice(base::BindRepeating(&Async));
  PostWithResultCallback();
  PastTaskWithMemberMethod();

  // Quit after 5 seconds
  base::ThreadTaskRunnerHandle::Get()->PostDelayedTask(
      FROM_HERE, quit, base::TimeDelta::FromSeconds(5));
}

}  // namespace

int main(int argc, char** argv) {
  base::CommandLine::Init(argc, argv);
  base::AtExitManager exit_manager;
  base::debug::EnableInProcessStackDumping();

  // Initialize logging so we can enable VLOG messages.
  logging::LoggingSettings settings;
  logging::InitLogging(settings);
  logging::SetLogItems(false, true, false, false);

  // Build UI thread task executor. This is used by platform
  // implementations for event polling & running background tasks.
  base::SingleThreadTaskExecutor main_task_executor(base::MessagePumpType::UI);
  base::ThreadPoolInstance::CreateAndStartWithDefaultParams("main");

  base::RunLoop run_loop;
  Test(run_loop.QuitClosure());
  run_loop.Run();

  return 0;
}

#include <memory>

#include "base/at_exit.h"
#include "base/bind.h"
#include "base/callback_forward.h"
#include "base/command_line.h"
#include "base/debug/stack_trace.h"
#include "base/location.h"
#include "base/logging.h"
#include "base/message_loop/message_pump_type.h"
#include "base/run_loop.h"
#include "base/strings/string_number_conversions.h"
#include "base/task/single_thread_task_executor.h"
#include "base/task/thread_pool/thread_pool_instance.h"
#include "base/threading/thread_task_runner_handle.h"

namespace {

const char* kGreeting = "greeting";
const char* kRepeat = "repeat";

std::string g_greeting = "hello";
base::RepeatingClosure g_quit_closure;

void Greeting(int count) {
  if (count == 0) {
    if (g_quit_closure) {
      g_quit_closure.Run();
      return;
    }
  }

  LOG(INFO) << g_greeting;

  base::ThreadTaskRunnerHandle::Get()->PostDelayedTask(
      FROM_HERE, base::BindOnce(&Greeting, count - 1),
      base::TimeDelta::FromSeconds(1));
}

}  // namespace

// $ simple_loop --greeting=hi repeat=3
// hi
// hi
// hi
int main(int argc, char** argv) {
  base::CommandLine::Init(argc, argv);
  base::AtExitManager exit_manager;
  base::debug::EnableInProcessStackDumping();

  // Initialize logging so we can enable VLOG messages.
  logging::LoggingSettings settings;
  logging::InitLogging(settings);

  // Read command line params
  const base::CommandLine& cmd_line = *base::CommandLine::ForCurrentProcess();
  std::string greeting = cmd_line.GetSwitchValueASCII(kGreeting);
  if (!greeting.empty()) {
    g_greeting = greeting;
  }

  int repeat_count = 5;
  auto repeat = cmd_line.GetSwitchValueASCII(kRepeat);
  if (!repeat.empty()) {
    base::StringToInt(repeat, &repeat_count);
  }

  // Build UI thread task executor. This is used by platform
  // implementations for event polling & running background tasks.
  base::SingleThreadTaskExecutor main_task_executor(base::MessagePumpType::UI);
  base::ThreadPoolInstance::CreateAndStartWithDefaultParams("Run Loop");

  base::RunLoop run_loop;
  g_quit_closure = run_loop.QuitClosure();

  // Post the first event to be polled out
  Greeting(repeat_count);

  run_loop.Run();

  return 0;
}

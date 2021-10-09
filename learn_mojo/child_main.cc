#include "base/command_line.h"
#include "base/run_loop.h"
#include "base/task/single_thread_task_executor.h"
#include "base/task/thread_pool/thread_pool_instance.h"
#include "base/threading/thread.h"
#include "mojo/core/embedder/embedder.h"
#include "mojo/core/embedder/scoped_ipc_support.h"
#include "mojo/public/cpp/bindings/pending_remote.h"
#include "mojo/public/cpp/bindings/receiver.h"
#include "mojo/public/cpp/platform/platform_channel.h"
#include "mojo/public/cpp/system/invitation.h"
#include "mojo/public/cpp/system/message_pipe.h"
#include "qlabs/learn_mojo/services/math/math_service.h"

int main(int argc, char** argv) {
  base::CommandLine::Init(argc, argv);

  // Get command line
  auto& command_line = *base::CommandLine::ForCurrentProcess();
  LOG(INFO) << "Launched with args: " << command_line.GetArgumentsString();

  // Init log
  logging::LoggingSettings settings;
  logging::InitLogging(settings);
  logging::SetLogItems(true, true, false, false);

  // Init mojo
  mojo::core::Init();
  base::Thread ipc_thread("IPC");
  ipc_thread.StartWithOptions(
      base::Thread::Options(base::MessagePumpType::IO, 0));
  mojo::core::ScopedIPCSupport ipc_support(
      ipc_thread.task_runner(),
      mojo::core::ScopedIPCSupport::ShutdownPolicy::CLEAN);

  // Init message pump
  base::SingleThreadTaskExecutor main_task_executor;
  base::ThreadPoolInstance::CreateAndStartWithDefaultParams("learn_mojo_child");

  // Init loop
  base::RunLoop loop;

  // Accept invitation
  LOG(INFO) << "Accept invitation";
  auto invitation = mojo::IncomingInvitation::Accept(
      mojo::PlatformChannel::RecoverPassedEndpointFromCommandLine(
          command_line));

  // Extract pipe
  auto pipe = invitation.ExtractMessagePipe("math_pipe");

  // Bind pipe to MathService impl.
  math::MathService math_service(
      mojo::PendingReceiver<math::mojom::MathService>(std::move(pipe)));

  loop.Run();

  return 0;
}

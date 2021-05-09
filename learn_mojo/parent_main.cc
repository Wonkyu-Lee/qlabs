#include "base/callback_forward.h"
#include "base/command_line.h"
#include "base/files/file_path.h"
#include "base/path_service.h"
#include "base/task/single_thread_task_executor.h"
#include "base/task/thread_pool/thread_pool_instance.h"
#include "base/threading/thread.h"
#include "mojo/core/embedder/embedder.h"
#include "mojo/core/embedder/scoped_ipc_support.h"
#include "mojo/public/cpp/bindings/pending_remote.h"
#include "mojo/public/cpp/bindings/remote.h"
#include "mojo/public/cpp/platform/platform_channel.h"
#include "mojo/public/cpp/system/invitation.h"
#include "mojo/public/cpp/system/message_pipe.h"
#include "qlabs/learn_mojo/services/math/public/mojom/math_service.mojom.h"

base::RepeatingClosure g_quit_closure;

int main(int argc, char** argv) {
  // Init Log
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
  base::ThreadPoolInstance::CreateAndStartWithDefaultParams("mojo_multi");

  // Init loop
  base::RunLoop loop;
  g_quit_closure = loop.QuitClosure();

  // Prepare channel
  std::string child_name = "mojo_child";
  mojo::PlatformChannel channel;
  base::LaunchOptions options;
  base::FilePath path;
  base::PathService::Get(base::DIR_EXE, &path);
  path = path.AppendASCII(child_name);
  base::CommandLine command_line(path);
  channel.PrepareToPassRemoteEndpoint(&options, &command_line);

  // Launch math_child with channel
  LOG(INFO) << "Launch '" << child_name
            << "' with args: " << command_line.GetArgumentsString()
            << " and options: " << options.real_path;
  base::Process child_process = base::LaunchProcess(command_line, options);
  channel.RemoteProcessLaunchAttempted();

  // Send invitation
  LOG(INFO) << "Send invitation";
  mojo::OutgoingInvitation invitation;
  auto pipe = invitation.AttachMessagePipe("math_pipe");
  mojo::OutgoingInvitation::Send(std::move(invitation), child_process.Handle(),
                                 channel.TakeLocalEndpoint());

  // Bind MathService interface
  mojo::Remote<math::mojom::MathService> math_service(
      mojo::PendingRemote<math::mojom::MathService>(std::move(pipe), 0));

  // Call math::mojom::MathService::Divide()
  LOG(INFO) << "Call math::mojom::MathService::Divide(42, 6)";
  math_service->Divide(42, 6, base::BindOnce([](int32_t quotient) {
                         LOG(INFO) << "Result: " << quotient;
                         g_quit_closure.Run();
                       }));

  loop.Run();

  return 0;
}

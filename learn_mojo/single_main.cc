#include "base/callback_forward.h"
#include "base/command_line.h"
#include "base/task/single_thread_task_executor.h"
#include "base/task/thread_pool/thread_pool_instance.h"
#include "base/threading/thread.h"
#include "mojo/core/embedder/embedder.h"
#include "mojo/public/cpp/bindings/pending_remote.h"
#include "mojo/public/cpp/bindings/remote.h"
#include "qlabs/learn_mojo/services/math/math_service.h"
#include "qlabs/learn_mojo/services/math/public/mojom/math_service.mojom.h"

base::RepeatingClosure g_quit_closure;

int main(int argc, char** argv) {
  // Init Log
  logging::LoggingSettings settings;
  logging::InitLogging(settings);
  logging::SetLogItems(true, true, false, false);

  // Init mojo
  mojo::core::Init();

  // Init message pump
  base::SingleThreadTaskExecutor main_task_executor;
  base::ThreadPoolInstance::CreateAndStartWithDefaultParams("mojo_single");

  // Init loop
  base::RunLoop loop;
  g_quit_closure = loop.QuitClosure();

#if 0
  mojo::MessagePipe pipe;
  auto pending_remote =
      mojo::PendingRemote<math::mojom::MathService>(std::move(pipe.handle0), 0);
  auto pending_receiver =
      mojo::PendingReceiver<math::mojom::MathService>(std::move(pipe.handle1));
  mojo::Remote<math::mojom::MathService> math_service(
      std::move(pending_remote));
  math::MathService math_service_impl(std::move(pending_receiver));
#else
  mojo::Remote<math::mojom::MathService> math_service;
  math::MathService math_service_impl(
      math_service.BindNewPipeAndPassReceiver());
#endif

  // Call math::mojom::MathService::Divide()
  LOG(INFO) << "Call math::mojom::MathService::Divide(42, 6)";
  math_service->Divide(42, 6, base::BindOnce([](int32_t quotient) {
                         LOG(INFO) << "Result: " << quotient;
                         g_quit_closure.Run();
                       }));

  loop.Run();

  return 0;
}

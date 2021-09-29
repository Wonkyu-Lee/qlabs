#include <memory>

#include "base/at_exit.h"
#include "base/bind.h"
#include "base/callback_forward.h"
#include "base/command_line.h"
#include "base/debug/stack_trace.h"
#include "base/files/file_path.h"
#include "base/files/file_util.h"
#include "base/location.h"
#include "base/logging.h"
#include "base/message_loop/message_pump_type.h"
#include "base/path_service.h"
#include "base/run_loop.h"
#include "base/strings/stringprintf.h"
#include "base/task/single_thread_task_executor.h"
#include "base/task/task_traits.h"
#include "base/task/thread_pool.h"
#include "base/task/thread_pool/thread_pool_instance.h"
#include "base/threading/thread_task_runner_handle.h"
#include "net/base/net_errors.h"
#include "net/socket/server_socket.h"
#include "net/socket/tcp_server_socket.h"
#include "net/socket/unix_domain_server_socket_posix.h"
#include "qlabs/learn_http_server/simple_http_handler.h"
#include "qlabs/learn_http_server/simple_http_server.h"

using namespace qlabs;

namespace {

bool GetOrCreateDirectory(const std::string& dir_name, base::FilePath* dir) {
  base::FilePath parent;
  base::PathService::Get(base::DIR_EXE, &parent);
  base::FilePath result_dir = parent.Append(dir_name);
  if (!base::PathExists(result_dir)) {
    if (!base::CreateDirectory(result_dir))
      return false;
  }

  *dir = result_dir;
  return true;
}

void Init() {
  base::ThreadTaskRunnerHandle::Get()->PostTask(FROM_HERE, base::BindOnce([]() {
                                                  // TODO
                                                }));
}

const int kBackLog = 10;

class MySocketFactory : public SimpleSocketFactory {
 public:
  std::unique_ptr<net::ServerSocket> CreateForHttpServer() override {
    return CreateLocalHostServerSocket(3101);
  }

 private:
  std::unique_ptr<net::ServerSocket> CreateLocalHostServerSocket(int port) {
    std::unique_ptr<net::ServerSocket> socket(
        new net::TCPServerSocket(nullptr, net::NetLogSource()));
    if (socket->ListenWithAddressAndPort("127.0.0.1", port, kBackLog) ==
        net::OK)
      return socket;
    if (socket->ListenWithAddressAndPort("::1", port, kBackLog) == net::OK)
      return socket;
    return std::unique_ptr<net::ServerSocket>();
  }
};

}  // namespace

int main(int argc, char** argv) {
  base::CommandLine::Init(argc, argv);
  base::AtExitManager exit_manager;
  base::debug::EnableInProcessStackDumping();

  // Initialize logging so we can enable VLOG messages.
  logging::LoggingSettings settings;
  logging::InitLogging(settings);
  logging::SetLogItems(true, true, false, false);

  // Build UI thread task executor. This is used by platform
  // implementations for event polling & running background tasks.
  base::SingleThreadTaskExecutor main_task_executor(base::MessagePumpType::UI);
  base::ThreadPoolInstance::CreateAndStartWithDefaultParams("Test WebServer");

  base::FilePath socket_dir;
  GetOrCreateDirectory("test_http_server_port", &socket_dir);

  base::FilePath frontend_dir;
  GetOrCreateDirectory("test_http_server_res", &frontend_dir);

  SimpleHttpServer::GetInstance()->Start(std::make_unique<MySocketFactory>(),
                                         socket_dir, frontend_dir);

  base::RunLoop run_loop;
  Init();
  run_loop.Run();

  SimpleHttpServer::GetInstance()->Stop();

  return 0;
}

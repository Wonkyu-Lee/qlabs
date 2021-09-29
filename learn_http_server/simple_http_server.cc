#include "qlabs/learn_http_server/simple_http_server.h"

#include "base/files/file_path.h"
#include "base/logging.h"
#include "base/single_thread_task_runner.h"
#include "qlabs/learn_http_server/simple_http_handler.h"

namespace qlabs {

SimpleHttpServer* SimpleHttpServer::GetInstance() {
  return base::Singleton<SimpleHttpServer>::get();
}

SimpleHttpServer::SimpleHttpServer() = default;
SimpleHttpServer::~SimpleHttpServer() = default;

void SimpleHttpServer::Start(
    std::unique_ptr<SimpleSocketFactory> server_socket_factory,
    const base::FilePath& active_port_output_directory,
    const base::FilePath& frontend_dir) {
  DETACH_FROM_SEQUENCE(sequence_checker_);
  if (http_handler_) {
    LOG(WARNING) << "Server is already started";
    return;
  }

  http_handler_ = std::make_unique<SimpleHttpHandler>(
      std::move(server_socket_factory), active_port_output_directory,
      frontend_dir);
}

void SimpleHttpServer::Stop() {
  DETACH_FROM_SEQUENCE(sequence_checker_);
  http_handler_ = nullptr;
}

}  // namespace qlabs

#include "qlabs/learn_http_server/simple_http_handler.h"

#include "base/bind.h"
#include "base/files/file_path.h"
#include "base/files/file_util.h"
#include "base/logging.h"
#include "base/memory/scoped_refptr.h"
#include "base/single_thread_task_runner.h"
#include "base/strings/string_number_conversions.h"
#include "base/strings/string_util.h"
#include "base/strings/stringprintf.h"
#include "base/task/post_task.h"
#include "base/task/thread_pool.h"
#include "base/threading/thread.h"
#include "base/threading/thread_task_runner_handle.h"
#include "net/base/ip_endpoint.h"
#include "net/base/net_errors.h"
#include "net/base/url_util.h"
#include "net/server/http_server.h"
#include "net/server/http_server_request_info.h"
#include "net/server/http_server_response_info.h"
#include "net/socket/server_socket.h"
#include "url/gurl.h"

namespace qlabs {

namespace {

const base::FilePath::CharType kActivePortFileName[] =
    FILE_PATH_LITERAL("SimpleActivePort");

const char kPublicUrlPrefix[] = "/qlabs/public";
const char kApiUrlPrefix[] = "/qlabs/api";
const char kHandlerThreadName[] = "SimpleHandlerThread";

const int32_t kSendBufferSize = 256 * 1024 * 1024;     // 256Mb
const int32_t kReceiveBufferSize = 100 * 1024 * 1024;  // 100Mb

constexpr net::NetworkTrafficAnnotationTag kTrafficAnnotation =
    net::DefineNetworkTrafficAnnotation("qlabs_http_handler", R"(
      semantics {
        sender: "Simple Http Handler"
        description:
          "It exposes qlabs protocol over websockets."
        destination: OTHER
        destination_other: "The data can be sent to any destination."
      }
      policy {
        cookies_allowed: NO
      })");

bool RequestIsSafeToServe(const net::HttpServerRequestInfo& info) {
  // For browser-originating requests, serve only those that are coming from
  // pages loaded off localhost or fixed IPs.
  std::string header = info.GetHeaderValue("host");
  if (header.empty())
    return true;
  GURL url = GURL("https://" + header);
  return url.HostIsIPAddress() || net::IsLocalHostname(url.host());
}

}  // namespace

static std::string PathWithoutParams(const std::string& path) {
  size_t query_position = path.find('?');
  if (query_position != std::string::npos)
    return path.substr(0, query_position);
  return path;
}

static std::string GetMimeType(const std::string& filename) {
  if (base::EndsWith(filename, ".html", base::CompareCase::INSENSITIVE_ASCII)) {
    return "text/html";
  } else if (base::EndsWith(filename, ".css",
                            base::CompareCase::INSENSITIVE_ASCII)) {
    return "text/css";
  } else if (base::EndsWith(filename, ".js",
                            base::CompareCase::INSENSITIVE_ASCII)) {
    return "application/javascript";
  } else if (base::EndsWith(filename, ".png",
                            base::CompareCase::INSENSITIVE_ASCII)) {
    return "image/png";
  } else if (base::EndsWith(filename, ".gif",
                            base::CompareCase::INSENSITIVE_ASCII)) {
    return "image/gif";
  } else if (base::EndsWith(filename, ".json",
                            base::CompareCase::INSENSITIVE_ASCII)) {
    return "application/json";
  } else if (base::EndsWith(filename, ".svg",
                            base::CompareCase::INSENSITIVE_ASCII)) {
    return "image/svg+xml";
  }
  LOG(ERROR) << "GetMimeType doesn't know mime type for: " << filename
             << " text/plain will be returned";
  return "text/plain";
}

class ServerWrapper : net::HttpServer::Delegate {
 public:
  ServerWrapper(scoped_refptr<base::SingleThreadTaskRunner> task_runner,
                base::WeakPtr<SimpleHttpHandler> handler,
                std::unique_ptr<net::ServerSocket> socket,
                const base::FilePath& frontend_dir)
      : handler_runner_(task_runner),
        handler_(handler),
        server_(new net::HttpServer(std::move(socket), this)),
        frontend_dir_(frontend_dir) {}

  ~ServerWrapper() override = default;

  int GetLocalAddress(net::IPEndPoint* address) {
    return server_->GetLocalAddress(address);
  }

  void AcceptWebSocket(int connection_id,
                       const net::HttpServerRequestInfo& request) {
    server_->SetSendBufferSize(connection_id, kSendBufferSize);
    server_->SetReceiveBufferSize(connection_id, kReceiveBufferSize);
    server_->AcceptWebSocket(connection_id, request, kTrafficAnnotation);
  }

  void SendOverWebSocket(int connection_id, std::string message) {
    server_->SendOverWebSocket(connection_id, std::move(message),
                               kTrafficAnnotation);
  }

  void SendResponse(int connection_id,
                    const net::HttpServerResponseInfo& response) {
    server_->SendResponse(connection_id, response, kTrafficAnnotation);
  }

  void Send200(int connection_id,
               const std::string& data,
               const std::string& mime_type) {
    server_->Send200(connection_id, data, mime_type, kTrafficAnnotation);
  }

  void Send404(int connection_id) {
    server_->Send404(connection_id, kTrafficAnnotation);
  }

  void Send500(int connection_id, const std::string& message) {
    server_->Send500(connection_id, message, kTrafficAnnotation);
  }

  void Close(int connection_id) { server_->Close(connection_id); }

 private:
  // net::HttpServer::Delegate implementation.
  void OnConnect(int connection_id) override {}

  void OnHttpRequest(int connection_id,
                     const net::HttpServerRequestInfo& info) override {
    if (!RequestIsSafeToServe(info)) {
      Send500(
          connection_id,
          "Host header is specified and is not an IP address or localhost.");
      return;
    }

    server_->SetSendBufferSize(connection_id, kSendBufferSize);

    if (!base::StartsWith(info.path, "/qlabs/public/",
                          base::CompareCase::SENSITIVE)) {
      LOG(ERROR) << "Path is not start with "
                 << "/qlabs/public/"
                 << "info.path: " << info.path;
      server_->Send404(connection_id, kTrafficAnnotation);
      return;
    }

    std::string prefix = "/qlabs/";
    size_t prefix_length = prefix.length();
    std::string filename = PathWithoutParams(info.path.substr(prefix_length));
    std::string mime_type = GetMimeType(filename);
    if (!frontend_dir_.empty()) {
      base::FilePath path = frontend_dir_.AppendASCII(filename);
      std::string data;
      base::ReadFileToString(path, &data);
      server_->Send200(connection_id, data, mime_type, kTrafficAnnotation);
      return;
    }

    server_->Send404(connection_id, kTrafficAnnotation);
  }

  void OnWebSocketRequest(int connection_id,
                          const net::HttpServerRequestInfo& info) override {
    handler_runner_->PostTask(
        FROM_HERE, base::BindOnce(&SimpleHttpHandler::OnWebSocketRequest,
                                  handler_, connection_id, info));
  }

  void OnWebSocketMessage(int connection_id, std::string data) override {
    handler_runner_->PostTask(
        FROM_HERE, base::BindOnce(&SimpleHttpHandler::OnWebSocketMessage,
                                  handler_, connection_id, std::move(data)));
  }

  void OnClose(int connection_id) override {}

  scoped_refptr<base::SingleThreadTaskRunner> handler_runner_;
  base::WeakPtr<SimpleHttpHandler> handler_;
  std::unique_ptr<net::HttpServer> server_;
  base::FilePath frontend_dir_;
};

class Agent {
 public:
  Agent(scoped_refptr<base::SingleThreadTaskRunner> task_runner,
        ServerWrapper* server_wrapper,
        int connection_id)
      : task_runner_(task_runner),
        server_wrapper_(server_wrapper),
        connection_id_(connection_id) {}

  void OnMessage(const std::string& data) {
    std::string message =
        base::StringPrintf("[%d] OnMessage(%s)", connection_id_, data.c_str());
    task_runner_->PostTask(FROM_HERE,
                           base::BindOnce(&ServerWrapper::SendOverWebSocket,
                                          base::Unretained(server_wrapper_),
                                          connection_id_, message));
  }

 private:
  const scoped_refptr<base::SingleThreadTaskRunner> task_runner_;
  ServerWrapper* const server_wrapper_;
  const int connection_id_;
};

void TerminateOnDeciatedRunner(
    std::unique_ptr<base::Thread> thread,
    std::unique_ptr<ServerWrapper> server_wrapper,
    std::unique_ptr<SimpleSocketFactory> socket_factory) {
  if (server_wrapper)
    thread->task_runner()->DeleteSoon(FROM_HERE, std::move(server_wrapper));
  if (socket_factory)
    thread->task_runner()->DeleteSoon(FROM_HERE, std::move(socket_factory));
  if (thread) {
    base::ThreadPool::PostTask(
        FROM_HERE,
        {base::WithBaseSyncPrimitives(), base::TaskPriority::BEST_EFFORT},
        BindOnce([](std::unique_ptr<base::Thread>) {}, std::move(thread)));
  }
}

void ServerStartedOnDedicatedRunner(
    base::WeakPtr<SimpleHttpHandler> handler,
    base::Thread* thread,
    ServerWrapper* server_wrapper,
    SimpleSocketFactory* socket_factory,
    std::unique_ptr<net::IPEndPoint> ip_address) {
  if (handler && thread && server_wrapper) {
    handler->ServerStarted(std::unique_ptr<base::Thread>(thread),
                           std::unique_ptr<ServerWrapper>(server_wrapper),
                           std::unique_ptr<SimpleSocketFactory>(socket_factory),
                           std::move(ip_address));
  } else {
    TerminateOnDeciatedRunner(
        std::unique_ptr<base::Thread>(thread),
        std::unique_ptr<ServerWrapper>(server_wrapper),
        std::unique_ptr<SimpleSocketFactory>(socket_factory));
  }
}

void StartServerOnHandlerThread(
    scoped_refptr<base::SingleThreadTaskRunner> handler_runner,
    base::WeakPtr<SimpleHttpHandler> handler,
    std::unique_ptr<base::Thread> thread,
    std::unique_ptr<SimpleSocketFactory> socket_factory,
    const base::FilePath& output_directory,
    const base::FilePath& frontend_dir) {
  std::unique_ptr<ServerWrapper> server_wrapper;
  std::unique_ptr<net::ServerSocket> server_socket =
      socket_factory->CreateForHttpServer();
  std::unique_ptr<net::IPEndPoint> ip_address(new net::IPEndPoint);
  if (server_socket) {
    server_wrapper.reset(new ServerWrapper(
        handler_runner, handler, std::move(server_socket), frontend_dir));
    if (server_wrapper->GetLocalAddress(ip_address.get()) != net::OK)
      ip_address.reset();
  } else {
    ip_address.reset();
  }

  if (ip_address) {
    std::string message = base::StringPrintf(
        "\nSimpleHttpServer listening on ws://%s%s\n"
        "\nSimpleHttpServer publishing on http://%s%s/index.html\n",
        ip_address->ToString().c_str(), kApiUrlPrefix,
        ip_address->ToString().c_str(), kPublicUrlPrefix);
    fprintf(stderr, "%s", message.c_str());
    fflush(stderr);

    // Write this port to a well-known file in the profile directory
    // so Telemetry, ChromeDriver, etc. can pick it up.
    if (!output_directory.empty()) {
      base::FilePath path = output_directory.Append(kActivePortFileName);
      std::string port_target_string =
          base::StringPrintf("%d\n%s", ip_address->port(), kApiUrlPrefix);
      if (base::WriteFile(path, port_target_string.c_str(),
                          static_cast<int>(port_target_string.length())) < 0) {
        LOG(ERROR) << "Error writing active port to file";
      }
    }
  } else {
    LOG(ERROR) << "Cannot start http server.";
    // pass through
    // server_wrapper is null, so TerminateOnDeciatedRunner() will be called
  }

  handler_runner->PostTask(
      FROM_HERE,
      base::BindOnce(&ServerStartedOnDedicatedRunner, std::move(handler),
                     thread.release(), server_wrapper.release(),
                     socket_factory.release(), std::move(ip_address)));
}

SimpleHttpHandler::SimpleHttpHandler(
    std::unique_ptr<SimpleSocketFactory> socket_factory,
    const base::FilePath& output_directory,
    const base::FilePath& frontend_dir)
    : handler_runner_(base::ThreadTaskRunnerHandle::Get()) {
  DETACH_FROM_SEQUENCE(sequence_checker_);

  std::unique_ptr<base::Thread> thread(new base::Thread(kHandlerThreadName));
  base::Thread::Options options;
  options.message_pump_type = base::MessagePumpType::IO;

  if (thread->StartWithOptions(std::move(options))) {
    thread->task_runner()->PostTask(
        FROM_HERE, base::BindOnce(&StartServerOnHandlerThread, handler_runner_,
                                  weak_factory_.GetWeakPtr(), std::move(thread),
                                  std::move(socket_factory), output_directory,
                                  frontend_dir));
  }
}

SimpleHttpHandler::~SimpleHttpHandler() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  connection_to_client_.clear();
  TerminateOnDeciatedRunner(std::move(thread_), std::move(server_wrapper_),
                            std::move(socket_factory_));
}

void SimpleHttpHandler::ServerStarted(
    std::unique_ptr<base::Thread> thread,
    std::unique_ptr<ServerWrapper> server_wrapper,
    std::unique_ptr<SimpleSocketFactory> socket_factory,
    std::unique_ptr<net::IPEndPoint> ip_address) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  thread_ = std::move(thread);
  server_wrapper_ = std::move(server_wrapper);
  socket_factory_ = std::move(socket_factory);
  server_ip_address_ = std::move(ip_address);
}

void SimpleHttpHandler::OnHttpRequest(int connection_id,
                                      const net::HttpServerRequestInfo& info) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  LOG(INFO) << "OnHttpRequest() path:" << info.path;
  LOG(INFO) << "OnHttpRequest() data:" << info.data;
}

void SimpleHttpHandler::OnWebSocketRequest(
    int connection_id,
    const net::HttpServerRequestInfo& info) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  LOG(INFO) << "OnWebSocketRequest() path:" << info.path;
  LOG(INFO) << "OnWebSocketRequest() data:" << info.data;

  if (!thread_)
    return;

  connection_to_client_[connection_id].reset(
      new Agent(thread_->task_runner(), server_wrapper_.get(), connection_id));

  thread_->task_runner()->PostTask(
      FROM_HERE, base::BindOnce(&ServerWrapper::AcceptWebSocket,
                                base::Unretained(server_wrapper_.get()),
                                connection_id, info));
}

void SimpleHttpHandler::OnWebSocketMessage(int connection_id,
                                           std::string data) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  LOG(INFO) << base::StringPrintf("OnWebSocketMessage(%d, %s)", connection_id,
                                  data.c_str());

  auto it = connection_to_client_.find(connection_id);
  if (it != connection_to_client_.end()) {
    it->second->OnMessage(data);
  }
}

}  // namespace qlabs

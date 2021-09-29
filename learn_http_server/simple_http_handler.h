#ifndef QLABS_LEARN_HTTP_SERVER_SIMPLE_HTTP_HANDLER_H_
#define QLABS_LEARN_HTTP_SERVER_SIMPLE_HTTP_HANDLER_H_

#include <map>

#include "base/macros.h"
#include "base/memory/scoped_refptr.h"
#include "base/memory/weak_ptr.h"
#include "base/sequence_checker.h"

namespace base {
class FilePath;
class DictionaryValue;
class Thread;
class Value;
class SingleThreadTaskRunner;
}  // namespace base

namespace content {
class DevToolsManagerDelegate;
class DevToolsSocketFactory;
}  // namespace content

namespace net {
class IPEndPoint;
class HttpServerRequestInfo;
class ServerSocket;
}  // namespace net

namespace qlabs {

class ServerWrapper;
class Agent;

class SimpleSocketFactory {
 public:
  virtual ~SimpleSocketFactory() {}
  virtual std::unique_ptr<net::ServerSocket> CreateForHttpServer() = 0;
};

class SimpleHttpHandler {
 public:
  // TODO: Use current thread hander.
  SimpleHttpHandler(std::unique_ptr<SimpleSocketFactory> socket_factory,
                    const base::FilePath& output_directory,
                    const base::FilePath& frontend_dir);

  ~SimpleHttpHandler();

  void ServerStarted(std::unique_ptr<base::Thread> thread,
                     std::unique_ptr<ServerWrapper> server_wrapper,
                     std::unique_ptr<SimpleSocketFactory> socket_factory,
                     std::unique_ptr<net::IPEndPoint> ip_address);

  void OnHttpRequest(int connection_id, const net::HttpServerRequestInfo& info);
  void OnWebSocketRequest(int connection_id,
                          const net::HttpServerRequestInfo& info);
  void OnWebSocketMessage(int connection_id, std::string data);

 private:
  friend class ServerWrapper;

  // The thread used by the handler to run server socket.
  std::unique_ptr<base::Thread> thread_;
  std::unique_ptr<ServerWrapper> server_wrapper_;
  std::unique_ptr<net::IPEndPoint> server_ip_address_;
  std::unique_ptr<SimpleSocketFactory> socket_factory_;

  scoped_refptr<base::SingleThreadTaskRunner> handler_runner_;
  std::map<int, std::unique_ptr<Agent>> connection_to_client_;

  SEQUENCE_CHECKER(sequence_checker_);
  DISALLOW_COPY_AND_ASSIGN(SimpleHttpHandler);

  base::WeakPtrFactory<SimpleHttpHandler> weak_factory_{this};
};

}  // namespace qlabs

#endif  // QLABS_LEARN_HTTP_SERVER_SIMPLE_HTTP_HANDLER_H_

#ifndef QLABS_LEARN_HTTP_SERVER_SIMPLE_HTTP_SERVER_H_
#define QLABS_LEARN_HTTP_SERVER_SIMPLE_HTTP_SERVER_H_

#include "base/macros.h"
#include "base/memory/singleton.h"
#include "base/sequence_checker.h"

namespace base {
class FilePath;
class SingleThreadTaskRunner;
}  // namespace base

namespace qlabs {

class SimpleSocketFactory;
class SimpleHttpHandler;

class SimpleHttpServer {
 public:
  static SimpleHttpServer* GetInstance();

  void Start(std::unique_ptr<SimpleSocketFactory> server_socket_factory,
             const base::FilePath& active_port_output_directory,
             const base::FilePath& frontend_dir);

  void Stop();

 private:
  friend struct base::DefaultSingletonTraits<SimpleHttpServer>;

  SimpleHttpServer();
  ~SimpleHttpServer();

  std::unique_ptr<SimpleHttpHandler> http_handler_;

  SEQUENCE_CHECKER(sequence_checker_);
};

}  // namespace qlabs

#endif  // QLABS_LEARN_HTTP_SERVER_SIMPLE_HTTP_SERVER_H_

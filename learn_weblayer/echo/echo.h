#ifndef QLABS_LEARN_WEBLAYER_ECHO_ECHO_H_
#define QLABS_LEARN_WEBLAYER_ECHO_ECHO_H_

#include "mojo/public/cpp/bindings/receiver.h"
#include "qlabs/learn_weblayer/echo/echo.mojom-forward.h"
#include "qlabs/learn_weblayer/echo/echo.mojom.h"

namespace qlabs {

class EchoImpl : public echo::mojom::Echo {
 public:
  explicit EchoImpl();
  ~EchoImpl() override;

  void BindInterface(mojo::PendingReceiver<echo::mojom::Echo> pending_receiver);

 private:
  void Execute(const std::string& request, ExecuteCallback callback) override;

  mojo::Receiver<echo::mojom::Echo> receiver_{this};

  DISALLOW_COPY_AND_ASSIGN(EchoImpl);
};

}  // namespace qlabs

#endif  // QLABS_LEARN_WEBLAYER_ECHO_ECHO_H_

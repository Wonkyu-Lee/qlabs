#include "qlabs/learn_weblayer/echo/echo.h"

namespace qlabs {

EchoImpl::EchoImpl() = default;

EchoImpl::~EchoImpl() = default;

void EchoImpl::BindInterface(
    mojo::PendingReceiver<echo::mojom::Echo> pending_receiver) {
  if (receiver_.is_bound())
    receiver_.reset();

  receiver_.Bind(std::move(pending_receiver));
}

void EchoImpl::Execute(const std::string& request, ExecuteCallback callback) {
  std::move(callback).Run(request);
}

}  // namespace qlabs

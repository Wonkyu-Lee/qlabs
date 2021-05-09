#include "qlabs/learn_mojo/services/math/math_service.h"

namespace math {

MathService::MathService(mojo::PendingReceiver<mojom::MathService> receiver)
    : receiver_(this, std::move(receiver)) {}

MathService::~MathService() = default;

void MathService::Divide(int32_t dividend,
                         int32_t divisor,
                         DivideCallback callback) {
  LOG(INFO) << "Run math::MathService::Divide()";
  // Respond with the quotient!
  std::move(callback).Run(dividend / divisor);
}

}  // namespace math

// src/chrome/services/math/math_service.h
#ifndef QLABS_LEARN_MOJO_SERVICES_MATH_METH_SERVICE_H_
#define QLABS_LEARN_MOJO_SERVICES_MATH_METH_SERVICE_H_

#include "base/macros.h"
#include "mojo/public/cpp/bindings/receiver.h"
#include "qlabs/learn_mojo/services/math/public/mojom/math_service.mojom.h"

namespace math {

class MathService : public mojom::MathService {
 public:
  explicit MathService(mojo::PendingReceiver<mojom::MathService> receiver);
  ~MathService() override;
  MathService(const MathService&) = delete;
  MathService& operator=(const MathService&) = delete;

 private:
  // mojom::MathService:
  void Divide(int32_t dividend,
              int32_t divisor,
              DivideCallback callback) override;

  mojo::Receiver<mojom::MathService> receiver_;
};

}  // namespace math

#endif  // QLABS_LEARN_MOJO_SERVICES_MATH_METH_SERVICE_H_

#pragma once

#include <memory>
#include <string>

#include "protocols/pointer_message.h"

namespace gazeora {

// Where touches go to reach the PC's mouse.
class PointerSender {
public:
    virtual ~PointerSender() = default;
    virtual void Send(const PointerEvent& event) = 0;
};

// Factory: the way to the PC for a stream `protocol` (as Player::Start takes it), queued so
// Send never blocks its caller.
std::unique_ptr<PointerSender> CreatePointerSender(const std::string& protocol,
                                                   const std::string& host);

}  // namespace gazeora

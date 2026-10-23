#include "pointer/pointer_sender.h"

#include "pointer/queued_pointer_sender.h"
#include "pointer/udp_pointer_sender.h"
#include "pointer/usb_pointer_sender.h"

namespace gazeora {

std::unique_ptr<PointerSender> CreatePointerSender(const std::string& protocol,
                                                   const std::string& host) {
    std::unique_ptr<PointerSender> inner;
    if (protocol == "usb") {
        inner = std::make_unique<UsbPointerSender>();
    } else {
        inner = std::make_unique<UdpPointerSender>(host);
    }
    return std::make_unique<QueuedPointerSender>(std::move(inner));
}

}  // namespace gazeora

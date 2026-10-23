#include "pointer/usb_pointer_sender.h"

#include "sources/usb_link.h"

namespace gazeora {

void UsbPointerSender::Send(const PointerEvent& event) {
    UsbLink::Get().Write(PointerMessage(event));
}

}  // namespace gazeora

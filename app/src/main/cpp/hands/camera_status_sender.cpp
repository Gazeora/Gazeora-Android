// Copyright (C) 2026 Nithin Aashik
// SPDX-License-Identifier: GPL-3.0-only OR LicenseRef-Gazeora-Commercial

#include "hands/camera_status_sender.h"

namespace gazeora {

void CameraStatusSender::Send(const CameraReport& report) {
    m_link->Write(CameraStatusMessage::Write(report));
}

}  // namespace gazeora

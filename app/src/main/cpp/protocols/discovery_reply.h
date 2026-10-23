#pragma once

#include <cstdint>
#include <map>
#include <string>

namespace gazeora {

// What Gazeora on a PC says about its stream when the app searches the network (Gazeora:
// discovery.h): a flat JSON object, e.g.
// {"service":"gazeora","version":1,"name":"DESKTOP-1","protocol":"udp","port":8554,
//  "path":"/","codec":"h265"}.
struct DiscoveryReply {
    std::string name;
    std::string protocol;  // a protocol id: rtsp-tcp, rtsp-udp, udp, http, usb
    int port = 0;
    std::string path;
    std::string codec;
};

constexpr uint16_t kDiscoveryPort = 8556;
constexpr char kDiscoveryRequest[] = "GAZEORA-DISCOVER 1";

// A flat JSON object's members, strings unescaped and numbers as written. False if `text`
// isn't one: nested objects and arrays aren't taken.
bool ParseFlatJson(const std::string& text, std::map<std::string, std::string>& out);

// False if it isn't a Gazeora reply. `fallbackName` names a PC that doesn't say.
bool ParseDiscoveryReply(const std::string& text, const std::string& fallbackName,
                         DiscoveryReply& out);

}  // namespace gazeora

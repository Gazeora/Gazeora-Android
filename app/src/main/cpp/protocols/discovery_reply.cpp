#include "protocols/discovery_reply.h"

#include <algorithm>
#include <cstdlib>

namespace gazeora {

namespace {

class JsonReader {
public:
    explicit JsonReader(const std::string& text) : m_text(text) {}

    void SkipSpace() {
        while (m_at < m_text.size() && (m_text[m_at] == ' ' || m_text[m_at] == '\t' ||
                                        m_text[m_at] == '\r' || m_text[m_at] == '\n')) {
            ++m_at;
        }
    }

    bool Take(char c) {
        SkipSpace();
        if (m_at < m_text.size() && m_text[m_at] == c) {
            ++m_at;
            return true;
        }
        return false;
    }

    bool AtEnd() {
        SkipSpace();
        return m_at == m_text.size();
    }

    bool String(std::string& out) {
        if (!Take('"')) {
            return false;
        }
        out.clear();
        while (m_at < m_text.size()) {
            char c = m_text[m_at++];
            if (c == '"') {
                return true;
            }
            if (c != '\\') {
                out += c;
                continue;
            }
            if (m_at >= m_text.size()) {
                return false;
            }
            char escaped = m_text[m_at++];
            switch (escaped) {
                case 'b': out += '\b'; break;
                case 'f': out += '\f'; break;
                case 'n': out += '\n'; break;
                case 'r': out += '\r'; break;
                case 't': out += '\t'; break;
                case 'u': {
                    uint32_t code;
                    if (!Hex4(code)) {
                        return false;
                    }
                    if (code >= 0xD800 && code < 0xDC00 && m_text.compare(m_at, 2, "\\u") == 0) {
                        m_at += 2;
                        uint32_t low;
                        if (!Hex4(low) || low < 0xDC00 || low > 0xDFFF) {
                            return false;
                        }
                        code = 0x10000 + ((code - 0xD800) << 10) + (low - 0xDC00);
                    }
                    Utf8(code, out);
                    break;
                }
                default: out += escaped; break;  // " \ /
            }
        }
        return false;
    }

    // A number, true, false or null, as written.
    bool Literal(std::string& out) {
        SkipSpace();
        size_t from = m_at;
        while (m_at < m_text.size() && m_text[m_at] != ',' && m_text[m_at] != '}' &&
               m_text[m_at] != ' ' && m_text[m_at] != '\r' && m_text[m_at] != '\n' &&
               m_text[m_at] != '\t') {
            char c = m_text[m_at];
            if (c == '{' || c == '[' || c == '"') {
                return false;
            }
            ++m_at;
        }
        out = m_text.substr(from, m_at - from);
        return !out.empty();
    }

private:
    bool Hex4(uint32_t& out) {
        if (m_at + 4 > m_text.size()) {
            return false;
        }
        out = 0;
        for (int i = 0; i < 4; ++i) {
            char c = m_text[m_at++];
            out <<= 4;
            if (c >= '0' && c <= '9') out |= uint32_t(c - '0');
            else if (c >= 'a' && c <= 'f') out |= uint32_t(c - 'a' + 10);
            else if (c >= 'A' && c <= 'F') out |= uint32_t(c - 'A' + 10);
            else return false;
        }
        return true;
    }

    static void Utf8(uint32_t code, std::string& out) {
        if (code < 0x80) {
            out += char(code);
        } else if (code < 0x800) {
            out += char(0xC0 | (code >> 6));
            out += char(0x80 | (code & 0x3F));
        } else if (code < 0x10000) {
            out += char(0xE0 | (code >> 12));
            out += char(0x80 | ((code >> 6) & 0x3F));
            out += char(0x80 | (code & 0x3F));
        } else {
            out += char(0xF0 | (code >> 18));
            out += char(0x80 | ((code >> 12) & 0x3F));
            out += char(0x80 | ((code >> 6) & 0x3F));
            out += char(0x80 | (code & 0x3F));
        }
    }

    const std::string& m_text;
    size_t m_at = 0;
};

std::string Member(const std::map<std::string, std::string>& members, const char* name,
                   const std::string& fallback) {
    auto it = members.find(name);
    return it == members.end() || it->second.empty() ? fallback : it->second;
}

}  // namespace

bool ParseFlatJson(const std::string& text, std::map<std::string, std::string>& out) {
    out.clear();
    JsonReader reader(text);
    if (!reader.Take('{')) {
        return false;
    }
    if (reader.Take('}')) {
        return reader.AtEnd();
    }
    do {
        std::string name, value;
        if (!reader.String(name) || !reader.Take(':')) {
            return false;
        }
        reader.SkipSpace();
        if (!reader.String(value) && !reader.Literal(value)) {
            return false;
        }
        out[name] = value;
    } while (reader.Take(','));
    return reader.Take('}') && reader.AtEnd();
}

bool ParseDiscoveryReply(const std::string& text, const std::string& fallbackName,
                         DiscoveryReply& out) {
    std::map<std::string, std::string> members;
    if (!ParseFlatJson(text, members) || Member(members, "service", "") != "gazeora") {
        return false;
    }
    out.protocol = Member(members, "protocol", "");
    if (out.protocol.empty()) {
        return false;
    }
    out.name = Member(members, "name", fallbackName);
    out.port = std::atoi(Member(members, "port", "0").c_str());
    out.path = Member(members, "path", "/");
    out.codec = Member(members, "codec", "h264");
    // The path goes into the request line as it is: a space or line break in it would add to
    // the request, so only visible ASCII after a slash.
    bool plainPath = out.path[0] == '/' &&
                     std::all_of(out.path.begin(), out.path.end(),
                                 [](char c) { return c > 0x20 && c < 0x7F; });
    return plainPath && out.port >= 0 && out.port <= 65535;  // 0: USB, which has none
}

}  // namespace gazeora

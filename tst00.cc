
#include <iostream>
#include <string_view>
#include <chrono>
#include <cstdint>

#define NOMINMAX
#define _WINSOCK_DEPRECATED_NO_WARNINGS
#include <winsock2.h>
#include <windows.h>
#pragma comment(lib, "ws2_32.lib")

namespace udp::inline win32
{
    inline struct initializer : WSAData {
        initializer() {
            auto ret = WSAStartup(MAKEWORD(2, 0), this);
            if (ret != 0) {
                std::cerr << "WSAStartup failed: " << ret << std::endl;
            }
        }
        ~initializer() {
            auto ret = WSACleanup();
            if (ret != 0) {
                std::cerr << "WSACleanup failed: " << ret << std::endl;
            }
        }
    } initializer_singleton;

    struct connection {
    public:
        connection() = delete;
        connection(connection const&) = delete;
        auto& operator=(connection const&) = delete;
        connection(std::string_view addr, uint16_t port) noexcept
            : desc{socket(AF_INET, SOCK_DGRAM, 0)}
            , addr{ .sin_family = AF_INET,
                    .sin_port = htons(port),
                    .sin_addr = { .S_un = { .S_addr = inet_addr(addr.data()) } } }
        {
            if (this->desc == INVALID_SOCKET) {
                std::cerr << "invalid socket: " << WSAGetLastError() << std::endl;
            }
        }
        bool is_open() const noexcept {
            return this->desc != INVALID_SOCKET;
        }
        ~connection() noexcept {
            if (this->is_open()) {
                closesocket(this->desc);
            }
        }

    public:
        int send(std::string_view data) const noexcept {
            auto ret = sendto(this->desc,
                              data.data(),
                              static_cast<int>(data.size()),
                              0,
                              reinterpret_cast<sockaddr const*>(&(this->addr)),
                              static_cast<int>(sizeof (this->addr)));
            if (ret != data.size()) {
                std::cerr << "sendto failed: " << ret << std::endl;
            }
            return ret;
        }

    private:
        SOCKET      desc;
        sockaddr_in addr;
    };
} // udp::win32

#include <iostream>
#include <iomanip>
#include <sstream>
#include <chrono>
#include <thread>
#include <numbers>
#include <cmath>
#include <tuple>

inline namespace aux
{
    template <size_t...> struct seq { };
    template <size_t N, size_t... I> struct gen_seq : gen_seq<N-1, N-1, I...> { };
    template <size_t... I> struct gen_seq<0, I...> : seq<I...> { };
    template <class Ch, class Tuple, size_t... I>
    void print(std::basic_ostream<Ch>& output, Tuple const& t, seq<I...>) noexcept {
        using swallow = int[];
        (void) swallow{0, (void(output << (I==0? "" : ",") << std::get<I>(t)), 0)...};
    }
    template <class Ch, class... Args>
    auto& operator<<(std::basic_ostream<Ch>& output, std::tuple<Args...> const& t) noexcept {
        output.put('{');
        print(output, t, gen_seq<sizeof... (Args)>());
        output.put('}');
        return output;
    }
    template <class Ch, class T>
    auto& operator<<(std::basic_ostream<Ch>& output, std::pair<Ch const*, T> const& x) noexcept {
        return output << "\"" << x.first << "\":" << x.second;
    }

    inline auto epoch_ms_now() noexcept {
        using namespace std::chrono;
        return duration_cast<milliseconds>(system_clock::now().time_since_epoch()).count();
    }
}

int main() {
    using namespace std::literals;
    using std::numbers::pi;
    udp::connection udp_conn("127.0.0.1", 9870);
    for (int i = 0; i < 10000; ++i) {
        std::ostringstream output;
        output << std::setprecision(16);
        output << std::tuple(std::pair("timestamp", epoch_ms_now() * 0.001),
                             std::pair("A", std::tuple(std::pair("x", std::cos(2*pi*i/1000)),
                                                       std::pair("y", std::sin(2*pi*i/1000)))),
                             std::pair("B", std::tuple(std::pair("x", std::cos(2*pi*i/1000)),
                                                       std::pair("y", std::sin(2*pi*i/1000)))))
               << std::endl;
        udp_conn.send(output.str());
        std::this_thread::sleep_for(16.667ms);
        std::cout << output.str();
    }
    return 0;
}

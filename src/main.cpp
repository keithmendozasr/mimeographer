/*
 * Copyright 2017 Keith Mendoza
 * 
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 * 
 *     http://www.apache.org/licenses/LICENSE-2.0
 * 
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#include <folly/init/Init.h>
#include <proxygen/httpserver/HTTPServer.h>
#include <proxygen/httpserver/RequestHandlerFactory.h>

#include "PrimaryHandler.h"

using namespace mimeographer;
using namespace proxygen;

using folly::EventBase;
using folly::EventBaseManager;
using folly::SocketAddress;

using Protocol = HTTPServer::Protocol;

DEFINE_int32(http_port, 11000, "Port to listen on with HTTP protocol");
DEFINE_int32(h2_port, 11002, "Port to listen on with HTTP/2 protocol");
DEFINE_string(ip, "localhost", "IP/Hostname to bind to");
DEFINE_int32(threads, 0, "Number of threads to listen on. Numbers <= 0 "
             "will use the number of cores on this machine.");

namespace mimeographer {

class MimeographerHandlerFactory : public RequestHandlerFactory {
public:
    void onServerStart(folly::EventBase* /*evb*/) noexcept override {}

    void onServerStop() noexcept override {}

    RequestHandler* onRequest(RequestHandler*, HTTPMessage*) noexcept override {
        return new PrimaryHandler();
    }
};

}

int main(int argc, char* argv[]) {
    folly::init(&argc, &argv, true);

    std::vector<HTTPServer::IPConfig> IPs = {
        {SocketAddress(FLAGS_ip, FLAGS_http_port, true), Protocol::HTTP},
        {SocketAddress(FLAGS_ip, FLAGS_h2_port, true), Protocol::HTTP2},
    };

    if (FLAGS_threads <= 0) {
        FLAGS_threads = sysconf(_SC_NPROCESSORS_ONLN);
        CHECK_GT(FLAGS_threads, 0);
    }

    HTTPServerOptions options;
    options.threads = static_cast<size_t>(FLAGS_threads);
    options.idleTimeout = std::chrono::milliseconds(60000);
    options.shutdownOn = {SIGINT, SIGTERM};
    options.enableContentCompression = false;
    options.handlerFactories = RequestHandlerChain()
            .addThen<MimeographerHandlerFactory>()
            .build();
    options.h2cEnabled = true;

    HTTPServer server(std::move(options));
    server.bind(IPs);

    // Start HTTPServer mainloop in a separate thread
    std::thread t([&] () {
        server.start();
    });

    t.join();
    return 0;
}

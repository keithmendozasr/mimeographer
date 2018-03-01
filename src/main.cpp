/*
 * Copyright 2017-present Keith Mendoza
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
#include <iomanip>

#include <folly/init/Init.h>
#include <proxygen/httpserver/HTTPServer.h>
#include <proxygen/httpserver/RequestHandlerFactory.h>

#include <cmark.h>

#include "PrimaryHandler.h"
#include "EditHandler.h"
#include "StaticHandler.h"
#include "UserHandler.h"

using namespace mimeographer;
using namespace proxygen;

using folly::EventBase;
using folly::EventBaseManager;
using folly::SocketAddress;

using Protocol = HTTPServer::Protocol;

DEFINE_int32(http_port, 11000, "Port to listen on with HTTP protocol");
DEFINE_int32(http2_port, 11002, "HTTP2 listen port");
DEFINE_string(ip, "localhost", "IP/Hostname to bind to");
DEFINE_int32(threads, 0, "Number of threads to listen on. Numbers <= 0 "
             "will use the number of cores on this machine.");
DEFINE_string(dbHost, "localhost", "DB server host");
DEFINE_string(dbUser, "", "DB login");
DEFINE_string(dbPass, "", "DB password");
DEFINE_string(dbName, "mimeographer", "Database name");
DEFINE_int32(dbPort, 5432, "DB server port");
DEFINE_string(uploadDest, "/tmp", "Folder to save uploaded files to");
DEFINE_string(hostName, "localhost", "Hostname mimeograph will use");
DEFINE_string(sslcert, "", "SSL Certificate");
DEFINE_string(sslkey, "", "SSL Private key");
DEFINE_string(staticBase, "/tmp", "Location of static files");

namespace mimeographer 
{

class MimeographerHandlerFactory : public RequestHandlerFactory 
{
private:
    const Config &config;

public:
    MimeographerHandlerFactory(const Config &config) : config(config) {}

    void onServerStart(folly::EventBase* /*evb*/) noexcept override
    {
        LOG(INFO) << "Server started";
    }

    void onServerStop() noexcept override 
    {
        LOG(INFO) << "Server stopped";
    }

    RequestHandler* onRequest(RequestHandler* handler, HTTPMessage* message) noexcept override
    {
        VLOG(2) << "Start " << __PRETTY_FUNCTION__;

        auto path = message->getPath();
        RequestHandler *ptr;
        if(message->getPath().substr(0, 5) == "/edit")
        {
            LOG(INFO) << "Processing edit";
            ptr = new EditHandler(config);
        }
        else if(message->getPath().substr(0, 7) == "/static" ||
            message->getPath() == "/favicon.ico")
        {
            LOG(INFO) << "Processing static file";
            ptr = new StaticHandler(config);
        }
        else if(message->getPath().substr(0, 5) == "/user")
        {
            LOG(INFO) << "Processing user";
            ptr = new UserHandler(config);
        }
        else
        {
            LOG(INFO) << "Processing with primary";
            ptr = new PrimaryHandler(config);
        }

        VLOG(2) << "End " << __PRETTY_FUNCTION__;
        return ptr;
    }
};

}

int main(int argc, char* argv[]) 
{
    folly::init(&argc, &argv, true);

    VLOG(2) << "Start " << __PRETTY_FUNCTION__;

    VLOG(3) << "Value of cmark_version: " << cmark_version();
    if(cmark_version() < ((0<<16) | (28 << 8) | 0)) 
        LOG(WARNING) << "cmark version is " << cmark_version_string()
            << ". Your milage may vary.";

    wangle::SSLContextConfig sslConfig;
    sslConfig.isDefault = true;
    sslConfig.setCertificate(FLAGS_sslcert, FLAGS_sslkey, "");

    HTTPServer::IPConfig ipConfig(
        SocketAddress(FLAGS_ip, FLAGS_http_port, true), Protocol::HTTP
    );
    ipConfig.sslConfigs.push_back(sslConfig);

    std::vector<HTTPServer::IPConfig> IPs;
    IPs.push_back(ipConfig);

    if (FLAGS_threads <= 0) 
    {
        FLAGS_threads = sysconf(_SC_NPROCESSORS_ONLN);
        CHECK_GT(FLAGS_threads, 0);
    }

    Config config(
        FLAGS_dbHost, FLAGS_dbUser, FLAGS_dbPass, FLAGS_dbName, FLAGS_dbPort,
        FLAGS_uploadDest, FLAGS_hostName, FLAGS_staticBase
    );

    HTTPServerOptions options;
    options.threads = static_cast<size_t>(FLAGS_threads);
    options.idleTimeout = std::chrono::milliseconds(60000);
    options.shutdownOn = {SIGINT, SIGTERM};
    options.enableContentCompression = false;
    options.handlerFactories = RequestHandlerChain()
        .addThen<MimeographerHandlerFactory>(config)
        .build();
    options.h2cEnabled = true;

    HTTPServer server(std::move(options));
    server.bind(IPs);

    // Start HTTPServer mainloop in a separate thread
    std::thread t([&] () { server.start(); });

    t.join();

    VLOG(2) << "End " << __PRETTY_FUNCTION__;
    return 0;
}

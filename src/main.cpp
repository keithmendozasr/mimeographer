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
#include <fstream>
#include <cerrno>
#include <cstring>

#include <folly/init/Init.h>
#include <proxygen/httpserver/HTTPServer.h>
#include <proxygen/httpserver/RequestHandlerFactory.h>

#include <cmark.h>
#include <json/json.h>

#include "mm_version.h"
#include "PrimaryHandler.h"
#include "EditHandler.h"
#include "StaticHandler.h"
#include "UserHandler.h"
#include "SiteTemplates.h"

using namespace std;
using namespace mimeographer;
using namespace proxygen;

using folly::EventBase;
using folly::EventBaseManager;
using folly::SocketAddress;

using Protocol = HTTPServer::Protocol;

DEFINE_int32(threads, 0, "Number of threads to listen on. Numbers <= 0 "
             "will use the number of cores on this machine.");
DEFINE_string(config, "/etc/mimeographer/mimeographer.cfg", "Configuration file");
DEFINE_bool(adduser, false, "Add a new user");

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
        else if(message->getPath().substr(0, 8) == "/static/" ||
            message->getPath().substr(0,9) == "/uploads/" ||
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
    // So that --version from gflags will print the correct version info
    google::SetVersionString(version());
    folly::init(&argc, &argv, true);

    //folly::init calls google log init stuff for us
    VLOG(2) << "Start " << __PRETTY_FUNCTION__;

    VLOG(3) << "Value of cmark_version: " << cmark_version();
    if(cmark_version() < ((0<<16) | (28 << 8) | 0)) 
        LOG(WARNING) << "cmark version is " << cmark_version_string()
            << ". Your milage may vary.";

    Json::Value cfgRoot;

    {
        //Scope ifstream for auto close when done
        ifstream f(FLAGS_config, ifstream::binary);
        if(!f.is_open())
        {
            int err = errno;
            LOG(FATAL) << "Failed to open configuration file at " << FLAGS_config
                << " Cause: " << strerror(err);
        }

        VLOG(2) << "Config file open. Feed it to parser";

        Json::CharReaderBuilder b;
        JSONCPP_STRING errMsg;
        if(!Json::parseFromStream(b, f, &cfgRoot, &errMsg))
        {
            LOG(FATAL) << "Failed to parse configuration file at " << FLAGS_config
                << " Cause: " << errMsg;
        }
    }

    Config config(
        cfgRoot.get("dbHost", "localhost").asString(),
        cfgRoot.get("dbUser", "").asString(),
        cfgRoot.get("dbPass", "").asString(),
        cfgRoot.get("dbName", "mimeographer").asString(),
        cfgRoot.get("dbPort", 5432).asInt(),
        cfgRoot.get("uploadDest", "/var/lib/mimeographer/uploads").asString(),
        cfgRoot.get("hostName", "localhost").asString(),
        cfgRoot.get("staticBase", "/var/lib/mimeographer").asString()
    );

    if(FLAGS_adduser)
    {
        LOG(INFO) << "Running in add user mode";
        string email, fullname, password, passConfirm;

        cout << "Enter user's email: ";
        getline(cin, email);

        cout << "Enter user's full name: ";
        getline(cin, fullname);

        do
        {
            cout << "Enter user's password: ";
            getline(cin, password);

            cout << "Renter password: ";
            getline(cin, passConfirm);

            if(password != passConfirm)
                cout << "Password mismatch" << endl;
        }while(password != passConfirm);

        VLOG(3) << "Email: " << email
            << "\nPassord: NOT PRINTING"
            << "\nFull name: " << fullname;

        LOG(INFO) << "Adding " << email << " to database";

        UserHandler u(config);
        int exitCode = 0;

        if(u.createLogin(email, password, fullname))
            cout << "User added" << endl;
        else
        {
            cout << "Failed to add user" << endl;
            exitCode = 1;
        }

        return exitCode;
    }
    else
        VLOG(1) << "Running as server";

    auto sslCert = cfgRoot.get("sslcert", "/etc/mimeographer/mimeographer.pem").
        asString();
    auto sslKey = cfgRoot.get("sslkey", "/etc/mimeographer/mimeographer.priv.pem").
        asString();
    auto ip = cfgRoot.get("ip", "localhost").asString();
    auto httpPort = cfgRoot.get("http_port", 11000).asInt();

    try
    {
        SiteTemplates::init(config);
    }
    catch(...)
    {
        LOG(FATAL) << "Error encountered loading site templates";
    }

    wangle::SSLContextConfig sslConfig;
    sslConfig.isDefault = true;
    sslConfig.setCertificate(sslCert, sslKey, "");

    HTTPServer::IPConfig ipConfig(
        SocketAddress(ip, httpPort, true), Protocol::HTTP
    );
    ipConfig.sslConfigs.push_back(sslConfig);

    std::vector<HTTPServer::IPConfig> IPs;
    IPs.push_back(ipConfig);

    if (FLAGS_threads <= 0) 
    {
        FLAGS_threads = sysconf(_SC_NPROCESSORS_ONLN);
        CHECK_GT(FLAGS_threads, 0);
    }


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

    LOG(INFO) << "Mimeographer " << version() << " started";

    // Start HTTPServer mainloop in a separate thread
    std::thread t([&] () { server.start(); });

    t.join();

    VLOG(2) << "End " << __PRETTY_FUNCTION__;
    return 0;
}

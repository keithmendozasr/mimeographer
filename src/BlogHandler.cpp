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
#include <proxygen/httpserver/ResponseBuilder.h>
#include <folly/io/IOBuf.h>

#include "BlogHandler.h"

using namespace std;
using namespace proxygen;
using namespace folly;

namespace mimeographer {
    void BlogHandler::onRequest(unique_ptr<HTTPMessage> headers) noexcept {
        LOG(INFO) << "Handling request from " 
            << headers->getClientIP() << ":"
            << headers->getClientPort()
            << " " << headers->getMethodString()
            << " " << headers->getPath();
    }

    void BlogHandler::onEOM() noexcept {
        ResponseBuilder(downstream_)
            .status(200, "OK")
            .body(string("Hello there from mimeographer"))
            .sendWithEOM();
    }

    void BlogHandler::requestComplete() noexcept {
        LOG(INFO) << "Done processing";
        delete this;
    }

    void BlogHandler::onError(ProxygenError ) noexcept {
        LOG(INFO) << "Error encountered while processing request";
        delete this;
    }
}

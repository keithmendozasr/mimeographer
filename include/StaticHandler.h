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
#pragma once

#include <folly/Memory.h>
#include <folly/File.h>

#include "HandlerBase.h"

namespace mimeographer 
{

class StaticHandler : public HandlerBase {
private:
    std::unique_ptr<folly::File> file_;
    bool readFileScheduled_{false};
    std::atomic<bool> paused_{false};
    bool finished_{false};
    std::string fileName;

    void readFile(folly::EventBase* evb);
    bool checkForCompletion();

public:
    StaticHandler(const Config &config) : HandlerBase(config) {}
    void onBody(std::unique_ptr<folly::IOBuf> body) noexcept override {}
    void onEOM() noexcept override {}
    void onUpgrade(proxygen::UpgradeProtocol proto) noexcept override {}
    void processRequest(){}

    void onRequest(std::unique_ptr<proxygen::HTTPMessage> headers)
        noexcept override;
    void requestComplete() noexcept override;
    void onError(proxygen::ProxygenError err) noexcept override;
    void onEgressPaused() noexcept override;
    void onEgressResumed() noexcept override;
};

}

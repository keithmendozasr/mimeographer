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
#pragma once

#include <proxygen/httpserver/RequestHandler.h>
#include <proxygen/httpserver/ResponseBuilder.h>

#include <regex>
#include <string>
#include <exception>

#include "gtest/gtest.h"

#include "Config.h"
#include "DBConn.h"

namespace mimeographer 
{

class HandlerBase : public proxygen::RequestHandler 
{
    FRIEND_TEST(HandlerBaseTest, buildPageHeader);
    FRIEND_TEST(HandlerBaseTest, buildPageTrailer);
    FRIEND_TEST(HandlerBaseTest, prependResponse);

private:
    const Config &config;
    std::unique_ptr<folly::IOBuf> handlerResponse;
    std::unique_ptr<proxygen::HTTPMessage> requestHeaders;

    std::unique_ptr<folly::IOBuf> buildPageHeader();
    std::unique_ptr<folly::IOBuf> buildPageTrailer();

protected:
    DBConn connectDb();
    inline void prependResponse(std::unique_ptr<folly::IOBuf> buf)
    {
        if(handlerResponse)
        {
            VLOG(3) << "Prepending to existing handlerResponse";
            handlerResponse->prependChain(move(buf));
        }
        else
        {
            VLOG(3) << "Creating new handlerResponse";
            handlerResponse = move(buf);
        }
    }

    inline const std::string & getPath() const
    {
        return requestHeaders->getPath();
    }

    inline const std::string & getMethod() const
    {
        return requestHeaders->getMethodString();
    }

public:
    HandlerBase(const Config &config) : config(config) {};

    void onRequest(std::unique_ptr<proxygen::HTTPMessage> headers)
            noexcept override;
    void onBody(std::unique_ptr<folly::IOBuf> body) noexcept override {};
    void onEOM() noexcept override;
    void onUpgrade(proxygen::UpgradeProtocol proto) noexcept override {};
    void requestComplete() noexcept override;
    void onError(proxygen::ProxygenError err) noexcept override;
    virtual void processRequest() = 0;
};

}

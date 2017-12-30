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

#include <string>
#include <exception>
#include <fstream>
#include <boost/optional.hpp>

#include <proxygen/httpserver/RequestHandler.h>
#include <proxygen/httpserver/ResponseBuilder.h>
#include <proxygen/lib/http/experimental/RFC1867.h>

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
    FRIEND_TEST(HandlerBaseTest, getPostParam);
    FRIEND_TEST(HandlerBaseTest, parseCookies);

private:
    const Config &config;
    std::unique_ptr<folly::IOBuf> handlerResponse;
    std::unique_ptr<proxygen::HTTPMessage> requestHeaders;

    enum PostParamType
    {
        VALUE,
        FILE_UPLOAD
    };

    struct PostParam
    {
        PostParamType type;
        std::string value;
        std::string filename;
        std::string localFilename;
    };

    std::map<std::string, PostParam> postParams;

    class PostBodyCallback : public proxygen::RFC1867Codec::Callback
    {
    private:
        HandlerBase &parent;
        std::ofstream saveFile;
        std::string localFilename, uploadFileParam;

    public:
        explicit PostBodyCallback(HandlerBase &parent) : parent(parent)
        {}

        void onParam(const std::string& name, const std::string& value,
            uint64_t postBytesProcessed);
        int onFileStart(const std::string& name, const std::string& filename,
            std::unique_ptr<proxygen::HTTPMessage> msg,
            uint64_t postBytesProcessed);
        int onFileData(std::unique_ptr<folly::IOBuf> data, 
            uint64_t postBytesProcessed);
        void onFileEnd(bool end, uint64_t postBytesProcessed);

        void onError()
        {
            LOG(ERROR) << "Error encountered parsing POST request body";
            parent.postParams.clear();
        }
    };

    PostBodyCallback pbCallback;
    std::unique_ptr<proxygen::RFC1867Codec> postParser;

    std::unique_ptr<folly::IOBuf> buildPageHeader();
    std::unique_ptr<folly::IOBuf> buildPageTrailer();

    std::map<std::string, std::string> cookieJar;
    void parseCookies(const std::string &cookies) noexcept;

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

    inline void addCookie(const std::string &name, const std::string &value)
    {
        cookieJar[name] = value;
    }

public:
    HandlerBase(const Config &config) : config(config), pbCallback(*this) {};

    void onRequest(std::unique_ptr<proxygen::HTTPMessage> headers)
            noexcept override;

    void onBody(std::unique_ptr<folly::IOBuf> body) noexcept override 
    {
        if(postParser)
            postParser->onIngress(std::move(body));
    };

    void onEOM() noexcept override;
    void onUpgrade(proxygen::UpgradeProtocol proto) noexcept override {};
    void requestComplete() noexcept override;
    void onError(proxygen::ProxygenError err) noexcept override;
    virtual void processRequest() = 0;

    ////
    /// Return the POST param if it exists. Otherwise nullptr
    /// \param name POST param field to look for
    ////
    boost::optional<const PostParam &> getPostParam(const std::string &name) const;
};

}

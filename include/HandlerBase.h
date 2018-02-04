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

#include <string>
#include <exception>
#include <fstream>
#include <boost/optional.hpp>
#include <utility>
#include <vector>

#include <proxygen/httpserver/RequestHandler.h>
#include <proxygen/httpserver/ResponseBuilder.h>
#include <proxygen/lib/http/experimental/RFC1867.h>
#include <folly/io/IOBuf.h>

#include "gtest/gtest_prod.h"

#include "Config.h"
#include "DBConn.h"
#include "UserSession.h"

namespace mimeographer 
{
class HandlerBase : public proxygen::RequestHandler 
{
    FRIEND_TEST(HandlerBaseTest, buildPageHeader);
    FRIEND_TEST(HandlerBaseTest, buildPageTrailer);
    FRIEND_TEST(HandlerBaseTest, prependResponse);
    FRIEND_TEST(HandlerBaseTest, getPostParam);
    FRIEND_TEST(HandlerBaseTest, parseCookies);
    
    FRIEND_TEST(PrimaryHandlerTest, buildFrontPage);
    FRIEND_TEST(PrimaryHandlerTest, renderArticle_header);
    FRIEND_TEST(PrimaryHandlerTest, renderArticle_lists);
    FRIEND_TEST(PrimaryHandlerTest, renderArticle_paragraph);
    FRIEND_TEST(PrimaryHandlerTest, renderArticle_blockquote);
    FRIEND_TEST(PrimaryHandlerTest, renderArticle_link);
    FRIEND_TEST(PrimaryHandlerTest, renderArticle_image);
    FRIEND_TEST(PrimaryHandlerTest, renderArticle_codeblock);
    FRIEND_TEST(PrimaryHandlerTest, renderArticle_htmlblock);
    FRIEND_TEST(PrimaryHandlerTest, renderArticle_htmlinline);
    FRIEND_TEST(PrimaryHandlerTest, renderArticle_em);
    FRIEND_TEST(PrimaryHandlerTest, renderArticle_strong);

protected:
    enum PostParamType
    {
        VALUE,
        FILE_UPLOAD
    };

private:
    std::unique_ptr<folly::IOBuf> handlerResponse;
    std::unique_ptr<proxygen::HTTPMessage> requestHeaders;

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
    std::map<std::string, std::string> cookieJar;

protected:
    const Config &config;
    DBConn db;
    UserSession session;

    std::unique_ptr<folly::IOBuf> buildPageHeader();
    std::unique_ptr<folly::IOBuf> buildPageTrailer();
    void parseCookies(const std::string &cookies) noexcept;

    inline void prependResponse(const std::string &data)
    {
        VLOG(2) << "Start " << __PRETTY_FUNCTION__;

        if(handlerResponse)
        {
            VLOG(3) << "Prepending to existing handlerResponse";
            handlerResponse->prependChain(std::move(folly::IOBuf::copyBuffer(data)));
        }
        else
        {
            VLOG(3) << "Creating new handlerResponse";
            handlerResponse = std::move(folly::IOBuf::copyBuffer(data));
        }

        VLOG(2) << "End " << __PRETTY_FUNCTION__;
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

    inline boost::optional<std::string> getCookie(const std::string &name)
    {
        VLOG(2) << "Start " << __PRETTY_FUNCTION__;

        boost::optional<std::string> retVal = boost::none;
        try
        {
            auto r = cookieJar.at(name);
            VLOG(1) << "Cookie with name " << name << " found";

            VLOG(2) << "End " << __PRETTY_FUNCTION__;
            retVal = r;
        }
        catch(std::out_of_range &e)
        {
            VLOG(1) << "No cookie with name " << name;
        }

        VLOG(2) << "End " << __PRETTY_FUNCTION__;
        return retVal;
    }

    ////
    /// Generate HTML for action buttons outside of the navbar
    /// \param links vector of target/label pairs to generate buttons for
    /// \return std::string of HTML button codes
    ////
    const std::string makeMenuButtons(
        const std::vector<std::pair<std::string, std::string>> &links) const;

public:
    HandlerBase(const Config &config);
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

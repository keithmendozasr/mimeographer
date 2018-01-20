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
#include <regex>

#include <proxygen/httpserver/RequestHandler.h>
#include <proxygen/httpserver/ResponseBuilder.h>
#include <folly/io/async/EventBaseManager.h>
#include <folly/FileUtil.h>
#include <folly/executors/GlobalExecutor.h>

#include "StaticHandler.h"
#include "HandlerError.h"

using namespace std;
using namespace proxygen;

namespace mimeographer {

void StaticHandler::readFile(folly::EventBase* evb) 
{
    VLOG(2) << "Start " << __PRETTY_FUNCTION__;

    folly::IOBufQueue buf;
    while (file_ && !paused_)
    {
        // read 4k-ish chunks and foward each one to the client
        auto data = buf.preallocate(4000, 4000);
        auto rc = folly::readNoInt(file_->fd(), data.first, data.second);
        if (rc < 0)
        {
            // error
            VLOG(4) << "Read error=" << rc;
            file_.reset();
            evb->runInEventBaseThread(
                [this] {
                    LOG(ERROR) << "Error reading file";
                    downstream_->sendAbort();
                }
            );
            break;
        }
        else if (rc == 0)
        {
            // done
            file_.reset();
            VLOG(1) << "Read EOF";
            evb->runInEventBaseThread(
                [this] {
                    ResponseBuilder(downstream_)
                        .sendWithEOM();
                }
            );
            break;
        }
        else
        {
            buf.postallocate(rc);
            evb->runInEventBaseThread(
                [this, body=buf.move()] () mutable {
                    ResponseBuilder(downstream_)
                        .body(std::move(body))
                        .send();
                }
            );
        }
    }

    // Notify the request thread that we terminated the readFile loop
    evb->runInEventBaseThread(
        [this] {
            readFileScheduled_ = false;
            if (!checkForCompletion() && !paused_) 
            {
                VLOG(1) << "Resuming deferred readFile";
                onEgressResumed();
            }
        }
    );

    VLOG(2) << "End " << __PRETTY_FUNCTION__;
}

bool StaticHandler::checkForCompletion()
{
    VLOG(2) << "Start " << __PRETTY_FUNCTION__;

    if (finished_ && !readFileScheduled_)
    {
        VLOG(1) << "deleting StaticHandler";
        delete this;

        VLOG(2) << "End " << __PRETTY_FUNCTION__;
        return true;
    }

    VLOG(2) << "End " << __PRETTY_FUNCTION__;
    return false;
}

void StaticHandler::onRequest(std::unique_ptr<HTTPMessage> headers) noexcept 
{   
    VLOG(2) << "Start " << __PRETTY_FUNCTION__;

    LOG(INFO) << "Handling request from " 
        << headers->getClientIP() << ":"
        << headers->getClientPort()
        << " " << headers->getMethodString()
        << " " << headers->getPath();

    if (headers->getMethod() != HTTPMethod::GET)
    {
        ResponseBuilder(downstream_)
            .status(400, "Bad method")
            .body("Only GET is supported")
            .sendWithEOM();
        return;
    }

    VLOG(1) << "Collect cookies";
    headers->getHeaders().forEachValueOfHeader(HTTPHeaderCode::HTTP_HEADER_COOKIE,
        [this](const string &val)
        {
            parseCookies(val);
            return false;
        });

    VLOG(1) << "Initialize session";
    auto uuid = getCookie("session");
    if(uuid)
    {
        VLOG(2) << "Session cookie available";
        session.initSession(*uuid);
    }
    else
    {
        VLOG(2) << "Session cookie not available";
        session.initSession();
    }
    addCookie("session", session.getUUID());

    try
    {
        static regex parser("/static/(.+)");
        smatch match;

        if(regex_match(headers->getPath(), match, parser))
        {
            fileName = config.staticBase + "/" + match[1].str();
            VLOG(3) << "Local fileName: " << fileName;
            file_ = std::make_unique<folly::File>(fileName.c_str());
        }
        else if(headers->getPath() == "/favicon.ico")
            file_ = std::make_unique<folly::File>(config.staticBase + "/favicon.ico");
        else
            throw HandlerError(404, "File Not Found");
    }
    catch (const std::system_error& ex) 
    {
        LOG(WARNING) << "Error encountered opening file " << fileName
            << ". Cause: " << folly::to<string>(folly::exceptionStr(ex));

        auto response = buildPageHeader();
        string msg("<p>File not found</p>");
        response->prependChain(folly::IOBuf::copyBuffer(msg));
        response->prependChain(buildPageTrailer());

        ResponseBuilder(downstream_)
            .status(404, "Not Found")
            .body(move(response))
            .sendWithEOM();
        return;
    }
    catch (const HandlerError &err)
    {
        LOG(ERROR) << "Bad request for " << headers->getPath();
        auto response = buildPageHeader();
        ostringstream msg;
        msg << "<p>" << err.what() << "</p>";
        response->prependChain(folly::IOBuf::copyBuffer(msg.str()));
        response->prependChain(buildPageTrailer());

        ResponseBuilder(downstream_)
            .status(err.getCode(), err.what())
            .header(HTTP_HEADER_CONTENT_TYPE, "text/html")
            .body(move(response))
            .sendWithEOM();

        VLOG(2) << "End " << __PRETTY_FUNCTION__;
        return;
    }

    ResponseBuilder(downstream_)
        .status(200, "Ok")
        .send();

    // use a CPU executor since read(2) of a file can block
    readFileScheduled_ = true;
    folly::getCPUExecutor()->add(
        std::bind(&StaticHandler::readFile, this,
        folly::EventBaseManager::get()->getEventBase())
    );

    VLOG(2) << "End " << __PRETTY_FUNCTION__;
}

void StaticHandler::onEgressPaused() noexcept 
{
    VLOG(2) << "Start " << __PRETTY_FUNCTION__;

    // This will terminate readFile soon
    VLOG(4) << "StaticHandler paused";
    paused_ = true;
    
    VLOG(2) << "End " << __PRETTY_FUNCTION__;
}

void StaticHandler::onEgressResumed() noexcept 
{
    VLOG(2) << "Start " << __PRETTY_FUNCTION__;

    VLOG(4) << "StaticHandler resumed";
    paused_ = false;
    // If readFileScheduled_, it will reschedule itself
    if (!readFileScheduled_ && file_)
    {
        readFileScheduled_ = true;
        folly::getCPUExecutor()->add(
        std::bind(&StaticHandler::readFile, this,
        folly::EventBaseManager::get()->getEventBase()));
    }
    else
    {
        VLOG(4) << "Deferred scheduling readFile";
    }
    
    VLOG(2) << "End " << __PRETTY_FUNCTION__;
}

void StaticHandler::requestComplete() noexcept 
{
    VLOG(2) << "Start " << __PRETTY_FUNCTION__;

    finished_ = true;
    paused_ = true;
    checkForCompletion();
    
    VLOG(2) << "End " << __PRETTY_FUNCTION__;
}

void StaticHandler::onError(ProxygenError /*err*/) noexcept
{
    VLOG(2) << "Start " << __PRETTY_FUNCTION__;

    finished_ = true;
    paused_ = true;
    checkForCompletion();
    
    VLOG(2) << "End " << __PRETTY_FUNCTION__;
}

}

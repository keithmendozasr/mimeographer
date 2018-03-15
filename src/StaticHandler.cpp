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
#include <regex>

#include <proxygen/httpserver/RequestHandler.h>
#include <proxygen/httpserver/ResponseBuilder.h>
#include <folly/io/async/EventBaseManager.h>
#include <folly/FileUtil.h>
#include <folly/executors/GlobalExecutor.h>

#include "StaticHandler.h"
#include "HandlerError.h"
#include "SiteTemplates.h"

using namespace std;
using namespace proxygen;
using namespace folly;

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
        string path = parsePath(headers->getPath());
        VLOG(3) << "Static file path: " << path;
        if(regex_match(path, match, parser))
        {
            fileName = config.staticBase + "/" + match[1].str();
            VLOG(3) << "Local fileName: " << fileName;
            file_ = std::make_unique<folly::File>(fileName.c_str());
        }
        else if(path == "/favicon.ico")
            file_ = std::make_unique<folly::File>(config.staticBase + "/favicon.ico");
        else
        {
            LOG(WARNING) << "File path not handled";
            throw HandlerError(404, "File Not Found");
        }
    }
    catch (const std::system_error& ex) 
    {
        LOG(WARNING) << "Error encountered opening file " << fileName
            << ". Cause: " << folly::to<string>(folly::exceptionStr(ex));

        auto response = buildPageHeader();
        string msg("<p>File not found</p>");
        response->prependChain(folly::IOBuf::copyBuffer(msg));
        response->prependChain(
            IOBuf::copyBuffer(SiteTemplates::getTemplate("contentclose"))
        );

        ResponseBuilder(downstream_)
            .status(404, "Not Found")
            .header(HTTP_HEADER_CONTENT_TYPE, "text/html")
            .header(HTTP_HEADER_X_XSS_PROTECTION, "1; mode=block")
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
        response->prependChain(
            IOBuf::copyBuffer(SiteTemplates::getTemplate("contentclose"))
        );

        ResponseBuilder(downstream_)
            .status(err.getCode(), err.what())
            .header(HTTP_HEADER_CONTENT_TYPE, "text/html")
            .header(HTTP_HEADER_X_XSS_PROTECTION, "1; mode=block")
            .body(move(response))
            .sendWithEOM();

        VLOG(2) << "End " << __PRETTY_FUNCTION__;
        return;
    }

    ResponseBuilder(downstream_)
        .status(200, "Ok")
        .header(HTTP_HEADER_CONTENT_TYPE, "application/octet-stream") //For now
        .header(HTTP_HEADER_X_FRAME_OPTIONS, "DENY")
        .header(HTTP_HEADER_X_CONTENT_TYPE_OPTIONS, "nosniff")
        .header(HTTP_HEADER_CACHE_CONTROL, "no-cache, no-store, must-revalidate")
        .header(HTTP_HEADER_PRAGMA, "no-cache")
        .header(HTTP_HEADER_X_XSS_PROTECTION, "1; mode=block")
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

string StaticHandler::parsePath(const string &path)
{
    VLOG(2) << "Start " << __PRETTY_FUNCTION__;

    string retVal;
    for(auto p = path.begin(); p != path.end(); p++)
    {
        VLOG(3) << "Value of p: " << *p;
        if(*p == '+')
        {
            VLOG(3) << "Replacing '+' with <space>";
            retVal += ' ';
        }
        else if(*p == '%')
        {
            VLOG(3) << "Handle hex-encoded char";
            p++;
            string tmp;
            tmp += *p;
            p++;
            tmp += *p;
            VLOG(3) << "Value of tmp: " << tmp;
            size_t pos;
            try
            {
                char chr = stoi(tmp, &pos, 16);
                VLOG(3) << "Value of chr: " << (int)chr;
                VLOG(3) << "Value of pos: " << pos;
                if(pos != 2)
                    throw logic_error("Failed to convert " + tmp);
                retVal += chr;
            }
            catch (const logic_error &e)
            {
                LOG(INFO) << "Failed to convert " << tmp << " to integer";
                VLOG(2) << "End " << __PRETTY_FUNCTION__;
                throw HandlerError(400, "Bad Request");
            }
        }
        else
        {
            VLOG(3) << "Append char";
            retVal += *p;
        }
    }

    VLOG(3) << "Value to return: " << retVal;
    VLOG(2) << "End " << __PRETTY_FUNCTION__;
    return move(retVal);
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

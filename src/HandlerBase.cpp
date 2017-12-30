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

#include <string>
#include <exception>
#include <utility>
#include <uuid/uuid.h>
#include <cstring>

#include <glog/logging.h>

#include "HandlerBase.h"
#include "HandlerError.h"
#include "HandlerRedirect.h"

using namespace std;
using namespace proxygen;
using namespace folly;

namespace mimeographer 
{

void HandlerBase::PostBodyCallback::onParam(const std::string& name,
    const std::string& value, uint64_t postBytesProcessed)
{
    VLOG(3) << __PRETTY_FUNCTION__ << " called. Name: " << name
        << " value: " << value << " bytes processed: " << postBytesProcessed;
    parent.postParams[name] = {
        PostParamType::VALUE,
        value
    };
}

int HandlerBase::PostBodyCallback::onFileStart(const std::string& name,
    const std::string& filename, std::unique_ptr<proxygen::HTTPMessage> msg,
    uint64_t postBytesProcessed)
{
    VLOG(1) << __PRETTY_FUNCTION__ << " called. Name: " << name
        << " Filename: " << filename
        << " Content-Type: "
        << msg->getHeaders().getSingleOrEmpty(proxygen::HTTPHeaderCode::HTTP_HEADER_CONTENT_TYPE);
    
    uuid_t uuid;
    uuid_generate_random(uuid);
    char fileuuid[37];
    uuid_unparse_lower(uuid, fileuuid);
    VLOG(3) << "File UUID: " << fileuuid;

    localFilename = parent.config.uploadDest;
    localFilename += (*(localFilename.end()-1) != '/' ? "/" : "");
    localFilename += fileuuid;
    VLOG(3) << "Local filename to use: " << localFilename;

    saveFile.open(localFilename, ios_base::out | ios_base::binary);
    if(!saveFile.is_open())
    {
        LOG(ERROR) << "Failed to open " << localFilename
            << " to store upload file for parameter \"" << name << "\"";
        return -1;
    }

    uploadFileParam = name;
    parent.postParams[uploadFileParam] = {
        PostParamType::FILE_UPLOAD, "", filename, localFilename 
    };

    VLOG(1) << "File " << localFilename << " opened to store file for \""
        << name << "\"";
    return 0;
}

int HandlerBase::PostBodyCallback::onFileData(std::unique_ptr<folly::IOBuf> data,
    uint64_t postBytesProcessed)
{
    if(!saveFile)
    {
        LOG(ERROR) << "Received upload file data when local file not open";
        return -1;
    }

    VLOG(3) << __PRETTY_FUNCTION__ << " called.";
    saveFile.write((const char *)data->data(), data->length());
    if(!saveFile)
    {
        LOG(ERROR) << "Error encountered writing to save file "
            << localFilename << " for field " << uploadFileParam;
        return -1;
    }
    VLOG(1) << "File data saved";
    return 0;
}

void HandlerBase::PostBodyCallback::onFileEnd(bool end, uint64_t postBytesProcessed)
{
    if(!saveFile)
        LOG(ERROR) << "Received file upload complete when local file not open";
    else if(end)
    {
        VLOG(1) << "Done saving " << uploadFileParam << " data to " << localFilename;
        saveFile.close();
    }
    else
    {
        LOG(INFO) << "Error encountered receiving upload file for "
            << uploadFileParam;
        parent.postParams.erase(uploadFileParam);
    }
}

DBConn HandlerBase::connectDb()
{
    return move(DBConn(config.dbUser, config.dbPass, config.dbHost,
        config.dbName, config.dbPort));
}

unique_ptr<IOBuf> HandlerBase::buildPageHeader() 
{
    static const string templateHeader = 
        "<!doctype html>\n"
        "<html lang=\"en\">\n"
        "<head>\n"
        "<title>Mimeographer</title>\n"
        "<meta charset=\"utf-8\">\n"
        "<meta name=\"viewport\" content=\"width=device-width, "
            "initial-scale=1, shrink-to-fit=no\">\n"
        "<link rel=\"stylesheet\" "
            "href=\"https://maxcdn.bootstrapcdn.com/bootstrap/4.0.0-beta.2/css/bootstrap.min.css\" "
            "integrity=\"sha384-PsH8R72JQ3SOdhVi3uxftmaW6Vc51MKb0q5P2rRUpPvrszuE4W1povHYgTpBfshb\" "
            "crossorigin=\"anonymous\">\n"
        "</head>\n"
        "<body>\n"

        // Navbar
        "<nav class=\"navbar navbar-expand-lg navbar-expand-xl navbar-dark bg-dark\">"
        "<a class=\"navbar-brand\" href=\"/\">Mimeographer</a>"
        "<button class=\"navbar-toggler\" type=\"button\" "
            "data-toggle=\"collapse\" data-target=\"#navbarSupportedContent\" "
            "aria-controls=\"navbarNav\" aria-expanded=\"false\" "
            "aria-label=\"Toggle navigation\">"
        "<span class=\"navbar-toggler-icon\"></span></button>"
        "<div class=\"collapse navbar-collapse\" id=\"navbarNav\">"
            "<div class=\"navbar-nav\">"
                "<a class=\"nav-item nav-link\" href=\"/about\">Archives</a>"
                "<a class=\"nav-item nav-link\" href=\"/about\">About</a>"
            "</div>"
        "</div>"
        "</nav>"

        // Opening display container
        "<div class=\"container-fluid\">\n"
        "<div class=\"row\">\n" // Main page row
        "<div class=\"col col-10 offset-1\">\n" // Main page column
        "<!-- BEGIN PAGE CONTENT -->\n";

    // NOTE: Any other sections of the page should be its own IOBuf and
    // appended to response outside of this section
    return move(IOBuf::copyBuffer(templateHeader));
}

unique_ptr<IOBuf> HandlerBase::buildPageTrailer() 
{
    // NOTE: This section is intended to close out the page itself. 
    static const string templateTail = 
        "<!-- END PAGE CONTENT -->\n"
        "</div>\n" // Closing main page column
        "</div>\n" // Closing main page row
        "</div>\n" //Closing main container
        "<script src=\"https://code.jquery.com/jquery-3.2.1.slim.min.js\" "
            "integrity=\"sha384-KJ3o2DKtIkvYIK3UENzmM7KCkRr/rE9/Qpg6aAZGJwFDMVNA/GpGFF93hXpG5KkN\" "
            "crossorigin=\"anonymous\"></script>\n"
        "<script src=\"https://cdnjs.cloudflare.com/ajax/libs/popper.js/1.12.3/umd/popper.min.js\" "
            "integrity=\"sha384-vFJXuSJphROIrBnz7yo7oB41mKfc8JzQZiCq4NCceLEaO4IHwicKwpJf9c9IpFgh\" "
            "crossorigin=\"anonymous\"></script>\n"
        "<script src=\"https://maxcdn.bootstrapcdn.com/bootstrap/4.0.0-beta.2/js/bootstrap.min.js\" "
            "integrity=\"sha384-alpBpkh1PFOepccYVYDB4do5UnbKysX5WZXm3XxPqe5iKTfUKjNkCk9SaVuEZflJ\" "
            "crossorigin=\"anonymous\"></script>\n"
        "</body>\n"
        "</html>";
    return move(IOBuf::copyBuffer(templateTail));
}

void HandlerBase::parseCookies(const string &cookies) noexcept
{
    VLOG(2) << "Start " << __PRETTY_FUNCTION__;
    char *cookiePartSave;
    auto cookie = strtok_r(const_cast<char *>(cookies.c_str()), "; ", &cookiePartSave);
    while(cookie)
    {
        VLOG(3) << "Cookie part: " << cookie;
        char *nameValSave;
        auto name = strtok_r(cookie, "=", &nameValSave);
        auto val = strtok_r(nullptr, "=", &nameValSave);
        VLOG(3) << "Cookie name: " << name << " value: " << val;
        cookieJar[name] = val;
        cookie = strtok_r(nullptr, "; ", &cookiePartSave);
    }
    VLOG(2) << "End " << __PRETTY_FUNCTION__;
}

void HandlerBase::onRequest(unique_ptr<HTTPMessage> headers) noexcept 
{
    LOG(INFO) << "Handling request from " 
        << headers->getClientIP() << ":"
        << headers->getClientPort()
        << " " << headers->getMethodString()
        << " " << headers->getPath();

    auto method = headers->getMethod();
    if(method && method == HTTPMethod::POST)
    {
        VLOG(1) << "Processing POST request";
        auto val = headers->getHeaders().getSingleOrEmpty(
            HTTPHeaderCode::HTTP_HEADER_CONTENT_TYPE);
        VLOG(3) << "Val: " << val;
        if(val.find("multipart/form-data") == 0)
        {
            auto boundary = val.substr(val.find("boundary=")+9);
            VLOG(3) << "Boundary: " << boundary;
            postParser = make_unique<RFC1867Codec>(boundary);
            postParser->setCallback(&pbCallback);
        }
        else
            LOG(INFO) << "Not processing POST request with Content-Type " << val;
    }
    else
        VLOG(1) << "Not POST";

    VLOG(1) << "Collect cookies";
    headers->getHeaders().forEachValueOfHeader(HTTPHeaderCode::HTTP_HEADER_COOKIE,
        [this](const string &val)
        {
            parseCookies(val);
            return false;
        });
    this->requestHeaders = move(headers);
}

void HandlerBase::onEOM() noexcept 
{
    for(auto i = postParams.begin(); i != postParams.end(); i++)
    {
        ostringstream str;
        str << "POST param: " << i->first;
        switch(i->second.type)
        {
        case PostParamType::VALUE:
            str << "\n\tType: value"
                << "\n\tValue: \"" << i->second.value << "\"";
            break;
        default:
            str << "\n\tType: Unknown";
        }
        VLOG(3) << str.str();
    }

    ResponseBuilder builder(downstream_);
    try 
    {
        if(postParser)
            postParser->onIngressEOM();
        processRequest();

        auto response = buildPageHeader();
        response->prependChain(move(handlerResponse));
        response->prependChain(buildPageTrailer());

        // Send the response that everything worked out well
        builder.status(200, "OK")
            .header(HTTP_HEADER_CONTENT_TYPE, "text/html");
        for(auto i : cookieJar)
        {
            VLOG(3) << "Add cookie " << i.first << "=" << i.second;
            string cookie = i.first + "=" + i.second
                + "; HttpOnly; Path=/; Domain=" + config.hostName;
            builder.header(HTTP_HEADER_SET_COOKIE, cookie);
        }
        
        builder.body(std::move(response))
            .sendWithEOM();
    }
    catch (const HandlerRedirect &e)
    {
        LOG(INFO) << "Redirecting user to " << e.getLocation();
        builder.status(e.getCode(), e.getStatusText())
            .header(HTTP_HEADER_LOCATION, e.getLocation())
            .sendWithEOM();
    }
    catch (const HandlerError &err)
    {
        auto response = buildPageHeader();
        response->prependChain(IOBuf::copyBuffer(err.what()));
        buildPageTrailer();

        builder.status(err.getCode(), err.what())
            .header(HTTP_HEADER_CONTENT_TYPE, "text/html")
            .body(move(response))
            .sendWithEOM();
    }
    catch (const exception &e) 
    {
        //Something went terribly wrong
        LOG(ERROR) << "Exception encountered processing request: "
            << e.what();

        auto response = buildPageHeader();
        response->prependChain(IOBuf::copyBuffer("<p>Something went really wrong</p>"));
        response->prependChain(buildPageTrailer());

        builder.status(500, "Internal error")
            .header(HTTP_HEADER_CONTENT_TYPE, "text/html")
            .body(move(response))
            .sendWithEOM();
    }
}

void HandlerBase::requestComplete() noexcept 
{
    LOG(INFO) << "Done processing";
    delete this;
}

void HandlerBase::onError(ProxygenError ) noexcept 
{
    LOG(INFO) << "Error encountered while processing request";
    delete this;
}

boost::optional<const HandlerBase::PostParam &> HandlerBase::getPostParam(const std::string &name) const
{
    PostParam retVal;
    try
    {
        return postParams.at(name);
    }
    catch(out_of_range &e)
    {
        LOG(WARNING) << "POST param " << name << " does not exist";
    }

    return boost::none;
}

}

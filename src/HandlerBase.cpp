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
    VLOG(2) << "Start " << __PRETTY_FUNCTION__;
    
    VLOG(3)<< "Param name: " << name
        << " value: " << value << " bytes processed: " << postBytesProcessed;
    parent.postParams[name] = {
        PostParamType::VALUE,
        value
    };
    
    VLOG(2) << "End " << __PRETTY_FUNCTION__;
}

int HandlerBase::PostBodyCallback::onFileStart(const std::string& name,
    const std::string& filename, std::unique_ptr<proxygen::HTTPMessage> msg,
    uint64_t postBytesProcessed)
{
    VLOG(2) << "Start " << __PRETTY_FUNCTION__;

    VLOG(3) << " Upload param name: " << name
        << " Filename: " << filename
        << " Content-Type: "
        << msg->getHeaders().getSingleOrEmpty(proxygen::HTTPHeaderCode::HTTP_HEADER_CONTENT_TYPE);
    
    uuid_t uuid;
    uuid_generate_random(uuid);
    char fileuuid[37];
    uuid_unparse_lower(uuid, fileuuid);
    VLOG(3) << "File UUID: " << fileuuid;

    localFilename = parent.config.staticBase
        + (*(parent.config.staticBase.end()-1) != '/' ? "/" : "")
        + parent.config.uploadDest + (*(parent.config.uploadDest.end()-1) != '/' ? "/" : "")
        + fileuuid + "_" + filename;
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

    VLOG(2) << "End " << __PRETTY_FUNCTION__;
    return 0;
}

int HandlerBase::PostBodyCallback::onFileData(std::unique_ptr<folly::IOBuf> data,
    uint64_t postBytesProcessed)
{
    VLOG(2) << "Start " << __PRETTY_FUNCTION__;

    if(!saveFile)
    {
        LOG(ERROR) << "Received upload file data when local file not open";
        return -1;
    }

    saveFile.write((const char *)data->data(), data->length());
    if(!saveFile)
    {
        LOG(ERROR) << "Error encountered writing to save file "
            << localFilename << " for field " << uploadFileParam;
        return -1;
    }
    VLOG(1) << "File data saved";

    VLOG(2) << "End " << __PRETTY_FUNCTION__;
    return 0;
}

void HandlerBase::PostBodyCallback::onFileEnd(bool end, uint64_t postBytesProcessed)
{
    VLOG(2) << "Start " << __PRETTY_FUNCTION__;

    if(!saveFile)
        LOG(ERROR) << "Received file upload complete when local file not open";
    else if(end)
    {
        VLOG(1) << "Done saving " << uploadFileParam << " data to " << localFilename;
        saveFile.close();
    }
    else
    {
        LOG(WARNING) << "Error encountered receiving upload file for "
            << uploadFileParam;
        parent.postParams.erase(uploadFileParam);
    }

    VLOG(2) << "End " << __PRETTY_FUNCTION__;
}

const std::string HandlerBase::generateDropdownMenu(const std::string &id,
    const std::string &label, 
    const std::vector<std::pair<std::string, std::string>> &items) const
{
    string retVal =
    "<li class=\"nav-item dropdown\">\n"
        "<a class=\"nav-link dropdown-toggle\" href=\"#\" "
            "id=\"" + id + "\" role=\"button\" data-toggle=\"dropdown\" "
            "aria-haspopup=\"true\" aria-expanded=\"false\">" +
            label + "\n"
        "</a>\n"
        "<div class=\"dropdown-menu\" aria-labelledby=\"" + id + "\">\n";
    for(auto i : items)
    {
        retVal += "<a class=\"dropdown-item\" href=\"" + i.first + "\">" +
            i.second + "</a>\n";
    }
    retVal += "</div>\n</li>\n";

    return move(retVal);
}

unique_ptr<IOBuf> HandlerBase::buildPageHeader() 
{
    VLOG(2) << "Start " << __PRETTY_FUNCTION__;

    string templateHeader = 
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
        "<nav class=\"navbar navbar-expand-lg navbar-expand-xl navbar-dark bg-dark\">\n"
        "<a class=\"navbar-brand\" href=\"/\">Mimeographer</a>\n"
        "<button class=\"navbar-toggler\" type=\"button\" "
            "data-toggle=\"collapse\" data-target=\"#navbarNav\" "
            "aria-controls=\"navbarNav\" aria-expanded=\"false\" "
            "aria-label=\"Toggle navigation\">\n"
        "<span class=\"navbar-toggler-icon\"></span></button>\n"
        "<div class=\"collapse navbar-collapse\" id=\"navbarNav\">\n"
            "<ul class=\"navbar-nav\">\n"
                "<li class=\"nav-item\">\n"
                    "<a class=\"nav-link\" href=\"/about\">Archives</a>\n"
                "</li>\n"
                "<li class=\"nav-item\">\n"
                    "<a class=\"nav-link\" href=\"/about\">About</a>\n"
                "</li>\n";

    if(session.userAuthenticated())
    {
        VLOG(1) << "User authenticated, add editor menu items";
        templateHeader += generateDropdownMenu(
            "artMgtDropdown", "Article Management",
            {
                { "/edit/new", "New Article" },
                { "/edit/article", "Edit Article" },
                { "/edit/upload", "Upload Image" },
                { "/edit/viewupload", "View uploads" }
            }) + 
            generateDropdownMenu(
                "userDropdown", "User",
                {
                    { "/user/logout", "Logout" },
                    { "/user/changepass", "Change password" }
                });
    }
    else
    {
        VLOG(1) << "User not authenticated";
        templateHeader +=
                "<li class=\"nav-item\">\n"
                    "<a class=\"nav-link\" href=\"/user/login\">Login</a>\n"
                "</li>\n";
    }

    templateHeader +=
            "</ul>\n"
        "</div>\n"
        "</nav>\n"

        // Opening display container
        "<div class=\"container-fluid\">\n"
        "<div class=\"row\">\n" // Main page row
        "<div class=\"col col-10 offset-1\">\n" // Main page column
        "<!-- BEGIN PAGE CONTENT -->\n";

    // NOTE: Any other sections of the page should be its own IOBuf and
    // appended to response outside of this section
    VLOG(2) << "End " << __PRETTY_FUNCTION__;
    return move(IOBuf::copyBuffer(templateHeader));
}

unique_ptr<IOBuf> HandlerBase::buildPageTrailer() 
{
    VLOG(2) << "Start " << __PRETTY_FUNCTION__;

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

    VLOG(2) << "End " << __PRETTY_FUNCTION__;
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

const string HandlerBase::makeMenuButtons(const vector<pair<string, string>> &links) const
{
    VLOG(2) << "Start " << __PRETTY_FUNCTION__;
    string retVal;
    for(auto i : links)
    {
        VLOG(1) << "Creating button for " << i.first << "/" << i.second << "pair";
        retVal += string(retVal.size() ? "\n" : "") + "<a href=\"" + i.first + "\" class=\"btn btn-primary\">"
            + i.second + "</a>";
    }
    
    VLOG(1) << "Done crating button set";

    VLOG(2) << "End " << __PRETTY_FUNCTION__;
    return move(retVal);
}

HandlerBase::HandlerBase(const Config &config) :
    pbCallback(*this),
    config(config),
    db(config.dbUser, config.dbPass, config.dbHost, config.dbName,
        config.dbPort),
    session(db)
{
    VLOG(2) << "Start " << __PRETTY_FUNCTION__;
    VLOG(2) << "End " << __PRETTY_FUNCTION__;
}

void HandlerBase::onRequest(unique_ptr<HTTPMessage> headers) noexcept 
{
    VLOG(2) << "Start " << __PRETTY_FUNCTION__;

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
        VLOG(3) << "Content-type header value: " << val;
        if(val.find("multipart/form-data") == 0)
        {
            VLOG(1) << "Content type is multipart/form-data";

            auto boundary = val.substr(val.find("boundary=")+9);
            VLOG(3) << "Boundary: " << boundary;

            postParser = make_unique<RFC1867Codec>(boundary);
            postParser->setCallback(&pbCallback);
        }
        else
            LOG(INFO) << "Not processing POST request with Content-Type " << val;
    }
    else
        VLOG(1) << "Not POST request";

    VLOG(1) << "Collect cookies";
    headers->getHeaders().forEachValueOfHeader(HTTPHeaderCode::HTTP_HEADER_COOKIE,
        [this](const string &val)
        {
            parseCookies(val);
            return false;
        });
    this->requestHeaders = move(headers);

    VLOG(1) << "Initialize session";
    auto uuid = getCookie("session");
    if(uuid)
    {
        VLOG(1) << "Session cookie available";
        session.initSession(*uuid);
    }
    else
    {
        VLOG(1) << "Session cookie not available";
        session.initSession();
    }
    addCookie("session", session.getUUID());

    VLOG(2) << "End " << __PRETTY_FUNCTION__;
}

void HandlerBase::onEOM() noexcept 
{
    VLOG(2) << "Start " << __PRETTY_FUNCTION__;

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
        case PostParamType::FILE_UPLOAD:
            str << "\n\tType: file_upload"
                << "\n\tUpload filename: \"" << i->second.filename << "\""
                << "\n\tLocal filename: \"" << i->second.localFilename << "\"";
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
        if(handlerResponse)
            response->prependChain(move(handlerResponse));
        response->prependChain(buildPageTrailer());

        // Send the response that everything worked out well
        builder.status(200, "OK")
            .header(HTTP_HEADER_CONTENT_TYPE, "text/html");

        VLOG(1) << "Send cookies";
        for(auto i : cookieJar)
        {
            VLOG(3) << "Add cookie " << i.first << "=" << i.second;
            string cookie = i.first + "=" + i.second
                + "; HttpOnly; Path=/; Domain=" + config.hostName;
            builder.header(HTTP_HEADER_SET_COOKIE, cookie);
        }
        
        VLOG(1) << "Send response body";
        builder.body(std::move(response))
            .sendWithEOM();
    }
    catch (const HandlerRedirect &e)
    {
        LOG(INFO) << "Redirecting user to " << e.getLocation();
        builder.status(e.getCode(), e.getStatusText());

        VLOG(1) << "Send cookies";
        for(auto i : cookieJar)
        {
            VLOG(3) << "Add cookie " << i.first << "=" << i.second;
            string cookie = i.first + "=" + i.second
                + "; HttpOnly; Path=/; Domain=" + config.hostName;
            builder.header(HTTP_HEADER_SET_COOKIE, cookie);
        }
            
        VLOG(1) << "Send redirect header";
        builder.header(HTTP_HEADER_LOCATION, e.getLocation())
            .sendWithEOM();
    }
    catch (const HandlerError &err)
    {
        LOG(WARNING) << "HandlerError encountered: " << err.what();

        auto response = buildPageHeader();
        ostringstream msg;
        msg << "<p>" << err.what() << "</p>";
        response->prependChain(IOBuf::copyBuffer(msg.str()));
        response->prependChain(buildPageTrailer());

        VLOG(1) << "Send response";
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

        VLOG(1) << "Send response";
        builder.status(500, "Internal error")
            .header(HTTP_HEADER_CONTENT_TYPE, "text/html")
            .body(move(response))
            .sendWithEOM();
    }

    VLOG(2) << "End " << __PRETTY_FUNCTION__;
}

void HandlerBase::requestComplete() noexcept 
{
    VLOG(2) << "Start " << __PRETTY_FUNCTION__;

    LOG(INFO) << "Done processing";

    VLOG(2) << "End " << __PRETTY_FUNCTION__;
    delete this;
}

void HandlerBase::onError(ProxygenError ) noexcept 
{
    VLOG(2) << "Start " << __PRETTY_FUNCTION__;

    LOG(INFO) << "Error encountered while processing request";

    VLOG(2) << "End " << __PRETTY_FUNCTION__;
    delete this;
}

boost::optional<const HandlerBase::PostParam &> HandlerBase::getPostParam(const std::string &name) const
{
    VLOG(2) << "Start " << __PRETTY_FUNCTION__;

    boost::optional<const HandlerBase::PostParam &> retVal = boost::none;
    try
    {
        VLOG(1) << "Check if params exists";
        retVal = postParams.at(name);
    }
    catch(out_of_range &e)
    {
        LOG(WARNING) << "POST param " << name << " does not exist";
    }

    VLOG(2) << "End " << __PRETTY_FUNCTION__;
    return retVal;
}

}

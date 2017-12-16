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
#include <regex>

#include <glog/logging.h>

#include "PrimaryHandler.h"

using namespace std;
using namespace proxygen;
using namespace folly;

namespace mimeographer 
{

DBConn PrimaryHandler::connectDb()
{
    DBConn conn(config.dbUser, config.dbPass, config.dbHost, config.dbName,
        config.dbPort);
    return move(conn);
}

void PrimaryHandler::buildFrontPage()
{
    VLOG(1) << "DB connection established";
    string data;
    auto conn = connectDb();
    for(auto article : conn.getHeadlines())
    {
        auto line = string("<h1><a href=\"/article/") + article[0] + "\">"
            + article[1] + "</a></h1>\n<p>"
            + article[2] + "</p>\n" ;
        if((data.capacity() - data.size() - line.size()) < 0)
        {
            VLOG(2) << "Loading existing list to buffer";
            response->prependChain(move(IOBuf::copyBuffer(data)));
            data = line;
        }
        else
        {
            VLOG(2) << "Append line to data buffer";
            data = data + line;
        }
    }
    response->prependChain(move(IOBuf::copyBuffer(data)));
    VLOG(1) << "Front page data processed";
}

void PrimaryHandler::buildArticlePage()
{
    static regex parser("/article/(\\d+)");
    smatch match;
    if(regex_match(headers->getPath(), match, parser))
    {
        ssub_match id = match[1];
        VLOG(2) << "Article id: " << id.str();
        auto conn = connectDb();
        auto article = conn.getArticle(id.str());
        response->prependChain(move(IOBuf::copyBuffer(
            string("<h1>") + get<0>(article) + "</h1>")));
        for(auto contPart : get<1>(article))
            response->prependChain(move(IOBuf::copyBuffer(contPart)));
    }
    else
    {
        LOG(WARNING) << "path didn't parse";
        throw Status404();
    }
}

void PrimaryHandler::buildPageHeader() 
{
    static const string templateHeader = 
        "<!doctype html>\n"
        "<html lang=\"en\">\n"
        "<head>\n"
        "<title>Hello, world!</title>\n"
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
        "<div class=\"row\">\n"
        "<!-- BEGIN PAGE CONTENT -->\n"; // Main page row

    // NOTE: Any other sections of the page should be its own IOBuf and
    // appended to response outside of this section
    response = std::move(IOBuf::copyBuffer(templateHeader));
}

void PrimaryHandler::buildContent() 
{
    if(response == nullptr) 
    {
        LOG(DFATAL) << "response IOBuf not initialized when "
            << __PRETTY_FUNCTION__ << " called";
        return;
    }

    static const string templateOpening = "<div class=\"col col-10 offset-1\">\n";
    static const string templateClosing = "</div>\n";

    response->prependChain(std::move(IOBuf::copyBuffer(templateOpening)));

    auto path = headers->getPath();
    VLOG(2) << "path.substr(9): " << path.substr(0,9);
    if(path == "/")
    {
        VLOG(1) << "Process front page";
        buildFrontPage();
    }
    else if(path.substr(0,9) == "/article/")
    {
        VLOG(1) << "Process article";
        buildArticlePage();
    }
    else
    {
        LOG(INFO) << path << "not handled";
        throw Status404();
    }
    response->prependChain(std::move(IOBuf::copyBuffer(templateClosing)));
}

void PrimaryHandler::buildPageTrailer() 
{
    // NOTE: If the issue is caused by not calling buildPageHeader() during
    // development it'll be caught by the DFATAL call. In production, the
    // issue could be caused by memory issues.
    if(response == nullptr) 
    {
        LOG(DFATAL) << "response IOBuf not initialized when " 
            << __PRETTY_FUNCTION__ << " called";

        return;
    }

    // NOTE: This section is intended to close out the page itself. 
    static const string templateTail = 
        "<!-- END PAGE CONTENT -->\n"
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
    response->prependChain(std::move(IOBuf::copyBuffer(templateTail)));
}

void PrimaryHandler::onRequest(unique_ptr<HTTPMessage> headers) noexcept 
{
    this->headers = move(headers);
    LOG(INFO) << "Handling request from " 
        << this->headers->getClientIP() << ":"
        << this->headers->getClientPort()
        << " " << this->headers->getMethodString()
        << " " << this->headers->getPath();
    VLOG(1) << "field1 cookie " << this->headers->getCookie("field1").toString();
    VLOG(1) << "field2 cookie " << this->headers->getCookie("field2").toString();
}

void PrimaryHandler::onEOM() noexcept 
{
    try 
    {
        buildPageHeader();
        buildContent();
        buildPageTrailer();

        // Send the response that everything worked out well
        ResponseBuilder(downstream_)
            .status(200, "OK")
            .header(HTTP_HEADER_CONTENT_TYPE, "text/html")
            .header("Set-Cookie", "field1=asdf")
            .header("Set-Cookie", "field2=dddd")
            .body(std::move(response))
            .sendWithEOM();
    }
    catch (const Status404 &)
    {
        if(response == nullptr)
            LOG(DFATAL) << "response IOBuf not initialized at Status404 handler";
        else
            response->prependChain(move(IOBuf::copyBuffer(
                "The path you provided doesn't exists."
            )));
        buildPageTrailer();
        ResponseBuilder(downstream_)
            .status(404, "File Not Found")
            .header(HTTP_HEADER_CONTENT_TYPE, "text/html")
            .body(move(response))
            .sendWithEOM();
    }
    catch (const exception &e) 
    {
        //Something went terribly wrong
        LOG(ERROR) << "Exception encountered processing request: "
            << e.what();

        ResponseBuilder(downstream_)
            .status(500, "Internal error")
            .header(HTTP_HEADER_CONTENT_TYPE, "text/html")
            .body("Something went really wrong")
            .sendWithEOM();
    }
}

void PrimaryHandler::requestComplete() noexcept 
{
    LOG(INFO) << "Done processing";
    delete this;
}

void PrimaryHandler::onError(ProxygenError ) noexcept 
{
    LOG(INFO) << "Error encountered while processing request";
    delete this;
}

}

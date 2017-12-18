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

#include "HandlerBase.h"
#include "HandlerError.h"

using namespace std;
using namespace proxygen;
using namespace folly;

namespace mimeographer 
{

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

void HandlerBase::onRequest(unique_ptr<HTTPMessage> headers) noexcept 
{
    LOG(INFO) << "Handling request from " 
        << headers->getClientIP() << ":"
        << headers->getClientPort()
        << " " << headers->getMethodString()
        << " " << headers->getPath();
    this->requestHeaders = move(headers);
}

void HandlerBase::onEOM() noexcept 
{
    ResponseBuilder builder(downstream_);
    try 
    {
        processRequest();

        auto response = buildPageHeader();
        response->prependChain(move(handlerResponse));
        response->prependChain(buildPageTrailer());

        // Send the response that everything worked out well
        builder.status(200, "OK")
            .header(HTTP_HEADER_CONTENT_TYPE, "text/html")
            .body(std::move(response))
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

}

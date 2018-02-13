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
#include <vector>

#include <glog/logging.h>

#include <folly/io/IOBuf.h>

#include <boost/filesystem.hpp>

#include "EditHandler.h"
#include "HandlerError.h"
#include "HandlerRedirect.h"
#include "SummaryBuilder.h"

using namespace std;
using namespace proxygen;
using namespace folly;
using namespace boost::filesystem;

namespace mimeographer 
{

void EditHandler::buildLoginPage(const bool &showMismatch)
{
    VLOG(2) << "Start " << __PRETTY_FUNCTION__;

	const static string html = 
        "<form method=\"post\" action=\"/edit/login\" enctype=\"multipart/form-data\" class=\"form-signin\" style=\"max-width:330px; margin:0 auto\">"
        "<h2 class=\"form-signin-heading\">Please sign in</h2>"
        "<label for=\"inputEmail\" class=\"sr-only\">Email address</label>"
        "<input type=\"email\" name=\"login\" id=\"inputEmail\" class=\"form-control\" placeholder=\"Email address\" required autofocus>"
        "<label for=\"inputPassword\" class=\"sr-only\">Password</label>"
        "<input type=\"password\" name=\"password\" id=\"inputPassword\" class=\"form-control\" placeholder=\"Password\" required>"
        "<button class=\"btn btn-lg btn-primary btn-block\" type=\"submit\">Sign in</button>"
        "</form>";

    const static string mismatchBanner =
        "<div class=\"alert alert-primary\" role=\"alert\">"
            "Username/password incorrect. Try again!"
        "</div>";
    if(showMismatch)
    {
        VLOG(1) << "Showing mismatch banner";
        prependResponse(mismatchBanner);
    }
    else
        VLOG(1) << "Not showing mismatch banner";

    prependResponse(html);

    VLOG(2) << "End " << __PRETTY_FUNCTION__;
}

void EditHandler::processLogin()
{
    VLOG(2) << "Start " << __PRETTY_FUNCTION__;

    const static string cookieName = "session";

    auto login = getPostParam("login");
    auto pass = getPostParam("password");
    if(login && pass && 
        login->type == PostParamType::VALUE && pass->type == PostParamType::VALUE)
    {
        VLOG(3) << "Login credentials supplied"
            << "\n\tLogin: " << login->value
            << "\n\tPassword: NOT PRINTED"
            << "\n\tUUID from cookie: " << session.getUUID();

        VLOG(1) << "Check provided credential";
        if(session.authenticateLogin(login->value, pass->value))
        {
            LOG(INFO) << "Login authenticated";
            addCookie(cookieName, session.getUUID());
            throw HandlerRedirect(HandlerRedirect::RedirCode::HTTP_303, "/edit");
        }
        else
            LOG(INFO) << "Login rejected";
    }
    else
        LOG(WARNING) << "POST missing login credential"; 

    // If code gets here; let user retry
    buildLoginPage(true);

    VLOG(2) << "End " <<  __PRETTY_FUNCTION__;
}

void EditHandler::buildMainPage()
{
    VLOG(2) << "Start " << __PRETTY_FUNCTION__;

    VLOG(1) << "Render main page";
    prependResponse(
        "<h1>Choose an action</h1>\n" + makeMenuButtons({
        { "/edit/new", "New Article" },
        { "/edit/article", "Edit An Article" },
        { "/edit/upload", "Upload image" },
        { "/edit/viewupload", "View uploads" }
    }));

    VLOG(2) << "End " <<  __PRETTY_FUNCTION__;
}

void EditHandler::buildEditor(const std::string &articleId)
{   
    VLOG(2) << "Start " << __PRETTY_FUNCTION__;

    string body;
    if(articleId != "")
    {
        VLOG(1) << "Retrieve article from database";
        body = db.getArticle(articleId);
    }

    VLOG(1) << "Render editor";
    string page =
        "<form method=\"post\" action=\"/edit/savearticle\" enctype=\"multipart/form-data\">\n"
            "<input type=\"hidden\" name=\"csrf\" value=\"" + session.genCSRFKey() + "\">\n";

    if(articleId != "")
    {
        VLOG(1) << "Add articleid input field";
        page = "<h1>Update article</h1>" + page;
        page +=
            "<input type=\"hidden\" name=\"articleid\" value=\"" + articleId + "\">\n";
    }
    else
    {
        page = "<h1>New Article</h1>" + page;
        VLOG(1) << "Not putting articleid input field";
    }

    page +=
            "<div class=\"form-group\">\n"
                "<label for=\"content\">Content</label>\n"
                "<textarea name=\"content\" class=\"form-control\"  rows=\"20\" "
                    "required id=\"content\" placeholder=\"Article content\" "
                    "maxlength=\"" + to_string(page.max_size()) + "\">\n";
    prependResponse(page);
    prependResponse(body);
    page = 
                "</textarea>\n"
            "</div>\n"
            "<button type=\"submit\" class=\"btn btn-primary\">Save</button>\n"
        "</form>\n";
    prependResponse(page);

    VLOG(2) << "End " <<  __PRETTY_FUNCTION__;
}

void EditHandler::processSaveArticle()
{
    VLOG(2) << "Start " << __PRETTY_FUNCTION__;

    if(getMethod() != "POST")
    {
        LOG(WARNING) << "Unexpecte method \"" << getMethod()
            << " when saving article";
        throw HandlerError(405, "Method not allowed");
    }

    string articleId, content;
    auto param = getPostParam("csrf");
    if(!param || param->type != PostParamType::VALUE ||
        !session.verifyCSRFKey(param->value))
    {
        LOG(WARNING) << "CSRF mismatch. CSRF provided: " << param->value;
        throw HandlerError(401, "Unauthorized");
    }
    VLOG(1) << "CSRF key validated";


    param = getPostParam("content");
    if(!param || param->type != PostParamType::VALUE)
    {   
        LOG(WARNING) << "\"content\" POST param missing or incorrect type";
        throw HandlerError(400, "Bad Request");
    }
    VLOG(1) << "content present";
    content = param->value;
    VLOG(3) << "Content: " << content.substr(0, 50);

    // articleid is only needed when editing
    param = getPostParam("articleid");
    if(param && param->type == PostParamType::VALUE)
    {
        articleId = param->value;
        for(auto i : articleId)
        {
            if(!isdigit(i))
            {
                LOG(WARNING) << "Value of articleid POST param, " << articleId
                    << " is not numeric";
                throw HandlerError(400, "Bad Request");
            }
        }
        VLOG(3) << "articleid passed validation";
    }
    else
        VLOG(3) << "articleid POST param not provided";

    SummaryBuilder builder;
    builder.build(content);

    string title = builder.getTitle();
    string preview = builder.getPreview();

    VLOG(3) << "Article title: " << title;
    VLOG(3) << "Article preview: " << preview;

    if(articleId == "")
    {
        LOG(INFO) << "Save new article to database";
        auto tmp = db.saveArticle(*(session.getUserId()), title, preview,
            content);
        if(tmp)
            prependResponse(string("<p>New article ID: ") + to_string(tmp)
                + "</p>");
    }
    else
    {
        LOG(INFO) << "Update article " << articleId;
        db.updateArticle(*(session.getUserId()), title, preview, content,
            articleId);
        prependResponse(string("<p>Article updated</p>"));
    }

    VLOG(2) << "End " <<  __PRETTY_FUNCTION__;
}

void EditHandler::buildEditSelect()
{
    VLOG(2) << "Start " << __PRETTY_FUNCTION__;

    VLOG(1) << "Build article list";

    string data;
    for(auto article : db.getHeadlines())
    {
        ostringstream line;
        line << "<div class=\"row\">\n"
             "<div class=\"col\"><a href=\"/edit/article/"
                << get<0>(article) << + "\">" << get<1>(article) << "</a></div>\n"
            << "<div class=\"col-11\">" << get<2>(article) << "</div>\n</div>\n";
        if((data.capacity() - data.size() - line.str().size()) < 0)
        {
            VLOG(2) << "Loading existing list to buffer";
            prependResponse(data);
            data = line.str();
        }
        else
        {
            VLOG(2) << "Append line to data buffer";
            data = data + line.str();
        }
    }
    prependResponse(data);

    VLOG(2) << "End " <<  __PRETTY_FUNCTION__;
}

void EditHandler::processEditArticle()
{
    VLOG(2) << "Start " << __PRETTY_FUNCTION__;

    static regex parser("/edit/article/(\\d+)");
    smatch match;
    if(regex_match(getPath(), match, parser))
    {
        auto id = match[1];
        VLOG(3) << "Article id: " << id.str();
        buildEditor(id.str());
    }
    else
    {
        if(getPath() == "/edit/article")
            buildEditSelect();
        else
        {
            LOG(WARNING) << "Path didn't parse";

            VLOG(2) << "End " <<  __PRETTY_FUNCTION__;
            throw HandlerError(404, "File not found");
        }
    }

    VLOG(2) << "End " <<  __PRETTY_FUNCTION__;
}

void EditHandler::buildUploadPage()
{
    VLOG(2) << "Start " << __PRETTY_FUNCTION__;
    VLOG(1) << "Render upload form";
    string page = "<h1>Upload image</h1>\n"
        "<form method=\"post\" action=\"/edit/upload\" enctype=\"multipart/form-data\">\n"
            "<input type=\"hidden\" name=\"csrf\" value=\"" + session.genCSRFKey() + "\">\n"
            "<div class=\"form-group\">\n"
                "<label for=\"fileupload\">File to upload</label>\n"
                "<input type=\"file\" accept=\"image/*\" name=\"fileupload\" "
                    "id=\"fileupload\" class=\"form-control\" />\n"
            "</div>\n"
            "<button type=\"submit\" class=\"btn btn-primary\">Upload</button>\n"
        "</form>\n";
    prependResponse(page);

    VLOG(2) << "End " << __PRETTY_FUNCTION__;
}

void EditHandler::processUpload()
{
    VLOG(2) << "Start " << __PRETTY_FUNCTION__;
    if(getMethod() == "GET")
    {
        LOG(INFO) << "Render upload page";
        buildUploadPage();
    }
    else if(getMethod() == "POST")
    {
        LOG(INFO) << "Process uploaded file";
        auto param = getPostParam("csrf");
        if(!param || param->type != PostParamType::VALUE ||
            !session.verifyCSRFKey(param->value))
        {
            LOG(WARNING) << "CSRF mismatch. CSRF provided: " << param->value;
            VLOG(2) << "End " << __PRETTY_FUNCTION__;
            throw HandlerError(401, "Unauthorized");
        }
        VLOG(1) << "CSRF key validated";

        param = getPostParam("fileupload");
        if(!param || param->type != PostParamType::FILE_UPLOAD)
        {
            LOG(WARNING) << "Upload file parameter not provided";
            VLOG(2) << "End " << __PRETTY_FUNCTION__;
            throw HandlerError(400, "Bad Request");
        }

        string::size_type baseLen = config.staticBase.size() -
            (*(config.staticBase.end()-1) == '/' ? 1 : 0);
        VLOG(3) << "Value of baseLen: " << baseLen;
        string displayPath = param->localFilename;
        displayPath.replace(0, baseLen, "/static");
        string body = "<p>File uploaded and saved as " + displayPath + "</p>";
        prependResponse(body);
    }
    else
    {
        LOG(WARNING) << "Unexpected method " << getMethod();
        VLOG(2) << "End " << __PRETTY_FUNCTION__;
        throw HandlerError(405, "Method not allowed");
    }

    VLOG(2) << "End " << __PRETTY_FUNCTION__;
}

void EditHandler::processViewUpload()
{
    VLOG(2) << "Start " << __PRETTY_FUNCTION__;
    string body = "<h1>Upload files</h1>";
    prependResponse(body);
    try
    {
        string uploadDir = config.staticBase + '/' + config.uploadDest;
        path p(uploadDir);
        if(exists(p))
        {
            VLOG(1) << uploadDir << " exists";
            if(is_directory(p))
            {
                VLOG(1) << uploadDir << " is a directory";
                vector<string> fileList;
                for(auto && f : directory_iterator(p))
                {
                    auto filename = f.path().filename().string();
                    VLOG(3) << "Add file to vector: " << filename;
                    fileList.push_back(filename);
                }
                sort(fileList.begin(), fileList.end());
                body = "";
                for(auto && x : fileList)
                {
                    auto displayPath = "/static/" + config.uploadDest
                        + (*(config.uploadDest.end()-1) != '/' ? "/" : "")
                        + x;
                    body += "<div class=\"row mb-3\">\n"
                        "<div class=\"col-8\">"
                            "<img class=\"img-fluid\" src=\""
                                + displayPath + "\" /></div>\n"
                        "<div class=\"col-4\"><p>" + displayPath + "</p></div>\n"
                        "</div>\n";
                }
                prependResponse(body);
            }
            else
            {
                LOG(ERROR) << config.uploadDest << " is not a directory";
                throw HandlerError(500, "Internal error");
            }
        }
        else
        {
            LOG(ERROR) << config.uploadDest << " does not exist";
            throw HandlerError(500, "Internal error");
        }
    }
    catch(const filesystem_error &e)
    {
        LOG(ERROR) << "Exception encountered enumerating " << config.uploadDest
            << ": " << e.what();
        VLOG(2) << "End " << __PRETTY_FUNCTION__;
        throw HandlerError(500, "Internal error");
    }

    VLOG(2) << "End " << __PRETTY_FUNCTION__;
}

void EditHandler::processRequest() 
{
    VLOG(2) << "Start " << __PRETTY_FUNCTION__;

    auto path = getPath();
    if(path.back() == '/')
        path = path.substr(0,path.size()-1);

    if(path == "/edit/login")
    {
        if(getMethod() == "POST")
        {
            LOG(INFO) << "Process login request";
            processLogin();
        }
        else if(getMethod() == "GET")
        {
            LOG(INFO) << "Display login page";
            auto sessionId = getCookie("session");
            VLOG(3) << "Value of session cookie: " << (sessionId ? *sessionId : "Not provided");
            if(!session.userAuthenticated())
                buildLoginPage();
            else
            {
                VLOG(1) << "User has active session, proceed to edit home page";

                VLOG(2) << "End " <<  __PRETTY_FUNCTION__;
                throw HandlerRedirect(HandlerRedirect::RedirCode::HTTP_303, "/edit");
            }
        }
    }
    else
    {
        VLOG(1) << "Not /edit/login";

        auto sessionId = getCookie("session");
        VLOG(3) << "Value of session cookie: " << (sessionId ? *sessionId : "Not provided");
        if(!session.userAuthenticated())
        {
            LOG(INFO) << "Redirect to login page";
            addCookie("session", session.getUUID());

            VLOG(2) << "End " <<  __PRETTY_FUNCTION__;
            throw HandlerRedirect(HandlerRedirect::RedirCode::HTTP_303,
                "/edit/login"
            );
        }
        
        LOG(INFO) << "User is logged-in";
        if(path == "/edit")
            buildMainPage();
        else if(path == "/edit/new")
            buildEditor();
        else if(path == "/edit/savearticle")
            processSaveArticle();
        else if(path.substr(0, strlen("/edit/article")) == "/edit/article")
            processEditArticle();
        else if(path == "/edit/upload")
            processUpload();
        else if(path == "/edit/viewupload")
            processViewUpload();
        else
        {
            LOG(INFO) << path << "not handled";

            VLOG(2) << "End " <<  __PRETTY_FUNCTION__;
            throw HandlerError(404, "File not found");
        }
    }

    VLOG(2) << "End " <<  __PRETTY_FUNCTION__;
}

}

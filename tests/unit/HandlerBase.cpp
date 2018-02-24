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
#include "gtest/gtest.h"

#include <folly/io/IOBuf.h>

#include "params.h"
#include "HandlerBase.h"

using namespace std;
using namespace folly;

namespace mimeographer
{

class HandlerBaseObj : public HandlerBase
{
    FRIEND_TEST(HandlerBaseTest, makeMenuButtons);

private:
    void processRequest() {};

public:
    HandlerBaseObj(const Config &config) : HandlerBase(config) {}
};

class HandlerBaseTest : public ::testing::Test
{
protected:
    Config config;
    HandlerBaseTest() :
        config(FLAGS_dbHost, FLAGS_dbUser, FLAGS_dbPass, FLAGS_dbName,
            FLAGS_dbPort, "/tmp", "localhost", "/tmp")
    {}
};

TEST_F(HandlerBaseTest, buildPageHeader)
{
    const string headerPart1 =
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
        "<nav class=\"navbar navbar-expand-lg navbar-expand-xl navbar-dark bg-dark\">\n"
        "<a class=\"navbar-brand\" href=\"/\">Mimeographer</a>\n"
        "<button class=\"navbar-toggler\" type=\"button\" "
            "data-toggle=\"collapse\" data-target=\"#navbarNav\" "
            "aria-controls=\"navbarNav\" aria-expanded=\"false\" "
            "aria-label=\"Toggle navigation\">\n"
        "<span class=\"navbar-toggler-icon\"></span></button>\n"
        "<div class=\"collapse navbar-collapse\" id=\"navbarNav\">\n"
            "<div class=\"navbar-nav\">\n"
                "<a class=\"nav-item nav-link\" href=\"/about\">Archives</a>\n"
                "<a class=\"nav-item nav-link\" href=\"/about\">About</a>\n"
            "</div>\n"
            "<div class=\"navbar-nav ml-auto\">\n";

    const string headerPart2 =
            "</div>\n"
        "</div>\n"
        "</nav>\n"
        "<div class=\"container-fluid\">\n"
        "<div class=\"row\">\n"
        "<div class=\"col col-10 offset-1\">\n"
        "<!-- BEGIN PAGE CONTENT -->\n";
    unique_ptr<IOBuf> expectVal(move(IOBuf::copyBuffer(
        headerPart1 +
        "<a class=\"nav-item nav-link\" href=\"/edit/login\">Login</a>\n" +
        headerPart2
    )));

    HandlerBaseObj obj(config);
    auto testData = obj.buildPageHeader();
    auto f = IOBufEqual();
    EXPECT_TRUE(f(expectVal, testData));

    expectVal = move(IOBuf::copyBuffer(
        headerPart1 +
        "<a class=\"nav-item nav-link\" href=\"/edit/new\">New Article</a>\n"
        "<a class=\"nav-item nav-link\" href=\"/edit/article\">Edit Article</a>\n"
        "<a class=\"nav-item nav-link\" href=\"/edit/upload\">Upload Image</a>\n"
        "<a class=\"nav-item nav-link\" href=\"/edit/viewupload\">View uploads</a>\n"
        "<a class=\"nav-item nav-link\" href=\"/edit/logout\">Logout</a>\n" +
        headerPart2
    ));
    obj.session.userId = 1;
    testData = obj.buildPageHeader();
    EXPECT_TRUE(f(expectVal, testData));
}

TEST_F(HandlerBaseTest, buildPageTrailer)
{
    unique_ptr<IOBuf> expectVal(move(IOBuf::copyBuffer(
        "<!-- END PAGE CONTENT -->\n"
        "</div>\n"
        "</div>\n"
        "</div>\n"
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
        "</html>")));

    HandlerBaseObj obj(config);
    auto testData = obj.buildPageTrailer();
    auto f = IOBufEqual();
    EXPECT_TRUE(f(expectVal, testData));
}

TEST_F(HandlerBaseTest, prependResponse)
{
    HandlerBaseObj obj(config);
    auto equalityOp = IOBufEqual();

    auto testString = "From empty";
    auto expectedVal = move(IOBuf::copyBuffer(testString));
    obj.prependResponse(testString);
    EXPECT_TRUE(equalityOp(obj.handlerResponse, expectedVal));

    testString = "Second string";
    expectedVal->prependChain(move(IOBuf::copyBuffer(testString)));
    obj.prependResponse(testString);
    EXPECT_TRUE(equalityOp(obj.handlerResponse, expectedVal));
}

TEST_F(HandlerBaseTest, getPostParam)
{
    HandlerBaseObj obj(config);
    EXPECT_EQ(obj.getPostParam("a"), boost::none);

    obj.postParams["a"] = { HandlerBase::PostParamType::VALUE, "field 1", "", "" };
    obj.postParams["somefile"] = {
        HandlerBase::PostParamType::FILE_UPLOAD, "",
        "uploadfile.txt", "localversion"
    };

    auto param = obj.getPostParam("a");
    EXPECT_TRUE(param);
    EXPECT_EQ(param->type, HandlerBase::PostParamType::VALUE);
    EXPECT_EQ(param->value, string("field 1"));

    param = obj.getPostParam("somefile");
    EXPECT_TRUE(param);
    EXPECT_EQ(param->type, HandlerBase::PostParamType::FILE_UPLOAD);
    EXPECT_EQ(param->value, string(""));
    EXPECT_EQ(param->filename, string("uploadfile.txt"));
    EXPECT_EQ(param->localFilename, string("localversion"));
}

TEST_F(HandlerBaseTest, parseCookies)
{
    HandlerBaseObj obj(config);
    obj.parseCookies("cookie1=asdfasdf; b=bbbb");
    EXPECT_EQ(obj.cookieJar.size(), 2);
    EXPECT_NO_THROW( {
        EXPECT_EQ(obj.cookieJar.at("cookie1"), "asdfasdf");
    });
    EXPECT_NO_THROW( {
        EXPECT_EQ(obj.cookieJar.at("b"), "bbbb");
    });
}

TEST_F(HandlerBaseTest, makeMenuButtons)
{
    HandlerBaseObj obj(config);
    {
        const string testData =
            "<a href=\"/edit/link1\" class=\"btn btn-primary\">link 1</a>";
        vector<pair<string,string>> data = { { "/edit/link1", "link 1"} };
        EXPECT_EQ(obj.makeMenuButtons(data), testData);
    }

    {
        const string testData =
            "<a href=\"/edit/link1\" class=\"btn btn-primary\">link 1</a>\n"
            "<a href=\"/edit/link2\" class=\"btn btn-primary\">link 2</a>";
        vector<pair<string,string>> data = {
            { "/edit/link1", "link 1"},
            { "/edit/link2", "link 2"}
        };
        EXPECT_EQ(obj.makeMenuButtons(data), testData);
    }
}

} // namespace mimeographer

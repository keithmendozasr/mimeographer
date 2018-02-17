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
#include "EditHandler.h"
#include "HandlerRedirect.h"

using namespace std;
using namespace folly;

namespace mimeographer
{

class EditHandlerTest : public ::testing::Test
{
protected:
    Config config;
    EditHandlerTest() :
        config(FLAGS_dbHost, FLAGS_dbUser, FLAGS_dbPass, FLAGS_dbName,
            FLAGS_dbPort, "/tmp", "localhost", "/tmp")
    {}
};

TEST_F(EditHandlerTest, buildLoginPage)
{
    auto form =
        "<form method=\"post\" action=\"/edit/login\" enctype=\"multipart/form-data\" class=\"form-signin\" style=\"max-width:330px; margin:0 auto\">"
        "<h2 class=\"form-signin-heading\">Please sign in</h2>"
        "<label for=\"inputEmail\" class=\"sr-only\">Email address</label>"
        "<input type=\"email\" name=\"login\" id=\"inputEmail\" class=\"form-control\" placeholder=\"Email address\" required autofocus>"
        "<label for=\"inputPassword\" class=\"sr-only\">Password</label>"
        "<input type=\"password\" name=\"password\" id=\"inputPassword\" class=\"form-control\" placeholder=\"Password\" required>"
        "<button class=\"btn btn-lg btn-primary btn-block\" type=\"submit\">Sign in</button>"
        "</form>";

    {
        unique_ptr<folly::IOBuf> expectVal(move(IOBuf::copyBuffer(form)));
        EditHandler obj(config);
        obj.buildLoginPage();
        IOBufEqual isEq;
        ASSERT_TRUE(isEq(expectVal, obj.handlerResponse));
    }

    {
        unique_ptr<folly::IOBuf> expectVal(move(IOBuf::copyBuffer(
            "<div class=\"alert alert-primary\" role=\"alert\">"
                "Username/password incorrect. Try again!"
            "</div>"
        )));
        expectVal->prependChain(move(IOBuf::copyBuffer(form)));

        EditHandler obj(config);
        IOBufEqual isEq;
        obj.buildLoginPage(true);
        ASSERT_TRUE(isEq(expectVal, obj.handlerResponse));
    }
}

TEST_F(EditHandlerTest, processLogin)
{
    EditHandler obj(config);
    obj.postParams["login"] = {
        HandlerBase::PostParamType::VALUE,
        "a@a.com"
    };

    obj.postParams["password"] = {
        HandlerBase::PostParamType::VALUE,
        "123456" 
    };

    obj.session.initSession();

    EXPECT_THROW(obj.processLogin(), HandlerRedirect);

    obj.postParams["password"] = {
        HandlerBase::PostParamType::VALUE,
        "0987"
    };

    EXPECT_NO_THROW(obj.processLogin());
}

} // namespace mimeographer

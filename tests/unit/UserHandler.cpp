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
#include "UserHandler.h"
#include "HandlerRedirect.h"

using namespace std;
using namespace folly;

namespace mimeographer
{

class UserHandlerTest : public ::testing::Test
{
protected:
    Config config;
    UserHandlerTest() :
        config(FLAGS_dbHost, FLAGS_dbUser, FLAGS_dbPass, FLAGS_dbName,
            FLAGS_dbPort, "/tmp", "localhost", "/tmp")
    {}

    void SetUp()
    {
        ASSERT_NO_THROW({
            DBConn db(config.dbUser, config.dbPass, config.dbHost, config.dbName,
                config.dbPort);
            db.savePassword(1, "ko8hPecckl3hX4Exh7f3-sqvqJBVaLzH4thFE-vNU4U",
                "VEOCBE1i2wM2tsrGwmLfsg8d74fv7M-AxsngFVcv2ow");
        });
    }
};

TEST_F(UserHandlerTest, buildLoginPage)
{
    auto form =
        "<form method=\"post\" action=\"/user/login\" enctype=\"multipart/form-data\" class=\"form-signin\" style=\"max-width:330px; margin:0 auto\">"
        "<h2 class=\"form-signin-heading\">Please sign in</h2>"
        "<label for=\"inputEmail\" class=\"sr-only\">Email address</label>"
        "<input type=\"email\" name=\"login\" id=\"inputEmail\" class=\"form-control\" placeholder=\"Email address\" required autofocus>"
        "<label for=\"inputPassword\" class=\"sr-only\">Password</label>"
        "<input type=\"password\" name=\"password\" id=\"inputPassword\" class=\"form-control\" placeholder=\"Password\" required>"
        "<button class=\"btn btn-lg btn-primary btn-block\" type=\"submit\">Sign in</button>"
        "</form>";

    {
        unique_ptr<folly::IOBuf> expectVal(move(IOBuf::copyBuffer(form)));
        UserHandler obj(config);
        obj.buildLoginPage();
        IOBufEqual isEq;
        EXPECT_TRUE(isEq(expectVal, obj.handlerResponse));
    }

    {
        unique_ptr<folly::IOBuf> expectVal(move(IOBuf::copyBuffer(
            "<div class=\"alert alert-primary\" role=\"alert\">"
                "Username/password incorrect. Try again!"
            "</div>"
        )));
        expectVal->prependChain(move(IOBuf::copyBuffer(form)));

        UserHandler obj(config);
        IOBufEqual isEq;
        obj.buildLoginPage(true);
        EXPECT_TRUE(isEq(expectVal, obj.handlerResponse));
    }
}

TEST_F(UserHandlerTest, processLogin)
{
    UserHandler obj(config);
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

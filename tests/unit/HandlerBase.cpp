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

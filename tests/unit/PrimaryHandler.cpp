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
#include "PrimaryHandler.h"

using namespace std;
using namespace folly;

namespace mimeographer
{

class PrimaryHandlerTest : public ::testing::Test
{
protected:
    Config config;
    PrimaryHandlerTest() :
        config(FLAGS_dbHost, FLAGS_dbUser, FLAGS_dbPass, FLAGS_dbName,
            FLAGS_dbPort, "/tmp", "localhost", "/tmp")
    {}
};

TEST_F(PrimaryHandlerTest, buildFrontPage)
{
    unique_ptr<IOBuf> expectVal(move(IOBuf::copyBuffer(
        "<h1>Test 1</h1>\n"
            "<p>Lorem ipsum dolor sit amet, consectetur adipiscing elit. Nulla "
            "auctor neque eget lobortis mollis. Morbi tempus eu felis eu auctor"
			". Vestibulum ante ipsum primis in faucibus orci luctus et ultrices"
			" posuere cubilia Curae; Cras tristique tincidunt arcu, eget dictum"
			" sapien interdum eget. Donec iaculis dapibus magna, nec vulputate "
			"ipsum molestie quis. Proin egestas dui non ante scelerisque feugia"
			"t. Vestibulum tempor, turpis vitae porttitor condimentum, sapien q"
			"uam rutrum erat, ut auctor dolor mi ac erat. Nullam aliquet ante r"
			"isus, sit amet convallis sapien ullamcorper vitae. Ut aliquet id t"
			"ortor sed suscipit. Pellentesque rutrum leo a neque congue, id lac"
			"inia libero finibus. Fusce eleifend venenatis vulputate. Vestibulu"
			"m vitae mauris a ex pretium posuere id ac dui. Quisque neque dolor"
			", gravida vel neque non, consequat imperdiet nunc. Fusce finibus, "
			"enim sed rutrum interdum, felis lorem tristique dolor, quis pulvin"
			"ar orci libero ut nisl. In hac habitasse platea dictumst. Nullam t"
			"empus vestibulum nisi eget cras amet.</p>\n"
    )));

    PrimaryHandler obj(config);
    obj.buildFrontPage();
    IOBufEqual isEq;
    EXPECT_TRUE(isEq(expectVal, obj.handlerResponse))
        << "handlerResponse value: " << obj.handlerResponse->data();
}

TEST_F(PrimaryHandlerTest, renderArticle_header)
{
    IOBufEqual isEq;    
    PrimaryHandler obj(config);
    unique_ptr<IOBuf> expectVal(move(IOBuf::copyBuffer(
        "<h1>Header 1</h1>\n"
        "<h2>Header 2</h2>\n"
    )));
    obj.renderArticle("# Header 1\n## Header 2");
    EXPECT_TRUE(isEq(expectVal, obj.handlerResponse));
}

TEST_F(PrimaryHandlerTest, renderArticle_lists)
{
    IOBufEqual isEq;    
    PrimaryHandler obj(config);
    unique_ptr<IOBuf> expectVal(move(IOBuf::copyBuffer(
        "<ol>\n<li>Item 1</li>\n"
        "<li>Item 2</li>\n</ol>\n"
        "<ul>\n<li>Unordered 1</li>\n"
        "<li>Unordered 2</li>\n</ul>\n"
    )));
    obj.renderArticle(
        "1. Item 1\r\n1. Item 2\r\n"
        "* Unordered 1\r\n* Unordered 2"
    );
    EXPECT_TRUE(isEq(expectVal, obj.handlerResponse));
}

TEST_F(PrimaryHandlerTest, renderArticle_paragraph)
{
    IOBufEqual isEq;    
    PrimaryHandler obj(config);
    unique_ptr<IOBuf> expectVal(move(IOBuf::copyBuffer(
        "<p>Line 1<br />\nLine 2</p>\n"
        "<ul>\n<li>Unordered item</li>\n</ul>\n"
        "<ol>\n<li>Ordered item</li>\n</ol>\n"
    )));

    obj.renderArticle(
        "Line 1  \nLine 2\n"
        "* Unordered item\n"
        "1. Ordered item"
    );
    EXPECT_TRUE(isEq(expectVal, obj.handlerResponse));
}

TEST_F(PrimaryHandlerTest, renderArticle_blockquote)
{
    IOBufEqual isEq;    
    PrimaryHandler obj(config);
    unique_ptr<IOBuf> expectVal(move(IOBuf::copyBuffer(
        "<blockquote>\n<p>Blockquote line 1 "
        "Blockquote line 2</p>\n</blockquote>\n"
    )));

    obj.renderArticle(
        "> Blockquote line 1\r\n"
        "> Blockquote line 2\r\n"
    );
    EXPECT_TRUE(isEq(expectVal, obj.handlerResponse));
}

TEST_F(PrimaryHandlerTest, renderArticle_link)
{
    IOBufEqual isEq;    
    PrimaryHandler obj(config);
    unique_ptr<IOBuf> expectVal(move(IOBuf::copyBuffer(
        "<p><a href=\"http://example.com/blah\">blah</a></p>\n"
        "<p><a href=\"http://example.com/blah\" title=\"blah blah\">blah</a></p>\n"
    )));
    obj.renderArticle(
        "[blah](http://example.com/blah)\r\n\r\n"
        "[blah](http://example.com/blah \"blah blah\")"
    );
    EXPECT_TRUE(isEq(expectVal, obj.handlerResponse));
}

TEST_F(PrimaryHandlerTest, renderArticle_image)
{
    IOBufEqual isEq;    
    PrimaryHandler obj(config);
    unique_ptr<IOBuf> expectVal(move(IOBuf::copyBuffer(
        "<p><img class=\"mx-auto d-block\" src=\"pic1.png\" alt=\"\" /> "
        "<img class=\"mx-auto d-block\" src=\"pic2.png\" alt=\"blah\" /> "
        "<img class=\"mx-auto d-block\" src=\"pic3.png\" title=\"title\" alt=\"blah\" /></p>\n"
    )));

	obj.renderArticle(
        "![](pic1.png)\r\n"
        "![blah](pic2.png)\r\n"
        "![blah](pic3.png \"title\")"
    );
    EXPECT_TRUE(isEq(expectVal, obj.handlerResponse));
}
    
TEST_F(PrimaryHandlerTest, renderArticle_codeblock)
{
    IOBufEqual isEq;    
    PrimaryHandler obj(config);
    unique_ptr<IOBuf> expectVal(move(IOBuf::copyBuffer(
        "<pre><code>"
        "{\n"
        "   foo();\n"
        "   bar();\n"
        "}\n</code></pre>\n"
    )));

    obj.renderArticle(
        "```\n"
        "{\n"
        "   foo();\n"
        "   bar();\n"
        "}\n"
        "```"
    );
    EXPECT_TRUE(isEq(expectVal, obj.handlerResponse));
}

TEST_F(PrimaryHandlerTest, renderArticle_htmlblock)
{
    IOBufEqual isEq;    
    PrimaryHandler obj(config);
    unique_ptr<IOBuf> expectVal(move(IOBuf::copyBuffer(
        "<blockquote>\n"
        "blockquote line 1<br />\n"
        "blockquote line 2\n"
        "</blockquote>\n"
    )));

    obj.renderArticle(
        "<blockquote>\r\n"
        "blockquote line 1<br />\r\n"
        "blockquote line 2\r\n"
        "</blockquote>"
    );
    EXPECT_TRUE(isEq(expectVal, obj.handlerResponse));
}

TEST_F(PrimaryHandlerTest, renderArticle_htmlinline)
{
    IOBufEqual isEq;    
    PrimaryHandler obj(config);
    unique_ptr<IOBuf> expectVal(move(IOBuf::copyBuffer(
        "<p><em>Emphasis</em><strong>Strong</strong></p>\n"
    )));

    obj.renderArticle("<em>Emphasis</em><strong>Strong</strong>");
    EXPECT_TRUE(isEq(expectVal, obj.handlerResponse));
}

TEST_F(PrimaryHandlerTest, renderArticle_em)
{
    IOBufEqual isEq;
    PrimaryHandler obj(config);
    unique_ptr<IOBuf> expectVal(move(IOBuf::copyBuffer(
        "<p><em>Emphasis</em></p>\n"
    )));

    obj.renderArticle("_Emphasis_");
    EXPECT_TRUE(isEq(expectVal, obj.handlerResponse));
}

TEST_F(PrimaryHandlerTest, renderArticle_strong)
{
    IOBufEqual isEq;
    PrimaryHandler obj(config);
    unique_ptr<IOBuf> expectVal(move(IOBuf::copyBuffer(
        "<p><strong>Strong</strong></p>\n"
    )));

    obj.renderArticle("__Strong__");
    EXPECT_TRUE(isEq(expectVal, obj.handlerResponse));
}
} // namespace mimeographer

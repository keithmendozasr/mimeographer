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

#include <functional>

#include "SummaryBuilder.h"

#include "gtest/gtest.h"

using namespace std;

namespace mimeographer
{

TEST(SummaryBuilderTest, buildTitleClean)
{
    string expectText = "Test Title";
    string markdown = "# " + expectText;

    unique_ptr<cmark_node, function<void(cmark_node*)>> rootNode(
        cmark_parse_document(markdown.c_str(), markdown.size(),
            CMARK_OPT_DEFAULT),
        [](cmark_node *node)
        {
            if(node)
                cmark_node_free(node);
        }
    );

    SummaryBuilder obj;
    obj.iterator = move(
        shared_ptr<cmark_iter>(cmark_iter_new(rootNode.get()), 
        [](cmark_iter *iter)
        {
            if(iter)
                cmark_iter_free(iter);
        }
    ));
    ASSERT_EQ(cmark_iter_next(obj.iterator.get()), CMARK_EVENT_ENTER);
    ASSERT_NO_THROW({
        obj.buildTitle();
        ASSERT_EQ(obj.title, expectText);
    });
}

TEST(SummaryBuilderTest, buildTitleWithInlines)
{
    string expectText = "Test Title with inlines and link";
    string markdown = 
        "# Test Title *with* **inlines** [and link](/randomspot)";

    unique_ptr<cmark_node, function<void(cmark_node*)>> rootNode(
        cmark_parse_document(markdown.c_str(), markdown.size(),
            CMARK_OPT_DEFAULT),
        [](cmark_node *node)
        {
            if(node)
                cmark_node_free(node);
        }
    );

    SummaryBuilder obj;
    obj.iterator = move(
        shared_ptr<cmark_iter>(cmark_iter_new(rootNode.get()), 
        [](cmark_iter *iter)
        {
            if(iter)
                cmark_iter_free(iter);
        }
    ));
    ASSERT_EQ(cmark_iter_next(obj.iterator.get()), CMARK_EVENT_ENTER);
    ASSERT_NO_THROW({
        obj.buildTitle();
        ASSERT_EQ(obj.title, expectText);
    });
}

TEST(SummaryBuilderTest, buildPreviewClean)
{
    string expectedText =
        "Lorem ipsum dolor sit amet, consectetur adipiscing elit. Duis pulvinar"
        "pellentesque fringilla. Vestibulum ante ipsum primis in faucibus orci "
        "luctus et ultrices posuere cubilia Curae; Nunc maximus augue magna, ve"
        "l euismod purus efficitur eu. Nunc massa nunc.";

    unique_ptr<cmark_node, function<void(cmark_node*)>> rootNode(
        cmark_parse_document(expectedText.c_str(), expectedText.size(),
            CMARK_OPT_DEFAULT),
        [](cmark_node *node)
        {
            if(node)
                cmark_node_free(node);
        }
    );

    SummaryBuilder obj;
    obj.iterator = move(
        shared_ptr<cmark_iter>(cmark_iter_new(rootNode.get()), 
        [](cmark_iter *iter)
        {
            if(iter)
                cmark_iter_free(iter);
        }
    ));

    ASSERT_EQ(cmark_iter_next(obj.iterator.get()), CMARK_EVENT_ENTER);
    ASSERT_NO_THROW({
        obj.buildPreview();
        ASSERT_EQ(obj.preview, expectedText);
    });
}

TEST(SummaryBuilderTest, buildPreviewWithInlines)
{
    string markdown =
        "Lorem ipsum dolor, ![random image](/someimage.png \"Image descriptor\") consectetur "
        "**adipiscing elit**. Duis pulvinar *pellentesque* fringilla. Vestibulum "
        "ante ipsum primis in faucibus orci luctus et ultrices posuere cubilia "
        "Curae; Nunc maximus augue magna, vel euismod purus efficitur eu. Nunc "
        "massa nunc.\n\n"
        "Fusce egestas sem ac metus mollis egestas. Donec ultrices turpis sed ex aliquam";

    string expectText =
        "Lorem ipsum dolor, random image consectetur adipiscing elit. Duis pulvinar pellen"
        "tesque fringilla. Vestibulum ante ipsum primis in faucibus orci luctu"
        "s et ultrices posuere cubilia Curae; Nunc maximus augue magna, vel eu"
        "ismod purus efficitur eu. Nunc massa ";

    unique_ptr<cmark_node, function<void(cmark_node*)>> rootNode(
        cmark_parse_document(markdown.c_str(), markdown.size(),
            CMARK_OPT_DEFAULT),
        [](cmark_node *node)
        {
            if(node)
                cmark_node_free(node);
        }
    );

    SummaryBuilder obj;
    obj.iterator = move(
        shared_ptr<cmark_iter>(cmark_iter_new(rootNode.get()), 
        [](cmark_iter *iter)
        {
            if(iter)
                cmark_iter_free(iter);
        }
    ));

    ASSERT_EQ(cmark_iter_next(obj.iterator.get()), CMARK_EVENT_ENTER);
    ASSERT_NO_THROW({
        obj.buildPreview();
        ASSERT_EQ(obj.preview, expectText);
    });
}

TEST(SummaryBuilderTest, build)
{
    string markdown =
        "# Test Title\n"
        "Lorem ipsum dolor sit amet, consectetur adipiscing elit. Curabitur vi"
        "tae dictum sem. Cras a lorem sed felis dictum elementum eu vel risus."
        " Donec pretium lobortis pulvinar. Donec eu sodales mi. Aenean id elem"
        "entum ante. Nam id urna hendrerit, mattis neque ut, faucibus purus. N"
        "am scelerisque vulputate blandit. Proin euismod viverra mollis. Donec"
        "auctor porta libero, in mollis enim vulputate eu. Sed rutrum mollis u"
        "rna nec facilisis. Aliquam vel neque posuere, vestibulum tellus id, b"
        "landit leo.\n\n"
        "Fusce egestas sem ac metus mollis egestas. Donec ultrices turpis sed "
        "ex aliquam, sed porttitor lectus porttitor. Integer pellentesque tris"
        "tique dolor, a tincidunt nisl rhoncus sit amet. Donec at risus quam. "
        "Proin vehicula nibh vel quam viverra bibendum. Proin eu libero sem. P"
        "roin ultricies neque nec leo convallis dignissim. Vestibulum sagittis"
        " neque dui, sit amet eleifend purus mattis vitae. Sed fermentum enim "
        "ligula, in cursus nisl semper non. Aenean a pulvinar purus, sit amet "
        "malesuada ante. In sed euismod lorem. Maecenas scelerisque bibendum n"
        "isi, vitae condimentum arcu viverra id. Integer augue est, molestie q"
        "uis semper lobortis, consequat eget quam.";

    string expectedTitle = "Test Title";

    string expectedPreview =
        "Lorem ipsum dolor sit amet, consectetur adipiscing elit. Curabitur vi"
        "tae dictum sem. Cras a lorem sed felis dictum elementum eu vel risus." 
        " Donec pretium lobortis pulvinar. Donec eu sodales mi. Aenean id elem"
        "entum ante. Nam id urna hendrerit, mattis neque u";

    SummaryBuilder obj;
    ASSERT_NO_THROW({
        obj.build(markdown);
        ASSERT_EQ(obj.getTitle(), expectedTitle);
        ASSERT_EQ(obj.getPreview(), expectedPreview);
    });
}

} // namespace

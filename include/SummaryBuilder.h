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
#pragma once

#include <memory>
#include <string>

#include <cmark.h>

#include "gtest/gtest_prod.h"

namespace mimeographer
{

class SummaryBuilder
{
    FRIEND_TEST(SummaryBuilderTest, buildTitleClean);
    FRIEND_TEST(SummaryBuilderTest, buildTitleWithInlines);
    FRIEND_TEST(SummaryBuilderTest, buildPreviewClean);
    FRIEND_TEST(SummaryBuilderTest, buildPreviewWithInlines);

private:
    std::shared_ptr<cmark_iter> iterator;
    std::string title, preview;

    void buildTitle();
    void buildPreview();

public:
    void build(const std::string &markdown);

    inline const std::string getTitle() const
    {
        return title;
    }

    inline const std::string getPreview() const
    {
        return preview;
    }
};

} //namespace

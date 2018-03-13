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

#include <array>

#include "gtest/gtest.h"

#include "params.h"
#include "SiteTemplates.h"

using namespace std;

namespace mimeographer
{

TEST(SiteTemplatesTest, init)
{
    Config config(FLAGS_dbHost, FLAGS_dbUser, FLAGS_dbPass, FLAGS_dbName,
            FLAGS_dbPort, "/tmp", "localhost", FLAGS_staticBase);
    EXPECT_NO_THROW({
        SiteTemplates::init(config);

        EXPECT_EQ(SiteTemplates::templateItems.at("header"),
            "HEADER TEST LINE 1\nHEADER TEST LINE 2\n");
        EXPECT_EQ(SiteTemplates::templateItems.at("navbase"),
            "NAVBASE\n");

        EXPECT_EQ(SiteTemplates::templateItems.at("login").size(), 4566);
    });

    EXPECT_NO_THROW({
        SiteTemplates::templateItems.at("login");
        SiteTemplates::templateItems.at("editnav");
        SiteTemplates::templateItems.at("usernav");
        SiteTemplates::templateItems.at("navclose");
        SiteTemplates::templateItems.at("contentopen");
    });
}

} // namespace

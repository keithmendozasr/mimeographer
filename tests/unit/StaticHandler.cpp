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

#include "params.h"
#include "HandlerError.h"
#include "StaticHandler.h"

using namespace std;

namespace mimeographer
{

TEST(StaticHandlerTest, parsePath)
{
    Config config(FLAGS_dbHost, FLAGS_dbUser, FLAGS_dbPass, FLAGS_dbName,
            FLAGS_dbPort, "/tmp", "localhost", "/tmp");
    StaticHandler obj(config);
    EXPECT_NO_THROW({
        EXPECT_EQ(obj.parsePath("asdf+ddd%20yyy%23"), string("asdf ddd yyy#"));
    });

    EXPECT_THROW({ obj.parsePath("asdf%2R"); }, HandlerError);
}

} // namespace

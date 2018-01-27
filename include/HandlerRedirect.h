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

#pragma once

#include <exception>
#include <string>
#include <array>

namespace mimeographer
{

class HandlerRedirect : public std::exception
{
public:
    enum RedirCode
    {
        HTTP_301,
        HTTP_303,
        HTTP_307,
        HTTP_308
    };

private:
    const RedirCode code;
    const std::string location;

    std::array<const std::string, 4> statusText = {
        "Moved Permanently",
        "See Other",
        "Temporary Redirect",
        "Permanent Redirect"
    };

    std::array<const unsigned short, 4> statusCode = { 301, 303, 307, 308 };

public:
    HandlerRedirect(const RedirCode &code, const std::string &location) :
        code(code), location(location) {}

    inline const unsigned short getCode() const
    {
        return statusCode[code];
    }

    inline const std::string getStatusText() const
    {
        return statusText[code];
    }

    inline const std::string getLocation() const
    {
        return location;
    }
};

}

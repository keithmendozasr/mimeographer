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

namespace mimeographer
{

class HandlerError : public std::exception
{
private:
    const unsigned short code;
    const std::string message;
public:
    HandlerError(const unsigned short &code, const std::string &message) :
        code(code), message(message) {}

    const unsigned short getCode() const
    {
        return code;
    }

    const char * what() const noexcept
    {
        return message.c_str();
    }
};

}

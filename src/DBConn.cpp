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

#include <sstream>
#include <cctype>
#include <iomanip>

#include <glog/logging.h>

#include "DBConn.h"

using namespace std;

namespace mimeographer 
{

const string DBConn::urlEncode(const std::string &str) const
{
    ostringstream buf;
    buf.fill('0');
    buf << hex;

    for(unsigned char ch : str)
    {
        if(isalpha(ch) || ch == '-' || ch == '.' || ch ==  '_' || ch == '~')
            buf << ch;
        else
            buf << '%' << uppercase << setw(2) << int(ch) << nouppercase;
    }

    VLOG(3) << __PRETTY_FUNCTION__ << "\tURL-encoded string to return: "
        << buf.str();

    return std::move(buf.str());
}

DBConn::DBConn(const std::string &username, const std::string &password,
    const std::string &dbName, const int port) :
    username(username), password(password), dbName(dbName), port(port)
{
    VLOG(3) << __PRETTY_FUNCTION__ << " called\n"
        << "Username: " << this->username
        << "\nPassword: NOT PRINTED ON PURPOSE"
        << "\ndbName: " << this->dbName
        << "\nPort: " << this->port;
}

}

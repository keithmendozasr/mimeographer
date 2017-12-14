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
#include <sstream>

#include <glog/logging.h>

#include "DBConn.h"

using namespace std;

namespace mimeographer 
{

const string DBConn::urlEncode(const string &str) const
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

    VLOG(2) << "URL-encoded string to return: "
        << buf.str();

    return move(buf.str());
}

DBConn::DBConn(const string &username, const string &password,
    const string &dbHost, const string &dbName, const unsigned short port)
{
    VLOG(2) << __PRETTY_FUNCTION__ << " called\n"
        << "Username: " << username
        << "\nPassword: NOT PRINTED ON PURPOSE"
        << "\ndbName: " << dbName
        << "\nPort: " << port;

    ostringstream URI;
    URI << "postgresql://" << username << ":" << password << "@"
        << dbHost << ":" << port << "/" << dbName 
        << "?connect_timeout=30&application_name=mimeographer";

    auto tmp = PQconnectdb(URI.str().c_str());
    // Just in case we're out of memory
    if(tmp == nullptr)
    {
        LOG(ERROR) << "Failed to allocate memory for PGconn object";
        throw bad_alloc();
    }

    conn = unique_ptr<PGconn, PGconnCleaner>(tmp);

    if(PQstatus(conn.get()) != CONNECTION_OK)
        throw DBError(conn.get());

    VLOG(1) << "DB connection established";
}

// TODO: Wrap this in a weak pointer or something
DBConn::DBError::DBError(const PGconn *conn)
{
    msg = PQerrorMessage(conn);
}

const char *DBConn::DBError::what() const noexcept
{
    return msg.c_str();
}

}

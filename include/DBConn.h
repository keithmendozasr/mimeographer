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

#include <string>
#include <exception>
#include <memory>

#include <glog/logging.h>
#include <gtest/gtest_prod.h>
#include <postgresql/libpq-fe.h>

namespace mimeographer 
{

////
/// Manage DB-related operations
////
class DBConn 
{
    friend class DBConnTest;
    FRIEND_TEST(DBConnTest, urlEncode);
    FRIEND_TEST(DBConnTest, constructor);

private:
    // This is here for the unit tester
    DBConn() = default;

    class PGconnCleaner
    {
    public:
        void operator()(PGconn *conn)
        {
            if(conn)
            {
                VLOG(2) << "Clean PGconn instance";
                PQfinish(conn);
            }
        }
    };
    std::unique_ptr<PGconn, PGconnCleaner> conn;

    ////
    /// URL-encode str
    /// \param str string to URL-encode
    /// \return std::string URL-encoded equivalent of str
    ////
    const std::string urlEncode(const std::string &str) const;

public:
    ////
    /// Constructor
    /// \param username Login name
    /// \param password DB password
    /// \param dbname DB name
    /// \param port DB listening port if not using default
    ////
    DBConn(const std::string &username, const std::string &password,
        const std::string &dbHost, const std::string &dbName,
        const unsigned short port=5432);

    ////
    // Exception class for DBConn
    ////
    class DBError : public std::exception
    {
    private:
        std::string msg;

    public:
        ////
        /// Constructor
        /// \param conn weak_ptr to PGconn that triggered the error
        ////
        DBError(const PGconn *conn);

        const char *what() const noexcept override;
    };

};

}

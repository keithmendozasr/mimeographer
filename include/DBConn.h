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
#include <vector>
#include <array>
#include <tuple>

#include <boost/optional.hpp>

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
    FRIEND_TEST(DBConnTest, splitString);

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

    ////
    /// Execute query for non-parameter query
    /// \param query Query string
    /// \return unique_ptr-managed PGresult
    ////
    class PGresultCleaner
    {
    public:
        void operator()(PGresult *ptr)
        {
            if(ptr)
            {
                VLOG(2) << "Clean PGresult";
                PQclear(ptr);
            }
        }
    };
    std::unique_ptr<PGresult, PGresultCleaner> execQuery(const std::string &query) const;

    ////
    /// Execute query with parameters
    /// \param query Query string
    /// \param params Query parameters
    /// \return unique_ptr-managed PGresult
    ////
    template <std::size_t S>
    std::unique_ptr<PGresult, PGresultCleaner> execQuery(
        const std::string &query, std::array<const char *, S> params) const;

    ////
    /// Split a "string" to a vector of std::string
    /// \param str string to separate into vectors
    /// \param strLen length of str
    /// \param capacity Length to split strings in
    ///     (default to std::string::capacity())
    ////
    std::vector<std::string> splitString(char const * const str,
        const size_t &strLen,
        const size_t &capacity = std::string().max_size()) const;

public:
    ////
    /// Exception class for DBConn
    ////
    class DBError : public std::exception
    {
    private:
        std::string msg;

    public:
        ////
        /// Constructor
        /// \param msg Error message
        ////
        DBError(const std::string &msg) : msg(msg) {};

        const char *what() const noexcept override
        {
            return msg.c_str();
        }
    };

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
    /// Return available articles
    ////
    typedef std::vector<std::array<std::string, 3>> headline;

    //TODO: Fix the enum name
    enum class headlinepart { id, title, leadline };
    headline getHeadlines() const;

    ////
    /// Return article specified by id
    ////
    typedef std::tuple<std::string, std::vector<std::string>> article;
    
    //TODO: Fix the enum name
    enum class articlepart { title, content };
    article getArticle(const std::string &id) const;

    ////
    /// Retrieve the user info stored from database, if found
    /// \param login User's login to find
    ////
    typedef std::array<std::string, 5> UserRecord;
    enum class UserParts { id, fullname, email, salt, password };
    boost::optional<UserRecord> getUserInfo(const std::string &email);

    ////
    /// Save session
    /// Any errors will cause an exception
    ///
    ////
    void saveSession(const std::string &uuid);

    ////
    /// Associate session with authenticated user
    /// \param uuid UUID to associate with user id
    /// \param userID User ID to associated to UUID
    ////
    void mapUuidToUser(const std::string &uuid, const int &userId);

    ////
    /// Return the session ID and associated user id--if any--if the given uuid
    /// was used in the last hour
    /// \param uuid UUID to retrieve
    ////
    typedef boost::optional<std::tuple<std::string,
        boost::optional<int>>> SessionInfo;
    SessionInfo getSessionInfo(const std::string &uuid);

    ////
    /// Save the CSRF key
    /// \param key CSRF key
    /// \param userid User ID the CSRF is used for
    /// \param sessionid Session ID the CSRF is used for
    ////
    void saveCSRFKey(const std::string &key, const int &userId,
        const std::string &sessionid);

    ////
    /// Retrieve CSRF saved previously
    /// \param userid User ID whose CSRF is being retrieved
    /// \param sessionid Session ID whose CSRF is being retrieved
    ////
    boost::optional<const std::string> getCSRFKey(const int &userId,
        const std::string &sessionid);

    ////
    /// Save an article
    /// \param userid Author's ID
    /// \param title Article title
    /// \param markdown Article markdown
    ////
    const int saveArticle(const int &userId, const std::string &title,
        const std::string &markdown);
};

}

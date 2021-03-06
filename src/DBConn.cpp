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
#include <sstream>
#include <cctype>
#include <iomanip>
#include <sstream>
#include <stdexcept>
#include <string>

#include "DBConn.h"

using namespace std;

namespace mimeographer 
{

const string DBConn::urlEncode(const string &str) const
{
    VLOG(2) << "Start " << __PRETTY_FUNCTION__;

    ostringstream buf;
    buf.fill('0');
    buf << hex;

    for(unsigned char ch : str)
    {
        if(isalnum(ch) || ch == '-' || ch == '.' || ch ==  '_' || ch == '~')
            buf << ch;
        else
            buf << '%' << uppercase << setw(2) << int(ch) << nouppercase;
    }

    VLOG(3) << "URL-encoded string to return: "
        << buf.str();

    VLOG(2) << "End " << __PRETTY_FUNCTION__;

    return move(buf.str());
}

unique_ptr<PGresult, DBConn::PGresultCleaner>
    DBConn::execQuery(const std::string &query) const
{
    VLOG(2) << "Start " << __PRETTY_FUNCTION__;

    auto tmp = PQexec(conn.get(), query.c_str());
    // Something went wrong that prevented PQexec from returning anything;
    // the connection may know why.
    if(!tmp)
    {
        const string errMsg = PQerrorMessage(conn.get());
        LOG(ERROR) << "PQexec encountered an error: " << errMsg;
        throw DBError(errMsg);
    }

    auto status = PQresultStatus(tmp);
    if(status != PGRES_TUPLES_OK && status != PGRES_COMMAND_OK)
    {
        const string errMsg = PQresultErrorMessage(tmp);
        LOG(ERROR) << "Error executing query: " << errMsg;
        throw DBError(errMsg);
    }

    VLOG(1) << "Query executed, return results pointer";
    VLOG(2) << "End " << __PRETTY_FUNCTION__;
    return move(unique_ptr<PGresult, PGresultCleaner>(tmp));
}

template<std::size_t S>
unique_ptr<PGresult, DBConn::PGresultCleaner> DBConn::execQuery(
    const string &query, array<const char *, S> params) const
{
    VLOG(2) << "Start " << __PRETTY_FUNCTION__;

    auto tmp = PQexecParams(conn.get(), query.c_str(),
        params.size(), nullptr, params.data(), nullptr, nullptr,0);
    if(!tmp)
    {
        const string errMsg = PQerrorMessage(conn.get());
        LOG(ERROR) << "PQexec encountered an error: " << errMsg;
        throw DBError(PQerrorMessage(conn.get()));
    }

    auto status = PQresultStatus(tmp);
    if(status != PGRES_TUPLES_OK && status != PGRES_COMMAND_OK)
    {
        const string errMsg = PQresultErrorMessage(tmp);
        LOG(ERROR) << "Error executing query: " << errMsg;
        throw (DBError(PQresultErrorMessage(tmp)));
    }

    VLOG(1) << "Returning result pointer";
    VLOG(2) << "End " << __PRETTY_FUNCTION__;
    return move(unique_ptr<PGresult, PGresultCleaner>(tmp));
}

DBConn::UserRecord DBConn::buildUserRecord(unique_ptr<PGresult, PGresultCleaner> dbResult)
{
    VLOG(2) << "Start " << __PRETTY_FUNCTION__;

    auto len = PQgetlength(dbResult.get(), 0, 0);
    VLOG(3) <<  "userid length: " << len;
    int userId = stoi(string(PQgetvalue(dbResult.get(), 0, 0), len));

    len = PQgetlength(dbResult.get(), 0, 1);
    VLOG(3) <<  "fullname length: " << len;
    string fullname(PQgetvalue(dbResult.get(), 0, 1), len);

    len = PQgetlength(dbResult.get(), 0, 2);
    VLOG(3) <<  "email length: " << len;
    string email(PQgetvalue(dbResult.get(), 0, 2), len);

    len = PQgetlength(dbResult.get(), 0, 3);
    VLOG(3) <<  "salt length: " << len;
    string salt(PQgetvalue(dbResult.get(), 0, 3), len);

    len = PQgetlength(dbResult.get(), 0, 4);
    VLOG(3) <<  "password length: " << len;
    string password(PQgetvalue(dbResult.get(), 0, 4), len);

    VLOG(3) << "Record to return:"
        << "id: \"" << userId
        << "\"\nfullname: \"" << fullname
        << "\"\nemail: \"" << email
        << "\"\nsalt: \"" << salt
        << "\"\npassword: \"" << password << "\"";

    VLOG(2) << "End " << __PRETTY_FUNCTION__;
    return make_tuple(userId, fullname, email, salt, password);
}

DBConn::DBConn(const string &username, const string &password,
    const string &dbHost, const string &dbName, const unsigned short port)
{
    VLOG(2) << "Start " << __PRETTY_FUNCTION__;
    VLOG(3) << "Username: " << username
        << "\nPassword: NOT PRINTED ON PURPOSE"
        << "\ndbName: " << dbName
        << "\nPort: " << port;

    ostringstream URI;
    URI << "postgresql://" << urlEncode(username) << ":" << urlEncode(password)
        << "@" << dbHost << ":" << port << "/" << urlEncode(dbName) 
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
    {
        const string errMsg = PQerrorMessage(conn.get());
        LOG(ERROR) << "Failed to connect to database: " << errMsg;
        throw DBError(errMsg);
    }

    VLOG(1) << "DB connection established";
    VLOG(2) << "End " << __PRETTY_FUNCTION__;
}

DBConn::headline DBConn::getHeadlines() const
{
    VLOG(2) << "Start " << __PRETTY_FUNCTION__;

    const static string query =
        "SELECT articleid, title, summary"
        " FROM article WHERE publishdate <= NOW()"
        " ORDER BY publishdate DESC";
    auto dbResult = execQuery(query);
    VLOG(1) << "Article query OK";

    auto rows = PQntuples(dbResult.get());
    VLOG(1) << "Number of articles found: " << rows;

    headline retVal;
    for(auto i=0; i<rows; i++)
    {
        auto len = PQgetlength(dbResult.get(), i, 0);
        VLOG(3) << "ID length at row " << i << ": " << len;
        int id = stoi(string(PQgetvalue(dbResult.get(), i, 0), len));
        VLOG(3) << "ID: " << id;

        len = PQgetlength(dbResult.get(), i, 1);
        VLOG(3) << "Title length at row " << i << ": " << len;
        string title = string(PQgetvalue(dbResult.get(), i, 1), len);
        VLOG(3) << "Title: " << title;

        len = PQgetlength(dbResult.get(), i, 2);
        VLOG(3) << "Summary length at row " << i << ": " << len;
        string summary = string(PQgetvalue(dbResult.get(), i, 2), len);
        VLOG(3) << "Summary: " << summary;

        retVal.push_back(make_tuple(id, title, summary));
    }

    VLOG(2) << "End " << __PRETTY_FUNCTION__;
    return move(retVal);
}

string DBConn::getArticle(const string &id) const
{
    VLOG(2) << "Start " << __PRETTY_FUNCTION__;

    const static string query = "SELECT content FROM article "
        "WHERE articleid=$1";
    auto dbResult = execQuery(query, array<const char *,1>({ id.c_str() }));

    VLOG(1) << "Number of articles: " << PQntuples(dbResult.get());
    if(PQntuples(dbResult.get()) != 1)
        throw range_error("Unexpected number of articles returned from DB");
   
    auto len = PQgetlength(dbResult.get(),0,0);
    VLOG(3) << "Content length: " << len;
    auto content = string(PQgetvalue(dbResult.get(), 0,0), len);

    VLOG(2) << "End " << __PRETTY_FUNCTION__;
    return move(content);
}

DBConn::UserRecord DBConn::getUserInfo(const std::string &email)
{
    VLOG(2) << "Start " << __PRETTY_FUNCTION__;

    const static string query =
        "SELECT userid, fullname, email, salt, password "
        "FROM users WHERE email=$1 AND isactive";
        
    auto dbResult = execQuery(query, array<const char *, 1>({ email.c_str() }));
    auto rsltCnt = PQntuples(dbResult.get());
    UserRecord retVal = boost::none;
    if(rsltCnt == 0)
        LOG(WARNING) << "No account with email " << email << " found";
    else if(rsltCnt > 1)
        LOG(WARNING) << "Unexpected number of records found for " << email
            << ": " << rsltCnt;
    else
        retVal = buildUserRecord(move(dbResult));

    VLOG(2) << "End " << __PRETTY_FUNCTION__;
    return retVal;
}

DBConn::UserRecord DBConn::getUserInfo(const int &userId)
{
    VLOG(2) << "Start " << __PRETTY_FUNCTION__;

    const static string query =
        "SELECT userid, fullname, email, salt, password "
        "FROM users WHERE userid=$1 AND isactive";
    auto dbResult = execQuery(query,
        array<const char *, 1>({ to_string(userId).c_str() })
    );
    auto rsltCnt = PQntuples(dbResult.get());
    UserRecord retVal = boost::none;
    if(rsltCnt == 0)
        LOG(WARNING) << "No account with user ID " << userId << " found";
    else if(rsltCnt > 1)
        LOG(WARNING) << "Unexpected number of records found for " << userId
            << ": " << rsltCnt;
    else
        retVal = buildUserRecord(move(dbResult));

    VLOG(2) << "End " << __PRETTY_FUNCTION__;
    return retVal;
}

void DBConn::saveSession(const string &uuid)
{
    VLOG(2) << "Start " << __PRETTY_FUNCTION__;

    const static string query = "INSERT INTO session (sessionid) VALUES($1) "
        "ON CONFLICT (sessionid) DO UPDATE SET last_seen = DEFAULT";

    execQuery(query, array<const char *, 1>({ uuid.c_str() }));
    VLOG(2) << "End " << __PRETTY_FUNCTION__;
}

void DBConn::mapUuidToUser(const string &uuid, const int &userId)
{
    VLOG(2) << "Start " << __PRETTY_FUNCTION__;

    const static string query = "INSERT INTO user_session(sessionid, userid) "
        "VALUES ($1, $2) ON CONFLICT ON CONSTRAINT user_session_pkey DO NOTHING";

    execQuery(query, array<const char *,2>({
        uuid.c_str(), to_string(userId).c_str()
    }));
    VLOG(2) << "End " << __PRETTY_FUNCTION__;
}

void DBConn::unmapUuidToUser(const string &uuid, const int &userId)
{
    VLOG(2) << "Start " << __PRETTY_FUNCTION__;

    const static string query = "DELETE FROM user_session WHERE sessionid = $1 "
        "AND userid = $2";

    execQuery(query, array<const char *,2>({
        uuid.c_str(), to_string(userId).c_str()
    }));
    VLOG(2) << "End " << __PRETTY_FUNCTION__;
}

DBConn::SessionInfo DBConn::getSessionInfo(const string &uuid)
{
    VLOG(2) << "Start " << __PRETTY_FUNCTION__;

    const static string query = "SELECT sessionid, userid "
        "FROM session LEFT JOIN user_session USING (sessionid) "
        "WHERE session.sessionid = $1 AND "
        "last_seen +  interval '1 hour' > NOW()";
    auto dbRslt = execQuery(query, array<const char *,1>({ uuid.c_str() }));
    auto rsltCnt = PQntuples(dbRslt.get());
    if(rsltCnt == 0)
    {
        VLOG(1) << "No associated user id with session";
        return boost::none;
    }

    size_t len = PQgetlength(dbRslt.get(), 0,0);
    string sessionid(PQgetvalue(dbRslt.get(), 0, 0), len);

    boost::optional<int> userid;
    if(PQgetisnull(dbRslt.get(), 0, 1))
    {
        VLOG(1) << "No user associated with session";
        userid = boost::none;
    }
    else
    {
        VLOG(1) << "User associated with session";
        len = PQgetlength(dbRslt.get(), 0, 1);
        userid = stoi(string(PQgetvalue(dbRslt.get(), 0, 1), len));
    }

    VLOG(2) << "End " << __PRETTY_FUNCTION__;
    return make_tuple(sessionid, userid);
}

void DBConn::saveCSRFKey(const string &key, const int &userId, const string &sessionid)
{
    VLOG(2) << "Start " << __PRETTY_FUNCTION__;

    const string query = "UPDATE user_session SET csrfkey = $1 "
        "WHERE userid = $2 AND sessionid = $3";
    execQuery(query,
        array<const char *, 3>({
            key.c_str(),
            to_string(userId).c_str(),
            sessionid.c_str()
        })
    );

    VLOG(2) << "End " << __PRETTY_FUNCTION__;
}

boost::optional<const string> DBConn::getCSRFKey(const int &userId, const string &sessionid)
{
    VLOG(2) << "Start " << __PRETTY_FUNCTION__;

    const string query = "SELECT csrfkey FROM user_session "
        "WHERE userid = $1 AND sessionid = $2";
    auto dbRslt = execQuery(query,
        array<const char *, 2>({
            to_string(userId).c_str(),
            sessionid.c_str()
        })
    );

    if(PQntuples(dbRslt.get()) == 0)
    {
        VLOG(1) << "No CSRF found in database for user " << userId
            << " session " << sessionid;
        return boost::none;
    }

    size_t len = PQgetlength(dbRslt.get(), 0, 0);
    VLOG(2) << "End " << __PRETTY_FUNCTION__;
    return move(string(PQgetvalue(dbRslt.get(), 0, 0), len));
}

const int DBConn::saveArticle(const int &userId, const string &title,
    const string &summary, const string &markdown)
{
    VLOG(2) << "Start " << __PRETTY_FUNCTION__;

    const string query = "INSERT INTO article(userid, title, summary, content) "
        "VALUES($1, $2, $3, $4) RETURNING articleid";
    auto dbRslt = execQuery(query,
        array<const char *, 4>({
            to_string(userId).c_str(),
            title.c_str(),
            summary.c_str(),
            markdown.c_str()
        })
    );

    if(PQntuples(dbRslt.get()) != 1)
        throw DBError("More than 1 article ID returned");

    size_t len = PQgetlength(dbRslt.get(), 0, 0);
    int rslt = stoi(string(PQgetvalue(dbRslt.get(), 0, 0), len));
    VLOG(3) << "New article's ID: " << rslt;

    VLOG(2) << "End " << __PRETTY_FUNCTION__;
    return move(rslt);
}

void DBConn::updateArticle(const int &userId, const string &title,
    const string &summary, const string &markdown, const string &articleId)
{
    VLOG(2) << "Start " << __PRETTY_FUNCTION__;

    const string query = "UPDATE article SET userid = $1, title = $2, "
        "summary = $3, content = $4, savedate = NOW() WHERE articleid = $5";
    execQuery(query,
        array<const char *, 5>({
            to_string(userId).c_str(),
            title.c_str(),
            summary.c_str(),
            markdown.c_str(),
            articleId.c_str()
        })
    );

    VLOG(1) << "Article " << articleId << " updated";
    VLOG(2) << "End " << __PRETTY_FUNCTION__;
}

void DBConn::savePassword(const int &userId, const std::string newPass,
   const std::string &newSalt)
{
    VLOG(2) << "Start " << __PRETTY_FUNCTION__;
    const string query = "UPDATE users SET password = $1, salt = $2 "
        "WHERE userid = $3";
    execQuery(query,
        array<const char *, 3>({
            newPass.c_str(),
            newSalt.c_str(),
            to_string(userId).c_str()
        })
    );

    VLOG(2) << "End " << __PRETTY_FUNCTION__;
}

const string DBConn::getLatestArticle() const
{
    VLOG(2) << "Start " << __PRETTY_FUNCTION__;
    const static string query = "SELECT content FROM article "
        "ORDER BY publishdate DESC LIMIT 1";
    auto dbResult = execQuery(query);

    VLOG(3) << "Number of articles: " << PQntuples(dbResult.get());
    string content = "";
    if(PQntuples(dbResult.get()) == 1)
    {
        auto len = PQgetlength(dbResult.get(), 0, 0);
        VLOG(3) << "Content length: " << len;
        content = string(PQgetvalue(dbResult.get(), 0,0), len);
    }

    VLOG(2) << "End " << __PRETTY_FUNCTION__;
    return move(content);
}

void DBConn::addUser(const string &email, const string &newPass,
    const string &newSalt, const string &name)
{
    VLOG(2) << "Start " << __PRETTY_FUNCTION__;
    const string query = "INSERT INTO users(email, password, salt, fullname) "
        "VALUES ($1, $2, $3, $4)";
    execQuery(query,
        array<const char *, 4>({
            email.c_str(),
            newPass.c_str(),
            newSalt.c_str(),
            name.c_str()
        })
    );

    VLOG(2) << "End " << __PRETTY_FUNCTION__;
}

} // namespace

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
#include <stdexcept>
#include <string>

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
        if(isalnum(ch) || ch == '-' || ch == '.' || ch ==  '_' || ch == '~')
            buf << ch;
        else
            buf << '%' << uppercase << setw(2) << int(ch) << nouppercase;
    }

    VLOG(2) << "URL-encoded string to return: "
        << buf.str();

    return move(buf.str());
}

unique_ptr<PGresult, DBConn::PGresultCleaner>
    DBConn::execQuery(const std::string &query) const
{
    auto tmp = PQexec(conn.get(), query.c_str());
    // Something went wrong that prevented PQexec from returning anything;
    // the connection may know why.
    if(!tmp)
        throw DBError(PQerrorMessage(conn.get()));

    auto status = PQresultStatus(tmp);
    if(status != PGRES_TUPLES_OK && status != PGRES_COMMAND_OK)
        throw DBError(PQresultErrorMessage(tmp));

    // Past this we pass back the managed pointer
    VLOG(1) << "Returning result pointer";
    return move(unique_ptr<PGresult, PGresultCleaner>(tmp));
}

template<std::size_t S>
unique_ptr<PGresult, DBConn::PGresultCleaner> DBConn::execQuery(
    const string &query, array<const char *, S> params) const
{
    auto tmp = PQexecParams(conn.get(), query.c_str(),
        params.size(), nullptr, params.data(), nullptr, nullptr,0);
    if(!tmp)
        throw DBError(PQerrorMessage(conn.get()));

    auto status = PQresultStatus(tmp);
    if(status != PGRES_TUPLES_OK && status != PGRES_COMMAND_OK)
        throw (DBError(PQresultErrorMessage(tmp)));

    VLOG(1) << "Returning result pointer";
    return move(unique_ptr<PGresult, PGresultCleaner>(tmp));
}

vector<string> DBConn::splitString(char const *const str,
    const size_t &strLen, const size_t &capacity) const
{
    vector<string> retVal;
    VLOG(3) << __PRETTY_FUNCTION__ << " start. String length: " << strLen
        << " Capacity: " << capacity;
    for(size_t i=0; i<strLen; i+= capacity)
    {
        VLOG(3) << "Value of i: " << i;
        auto copysize = (strLen - i) < capacity ? (strLen-i) : capacity;
        VLOG(3) << "Value of copysize: " << copysize;
        retVal.push_back(move(string(&str[i],copysize)));
    }

    VLOG(3) << __PRETTY_FUNCTION__ << " end";
    return move(retVal);
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
        throw DBError(PQerrorMessage(conn.get()));

    VLOG(1) << "DB connection established";
}

DBConn::headline DBConn::getHeadlines() const
{
    const static string query =
        "SELECT articleid,title,substr(content,0,255) as leadline"
        " FROM article WHERE publishdate <= NOW()"
        " ORDER BY publishdate";
    auto dbResult = execQuery(query);
    VLOG(1) << "Article query OK";

    auto rows = PQntuples(dbResult.get());
    VLOG(1) << "Number of articles found: " << rows;

    headline retVal;
    for(auto i=0; i<rows; i++)
    {
        auto len = PQgetlength(dbResult.get(), i, 0);
        VLOG(2) << "ID length at row " << i << ": " << len;
        int id = stoi(string(PQgetvalue(dbResult.get(), i, 0), len));
        VLOG(2) << "ID: " << id;

        len = PQgetlength(dbResult.get(), i, 1);
        VLOG(2) << "Title length at row " << i << ": " << len;
        string title = string(PQgetvalue(dbResult.get(), i, 1), len);
        VLOG(2) << "Title: " << title;

        len = PQgetlength(dbResult.get(), i, 2);
        VLOG(2) << "leadline length at row " << i << ": " << len;
        string leadline = string(PQgetvalue(dbResult.get(), i, 2), len);
        VLOG(2) << "leadline: " << leadline;

        retVal.push_back(make_tuple(id, title, leadline));
    }

    return move(retVal);
}

DBConn::article DBConn::getArticle(const string &id) const
{
    const static string query = "SELECT title,content FROM article "
        "WHERE articleid=$1";
    auto dbResult = execQuery(query, array<const char *,1>({ id.c_str() }));

    VLOG(3) << "Number of articles: " << PQntuples(dbResult.get());
    if(PQntuples(dbResult.get()) != 1)
        throw range_error("Unexpected number of articles returned from DB");
   
    auto len = PQgetlength(dbResult.get(), 0,0);
    VLOG(3) << "Title length: " << len;
    auto title = string(PQgetvalue(dbResult.get(), 0,0), len);

    len = PQgetlength(dbResult.get(),0,1);
    VLOG(3) << "Content length: " << len;
    auto content = splitString((char const *const)PQgetvalue(dbResult.get(), 0,1), len);

    return move(make_tuple(title, content));
}

DBConn::UserRecord DBConn::getUserInfo(const std::string &email)
{
    const static string query =
        "SELECT userid, fullname, salt, password "
        "FROM users WHERE email=$1 AND isactive";
    auto dbResult = execQuery(query, array<const char *, 1>({ email.c_str() }));
    auto rsltCnt = PQntuples(dbResult.get());
    if(rsltCnt == 0)
    {
        LOG(INFO) << "No account with email " << email << " found";
        return boost::none;
    }
    else if(rsltCnt > 1)
    {
        LOG(WARNING) << "Unexpected number of records found for " << email
            << ": " << rsltCnt;
        return boost::none;
    }

    UserRecord rslt;
    
    auto len = PQgetlength(dbResult.get(), 0, 0);
    VLOG(3) <<  "userid length: " << len;
    int userid = stoi(string(PQgetvalue(dbResult.get(), 0, 0), len));

    len = PQgetlength(dbResult.get(), 0, 1);
    VLOG(3) <<  "fullname length: " << len;
    string fullname(PQgetvalue(dbResult.get(), 0, 1), len);

    len = PQgetlength(dbResult.get(), 0, 2);
    VLOG(3) <<  "salt length: " << len;
    string salt(PQgetvalue(dbResult.get(), 0, 2), len);

    len = PQgetlength(dbResult.get(), 0, 3);
    VLOG(3) <<  "password length: " << len;
    string password(PQgetvalue(dbResult.get(), 0, 3), len);

    VLOG(3) << "Record to return:"
        << "id: \"" << userid
        << "\"\nfullname: \"" << fullname
        << "\"\nemail: \"" << email
        << "\"\nsalt: \"" << salt
        << "\"\npassword: \"" << password << "\"";

    return make_tuple(userid, fullname, email, salt, password);
}

void DBConn::saveSession(const string &uuid)
{
    const static string query = "INSERT INTO session (sessionid) VALUES($1) "
        "ON CONFLICT (sessionid) DO UPDATE SET last_seen = DEFAULT";

    execQuery(query, array<const char *, 1>({ uuid.c_str() }));
}

void DBConn::mapUuidToUser(const string &uuid, const int &userId)
{
    const static string query = "INSERT INTO user_session(sessionid, userid) "
        "VALUES ($1, $2) ON CONFLICT ON CONSTRAINT user_session_pkey DO NOTHING";

    execQuery(query, array<const char *,2>({
        uuid.c_str(), to_string(userId).c_str()
    }));
}

DBConn::SessionInfo DBConn::getSessionInfo(const string &uuid)
{
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

    return make_tuple(sessionid, userid);
}

void DBConn::saveCSRFKey(const string &key, const int &userId, const string &sessionid)
{
    const string query = "UPDATE user_session SET csrfkey = $1 "
        "WHERE userid = $2 AND sessionid = $3";
    execQuery(query,
        array<const char *, 3>({
            key.c_str(),
            to_string(userId).c_str(),
            sessionid.c_str()
        })
    );
}

boost::optional<const string> DBConn::getCSRFKey(const int &userId, const string &sessionid)
{
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
        LOG(WARNING) << "No CSRF found in database for user " << userId
            << " session " << sessionid;
        return boost::none;
    }

    size_t len = PQgetlength(dbRslt.get(), 0, 0);
    return move(string(PQgetvalue(dbRslt.get(), 0, 0), len));
}

const int DBConn::saveArticle(const int &userId, const string &title, const string &markdown)
{
    const string query = "INSERT INTO article(userid, title, content, publishdate) "
        "VALUES($1, $2, $3, NOW()) RETURNING articleid";
    auto dbRslt = execQuery(query,
        array<const char *, 3>({
            to_string(userId).c_str(),
            title.c_str(),
            markdown.c_str()
        })
    );

    if(PQntuples(dbRslt.get()) != 1)
        throw DBError("More than 1 article ID returned");

    size_t len = PQgetlength(dbRslt.get(), 0, 0);
    int rslt = stoi(string(PQgetvalue(dbRslt.get(), 0, 0), len));
    VLOG(3) << "New article's ID: " << rslt;
    return move(rslt);
}

} // namespace

CREATE ROLE mimeographer_webserver;

GRANT SELECT, INSERT, UPDATE
    ON users, article, session, user_session
    TO mimeographer_webserver;

GRANT DELETE ON user_session
    TO mimeographer_webserver;

GRANT USAGE ON article_articleid_seq
    TO mimeographer_webserver;

GRANT USAGE ON users_userid_seq
    TO mimeographer_webserver;

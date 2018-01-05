CREATE ROLE mimeographer_webserver;

GRANT SELECT, INSERT, UPDATE 
    ON users, article, extra_user_permissions, comments, session, user_session
    TO mimeographer_webserver;

GRANT USAGE ON article_articleid_seq
    TO mimeographer_webserver;

GRANT SELECT ON permissions
    TO mimeographer_webserver; 

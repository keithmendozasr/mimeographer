CREATE ROLE mimeographer_unittest;

GRANT mimeographer_webserver TO mimeographer_unittest;

GRANT TRUNCATE
    ON session, user_session
    TO mimeographer_unittest;

GRANT DELETE ON users TO mimeographer_unittest;

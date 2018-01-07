CREATE ROLE mimeographer_unittest;

GRANT TRUNCATE
    ON session, user_session
    TO mimeographer_unittest;

CREATE ROLE mimeographer_webserver;

GRANT SELECT, INSERT, UPDATE 
    ON users, article, extra_user_permissions, comments
    TO mimeographer_webserver;

GRANT SELECT ON permissions
    TO mimeographer_webserver; 

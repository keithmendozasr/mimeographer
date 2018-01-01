TRUNCATE TABLE users RESTART IDENTITY;
INSERT INTO users(id, fullname, email, salt, password, isactive)
    VALUES(1, 'Test User', 'a@a.com', 'VEOCBE1i2wM2tsrGwmLfsg8d74fv7M-AxsngFVcv2ow',
        'kt56uQBSTP-bT4ybmGCgsmU48BBx__mcE61X7UsWxpE', true),
    (2, 'Inactive User', 'off@example.com', '', '', false);
ALTER SEQUENCE users_id_seq START 3;
ALTER SEQUENCE users_id_seq RESTART;

CREATE TABLE IF NOT EXISTS users (
    userid SERIAL PRIMARY KEY,
    fullname VARCHAR(256) NOT NULL,
    email VARCHAR(255) NOT NULL UNIQUE,
    salt CHAR(43) NOT NULL, --32-byte salt BASE64-encoded
    password CHAR(43) NOT NULL, --sha256 of password+salt BASE64-encoded
    isactive BOOL DEFAULT TRUE
);

CREATE TABLE IF NOT EXISTS permissions (
    permissionid SMALLSERIAL PRIMARY KEY,
    type VARCHAR(50) NOT NULL UNIQUE,
    isactive BOOL DEFAULT TRUE
);

CREATE TABLE IF NOT EXISTS extra_user_permissions (
    userid INT NOT NULL REFERENCES users(userid),
    permission SMALLINT NOT NULL REFERENCES permissions(permissionid)
        ON UPDATE CASCADE ON DELETE CASCADE,
    PRIMARY KEY(userid,permission)
);

CREATE TABLE IF NOT EXISTS article (
    articleid SERIAL PRIMARY KEY,
    title VARCHAR(255) NOT NULL,
    markdown TEXT NOT NULL,
    content TEXT NOT NULL,
    userid INT NOT NULL REFERENCES users(userid)
        ON UPDATE CASCADE ON DELETE RESTRICT,
    publishdate TIMESTAMP DEFAULT NULL,
    savedate TIMESTAMP NOT NULL DEFAULT NOW()
);

CREATE TABLE IF NOT EXISTS comments (
    id INT NOT NULL PRIMARY KEY,
    article INT NOT NULL REFERENCES article(articleid)
        ON UPDATE CASCADE ON DELETE CASCADE,
    content TEXT,
    publishdate TIMESTAMP NOT NULL DEFAULT NOW()
);

CREATE TABLE IF NOT EXISTS session (
    sessionid UUID PRIMARY KEY,
    last_seen TIMESTAMP NOT NULL DEFAULT NOW()
);
CREATE INDEX session_time ON session (last_seen);

CREATE TABLE IF NOT EXISTS user_session (
    userid INT NOT NULL REFERENCES users(userid)
        ON DELETE CASCADE ON UPDATE CASCADE,
    sessionid UUID NOT NULL REFERENCES session(sessionid)
        ON DELETE CASCADE ON UPDATE CASCADE,
    xsrf UUID,
    PRIMARY KEY(userid,sessionid)
);

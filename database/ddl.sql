CREATE TABLE IF NOT EXISTS users (
    userid SERIAL PRIMARY KEY,
    fullname VARCHAR(256) NOT NULL,
    email VARCHAR(255) NOT NULL UNIQUE,
    salt CHAR(43) NOT NULL, --32-byte salt BASE64-encoded
    password CHAR(43) NOT NULL, --sha256 of password+salt BASE64-encoded
    isactive BOOL DEFAULT TRUE
);

CREATE TABLE IF NOT EXISTS article (
    articleid SERIAL PRIMARY KEY,
    title VARCHAR(256) NOT NULL,
    content TEXT NOT NULL,
    userid INT NOT NULL REFERENCES users(userid)
        ON UPDATE CASCADE ON DELETE RESTRICT,
    publishdate TIMESTAMP DEFAULT NOW(),
    savedate TIMESTAMP NOT NULL DEFAULT NOW(),
    summary VARCHAR(256) NOT NULL
);
CREATE INDEX arcticle_publish_date ON article(publishdate);

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
    csrfkey UUID,
    PRIMARY KEY(userid,sessionid)
);

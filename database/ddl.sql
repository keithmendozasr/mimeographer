CREATE TABLE IF NOT EXISTS users (
    id SERIAL PRIMARY KEY,
    fullname VARCHAR(256) NOT NULL,
    email VARCHAR(255) NOT NULL UNIQUE,
    password1 CHAR(32) NOT NULL, --salt
    password2 CHAR(32) NOT NULL, --sha256 output
    isactive BOOL DEFAULT TRUE
);

CREATE TABLE IF NOT EXISTS permissions (
    id SMALLSERIAL PRIMARY KEY,
    type VARCHAR(50) NOT NULL UNIQUE,
    isactive BOOL DEFAULT TRUE
);

CREATE TABLE IF NOT EXISTS extra_user_permissions (
    userid INT NOT NULL REFERENCES users(id),
    permission SMALLINT NOT NULL REFERENCES permissions(id)
        ON UPDATE CASCADE ON DELETE CASCADE,
    PRIMARY KEY(userid,permission)
);

CREATE TABLE IF NOT EXISTS article (
    id SERIAL PRIMARY KEY,
    title VARCHAR(255) NOT NULL,
    markdown TEXT NOT NULL,
    content TEXT NOT NULL,
    author INT NOT NULL REFERENCES users(id)
        ON UPDATE CASCADE ON DELETE RESTRICT,
    publishdate TIMESTAMP DEFAULT NULL,
    savedate TIMESTAMP NOT NULL DEFAULT NOW()
);

CREATE TABLE IF NOT EXISTS comments (
    id INT NOT NULL PRIMARY KEY,
    article INT NOT NULL REFERENCES article(id)
        ON UPDATE CASCADE ON DELETE CASCADE,
    content TEXT,
    publishdate TIMESTAMP NOT NULL DEFAULT NOW()
);

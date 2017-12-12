TRUNCATE users,article RESTART IDENTITY CASCADE;

INSERT INTO users(fullname,email,password1,password2) VALUES
    ('Test user', 'testuser@example.com', '123456','123456');
    
INSERT INTO article(title, markdown, content,author,publishdate) VALUES
    ('Test 1','blah','blah',1,NOW());
INSERT INTO article(title, markdown, content,author,publishdate) VALUES
    ('Test 2','blah','blah',1,NOW());
INSERT INTO article(title, markdown, content,author,publishdate) VALUES
    ('Test 3','blah','blah',1,NOW());
INSERT INTO article(title, markdown, content,author,publishdate) VALUES
    ('Test 4','blah','blah',1,NOW());
INSERT INTO article(title, markdown, content,author,publishdate) VALUES
    ('Test 5','blah','blah',1,NOW());
INSERT INTO article(title, markdown, content,author,publishdate) VALUES
    ('Test 6','blah','blah',1,NOW());
INSERT INTO article(title, markdown, content,author,publishdate) VALUES
    ('Test 7','blah','blah',1,NOW());
INSERT INTO article(title, markdown, content,author,publishdate) VALUES
    ('Test 8','blah','blah',1,NOW());
INSERT INTO article(title, markdown, content,author,publishdate) VALUES
    ('Test 9','blah','blah',1,NOW());
INSERT INTO article(title, markdown, content,author,publishdate) VALUES
    ('Test 10','blah','blah',1,NOW());

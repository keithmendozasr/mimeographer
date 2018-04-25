TRUNCATE users,article RESTART IDENTITY CASCADE;

INSERT INTO users(userid, fullname, email, salt, password, isactive) VALUES
    (1, 'Test user', 'a@a.com',
    'VEOCBE1i2wM2tsrGwmLfsg8d74fv7M-AxsngFVcv2ow',
    'ko8hPecckl3hX4Exh7f3-sqvqJBVaLzH4thFE-vNU4U', true),
    (2, 'Inactive User', 'off@example.com', '', '', false);
ALTER SEQUENCE users_userid_seq RESTART WITH 3;
    
INSERT INTO article(title, summary, content, userid, publishdate) VALUES
    ('Test 1',
        'Lorem ipsum dolor sit amet, consectetur adipiscing elit. Morbi interdum enim ex, eget hendrerit neque fringilla at. Aenean dapibus leo et ligula sodales tincidunt. Nam sit amet mi vulputate, suscipit mi laoreet, tincidunt tortor. Pellentesque euismod amet.',
        E'# Test 1\nLorem ipsum dolor sit amet, consectetur adipiscing elit. Nulla auctor neque eget lobortis mollis. Morbi tempus eu felis eu auctor. Vestibulum ante ipsum primis in faucibus orci luctus et ultrices posuere cubilia Curae; Cras tristique tincidunt arcu, eget dictum sapien interdum eget. Donec iaculis dapibus magna, nec vulputate ipsum molestie quis. Proin egestas dui non ante scelerisque feugiat. Vestibulum tempor, turpis vitae porttitor condimentum, sapien quam rutrum erat, ut auctor dolor mi ac erat. Nullam aliquet ante risus, sit amet convallis sapien ullamcorper vitae. Ut aliquet id tortor sed suscipit. Pellentesque rutrum leo a neque congue, id lacinia libero finibus. Fusce eleifend venenatis vulputate. Vestibulum vitae mauris a ex pretium posuere id ac dui. Quisque neque dolor, gravida vel neque non, consequat imperdiet nunc. Fusce finibus, enim sed rutrum interdum, felis lorem tristique dolor, quis pulvinar orci libero ut nisl. In hac habitasse platea dictumst. Nullam tempus vestibulum nisi eget cras amet.',
        1,NOW()),
    ('Test 2','Start of 1st paragraph',
        E'# Test 2\r\nStart of 1st paragraph\r\n1. Item 1\r\n2. Item 2\r\n* Item 1\r\n* Item 2\r\n* [link](http://example.com)\r\n\r\n```\r\n{\r\n    code block;\r\n}\r\n```\r\n\r\n`inline code`\r\n\r\n<div>Embedded html</div>\r\n\r\n<span>html inline</span>\r\n\r\n> Lorem Ipsum is simply dummy text of the printing and typesetting industry.\r\n> Lorem Ipsum has been the industry\'s standard dummy \r\n> text ever since the 1500s, when an unknown printer took a galley\r\n> of type and scrambled it to make a type specimen book. It has\r\n> survived not only five centuries, but also the leap into electronic\r\n> typesetting, remaining essentially unchanged. It was popularised\r\n> in the 1960s with the release of Letraset sheets containing Lorem\r\n> Ipsum passages, and more recently with desktop publishing\r\n> software like Aldus PageMaker including versions of Lorem Ipsum.\r\n\r\nStart paragraph with softbreak  \r\nSecond line of softbroken paragraph\r\n\r\nFinally, the image that started it all\r\n![Big image](/static/uploads/b06953ed-62e3-486c-af0f-7eb08df357f5_military-raptor-jet-f-22-airplane-40753.jpeg)\r\n![Big image with title](/static/uploads/b06953ed-62e3-486c-af0f-7eb08df357f5_military-raptor-jet-f-22-airplane-40753.jpeg "this time with title")\r\n![](/static/uploads/b06953ed-62e3-486c-af0f-7eb08df357f5_military-raptor-jet-f-22-airplane-40753.jpeg)',
        1,NOW() - interval '30 seconds');

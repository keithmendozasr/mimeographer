TRUNCATE users,article RESTART IDENTITY CASCADE;

INSERT INTO users(fullname, email, salt, password) VALUES
    ('Test user', 'testuser@example.com',
    'VEOCBE1i2wM2tsrGwmLfsg8d74fv7M-AxsngFVcv2ow',
    'kt56uQBSTP-bT4ybmGCgsmU48BBx__mcE61X7UsWxpE');
    
INSERT INTO article(title, markdown, content, userid, publishdate) VALUES
    ('Test 1','blah','Lorem ipsum dolor sit amet, consectetur adipiscing elit. Nulla auctor neque eget lobortis mollis. Morbi tempus eu felis eu auctor. Vestibulum ante ipsum primis in faucibus orci luctus et ultrices posuere cubilia Curae; Cras tristique tincidunt arcu, eget dictum sapien interdum eget. Donec iaculis dapibus magna, nec vulputate ipsum molestie quis. Proin egestas dui non ante scelerisque feugiat. Vestibulum tempor, turpis vitae porttitor condimentum, sapien quam rutrum erat, ut auctor dolor mi ac erat. Nullam aliquet ante risus, sit amet convallis sapien ullamcorper vitae. Ut aliquet id tortor sed suscipit. Pellentesque rutrum leo a neque congue, id lacinia libero finibus. Fusce eleifend venenatis vulputate. Vestibulum vitae mauris a ex pretium posuere id ac dui. Quisque neque dolor, gravida vel neque non, consequat imperdiet nunc. Fusce finibus, enim sed rutrum interdum, felis lorem tristique dolor, quis pulvinar orci libero ut nisl. In hac habitasse platea dictumst. Nullam tempus vestibulum nisi eget cras amet.',1,NOW()),
    ('Test 2','blah','Lorem ipsum dolor sit amet, consectetur adipiscing elit. Nulla auctor neque eget lobortis mollis. Morbi tempus eu felis eu auctor. Vestibulum ante ipsum primis in faucibus orci luctus et ultrices posuere cubilia Curae; Cras tristique tincidunt arcu, eget dictum sapien interdum eget. Donec iaculis dapibus magna, nec vulputate ipsum molestie quis. Proin egestas dui non ante scelerisque feugiat. Vestibulum tempor, turpis vitae porttitor condimentum, sapien quam rutrum erat, ut auctor dolor mi ac erat. Nullam aliquet ante risus, sit amet convallis sapien ullamcorper vitae. Ut aliquet id tortor sed suscipit. Pellentesque rutrum leo a neque congue, id lacinia libero finibus. Fusce eleifend venenatis vulputate. Vestibulum vitae mauris a ex pretium posuere id ac dui. Quisque neque dolor, gravida vel neque non, consequat imperdiet nunc. Fusce finibus, enim sed rutrum interdum, felis lorem tristique dolor, quis pulvinar orci libero ut nisl. In hac habitasse platea dictumst. Nullam tempus vestibulum nisi eget cras amet.',1,NOW()),
    ('Test 3','blah','Lorem ipsum dolor sit amet, consectetur adipiscing elit. Nulla auctor neque eget lobortis mollis. Morbi tempus eu felis eu auctor. Vestibulum ante ipsum primis in faucibus orci luctus et ultrices posuere cubilia Curae; Cras tristique tincidunt arcu, eget dictum sapien interdum eget. Donec iaculis dapibus magna, nec vulputate ipsum molestie quis. Proin egestas dui non ante scelerisque feugiat. Vestibulum tempor, turpis vitae porttitor condimentum, sapien quam rutrum erat, ut auctor dolor mi ac erat. Nullam aliquet ante risus, sit amet convallis sapien ullamcorper vitae. Ut aliquet id tortor sed suscipit. Pellentesque rutrum leo a neque congue, id lacinia libero finibus. Fusce eleifend venenatis vulputate. Vestibulum vitae mauris a ex pretium posuere id ac dui. Quisque neque dolor, gravida vel neque non, consequat imperdiet nunc. Fusce finibus, enim sed rutrum interdum, felis lorem tristique dolor, quis pulvinar orci libero ut nisl. In hac habitasse platea dictumst. Nullam tempus vestibulum nisi eget cras amet.',1,NOW()),
    ('Test 4','blah','Lorem ipsum dolor sit amet, consectetur adipiscing elit. Nulla auctor neque eget lobortis mollis. Morbi tempus eu felis eu auctor. Vestibulum ante ipsum primis in faucibus orci luctus et ultrices posuere cubilia Curae; Cras tristique tincidunt arcu, eget dictum sapien interdum eget. Donec iaculis dapibus magna, nec vulputate ipsum molestie quis. Proin egestas dui non ante scelerisque feugiat. Vestibulum tempor, turpis vitae porttitor condimentum, sapien quam rutrum erat, ut auctor dolor mi ac erat. Nullam aliquet ante risus, sit amet convallis sapien ullamcorper vitae. Ut aliquet id tortor sed suscipit. Pellentesque rutrum leo a neque congue, id lacinia libero finibus. Fusce eleifend venenatis vulputate. Vestibulum vitae mauris a ex pretium posuere id ac dui. Quisque neque dolor, gravida vel neque non, consequat imperdiet nunc. Fusce finibus, enim sed rutrum interdum, felis lorem tristique dolor, quis pulvinar orci libero ut nisl. In hac habitasse platea dictumst. Nullam tempus vestibulum nisi eget cras amet.',1,NOW()),
    ('Test 5','blah','Lorem ipsum dolor sit amet, consectetur adipiscing elit. Nulla auctor neque eget lobortis mollis. Morbi tempus eu felis eu auctor. Vestibulum ante ipsum primis in faucibus orci luctus et ultrices posuere cubilia Curae; Cras tristique tincidunt arcu, eget dictum sapien interdum eget. Donec iaculis dapibus magna, nec vulputate ipsum molestie quis. Proin egestas dui non ante scelerisque feugiat. Vestibulum tempor, turpis vitae porttitor condimentum, sapien quam rutrum erat, ut auctor dolor mi ac erat. Nullam aliquet ante risus, sit amet convallis sapien ullamcorper vitae. Ut aliquet id tortor sed suscipit. Pellentesque rutrum leo a neque congue, id lacinia libero finibus. Fusce eleifend venenatis vulputate. Vestibulum vitae mauris a ex pretium posuere id ac dui. Quisque neque dolor, gravida vel neque non, consequat imperdiet nunc. Fusce finibus, enim sed rutrum interdum, felis lorem tristique dolor, quis pulvinar orci libero ut nisl. In hac habitasse platea dictumst. Nullam tempus vestibulum nisi eget cras amet.',1,NOW()),
    ('Test 6','blah','Lorem ipsum dolor sit amet, consectetur adipiscing elit. Nulla auctor neque eget lobortis mollis. Morbi tempus eu felis eu auctor. Vestibulum ante ipsum primis in faucibus orci luctus et ultrices posuere cubilia Curae; Cras tristique tincidunt arcu, eget dictum sapien interdum eget. Donec iaculis dapibus magna, nec vulputate ipsum molestie quis. Proin egestas dui non ante scelerisque feugiat. Vestibulum tempor, turpis vitae porttitor condimentum, sapien quam rutrum erat, ut auctor dolor mi ac erat. Nullam aliquet ante risus, sit amet convallis sapien ullamcorper vitae. Ut aliquet id tortor sed suscipit. Pellentesque rutrum leo a neque congue, id lacinia libero finibus. Fusce eleifend venenatis vulputate. Vestibulum vitae mauris a ex pretium posuere id ac dui. Quisque neque dolor, gravida vel neque non, consequat imperdiet nunc. Fusce finibus, enim sed rutrum interdum, felis lorem tristique dolor, quis pulvinar orci libero ut nisl. In hac habitasse platea dictumst. Nullam tempus vestibulum nisi eget cras amet.',1,NOW()),
    ('Test 7','blah','Lorem ipsum dolor sit amet, consectetur adipiscing elit. Nulla auctor neque eget lobortis mollis. Morbi tempus eu felis eu auctor. Vestibulum ante ipsum primis in faucibus orci luctus et ultrices posuere cubilia Curae; Cras tristique tincidunt arcu, eget dictum sapien interdum eget. Donec iaculis dapibus magna, nec vulputate ipsum molestie quis. Proin egestas dui non ante scelerisque feugiat. Vestibulum tempor, turpis vitae porttitor condimentum, sapien quam rutrum erat, ut auctor dolor mi ac erat. Nullam aliquet ante risus, sit amet convallis sapien ullamcorper vitae. Ut aliquet id tortor sed suscipit. Pellentesque rutrum leo a neque congue, id lacinia libero finibus. Fusce eleifend venenatis vulputate. Vestibulum vitae mauris a ex pretium posuere id ac dui. Quisque neque dolor, gravida vel neque non, consequat imperdiet nunc. Fusce finibus, enim sed rutrum interdum, felis lorem tristique dolor, quis pulvinar orci libero ut nisl. In hac habitasse platea dictumst. Nullam tempus vestibulum nisi eget cras amet.',1,NOW()),
    ('Test 8','blah','Lorem ipsum dolor sit amet, consectetur adipiscing elit. Nulla auctor neque eget lobortis mollis. Morbi tempus eu felis eu auctor. Vestibulum ante ipsum primis in faucibus orci luctus et ultrices posuere cubilia Curae; Cras tristique tincidunt arcu, eget dictum sapien interdum eget. Donec iaculis dapibus magna, nec vulputate ipsum molestie quis. Proin egestas dui non ante scelerisque feugiat. Vestibulum tempor, turpis vitae porttitor condimentum, sapien quam rutrum erat, ut auctor dolor mi ac erat. Nullam aliquet ante risus, sit amet convallis sapien ullamcorper vitae. Ut aliquet id tortor sed suscipit. Pellentesque rutrum leo a neque congue, id lacinia libero finibus. Fusce eleifend venenatis vulputate. Vestibulum vitae mauris a ex pretium posuere id ac dui. Quisque neque dolor, gravida vel neque non, consequat imperdiet nunc. Fusce finibus, enim sed rutrum interdum, felis lorem tristique dolor, quis pulvinar orci libero ut nisl. In hac habitasse platea dictumst. Nullam tempus vestibulum nisi eget cras amet.',1,NOW()),
    ('Test 9','blah','Lorem ipsum dolor sit amet, consectetur adipiscing elit. Nulla auctor neque eget lobortis mollis. Morbi tempus eu felis eu auctor. Vestibulum ante ipsum primis in faucibus orci luctus et ultrices posuere cubilia Curae; Cras tristique tincidunt arcu, eget dictum sapien interdum eget. Donec iaculis dapibus magna, nec vulputate ipsum molestie quis. Proin egestas dui non ante scelerisque feugiat. Vestibulum tempor, turpis vitae porttitor condimentum, sapien quam rutrum erat, ut auctor dolor mi ac erat. Nullam aliquet ante risus, sit amet convallis sapien ullamcorper vitae. Ut aliquet id tortor sed suscipit. Pellentesque rutrum leo a neque congue, id lacinia libero finibus. Fusce eleifend venenatis vulputate. Vestibulum vitae mauris a ex pretium posuere id ac dui. Quisque neque dolor, gravida vel neque non, consequat imperdiet nunc. Fusce finibus, enim sed rutrum interdum, felis lorem tristique dolor, quis pulvinar orci libero ut nisl. In hac habitasse platea dictumst. Nullam tempus vestibulum nisi eget cras amet.',1,NOW()),
    ('Test 10','blah','Lorem ipsum dolor sit amet, consectetur adipiscing elit. Nulla auctor neque eget lobortis mollis. Morbi tempus eu felis eu auctor. Vestibulum ante ipsum primis in faucibus orci luctus et ultrices posuere cubilia Curae; Cras tristique tincidunt arcu, eget dictum sapien interdum eget. Donec iaculis dapibus magna, nec vulputate ipsum molestie quis. Proin egestas dui non ante scelerisque feugiat. Vestibulum tempor, turpis vitae porttitor condimentum, sapien quam rutrum erat, ut auctor dolor mi ac erat. Nullam aliquet ante risus, sit amet convallis sapien ullamcorper vitae. Ut aliquet id tortor sed suscipit. Pellentesque rutrum leo a neque congue, id lacinia libero finibus. Fusce eleifend venenatis vulputate. Vestibulum vitae mauris a ex pretium posuere id ac dui. Quisque neque dolor, gravida vel neque non, consequat imperdiet nunc. Fusce finibus, enim sed rutrum interdum, felis lorem tristique dolor, quis pulvinar orci libero ut nisl. In hac habitasse platea dictumst. Nullam tempus vestibulum nisi eget cras amet.',1,NOW());

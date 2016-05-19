cinja
=====

introduction
------------

Cinja is a lightweight template engine, its syntax is based on jinja2.

Currently it supportes:

*  variable substitution: 
`{{somevar}}`
*  for-loop: 
```
{% for user in users %}  // users's type must be std::vector<std::string>
{% endfor %}
```
* if statement:
```
{% if exists %}       // currently cinja only supports a variable as condition. 0/0.0/false(ignore upper or lower case) stands for false.
{% endif %}           // else-clause is not supported now.
```

Install
-------

Cinja is a header-only project. You can use it by simply copying include/cinja.hpp to your project.

Building prerequisite:
*  C++ compiler supported C++14
*  boost library 1.58.0+


Usage
-----

Initialize a template object:
```
t = Template("template source: {{ example }}");
```
Set variable's value:
```
t.setValue("example", 1);
```
Render:
```
auto renderedString = t.render();
```

Please be sure that your template is valid, cinja will not check the currentness of the input.


License
-------

BSD-2


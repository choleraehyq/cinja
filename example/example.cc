#include "include/cinja.hpp"

#include <iostream>
#include <string>

int main() {
    auto t = cinja::Template("\
{{name}}\n\
{{ number  }}\n\
{{ float}}\n\
{% if ok %}\
    {% for i in v %}\
        {{i}}\n\
    {% endfor %}\
{%endif%}\
");
    t.setValue("name", "Hu");
    t.setValue("number", 123);
    t.setValue("float", 10.1);
    t.setValue("ok", "true");
    t.setValue("v", std::vector<std::string>{"a", "b"});
    std::cout << t.render();
    return 0;
}
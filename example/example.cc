#include "include/cinja.hpp"

#include <string>

int main() {
    auto t = cinja::Template("\
{{name}}\n\
{{ number  }}\n\
{{ float}}\n\
{% if ok %}\n\
    {% for i in v %}\n\
        {{i}}\n\
    {% endfor %}\n\
{%endif%}\n\
");
    t.setValue("name", "Hu");
    t.setValue("number", 123);
    t.setValue("float", 10.1);
    t.setValue("ok", "true");
    t.setValue("v", std::vector<std::string>{"a", "b"});
    std::cout << t.render();
    return 0;
}
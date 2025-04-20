#include "build/_deps/glm-src/glm/vec2.hpp"
#include <iostream>

int main() {
    glm::vec2 v(1.0f, 2.0f);
    std::cout << "GLM vec2: (" << v.x << ", " << v.y << ")\n";
    return 0;
}

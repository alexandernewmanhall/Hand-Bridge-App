#define CATCH_CONFIG_MAIN
#include <catch2/catch.hpp>
#include "core/AspectMapper.hpp"
#include <glm/vec3.hpp>

TEST_CASE("AspectMapper basic mapping", "[AspectMapper]") {
    AspectMapper m;
    // Default: 16x9, hardcoded extents
    glm::vec3 left = m.map({-600, 450, 0});
    glm::vec3 right = m.map({600, 450, 0});
    glm::vec3 top = m.map({0, 0, 0});
    glm::vec3 bottom = m.map({0, 900, 0});
    glm::vec3 center = m.map({0, 450, 0});
    REQUIRE(left.x == Approx(-1.f));
    REQUIRE(right.x == Approx(1.f));
    REQUIRE(center.x == Approx(0.f));
    REQUIRE(top.y == Approx(1.f));
    REQUIRE(bottom.y == Approx(-1.f));
    REQUIRE(center.y == Approx(0.f));
}

TEST_CASE("AspectMapper preset switching", "[AspectMapper]") {
    AspectMapper m;
    m.setPreset({1,1,true});
    glm::vec3 top = m.map({0,0,0});
    glm::vec3 bottom = m.map({0,900,0});
    REQUIRE(top.y == Approx(1.f));
    REQUIRE(bottom.y == Approx(-1.f));
}

#include "ftl/slice.h"
#include <ftl/catch_or_ignore.h>

TEST_CASE("test_slice", "asdf")
{
    ftl::slice<int> sl, sl2, sl3;
    REQUIRE(sl.size() == 0);
    REQUIRE(sl.capacity() == 0);

    sl.expand(3);
    REQUIRE(sl.size() == 3);
    REQUIRE(sl.capacity() == 3);

    REQUIRE(sl2 != sl);
    sl2 = sl;
    REQUIRE(sl2 == sl);

    sl2.resize(2);
    REQUIRE(sl2.size() == 2);
    REQUIRE(sl.size() == 3);
    REQUIRE(sl.capacity() == 3);
    REQUIRE(sl.begin() == sl2.begin());

    REQUIRE(sl2.resize(4) == false);
    REQUIRE(sl2.resize(4, true) == true);
    REQUIRE(sl2.size() == 4);
    REQUIRE(sl.size() == 3);
    REQUIRE(sl.capacity() == 4);
    REQUIRE(sl.begin() == sl2.begin());

    sl3 = sl2.copy(0, 2);
    REQUIRE(sl3.size() == 2);
    REQUIRE(sl3.capacity() == 2);
    REQUIRE(sl3.begin() != sl2.begin());
}

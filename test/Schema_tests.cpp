#include <ftl/dynamic_schema.h>
#include <ftl/unittest.h>
using namespace ftl;

/**

<root>
    <teacher>
        <name>abc</name>
        <id>123></id>
        <students>
            <student name="John", rate="12>
            <student>
                <name>Tom</name>
                <rate>12</rate>
            </student>
        </students>
    </teacher>
    <classrooms>
        <listentry>room1</listentry>
        <listentry>room2</listentry>
    </classrooms>
    <roomnumbers>
        <mapentry key="room1" value="23"/>
        <mapentry>
            <key>room2</key>
            <value>90</value>
        </mapentry>
</root>

{
    teacher: { name : abc,
               id: 123,
               students : [ {name: John, rate: 12}, {name: Tom, rate: 123} ]
             },
    classrooms: [ room1, room2 ],
    roomnumbers: {room1: 23, room2: 90}
}
**/

struct RootSchema
{
    struct Teacher
    {
        static auto &get_children_schema()
        {
            static const auto children = std::make_tuple( make_member_setter( &Teacher::name, "name" ),
                                                          make_member_setter( &Teacher::id, "id" ),
                                                          make_member_setter( &Teacher::students, "students" ) );
            return children;
        }

        struct Student
        {
            static auto &get_children_schema()
            {
                static const auto children =
                        std::make_tuple( make_member_setter( &Student::name, "name" ), make_member_setter( &Student::rate, "rate" ) );
                return children;
            }
            std::string name;
            float rate;
        };
        std::string name;
        int id;
        std::vector<Student> students;
    };

    static auto &get_children_schema()
    {
        static const auto &children = std::make_tuple( make_member_setter( &RootSchema::teacher, "teacher" ),
                                                       make_member_setter( &RootSchema::classrooms, "classrooms" ),
                                                       make_member_setter( &RootSchema::roomnumbers, "roomnumbers" ) );
        return children;
    }

    Teacher teacher;
    std::vector<std::string> classrooms;
    std::unordered_map<std::string, int> roomnumbers;
};


ADD_TEST_CASE( schema_tests )
{
    static_assert( HasMember_value_type<DynVec>::value, "value" );
    static_assert( !HasMember_c_str<DynVec>::value, "" );
    static_assert( IsLikeVec_v<DynVec>, "IsLikeVec_v" );
    static_assert( !IsLikeVec_v<DynStr>, "HasMember_c_str" );
    RootSchema schema;
    DynVar dynRoot = make_dynamic<DynVar, DynMap>();

    auto classrooms_dyn = make_dynamic<DynVar, DynVec>();
    classrooms_dyn.vec_add<std::string>( "room1" );
    classrooms_dyn.vec_add<std::string>( "room2" );
    dynRoot.map_add<DynVar>( "classrooms", std::move( classrooms_dyn ) );

    std::cout << dynRoot << std::endl;

    parse_schema( schema, dynRoot, std::cout );

    REQUIRE_EQ( schema.classrooms[0], "room1" );
}

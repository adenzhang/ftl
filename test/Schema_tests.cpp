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
                                                          make_member_setter( &Teacher::set_student_id, "id" ),
                                                          make_member_setter( &Teacher::students, "students" ) );
            return children;
        }

        struct Student
        {
            static auto &get_children_schema()
            {
                static const auto children =
                        std::make_tuple( make_member_setter( &Student::name, "name" ), make_member_setter( &Student::set_rate, "rate" ) );
                return children;
            }

            // @brief on_initialized is defined, it'll be called when initialization completes.
            void on_initialized( std::ostream &os )
            {
                os << "  Student " << name << " is initialized. \n";
            }


            // member method set_rate will be called by schema parser
            void set_rate( float arate )
            {
                rate = arate;
            }

            std::string name;

        protected:
            float rate;
        };

        // global function to set id to student.
        static void set_student_id( Teacher &obj, int id )
        {
            obj.id = id;
        }

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

static_assert( HasMember_on_initialized<RootSchema::Teacher::Student>::value, "Student HasMember_on_initialized" );

ADD_TEST_CASE( schema_tests )
{
    SECTION( "test dynamic_read_json" )
    {
        std::stringstream ss( "{  b: [ { ab:1, c:3 }, { ac: 23, ec: 2w } ],  c: \'asdf  we\' }" );
        DynVar dyn;

        dynamic_read_json( dyn, ss, std::cout );
        std::cout << dyn << std::endl;
    }
    static_assert( HasMember_value_type<DynVec>::value, "value" );
    static_assert( !HasMember_c_str<DynVec>::value, "" );
    static_assert( IsLikeMap_v<DynMap>, "IsLikeMap_v " );
    static_assert( IsLikeVec_v<DynVec &>, "IsLikeVec_v" );
    static_assert( IsLikeVec_v<std::vector<std::string>>, "IsLikeVec_v" );
    static_assert( !IsLikeVec_v<DynStr>, "HasMember_c_str" );
    static_assert( IsLikeVec_v<std::vector<RootSchema::Teacher::Student>>, "IsLikeVec_v" );

    SECTION( "test basic schema from dynamic" )
    {
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
    SECTION( "test read basic schema from json" )
    {
        const char *text = R"(
                           {
                               teacher: { name : abc,
                                          id: 123,
                                          students : [ {name: John, rate: 12}, {name: Tom, rate: 123} ]
                                        },
                               classrooms: [ room1, room2 ],
                               roomnumbers: {room1: 23, room2: 90}
                           }
)";
        std::stringstream ss( text );
        DynVar dyn;
        RootSchema schema;

        REQUIRE( dynamic_read_json( dyn, ss, std::cout ) );
        std::cout << dyn << std::endl;

        auto ok = parse_schema( schema, dyn, std::cout );
        REQUIRE( ok );

        REQUIRE_EQ( schema.classrooms.size(), size_t( 2 ) );
        REQUIRE_EQ( schema.roomnumbers.size(), size_t( 2 ) );
        REQUIRE_EQ( schema.teacher.id, 123 );
        REQUIRE_EQ( schema.teacher.name, "abc" );
        REQUIRE_EQ( schema.teacher.students.size(), size_t( 2 ) );
    }
    DynVar dyn;
}

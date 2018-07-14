#pragma once
#include <vector>
#include <string>

#define INFO std::cout << std::endl
#define ALERT std::cout << std::endl

#include <uv.h>
#include <map>

extern uv_loop_t* loop;
extern size_t HTTP_PORT;
extern size_t TCP_PORT;

static std::string SERVER_NAME = "Remote Server";
static std::string SERVER_VERSION = "0.1";

class AnyType
{
    public:
        enum class AnyTypeEnum
        {
            EMPTY,
            STRING,
            INT,
            DOUBLE,
            POINTER,
            BOOL
        };

        union {
                int    Int;
                std::string String;
                double Double;
                bool Bool;
                void* Pointer;
        };

        AnyTypeEnum Type;

        AnyType() { Type = AnyTypeEnum::EMPTY; }
        AnyType(int data) { Int = data; Type = AnyTypeEnum::EMPTY; }
        AnyType(std::string & data): String(std::move(data)) { Type = AnyTypeEnum::STRING; }
        AnyType(double data) { Double = data; Type = AnyTypeEnum::DOUBLE; }
        AnyType(bool data) { Bool = data; Type = AnyTypeEnum::BOOL; }
        AnyType(void* data) { Pointer = data; Type = AnyTypeEnum::POINTER; }

        ~AnyType()
        {
            if (Type == AnyTypeEnum::STRING)
                String.~basic_string();
        }
};

#define BEGIN_INIT_WITH_NAME(name) namespace { class __class_##name { public: __class_##name () {
#define END_INIT_WITH_NAME(name) } }; __class_##name init_##name; }

#define ADD_ROUTE(name, route, type, app, function) Response * name function \
    namespace { \
    BEGIN_INIT_WITH_NAME(name)\
    addRouteToApp( app , #route , type , name );\
    END_INIT_WITH_NAME(name)\
    }

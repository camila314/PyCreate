#include <fmt/core.h>
#include <fmt/ranges.h>
#include <broma.hpp>

char const* cpp_begin = R"BALL(//Codegen
#include <important.hpp>
#include <Geode/Geode.hpp>
#include <Geode/loader/IPC.hpp>

using namespace geode::prelude;

// dont use void use nullptr
template <typename R, typename U, typename ...Args>
matjson::Value listenerBuild(ipc::IPCEvent* event, U fn) {
    auto args = (*event->messageData).as_array();

    if constexpr (std::is_member_function_pointer<U>::value) {
        return toJson<true>(callMemberFn<R>(fn, fromJsonArr<Args>(args)...));
    } else {
        if constexpr (std::is_same_v<R, void> || std::is_same_v<R, TodoReturnPlaceholder>) {
            auto fn2 = reference_cast<swap_todo_t<U>>(fn);
            fn2(fromJsonArr<Args>(args)...);
            return nullptr;
        } else {
            return toJson<true>(fn(fromJsonArr<Args>(args)...));
        }
    }
}

template <int32_t id>
struct ListenerClass {
    static void apply() {}
};

#define LISTEN(id, rt, function, ...) \
    constexpr int32_t num##id = __COUNTER__; \
    template <> \
    struct geode::modifier::ModifyDerive<ListenerClass<num##id>, cocos2d::CCSpriteFrameCache> { \
        public: static void apply() { \
            ipc::listen("bind-" #id, +[](ipc::IPCEvent* event) -> matjson::Value { \
                auto args = (*event->messageData).as_array(); \
                matjson::Value out; \
                bool done = false; \
                Loader::get()->queueInMainThread([&]() { \
                    out = listenerBuild<rt, decltype(function)__VA_OPT__(,) __VA_ARGS__>(event, function); \
                    done = true; \
                }); \
                while (!done) {} \
                return out; \
            }); \
        } \
    }; \
    template <> \
    struct ListenerClass<num##id> { \
        static void apply() { \
            geode::modifier::ModifyDerive<ListenerClass<num##id>, cocos2d::CCSpriteFrameCache>::apply(); \
            ListenerClass<num##id - 1>::apply(); \
        } \
    };

)BALL";

constexpr char const* cpp_end = R"BALL(
$execute {
    ListenerClass<__COUNTER__ - 1>::apply();
}
)BALL";

constexpr char const* enums[] = {
    "cocos2d::ccGLServerState",
    "cocos2d::ccTouchesMode",
    "cocos2d::enumKeyCodes",
    "cocos2d::CCTexture2DPixelFormat",
    "cocos2d::PopTransition",
    "IconType",
    "UnlockType",
    "DialogAnimationType",
    "DialogChatPlacement",
    "BoomListType",
    "cocos2d::CCTextAlignment",
    "cocos2d::CCImage::EImageFormat",
    "GauntletType",
    "MenuAnimationType",
    "TableViewCellEditingStyle",
    "DemonDifficultyType",
    "LikeItemType",
    "CommentError",
    "UpdateResponse",
    "cocos2d::tCCPositionType",
    "GameObjectType",
    "GJTimedLevelType",
    "UserListType",
    "CommentKeyType",
    "LevelLeaderboardType",
    "LevelLeaderboardMode",
    "CommentType",
    "GJScoreType",
    "GJHttpType",
    "SearchType",
    "StatKey",
    "ShopType",
    "PlayerButton",
    "CellAction",
    "GJRewardType",
    "SpecialRewardItem",
    "ChestSpriteState",
    "GJSongError",
    "GJMusicAction",
    "LeaderboardState",
    "GJDifficultyName",
};

constexpr char const* structs[] = {
    "cocos2d::ccColor3B",
    "cocos2d::ccColor4B",
    "cocos2d::ccColor4F",
    "cocos2d::_ccColor3B",
    "cocos2d::_ccColor4B",
    "cocos2d::_ccColor4F",
    "cocos2d::ccHSVValue",
    "cocos2d::_ccBlendFunc",
    "cocos2d::CCRect",
    "cocos2d::CCSize",
    "cocos2d::CCPoint",
    "cocos2d::CCAffineTransform",
    "cocos2d::CCIMEKeyboardNotificationInfo",
    "cocos2d::CCArray*"
};

constexpr char const* cpp_template = "LISTEN({id}, {return}, {function}{args})\n";
constexpr char const* cpp_cast_template = "static_cast<{return}({member}*)({args}){const}>(&{value})";

std::string parseFnCpp(broma::Class const& cls, broma::MemberFunctionProto* fn, size_t id) {

    std::string args;
    for (auto& arg : fn->args)
        args += ", " + arg.first.name;

    std::string stripped_args = args.empty() ? args : args.substr(2);

    if (!fn->is_static)
        args = ", " + cls.name + "*" + args;

    std::string member = fn->is_static ? "" : (cls.name + "::");
    std::string function = fmt::format(cpp_cast_template,
        fmt::arg("return", fn->ret.name),
        fmt::arg("member", member),
        fmt::arg("args", stripped_args),
        fmt::arg("value", cls.name + "::" + fn->name),
        fmt::arg("const", fn->is_const ? " const" : "")
    );

    return fmt::format(cpp_template,
        fmt::arg("id", id),
        fmt::arg("return", fn->ret.name),
        fmt::arg("function", function),
        fmt::arg("args", args)
    );
}

std::string parseCpp(broma::Root& root) {
    std::string out = cpp_begin;

    for (auto& cls : root.classes) {
        for (auto& field : cls.fields) {
            if (auto bind = field.get_as<broma::FunctionBindField>()) {
                #if __APPLE__
                    if (bind->binds.mac == -1)
                        continue;
                #else
                    if (bind->binds.win == -1)
                        continue;
                #endif
            }

            if (auto fn = field.get_fn()) {
                if (fn->type == broma::FunctionType::Normal) {
                    out += parseFnCpp(cls, fn, field.field_id);
                }
            }
        }
    }

    out += cpp_end;

    return out;
}

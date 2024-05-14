#include <fmt/core.h>
#include <fmt/ranges.h>
#include <broma.hpp>

constexpr char const* hook_begin = R"BALL(//Codegen
#include <important.hpp>
#include <Geode/Geode.hpp>
#include <Geode/utils/web.hpp>
#include <Geode/Modify.hpp>
#include <Geode/loader/IPC.hpp>

using namespace geode::prelude;

template <int id, typename ...Args>
void callback(Args... args) {
	std::vector<matjson::Value> arr = {toJson<false>(args)...};
	web::AsyncWebRequest()
		.body(arr)
		.post("http://localhost:3945/" + std::to_string(id));

}
)BALL";

constexpr char const* hook_cls = R"BALL(
class $modify({cls}) {{
	static void onModify(auto& self) {{
		{enables}
	}}
	{content}
}};
)BALL";

constexpr char const* hook_fn = R"BALL(
	{if_static}{ret} {name}({argdef}) {{
		callback<{id}>({this_args});

		if constexpr (std::is_same_v<{ret}, void>) {{
			{cls}::{name}({args});
		}} else {{
			return {cls}::{name}({args});
		}}
	}})BALL";

constexpr char const* hook_enable = R"BALL(
		{{
			if (auto h = self.getHook("{cls}::{name}")) {{
				h.unwrap()->setAutoEnable(false);
				new EventListener([=](auto ev) -> matjson::Value {{
					auto _ = h.unwrap()->enable();
					return {{ }};
				}}, geode::ipc::IPCFilter(getMod()->getID(), "hook-{id}"));
			}}
		}})BALL";

std::string stripQualifiers(std::string const& inp) {
	// find index of last colon
	size_t pos = inp.find_last_of(':');
	if (pos == std::string::npos)
		return inp;

	return inp.substr(pos + 1);
}

std::string parseEnableCppHook(broma::Class const& cls, broma::MemberFunctionProto* fn, size_t id) {
	return fmt::format(hook_enable,
		fmt::arg("id", id),
		fmt::arg("cls", cls.name),
		fmt::arg("name", fn->name)
	);
}

std::string parseFnCppHook(broma::Class const& cls, broma::MemberFunctionProto* fn, size_t id) {
	std::string ret = fn->ret.name;
	std::string name = fn->name;
	std::vector<std::string> argdef;
	std::vector<std::string> args;
	std::vector<std::string> this_args;

	if (ret == "TodoReturn")
		return "";

	if (!fn->is_static)
		this_args.push_back("this");

	int i = 0;
	for (auto& [k, v] : fn->args) {
		argdef.push_back(k.name + " arg" + std::to_string(i));
		args.push_back("arg" + std::to_string(i));
		this_args.push_back("arg" + std::to_string(i));
		++i;
	}

	return fmt::format(hook_fn,
		fmt::arg("id", id),
		fmt::arg("if_static", fn->is_static ? "static " : ""),
		fmt::arg("ret", ret),
		fmt::arg("cls", cls.name),
		fmt::arg("name", name),
		fmt::arg("argdef", fmt::join(argdef, ", ")),
		fmt::arg("args", fmt::join(args, ", ")),
		fmt::arg("this_args", fmt::join(this_args, ", "))
	);
}


std::string parseClassCppHook(broma::Class& cls) {
	if (cls.name == "cocos2d" || cls.name == "cocos2d::CCSpriteFrameCache" || cls.name == "cocos2d::CCTexture2D")
		return "";

	std::string content;
	std::string enables;
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
				content += parseFnCppHook(cls, fn, field.field_id);
				enables += parseEnableCppHook(cls, fn, field.field_id);
			}
		}
	}

	return fmt::format(hook_cls,
		fmt::arg("cls", stripQualifiers(cls.name)),
		fmt::arg("content", content),
		fmt::arg("enables", enables)
	);
}

std::string parseCppHook(broma::Root& root) {
	std::string out = hook_begin;

	for (auto& cls : root.classes) {
		out += parseClassCppHook(cls);
	}

	return out;
};
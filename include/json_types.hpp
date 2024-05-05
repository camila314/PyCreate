#pragma once

#include "Geode/cocos/cocoa/CCAffineTransform.h"
#include <Geode/Geode.hpp>
#include <type_traits>
#include <matjson.hpp>

using namespace geode::prelude;

template<typename T>
concept pointer = (std::is_pointer_v<T> || std::is_member_function_pointer_v<T>) && !std::is_same_v<CCArray*, T> && !std::is_same_v<char const*, T>;

template<typename T>
concept supports_as = !pointer<T> && requires(matjson::Value const& t) {
	t.as<T>();
};

template<typename T> concept enum_type = std::is_enum_v<T>;

#define ENUM_SER(name) \
	template<> \
	struct matjson::Serialize<name> { \
		static name from_json(matjson::Value const& val) { \
			return static_cast<name>(val.as<int>()); \
		} \
		static matjson::Value to_json(name const& val) { \
			return static_cast<int>(val); \
		} \
	};

#define REF_SER(name) \
	template<> \
	struct matjson::Serialize<name> { \
		static name from_json(matjson::Value const& val) { \
			return *reference_cast<name*>(std::stol(val.as_string(), 0, 16)); \
		} \
		static matjson::Value to_json(name const& val) { \
			std::stringstream ss; \
			ss << std::hex << reference_cast<long>(&val); \
			return ss.str(); \
		} \
	};

#define EMPTY_SER(name) \
	template <> \
	struct matjson::Serialize<name> { \
		static name from_json(matjson::Value const& val) { \
			return {}; \
		} \
		static matjson::Value to_json(name const& val) { \
			return nullptr; \
		} \
	};

REF_SER(CCIndexPath)

#undef CommentType

/*ENUM_SER(cocos2d::ccGLServerState)
ENUM_SER(cocos2d::ccTouchesMode)
ENUM_SER(cocos2d::enumKeyCodes)
ENUM_SER(cocos2d::CCTexture2DPixelFormat)
ENUM_SER(cocos2d::PopTransition)
ENUM_SER(IconType)
ENUM_SER(UnlockType)
ENUM_SER(DialogAnimationType)
ENUM_SER(DialogChatPlacement)
ENUM_SER(BoomListType)
ENUM_SER(cocos2d::CCTextAlignment)
ENUM_SER(cocos2d::CCImage::EImageFormat)
ENUM_SER(GauntletType)
ENUM_SER(MenuAnimationType)
ENUM_SER(TableViewCellEditingStyle)
ENUM_SER(DemonDifficultyType)
ENUM_SER(LikeItemType)
ENUM_SER(CommentError)
ENUM_SER(UpdateResponse)
ENUM_SER(cocos2d::tCCPositionType)
ENUM_SER(GameObjectType)
ENUM_SER(GJTimedLevelType)
ENUM_SER(UserListType)
ENUM_SER(CommentKeyType)
ENUM_SER(LevelLeaderboardType)
ENUM_SER(LevelLeaderboardMode)
ENUM_SER(CommentType)
ENUM_SER(GJScoreType)
ENUM_SER(GJHttpType)
ENUM_SER(SearchType)
ENUM_SER(StatKey)
ENUM_SER(ShopType)
ENUM_SER(PlayerButton)
ENUM_SER(CellAction)
ENUM_SER(GJRewardType)
ENUM_SER(SpecialRewardItem)
ENUM_SER(ChestSpriteState)
ENUM_SER(GJSongError)
ENUM_SER(GJMusicAction)
ENUM_SER(LeaderboardState)
ENUM_SER(GJDifficultyName)
ENUM_SER(GJGameEvent)
ENUM_SER(GhostType)
ENUM_SER(AccountError)
ENUM_SER(AudioSortType)
ENUM_SER(AudioTargetType)
ENUM_SER(BackupAccountError)
ENUM_SER(CurrencyRewardType)
ENUM_SER(CurrencySpriteType)
ENUM_SER(EditCommand)
ENUM_SER(FMODReverbPreset)
ENUM_SER(GJErrorCode)
ENUM_SER(GJMPErrorCode)
ENUM_SER(ObjectScaleType)
ENUM_SER(UndoCommand)
ENUM_SER(PlayerCollisionDirection)
ENUM_SER(AudioModType)
ENUM_SER(FMODQueuedMusic)
ENUM_SER(AudioGuidelinesType)*/

EMPTY_SER(CAState)
EMPTY_SER(ChanceObject)
EMPTY_SER(cocos2d::ParticleStruct)
EMPTY_SER(EffectManagerState)
EMPTY_SER(FMODAudioState)
EMPTY_SER(SFXTriggerInstance)
using silly2 = std::map<std::string, std::string>;
EMPTY_SER(silly2)
EMPTY_SER(UIButtonConfig)
EMPTY_SER(GJFeatureState)


template <enum_type T>
struct matjson::Serialize<T> {
	static T from_json(matjson::Value const& val) {
		return static_cast<T>(val.as<int>());
	}

	static matjson::Value to_json(T const& val) {
		return static_cast<int>(val);
	}
};

template <pointer T>
struct matjson::Serialize<T> {
	static T from_json(matjson::Value const& val) {
		return reference_cast<T>(std::stol(val.as_string(), 0, 16));
	}

	static matjson::Value to_json(T const& val) {
		if constexpr (std::is_base_of_v<CCObject, T>) {
			val->retain();
		}

		std::stringstream ss;
		ss << std::hex << reference_cast<long>(val);
		return ss.str();
	}
};

template <typename T>
struct matjson::Serialize<std::vector<T>> {
	static std::vector<T> from_json(matjson::Value const& val) {
		std::vector<T> vec;
		for (auto& v : val.as_array()) {
			vec.push_back(v.as<T>());
		}
		return vec;
	}

	static matjson::Value to_json(std::vector<T> const& val) {
		std::vector<matjson::Value> arr;
		for (auto& v : val) {
			arr.push_back(Serialize<T>().to_json(v));
		}
		return arr;
	}
};

template <typename T>
struct matjson::Serialize<std::set<T>> {
	static std::set<T> from_json(matjson::Value const& val) {
		std::set<T> set;
		for (auto& v : val.as_array()) {
			set.insert(v.as<T>());
		}
		return set;
	}

	static matjson::Value to_json(std::set<T> const& val) {
		std::vector<matjson::Value> arr;
		for (auto& v : val) {
			arr.push_back(Serialize<T>().to_json(v));
		}
		return arr;
	}
};


template <>
struct matjson::Serialize<char const*> {
	static char const* from_json(matjson::Value const& val) {
		auto obj = CCString::create(val.as_string());
		return obj->getCString();
	}

	static matjson::Value to_json(char const* val) {
		return val;
	}
};

template <>
struct matjson::Serialize<gd::string> {
	static gd::string from_json(matjson::Value const& val) {
		return val.as_string();
	}

	static matjson::Value to_json(gd::string const& val) {
		return std::string(val);
	}
};


template <>
struct matjson::Serialize<CCPoint> {
	static CCPoint from_json(matjson::Value const& val) {
		return { (float)val[0].as<float>(), (float)val[1].as<float>() };
	}

	static matjson::Value to_json(CCPoint const& val) {
		std::vector<matjson::Value> arr = { val.x, val.y };
		return arr;
	}
};

template <>
struct matjson::Serialize<CCSize> {
	static CCSize from_json(matjson::Value const& val) {
		return { (float)val[0].as<float>(), (float)val[1].as<float>() };
	}

	static matjson::Value to_json(CCSize const& val) {
		std::vector<matjson::Value> arr = { val.width, val.height };
		return arr;
	}
};

template <>
struct matjson::Serialize<CCRect> {
	static CCRect from_json(matjson::Value const& val) {
		return { (float)val[0].as<float>(), (float)val[1].as<float>(), (float)val[2].as<float>(), (float)val[3].as<float>() };
	}

	static matjson::Value to_json(CCRect const& val) {
		std::vector<matjson::Value> arr = { val.origin.x, val.origin.y, val.size.width, val.size.height };
		return arr;
	}
};

template <>
struct matjson::Serialize<_ccColor4F> {
	static _ccColor4F from_json(matjson::Value const& val) {
		return { (float)val[0].as<float>(), (float)val[1].as<float>(), (float)val[2].as<float>(), (float)val[3].as<float>() };
	}

	static matjson::Value to_json(_ccColor4F const& val) {
		std::vector<matjson::Value> arr = { val.r, val.g, val.b, val.a };
		return arr;
	}
};

template <>
struct matjson::Serialize<_ccHSVValue> {
	static _ccHSVValue from_json(matjson::Value const& val) {
		return { (float)val[0].as<float>(), (float)val[1].as<float>(), (float)val[2].as<float>(), (GLubyte)val[3].as<int>(), (GLubyte)val[4].as<int>(), val[5].as<bool>(), val[6].as<bool>() };
	}

	static matjson::Value to_json(_ccHSVValue const& val) {
		std::vector<matjson::Value> arr = { val.h, val.s, val.v, (int)val.absoluteSaturation, (int)val.absoluteBrightness, val.saturationChecked, val.brightnessChecked };
		return arr;
	}
};

// _ccBlendFunc
template <>
struct matjson::Serialize<_ccBlendFunc> {
	static _ccBlendFunc from_json(matjson::Value const& val) {
		return { (GLenum)val[0].as<int>(), (GLenum)val[1].as<int>() };
	}

	static matjson::Value to_json(_ccBlendFunc const& val) {
		std::vector<matjson::Value> arr = { (int)val.src, (int)val.dst };
		return arr;
	}
};

template <>
struct matjson::Serialize<CCIMEKeyboardNotificationInfo> {
	static CCIMEKeyboardNotificationInfo from_json(matjson::Value const& val) {
		return { Serialize<CCRect>().from_json(val[0]), Serialize<CCRect>().from_json(val[1]), (float)val[2].as<float>() };
	}

	static matjson::Value to_json(CCIMEKeyboardNotificationInfo const& val) {
		std::vector<matjson::Value> arr = { Serialize<CCRect>().to_json(val.begin), Serialize<CCRect>().to_json(val.end), val.duration };
		return arr;
	}
};


template <>
struct matjson::Serialize<CCAffineTransform> {
	static CCAffineTransform from_json(matjson::Value const& val) {
		return { (float)val[0].as<float>(), (float)val[1].as<float>(), (float)val[2].as<float>(), (float)val[3].as<float>(), (float)val[4].as<float>(), (float)val[5].as<float>() };
	}

	static matjson::Value to_json(CCAffineTransform const& val) {
		std::vector<matjson::Value> arr = { val.a, val.b, val.c, val.d, val.tx, val.ty };
		return arr;
	}
};

template <>
struct matjson::Serialize<CCArray*> {
	static CCArray* from_json(matjson::Value const& val) {
		auto arr = CCArray::create();
		for (auto& v : val.as_array()) {
			arr->addObject(Serialize<CCObject*>().from_json(v));
		}
		return arr;
	}

	static matjson::Value to_json(CCArray* val) {
		std::vector<matjson::Value> arr;
		if (val == nullptr)
			return arr;
		for (int i = 0; i < val->count(); i++) {
			val->objectAtIndex(i)->retain();
			arr.push_back(Serialize<CCObject*>().to_json(val->objectAtIndex(i)));
		}
		return arr;
	}
};

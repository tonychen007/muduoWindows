#pragma once

#include "noncopyable.h"
#include <Windows.h>


template<typename T>
struct has_no_destroy {
	template <typename C> static char test(decltype(&C::no_destroy));
	template <typename C> static int32_t test(...);
	const static bool value = sizeof(test<T>(0)) == 1;
};

template<typename T>
class Singleton : noncopyable {
public:
	Singleton() = delete;
	~Singleton() = delete;

	static T& instance() {
		InitOnceExecuteOnce(&once_, init, NULL, NULL);
		assert(value_ != NULL);
		return *value_;
	}

private:
	static BOOL CALLBACK init(PINIT_ONCE InitOnce, PVOID Parameter, PVOID* lpContext) {
		//printf("Inside Singleton::init\n");
		value_ = new T();
		if (!has_no_destroy<T>::value) {
			::atexit(destroy);
		}

		return TRUE;
	}

	static void destroy() {
		typedef char T_must_be_complete_type[sizeof(T) == 0 ? -1 : 1];
		T_must_be_complete_type dummy; (void)dummy;

		delete value_;
		value_ = NULL;
	}
private:
	static T* value_;
	static INIT_ONCE once_;
};

template<typename T>
T* Singleton<T>::value_ = NULL;

template<typename T>
INIT_ONCE Singleton<T>::once_ = INIT_ONCE_STATIC_INIT;
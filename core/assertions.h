/*
===================================================================================================

	Assertion Handler

	Permanently ignoring an assertion on a specific line will disable all asserts on that line
	so don't put assertions on the same line!

	cassert and assert.h in MSVC do NOT have include guards for some reason, which means every
	include would stomp on our override if we had one! That sucks

===================================================================================================
*/

#pragma once

extern void AssertionFailed( const char *message, const char *expression, const char *file, uint32 line );

#if defined Q_DEBUG && !defined Q_ASSERTIONS_ENABLED
#define Q_ASSERTIONS_ENABLED
#endif

#ifdef Q_ASSERTIONS_ENABLED

#define Assert(exp)				(static_cast<bool>(exp) ? void(0) : AssertionFailed(nullptr, #exp, __FILE__, __LINE__))

#define AssertMsg(exp, msg)		(static_cast<bool>(exp) ? void(0) : AssertionFailed(msg, #exp, __FILE__, __LINE__))

#else

#define Assert(exp)

#define AssertMsg(exp, msg)

#endif

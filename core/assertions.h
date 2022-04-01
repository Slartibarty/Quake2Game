/*
===================================================================================================

	Assertion Handler

	Permanently ignoring an assertion on a specific line will disable all asserts on that line
	so don't put assertions on the same line!

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

#ifdef assert
#error some standard header included assert before we could define it ourselves!
#endif

#define assert Assert

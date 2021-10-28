
// Core
#include "../../core/core.h"

// C++ headers we like
#include <vector>
#include <string>

// Framework
#include "../../framework/framework_public.h"

// GLEW
#include "GL/glew.h"

#define QT_NO_OPENGL

#pragma push_macro("free")
#undef free

// Qt
#if 0
#include <QtGui/QWheelEvent>
#include <QtGui/QPainter>
#include <QtGui/QPen>
#include <QtWidgets/QWidget>
#include <QtWidgets/QFrame>
#include <QtWidgets/QMainWindow>
#include <QtWidgets/QMdiArea>
#include <QtWidgets/QMdiSubWindow>
#include <QtWidgets/QGridLayout>
#include <QtOpenGLWidgets/QOpenGLWidget>
#else
#include <QtWidgets/QtWidgets>
#include <QtOpenGLWidgets/QtOpenGLWidgets>
#endif

#pragma pop_macro("free")

#define APP_COMPANYNAME		"Freeze Team"
#define APP_NAME			"MoonEd"
#define APP_VERSION			"1.0.0"

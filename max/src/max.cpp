/*
 * Copyright 2024 Marcus Madland. All rights reserved.
 * License: https://github.com/marcusmadland/max/blob/mainICENSE
 */

#include <bx/platform.h>

#include "max_p.h"
#include <max/embedded_shader.h>
#include <bimg/decode.h>
#include <bx/file.h>
#include <bx/mutex.h>

#include "topology.h"

#if BX_PLATFORM_OSX || BX_PLATFORM_IOS || BX_PLATFORM_VISIONOS
#	include <objc/message.h>
#elif BX_PLATFORM_WINDOWS
#	ifndef WIN32_LEAN_AND_MEAN
#		define WIN32_LEAN_AND_MEAN
#	endif // WIN32_LEAN_AND_MEAN
#	include <windows.h>
#endif // BX_PLATFORM_OSX

BX_ERROR_RESULT(MAX_ERROR_TEXTURE_VALIDATION,      BX_MAKEFOURCC('b', 'g', 0, 1) );
BX_ERROR_RESULT(MAX_ERROR_FRAME_BUFFER_VALIDATION, BX_MAKEFOURCC('b', 'g', 0, 2) );
BX_ERROR_RESULT(MAX_ERROR_IDENTIFIER_VALIDATION,   BX_MAKEFOURCC('b', 'g', 0, 3) );

namespace max
{
#define MAX_API_THREAD_MAGIC UINT32_C(0x78666762)

#if MAX_CONFIG_MULTITHREADED

#	define MAX_CHECK_API_THREAD()                                   \
		BX_ASSERT(NULL != s_ctx, "Library is not initialized yet."); \
		BX_ASSERT(MAX_API_THREAD_MAGIC == s_threadIndex, "Must be called from main thread.")

#	define MAX_CHECK_RENDER_THREAD()                         \
		BX_ASSERT( (NULL != s_ctx && s_ctx->m_singleThreaded) \
			|| ~MAX_API_THREAD_MAGIC == s_threadIndex        \
			, "Must be called from render thread."            \
			)

#else
#	define MAX_CHECK_API_THREAD()
#	define MAX_CHECK_RENDER_THREAD()
#endif // MAX_CONFIG_MULTITHREADED

#define MAX_CHECK_CAPS(_caps, _msg)                                                   \
	BX_ASSERT(0 != (g_caps.supported & (_caps) )                                       \
		, _msg " Use max::getCaps to check " #_caps " backend renderer capabilities." \
		);

#if MAX_CONFIG_IMPLEMENT_DEFAULT_ALLOCATOR
	bx::AllocatorI* getDefaultAllocator()
	{
		BX_PRAGMA_DIAGNOSTIC_PUSH();
		BX_PRAGMA_DIAGNOSTIC_IGNORED_MSVC(4459); // warning C4459: declaration of 's_allocator' hides global declaration
		BX_PRAGMA_DIAGNOSTIC_IGNORED_CLANG_GCC("-Wshadow");
		static bx::DefaultAllocator s_allocator;
		return &s_allocator;
		BX_PRAGMA_DIAGNOSTIC_POP();
	}
#endif // MAX_CONFIG_IMPLEMENT_DEFAULT_ALLOCATOR

#if MAX_CONFIG_USE_TINYSTL
	void* TinyStlAllocator::static_allocate(size_t _bytes)
	{
		return bx::alloc(getDefaultAllocator(), _bytes);
	}

	void TinyStlAllocator::static_deallocate(void* _ptr, size_t /*_bytes*/)
	{
		if (NULL != _ptr)
		{
			bx::free(getDefaultAllocator(), _ptr);
		}
	}
#endif // MAX_CONFIG_USE_TINYSTL

	struct CallbackStub : public CallbackI
	{
		virtual ~CallbackStub()
		{
		}

		virtual void fatal(const char* _filePath, uint16_t _line, Fatal::Enum _code, const char* _str) override
		{
			max::trace(_filePath, _line, "MAX FATAL 0x%08x: %s\n", _code, _str);

			if (Fatal::DebugCheck == _code)
			{
				bx::debugBreak();
			}
			else
			{
				abort();
			}
		}

		virtual void traceVargs(const char* _filePath, uint16_t _line, const char* _format, va_list _argList) override
		{
			char temp[2048];
			char* out = temp;
			va_list argListCopy;
			va_copy(argListCopy, _argList);
			int32_t len   = bx::snprintf(out, sizeof(temp), "%s (%d): ", _filePath, _line);
			int32_t total = len + bx::vsnprintf(out + len, sizeof(temp)-len, _format, argListCopy);
			va_end(argListCopy);
			if ( (int32_t)sizeof(temp) < total)
			{
				out = (char*)alloca(total+1);
				bx::memCopy(out, temp, len);
				bx::vsnprintf(out + len, total-len, _format, _argList);
			}
			out[total] = '\0';
			bx::debugOutput(out);
		}

		virtual void profilerBegin(const char* /*_name*/, uint32_t /*_abgr*/, const char* /*_filePath*/, uint16_t /*_line*/) override
		{
		}

		virtual void profilerBeginLiteral(const char* /*_name*/, uint32_t /*_abgr*/, const char* /*_filePath*/, uint16_t /*_line*/) override
		{
		}

		virtual void profilerEnd() override
		{
		}

		virtual uint32_t cacheReadSize(uint64_t /*_id*/) override
		{
			return 0;
		}

		virtual bool cacheRead(uint64_t /*_id*/, void* /*_data*/, uint32_t /*_size*/) override
		{
			return false;
		}

		virtual void cacheWrite(uint64_t /*_id*/, const void* /*_data*/, uint32_t /*_size*/) override
		{
		}

		virtual void screenShot(const char* _filePath, uint32_t _width, uint32_t _height, uint32_t _pitch, const void* _data, uint32_t _size, bool _yflip) override
		{
			BX_UNUSED(_filePath, _width, _height, _pitch, _data, _size, _yflip);

			const int32_t len = bx::strLen(_filePath)+5;
			char* filePath = (char*)alloca(len);
			bx::strCopy(filePath, len, _filePath);
			bx::strCat(filePath, len, ".tga");

			bx::FileWriter writer;
			if (bx::open(&writer, filePath) )
			{
				bimg::imageWriteTga(&writer, _width, _height, _pitch, _data, false, _yflip);
				bx::close(&writer);
			}
		}

		virtual void captureBegin(uint32_t /*_width*/, uint32_t /*_height*/, uint32_t /*_pitch*/, TextureFormat::Enum /*_format*/, bool /*_yflip*/) override
		{
			BX_TRACE("Warning: using capture without callback (a.k.a. pointless).");
		}

		virtual void captureEnd() override
		{
		}

		virtual void captureFrame(const void* /*_data*/, uint32_t /*_size*/) override
		{
		}
	};

#ifndef MAX_CONFIG_MEMORY_TRACKING
#	define MAX_CONFIG_MEMORY_TRACKING (MAX_CONFIG_DEBUG && BX_CONFIG_SUPPORTS_THREADING)
#endif // MAX_CONFIG_MEMORY_TRACKING

	const size_t kNaturalAlignment = 8;

	class AllocatorStub : public bx::AllocatorI
	{
	public:
		AllocatorStub()
#if MAX_CONFIG_MEMORY_TRACKING
			: m_numBlocks(0)
			, m_maxBlocks(0)
#endif // MAX_CONFIG_MEMORY_TRACKING
		{
		}

		virtual void* realloc(void* _ptr, size_t _size, size_t _align, const char* _file, uint32_t _line) override
		{
			if (0 == _size)
			{
				if (NULL != _ptr)
				{
					if (kNaturalAlignment >= _align)
					{
#if MAX_CONFIG_MEMORY_TRACKING
						{
							bx::MutexScope scope(m_mutex);
							BX_ASSERT(m_numBlocks > 0, "Number of blocks is 0. Possible alloc/free mismatch?");
							--m_numBlocks;
						}
#endif // MAX_CONFIG_MEMORY_TRACKING

						::free(_ptr);
					}
					else
					{
						bx::alignedFree(this, _ptr, _align, bx::Location(_file, _line) );
					}
				}

				return NULL;
			}
			else if (NULL == _ptr)
			{
				if (kNaturalAlignment >= _align)
				{
#if MAX_CONFIG_MEMORY_TRACKING
					{
						bx::MutexScope scope(m_mutex);
						++m_numBlocks;
						m_maxBlocks = bx::max(m_maxBlocks, m_numBlocks);
					}
#endif // MAX_CONFIG_MEMORY_TRACKING

					return ::malloc(_size);
				}

				return bx::alignedAlloc(this, _size, _align, bx::Location(_file, _line) );
			}

			if (kNaturalAlignment >= _align)
			{
				return ::realloc(_ptr, _size);
			}

			return bx::alignedRealloc(this, _ptr, _size, _align, bx::Location(_file, _line) );
		}

		void checkLeaks();

	protected:
#if MAX_CONFIG_MEMORY_TRACKING
		bx::Mutex m_mutex;
		uint32_t m_numBlocks;
		uint32_t m_maxBlocks;
#endif // MAX_CONFIG_MEMORY_TRACKING
	};

	static CallbackStub*  s_callbackStub  = NULL;
	static AllocatorStub* s_allocatorStub = NULL;
	static bool s_graphicsDebuggerPresent = false;

	CallbackI* g_callback = NULL;
	bx::AllocatorI* g_allocator = NULL;

	static uint32_t s_debug =  MAX_DEBUG_NONE;
	static uint32_t s_reset =  MAX_RESET_NONE;
	static uint32_t s_width =  MAX_DEFAULT_WIDTH;
	static uint32_t s_height = MAX_DEFAULT_HEIGHT;
	static bool s_exit = false;

	Caps g_caps;

	bx::AllocatorI* getAllocator()
	{
		//if (NULL == g_allocator)
		//{
		//	g_allocator = getDefaultAllocator();
		//}
		//
		//return g_allocator;
		return getDefaultAllocator();
	}

#if MAX_CONFIG_MULTITHREADED && !defined(BX_THREAD_LOCAL)
	class ThreadData
	{
		BX_CLASS(ThreadData
			, NO_DEFAULT_CTOR
			, NO_COPY
			);

	public:
		ThreadData(uintptr_t _rhs)
		{
			union { uintptr_t ui; void* ptr; } cast = { _rhs };
			m_tls.set(cast.ptr);
		}

		operator uintptr_t() const
		{
			union { uintptr_t ui; void* ptr; } cast;
			cast.ptr = m_tls.get();
			return cast.ui;
		}

		uintptr_t operator=(uintptr_t _rhs)
		{
			union { uintptr_t ui; void* ptr; } cast = { _rhs };
			m_tls.set(cast.ptr);
			return _rhs;
		}

		bool operator==(uintptr_t _rhs) const
		{
			uintptr_t lhs = *this;
			return lhs == _rhs;
		}

	private:
		bx::TlsData m_tls;
	};

	static ThreadData s_threadIndex(0);
#elif !MAX_CONFIG_MULTITHREADED
	static uint32_t s_threadIndex(0);
#else
	static BX_THREAD_LOCAL uint32_t s_threadIndex(0);
#endif

	static Context* s_ctx = NULL;
	static bool s_renderFrameCalled = false;
	InternalData g_internalData;
	PlatformData g_platformData;
	bool g_platformDataChangedSinceReset = false;

#if BX_PLATFORM_EMSCRIPTEN
	static AppI* s_app;
	static void updateApp()
	{
		appUpdate();
		s_app->update();
	}
#endif // BX_PLATFORM_EMSCRIPTEN

	max::VertexLayout DebugPosVertex::ms_layout;
	max::VertexLayout DebugUvVertex::ms_layout;
	max::VertexLayout DebugShapeVertex::ms_layout;
	max::VertexLayout DebugMeshVertex::ms_layout;

	DebugDrawShared s_dds;
	DebugDrawEncoderImpl s_dde;

	static AppI* s_currentApp = NULL;
	static AppI* s_apps = NULL;
	static uint32_t s_numApps = 0;

	static char s_restartArgs[1024] = { '\0' };

	static AppI* getCurrentApp(AppI* _set = NULL)
	{
		if (NULL != _set)
		{
			s_currentApp = _set;
		}
		else if (NULL == s_currentApp)
		{
			s_currentApp = getFirstApp();
		}

		return s_currentApp;
	}

	static AppI* getNextWrap(AppI* _app)
	{
		AppI* next = _app->getNext();
		if (NULL != next)
		{
			return next;
		}

		return getFirstApp();
	}

	struct AppInternal
	{
		AppI* m_next;
		const char* m_name;
	};

	static ptrdiff_t s_offset = 0;
	static int64_t s_timeOffset = 0;
	static float s_deltaTime = 0.0f;

	AppI::AppI(const char* _name)
	{
		BX_STATIC_ASSERT(sizeof(AppInternal) <= sizeof(m_internal));
		s_offset = BX_OFFSETOF(AppI, m_internal);

		AppInternal* ai = (AppInternal*)m_internal;

		ai->m_name = _name;
		ai->m_next = s_apps;

		s_apps = this;
		s_numApps++;
	}

	AppI::~AppI()
	{
		for (AppI* prev = NULL, *app = s_apps, *next = app->getNext()
			; NULL != app
			; prev = app, app = next, next = app->getNext())
		{
			if (app == this)
			{
				if (NULL != prev)
				{
					AppInternal* ai = bx::addressOf<AppInternal>(prev, s_offset);
					ai->m_next = next;
				}
				else
				{
					s_apps = next;
				}

				--s_numApps;

				break;
			}
		}
	}

	const char* AppI::getName() const
	{
		AppInternal* ai = (AppInternal*)m_internal;
		return ai->m_name;
	}

	AppI* AppI::getNext()
	{
		AppInternal* ai = (AppInternal*)m_internal;
		return ai->m_next;
	}

	AppI* getFirstApp()
	{
		return s_apps;
	}

	uint32_t getNumApps()
	{
		return s_numApps;
	}

	const float getDeltaTime()
	{
		return s_deltaTime;
	}

	void appInit()
	{
		s_timeOffset = bx::getHPCounter();
	}

	bool appUpdate()
	{
		int64_t now = bx::getHPCounter();
		static int64_t last = now;
		const int64_t frameTime = now - last;
		last = now;
		const double freq = double(bx::getHPFrequency());
		const float time = (float)((now - s_timeOffset) / double(bx::getHPFrequency()));
		s_deltaTime = float(frameTime / freq);

		//
		if (s_ctx->m_physicsCtx != NULL)
		{
			s_ctx->m_physicsCtx->simulate(s_deltaTime);
		}

		return true;
	}

	int runApp(AppI* _app, int _argc, const char* const* _argv)
	{
		setWindowTitle({ 0 }, _app->getName());
		setWindowSize({ 0 }, s_width, s_height);

		appInit();
		_app->init(_argc, _argv, s_width, s_height);
		max::frame();

#if BX_PLATFORM_EMSCRIPTEN
		s_app = _app;
		emscripten_set_main_loop(&updateApp, -1, 1);
#else
		while (appUpdate() && _app->update())
		{
			if (0 != bx::strLen(s_restartArgs))
			{
				break;
			}
		}
#endif // BX_PLATFORM_EMSCRIPTEN

		return _app->shutdown();
	}

	static int32_t sortApp(const void* _lhs, const void* _rhs)
	{
		const AppI* lhs = *(const AppI**)_lhs;
		const AppI* rhs = *(const AppI**)_rhs;

		return bx::strCmpI(lhs->getName(), rhs->getName());
	}

	static void sortApps()
	{
		if (2 > s_numApps)
		{
			return;
		}

		AppI** apps = (AppI**)bx::alloc(getDefaultAllocator(), s_numApps * sizeof(AppI*));

		uint32_t ii = 0;
		for (AppI* app = getFirstApp(); NULL != app; app = app->getNext())
		{
			apps[ii++] = app;
		}
		bx::quickSort(apps, s_numApps, sizeof(AppI*), sortApp);

		s_apps = apps[0];
		for (ii = 1; ii < s_numApps; ++ii)
		{
			AppI* app = apps[ii - 1];

			AppInternal* ai = bx::addressOf<AppInternal>(app, s_offset);
			ai->m_next = apps[ii];
		}

		{
			AppInternal* ai = bx::addressOf<AppInternal>(apps[s_numApps - 1], s_offset);
			ai->m_next = NULL;
		}

		bx::free(getDefaultAllocator(), apps);
	}

	int main(int _argc, const char* const* _argv)
	{
		bx::FilePath fp(_argv[0]);
		char title[bx::kMaxFilePath];
		bx::strCopy(title, BX_COUNTOF(title), fp.getBaseName());

		setWindowTitle({ 0 }, title);
		setWindowSize({ 0 }, MAX_DEFAULT_WIDTH, MAX_DEFAULT_HEIGHT);

		sortApps();

		const char* find = "";
		if (1 < _argc)
		{
			find = _argv[_argc - 1];
		}

	restart:
		AppI* selected = NULL;

		for (AppI* app = getFirstApp(); NULL != app; app = app->getNext())
		{
			if (NULL == selected
				&& !bx::strFindI(app->getName(), find).isEmpty())
			{
				selected = app;
			}
#if 0
			DBG("%c %s, %s"
				, app == selected ? '>' : ' '
				, app->getName()
				, app->getDescription()
			);
#endif // 0
		}

		int32_t result = bx::kExitSuccess;
		s_restartArgs[0] = '\0';
		if (0 == s_numApps)
		{
			result = ::_main_(_argc, (char**)_argv);
		}
		else
		{
			result = runApp(getCurrentApp(selected), _argc, _argv);
		}

		if (0 != bx::strLen(s_restartArgs))
		{
			find = s_restartArgs;
			goto restart;
		}

		return result;
	}

	WindowState s_window[MAX_CONFIG_MAX_WINDOWS];

	bool processEvents(uint32_t& _width, uint32_t& _height, uint32_t& _debug, uint32_t& _reset, MouseState* _mouse)
	{
		bool needReset = s_reset != _reset;

		s_debug = _debug;
		s_reset = _reset;

		WindowHandle handle = { UINT16_MAX };

		bool mouseLock = inputIsMouseLocked();

		const Event* ev;
		do
		{
			struct SE { const Event* m_ev; SE() : m_ev(poll()) {} ~SE() { if (NULL != m_ev) { release(m_ev); } } } scopeEvent;
			ev = scopeEvent.m_ev;

			if (NULL != ev)
			{
				switch (ev->m_type)
				{
				case Event::Axis:
				{
					const AxisEvent* axis = static_cast<const AxisEvent*>(ev);
					inputSetGamepadAxis(axis->m_gamepad, axis->m_axis, axis->m_value);
				}
				break;

				case Event::Char:
				{
					const CharEvent* chev = static_cast<const CharEvent*>(ev);
					inputChar(chev->m_len, chev->m_char);
				}
				break;

				case Event::Exit:
					return true;

				case Event::Gamepad:
				{
					// const GamepadEvent* gev = static_cast<const GamepadEvent*>(ev);
					// BX_TRACE("gamepad %d, %d", gev->m_gamepad.idx, gev->m_connected);
				}
				break;

				case Event::Mouse:
				{
					const MouseEvent* mouse = static_cast<const MouseEvent*>(ev);
					handle = mouse->m_handle;

					inputSetMousePos(mouse->m_mx, mouse->m_my, mouse->m_mz);
					if (!mouse->m_move)
					{
						inputSetMouseButtonState(mouse->m_button, mouse->m_down);
					}

					if (NULL != _mouse
						&& !mouseLock)
					{
						_mouse->m_mx = mouse->m_mx;
						_mouse->m_my = mouse->m_my;
						_mouse->m_mz = mouse->m_mz;
						if (!mouse->m_move)
						{
							_mouse->m_buttons[mouse->m_button] = mouse->m_down;
						}
					}
				}
				break;

				case Event::Key:
				{
					const KeyEvent* key = static_cast<const KeyEvent*>(ev);
					handle = key->m_handle;

					inputSetKeyState(key->m_key, key->m_modifiers, key->m_down);
				}
				break;

				case Event::Size:
				{
					const SizeEvent* size = static_cast<const SizeEvent*>(ev);
					WindowState& win = s_window[0];
					win.m_handle = size->m_handle;
					win.m_width = size->m_width;
					win.m_height = size->m_height;

					handle = size->m_handle;
					_width = size->m_width;
					_height = size->m_height;

					needReset = true;
				}
				break;

				case Event::Window:
					break;

				case Event::Suspend:
					break;

				case Event::DropFile:
				{
					const DropFileEvent* drop = static_cast<const DropFileEvent*>(ev);
					BX_TRACE("%s", drop->m_filePath.getCPtr());
				}
				break;

				default:
					break;
				}
			}

			inputProcess();

		} while (NULL != ev);

		needReset |= _reset != s_reset;

		if (handle.idx == 0
			&& needReset)
		{
			_reset = s_reset;
			max::reset(_width, _height, _reset);
			inputSetMouseResolution(uint16_t(_width), uint16_t(_height));
		}

		_debug = s_debug;

		s_width = _width;
		s_height = _height;

		return s_exit;
	}

	bool processWindowEvents(WindowState& _state, uint32_t& _debug, uint32_t& _reset)
	{
		bool needReset = s_reset != _reset;

		s_debug = _debug;
		s_reset = _reset;

		WindowHandle handle = { UINT16_MAX };

		bool mouseLock = inputIsMouseLocked();
		bool clearDropFile = true;

		const Event* ev;
		do
		{
			struct SE
			{
				SE(WindowHandle _handle)
					: m_ev(poll(_handle))
				{
				}

				~SE()
				{
					if (NULL != m_ev)
					{
						release(m_ev);
					}
				}

				const Event* m_ev;

			} scopeEvent(handle);
			ev = scopeEvent.m_ev;

			if (NULL != ev)
			{
				handle = ev->m_handle;
				WindowState& win = s_window[handle.idx];

				switch (ev->m_type)
				{
				case Event::Axis:
				{
					const AxisEvent* axis = static_cast<const AxisEvent*>(ev);
					inputSetGamepadAxis(axis->m_gamepad, axis->m_axis, axis->m_value);
				}
				break;

				case Event::Char:
				{
					const CharEvent* chev = static_cast<const CharEvent*>(ev);
					win.m_handle = chev->m_handle;
					inputChar(chev->m_len, chev->m_char);
				}
				break;

				case Event::Exit:
					return true;

				case Event::Gamepad:
				{
					const GamepadEvent* gev = static_cast<const GamepadEvent*>(ev);
					BX_TRACE("gamepad %d, %d", gev->m_gamepad.idx, gev->m_connected);
				}
				break;

				case Event::Mouse:
				{
					const MouseEvent* mouse = static_cast<const MouseEvent*>(ev);
					win.m_handle = mouse->m_handle;

					if (mouse->m_move)
					{
						inputSetMousePos(mouse->m_mx, mouse->m_my, mouse->m_mz);
					}
					else
					{
						inputSetMouseButtonState(mouse->m_button, mouse->m_down);
					}

					if (!mouseLock)
					{
						if (mouse->m_move)
						{
							win.m_mouse.m_mx = mouse->m_mx;
							win.m_mouse.m_my = mouse->m_my;
							win.m_mouse.m_mz = mouse->m_mz;
						}
						else
						{
							win.m_mouse.m_buttons[mouse->m_button] = mouse->m_down;
						}
					}
				}
				break;

				case Event::Key:
				{
					const KeyEvent* key = static_cast<const KeyEvent*>(ev);
					win.m_handle = key->m_handle;

					inputSetKeyState(key->m_key, key->m_modifiers, key->m_down);
				}
				break;

				case Event::Size:
				{
					const SizeEvent* size = static_cast<const SizeEvent*>(ev);
					win.m_handle = size->m_handle;
					win.m_width = size->m_width;
					win.m_height = size->m_height;

					needReset = win.m_handle.idx == 0 ? true : needReset;
				}
				break;

				case Event::Window:
				{
					const WindowEvent* window = static_cast<const WindowEvent*>(ev);
					win.m_handle = window->m_handle;
					win.m_nwh = window->m_nwh;
					ev = NULL;
				}
				break;

				case Event::Suspend:
					break;

				case Event::DropFile:
				{
					const DropFileEvent* drop = static_cast<const DropFileEvent*>(ev);
					win.m_dropFile = drop->m_filePath.getCPtr();
					clearDropFile = false;
				}
				break;

				default:
					break;
				}
			}

			inputProcess();

		} while (NULL != ev);

		if (isValid(handle))
		{
			WindowState& win = s_window[handle.idx];
			if (clearDropFile)
			{
				win.m_dropFile = "";
			}

			_state = win;

			if (handle.idx == 0)
			{
				inputSetMouseResolution(uint16_t(win.m_width), uint16_t(win.m_height));
			}
		}

		needReset |= _reset != s_reset;

		if (needReset)
		{
			_reset = s_reset;
			max::reset(s_window[0].m_width, s_window[0].m_height, _reset);
			inputSetMouseResolution(uint16_t(s_window[0].m_width), uint16_t(s_window[0].m_height));
		}

		_debug = s_debug;

		return s_exit;
	}

	static Handle::TypeName s_typeName[] =
	{
		{ "DIB",  "DynamicIndexBuffer"  },
		{ "DVB",  "DynamicVertexBuffer" },
		{ "FB",   "FrameBuffer"         },
		{ "IB",   "IndexBuffer"         },
		{ "IndB", "IndirectBuffer"      },
		{ "OQ",   "OcclusionQuery"      },
		{ "P",    "Program"             },
		{ "S",    "Shader"              },
		{ "T",    "Texture"             },
		{ "U",    "Uniform"             },
		{ "VB",   "VertexBuffer"        },
		{ "VL",   "VertexLayout"        },
		{ "?",    "?"                   },
	};
	BX_STATIC_ASSERT(BX_COUNTOF(s_typeName) == Handle::Count+1, "");

	const Handle::TypeName& Handle::getTypeName(Handle::Enum _enum)
	{
		BX_ASSERT(_enum < Handle::Count, "Invalid Handle::Enum %d!", _enum);
		return s_typeName[bx::min(_enum, Handle::Count)];
	}

	static const char* s_keyName[] =
	{
		"None",
		"Esc",
		"Return",
		"Tab",
		"Space",
		"Backspace",
		"Up",
		"Down",
		"Left",
		"Right",
		"Insert",
		"Delete",
		"Home",
		"End",
		"PageUp",
		"PageDown",
		"Print",
		"Plus",
		"Minus",
		"LeftBracket",
		"RightBracket",
		"Semicolon",
		"Quote",
		"Comma",
		"Period",
		"Slash",
		"Backslash",
		"Tilde",
		"F1",
		"F2",
		"F3",
		"F4",
		"F5",
		"F6",
		"F7",
		"F8",
		"F9",
		"F10",
		"F11",
		"F12",
		"NumPad0",
		"NumPad1",
		"NumPad2",
		"NumPad3",
		"NumPad4",
		"NumPad5",
		"NumPad6",
		"NumPad7",
		"NumPad8",
		"NumPad9",
		"Key0",
		"Key1",
		"Key2",
		"Key3",
		"Key4",
		"Key5",
		"Key6",
		"Key7",
		"Key8",
		"Key9",
		"KeyA",
		"KeyB",
		"KeyC",
		"KeyD",
		"KeyE",
		"KeyF",
		"KeyG",
		"KeyH",
		"KeyI",
		"KeyJ",
		"KeyK",
		"KeyL",
		"KeyM",
		"KeyN",
		"KeyO",
		"KeyP",
		"KeyQ",
		"KeyR",
		"KeyS",
		"KeyT",
		"KeyU",
		"KeyV",
		"KeyW",
		"KeyX",
		"KeyY",
		"KeyZ",
		"GamepadA",
		"GamepadB",
		"GamepadX",
		"GamepadY",
		"GamepadThumbL",
		"GamepadThumbR",
		"GamepadShoulderL",
		"GamepadShoulderR",
		"GamepadUp",
		"GamepadDown",
		"GamepadLeft",
		"GamepadRight",
		"GamepadBack",
		"GamepadStart",
		"GamepadGuide",
	};
	BX_STATIC_ASSERT(Key::Count == BX_COUNTOF(s_keyName));

	const char* getKeyName(Key::Enum _key)
	{
		BX_ASSERT(_key < Key::Count, "Invalid key %d.", _key);
		return s_keyName[_key];
	}

	void AllocatorStub::checkLeaks()
	{
#if MAX_CONFIG_MEMORY_TRACKING
		// BK - CallbackStub will be deleted after printing this info, so there is always one
		// leak if CallbackStub is used.
		BX_WARN(uint32_t(NULL != s_callbackStub ? 1 : 0) == m_numBlocks
			, "\n\n"
			  "\n########################################################"
			  "\n"
			  "\nMEMORY LEAK: Number of leaked blocks %d (Max blocks: %d)"
			  "\n"
			  "\n########################################################"
			  "\n\n"
			, m_numBlocks
			, m_maxBlocks
			);
#endif // MAX_CONFIG_MEMORY_TRACKING
	}

	void setPlatformData(const PlatformData& _data)
	{
		if (NULL != s_ctx)
		{
			MAX_FATAL(true
				&& g_platformData.ndt     == _data.ndt
				&& g_platformData.context == _data.context
				, Fatal::UnableToInitialize
				, "Only backbuffer pointer and native window handle can be changed after initialization!"
				);
		}
		bx::memCopy(&g_platformData, &_data, sizeof(PlatformData) );
		g_platformDataChangedSinceReset = true;
	}

	const InternalData* getInternalData()
	{
		return &g_internalData;
	}

	uintptr_t overrideInternal(TextureHandle _handle, uintptr_t _ptr)
	{
		MAX_CHECK_RENDER_THREAD();
		RendererContextI* rci = s_ctx->m_renderCtx;
		if (0 == rci->getInternal(_handle) )
		{
			return 0;
		}

		rci->overrideInternal(_handle, _ptr);

		return rci->getInternal(_handle);
	}

	uintptr_t overrideInternal(TextureHandle _handle, uint16_t _width, uint16_t _height, uint8_t _numMips, TextureFormat::Enum _format, uint64_t _flags)
	{
		MAX_CHECK_RENDER_THREAD();
		RendererContextI* rci = s_ctx->m_renderCtx;
		if (0 == rci->getInternal(_handle) )
		{
			return 0;
		}

		uint32_t size = sizeof(uint32_t) + sizeof(TextureCreate);
		Memory* mem = const_cast<Memory*>(alloc(size) );

		bx::StaticMemoryBlockWriter writer(mem->data, mem->size);
		uint32_t magic = MAX_CHUNK_MAGIC_TEX;
		bx::write(&writer, magic, bx::ErrorAssert{});

		TextureCreate tc;
		tc.m_width     = _width;
		tc.m_height    = _height;
		tc.m_depth     = 0;
		tc.m_numLayers = 1;
		tc.m_numMips   = bx::max<uint8_t>(1, _numMips);
		tc.m_format    = _format;
		tc.m_cubeMap   = false;
		tc.m_mem       = NULL;
		bx::write(&writer, tc, bx::ErrorAssert{});

		rci->destroyTexture(_handle);
		rci->createTexture(_handle, mem, _flags, 0);

		release(mem);

		return rci->getInternal(_handle);
	}

	void setGraphicsDebuggerPresent(bool _present)
	{
		BX_TRACE("Graphics debugger is %spresent.", _present ? "" : "not ");
		s_graphicsDebuggerPresent = _present;
	}

	bool isGraphicsDebuggerPresent()
	{
		return s_graphicsDebuggerPresent;
	}

	void fatal(const char* _filePath, uint16_t _line, Fatal::Enum _code, const char* _format, ...)
	{
		va_list argList;
		va_start(argList, _format);

		char temp[8192];
		char* out = temp;
		int32_t len = bx::vsnprintf(out, sizeof(temp), _format, argList);
		if ( (int32_t)sizeof(temp) < len)
		{
			out = (char*)alloca(len+1);
			len = bx::vsnprintf(out, len, _format, argList);
		}
		out[len] = '\0';

		if (BX_UNLIKELY(NULL == g_callback) )
		{
			bx::debugPrintf("%s(%d): MAX FATAL 0x%08x: %s", _filePath, _line, _code, out);
			abort();
		}
		else
		{
			g_callback->fatal(_filePath, _line, _code, out);
		}

		va_end(argList);
	}

	void trace(const char* _filePath, uint16_t _line, const char* _format, ...)
	{
		va_list argList;
		va_start(argList, _format);

		if (BX_UNLIKELY(NULL == g_callback) )
		{
			bx::debugPrintfVargs(_format, argList);
		}
		else
		{
			g_callback->traceVargs(_filePath, _line, _format, argList);
		}

		va_end(argList);
	}

#include "vs_debugfont.bin.h"
#include "fs_debugfont.bin.h"
#include "vs_clear.bin.h"
#include "fs_clear0.bin.h"
#include "fs_clear1.bin.h"
#include "fs_clear2.bin.h"
#include "fs_clear3.bin.h"
#include "fs_clear4.bin.h"
#include "fs_clear5.bin.h"
#include "fs_clear6.bin.h"
#include "fs_clear7.bin.h"
#include "vs_debugdraw_lines.bin.h"
#include "fs_debugdraw_lines.bin.h"
#include "vs_debugdraw_lines_stipple.bin.h"
#include "fs_debugdraw_lines_stipple.bin.h"
#include "vs_debugdraw_fill.bin.h"
#include "vs_debugdraw_fill_mesh.bin.h"
#include "fs_debugdraw_fill.bin.h"
#include "vs_debugdraw_fill_lit.bin.h"
#include "vs_debugdraw_fill_lit_mesh.bin.h"
#include "fs_debugdraw_fill_lit.bin.h"
#include "vs_debugdraw_fill_texture.bin.h"
#include "fs_debugdraw_fill_texture.bin.h"

	static const EmbeddedShader s_embeddedShaders[] =
	{
		MAX_EMBEDDED_SHADER(vs_debugfont),
		MAX_EMBEDDED_SHADER(fs_debugfont),
		MAX_EMBEDDED_SHADER(vs_clear),
		MAX_EMBEDDED_SHADER(fs_clear0),
		MAX_EMBEDDED_SHADER(fs_clear1),
		MAX_EMBEDDED_SHADER(fs_clear2),
		MAX_EMBEDDED_SHADER(fs_clear3),
		MAX_EMBEDDED_SHADER(fs_clear4),
		MAX_EMBEDDED_SHADER(fs_clear5),
		MAX_EMBEDDED_SHADER(fs_clear6),
		MAX_EMBEDDED_SHADER(fs_clear7),
		MAX_EMBEDDED_SHADER(vs_debugdraw_lines),
		MAX_EMBEDDED_SHADER(fs_debugdraw_lines),
		MAX_EMBEDDED_SHADER(vs_debugdraw_lines_stipple),
		MAX_EMBEDDED_SHADER(fs_debugdraw_lines_stipple),
		MAX_EMBEDDED_SHADER(vs_debugdraw_fill),
		MAX_EMBEDDED_SHADER(vs_debugdraw_fill_mesh),
		MAX_EMBEDDED_SHADER(fs_debugdraw_fill),
		MAX_EMBEDDED_SHADER(vs_debugdraw_fill_lit),
		MAX_EMBEDDED_SHADER(vs_debugdraw_fill_lit_mesh),
		MAX_EMBEDDED_SHADER(fs_debugdraw_fill_lit),
		MAX_EMBEDDED_SHADER(vs_debugdraw_fill_texture),
		MAX_EMBEDDED_SHADER(fs_debugdraw_fill_texture),

		MAX_EMBEDDED_SHADER_END()
	};

	ShaderHandle createEmbeddedShader(const EmbeddedShader* _es, RendererType::Enum _type, const char* _name)
	{
		for (const EmbeddedShader* es = _es; NULL != es->name; ++es)
		{
			if (0 == bx::strCmp(_name, es->name) )
			{
				for (const EmbeddedShader::Data* esd = es->data; RendererType::Count != esd->type; ++esd)
				{
					if (_type == esd->type
					&&  1 < esd->size)
					{
						ShaderHandle handle = createShader(makeRef(esd->data, esd->size) );
						if (isValid(handle) )
						{
							setName(handle, _name);
						}

						return handle;
					}
				}
			}
		}

		ShaderHandle handle = MAX_INVALID_HANDLE;
		return handle;
	}

	void dump(const VertexLayout& _layout)
	{
		if (BX_ENABLED(MAX_CONFIG_DEBUG) )
		{
			BX_TRACE("VertexLayout %08x (%08x), stride %d"
				, _layout.m_hash
				, bx::hash<bx::HashMurmur2A>(_layout.m_attributes)
				, _layout.m_stride
				);

			for (uint32_t attr = 0; attr < Attrib::Count; ++attr)
			{
				if (UINT16_MAX != _layout.m_attributes[attr])
				{
					uint8_t num;
					AttribType::Enum type;
					bool normalized;
					bool asInt;
					_layout.decode(Attrib::Enum(attr), num, type, normalized, asInt);

					BX_TRACE("\tattr %2d: %-20s num %d, type %d, norm [%c], asint [%c], offset %2d"
						, attr
						, getAttribName(Attrib::Enum(attr) )
						, num
						, type
						, normalized ? 'x' : ' '
						, asInt      ? 'x' : ' '
						, _layout.m_offset[attr]
						);
				}
			}
		}
	}

#include "charset.h"

	void charsetFillTexture(const uint8_t* _charset, uint8_t* _rgba, uint32_t _height, uint32_t _pitch, uint32_t _bpp)
	{
		for (uint32_t ii = 0; ii < 256; ++ii)
		{
			uint8_t* pix = &_rgba[ii*8*_bpp];
			for (uint32_t yy = 0; yy < _height; ++yy)
			{
				for (uint32_t xx = 0; xx < 8; ++xx)
				{
					uint8_t bit = 1<<(7-xx);
					bx::memSet(&pix[xx*_bpp], _charset[ii*_height+yy]&bit ? 255 : 0, _bpp);
				}

				pix += _pitch;
			}
		}
	}

	static uint8_t parseAttrTo(char*& _ptr, char _to, uint8_t _default)
	{
		const bx::StringView str = bx::strFind(_ptr, _to);
		if (!str.isEmpty()
		&&  3 > str.getPtr()-_ptr)
		{
			char tmp[4];

			int32_t len = int32_t(str.getPtr()-_ptr);
			bx::strCopy(tmp, sizeof(tmp), _ptr, len);

			uint32_t attr;
			bx::fromString(&attr, tmp);

			_ptr += len+1;
			return uint8_t(attr);
		}

		return _default;
	}

	static uint8_t parseAttr(char*& _ptr, uint8_t _default)
	{
		char* ptr = _ptr;
		if (*ptr++ != '[')
		{
			return _default;
		}

		if (0 == bx::strCmp(ptr, "0m", 2) )
		{
			_ptr = ptr + 2;
			return _default;
		}

		uint8_t fg = parseAttrTo(ptr, ';', _default & 0xf);
		uint8_t bg = parseAttrTo(ptr, 'm', _default >> 4);

		uint8_t attr = (bg<<4) | fg;
		_ptr = ptr;
		return attr;
	}

	void TextVideoMem::printfVargs(uint16_t _x, uint16_t _y, uint8_t _attr, const char* _format, va_list _argList)
	{
		if (_x < m_width && _y < m_height)
		{
			va_list argListCopy;
			va_copy(argListCopy, _argList);
			uint32_t num = bx::vsnprintf(NULL, 0, _format, argListCopy) + 1;
			char* temp = (char*)alloca(num);
			va_copy(argListCopy, _argList);
			num = bx::vsnprintf(temp, num, _format, argListCopy);

			uint8_t attr = _attr;
			MemSlot* mem = &m_mem[_y*m_width+_x];
			for (uint32_t ii = 0, xx = _x; ii < num && xx < m_width; ++ii)
			{
				char ch = temp[ii];
				if (BX_UNLIKELY(ch == '\x1b') )
				{
					char* ptr = &temp[ii+1];
					attr = parseAttr(ptr, _attr);
					ii += uint32_t(ptr - &temp[ii+1]);
				}
				else
				{
					mem->character = ch;
					mem->attribute = attr;
					++mem;
					++xx;
				}
			}
		}
	}

	static const uint32_t numCharsPerBatch = 1024;
	static const uint32_t numBatchVertices = numCharsPerBatch*4;
	static const uint32_t numBatchIndices  = numCharsPerBatch*6;

	void TextVideoMemBlitter::init(uint8_t scale)
	{
		MAX_CHECK_API_THREAD();
		m_layout
			.begin()
			.add(Attrib::Position,  3, AttribType::Float)
			.add(Attrib::Color0,    4, AttribType::Uint8, true)
			.add(Attrib::Color1,    4, AttribType::Uint8, true)
			.add(Attrib::TexCoord0, 2, AttribType::Float)
			.end();

		uint16_t width  = 2048;
		uint16_t height = 24;
		uint8_t  bpp    = 1;
		uint32_t pitch  = width*bpp;

		const Memory* mem;

		mem = alloc(pitch*height);
		uint8_t* rgba = mem->data;
		charsetFillTexture(vga8x8, rgba, 8, pitch, bpp);
		charsetFillTexture(vga8x16, &rgba[8*pitch], 16, pitch, bpp);
		m_texture = createTexture2D(width, height, false, 1, TextureFormat::R8
			, MAX_SAMPLER_MIN_POINT
			| MAX_SAMPLER_MAG_POINT
			| MAX_SAMPLER_MIP_POINT
			| MAX_SAMPLER_U_CLAMP
			| MAX_SAMPLER_V_CLAMP
			, mem
			);

		ShaderHandle vsh = createEmbeddedShader(s_embeddedShaders, g_caps.rendererType, "vs_debugfont");
		ShaderHandle fsh = createEmbeddedShader(s_embeddedShaders, g_caps.rendererType, "fs_debugfont");

		BX_ASSERT(isValid(vsh) && isValid(fsh), "Failed to create embedded blit shaders");

		m_program = createProgram(vsh, fsh, true);

		m_vb = s_ctx->createTransientVertexBuffer(numBatchVertices*m_layout.m_stride, &m_layout);
		m_ib = s_ctx->createTransientIndexBuffer(numBatchIndices*2);
		m_scale = bx::max<uint8_t>(scale, 1);
	}

	void TextVideoMemBlitter::shutdown()
	{
		MAX_CHECK_API_THREAD();

		if (isValid(m_program) )
		{
			destroy(m_program);
		}

		destroy(m_texture);
		s_ctx->destroyTransientVertexBuffer(m_vb);
		s_ctx->destroyTransientIndexBuffer(m_ib);
	}

	static const uint32_t s_paletteSrgb[] =
	{
		0x0,        // Black
		0xffa46534, // Blue
		0xff069a4e, // Green
		0xff9a9806, // Cyan
		0xff0000cc, // Red
		0xff7b5075, // Magenta
		0xff00a0c4, // Brown
		0xffcfd7d3, // Light Gray
		0xff535755, // Dark Gray
		0xffcf9f72, // Light Blue
		0xff34e28a, // Light Green
		0xffe2e234, // Light Cyan
		0xff2929ef, // Light Red
		0xffa87fad, // Light Magenta
		0xff4fe9fc, // Yellow
		0xffeceeee, // White
	};
	BX_STATIC_ASSERT(BX_COUNTOF(s_paletteSrgb) == 16);

	static const uint32_t s_paletteLinear[] =
	{
		0x0,        // Black
		0xff5e2108, // Blue
		0xff005213, // Green
		0xff525000, // Cyan
		0xff000099, // Red
		0xff32142d, // Magenta
		0xff00598c, // Brown
		0xff9fada6, // Light Gray
		0xff161817, // Dark Gray
		0xff9f582a, // Light Blue
		0xff08c140, // Light Green
		0xffc1c108, // Light Cyan
		0xff0505dc, // Light Red
		0xff63366a, // Light Magenta
		0xff13cff8, // Yellow
		0xffd5dada  // White
	};
	BX_STATIC_ASSERT(BX_COUNTOF(s_paletteLinear) == 16);

	void blit(RendererContextI* _renderCtx, TextVideoMemBlitter& _blitter, const TextVideoMem& _mem)
	{
		struct Vertex
		{
			float m_x;
			float m_y;
			float m_z;
			uint32_t m_fg;
			uint32_t m_bg;
			float m_u;
			float m_v;
		};

		uint32_t yy = 0;
		uint32_t xx = 0;

		const float texelWidth  = 1.0f/2048.0f;
		const float texelHeight = 1.0f/24.0f;
		const float utop        = (_mem.m_small ? 0.0f :  8.0f)*texelHeight;
		const float ubottom     = (_mem.m_small ? 8.0f : 24.0f)*texelHeight;
		const float fontHeight  = (_mem.m_small ? 8.0f : 16.0f)*_blitter.m_scale;
		const float fontWidth   = 8.0f * _blitter.m_scale;

		_renderCtx->blitSetup(_blitter);

		const uint32_t* palette = 0 != (s_ctx->m_init.resolution.reset & MAX_RESET_SRGB_BACKBUFFER)
			? s_paletteLinear
			: s_paletteSrgb
			;

		for (;yy < _mem.m_height;)
		{
			Vertex* vertex = (Vertex*)_blitter.m_vb->data;
			uint16_t* indices = (uint16_t*)_blitter.m_ib->data;
			uint32_t startVertex = 0;
			uint32_t numIndices = 0;

			for (; yy < _mem.m_height && numIndices < numBatchIndices; ++yy)
			{
				xx = xx < _mem.m_width ? xx : 0;
				const TextVideoMem::MemSlot* line = &_mem.m_mem[yy*_mem.m_width+xx];

				for (; xx < _mem.m_width && numIndices < numBatchIndices; ++xx)
				{
					uint32_t ch = line->character;
					const uint8_t attr = line->attribute;

					if (ch > 0xff)
					{
						ch = 0;
					}

					if (0 != (ch|attr)
					&& (' ' != ch || 0 != (attr&0xf0) ) )
					{
						const uint32_t fg = palette[attr&0xf];
						const uint32_t bg = palette[(attr>>4)&0xf];

						Vertex vert[4] =
						{
							{ (xx  )*fontWidth, (yy  )*fontHeight, 0.0f, fg, bg, (ch  )*8.0f*texelWidth, utop },
							{ (xx+1)*fontWidth, (yy  )*fontHeight, 0.0f, fg, bg, (ch+1)*8.0f*texelWidth, utop },
							{ (xx+1)*fontWidth, (yy+1)*fontHeight, 0.0f, fg, bg, (ch+1)*8.0f*texelWidth, ubottom },
							{ (xx  )*fontWidth, (yy+1)*fontHeight, 0.0f, fg, bg, (ch  )*8.0f*texelWidth, ubottom },
						};

						bx::memCopy(vertex, vert, sizeof(vert) );
						vertex += 4;

						indices[0] = uint16_t(startVertex+0);
						indices[1] = uint16_t(startVertex+1);
						indices[2] = uint16_t(startVertex+2);
						indices[3] = uint16_t(startVertex+2);
						indices[4] = uint16_t(startVertex+3);
						indices[5] = uint16_t(startVertex+0);

						startVertex += 4;
						indices += 6;

						numIndices += 6;
					}

					line++;
				}

				if (numIndices >= numBatchIndices)
				{
					break;
				}
			}

			_renderCtx->blitRender(_blitter, numIndices);
		}
	}

	void ClearQuad::init()
	{
		MAX_CHECK_API_THREAD();

		if (RendererType::Noop != g_caps.rendererType)
		{
			m_layout
				.begin()
				.add(Attrib::Position, 2, AttribType::Float)
				.end();

			ShaderHandle vsh = createEmbeddedShader(s_embeddedShaders, g_caps.rendererType, "vs_clear");
			BX_ASSERT(isValid(vsh), "Failed to create clear quad embedded vertex shader \"vs_clear\"");

			for (uint32_t ii = 0, num = g_caps.limits.maxFBAttachments; ii < num; ++ii)
			{
				char name[32];
				bx::snprintf(name, BX_COUNTOF(name), "fs_clear%d", ii);
				ShaderHandle fsh = createEmbeddedShader(s_embeddedShaders, g_caps.rendererType, name);
				BX_ASSERT(isValid(fsh), "Failed to create clear quad embedded fragment shader \"%s\"", name);

				m_program[ii] = createProgram(vsh, fsh);
				BX_ASSERT(isValid(m_program[ii]), "Failed to create clear quad program.");
				destroy(fsh);
			}

			destroy(vsh);

			struct Vertex
			{
				float m_x;
				float m_y;
			};

			const uint16_t stride = m_layout.m_stride;
			const max::Memory* mem = max::alloc(4 * stride);
			Vertex* vertex = (Vertex*)mem->data;
			BX_ASSERT(stride == sizeof(Vertex), "Stride/Vertex mismatch (stride %d, sizeof(Vertex) %d)", stride, sizeof(Vertex));

			vertex->m_x = -1.0f;
			vertex->m_y = -1.0f;
			vertex++;
			vertex->m_x = 1.0f;
			vertex->m_y = -1.0f;
			vertex++;
			vertex->m_x = -1.0f;
			vertex->m_y = 1.0f;
			vertex++;
			vertex->m_x = 1.0f;
			vertex->m_y = 1.0f;

			m_vb = s_ctx->createVertexBuffer(mem, m_layout, 0);
		}
	}

	void ClearQuad::shutdown()
	{
		MAX_CHECK_API_THREAD();

		if (RendererType::Noop != g_caps.rendererType)
		{
			for (uint32_t ii = 0, num = g_caps.limits.maxFBAttachments; ii < num; ++ii)
			{
				if (isValid(m_program[ii]) )
				{
					destroy(m_program[ii]);
					m_program[ii].idx = kInvalidHandle;
				}
			}

			s_ctx->destroyVertexBuffer(m_vb);
		}
	}

	const char* s_uniformTypeName[] =
	{
		"sampler1",
		NULL,
		"vec4",
		"mat3",
		"mat4",
	};
	BX_STATIC_ASSERT(UniformType::Count == BX_COUNTOF(s_uniformTypeName) );

	const char* getUniformTypeName(UniformType::Enum _enum)
	{
		BX_ASSERT(_enum < UniformType::Count, "%d < UniformType::Count %d", _enum, UniformType::Count);
		return s_uniformTypeName[_enum];
	}

	UniformType::Enum nameToUniformTypeEnum(const char* _name)
	{
		for (uint32_t ii = 0; ii < UniformType::Count; ++ii)
		{
			if (NULL != s_uniformTypeName[ii]
			&&  0 == bx::strCmp(_name, s_uniformTypeName[ii]) )
			{
				return UniformType::Enum(ii);
			}
		}

		return UniformType::Count;
	}

	static const char* s_predefinedName[PredefinedUniform::Count] =
	{
		"u_viewRect",
		"u_viewTexel",
		"u_view",
		"u_invView",
		"u_proj",
		"u_invProj",
		"u_viewProj",
		"u_invViewProj",
		"u_model",
		"u_modelView",
		"u_modelViewProj",
		"u_alphaRef4",
	};

	const char* getPredefinedUniformName(PredefinedUniform::Enum _enum)
	{
		return s_predefinedName[_enum];
	}

	PredefinedUniform::Enum nameToPredefinedUniformEnum(const bx::StringView& _name)
	{
		for (uint32_t ii = 0; ii < PredefinedUniform::Count; ++ii)
		{
			if (0 == bx::strCmp(_name, s_predefinedName[ii]) )
			{
				return PredefinedUniform::Enum(ii);
			}
		}

		return PredefinedUniform::Count;
	}

	void srtToMatrix4_x1(void* _dst, const void* _src)
	{
		      Matrix4* mtx = reinterpret_cast<  Matrix4*>(_dst);
		const     Srt* srt = reinterpret_cast<const Srt*>(_src);

		const float rx = srt->rotate[0];
		const float ry = srt->rotate[1];
		const float rz = srt->rotate[2];
		const float rw = srt->rotate[3];

		const float xx2 = 2.0f * rx * rx;
		const float yy2 = 2.0f * ry * ry;
		const float zz2 = 2.0f * rz * rz;
		const float yx2 = 2.0f * ry * rx;
		const float yz2 = 2.0f * ry * rz;
		const float yw2 = 2.0f * ry * rw;
		const float wz2 = 2.0f * rw * rz;
		const float wx2 = 2.0f * rw * rx;
		const float xz2 = 2.0f * rx * rz;

		const float sx = srt->scale[0];
		const float sy = srt->scale[1];
		const float sz = srt->scale[2];

		mtx->un.val[ 0] = (1.0f - yy2 - zz2)*sx;
		mtx->un.val[ 1] = (       yx2 + wz2)*sx;
		mtx->un.val[ 2] = (       xz2 - yw2)*sx;
		mtx->un.val[ 3] = 0.0f;

		mtx->un.val[ 4] = (       yx2 - wz2)*sy;
		mtx->un.val[ 5] = (1.0f - xx2 - zz2)*sy;
		mtx->un.val[ 6] = (       yz2 + wx2)*sy;
		mtx->un.val[ 7] = 0.0f;

		mtx->un.val[ 8] = (       xz2 + yw2)*sz;
		mtx->un.val[ 9] = (       yz2 - wx2)*sz;
		mtx->un.val[10] = (1.0f - xx2 - yy2)*sz;
		mtx->un.val[11] = 0.0f;

		const float tx = srt->translate[0];
		const float ty = srt->translate[1];
		const float tz = srt->translate[2];

		mtx->un.val[12] = tx;
		mtx->un.val[13] = ty;
		mtx->un.val[14] = tz;
		mtx->un.val[15] = 1.0f;
	}

	void transpose(void* _dst, uint32_t _dstStride, const void* _src, uint32_t _srcStride = sizeof(bx::simd128_t) )
	{
		      uint8_t* dst = reinterpret_cast<      uint8_t *>(_dst);
		const uint8_t* src = reinterpret_cast<const uint8_t *>(_src);

		using namespace bx;

		const simd128_t r0 = simd_ld<simd128_t>(src);
		src += _srcStride;

		const simd128_t r1 = simd_ld<simd128_t>(src);
		src += _srcStride;

		const simd128_t r2 = simd_ld<simd128_t>(src);
		src += _srcStride;

		const simd128_t r3 = simd_ld<simd128_t>(src);

		const simd128_t aibj = simd_shuf_xAyB(r0,   r2);   // aibj
		const simd128_t emfn = simd_shuf_xAyB(r1,   r3);   // emfn
		const simd128_t ckdl = simd_shuf_zCwD(r0,   r2);   // ckdl
		const simd128_t gohp = simd_shuf_zCwD(r1,   r3);   // gohp
		const simd128_t aeim = simd_shuf_xAyB(aibj, emfn); // aeim
		const simd128_t bfjn = simd_shuf_zCwD(aibj, emfn); // bfjn
		const simd128_t cgko = simd_shuf_xAyB(ckdl, gohp); // cgko
		const simd128_t dhlp = simd_shuf_zCwD(ckdl, gohp); // dhlp

		simd_st(dst, aeim);
		dst += _dstStride;

		simd_st(dst, bfjn);
		dst += _dstStride;

		simd_st(dst, cgko);
		dst += _dstStride;

		simd_st(dst, dhlp);
	}

	void srtToMatrix4_x4_Ref(void* _dst, const void* _src)
	{
		      uint8_t* dst = reinterpret_cast<      uint8_t*>(_dst);
		const uint8_t* src = reinterpret_cast<const uint8_t*>(_src);

		srtToMatrix4_x1(dst + 0*sizeof(Matrix4), src + 0*sizeof(Srt) );
		srtToMatrix4_x1(dst + 1*sizeof(Matrix4), src + 1*sizeof(Srt) );
		srtToMatrix4_x1(dst + 2*sizeof(Matrix4), src + 2*sizeof(Srt) );
		srtToMatrix4_x1(dst + 3*sizeof(Matrix4), src + 3*sizeof(Srt) );
	}

	void srtToMatrix4_x4_Simd(void* _dst, const void* _src)
	{
		using namespace bx;

		      simd128_t* dst = reinterpret_cast<      simd128_t*>(_dst);
		const simd128_t* src = reinterpret_cast<const simd128_t*>(_src);

		simd128_t rotate[4];
		simd128_t translate[4];
		simd128_t scale[4];

		transpose(rotate,    sizeof(simd128_t), src + 0, sizeof(Srt) );
		transpose(translate, sizeof(simd128_t), src + 1, sizeof(Srt) );
		transpose(scale,     sizeof(simd128_t), src + 2, sizeof(Srt) );

		const simd128_t rx    = simd_ld<simd128_t>(rotate + 0);
		const simd128_t ry    = simd_ld<simd128_t>(rotate + 1);
		const simd128_t rz    = simd_ld<simd128_t>(rotate + 2);
		const simd128_t rw    = simd_ld<simd128_t>(rotate + 3);

		const simd128_t tx    = simd_ld<simd128_t>(translate + 0);
		const simd128_t ty    = simd_ld<simd128_t>(translate + 1);
		const simd128_t tz    = simd_ld<simd128_t>(translate + 2);

		const simd128_t sx    = simd_ld<simd128_t>(scale + 0);
		const simd128_t sy    = simd_ld<simd128_t>(scale + 1);
		const simd128_t sz    = simd_ld<simd128_t>(scale + 2);

		const simd128_t zero  = simd_splat(0.0f);
		const simd128_t one   = simd_splat(1.0f);
		const simd128_t two   = simd_splat(2.0f);

		const simd128_t xx    = simd_mul(rx,    rx);
		const simd128_t xx2   = simd_mul(two,   xx);
		const simd128_t yy    = simd_mul(ry,    ry);
		const simd128_t yy2   = simd_mul(two,   yy);
		const simd128_t zz    = simd_mul(rz,    rz);
		const simd128_t zz2   = simd_mul(two,   zz);
		const simd128_t yx    = simd_mul(ry,    rx);
		const simd128_t yx2   = simd_mul(two,   yx);
		const simd128_t yz    = simd_mul(ry,    rz);
		const simd128_t yz2   = simd_mul(two,   yz);
		const simd128_t yw    = simd_mul(ry,    rw);
		const simd128_t yw2   = simd_mul(two,   yw);
		const simd128_t wz    = simd_mul(rw,    rz);
		const simd128_t wz2   = simd_mul(two,   wz);
		const simd128_t wx    = simd_mul(rw,    rx);
		const simd128_t wx2   = simd_mul(two,   wx);
		const simd128_t xz    = simd_mul(rx,    rz);
		const simd128_t xz2   = simd_mul(two,   xz);
		const simd128_t t0x   = simd_sub(one,   yy2);
		const simd128_t r0x   = simd_sub(t0x,   zz2);
		const simd128_t r0y   = simd_add(yx2,   wz2);
		const simd128_t r0z   = simd_sub(xz2,   yw2);
		const simd128_t r1x   = simd_sub(yx2,   wz2);
		const simd128_t omxx2 = simd_sub(one,   xx2);
		const simd128_t r1y   = simd_sub(omxx2, zz2);
		const simd128_t r1z   = simd_add(yz2,   wx2);
		const simd128_t r2x   = simd_add(xz2,   yw2);
		const simd128_t r2y   = simd_sub(yz2,   wx2);
		const simd128_t r2z   = simd_sub(omxx2, yy2);

		simd128_t tmp[4];
		tmp[0] = simd_mul(r0x, sx);
		tmp[1] = simd_mul(r0y, sx);
		tmp[2] = simd_mul(r0z, sx);
		tmp[3] = zero;
		transpose(dst + 0, sizeof(Matrix4), tmp);

		tmp[0] = simd_mul(r1x, sy);
		tmp[1] = simd_mul(r1y, sy);
		tmp[2] = simd_mul(r1z, sy);
		tmp[3] = zero;
		transpose(dst + 1, sizeof(Matrix4), tmp);

		tmp[0] = simd_mul(r2x, sz);
		tmp[1] = simd_mul(r2y, sz);
		tmp[2] = simd_mul(r2z, sz);
		tmp[3] = zero;
		transpose(dst + 2, sizeof(Matrix4), tmp);

		tmp[0] = tx;
		tmp[1] = ty;
		tmp[2] = tz;
		tmp[3] = one;
		transpose(dst + 3, sizeof(Matrix4), tmp);
	}

	void srtToMatrix4(void* _dst, const void* _src, uint32_t _num)
	{
		      uint8_t* dst = reinterpret_cast<      uint8_t*>(_dst);
		const uint8_t* src = reinterpret_cast<const uint8_t*>(_src);

		if (!bx::isAligned(src, 16) )
		{
			for (uint32_t ii = 0, num = _num / 4; ii < num; ++ii)
			{
				srtToMatrix4_x4_Ref(dst, src);
				src += 4*sizeof(Srt);
				dst += 4*sizeof(Matrix4);
			}
		}
		else
		{
			for (uint32_t ii = 0, num = _num / 4; ii < num; ++ii)
			{
				srtToMatrix4_x4_Simd(dst, src);
				src += 4*sizeof(Srt);
				dst += 4*sizeof(Matrix4);
			}
		}

		for (uint32_t ii = 0, num = _num & 3; ii < num; ++ii)
		{
			srtToMatrix4_x1(dst, src);
			src += sizeof(Srt);
			dst += sizeof(Matrix4);
		}
	}

	void DebugDrawShared::init()
	{
		DebugPosVertex::init();
		DebugUvVertex::init();
		DebugShapeVertex::init();
		DebugMeshVertex::init();

		max::RendererType::Enum type = max::getRendererType();

		m_program[DebugProgram::Lines] = max::createProgram(
			max::createEmbeddedShader(s_embeddedShaders, type, "vs_debugdraw_lines")
			, max::createEmbeddedShader(s_embeddedShaders, type, "fs_debugdraw_lines")
			, true
		);

		m_program[DebugProgram::LinesStipple] = max::createProgram(
			max::createEmbeddedShader(s_embeddedShaders, type, "vs_debugdraw_lines_stipple")
			, max::createEmbeddedShader(s_embeddedShaders, type, "fs_debugdraw_lines_stipple")
			, true
		);

		m_program[DebugProgram::Fill] = max::createProgram(
			max::createEmbeddedShader(s_embeddedShaders, type, "vs_debugdraw_fill")
			, max::createEmbeddedShader(s_embeddedShaders, type, "fs_debugdraw_fill")
			, true
		);

		m_program[DebugProgram::FillMesh] = max::createProgram(
			max::createEmbeddedShader(s_embeddedShaders, type, "vs_debugdraw_fill_mesh")
			, max::createEmbeddedShader(s_embeddedShaders, type, "fs_debugdraw_fill")
			, true
		);

		m_program[DebugProgram::FillLit] = max::createProgram(
			max::createEmbeddedShader(s_embeddedShaders, type, "vs_debugdraw_fill_lit")
			, max::createEmbeddedShader(s_embeddedShaders, type, "fs_debugdraw_fill_lit")
			, true
		);

		m_program[DebugProgram::FillLitMesh] = max::createProgram(
			max::createEmbeddedShader(s_embeddedShaders, type, "vs_debugdraw_fill_lit_mesh")
			, max::createEmbeddedShader(s_embeddedShaders, type, "fs_debugdraw_fill_lit")
			, true
		);

		m_program[DebugProgram::FillTexture] = max::createProgram(
			max::createEmbeddedShader(s_embeddedShaders, type, "vs_debugdraw_fill_texture")
			, max::createEmbeddedShader(s_embeddedShaders, type, "fs_debugdraw_fill_texture")
			, true
		);

		u_params = max::createUniform("u_params", max::UniformType::Vec4, 4);
		s_texColor = max::createUniform("s_texColor", max::UniformType::Sampler);

		void* vertices[DebugMesh::Count] = {};
		uint16_t* indices[DebugMesh::Count] = {};
		uint16_t stride = DebugShapeVertex::ms_layout.getStride();

		uint32_t startVertex = 0;
		uint32_t startIndex = 0;

		for (uint32_t mesh = 0; mesh < 4; ++mesh)
		{
			DebugMesh::Enum id = DebugMesh::Enum(DebugMesh::Sphere0 + mesh);

			const uint8_t  tess = uint8_t(3 - mesh);
			const uint32_t numVertices = genSphere(tess);
			const uint32_t numIndices = numVertices;

			vertices[id] = bx::alloc(g_allocator, numVertices * stride);
			bx::memSet(vertices[id], 0, numVertices * stride);
			genSphere(tess, vertices[id], stride);

			uint16_t* trilist = (uint16_t*)bx::alloc(g_allocator, numIndices * sizeof(uint16_t));
			for (uint32_t ii = 0; ii < numIndices; ++ii)
			{
				trilist[ii] = uint16_t(ii);
			}

			uint32_t numLineListIndices = max::topologyConvert(
				max::TopologyConvert::TriListToLineList
				, NULL
				, 0
				, trilist
				, numIndices
				, false
			);
			indices[id] = (uint16_t*)bx::alloc(g_allocator, (numIndices + numLineListIndices) * sizeof(uint16_t));
			uint16_t* indicesOut = indices[id];
			bx::memCopy(indicesOut, trilist, numIndices * sizeof(uint16_t));

			max::topologyConvert(
				max::TopologyConvert::TriListToLineList
				, &indicesOut[numIndices]
				, numLineListIndices * sizeof(uint16_t)
				, trilist
				, numIndices
				, false
			);

			m_mesh[id].m_startVertex = startVertex;
			m_mesh[id].m_numVertices = numVertices;
			m_mesh[id].m_startIndex[0] = startIndex;
			m_mesh[id].m_numIndices[0] = numIndices;
			m_mesh[id].m_startIndex[1] = startIndex + numIndices;
			m_mesh[id].m_numIndices[1] = numLineListIndices;

			startVertex += numVertices;
			startIndex += numIndices + numLineListIndices;

			bx::free(g_allocator, trilist);
		}

		for (uint32_t mesh = 0; mesh < 4; ++mesh)
		{
			DebugMesh::Enum id = DebugMesh::Enum(DebugMesh::Cone0 + mesh);

			const uint32_t num = getCircleLod(uint8_t(mesh));
			const float step = bx::kPi * 2.0f / num;

			const uint32_t numVertices = num + 1;
			const uint32_t numIndices = num * 6;
			const uint32_t numLineListIndices = num * 4;

			vertices[id] = bx::alloc(g_allocator, numVertices * stride);
			indices[id] = (uint16_t*)bx::alloc(g_allocator, (numIndices + numLineListIndices) * sizeof(uint16_t));
			bx::memSet(indices[id], 0, (numIndices + numLineListIndices) * sizeof(uint16_t));

			DebugShapeVertex* vertex = (DebugShapeVertex*)vertices[id];
			uint16_t* index = indices[id];

			vertex[num].m_x = 0.0f;
			vertex[num].m_y = 0.0f;
			vertex[num].m_z = 0.0f;
			vertex[num].m_indices[0] = 1;

			for (uint32_t ii = 0; ii < num; ++ii)
			{
				const float angle = step * ii;

				float xy[2];
				circle(xy, angle);

				vertex[ii].m_x = xy[1];
				vertex[ii].m_y = 0.0f;
				vertex[ii].m_z = xy[0];
				vertex[ii].m_indices[0] = 0;

				index[ii * 3 + 0] = uint16_t(num);
				index[ii * 3 + 1] = uint16_t((ii + 1) % num);
				index[ii * 3 + 2] = uint16_t(ii);

				index[num * 3 + ii * 3 + 0] = 0;
				index[num * 3 + ii * 3 + 1] = uint16_t(ii);
				index[num * 3 + ii * 3 + 2] = uint16_t((ii + 1) % num);

				index[numIndices + ii * 2 + 0] = uint16_t(ii);
				index[numIndices + ii * 2 + 1] = uint16_t(num);

				index[numIndices + num * 2 + ii * 2 + 0] = uint16_t(ii);
				index[numIndices + num * 2 + ii * 2 + 1] = uint16_t((ii + 1) % num);
			}

			m_mesh[id].m_startVertex = startVertex;
			m_mesh[id].m_numVertices = numVertices;
			m_mesh[id].m_startIndex[0] = startIndex;
			m_mesh[id].m_numIndices[0] = numIndices;
			m_mesh[id].m_startIndex[1] = startIndex + numIndices;
			m_mesh[id].m_numIndices[1] = numLineListIndices;

			startVertex += numVertices;
			startIndex += numIndices + numLineListIndices;
		}

		for (uint32_t mesh = 0; mesh < 4; ++mesh)
		{
			DebugMesh::Enum id = DebugMesh::Enum(DebugMesh::Cylinder0 + mesh);

			const uint32_t num = getCircleLod(uint8_t(mesh));
			const float step = bx::kPi * 2.0f / num;

			const uint32_t numVertices = num * 2;
			const uint32_t numIndices = num * 12;
			const uint32_t numLineListIndices = num * 6;

			vertices[id] = bx::alloc(g_allocator, numVertices * stride);
			indices[id] = (uint16_t*)bx::alloc(g_allocator, (numIndices + numLineListIndices) * sizeof(uint16_t));
			bx::memSet(indices[id], 0, (numIndices + numLineListIndices) * sizeof(uint16_t));

			DebugShapeVertex* vertex = (DebugShapeVertex*)vertices[id];
			uint16_t* index = indices[id];

			for (uint32_t ii = 0; ii < num; ++ii)
			{
				const float angle = step * ii;

				float xy[2];
				circle(xy, angle);

				vertex[ii].m_x = xy[1];
				vertex[ii].m_y = 0.0f;
				vertex[ii].m_z = xy[0];
				vertex[ii].m_indices[0] = 0;

				vertex[ii + num].m_x = xy[1];
				vertex[ii + num].m_y = 0.0f;
				vertex[ii + num].m_z = xy[0];
				vertex[ii + num].m_indices[0] = 1;

				index[ii * 6 + 0] = uint16_t(ii + num);
				index[ii * 6 + 1] = uint16_t((ii + 1) % num);
				index[ii * 6 + 2] = uint16_t(ii);
				index[ii * 6 + 3] = uint16_t(ii + num);
				index[ii * 6 + 4] = uint16_t((ii + 1) % num + num);
				index[ii * 6 + 5] = uint16_t((ii + 1) % num);

				index[num * 6 + ii * 6 + 0] = uint16_t(0);
				index[num * 6 + ii * 6 + 1] = uint16_t(ii);
				index[num * 6 + ii * 6 + 2] = uint16_t((ii + 1) % num);
				index[num * 6 + ii * 6 + 3] = uint16_t(num);
				index[num * 6 + ii * 6 + 4] = uint16_t((ii + 1) % num + num);
				index[num * 6 + ii * 6 + 5] = uint16_t(ii + num);

				index[numIndices + ii * 2 + 0] = uint16_t(ii);
				index[numIndices + ii * 2 + 1] = uint16_t(ii + num);

				index[numIndices + num * 2 + ii * 2 + 0] = uint16_t(ii);
				index[numIndices + num * 2 + ii * 2 + 1] = uint16_t((ii + 1) % num);

				index[numIndices + num * 4 + ii * 2 + 0] = uint16_t(num + ii);
				index[numIndices + num * 4 + ii * 2 + 1] = uint16_t(num + (ii + 1) % num);
			}

			m_mesh[id].m_startVertex = startVertex;
			m_mesh[id].m_numVertices = numVertices;
			m_mesh[id].m_startIndex[0] = startIndex;
			m_mesh[id].m_numIndices[0] = numIndices;
			m_mesh[id].m_startIndex[1] = startIndex + numIndices;
			m_mesh[id].m_numIndices[1] = numLineListIndices;

			startVertex += numVertices;
			startIndex += numIndices + numLineListIndices;
		}

		for (uint32_t mesh = 0; mesh < 4; ++mesh)
		{
			DebugMesh::Enum id = DebugMesh::Enum(DebugMesh::Capsule0 + mesh);

			const uint32_t num = getCircleLod(uint8_t(mesh));
			const float step = bx::kPi * 2.0f / num;

			const uint32_t numVertices = num * 2;
			const uint32_t numIndices = num * 6;
			const uint32_t numLineListIndices = num * 6;

			vertices[id] = bx::alloc(g_allocator, numVertices * stride);
			indices[id] = (uint16_t*)bx::alloc(g_allocator, (numIndices + numLineListIndices) * sizeof(uint16_t));
			bx::memSet(indices[id], 0, (numIndices + numLineListIndices) * sizeof(uint16_t));

			DebugShapeVertex* vertex = (DebugShapeVertex*)vertices[id];
			uint16_t* index = indices[id];

			for (uint32_t ii = 0; ii < num; ++ii)
			{
				const float angle = step * ii;

				float xy[2];
				circle(xy, angle);

				vertex[ii].m_x = xy[1];
				vertex[ii].m_y = 0.0f;
				vertex[ii].m_z = xy[0];
				vertex[ii].m_indices[0] = 0;

				vertex[ii + num].m_x = xy[1];
				vertex[ii + num].m_y = 0.0f;
				vertex[ii + num].m_z = xy[0];
				vertex[ii + num].m_indices[0] = 1;

				index[ii * 6 + 0] = uint16_t(ii + num);
				index[ii * 6 + 1] = uint16_t((ii + 1) % num);
				index[ii * 6 + 2] = uint16_t(ii);
				index[ii * 6 + 3] = uint16_t(ii + num);
				index[ii * 6 + 4] = uint16_t((ii + 1) % num + num);
				index[ii * 6 + 5] = uint16_t((ii + 1) % num);

				//				index[num*6+ii*6+0] = uint16_t(0);
				//				index[num*6+ii*6+1] = uint16_t(ii);
				//				index[num*6+ii*6+2] = uint16_t( (ii+1)%num);
				//				index[num*6+ii*6+3] = uint16_t(num);
				//				index[num*6+ii*6+4] = uint16_t( (ii+1)%num+num);
				//				index[num*6+ii*6+5] = uint16_t(ii+num);

				index[numIndices + ii * 2 + 0] = uint16_t(ii);
				index[numIndices + ii * 2 + 1] = uint16_t(ii + num);

				index[numIndices + num * 2 + ii * 2 + 0] = uint16_t(ii);
				index[numIndices + num * 2 + ii * 2 + 1] = uint16_t((ii + 1) % num);

				index[numIndices + num * 4 + ii * 2 + 0] = uint16_t(num + ii);
				index[numIndices + num * 4 + ii * 2 + 1] = uint16_t(num + (ii + 1) % num);
			}

			m_mesh[id].m_startVertex = startVertex;
			m_mesh[id].m_numVertices = numVertices;
			m_mesh[id].m_startIndex[0] = startIndex;
			m_mesh[id].m_numIndices[0] = numIndices;
			m_mesh[id].m_startIndex[1] = startIndex + numIndices;
			m_mesh[id].m_numIndices[1] = numLineListIndices;

			startVertex += numVertices;
			startIndex += numIndices + numLineListIndices;
		}

		m_mesh[DebugMesh::Quad].m_startVertex = startVertex;
		m_mesh[DebugMesh::Quad].m_numVertices = BX_COUNTOF(s_quadVertices);
		m_mesh[DebugMesh::Quad].m_startIndex[0] = startIndex;
		m_mesh[DebugMesh::Quad].m_numIndices[0] = BX_COUNTOF(s_quadIndices);
		m_mesh[DebugMesh::Quad].m_startIndex[1] = 0;
		m_mesh[DebugMesh::Quad].m_numIndices[1] = 0;
		startVertex += BX_COUNTOF(s_quadVertices);
		startIndex += BX_COUNTOF(s_quadIndices);

		m_mesh[DebugMesh::Cube].m_startVertex = startVertex;
		m_mesh[DebugMesh::Cube].m_numVertices = BX_COUNTOF(s_cubeVertices);
		m_mesh[DebugMesh::Cube].m_startIndex[0] = startIndex;
		m_mesh[DebugMesh::Cube].m_numIndices[0] = BX_COUNTOF(s_cubeIndices);
		m_mesh[DebugMesh::Cube].m_startIndex[1] = 0;
		m_mesh[DebugMesh::Cube].m_numIndices[1] = 0;
		startVertex += m_mesh[DebugMesh::Cube].m_numVertices;
		startIndex += m_mesh[DebugMesh::Cube].m_numIndices[0];

		const max::Memory* vb = max::alloc(startVertex * stride);
		const max::Memory* ib = max::alloc(startIndex * sizeof(uint16_t));

		for (uint32_t mesh = DebugMesh::Sphere0; mesh < DebugMesh::Quad; ++mesh)
		{
			DebugMesh::Enum id = DebugMesh::Enum(mesh);
			bx::memCopy(&vb->data[m_mesh[id].m_startVertex * stride]
				, vertices[id]
				, m_mesh[id].m_numVertices * stride
			);

			bx::memCopy(&ib->data[m_mesh[id].m_startIndex[0] * sizeof(uint16_t)]
				, indices[id]
				, (m_mesh[id].m_numIndices[0] + m_mesh[id].m_numIndices[1]) * sizeof(uint16_t)
			);

			bx::free(g_allocator, vertices[id]);
			bx::free(g_allocator, indices[id]);
		}

		bx::memCopy(&vb->data[m_mesh[DebugMesh::Quad].m_startVertex * stride]
			, s_quadVertices
			, sizeof(s_quadVertices)
		);

		bx::memCopy(&ib->data[m_mesh[DebugMesh::Quad].m_startIndex[0] * sizeof(uint16_t)]
			, s_quadIndices
			, sizeof(s_quadIndices)
		);

		bx::memCopy(&vb->data[m_mesh[DebugMesh::Cube].m_startVertex * stride]
			, s_cubeVertices
			, sizeof(s_cubeVertices)
		);

		bx::memCopy(&ib->data[m_mesh[DebugMesh::Cube].m_startIndex[0] * sizeof(uint16_t)]
			, s_cubeIndices
			, sizeof(s_cubeIndices)
		);

		m_vbh = max::createVertexBuffer(vb, DebugShapeVertex::ms_layout);
		m_ibh = max::createIndexBuffer(ib);
	}

	void DebugDrawShared::shutdown()
	{
		max::destroy(m_ibh);
		max::destroy(m_vbh);
		for (uint32_t ii = 0; ii < DebugProgram::Count; ++ii)
		{
			max::destroy(m_program[ii]);
		}
		max::destroy(u_params);
		max::destroy(s_texColor);
	}

	void DebugDrawEncoderImpl::setUParams(const DebugAttrib& _attrib, bool _wireframe)
	{
		const float flip = 0 == (_attrib.m_state & MAX_STATE_CULL_CCW) ? 1.0f : -1.0f;
		const uint8_t alpha = _attrib.m_abgr >> 24;

		float params[4][4] =
		{
			{ // lightDir
				 0.0f * flip,
				-1.0f * flip,
				 0.0f * flip,
				 3.0f, // shininess
			},
			{ // skyColor
				1.0f,
				0.9f,
				0.8f,
				0.0f, // unused
			},
			{ // groundColor.xyz0
				0.2f,
				0.22f,
				0.5f,
				0.0f, // unused
			},
			{ // matColor
				((_attrib.m_abgr) & 0xff) / 255.0f,
				((_attrib.m_abgr >> 8) & 0xff) / 255.0f,
				((_attrib.m_abgr >> 16) & 0xff) / 255.0f,
				(alpha) / 255.0f,
			},
		};

		bx::store(params[0], bx::normalize(bx::load<bx::Vec3>(params[0])));
		m_encoder->setUniform(s_dds.u_params, params, 4);

		m_encoder->setState(0
			| _attrib.m_state
			| (_wireframe ? MAX_STATE_PT_LINES | MAX_STATE_LINEAA | MAX_STATE_BLEND_ALPHA
				: (alpha < 0xff) ? MAX_STATE_BLEND_ALPHA : 0)
		);
	}

	void DebugDrawEncoderImpl::draw(bool _lineList, uint32_t _numVertices, const bx::Vec3* _vertices, uint32_t _numIndices, const uint16_t* _indices)
	{
		flush();

		if (_numVertices == max::getAvailTransientVertexBuffer(_numVertices, DebugMeshVertex::ms_layout))
		{
			max::TransientVertexBuffer tvb;
			max::allocTransientVertexBuffer(&tvb, _numVertices, DebugMeshVertex::ms_layout);
			bx::memCopy(tvb.data, _vertices, _numVertices * DebugMeshVertex::ms_layout.m_stride);
			m_encoder->setVertexBuffer(0, &tvb);

			const DebugAttrib& attrib = m_attrib[m_stack];
			const bool wireframe = _lineList || attrib.m_wireframe;
			setUParams(attrib, wireframe);

			if (0 < _numIndices)
			{
				uint32_t numIndices = _numIndices;
				max::TransientIndexBuffer tib;
				if (!_lineList && wireframe)
				{
					numIndices = max::topologyConvert(
						max::TopologyConvert::TriListToLineList
						, NULL
						, 0
						, _indices
						, _numIndices
						, false
					);

					max::allocTransientIndexBuffer(&tib, numIndices);
					max::topologyConvert(
						max::TopologyConvert::TriListToLineList
						, tib.data
						, numIndices * sizeof(uint16_t)
						, _indices
						, _numIndices
						, false
					);
				}
				else
				{
					max::allocTransientIndexBuffer(&tib, numIndices);
					bx::memCopy(tib.data, _indices, numIndices * sizeof(uint16_t));
				}

				m_encoder->setIndexBuffer(&tib);
			}

			m_encoder->setTransform(m_mtxStack[m_mtxStackCurrent].mtx);
			max::ProgramHandle program = s_dds.m_program[wireframe
				? DebugProgram::FillMesh
				: DebugProgram::FillLitMesh
			];
			m_encoder->submit(m_viewId, program);
		}
	}

	void DebugDrawEncoderImpl::draw(DebugMesh::Enum _mesh, const float* _mtx, uint16_t _num, bool _wireframe)
	{
		pushTransform(_mtx, _num, false /* flush */);

		const DebugMesh& mesh = s_dds.m_mesh[_mesh];

		if (0 != mesh.m_numIndices[_wireframe])
		{
			m_encoder->setIndexBuffer(s_dds.m_ibh
				, mesh.m_startIndex[_wireframe]
				, mesh.m_numIndices[_wireframe]
			);
		}

		const DebugAttrib& attrib = m_attrib[m_stack];
		setUParams(attrib, _wireframe);

		MatrixStack& stack = m_mtxStack[m_mtxStackCurrent];
		m_encoder->setTransform(stack.mtx, stack.num);

		m_encoder->setVertexBuffer(0, s_dds.m_vbh, mesh.m_startVertex, mesh.m_numVertices);
		m_encoder->submit(m_viewId, s_dds.m_program[_wireframe ? DebugProgram::Fill : DebugProgram::FillLit]);

		popTransform(false /* flush */);
	}

	void DebugDrawEncoderImpl::flush()
	{
		if (0 != m_pos)
		{
			if (checkAvailTransientBuffers(m_pos, DebugPosVertex::ms_layout, m_indexPos))
			{
				max::TransientVertexBuffer tvb;
				max::allocTransientVertexBuffer(&tvb, m_pos, DebugPosVertex::ms_layout);
				bx::memCopy(tvb.data, m_cache, m_pos * DebugPosVertex::ms_layout.m_stride);

				max::TransientIndexBuffer tib;
				max::allocTransientIndexBuffer(&tib, m_indexPos);
				bx::memCopy(tib.data, m_indices, m_indexPos * sizeof(uint16_t));

				const DebugAttrib& attrib = m_attrib[m_stack];

				m_encoder->setVertexBuffer(0, &tvb);
				m_encoder->setIndexBuffer(&tib);
				m_encoder->setState(0
					| MAX_STATE_WRITE_RGB
					| MAX_STATE_PT_LINES
					| attrib.m_state
					| MAX_STATE_LINEAA
					| MAX_STATE_BLEND_ALPHA
				);
				m_encoder->setTransform(m_mtxStack[m_mtxStackCurrent].mtx);
				max::ProgramHandle program = s_dds.m_program[attrib.m_stipple ? 1 : 0];
				m_encoder->submit(m_viewId, program);
			}

			m_state = State::None;
			m_pos = 0;
			m_indexPos = 0;
			m_vertexPos = 0;
		}
	}

	void DebugDrawEncoderImpl::flushQuad()
	{
		if (0 != m_posQuad)
		{
			const uint32_t numIndices = m_posQuad / 4 * 6;
			if (checkAvailTransientBuffers(m_posQuad, DebugUvVertex::ms_layout, numIndices))
			{
				max::TransientVertexBuffer tvb;
				max::allocTransientVertexBuffer(&tvb, m_posQuad, DebugUvVertex::ms_layout);
				bx::memCopy(tvb.data, m_cacheQuad, m_posQuad * DebugUvVertex::ms_layout.m_stride);

				max::TransientIndexBuffer tib;
				max::allocTransientIndexBuffer(&tib, numIndices);
				uint16_t* indices = (uint16_t*)tib.data;
				for (uint16_t ii = 0, num = m_posQuad / 4; ii < num; ++ii)
				{
					uint16_t startVertex = ii * 4;
					indices[0] = startVertex + 0;
					indices[1] = startVertex + 1;
					indices[2] = startVertex + 2;
					indices[3] = startVertex + 1;
					indices[4] = startVertex + 3;
					indices[5] = startVertex + 2;
					indices += 6;
				}

				const DebugAttrib& attrib = m_attrib[m_stack];

				m_encoder->setVertexBuffer(0, &tvb);
				m_encoder->setIndexBuffer(&tib);
				m_encoder->setState(0
					| (attrib.m_state & ~MAX_STATE_CULL_MASK)
				);
				m_encoder->setTransform(m_mtxStack[m_mtxStackCurrent].mtx);
				m_encoder->submit(m_viewId, s_dds.m_program[DebugProgram::FillTexture]);
			}

			m_posQuad = 0;
		}
	}

	void EncoderImpl::submit(ViewId _id, ProgramHandle _program, OcclusionQueryHandle _occlusionQuery, uint32_t _depth, uint8_t _flags)
	{
		if (BX_ENABLED(MAX_CONFIG_DEBUG_UNIFORM)
		&& (_flags & MAX_DISCARD_STATE))
		{
			m_uniformSet.clear();
		}

		if (BX_ENABLED(MAX_CONFIG_DEBUG_OCCLUSION)
		&&  isValid(_occlusionQuery) )
		{
			BX_ASSERT(m_occlusionQuerySet.end() == m_occlusionQuerySet.find(_occlusionQuery.idx)
				, "OcclusionQuery %d was already used for this frame."
				, _occlusionQuery.idx
				);
			m_occlusionQuerySet.insert(_occlusionQuery.idx);
		}

		if (m_discard)
		{
			discard(_flags);
			return;
		}

		if (0 == m_draw.m_numVertices
		&&  0 == m_draw.m_numIndices)
		{
			discard(_flags);
			++m_numDropped;
			return;
		}

		const uint32_t renderItemIdx = bx::atomicFetchAndAddsat<uint32_t>(&m_frame->m_numRenderItems, 1, MAX_CONFIG_MAX_DRAW_CALLS);
		if (MAX_CONFIG_MAX_DRAW_CALLS <= renderItemIdx)
		{
			discard(_flags);
			++m_numDropped;
			return;
		}

		++m_numSubmitted;

		UniformBuffer* uniformBuffer = m_frame->m_uniformBuffer[m_uniformIdx];
		m_uniformEnd = uniformBuffer->getPos();

		m_key.m_program = isValid(_program)
			? _program
			: ProgramHandle{0}
			;

		m_key.m_view = _id;

		SortKey::Enum type;
		switch (s_ctx->m_view[_id].m_mode)
		{
		case ViewMode::Sequential:      m_key.m_seq   = s_ctx->getSeqIncr(_id); type = SortKey::SortSequence; break;
		case ViewMode::DepthAscending:  m_key.m_depth =            _depth;      type = SortKey::SortDepth;    break;
		case ViewMode::DepthDescending: m_key.m_depth = UINT32_MAX-_depth;      type = SortKey::SortDepth;    break;
		default:                        m_key.m_depth =            _depth;      type = SortKey::SortProgram;  break;
		}

		uint64_t key = m_key.encodeDraw(type);

		m_frame->m_sortKeys[renderItemIdx]   = key;
		m_frame->m_sortValues[renderItemIdx] = RenderItemCount(renderItemIdx);

		m_draw.m_uniformIdx   = m_uniformIdx;
		m_draw.m_uniformBegin = m_uniformBegin;
		m_draw.m_uniformEnd   = m_uniformEnd;

		if (UINT8_MAX != m_draw.m_streamMask)
		{
			uint32_t numVertices = UINT32_MAX;
			for (uint32_t idx = 0, streamMask = m_draw.m_streamMask
				; 0 != streamMask
				; streamMask >>= 1, idx += 1
				)
			{
				const uint32_t ntz = bx::uint32_cnttz(streamMask);
				streamMask >>= ntz;
				idx         += ntz;
				numVertices = bx::min(numVertices, m_numVertices[idx]);
			}

			m_draw.m_numVertices = numVertices;
		}
		else
		{
			m_draw.m_numVertices = m_numVertices[0];
		}

		if (isValid(_occlusionQuery) )
		{
			m_draw.m_stateFlags |= MAX_STATE_INTERNAL_OCCLUSION_QUERY;
			m_draw.m_occlusionQuery = _occlusionQuery;
		}

		m_frame->m_renderItem[renderItemIdx].draw = m_draw;
		m_frame->m_renderItemBind[renderItemIdx]  = m_bind;

		m_draw.clear(_flags);
		m_bind.clear(_flags);
		if (_flags & MAX_DISCARD_STATE)
		{
			m_uniformBegin = m_uniformEnd;
		}
	}

	void EncoderImpl::dispatch(ViewId _id, ProgramHandle _handle, uint32_t _numX, uint32_t _numY, uint32_t _numZ, uint8_t _flags)
	{
		if (BX_ENABLED(MAX_CONFIG_DEBUG_UNIFORM) )
		{
			m_uniformSet.clear();
		}

		if (m_discard)
		{
			discard(_flags);
			return;
		}

		const uint32_t renderItemIdx = bx::atomicFetchAndAddsat<uint32_t>(&m_frame->m_numRenderItems, 1, MAX_CONFIG_MAX_DRAW_CALLS);
		if (MAX_CONFIG_MAX_DRAW_CALLS-1 <= renderItemIdx)
		{
			discard(_flags);
			++m_numDropped;
			return;
		}

		++m_numSubmitted;

		UniformBuffer* uniformBuffer = m_frame->m_uniformBuffer[m_uniformIdx];
		m_uniformEnd = uniformBuffer->getPos();

		m_compute.m_startMatrix = m_draw.m_startMatrix;
		m_compute.m_numMatrices = m_draw.m_numMatrices;
		m_compute.m_numX   = bx::max(_numX, 1u);
		m_compute.m_numY   = bx::max(_numY, 1u);
		m_compute.m_numZ   = bx::max(_numZ, 1u);

		m_key.m_program = _handle;
		m_key.m_depth   = 0;
		m_key.m_view    = _id;
		m_key.m_seq     = s_ctx->getSeqIncr(_id);

		uint64_t key = m_key.encodeCompute();
		m_frame->m_sortKeys[renderItemIdx]   = key;
		m_frame->m_sortValues[renderItemIdx] = RenderItemCount(renderItemIdx);

		m_compute.m_uniformIdx   = m_uniformIdx;
		m_compute.m_uniformBegin = m_uniformBegin;
		m_compute.m_uniformEnd   = m_uniformEnd;
		m_frame->m_renderItem[renderItemIdx].compute = m_compute;
		m_frame->m_renderItemBind[renderItemIdx]     = m_bind;

		m_compute.clear(_flags);
		m_bind.clear(_flags);
		m_uniformBegin = m_uniformEnd;
	}

	void EncoderImpl::blit(ViewId _id, TextureHandle _dst, uint8_t _dstMip, uint16_t _dstX, uint16_t _dstY, uint16_t _dstZ, TextureHandle _src, uint8_t _srcMip, uint16_t _srcX, uint16_t _srcY, uint16_t _srcZ, uint16_t _width, uint16_t _height, uint16_t _depth)
	{
		BX_WARN(m_frame->m_numBlitItems < MAX_CONFIG_MAX_BLIT_ITEMS
			, "Exceed number of available blit items per frame. MAX_CONFIG_MAX_BLIT_ITEMS is %d. Skipping blit."
			, MAX_CONFIG_MAX_BLIT_ITEMS
			);
		if (m_frame->m_numBlitItems < MAX_CONFIG_MAX_BLIT_ITEMS)
		{
			uint16_t item = m_frame->m_numBlitItems++;

			BlitItem& bi = m_frame->m_blitItem[item];
			bi.m_srcX    = _srcX;
			bi.m_srcY    = _srcY;
			bi.m_srcZ    = _srcZ;
			bi.m_dstX    = _dstX;
			bi.m_dstY    = _dstY;
			bi.m_dstZ    = _dstZ;
			bi.m_width   = _width;
			bi.m_height  = _height;
			bi.m_depth   = _depth;
			bi.m_srcMip  = _srcMip;
			bi.m_dstMip  = _dstMip;
			bi.m_src     = _src;
			bi.m_dst     = _dst;

			BlitKey key;
			key.m_view = _id;
			key.m_item = item;
			m_frame->m_blitKeys[item] = key.encode();
		}
	}

	void Frame::sort()
	{
		MAX_PROFILER_SCOPE("max/Sort", 0xff2040ff);

		ViewId viewRemap[MAX_CONFIG_MAX_VIEWS];
		for (uint32_t ii = 0; ii < MAX_CONFIG_MAX_VIEWS; ++ii)
		{
			viewRemap[m_viewRemap[ii] ] = ViewId(ii);

			View& view = m_view[ii];
			Rect rect(0, 0, uint16_t(m_resolution.width), uint16_t(m_resolution.height) );

			if (isValid(view.m_fbh) )
			{
				const FrameBufferRef& fbr = s_ctx->m_frameBufferRef[view.m_fbh.idx];
				const BackbufferRatio::Enum bbRatio = fbr.m_window
					? BackbufferRatio::Count
					: BackbufferRatio::Enum(s_ctx->m_textureRef[fbr.un.m_th[0].idx].m_bbRatio)
					;

				if (BackbufferRatio::Count != bbRatio)
				{
					getTextureSizeFromRatio(bbRatio, rect.m_width, rect.m_height);
				}
				else
				{
					rect.m_width  = fbr.m_width;
					rect.m_height = fbr.m_height;
				}
			}

			view.m_rect.intersect(rect);

			if (!view.m_scissor.isZero() )
			{
				view.m_scissor.intersect(rect);
			}
		}

		for (uint32_t ii = 0, num = m_numRenderItems; ii < num; ++ii)
		{
			m_sortKeys[ii] = SortKey::remapView(m_sortKeys[ii], viewRemap);
		}

		bx::radixSort(m_sortKeys, s_ctx->m_tempKeys, m_sortValues, s_ctx->m_tempValues, m_numRenderItems);

		for (uint32_t ii = 0, num = m_numBlitItems; ii < num; ++ii)
		{
			m_blitKeys[ii] = BlitKey::remapView(m_blitKeys[ii], viewRemap);
		}

		bx::radixSort(m_blitKeys, (uint32_t*)&s_ctx->m_tempKeys, m_numBlitItems);
	}

	RenderFrame::Enum renderFrame(int32_t _msecs)
	{
		if (BX_ENABLED(MAX_CONFIG_MULTITHREADED) )
		{
			if (s_renderFrameCalled)
			{
				MAX_CHECK_RENDER_THREAD();
			}

			if (NULL == s_ctx)
			{
				s_renderFrameCalled = true;
				s_threadIndex = ~MAX_API_THREAD_MAGIC;
				return RenderFrame::NoContext;
			}

			int32_t msecs = -1 == _msecs
				? MAX_CONFIG_API_SEMAPHORE_TIMEOUT
				: _msecs
				;
			RenderFrame::Enum result = s_ctx->renderFrame(msecs);
			if (RenderFrame::Exiting == result)
			{
				Context* ctx = s_ctx;
				ctx->apiSemWait();
				s_ctx = NULL;
				ctx->renderSemPost();
			}

			return result;
		}

		BX_ASSERT(false, "This call only makes sense if used with multi-threaded renderer.");
		return RenderFrame::NoContext;
	}

	const uint32_t g_uniformTypeSize[UniformType::Count+1] =
	{
		sizeof(int32_t),
		0,
		4*sizeof(float),
		3*3*sizeof(float),
		4*4*sizeof(float),
		1,
	};

	void UniformBuffer::writeUniform(UniformType::Enum _type, uint16_t _loc, const void* _value, uint16_t _num)
	{
		const uint32_t opcode = encodeOpcode(_type, _loc, _num, true);
		write(opcode);
		write(_value, g_uniformTypeSize[_type]*_num);
	}

	void UniformBuffer::writeUniformHandle(UniformType::Enum _type, uint16_t _loc, UniformHandle _handle, uint16_t _num)
	{
		const uint32_t opcode = encodeOpcode(_type, _loc, _num, false);
		write(opcode);
		write(&_handle, sizeof(UniformHandle) );
	}

	void UniformBuffer::writeMarker(const bx::StringView& _name)
	{
		const uint16_t num = bx::narrowCast<uint16_t>(_name.getLength()+1);
		const uint32_t opcode = encodeOpcode(max::UniformType::Count, 0, num, true);
		write(opcode);
		write(_name.getPtr(), num-1);
		const char zero = '\0';
		write(&zero, 1);
	}

	struct CapsFlags
	{
		uint64_t m_flag;
		const char* m_str;
	};

	static const CapsFlags s_capsFlags[] =
	{
#define CAPS_FLAGS(_x) { _x, #_x }
		CAPS_FLAGS(MAX_CAPS_ALPHA_TO_COVERAGE),
		CAPS_FLAGS(MAX_CAPS_BLEND_INDEPENDENT),
		CAPS_FLAGS(MAX_CAPS_COMPUTE),
		CAPS_FLAGS(MAX_CAPS_CONSERVATIVE_RASTER),
		CAPS_FLAGS(MAX_CAPS_DRAW_INDIRECT),
		CAPS_FLAGS(MAX_CAPS_FRAGMENT_DEPTH),
		CAPS_FLAGS(MAX_CAPS_FRAGMENT_ORDERING),
		CAPS_FLAGS(MAX_CAPS_GRAPHICS_DEBUGGER),
		CAPS_FLAGS(MAX_CAPS_HDR10),
		CAPS_FLAGS(MAX_CAPS_HIDPI),
		CAPS_FLAGS(MAX_CAPS_IMAGE_RW),
		CAPS_FLAGS(MAX_CAPS_INDEX32),
		CAPS_FLAGS(MAX_CAPS_INSTANCING),
		CAPS_FLAGS(MAX_CAPS_OCCLUSION_QUERY),
		CAPS_FLAGS(MAX_CAPS_RENDERER_MULTITHREADED),
		CAPS_FLAGS(MAX_CAPS_SWAP_CHAIN),
		CAPS_FLAGS(MAX_CAPS_TEXTURE_2D_ARRAY),
		CAPS_FLAGS(MAX_CAPS_TEXTURE_3D),
		CAPS_FLAGS(MAX_CAPS_TEXTURE_BLIT),
		CAPS_FLAGS(MAX_CAPS_TRANSPARENT_BACKBUFFER),
		CAPS_FLAGS(MAX_CAPS_TEXTURE_COMPARE_ALL),
		CAPS_FLAGS(MAX_CAPS_TEXTURE_COMPARE_LEQUAL),
		CAPS_FLAGS(MAX_CAPS_TEXTURE_CUBE_ARRAY),
		CAPS_FLAGS(MAX_CAPS_TEXTURE_DIRECT_ACCESS),
		CAPS_FLAGS(MAX_CAPS_TEXTURE_READ_BACK),
		CAPS_FLAGS(MAX_CAPS_VERTEX_ATTRIB_HALF),
		CAPS_FLAGS(MAX_CAPS_VERTEX_ATTRIB_UINT10),
		CAPS_FLAGS(MAX_CAPS_VERTEX_ID),
		CAPS_FLAGS(MAX_CAPS_PRIMITIVE_ID),
		CAPS_FLAGS(MAX_CAPS_VIEWPORT_LAYER_ARRAY),
#undef CAPS_FLAGS
	};

	static void dumpCaps()
	{
		BX_TRACE("");

		if (0 < g_caps.numGPUs)
		{
			BX_TRACE("Detected GPUs (%d):", g_caps.numGPUs);
			BX_TRACE("\t +----------------   Index");
			BX_TRACE("\t |  +-------------   Device ID");
			BX_TRACE("\t |  |    +--------   Vendor ID");
			for (uint32_t ii = 0; ii < g_caps.numGPUs; ++ii)
			{
				const Caps::GPU& gpu = g_caps.gpu[ii];
				BX_UNUSED(gpu);

				BX_TRACE("\t %d: %04x %04x"
					, ii
					, gpu.deviceId
					, gpu.vendorId
					);
			}

			BX_TRACE("");
		}

		BX_TRACE("GPU device, Device ID: %04x, Vendor ID: %04x", g_caps.deviceId, g_caps.vendorId);
		BX_TRACE("");

		RendererType::Enum renderers[RendererType::Count];
		uint8_t num = getSupportedRenderers(BX_COUNTOF(renderers), renderers);

		BX_TRACE("Supported renderer backends (%d):", num);
		for (uint32_t ii = 0; ii < num; ++ii)
		{
			BX_TRACE("\t - %s", getRendererName(renderers[ii]) );
		}

		BX_TRACE("");
		BX_TRACE("Sort key masks:");
		BX_TRACE("\t   View     %016" PRIx64, kSortKeyViewMask);
		BX_TRACE("\t   Draw bit %016" PRIx64, kSortKeyDrawBit);

		BX_TRACE("");
		BX_TRACE("\tD  Type     %016" PRIx64, kSortKeyDrawTypeMask);

		BX_TRACE("");
		BX_TRACE("\tD0 Blend    %016" PRIx64, kSortKeyDraw0BlendMask);
		BX_TRACE("\tD0 Program  %016" PRIx64, kSortKeyDraw0ProgramMask);
		BX_TRACE("\tD0 Depth    %016" PRIx64, kSortKeyDraw0DepthMask);

		BX_TRACE("");
		BX_TRACE("\tD1 Depth    %016" PRIx64, kSortKeyDraw1DepthMask);
		BX_TRACE("\tD1 Blend    %016" PRIx64, kSortKeyDraw1BlendMask);
		BX_TRACE("\tD1 Program  %016" PRIx64, kSortKeyDraw1ProgramMask);

		BX_TRACE("");
		BX_TRACE("\tD2 Seq      %016" PRIx64, kSortKeyDraw2SeqMask);
		BX_TRACE("\tD2 Blend    %016" PRIx64, kSortKeyDraw2BlendMask);
		BX_TRACE("\tD2 Program  %016" PRIx64, kSortKeyDraw2ProgramMask);

		BX_TRACE("");
		BX_TRACE("\t C Seq      %016" PRIx64, kSortKeyComputeSeqMask);
		BX_TRACE("\t C Program  %016" PRIx64, kSortKeyComputeProgramMask);

		BX_TRACE("");
		BX_TRACE("Capabilities (renderer %s, vendor 0x%04x, device 0x%04x):"
				, s_ctx->m_renderCtx->getRendererName()
				, g_caps.vendorId
				, g_caps.deviceId
				);
		for (uint32_t ii = 0; ii < BX_COUNTOF(s_capsFlags); ++ii)
		{
			BX_TRACE("\t[%c] %s"
				, 0 != (g_caps.supported & s_capsFlags[ii].m_flag) ? 'x' : ' '
				, s_capsFlags[ii].m_str
				);
		}
		BX_UNUSED(s_capsFlags);

		BX_TRACE("");
		BX_TRACE("Limits:");
#define LIMITS(_x) BX_TRACE("\t%-24s%10d", #_x, g_caps.limits._x)
		LIMITS(maxDrawCalls);
		LIMITS(maxBlits);
		LIMITS(maxTextureSize);
		LIMITS(maxTextureLayers);
		LIMITS(maxViews);
		LIMITS(maxFrameBuffers);
		LIMITS(maxFBAttachments);
		LIMITS(maxPrograms);
		LIMITS(maxShaders);
		LIMITS(maxTextures);
		LIMITS(maxTextureSamplers);
		LIMITS(maxComputeBindings);
		LIMITS(maxVertexLayouts);
		LIMITS(maxVertexStreams);
		LIMITS(maxIndexBuffers);
		LIMITS(maxVertexBuffers);
		LIMITS(maxDynamicIndexBuffers);
		LIMITS(maxDynamicVertexBuffers);
		LIMITS(maxUniforms);
		LIMITS(maxOcclusionQueries);
		LIMITS(maxEncoders);
		LIMITS(minResourceCbSize);
		LIMITS(transientVbSize);
		LIMITS(transientIbSize);
#undef LIMITS

		BX_TRACE("");
		BX_TRACE("Supported texture formats:");
		BX_TRACE("\t +----------------   2D: x = supported / * = emulated");
		BX_TRACE("\t |+---------------   2D: sRGB format");
		BX_TRACE("\t ||+--------------   3D: x = supported / * = emulated");
		BX_TRACE("\t |||+-------------   3D: sRGB format");
		BX_TRACE("\t ||||+------------ Cube: x = supported / * = emulated");
		BX_TRACE("\t |||||+----------- Cube: sRGB format");
		BX_TRACE("\t ||||||+---------- vertex format");
		BX_TRACE("\t |||||||+--------- image: i = read-write / r = read / w = write");
		BX_TRACE("\t ||||||||+-------- framebuffer");
		BX_TRACE("\t |||||||||+------- MSAA framebuffer");
		BX_TRACE("\t ||||||||||+------ MSAA texture");
		BX_TRACE("\t |||||||||||+----- Auto-generated mips");
		BX_TRACE("\t ||||||||||||  +-- name");
		for (uint32_t ii = 0; ii < TextureFormat::Count; ++ii)
		{
			if (TextureFormat::Unknown != ii
			&&  TextureFormat::UnknownDepth != ii)
			{
				uint32_t flags = g_caps.formats[ii];
				BX_TRACE("\t[%c%c%c%c%c%c%c%c%c%c%c%c] %s"
					, flags&MAX_CAPS_FORMAT_TEXTURE_2D               ? 'x' : flags&MAX_CAPS_FORMAT_TEXTURE_2D_EMULATED ? '*' : ' '
					, flags&MAX_CAPS_FORMAT_TEXTURE_2D_SRGB          ? 'l' : ' '
					, flags&MAX_CAPS_FORMAT_TEXTURE_3D               ? 'x' : flags&MAX_CAPS_FORMAT_TEXTURE_3D_EMULATED ? '*' : ' '
					, flags&MAX_CAPS_FORMAT_TEXTURE_3D_SRGB          ? 'l' : ' '
					, flags&MAX_CAPS_FORMAT_TEXTURE_CUBE             ? 'x' : flags&MAX_CAPS_FORMAT_TEXTURE_CUBE_EMULATED ? '*' : ' '
					, flags&MAX_CAPS_FORMAT_TEXTURE_CUBE_SRGB        ? 'l' : ' '
					, flags&MAX_CAPS_FORMAT_TEXTURE_VERTEX           ? 'v' : ' '
					, (flags&MAX_CAPS_FORMAT_TEXTURE_IMAGE_READ) &&
					  (flags&MAX_CAPS_FORMAT_TEXTURE_IMAGE_WRITE)    ? 'i' : flags&MAX_CAPS_FORMAT_TEXTURE_IMAGE_READ ? 'r' : flags&MAX_CAPS_FORMAT_TEXTURE_IMAGE_WRITE ? 'w' : ' '
					, flags&MAX_CAPS_FORMAT_TEXTURE_FRAMEBUFFER      ? 'f' : ' '
					, flags&MAX_CAPS_FORMAT_TEXTURE_FRAMEBUFFER_MSAA ? '+' : ' '
					, flags&MAX_CAPS_FORMAT_TEXTURE_MSAA             ? 'm' : ' '
					, flags&MAX_CAPS_FORMAT_TEXTURE_MIP_AUTOGEN      ? 'M' : ' '
					, getName(TextureFormat::Enum(ii) )
					);
				BX_UNUSED(flags);
			}
		}

		BX_TRACE("");
		BX_TRACE("NDC depth [%d, 1], origin %s left."
			, g_caps.homogeneousDepth ? -1 : 0
			, g_caps.originBottomLeft ? "bottom" : "top"
			);

		BX_TRACE("");
	}

	void dump(const Resolution& _resolution)
	{
		const uint32_t reset = _resolution.reset;
		const uint32_t msaa = (reset&MAX_RESET_MSAA_MASK)>>MAX_RESET_MSAA_SHIFT;
		BX_UNUSED(reset, msaa);

		BX_TRACE("Reset back-buffer swap chain:");
		BX_TRACE("\t%dx%d, format: %s, numBackBuffers: %d, maxFrameLatency: %d"
			, _resolution.width
			, _resolution.height
			, TextureFormat::Count == _resolution.format
				? "*default*"
				: bimg::getName(bimg::TextureFormat::Enum(_resolution.format) )
			, _resolution.numBackBuffers
			, _resolution.maxFrameLatency
			);

		BX_TRACE("\t[%c] MSAAx%d",                 0 != msaa                                        ? 'x' : ' ', 1<<msaa);
		BX_TRACE("\t[%c] Fullscreen",              0 != (reset & MAX_RESET_FULLSCREEN)             ? 'x' : ' ');
		BX_TRACE("\t[%c] V-sync",                  0 != (reset & MAX_RESET_VSYNC)                  ? 'x' : ' ');
		BX_TRACE("\t[%c] Max Anisotropy",          0 != (reset & MAX_RESET_MAXANISOTROPY)          ? 'x' : ' ');
		BX_TRACE("\t[%c] Capture",                 0 != (reset & MAX_RESET_CAPTURE)                ? 'x' : ' ');
		BX_TRACE("\t[%c] Flush After Render",      0 != (reset & MAX_RESET_FLUSH_AFTER_RENDER)     ? 'x' : ' ');
		BX_TRACE("\t[%c] Flip After Render",       0 != (reset & MAX_RESET_FLIP_AFTER_RENDER)      ? 'x' : ' ');
		BX_TRACE("\t[%c] sRGB Back Buffer",        0 != (reset & MAX_RESET_SRGB_BACKBUFFER)        ? 'x' : ' ');
		BX_TRACE("\t[%c] Transparent Back Buffer", 0 != (reset & MAX_RESET_TRANSPARENT_BACKBUFFER) ? 'x' : ' ');
		BX_TRACE("\t[%c] HDR10",                   0 != (reset & MAX_RESET_HDR10)                  ? 'x' : ' ');
		BX_TRACE("\t[%c] Hi-DPI",                  0 != (reset & MAX_RESET_HIDPI)                  ? 'x' : ' ');
		BX_TRACE("\t[%c] Depth Clamp",             0 != (reset & MAX_RESET_DEPTH_CLAMP)            ? 'x' : ' ');
		BX_TRACE("\t[%c] Suspend",                 0 != (reset & MAX_RESET_SUSPEND)                ? 'x' : ' ');
	}

	TextureFormat::Enum getViableTextureFormat(const bimg::ImageContainer& _imageContainer)
	{
		const uint32_t formatCaps = g_caps.formats[_imageContainer.m_format];
		bool convert = 0 == formatCaps;

		if (_imageContainer.m_cubeMap)
		{
			convert |= 0 == (formatCaps & MAX_CAPS_FORMAT_TEXTURE_CUBE)
					&& 0 != (formatCaps & MAX_CAPS_FORMAT_TEXTURE_CUBE_EMULATED)
					;
		}
		else if (_imageContainer.m_depth > 1)
		{
			convert |= 0 == (formatCaps & MAX_CAPS_FORMAT_TEXTURE_3D)
					&& 0 != (formatCaps & MAX_CAPS_FORMAT_TEXTURE_3D_EMULATED)
					;
		}
		else
		{
			convert |= 0 == (formatCaps & MAX_CAPS_FORMAT_TEXTURE_2D)
					&& 0 != (formatCaps & MAX_CAPS_FORMAT_TEXTURE_2D_EMULATED)
					;
		}

		if (convert)
		{
			return TextureFormat::BGRA8;
		}

		return TextureFormat::Enum(_imageContainer.m_format);
	}

	const char* getName(TextureFormat::Enum _fmt)
	{
		return bimg::getName(bimg::TextureFormat::Enum(_fmt));
	}

	const char* getName(UniformHandle _handle)
	{
		return s_ctx->m_uniformRef[_handle.idx].m_name.getPtr();
	}

	const char* getName(ShaderHandle _handle)
	{
		return s_ctx->m_shaderRef[_handle.idx].m_name.getPtr();
	}

	static const char* s_topologyName[] =
	{
		"Triangles",
		"TriStrip",
		"Lines",
		"LineStrip",
		"Points",
	};
	BX_STATIC_ASSERT(Topology::Count == BX_COUNTOF(s_topologyName) );

	const char* getName(Topology::Enum _topology)
	{
		return s_topologyName[bx::min(_topology, Topology::PointList)];
	}

	const char* getShaderTypeName(uint32_t _magic)
	{
		if (isShaderType(_magic, 'C') )
		{
			return "Compute";
		}
		else if (isShaderType(_magic, 'F') )
		{
			return "Fragment";
		}
		else if (isShaderType(_magic, 'V') )
		{
			return "Vertex";
		}

		BX_ASSERT(false, "Invalid shader type!");

		return NULL;
	}

	static TextureFormat::Enum s_emulatedFormats[] =
	{
		TextureFormat::BC1,
		TextureFormat::BC2,
		TextureFormat::BC3,
		TextureFormat::BC4,
		TextureFormat::BC5,
		TextureFormat::ETC1,
		TextureFormat::ETC2,
		TextureFormat::ETC2A,
		TextureFormat::ETC2A1,
		TextureFormat::PTC12,
		TextureFormat::PTC14,
		TextureFormat::PTC12A,
		TextureFormat::PTC14A,
		TextureFormat::PTC22,
		TextureFormat::PTC24,
		TextureFormat::ATC,
		TextureFormat::ATCE,
		TextureFormat::ATCI,
		TextureFormat::ASTC4x4,
		TextureFormat::ASTC5x4,
		TextureFormat::ASTC5x5,
		TextureFormat::ASTC6x5,
		TextureFormat::ASTC6x6,
		TextureFormat::ASTC8x5,
		TextureFormat::ASTC8x6,
		TextureFormat::ASTC8x8,
		TextureFormat::ASTC10x5,
		TextureFormat::ASTC10x6,
		TextureFormat::ASTC10x8,
		TextureFormat::ASTC10x10,
		TextureFormat::ASTC12x10,
		TextureFormat::ASTC12x12,
		TextureFormat::BGRA8, // GL doesn't support BGRA8 without extensions.
		TextureFormat::RGBA8, // D3D9 doesn't support RGBA8
	};

	///
	RendererContextI* rendererCreate(const Init& _init);

	///
	void rendererDestroy(RendererContextI* _renderCtx);

	///
	PhysicsContextI* physicsCreate(const Init& _init);

	///
	void physicsDestroy(PhysicsContextI* _physicsCtx);

	bool Context::init(const Init& _init)
	{
		if (m_rendererInitialized)
		{
			BX_TRACE("Already initialized!");
			return false;
		}

		m_headless = true
			&&  RendererType::Noop != _init.rendererType
			&&  NULL == _init.platformData.ndt
			&&  NULL == _init.platformData.nwh
			&&  NULL == _init.platformData.context
			&&  NULL == _init.platformData.backBuffer
			&&  NULL == _init.platformData.backBufferDS
			;
		BX_WARN(!m_headless, "max platform data like window handle or backbuffer is not set, creating headless device.");

		if (m_headless
		&&  0 != _init.resolution.width
		&&  0 != _init.resolution.height)
		{
			BX_TRACE("Initializing headless mode, resolution of non-existing backbuffer can't be larger than 0x0!");
			return false;
		}

		m_init = _init;
		m_init.resolution.reset &= ~MAX_RESET_INTERNAL_FORCE;
		m_init.resolution.numBackBuffers  = bx::clamp<uint8_t>(_init.resolution.numBackBuffers, 2, MAX_CONFIG_MAX_BACK_BUFFERS);
		m_init.resolution.maxFrameLatency =   bx::min<uint8_t>(_init.resolution.maxFrameLatency,   MAX_CONFIG_MAX_FRAME_LATENCY);
		m_init.resolution.debugTextScale  = bx::clamp<uint8_t>(_init.resolution.debugTextScale, 1, MAX_CONFIG_DEBUG_TEXT_MAX_SCALE);
		dump(m_init.resolution);

		bx::memCopy(&g_platformData, &m_init.platformData, sizeof(PlatformData) );

		m_exit    = false;
		m_flipped = true;
		m_debug   = MAX_DEBUG_NONE;
		m_frameTimeLast = bx::getHPCounter();
		m_flipAfterRender = !!(m_init.resolution.reset & MAX_RESET_FLIP_AFTER_RENDER);

		m_submit->create(_init.limits.minResourceCbSize);

#if MAX_CONFIG_MULTITHREADED
		m_render->create(_init.limits.minResourceCbSize);

		if (s_renderFrameCalled)
		{
			// When max::renderFrame is called before init render thread
			// should not be created.
			BX_TRACE("Application called max::renderFrame directly, not creating render thread.");
			m_singleThreaded = true
				&& ~MAX_API_THREAD_MAGIC == s_threadIndex
				;
		}
		else
		{
			BX_TRACE("Creating rendering thread.");
			m_thread.init(renderThread, this, 0, "max - renderer backend thread");
			m_singleThreaded = false;
		}
#else
		BX_TRACE("Multithreaded renderer is disabled.");
		m_singleThreaded = true;
#endif // MAX_CONFIG_MULTITHREADED

		BX_TRACE("Running in %s-threaded mode", m_singleThreaded ? "single" : "multi");

		s_threadIndex = MAX_API_THREAD_MAGIC;

		for (uint32_t ii = 0; ii < BX_COUNTOF(m_viewRemap); ++ii)
		{
			m_viewRemap[ii] = ViewId(ii);
		}

		for (uint32_t ii = 0; ii < MAX_CONFIG_MAX_VIEWS; ++ii)
		{
			resetView(ViewId(ii) );
		}

		for (uint32_t ii = 0; ii < BX_COUNTOF(m_clearColor); ++ii)
		{
			m_clearColor[ii][0] = 0.0f;
			m_clearColor[ii][1] = 0.0f;
			m_clearColor[ii][2] = 0.0f;
			m_clearColor[ii][3] = 1.0f;
		}

		m_vertexLayoutRef.init();

		CommandBuffer& cmdbuf = getCommandBuffer(CommandBuffer::RendererInit);
		cmdbuf.write(_init);

		frameNoRenderWait();

		m_encoderHandle = bx::createHandleAlloc(g_allocator, _init.limits.maxEncoders);
		m_encoder       = (EncoderImpl*)bx::alignedAlloc(g_allocator, sizeof(EncoderImpl)*_init.limits.maxEncoders, BX_ALIGNOF(EncoderImpl) );
		m_encoderStats  = (EncoderStats*)bx::alloc(g_allocator, sizeof(EncoderStats)*_init.limits.maxEncoders);
		for (uint32_t ii = 0, num = _init.limits.maxEncoders; ii < num; ++ii)
		{
			BX_PLACEMENT_NEW(&m_encoder[ii], EncoderImpl);
		}

		uint16_t idx = m_encoderHandle->alloc();
		BX_ASSERT(0 == idx, "Internal encoder handle is not 0 (idx %d).", idx); BX_UNUSED(idx);
		m_encoder[0].begin(m_submit, 0);
		m_encoder0 = BX_ENABLED(MAX_CONFIG_ENCODER_API_ONLY)
			? NULL
			: reinterpret_cast<Encoder*>(&m_encoder[0])
			;

		// Make sure renderer init is called from render thread.
		// g_caps is initialized and available after this point.
		frame();

		if (!m_rendererInitialized)
		{
			getCommandBuffer(CommandBuffer::RendererShutdownEnd);
			frame();
			frame();
			m_vertexLayoutRef.shutdown(m_layoutHandle);
			m_submit->destroy();
#if MAX_CONFIG_MULTITHREADED
			m_render->destroy();
#endif // MAX_CONFIG_MULTITHREADED
			return false;
		}

		for (uint32_t ii = 0; ii < BX_COUNTOF(s_emulatedFormats); ++ii)
		{
			const uint32_t fmt = s_emulatedFormats[ii];
			g_caps.formats[fmt] |= 0 == (g_caps.formats[fmt] & MAX_CAPS_FORMAT_TEXTURE_2D  ) ? MAX_CAPS_FORMAT_TEXTURE_2D_EMULATED   : 0;
			g_caps.formats[fmt] |= 0 == (g_caps.formats[fmt] & MAX_CAPS_FORMAT_TEXTURE_3D  ) ? MAX_CAPS_FORMAT_TEXTURE_3D_EMULATED   : 0;
			g_caps.formats[fmt] |= 0 == (g_caps.formats[fmt] & MAX_CAPS_FORMAT_TEXTURE_CUBE) ? MAX_CAPS_FORMAT_TEXTURE_CUBE_EMULATED : 0;
		}

		for (uint32_t ii = 0; ii < TextureFormat::UnknownDepth; ++ii)
		{
			bool convertable = bimg::imageConvert(bimg::TextureFormat::BGRA8, bimg::TextureFormat::Enum(ii) );
			g_caps.formats[ii] |= 0 == (g_caps.formats[ii] & MAX_CAPS_FORMAT_TEXTURE_2D  ) && convertable ? MAX_CAPS_FORMAT_TEXTURE_2D_EMULATED   : 0;
			g_caps.formats[ii] |= 0 == (g_caps.formats[ii] & MAX_CAPS_FORMAT_TEXTURE_3D  ) && convertable ? MAX_CAPS_FORMAT_TEXTURE_3D_EMULATED   : 0;
			g_caps.formats[ii] |= 0 == (g_caps.formats[ii] & MAX_CAPS_FORMAT_TEXTURE_CUBE) && convertable ? MAX_CAPS_FORMAT_TEXTURE_CUBE_EMULATED : 0;
		}

		g_caps.rendererType = m_renderCtx->getRendererType();
		initAttribTypeSizeTable(g_caps.rendererType);

		g_caps.supported &= _init.capabilities;
		g_caps.supported |= 0
			| (BX_ENABLED(MAX_CONFIG_MULTITHREADED) && !m_singleThreaded ? MAX_CAPS_RENDERER_MULTITHREADED : 0)
			| (isGraphicsDebuggerPresent() ? MAX_CAPS_GRAPHICS_DEBUGGER : 0)
			;

		dumpCaps();

		m_textVideoMemBlitter.init(m_init.resolution.debugTextScale);
		m_clearQuad.init();

		m_submit->m_transientVb = createTransientVertexBuffer(_init.limits.transientVbSize);
		m_submit->m_transientIb = createTransientIndexBuffer(_init.limits.transientIbSize);
		frame();

		if (BX_ENABLED(MAX_CONFIG_MULTITHREADED) )
		{
			m_submit->m_transientVb = createTransientVertexBuffer(_init.limits.transientVbSize);
			m_submit->m_transientIb = createTransientIndexBuffer(_init.limits.transientIbSize);
			frame();
		}

		g_internalData.caps = getCaps();

		m_physicsCtx = physicsCreate(_init);

		resetInput();

		s_dds.init();
		s_dde.init( begin(false) );

		// @todo Move elsewhere? 
		m_entityQuery.alloc(MAX_CONFIG_MAX_ENTITIES);
		m_meshQuery.alloc(MAX_CONFIG_MAX_MESH_GROUPS);

		return true;
	}

	void Context::shutdown()
	{
		// @todo Move elsewhere? 
		m_entityQuery.free();
		m_meshQuery.free();

		s_dde.shutdown();
		s_dds.shutdown();

		physicsDestroy(m_physicsCtx);

		getCommandBuffer(CommandBuffer::RendererShutdownBegin);
		frame();

		destroyTransientVertexBuffer(m_submit->m_transientVb);
		destroyTransientIndexBuffer(m_submit->m_transientIb);
		m_textVideoMemBlitter.shutdown();
		m_clearQuad.shutdown();
		frame();

		if (BX_ENABLED(MAX_CONFIG_MULTITHREADED) )
		{
			destroyTransientVertexBuffer(m_submit->m_transientVb);
			destroyTransientIndexBuffer(m_submit->m_transientIb);
			frame();
		}

		frame(); // If any VertexLayouts needs to be destroyed.

		getCommandBuffer(CommandBuffer::RendererShutdownEnd);
		frame();

		m_encoder[0].end(true);
		m_encoderHandle->free(0);
		bx::destroyHandleAlloc(g_allocator, m_encoderHandle);
		m_encoderHandle = NULL;

		for (uint32_t ii = 0, num = g_caps.limits.maxEncoders; ii < num; ++ii)
		{
			m_encoder[ii].~EncoderImpl();
		}

		bx::alignedFree(g_allocator, m_encoder, BX_ALIGNOF(EncoderImpl) );
		bx::free(g_allocator, m_encoderStats);

		m_dynVertexBufferAllocator.compact();
		m_dynIndexBufferAllocator.compact();

		BX_ASSERT(
			  m_layoutHandle.getNumHandles() == m_vertexLayoutRef.m_vertexLayoutMap.getNumElements()
			, "VertexLayoutRef mismatch, num handles %d, handles in hash map %d."
			, m_layoutHandle.getNumHandles()
			, m_vertexLayoutRef.m_vertexLayoutMap.getNumElements()
			);

		m_vertexLayoutRef.shutdown(m_layoutHandle);

#if MAX_CONFIG_MULTITHREADED
		// Render thread shutdown sequence.
		renderSemWait(); // Wait for previous frame.
		apiSemPost();   // OK to set context to NULL.
		// s_ctx is NULL here.
		renderSemWait(); // In RenderFrame::Exiting state.

		if (m_thread.isRunning() )
		{
			m_thread.shutdown();
		}

		m_render->destroy();
#endif // MAX_CONFIG_MULTITHREADED

		bx::memSet(&g_internalData, 0, sizeof(InternalData) );
		s_ctx = NULL;

		m_submit->destroy();

		if (BX_ENABLED(MAX_CONFIG_DEBUG) )
		{
#define CHECK_HANDLE_LEAK(_name, _handleAlloc)                                        \
	BX_MACRO_BLOCK_BEGIN                                                              \
		if (0 != _handleAlloc.getNumHandles() )                                       \
		{                                                                             \
			BX_TRACE("LEAK: %s %d (max: %d)"                                          \
				, _name                                                               \
				, _handleAlloc.getNumHandles()                                        \
				, _handleAlloc.getMaxHandles()                                        \
				);                                                                    \
			for (uint16_t ii = 0, num = _handleAlloc.getNumHandles(); ii < num; ++ii) \
			{                                                                         \
				BX_TRACE("\t%3d: %4d", ii, _handleAlloc.getHandleAt(ii) );            \
			}                                                                         \
		}                                                                             \
	BX_MACRO_BLOCK_END

#define CHECK_HANDLE_LEAK_NAME(_name, _handleAlloc, _type, _ref)                      \
	BX_MACRO_BLOCK_BEGIN                                                              \
		if (0 != _handleAlloc.getNumHandles() )                                       \
		{                                                                             \
			BX_TRACE("LEAK: %s %d (max: %d)"                                          \
				, _name                                                               \
				, _handleAlloc.getNumHandles()                                        \
				, _handleAlloc.getMaxHandles()                                        \
				);                                                                    \
			for (uint16_t ii = 0, num = _handleAlloc.getNumHandles(); ii < num; ++ii) \
			{                                                                         \
				uint16_t idx = _handleAlloc.getHandleAt(ii);                          \
				const _type& ref = _ref[idx]; BX_UNUSED(ref);                         \
				BX_TRACE("\t%3d: %4d %s"                                              \
					, ii                                                              \
					, idx                                                             \
					, ref.m_name.getPtr()                                             \
					);                                                                \
			}                                                                         \
		}                                                                             \
	BX_MACRO_BLOCK_END

#define CHECK_HANDLE_LEAK_RC_NAME(_name, _handleAlloc, _type, _ref)                   \
	BX_MACRO_BLOCK_BEGIN                                                              \
		if (0 != _handleAlloc.getNumHandles() )                                       \
		{                                                                             \
			BX_TRACE("LEAK: %s %d (max: %d)"                                          \
				, _name                                                               \
				, _handleAlloc.getNumHandles()                                        \
				, _handleAlloc.getMaxHandles()                                        \
				);                                                                    \
			for (uint16_t ii = 0, num = _handleAlloc.getNumHandles(); ii < num; ++ii) \
			{                                                                         \
				uint16_t idx = _handleAlloc.getHandleAt(ii);                          \
				const _type& ref = _ref[idx]; BX_UNUSED(ref);                         \
				BX_TRACE("\t%3d: %4d %s (count %d)"                                   \
					, ii                                                              \
					, idx                                                             \
					, ref.m_name.getPtr()                                             \
					, ref.m_refCount                                                  \
					);                                                                \
			}                                                                         \
		}                                                                             \
	BX_MACRO_BLOCK_END

			CHECK_HANDLE_LEAK        ("DynamicIndexBufferHandle",  m_dynamicIndexBufferHandle                                  );
			CHECK_HANDLE_LEAK        ("DynamicVertexBufferHandle", m_dynamicVertexBufferHandle                                 );
			CHECK_HANDLE_LEAK_NAME   ("IndexBufferHandle",         m_indexBufferHandle,        IndexBuffer,    m_indexBuffers  );
			CHECK_HANDLE_LEAK        ("VertexLayoutHandle",        m_layoutHandle                                              );
			CHECK_HANDLE_LEAK_NAME   ("VertexBufferHandle",        m_vertexBufferHandle,       VertexBuffer,   m_vertexBuffers );
			CHECK_HANDLE_LEAK_RC_NAME("ShaderHandle",              m_shaderHandle,             ShaderRef,      m_shaderRef     );
			CHECK_HANDLE_LEAK        ("ProgramHandle",             m_programHandle                                             );
			CHECK_HANDLE_LEAK_RC_NAME("TextureHandle",             m_textureHandle,            TextureRef,     m_textureRef    );
			CHECK_HANDLE_LEAK_NAME   ("FrameBufferHandle",         m_frameBufferHandle,        FrameBufferRef, m_frameBufferRef);
			CHECK_HANDLE_LEAK_RC_NAME("UniformHandle",             m_uniformHandle,            UniformRef,     m_uniformRef    );
			CHECK_HANDLE_LEAK        ("BodyHandle",				   m_bodyHandle												   );
			CHECK_HANDLE_LEAK        ("OcclusionQueryHandle",      m_occlusionQueryHandle                                      );
			CHECK_HANDLE_LEAK        ("MaterialHandle",            m_materialHandle                                            );
			CHECK_HANDLE_LEAK        ("MeshHandle",                m_meshHandle                                                );
			CHECK_HANDLE_LEAK        ("ComponentHandle",           m_componentHandle                                           );
			CHECK_HANDLE_LEAK        ("EntityHandle",              m_entityHandle                                              );
#undef CHECK_HANDLE_LEAK
#undef CHECK_HANDLE_LEAK_NAME
		}
	}

	void Context::freeDynamicBuffers()
	{
		for (uint16_t ii = 0, num = m_numFreeDynamicIndexBufferHandles; ii < num; ++ii)
		{
			destroyDynamicIndexBufferInternal(m_freeDynamicIndexBufferHandle[ii]);
		}
		m_numFreeDynamicIndexBufferHandles = 0;

		for (uint16_t ii = 0, num = m_numFreeDynamicVertexBufferHandles; ii < num; ++ii)
		{
			destroyDynamicVertexBufferInternal(m_freeDynamicVertexBufferHandle[ii]);
		}
		m_numFreeDynamicVertexBufferHandles = 0;

		for (uint16_t ii = 0, num = m_numFreeOcclusionQueryHandles; ii < num; ++ii)
		{
			m_occlusionQueryHandle.free(m_freeOcclusionQueryHandle[ii].idx);
		}
		m_numFreeOcclusionQueryHandles = 0;

		for (uint16_t ii = 0, num = m_numFreeBodyHandles; ii < num; ++ii)
		{
			m_bodyHandle.free(m_freeBodyHandle[ii].idx);
		}
		m_numFreeBodyHandles = 0;
	}

	void Context::freeAllHandles(Frame* _frame)
	{
		for (uint16_t ii = 0, num = _frame->m_freeIndexBuffer.getNumQueued(); ii < num; ++ii)
		{
			m_indexBufferHandle.free(_frame->m_freeIndexBuffer.get(ii).idx);
		}

		for (uint16_t ii = 0, num = _frame->m_freeVertexBuffer.getNumQueued(); ii < num; ++ii)
		{
			destroyVertexBufferInternal(_frame->m_freeVertexBuffer.get(ii));
		}

		for (uint16_t ii = 0, num = _frame->m_freeVertexLayout.getNumQueued(); ii < num; ++ii)
		{
			m_layoutHandle.free(_frame->m_freeVertexLayout.get(ii).idx);
		}

		for (uint16_t ii = 0, num = _frame->m_freeShader.getNumQueued(); ii < num; ++ii)
		{
			m_shaderHandle.free(_frame->m_freeShader.get(ii).idx);
		}

		for (uint16_t ii = 0, num = _frame->m_freeProgram.getNumQueued(); ii < num; ++ii)
		{
			m_programHandle.free(_frame->m_freeProgram.get(ii).idx);
		}

		for (uint16_t ii = 0, num = _frame->m_freeTexture.getNumQueued(); ii < num; ++ii)
		{
			m_textureHandle.free(_frame->m_freeTexture.get(ii).idx);
		}

		for (uint16_t ii = 0, num = _frame->m_freeFrameBuffer.getNumQueued(); ii < num; ++ii)
		{
			m_frameBufferHandle.free(_frame->m_freeFrameBuffer.get(ii).idx);
		}

		for (uint16_t ii = 0, num = _frame->m_freeUniform.getNumQueued(); ii < num; ++ii)
		{
			m_uniformHandle.free(_frame->m_freeUniform.get(ii).idx);
		}

		for (uint16_t ii = 0, num = _frame->m_freeMaterial.getNumQueued(); ii < num; ++ii)
		{
			m_materialHandle.free(_frame->m_freeMaterial.get(ii).idx);
		}

		for (uint16_t ii = 0, num = _frame->m_freeMesh.getNumQueued(); ii < num; ++ii)
		{
			m_meshHandle.free(_frame->m_freeMesh.get(ii).idx);
		}

		for (uint16_t ii = 0, num = _frame->m_freeComponent.getNumQueued(); ii < num; ++ii)
		{
			m_componentHandle.free(_frame->m_freeComponent.get(ii).idx);
		}

		for (uint16_t ii = 0, num = _frame->m_freeEntity.getNumQueued(); ii < num; ++ii)
		{
			m_entityHandle.free(_frame->m_freeEntity.get(ii).idx);
		}
	}

	Encoder* Context::begin(bool _forThread)
	{
		EncoderImpl* encoder = &m_encoder[0];

#if MAX_CONFIG_MULTITHREADED
		if (_forThread || MAX_API_THREAD_MAGIC != s_threadIndex)
		{
			bx::MutexScope scopeLock(m_encoderApiLock);

			uint16_t idx = m_encoderHandle->alloc();
			if (kInvalidHandle == idx)
			{
				return NULL;
			}

			encoder = &m_encoder[idx];
			encoder->begin(m_submit, uint8_t(idx) );
		}
#else
		BX_UNUSED(_forThread);
#endif // MAX_CONFIG_MULTITHREADED

		return reinterpret_cast<Encoder*>(encoder);
	}

	void Context::end(Encoder* _encoder)
	{
#if MAX_CONFIG_MULTITHREADED
		EncoderImpl* encoder = reinterpret_cast<EncoderImpl*>(_encoder);
		if (encoder != &m_encoder[0])
		{
			encoder->end(true);
			m_encoderEndSem.post();
		}
#else
		BX_UNUSED(_encoder);
#endif // MAX_CONFIG_MULTITHREADED
	}

	uint32_t Context::frame(bool _capture)
	{
		m_encoder[0].end(true);

#if MAX_CONFIG_MULTITHREADED
		bx::MutexScope resourceApiScope(m_resourceApiLock);

		encoderApiWait();
		bx::MutexScope encoderApiScope(m_encoderApiLock);
#else
		encoderApiWait();
#endif // MAX_CONFIG_MULTITHREADED

		m_submit->m_capture = _capture;

		uint32_t frameNum = m_submit->m_frameNum;

		MAX_PROFILER_SCOPE("max/API thread frame", 0xff2040ff);
		// wait for render thread to finish
		renderSemWait();
		frameNoRenderWait();

		m_encoder[0].begin(m_submit, 0);

		return frameNum;
	}

	void Context::frameNoRenderWait()
	{
		swap();

		// release render thread
		apiSemPost();
	}

	void Context::swap()
	{
		freeDynamicBuffers();
		m_submit->m_resolution = m_init.resolution;
		m_init.resolution.reset &= ~MAX_RESET_INTERNAL_FORCE;
		m_submit->m_debug = m_debug;
		m_submit->m_perfStats.numViews = 0;

		bx::memCopy(m_submit->m_viewRemap, m_viewRemap, sizeof(m_viewRemap) );
		bx::memCopy(m_submit->m_view, m_view, sizeof(m_view) );

		if (m_colorPaletteDirty > 0)
		{
			--m_colorPaletteDirty;
			bx::memCopy(m_submit->m_colorPalette, m_clearColor, sizeof(m_clearColor) );
		}

		freeAllHandles(m_submit);
		m_submit->resetFreeHandles();

		m_submit->finish();

		bx::swap(m_render, m_submit);

		bx::memCopy(m_render->m_occlusion, m_submit->m_occlusion, sizeof(m_submit->m_occlusion) );

		if (!BX_ENABLED(MAX_CONFIG_MULTITHREADED)
		||  m_singleThreaded)
		{
			renderFrame();
		}

		uint32_t nextFrameNum = m_render->m_frameNum + 1;
		m_submit->start(nextFrameNum);

		bx::memSet(m_seq, 0, sizeof(m_seq) );

		m_submit->m_textVideoMem->resize(
			  m_render->m_textVideoMem->m_small
			, m_init.resolution.width
			, m_init.resolution.height
			);

		int64_t now = bx::getHPCounter();
		m_submit->m_perfStats.cpuTimeFrame = now - m_frameTimeLast;
		m_frameTimeLast = now;
	}

	void Context::flip()
	{
		if (m_rendererInitialized
		&& !m_flipped)
		{
			m_renderCtx->flip();
			m_flipped = true;

			if (m_renderCtx->isDeviceRemoved() )
			{
				// Something horribly went wrong, fallback to noop renderer.
				rendererDestroy(m_renderCtx);

				Init init;
				init.rendererType = RendererType::Noop;
				m_renderCtx = rendererCreate(init);
				g_caps.rendererType = RendererType::Noop;
			}
		}
	}

#if BX_PLATFORM_OSX || BX_PLATFORM_IOS || BX_PLATFORM_VISIONOS
	struct NSAutoreleasePoolScope
	{
		NSAutoreleasePoolScope()
		{
			id obj = class_createInstance(objc_getClass("NSAutoreleasePool"), 0);
			typedef id(*objc_msgSend_init)(void*, SEL);
			pool = ((objc_msgSend_init)objc_msgSend)(obj, sel_getUid("init") );
		}

		~NSAutoreleasePoolScope()
		{
			typedef void(*objc_msgSend_release)(void*, SEL);
			((objc_msgSend_release)objc_msgSend)(pool, sel_getUid("release") );
		}

		id pool;
	};
#endif // BX_PLATFORM_OSX

	RenderFrame::Enum Context::renderFrame(int32_t _msecs)
	{
		MAX_PROFILER_SCOPE("max::renderFrame", 0xff2040ff);

#if BX_PLATFORM_OSX || BX_PLATFORM_IOS || BX_PLATFORM_VISIONOS
		NSAutoreleasePoolScope pool;
#endif // BX_PLATFORM_OSX

		if (!m_flipAfterRender)
		{
			MAX_PROFILER_SCOPE("max/flip", 0xff2040ff);
			flip();
		}

		if (apiSemWait(_msecs) )
		{
			{
				MAX_PROFILER_SCOPE("max/Exec commands pre", 0xff2040ff);
				rendererExecCommands(m_render->m_cmdPre);
			}

			if (m_rendererInitialized)
			{
				{
					MAX_PROFILER_SCOPE("max/Render submit", 0xff2040ff);
					m_renderCtx->submit(m_render, m_clearQuad, m_textVideoMemBlitter);
					m_flipped = false;
				}

				{
					MAX_PROFILER_SCOPE("max/Screenshot", 0xff2040ff);
					for (uint8_t ii = 0, num = m_render->m_numScreenShots; ii < num; ++ii)
					{
						const ScreenShot& screenShot = m_render->m_screenShot[ii];
						m_renderCtx->requestScreenShot(screenShot.handle, screenShot.filePath.getCPtr() );
					}
				}
			}

			{
				MAX_PROFILER_SCOPE("max/Exec commands post", 0xff2040ff);
				rendererExecCommands(m_render->m_cmdPost);
			}

			renderSemPost();

			if (m_flipAfterRender)
			{
				MAX_PROFILER_SCOPE("max/flip", 0xff2040ff);
				flip();
			}
		}
		else
		{
			return RenderFrame::Timeout;
		}

		return m_exit
			? RenderFrame::Exiting
			: RenderFrame::Render
			;
	}

	void rendererUpdateUniforms(RendererContextI* _renderCtx, UniformBuffer* _uniformBuffer, uint32_t _begin, uint32_t _end)
	{
		_uniformBuffer->reset(_begin);
		while (_uniformBuffer->getPos() < _end)
		{
			uint32_t opcode = _uniformBuffer->read();

			if (UniformType::End == opcode)
			{
				break;
			}

			UniformType::Enum type;
			uint16_t loc;
			uint16_t num;
			uint16_t copy;
			UniformBuffer::decodeOpcode(opcode, type, loc, num, copy);

			const uint32_t size = g_uniformTypeSize[type]*num;
			const char* data = _uniformBuffer->read(size);

			if (UniformType::Count > type)
			{
				if (copy)
				{
					_renderCtx->updateUniform(loc, data, size);
				}
				else
				{
					_renderCtx->updateUniform(loc, *(const char**)(data), size);
				}
			}
			else
			{
				_renderCtx->setMarker(data, uint16_t(size)-1);
			}
		}
	}

	void Context::flushTextureUpdateBatch(CommandBuffer& _cmdbuf)
	{
		MAX_PROFILER_SCOPE("flushTextureUpdateBatch", 0xff2040ff);
		if (m_textureUpdateBatch.sort() )
		{
			const uint32_t pos = _cmdbuf.m_pos;

			uint32_t currentKey = UINT32_MAX;

			for (uint32_t ii = 0, num = m_textureUpdateBatch.m_num; ii < num; ++ii)
			{
				_cmdbuf.m_pos = m_textureUpdateBatch.m_values[ii];

				TextureHandle handle;
				_cmdbuf.read(handle);

				uint8_t side;
				_cmdbuf.read(side);

				uint8_t mip;
				_cmdbuf.read(mip);

				Rect rect;
				_cmdbuf.read(rect);

				uint16_t zz;
				_cmdbuf.read(zz);

				uint16_t depth;
				_cmdbuf.read(depth);

				uint16_t pitch;
				_cmdbuf.read(pitch);

				const Memory* mem;
				_cmdbuf.read(mem);

				uint32_t key = m_textureUpdateBatch.m_keys[ii];
				if (key != currentKey)
				{
					if (currentKey != UINT32_MAX)
					{
						m_renderCtx->updateTextureEnd();
					}
					currentKey = key;
					m_renderCtx->updateTextureBegin(handle, side, mip);
				}

				m_renderCtx->updateTexture(handle, side, mip, rect, zz, depth, pitch, mem);

				release(mem);
			}

			if (currentKey != UINT32_MAX)
			{
				m_renderCtx->updateTextureEnd();
			}

			m_textureUpdateBatch.reset();

			_cmdbuf.m_pos = pos;
		}
	}

	typedef RendererContextI* (*RendererCreateFn)(const Init& _init);
	typedef void (*RendererDestroyFn)();

#define MAX_RENDERER_CONTEXT(_namespace)                           \
	namespace _namespace                                            \
	{                                                               \
		extern RendererContextI* rendererCreate(const Init& _init); \
		extern void rendererDestroy();                              \
	}

	MAX_RENDERER_CONTEXT(noop);
	MAX_RENDERER_CONTEXT(agc);
	MAX_RENDERER_CONTEXT(d3d11);
	MAX_RENDERER_CONTEXT(d3d12);
	MAX_RENDERER_CONTEXT(gnm);
	MAX_RENDERER_CONTEXT(mtl);
	MAX_RENDERER_CONTEXT(nvn);
	MAX_RENDERER_CONTEXT(gl);
	MAX_RENDERER_CONTEXT(vk);

#undef MAX_RENDERER_CONTEXT

	struct RendererCreator
	{
		RendererCreateFn  createFn;
		RendererDestroyFn destroyFn;
		const char* name;
		bool supported;
	};

	static RendererCreator s_rendererCreator[] =
	{
		{ noop::rendererCreate,   noop::rendererDestroy,   MAX_RENDERER_NOOP_NAME,       true                              }, // Noop
		{ agc::rendererCreate,    agc::rendererDestroy,    MAX_RENDERER_AGC_NAME,        !!MAX_CONFIG_RENDERER_AGC        }, // GNM
		{ d3d11::rendererCreate,  d3d11::rendererDestroy,  MAX_RENDERER_DIRECT3D11_NAME, !!MAX_CONFIG_RENDERER_DIRECT3D11 }, // Direct3D11
		{ d3d12::rendererCreate,  d3d12::rendererDestroy,  MAX_RENDERER_DIRECT3D12_NAME, !!MAX_CONFIG_RENDERER_DIRECT3D12 }, // Direct3D12
		{ gnm::rendererCreate,    gnm::rendererDestroy,    MAX_RENDERER_GNM_NAME,        !!MAX_CONFIG_RENDERER_GNM        }, // GNM
#if BX_PLATFORM_OSX || BX_PLATFORM_IOS || BX_PLATFORM_VISIONOS
		{ mtl::rendererCreate,    mtl::rendererDestroy,    MAX_RENDERER_METAL_NAME,      !!MAX_CONFIG_RENDERER_METAL      }, // Metal
#else
		{ noop::rendererCreate,   noop::rendererDestroy,   MAX_RENDERER_NOOP_NAME,       false                             }, // Noop
#endif // BX_PLATFORM_OSX || BX_PLATFORM_IOS || BX_PLATFORM_VISIONOS
		{ nvn::rendererCreate,    nvn::rendererDestroy,    MAX_RENDERER_NVN_NAME,        !!MAX_CONFIG_RENDERER_NVN        }, // NVN
		{ gl::rendererCreate,     gl::rendererDestroy,     MAX_RENDERER_OPENGL_NAME,     !!MAX_CONFIG_RENDERER_OPENGLES   }, // OpenGLES
		{ gl::rendererCreate,     gl::rendererDestroy,     MAX_RENDERER_OPENGL_NAME,     !!MAX_CONFIG_RENDERER_OPENGL     }, // OpenGL
		{ vk::rendererCreate,     vk::rendererDestroy,     MAX_RENDERER_VULKAN_NAME,     !!MAX_CONFIG_RENDERER_VULKAN     }, // Vulkan
	};
	BX_STATIC_ASSERT(BX_COUNTOF(s_rendererCreator) == RendererType::Count);

	bool windowsVersionIs(Condition::Enum _op, uint32_t _version, uint32_t _build)
	{
#if BX_PLATFORM_WINDOWS
		RTL_OSVERSIONINFOW ovi;
		bx::memSet(&ovi, 0 , sizeof(ovi));
		ovi.dwOSVersionInfoSize = sizeof(ovi);
		const HMODULE hMod = GetModuleHandleW(L"ntdll.dll");
		if (NULL != hMod)
		{
			FARPROC (WINAPI* rtlGetVersionPtr) (PRTL_OSVERSIONINFOW) = reinterpret_cast<FARPROC (WINAPI*)(PRTL_OSVERSIONINFOW)>(bx::dlsym(hMod, "RtlGetVersion"));
			if (NULL != rtlGetVersionPtr)
			{
				rtlGetVersionPtr(&ovi);
				if (ovi.dwMajorVersion == 0)
				{
					return false;
				}
				ovi.dwBuildNumber =  UINT32_MAX == _build ? UINT32_MAX : ovi.dwBuildNumber;
			}
		}
		// _WIN32_WINNT_WIN10   0x0A00
		// _WIN32_WINNT_WINBLUE 0x0603
		// _WIN32_WINNT_WIN8    0x0602
		// _WIN32_WINNT_WIN7    0x0601
		// _WIN32_WINNT_VISTA   0x0600
		const DWORD cMajorVersion = HIBYTE(_version);
		const DWORD cMinorVersion = LOBYTE(_version);
		switch (_op)
		{
			case Condition::LessEqual:
				return (ovi.dwMajorVersion < cMajorVersion || (ovi.dwMajorVersion == cMajorVersion && ovi.dwMinorVersion <= cMinorVersion)) && ovi.dwBuildNumber <= _build;
			case Condition::GreaterEqual:
				return (ovi.dwMajorVersion > cMajorVersion || (ovi.dwMajorVersion == cMajorVersion && ovi.dwMinorVersion >= cMinorVersion)) && ovi.dwBuildNumber >= _build;
			default:
				return false;
		}
#else
		BX_UNUSED(_op, _version, _build);
		return false;
#endif // BX_PLATFORM_WINDOWS
	}

	RendererContextI* rendererCreate(const Init& _init)
	{
		int32_t scores[RendererType::Count];
		uint32_t numScores = 0;

		for (uint32_t ii = 0; ii < RendererType::Count; ++ii)
		{
			RendererType::Enum renderer = RendererType::Enum(ii);
			if (s_rendererCreator[ii].supported)
			{
				int32_t score = 0;
				if (_init.rendererType == renderer)
				{
					score += 1000;
				}

				score += RendererType::Noop != renderer ? 1 : 0;

				if (BX_ENABLED(BX_PLATFORM_WINDOWS) )
				{
					if (windowsVersionIs(Condition::GreaterEqual, 0x0602) )
					{
						score += RendererType::Direct3D11 == renderer ? 20 : 0;
						score += RendererType::Direct3D12 == renderer ? 10 : 0;
					}
					else if (windowsVersionIs(Condition::GreaterEqual, 0x0601) )
					{
						score += RendererType::Direct3D11 == renderer ?   20 : 0;
						score += RendererType::Direct3D12 == renderer ? -100 : 0;
					}
					else
					{
						score += RendererType::Direct3D12 == renderer ? -100 : 0;
					}
				}
				else if (BX_ENABLED(BX_PLATFORM_LINUX) )
				{
					score += RendererType::Vulkan     == renderer ? 50 : 0;
					score += RendererType::OpenGL     == renderer ? 40 : 0;
					score += RendererType::OpenGLES   == renderer ? 30 : 0;
					score += RendererType::Direct3D12 == renderer ? 20 : 0;
					score += RendererType::Direct3D11 == renderer ? 10 : 0;
				}
				else if (BX_ENABLED(BX_PLATFORM_OSX) )
				{
					score += RendererType::Metal    == renderer ? 20 : 0;
					score += RendererType::Vulkan   == renderer ? 10 : 0;
				}
				else if (BX_ENABLED(BX_PLATFORM_IOS) || BX_ENABLED(BX_PLATFORM_VISIONOS))
				{
					score += RendererType::Metal    == renderer ? 20 : 0;
				}
				else if (BX_ENABLED(0
					 ||  BX_PLATFORM_ANDROID
					 ||  BX_PLATFORM_EMSCRIPTEN
					 ||  BX_PLATFORM_RPI
					 ) )
				{
					score += RendererType::OpenGLES == renderer ? 20 : 0;
				}
				else if (BX_ENABLED(BX_PLATFORM_PS4) )
				{
					score += RendererType::Gnm      == renderer ? 20 : 0;
				}
				else if (BX_ENABLED(0
					 ||  BX_PLATFORM_XBOXONE
					 ||  BX_PLATFORM_WINRT
					 ) )
				{
					score += RendererType::Direct3D12 == renderer ? 20 : 0;
					score += RendererType::Direct3D11 == renderer ? 10 : 0;
				}

				scores[numScores++] = (score<<8) | uint8_t(renderer);
			}
		}

		bx::quickSort(scores, numScores, bx::compareDescending<int32_t>);

		RendererContextI* renderCtx = NULL;
		for (uint32_t ii = 0; ii < numScores; ++ii)
		{
			RendererType::Enum renderer = RendererType::Enum(scores[ii] & 0xff);
			renderCtx = s_rendererCreator[renderer].createFn(_init);
			if (NULL != renderCtx)
			{
				break;
			}

			s_rendererCreator[renderer].supported = false;
		}

		return renderCtx;
	}

	void rendererDestroy(RendererContextI* _renderCtx)
	{
		if (NULL != _renderCtx)
		{
			s_rendererCreator[_renderCtx->getRendererType()].destroyFn();
		}
	}

	void Context::rendererExecCommands(CommandBuffer& _cmdbuf)
	{
		_cmdbuf.reset();

		bool end = false;

		if (NULL == m_renderCtx)
		{
			uint8_t command;
			_cmdbuf.read(command);

			switch (command)
			{
			case CommandBuffer::RendererShutdownEnd:
				m_exit = true;
				return;

			case CommandBuffer::End:
				return;

			default:
				{
					BX_ASSERT(CommandBuffer::RendererInit == command
						, "RendererInit must be the first command in command buffer before initialization. Unexpected command %d?"
						, command
						);
					BX_ASSERT(!m_rendererInitialized, "This shouldn't happen! Bad synchronization?");

					Init init;
					_cmdbuf.read(init);

					m_renderCtx = rendererCreate(init);

					m_rendererInitialized = NULL != m_renderCtx;

					if (!m_rendererInitialized)
					{
						_cmdbuf.read(command);
						BX_ASSERT(CommandBuffer::End == command, "Unexpected command %d?"
							, command
							);
						return;
					}
				}
				break;
			}
		}

		do
		{
			uint8_t command;
			_cmdbuf.read(command);

			switch (command)
			{
			case CommandBuffer::RendererShutdownBegin:
				{
					BX_ASSERT(m_rendererInitialized, "This shouldn't happen! Bad synchronization?");
					m_rendererInitialized = false;
				}
				break;

			case CommandBuffer::RendererShutdownEnd:
				{
					BX_ASSERT(!m_rendererInitialized && !m_exit, "This shouldn't happen! Bad synchronization?");

					rendererDestroy(m_renderCtx);
					m_renderCtx = NULL;

					m_exit = true;
				}
				[[fallthrough]];

			case CommandBuffer::End:
				end = true;
				break;

			case CommandBuffer::CreateIndexBuffer:
				{
					MAX_PROFILER_SCOPE("CreateIndexBuffer", 0xff2040ff);

					IndexBufferHandle handle;
					_cmdbuf.read(handle);

					const Memory* mem;
					_cmdbuf.read(mem);

					uint16_t flags;
					_cmdbuf.read(flags);

					m_renderCtx->createIndexBuffer(handle, mem, flags);

					release(mem);
				}
				break;

			case CommandBuffer::DestroyIndexBuffer:
				{
					MAX_PROFILER_SCOPE("DestroyIndexBuffer", 0xff2040ff);

					IndexBufferHandle handle;
					_cmdbuf.read(handle);

					m_renderCtx->destroyIndexBuffer(handle);
				}
				break;

			case CommandBuffer::CreateVertexLayout:
				{
					MAX_PROFILER_SCOPE("CreateVertexLayout", 0xff2040ff);

					VertexLayoutHandle handle;
					_cmdbuf.read(handle);

					VertexLayout layout;
					_cmdbuf.read(layout);

					m_renderCtx->createVertexLayout(handle, layout);
				}
				break;

			case CommandBuffer::DestroyVertexLayout:
				{
					MAX_PROFILER_SCOPE("DestroyVertexLayout", 0xff2040ff);

					VertexLayoutHandle handle;
					_cmdbuf.read(handle);

					m_renderCtx->destroyVertexLayout(handle);
				}
				break;

			case CommandBuffer::CreateVertexBuffer:
				{
					MAX_PROFILER_SCOPE("CreateVertexBuffer", 0xff2040ff);

					VertexBufferHandle handle;
					_cmdbuf.read(handle);

					const Memory* mem;
					_cmdbuf.read(mem);

					VertexLayoutHandle layoutHandle;
					_cmdbuf.read(layoutHandle);

					uint16_t flags;
					_cmdbuf.read(flags);

					m_renderCtx->createVertexBuffer(handle, mem, layoutHandle, flags);

					release(mem);
				}
				break;

			case CommandBuffer::DestroyVertexBuffer:
				{
					MAX_PROFILER_SCOPE("DestroyVertexBuffer", 0xff2040ff);

					VertexBufferHandle handle;
					_cmdbuf.read(handle);

					m_renderCtx->destroyVertexBuffer(handle);
				}
				break;

			case CommandBuffer::CreateDynamicIndexBuffer:
				{
					MAX_PROFILER_SCOPE("CreateDynamicIndexBuffer", 0xff2040ff);

					IndexBufferHandle handle;
					_cmdbuf.read(handle);

					uint32_t size;
					_cmdbuf.read(size);

					uint16_t flags;
					_cmdbuf.read(flags);

					m_renderCtx->createDynamicIndexBuffer(handle, size, flags);
				}
				break;

			case CommandBuffer::UpdateDynamicIndexBuffer:
				{
					MAX_PROFILER_SCOPE("UpdateDynamicIndexBuffer", 0xff2040ff);

					IndexBufferHandle handle;
					_cmdbuf.read(handle);

					uint32_t offset;
					_cmdbuf.read(offset);

					uint32_t size;
					_cmdbuf.read(size);

					const Memory* mem;
					_cmdbuf.read(mem);

					m_renderCtx->updateDynamicIndexBuffer(handle, offset, size, mem);

					release(mem);
				}
				break;

			case CommandBuffer::DestroyDynamicIndexBuffer:
				{
					MAX_PROFILER_SCOPE("DestroyDynamicIndexBuffer", 0xff2040ff);

					IndexBufferHandle handle;
					_cmdbuf.read(handle);

					m_renderCtx->destroyDynamicIndexBuffer(handle);
				}
				break;

			case CommandBuffer::CreateDynamicVertexBuffer:
				{
					MAX_PROFILER_SCOPE("CreateDynamicVertexBuffer", 0xff2040ff);

					VertexBufferHandle handle;
					_cmdbuf.read(handle);

					uint32_t size;
					_cmdbuf.read(size);

					uint16_t flags;
					_cmdbuf.read(flags);

					m_renderCtx->createDynamicVertexBuffer(handle, size, flags);
				}
				break;

			case CommandBuffer::UpdateDynamicVertexBuffer:
				{
					MAX_PROFILER_SCOPE("UpdateDynamicVertexBuffer", 0xff2040ff);

					VertexBufferHandle handle;
					_cmdbuf.read(handle);

					uint32_t offset;
					_cmdbuf.read(offset);

					uint32_t size;
					_cmdbuf.read(size);

					const Memory* mem;
					_cmdbuf.read(mem);

					m_renderCtx->updateDynamicVertexBuffer(handle, offset, size, mem);

					release(mem);
				}
				break;

			case CommandBuffer::DestroyDynamicVertexBuffer:
				{
					MAX_PROFILER_SCOPE("DestroyDynamicVertexBuffer", 0xff2040ff);

					VertexBufferHandle handle;
					_cmdbuf.read(handle);

					m_renderCtx->destroyDynamicVertexBuffer(handle);
				}
				break;

			case CommandBuffer::CreateShader:
				{
					MAX_PROFILER_SCOPE("CreateShader", 0xff2040ff);

					ShaderHandle handle;
					_cmdbuf.read(handle);

					const Memory* mem;
					_cmdbuf.read(mem);

					m_renderCtx->createShader(handle, mem);

					release(mem);
				}
				break;

			case CommandBuffer::DestroyShader:
				{
					MAX_PROFILER_SCOPE("DestroyShader", 0xff2040ff);

					ShaderHandle handle;
					_cmdbuf.read(handle);

					m_renderCtx->destroyShader(handle);
				}
				break;

			case CommandBuffer::CreateProgram:
				{
					MAX_PROFILER_SCOPE("CreateProgram", 0xff2040ff);

					ProgramHandle handle;
					_cmdbuf.read(handle);

					ShaderHandle vsh;
					_cmdbuf.read(vsh);

					ShaderHandle fsh;
					_cmdbuf.read(fsh);

					m_renderCtx->createProgram(handle, vsh, fsh);
				}
				break;

			case CommandBuffer::DestroyProgram:
				{
					MAX_PROFILER_SCOPE("DestroyProgram", 0xff2040ff);

					ProgramHandle handle;
					_cmdbuf.read(handle);

					m_renderCtx->destroyProgram(handle);
				}
				break;

			case CommandBuffer::CreateTexture:
				{
					MAX_PROFILER_SCOPE("CreateTexture", 0xff2040ff);

					TextureHandle handle;
					_cmdbuf.read(handle);

					const Memory* mem;
					_cmdbuf.read(mem);

					uint64_t flags;
					_cmdbuf.read(flags);

					uint8_t skip;
					_cmdbuf.read(skip);

					void* ptr = m_renderCtx->createTexture(handle, mem, flags, skip);
					if (NULL != ptr)
					{
						setDirectAccessPtr(handle, ptr);
					}

					bx::MemoryReader reader(mem->data, mem->size);
					bx::Error err;

					uint32_t magic;
					bx::read(&reader, magic, &err);

					if (MAX_CHUNK_MAGIC_TEX == magic)
					{
						TextureCreate tc;
						bx::read(&reader, tc, &err);

						if (NULL != tc.m_mem)
						{
							release(tc.m_mem);
						}
					}

					release(mem);
				}
				break;

			case CommandBuffer::UpdateTexture:
				{
					MAX_PROFILER_SCOPE("UpdateTexture", 0xff2040ff);

					if (m_textureUpdateBatch.isFull() )
					{
						flushTextureUpdateBatch(_cmdbuf);
					}

					uint32_t value = _cmdbuf.m_pos;

					TextureHandle handle;
					_cmdbuf.read(handle);

					uint8_t side;
					_cmdbuf.read(side);

					uint8_t mip;
					_cmdbuf.read(mip);

					_cmdbuf.skip<Rect>();
					_cmdbuf.skip<uint16_t>();
					_cmdbuf.skip<uint16_t>();
					_cmdbuf.skip<uint16_t>();
					_cmdbuf.skip<Memory*>();

					uint32_t key = (handle.idx<<16)
						| (side<<8)
						| mip
						;

					m_textureUpdateBatch.add(key, value);
				}
				break;

			case CommandBuffer::ReadTexture:
				{
					MAX_PROFILER_SCOPE("ReadTexture", 0xff2040ff);

					TextureHandle handle;
					_cmdbuf.read(handle);

					void* data;
					_cmdbuf.read(data);

					uint8_t mip;
					_cmdbuf.read(mip);

					m_renderCtx->readTexture(handle, data, mip);
				}
				break;

			case CommandBuffer::ResizeTexture:
				{
					MAX_PROFILER_SCOPE("ResizeTexture", 0xff2040ff);

					TextureHandle handle;
					_cmdbuf.read(handle);

					uint16_t width;
					_cmdbuf.read(width);

					uint16_t height;
					_cmdbuf.read(height);

					uint8_t numMips;
					_cmdbuf.read(numMips);

					uint16_t numLayers;
					_cmdbuf.read(numLayers);

					m_renderCtx->resizeTexture(handle, width, height, numMips, numLayers);
				}
				break;

			case CommandBuffer::DestroyTexture:
				{
					MAX_PROFILER_SCOPE("DestroyTexture", 0xff2040ff);

					TextureHandle handle;
					_cmdbuf.read(handle);

					m_renderCtx->destroyTexture(handle);
				}
				break;

			case CommandBuffer::CreateFrameBuffer:
				{
					MAX_PROFILER_SCOPE("CreateFrameBuffer", 0xff2040ff);

					FrameBufferHandle handle;
					_cmdbuf.read(handle);

					bool window;
					_cmdbuf.read(window);

					if (window)
					{
						void* nwh;
						_cmdbuf.read(nwh);

						uint16_t width;
						_cmdbuf.read(width);

						uint16_t height;
						_cmdbuf.read(height);

						TextureFormat::Enum format;
						_cmdbuf.read(format);

						TextureFormat::Enum depthFormat;
						_cmdbuf.read(depthFormat);

						m_renderCtx->createFrameBuffer(handle, nwh, width, height, format, depthFormat);
					}
					else
					{
						uint8_t num;
						_cmdbuf.read(num);

						Attachment attachment[MAX_CONFIG_MAX_FRAME_BUFFER_ATTACHMENTS];
						_cmdbuf.read(attachment, sizeof(Attachment) * num);

						m_renderCtx->createFrameBuffer(handle, num, attachment);
					}
				}
				break;

			case CommandBuffer::DestroyFrameBuffer:
				{
					MAX_PROFILER_SCOPE("DestroyFrameBuffer", 0xff2040ff);

					FrameBufferHandle handle;
					_cmdbuf.read(handle);

					m_renderCtx->destroyFrameBuffer(handle);
				}
				break;

			case CommandBuffer::CreateUniform:
				{
					MAX_PROFILER_SCOPE("CreateUniform", 0xff2040ff);

					UniformHandle handle;
					_cmdbuf.read(handle);

					UniformType::Enum type;
					_cmdbuf.read(type);

					uint16_t num;
					_cmdbuf.read(num);

					uint8_t len;
					_cmdbuf.read(len);

					const char* name = (const char*)_cmdbuf.skip(len);

					m_renderCtx->createUniform(handle, type, num, name);
				}
				break;

			case CommandBuffer::DestroyUniform:
				{
					MAX_PROFILER_SCOPE("DestroyUniform", 0xff2040ff);

					UniformHandle handle;
					_cmdbuf.read(handle);

					m_renderCtx->destroyUniform(handle);
				}
				break;

			case CommandBuffer::UpdateViewName:
				{
					MAX_PROFILER_SCOPE("UpdateViewName", 0xff2040ff);

					ViewId id;
					_cmdbuf.read(id);

					uint16_t len;
					_cmdbuf.read(len);

					const char* name = (const char*)_cmdbuf.skip(len);

					m_renderCtx->updateViewName(id, name);
				}
				break;

			case CommandBuffer::InvalidateOcclusionQuery:
				{
					MAX_PROFILER_SCOPE("InvalidateOcclusionQuery", 0xff2040ff);

					OcclusionQueryHandle handle;
					_cmdbuf.read(handle);

					m_renderCtx->invalidateOcclusionQuery(handle);
				}
				break;

			case CommandBuffer::SetName:
				{
					MAX_PROFILER_SCOPE("SetName", 0xff2040ff);

					Handle handle;
					_cmdbuf.read(handle);

					uint16_t len;
					_cmdbuf.read(len);

					const char* name = (const char*)_cmdbuf.skip(len);

					m_renderCtx->setName(handle, name, len-1);
				}
				break;

			default:
				BX_ASSERT(false, "Invalid command: %d", command);
				break;
			}
		} while (!end);

		flushTextureUpdateBatch(_cmdbuf);
	}

	typedef PhysicsContextI* (*PhysicsCreateFn)(const Init& _init);
	typedef void (*PhysicsDestroyFn)();

#define MAX_PHYSICS_CONTEXT(_namespace)                           \
	namespace _namespace                                            \
	{                                                               \
		extern PhysicsContextI* physicsCreate(const Init& _init); \
		extern void physicsDestroy();                              \
	}

	MAX_PHYSICS_CONTEXT(noop);
	MAX_PHYSICS_CONTEXT(jolt);

#undef MAX_PHYSICS_CONTEXT

	struct PhysicsCreator
	{
		PhysicsCreateFn  createFn;
		PhysicsDestroyFn destroyFn;
		const char* name;
		bool supported;
	};

	static PhysicsCreator s_physicsCreator[] =
	{
		{ noop::physicsCreate, noop::physicsDestroy, MAX_PHYSICS_NOOP_NAME, true }, // Noop
		{ jolt::physicsCreate, jolt::physicsDestroy, MAX_PHYSICS_JOLT_NAME, true }, // Jolt
	};
	BX_STATIC_ASSERT(BX_COUNTOF(s_physicsCreator) == PhysicsType::Count);

	PhysicsContextI* physicsCreate(const Init& _init)
	{
		PhysicsType::Enum physics = _init.physicsType;

		if (physics == PhysicsType::Count)
		{
			physics = PhysicsType::Jolt;
		}

		PhysicsContextI* physicsCtx = s_physicsCreator[physics].createFn(_init);
		return physicsCtx;
	}

	void physicsDestroy(PhysicsContextI* _physicsCtx)
	{
		if (NULL != _physicsCtx)
		{
			s_physicsCreator[_physicsCtx->getPhysicsType()].destroyFn();
		}
	}

	uint32_t weldVertices(void* _output, const VertexLayout& _layout, const void* _data, uint32_t _num, bool _index32, float _epsilon)
	{
		return weldVertices(_output, _layout, _data, _num, _index32, _epsilon, g_allocator);
	}

	uint32_t topologyConvert(TopologyConvert::Enum _conversion, void* _dst, uint32_t _dstSize, const void* _indices, uint32_t _numIndices, bool _index32)
	{
		return topologyConvert(_conversion, _dst, _dstSize, _indices, _numIndices, _index32, g_allocator);
	}

	void topologySortTriList(TopologySort::Enum _sort, void* _dst, uint32_t _dstSize, const float _dir[3], const float _pos[3], const void* _vertices, uint32_t _stride, const void* _indices, uint32_t _numIndices, bool _index32)
	{
		topologySortTriList(_sort, _dst, _dstSize, _dir, _pos, _vertices, _stride, _indices, _numIndices, _index32, g_allocator);
	}

	uint8_t getSupportedRenderers(uint8_t _max, RendererType::Enum* _enum)
	{
		_enum = _max == 0 ? NULL : _enum;

		uint8_t num = 0;
		for (uint8_t ii = 0; ii < RendererType::Count; ++ii)
		{
			if ( (RendererType::Direct3D11 == ii || RendererType::Direct3D12 == ii)
			&&  windowsVersionIs(Condition::LessEqual, 0x0502) )
			{
				continue;
			}

			if (NULL == _enum)
			{
				num++;
			}
			else
			{
				if (num < _max
				&&  s_rendererCreator[ii].supported)
				{
					_enum[num++] = RendererType::Enum(ii);
				}
			}
		}

		return num;
	}

	const char* getRendererName(RendererType::Enum _type)
	{
		BX_ASSERT(_type < RendererType::Count, "Invalid renderer type %d.", _type);
		return s_rendererCreator[_type].name;
	}

	PlatformData::PlatformData()
		: ndt(NULL)
		, nwh(NULL)
		, context(NULL)
		, backBuffer(NULL)
		, backBufferDS(NULL)
		, type(NativeWindowHandleType::Default)
	{
	}

	Resolution::Resolution()
		: format(TextureFormat::RGBA8)
		, width(1280)
		, height(720)
		, reset(MAX_RESET_NONE)
		, numBackBuffers(2)
		, maxFrameLatency(0)
		, debugTextScale(0)
	{
	}

	Init::Limits::Limits()
		: maxEncoders(MAX_CONFIG_DEFAULT_MAX_ENCODERS)
		, minResourceCbSize(MAX_CONFIG_MIN_RESOURCE_COMMAND_BUFFER_SIZE)
		, transientVbSize(MAX_CONFIG_TRANSIENT_VERTEX_BUFFER_SIZE)
		, transientIbSize(MAX_CONFIG_TRANSIENT_INDEX_BUFFER_SIZE)
	{
	}

	Init::Init()
		: rendererType(RendererType::Count)
		, physicsType(PhysicsType::Count)
		, vendorId(MAX_PCI_ID_NONE)
		, deviceId(0)
		, capabilities(UINT64_MAX)
		, debug(BX_ENABLED(MAX_CONFIG_DEBUG) )
		, profile(BX_ENABLED(MAX_CONFIG_DEBUG_ANNOTATION) )
		, callback(NULL)
		, allocator(NULL)
	{
	}

	void Attachment::init(TextureHandle _handle, Access::Enum _access, uint16_t _layer, uint16_t _numLayers, uint16_t _mip, uint8_t _resolve)
	{
		access    = _access;
		handle    = _handle;
		mip       = _mip;
		layer     = _layer;
		numLayers = _numLayers;
		resolve   = _resolve;
	}

	bool init(const Init& _userInit)
	{
		if (NULL != s_ctx)
		{
			BX_TRACE("max is already initialized.");
			return false;
		}

		Init init = _userInit;

		init.limits.maxEncoders       = bx::clamp<uint16_t>(init.limits.maxEncoders, 1, (0 != MAX_CONFIG_MULTITHREADED) ? 128 : 1);
		init.limits.minResourceCbSize = bx::min<uint32_t>(init.limits.minResourceCbSize, MAX_CONFIG_MIN_RESOURCE_COMMAND_BUFFER_SIZE);

		struct ErrorState
		{
			enum Enum
			{
				Default,
				ContextAllocated,
			};
		};

		ErrorState::Enum errorState = ErrorState::Default;

		if (NULL != init.allocator)
		{
			g_allocator = init.allocator;
		}
		else
		{
			bx::DefaultAllocator allocator;
			g_allocator =
				s_allocatorStub = BX_NEW(&allocator, AllocatorStub);
		}

		if (NULL != init.callback)
		{
			g_callback = init.callback;
		}
		else
		{
			g_callback =
				s_callbackStub = BX_NEW(g_allocator, CallbackStub);
		}

		bx::memSet(&g_caps, 0, sizeof(g_caps) );
		g_caps.limits.maxDrawCalls            = MAX_CONFIG_MAX_DRAW_CALLS;
		g_caps.limits.maxBlits                = MAX_CONFIG_MAX_BLIT_ITEMS;
		g_caps.limits.maxTextureSize          = 0;
		g_caps.limits.maxTextureLayers        = 1;
		g_caps.limits.maxViews                = MAX_CONFIG_MAX_VIEWS;
		g_caps.limits.maxFrameBuffers         = MAX_CONFIG_MAX_FRAME_BUFFERS;
		g_caps.limits.maxPrograms             = MAX_CONFIG_MAX_PROGRAMS;
		g_caps.limits.maxShaders              = MAX_CONFIG_MAX_SHADERS;
		g_caps.limits.maxTextures             = MAX_CONFIG_MAX_TEXTURES;
		g_caps.limits.maxTextureSamplers      = MAX_CONFIG_MAX_TEXTURE_SAMPLERS;
		g_caps.limits.maxComputeBindings      = 0;
		g_caps.limits.maxVertexLayouts        = MAX_CONFIG_MAX_VERTEX_LAYOUTS;
		g_caps.limits.maxVertexStreams        = 1;
		g_caps.limits.maxIndexBuffers         = MAX_CONFIG_MAX_INDEX_BUFFERS;
		g_caps.limits.maxVertexBuffers        = MAX_CONFIG_MAX_VERTEX_BUFFERS;
		g_caps.limits.maxDynamicIndexBuffers  = MAX_CONFIG_MAX_DYNAMIC_INDEX_BUFFERS;
		g_caps.limits.maxDynamicVertexBuffers = MAX_CONFIG_MAX_DYNAMIC_VERTEX_BUFFERS;
		g_caps.limits.maxUniforms             = MAX_CONFIG_MAX_UNIFORMS;
		g_caps.limits.maxOcclusionQueries     = MAX_CONFIG_MAX_OCCLUSION_QUERIES;
		g_caps.limits.maxFBAttachments        = 1;
		g_caps.limits.maxEncoders             = init.limits.maxEncoders;
		g_caps.limits.minResourceCbSize       = init.limits.minResourceCbSize;
		g_caps.limits.transientVbSize         = init.limits.transientVbSize;
		g_caps.limits.transientIbSize         = init.limits.transientIbSize;

		g_caps.vendorId = init.vendorId;
		g_caps.deviceId = init.deviceId;

		BX_TRACE("Init...");

		// max 1.104.7082
		//      ^ ^^^ ^^^^
		//      | |   +--- Commit number  (https://github.com/bkaradzic/max / git rev-list --count HEAD)
		//      | +------- API version    (from https://github.com/bkaradzic/max/blob/master/scripts/max.idl#L4)
		//      +--------- Major revision (always 1)
		BX_TRACE("Version 1.%d.%d (commit: " MAX_REV_SHA1 ")", MAX_API_VERSION, MAX_REV_NUMBER);

		errorState = ErrorState::ContextAllocated;

		s_ctx = BX_ALIGNED_NEW(g_allocator, Context, Context::kAlignment);
		if (s_ctx->init(init) )
		{
			BX_TRACE("Init complete.");
			return true;
		}

		BX_TRACE("Init failed.");

		switch (errorState)
		{
		case ErrorState::ContextAllocated:
			bx::deleteObject(g_allocator, s_ctx, Context::kAlignment);
			s_ctx = NULL;
			[[fallthrough]];

		case ErrorState::Default:
			if (NULL != s_callbackStub)
			{
				bx::deleteObject(g_allocator, s_callbackStub);
				s_callbackStub = NULL;
			}

			if (NULL != s_allocatorStub)
			{
				bx::DefaultAllocator allocator;
				bx::deleteObject(&allocator, s_allocatorStub);
				s_allocatorStub = NULL;
			}

			s_threadIndex = 0;
			g_callback    = NULL;
			g_allocator   = NULL;
			break;
		}

		return false;
	}

	void shutdown()
	{
		BX_TRACE("Shutdown...");

		MAX_CHECK_API_THREAD();
		Context* ctx = s_ctx; // it's going to be NULLd inside shutdown.
		ctx->shutdown();
		BX_ASSERT(NULL == s_ctx, "max is should be uninitialized here.");

		bx::deleteObject(g_allocator, ctx, Context::kAlignment);

		BX_TRACE("Shutdown complete.");

		if (NULL != s_allocatorStub)
		{
			s_allocatorStub->checkLeaks();
		}

		if (NULL != s_callbackStub)
		{
			bx::deleteObject(g_allocator, s_callbackStub);
			s_callbackStub = NULL;
		}

		if (NULL != s_allocatorStub)
		{
			bx::DefaultAllocator allocator;
			bx::deleteObject(&allocator, s_allocatorStub);
			s_allocatorStub = NULL;
		}

		s_threadIndex = 0;
		g_callback    = NULL;
		g_allocator   = NULL;
	}

	void reset(uint32_t _width, uint32_t _height, uint32_t _flags, TextureFormat::Enum _format)
	{
		MAX_CHECK_API_THREAD();
		BX_ASSERT(0 == (_flags&MAX_RESET_RESERVED_MASK), "Do not set reset reserved flags!");
		s_ctx->reset(_width, _height, _flags, _format);
	}

	Encoder* begin(bool _forThread)
	{
		return s_ctx->begin(_forThread);
	}

#define MAX_ENCODER(_func) reinterpret_cast<EncoderImpl*>(this)->_func

	void Encoder::setMarker(const char* _name, int32_t _len)
	{
		MAX_ENCODER(setMarker(bx::StringView(_name, _len) ) );
	}

	void Encoder::setState(uint64_t _state, uint32_t _rgba)
	{
		BX_ASSERT(0 == (_state&MAX_STATE_RESERVED_MASK), "Do not set state reserved flags!");
		MAX_ENCODER(setState(_state, _rgba) );
	}

	void Encoder::setCondition(OcclusionQueryHandle _handle, bool _visible)
	{
		MAX_CHECK_CAPS(MAX_CAPS_OCCLUSION_QUERY, "Occlusion query is not supported!");
		MAX_ENCODER(setCondition(_handle, _visible) );
	}

	void Encoder::setStencil(uint32_t _fstencil, uint32_t _bstencil)
	{
		MAX_ENCODER(setStencil(_fstencil, _bstencil) );
	}

	uint16_t Encoder::setScissor(uint16_t _x, uint16_t _y, uint16_t _width, uint16_t _height)
	{
		return MAX_ENCODER(setScissor(_x, _y, _width, _height) );
	}

	void Encoder::setScissor(uint16_t _cache)
	{
		MAX_ENCODER(setScissor(_cache) );
	}

	uint32_t Encoder::setTransform(const void* _mtx, uint16_t _num)
	{
		return MAX_ENCODER(setTransform(_mtx, _num) );
	}

	uint32_t Encoder::allocTransform(Transform* _transform, uint16_t _num)
	{
		return MAX_ENCODER(allocTransform(_transform, _num) );
	}

	void Encoder::setTransform(uint32_t _cache, uint16_t _num)
	{
		MAX_ENCODER(setTransform(_cache, _num) );
	}

	void Encoder::setUniform(UniformHandle _handle, const void* _value, uint16_t _num)
	{
		MAX_CHECK_HANDLE("setUniform", s_ctx->m_uniformHandle, _handle);
		const UniformRef& uniform = s_ctx->m_uniformRef[_handle.idx];
		BX_ASSERT(isValid(_handle) && 0 < uniform.m_refCount, "Setting invalid uniform (handle %3d)!", _handle.idx);
		BX_ASSERT(_num == UINT16_MAX || uniform.m_num >= _num, "Truncated uniform update. %d (max: %d)", _num, uniform.m_num);
		MAX_ENCODER(setUniform(uniform.m_type, _handle, _value, UINT16_MAX != _num ? _num : uniform.m_num) );
	}

	void Encoder::setIndexBuffer(IndexBufferHandle _handle)
	{
		setIndexBuffer(_handle, 0, UINT32_MAX);
	}

	void Encoder::setIndexBuffer(IndexBufferHandle _handle, uint32_t _firstIndex, uint32_t _numIndices)
	{
		MAX_CHECK_HANDLE("setIndexBuffer", s_ctx->m_indexBufferHandle, _handle);
		const IndexBuffer& ib = s_ctx->m_indexBuffers[_handle.idx];
		MAX_ENCODER(setIndexBuffer(_handle, ib, _firstIndex, _numIndices) );
	}

	void Encoder::setIndexBuffer(DynamicIndexBufferHandle _handle)
	{
		setIndexBuffer(_handle, 0, UINT32_MAX);
	}

	void Encoder::setIndexBuffer(DynamicIndexBufferHandle _handle, uint32_t _firstIndex, uint32_t _numIndices)
	{
		MAX_CHECK_HANDLE("setIndexBuffer", s_ctx->m_dynamicIndexBufferHandle, _handle);
		const DynamicIndexBuffer& dib = s_ctx->m_dynamicIndexBuffers[_handle.idx];
		MAX_ENCODER(setIndexBuffer(dib, _firstIndex, _numIndices) );
	}

	void Encoder::setIndexBuffer(const TransientIndexBuffer* _tib)
	{
		setIndexBuffer(_tib, 0, UINT32_MAX);
	}

	void Encoder::setIndexBuffer(const TransientIndexBuffer* _tib, uint32_t _firstIndex, uint32_t _numIndices)
	{
		BX_ASSERT(NULL != _tib, "_tib can't be NULL");
		MAX_CHECK_HANDLE("setIndexBuffer", s_ctx->m_indexBufferHandle, _tib->handle);
		MAX_ENCODER(setIndexBuffer(_tib, _firstIndex, _numIndices) );
	}

	void Encoder::setVertexBuffer(
		  uint8_t _stream
		, VertexBufferHandle _handle
		, uint32_t _startVertex
		, uint32_t _numVertices
		, VertexLayoutHandle _layoutHandle
	)
	{
		MAX_CHECK_HANDLE("setVertexBuffer", s_ctx->m_vertexBufferHandle, _handle);
		MAX_CHECK_HANDLE_INVALID_OK("setVertexBuffer", s_ctx->m_layoutHandle, _layoutHandle);
		MAX_ENCODER(setVertexBuffer(_stream, _handle, _startVertex, _numVertices, _layoutHandle) );
	}

	void Encoder::setVertexBuffer(uint8_t _stream, VertexBufferHandle _handle)
	{
		setVertexBuffer(_stream, _handle, 0, UINT32_MAX);
	}

	void Encoder::setVertexBuffer(
		  uint8_t _stream
		, DynamicVertexBufferHandle _handle
		, uint32_t _startVertex
		, uint32_t _numVertices
		, VertexLayoutHandle _layoutHandle
		)
	{
		MAX_CHECK_HANDLE("setVertexBuffer", s_ctx->m_dynamicVertexBufferHandle, _handle);
		MAX_CHECK_HANDLE_INVALID_OK("setVertexBuffer", s_ctx->m_layoutHandle, _layoutHandle);
		const DynamicVertexBuffer& dvb = s_ctx->m_dynamicVertexBuffers[_handle.idx];
		MAX_ENCODER(setVertexBuffer(_stream, dvb, _startVertex, _numVertices, _layoutHandle) );
	}

	void Encoder::setVertexBuffer(uint8_t _stream, DynamicVertexBufferHandle _handle)
	{
		setVertexBuffer(_stream, _handle, 0, UINT32_MAX);
	}

	void Encoder::setVertexBuffer(
		  uint8_t _stream
		, const TransientVertexBuffer* _tvb
		, uint32_t _startVertex
		, uint32_t _numVertices
		, VertexLayoutHandle _layoutHandle
		)
	{
		BX_ASSERT(NULL != _tvb, "_tvb can't be NULL");
		MAX_CHECK_HANDLE("setVertexBuffer", s_ctx->m_vertexBufferHandle, _tvb->handle);
		MAX_CHECK_HANDLE_INVALID_OK("setVertexBuffer", s_ctx->m_layoutHandle, _layoutHandle);
		MAX_ENCODER(setVertexBuffer(_stream, _tvb, _startVertex, _numVertices, _layoutHandle) );
	}

	void Encoder::setVertexBuffer(uint8_t _stream, const TransientVertexBuffer* _tvb)
	{
		setVertexBuffer(_stream, _tvb, 0, UINT32_MAX);
	}

	void Encoder::setVertexCount(uint32_t _numVertices)
	{
		MAX_CHECK_CAPS(MAX_CAPS_VERTEX_ID, "Auto generated vertices are not supported!");
		MAX_ENCODER(setVertexCount(_numVertices) );
	}

	void Encoder::setInstanceDataBuffer(const InstanceDataBuffer* _idb)
	{
		setInstanceDataBuffer(_idb, 0, UINT32_MAX);
	}

	void Encoder::setInstanceDataBuffer(const InstanceDataBuffer* _idb, uint32_t _start, uint32_t _num)
	{
		BX_ASSERT(NULL != _idb, "_idb can't be NULL");
		MAX_ENCODER(setInstanceDataBuffer(_idb, _start, _num) );
	}

	void Encoder::setInstanceDataBuffer(VertexBufferHandle _handle, uint32_t _startVertex, uint32_t _num)
	{
		MAX_CHECK_HANDLE("setInstanceDataBuffer", s_ctx->m_vertexBufferHandle, _handle);
		const VertexBuffer& vb = s_ctx->m_vertexBuffers[_handle.idx];
		MAX_ENCODER(setInstanceDataBuffer(_handle, _startVertex, _num, vb.m_stride) );
	}

	void Encoder::setInstanceDataBuffer(DynamicVertexBufferHandle _handle, uint32_t _startVertex, uint32_t _num)
	{
		MAX_CHECK_HANDLE("setInstanceDataBuffer", s_ctx->m_dynamicVertexBufferHandle, _handle);
		const DynamicVertexBuffer& dvb = s_ctx->m_dynamicVertexBuffers[_handle.idx];
		MAX_ENCODER(setInstanceDataBuffer(dvb.m_handle
			, dvb.m_startVertex + _startVertex
			, _num
			, dvb.m_stride
			) );
	}

	void Encoder::setInstanceCount(uint32_t _numInstances)
	{
		MAX_CHECK_CAPS(MAX_CAPS_VERTEX_ID, "Auto generated instances are not supported!");
		MAX_ENCODER(setInstanceCount(_numInstances) );
	}

	void Encoder::setTexture(uint8_t _stage, UniformHandle _sampler, TextureHandle _handle, uint32_t _flags)
	{
		MAX_CHECK_HANDLE("setTexture/UniformHandle", s_ctx->m_uniformHandle, _sampler);
		MAX_CHECK_HANDLE_INVALID_OK("setTexture/TextureHandle", s_ctx->m_textureHandle, _handle);
		BX_ASSERT(_stage < g_caps.limits.maxTextureSamplers, "Invalid stage %d (max %d).", _stage, g_caps.limits.maxTextureSamplers);

		if (isValid(_handle) )
		{
			const TextureRef& ref = s_ctx->m_textureRef[_handle.idx];
			BX_ASSERT(!ref.isReadBack()
				, "Can't sample from texture which was created with MAX_TEXTURE_READ_BACK. This is CPU only texture."
				);
			BX_UNUSED(ref);
		}

		MAX_ENCODER(setTexture(_stage, _sampler, _handle, _flags) );
	}

	void Encoder::touch(ViewId _id)
	{
		discard();
		submit(_id, MAX_INVALID_HANDLE);
	}

	void Encoder::submit(ViewId _id, ProgramHandle _program, uint32_t _depth, uint8_t _flags)
	{
		OcclusionQueryHandle handle = MAX_INVALID_HANDLE;
		submit(_id, _program, handle, _depth, _flags);
	}

	void Encoder::submit(ViewId _id, ProgramHandle _program, OcclusionQueryHandle _occlusionQuery, uint32_t _depth, uint8_t _flags)
	{
		BX_ASSERT(false
			|| !isValid(_occlusionQuery)
			|| 0 != (g_caps.supported & MAX_CAPS_OCCLUSION_QUERY)
			, "Occlusion query is not supported! Use max::getCaps to check MAX_CAPS_OCCLUSION_QUERY backend renderer capabilities."
			);
		MAX_CHECK_HANDLE_INVALID_OK("submit", s_ctx->m_programHandle, _program);
		MAX_CHECK_HANDLE_INVALID_OK("submit", s_ctx->m_occlusionQueryHandle, _occlusionQuery);
		MAX_ENCODER(submit(_id, _program, _occlusionQuery, _depth, _flags) );
	}

	void Encoder::submit(ViewId _id, ProgramHandle _program, IndirectBufferHandle _indirectHandle, uint32_t _start, uint32_t _num, uint32_t _depth, uint8_t _flags)
	{
		MAX_CHECK_HANDLE_INVALID_OK("submit", s_ctx->m_programHandle, _program);
		MAX_CHECK_HANDLE("submit", s_ctx->m_vertexBufferHandle, _indirectHandle);
		MAX_CHECK_CAPS(MAX_CAPS_DRAW_INDIRECT, "Draw indirect is not supported!");
		MAX_ENCODER(submit(_id, _program, _indirectHandle, _start, _num, _depth, _flags) );
	}

	void Encoder::submit(ViewId _id, ProgramHandle _program, IndirectBufferHandle _indirectHandle, uint32_t _start, IndexBufferHandle _numHandle, uint32_t _numIndex, uint32_t _numMax, uint32_t _depth, uint8_t _flags)
	{
		MAX_CHECK_HANDLE_INVALID_OK("submit", s_ctx->m_programHandle, _program);
		MAX_CHECK_HANDLE("submit", s_ctx->m_vertexBufferHandle, _indirectHandle);
		MAX_CHECK_HANDLE("submit", s_ctx->m_indexBufferHandle, _numHandle);
		MAX_CHECK_CAPS(MAX_CAPS_DRAW_INDIRECT, "Draw indirect is not supported!");
		MAX_CHECK_CAPS(MAX_CAPS_DRAW_INDIRECT_COUNT, "Draw indirect count is not supported!");
		MAX_ENCODER(submit(_id, _program, _indirectHandle, _start, _numHandle, _numIndex, _numMax, _depth, _flags) );
	}

	void Encoder::setBuffer(uint8_t _stage, IndexBufferHandle _handle, Access::Enum _access)
	{
		BX_ASSERT(_stage < g_caps.limits.maxComputeBindings, "Invalid stage %d (max %d).", _stage, g_caps.limits.maxComputeBindings);
		MAX_CHECK_HANDLE("setBuffer", s_ctx->m_indexBufferHandle, _handle);
		MAX_ENCODER(setBuffer(_stage, _handle, _access) );
	}

	void Encoder::setBuffer(uint8_t _stage, VertexBufferHandle _handle, Access::Enum _access)
	{
		BX_ASSERT(_stage < g_caps.limits.maxComputeBindings, "Invalid stage %d (max %d).", _stage, g_caps.limits.maxComputeBindings);
		MAX_CHECK_HANDLE("setBuffer", s_ctx->m_vertexBufferHandle, _handle);
		MAX_ENCODER(setBuffer(_stage, _handle, _access) );
	}

	void Encoder::setBuffer(uint8_t _stage, DynamicIndexBufferHandle _handle, Access::Enum _access)
	{
		BX_ASSERT(_stage < g_caps.limits.maxComputeBindings, "Invalid stage %d (max %d).", _stage, g_caps.limits.maxComputeBindings);
		MAX_CHECK_HANDLE("setBuffer", s_ctx->m_dynamicIndexBufferHandle, _handle);
		const DynamicIndexBuffer& dib = s_ctx->m_dynamicIndexBuffers[_handle.idx];
		MAX_ENCODER(setBuffer(_stage, dib.m_handle, _access) );
	}

	void Encoder::setBuffer(uint8_t _stage, DynamicVertexBufferHandle _handle, Access::Enum _access)
	{
		BX_ASSERT(_stage < g_caps.limits.maxComputeBindings, "Invalid stage %d (max %d).", _stage, g_caps.limits.maxComputeBindings);
		MAX_CHECK_HANDLE("setBuffer", s_ctx->m_dynamicVertexBufferHandle, _handle);
		const DynamicVertexBuffer& dvb = s_ctx->m_dynamicVertexBuffers[_handle.idx];
		MAX_ENCODER(setBuffer(_stage, dvb.m_handle, _access) );
	}

	void Encoder::setBuffer(uint8_t _stage, IndirectBufferHandle _handle, Access::Enum _access)
	{
		BX_ASSERT(_stage < g_caps.limits.maxComputeBindings, "Invalid stage %d (max %d).", _stage, g_caps.limits.maxComputeBindings);
		MAX_CHECK_HANDLE("setBuffer", s_ctx->m_vertexBufferHandle, _handle);
		VertexBufferHandle handle = { _handle.idx };
		MAX_ENCODER(setBuffer(_stage, handle, _access) );
	}

	void Encoder::setImage(uint8_t _stage, TextureHandle _handle, uint8_t _mip, Access::Enum _access, TextureFormat::Enum _format)
	{
		BX_ASSERT(_stage < g_caps.limits.maxComputeBindings, "Invalid stage %d (max %d).", _stage, g_caps.limits.maxComputeBindings);
		MAX_CHECK_HANDLE_INVALID_OK("setImage/TextureHandle", s_ctx->m_textureHandle, _handle);
		_format = TextureFormat::Count == _format
			? TextureFormat::Enum(s_ctx->m_textureRef[_handle.idx].m_format)
			: _format
			;
		BX_ASSERT(_format != TextureFormat::BGRA8
			, "Can't use TextureFormat::BGRA8 with compute, use TextureFormat::RGBA8 instead."
			);

		if (isValid(_handle) )
		{
			const TextureRef& ref = s_ctx->m_textureRef[_handle.idx];
			BX_ASSERT(!ref.isReadBack()
				, "Can't texture (handle %d, '%S') which was created with MAX_TEXTURE_READ_BACK with compute. This is CPU only texture."
				, _handle.idx
				, &ref.m_name
				);
			BX_UNUSED(ref);
		}

		MAX_ENCODER(setImage(_stage, _handle, _mip, _access, _format) );
	}

	void Encoder::dispatch(ViewId _id, ProgramHandle _program, uint32_t _numX, uint32_t _numY, uint32_t _numZ, uint8_t _flags)
	{
		MAX_CHECK_CAPS(MAX_CAPS_COMPUTE, "Compute is not supported!");
		MAX_CHECK_HANDLE_INVALID_OK("dispatch", s_ctx->m_programHandle, _program);
		MAX_ENCODER(dispatch(_id, _program, _numX, _numY, _numZ, _flags) );
	}

	void Encoder::dispatch(ViewId _id, ProgramHandle _program, IndirectBufferHandle _indirectHandle, uint32_t _start, uint32_t _num, uint8_t _flags)
	{
		MAX_CHECK_CAPS(MAX_CAPS_DRAW_INDIRECT, "Dispatch indirect is not supported!");
		MAX_CHECK_CAPS(MAX_CAPS_COMPUTE, "Compute is not supported!");
		MAX_CHECK_HANDLE_INVALID_OK("dispatch", s_ctx->m_programHandle, _program);
		MAX_CHECK_HANDLE("dispatch", s_ctx->m_vertexBufferHandle, _indirectHandle);
		MAX_ENCODER(dispatch(_id, _program, _indirectHandle, _start, _num, _flags) );
	}

	void Encoder::discard(uint8_t _flags)
	{
		MAX_ENCODER(discard(_flags) );
	}

	void Encoder::blit(ViewId _id, TextureHandle _dst, uint16_t _dstX, uint16_t _dstY, TextureHandle _src, uint16_t _srcX, uint16_t _srcY, uint16_t _width, uint16_t _height)
	{
		blit(_id, _dst, 0, _dstX, _dstY, 0, _src, 0, _srcX, _srcY, 0, _width, _height, 0);
	}

	void Encoder::blit(ViewId _id, TextureHandle _dst, uint8_t _dstMip, uint16_t _dstX, uint16_t _dstY, uint16_t _dstZ, TextureHandle _src, uint8_t _srcMip, uint16_t _srcX, uint16_t _srcY, uint16_t _srcZ, uint16_t _width, uint16_t _height, uint16_t _depth)
	{
		MAX_CHECK_CAPS(MAX_CAPS_TEXTURE_BLIT, "Texture blit is not supported!");
		MAX_CHECK_HANDLE("blit/src TextureHandle", s_ctx->m_textureHandle, _src);
		MAX_CHECK_HANDLE("blit/dst TextureHandle", s_ctx->m_textureHandle, _dst);

		const TextureRef& src = s_ctx->m_textureRef[_src.idx];
		const TextureRef& dst = s_ctx->m_textureRef[_dst.idx];

		BX_ASSERT(dst.isBlitDst()
			, "Blit destination texture (handle %d, '%S') is not created with `MAX_TEXTURE_BLIT_DST` flag."
			, _dst.idx
			, &dst.m_name
			);

		BX_ASSERT(src.m_format == dst.m_format
			, "Texture format must match (src %s, dst %s)."
			, bimg::getName(bimg::TextureFormat::Enum(src.m_format) )
			, bimg::getName(bimg::TextureFormat::Enum(dst.m_format) )
			);
		BX_ASSERT(_srcMip < src.m_numMips, "Invalid blit src mip (%d > %d)", _srcMip, src.m_numMips - 1);
		BX_ASSERT(_dstMip < dst.m_numMips, "Invalid blit dst mip (%d > %d)", _dstMip, dst.m_numMips - 1);

		uint32_t srcWidth  = bx::max<uint32_t>(1, src.m_width  >> _srcMip);
		uint32_t srcHeight = bx::max<uint32_t>(1, src.m_height >> _srcMip);
		uint32_t dstWidth  = bx::max<uint32_t>(1, dst.m_width  >> _dstMip);
		uint32_t dstHeight = bx::max<uint32_t>(1, dst.m_height >> _dstMip);

		uint32_t srcDepth  = src.isCubeMap() ? 6 : bx::max<uint32_t>(1, src.m_depth >> _srcMip);
		uint32_t dstDepth  = dst.isCubeMap() ? 6 : bx::max<uint32_t>(1, dst.m_depth >> _dstMip);

		BX_ASSERT(_srcX < srcWidth && _srcY < srcHeight && _srcZ < srcDepth
			, "Blit src coordinates out of range (%d, %d, %d) >= (%d, %d, %d)"
			, _srcX, _srcY, _srcZ
			, srcWidth, srcHeight, srcDepth
			);
		BX_ASSERT(_dstX < dstWidth && _dstY < dstHeight && _dstZ < dstDepth
			, "Blit dst coordinates out of range (%d, %d, %d) >= (%d, %d, %d)"
			, _dstX, _dstY, _dstZ
			, dstWidth, dstHeight, dstDepth
			);

		srcWidth  = bx::min<uint32_t>(srcWidth,  _srcX + _width ) - _srcX;
		srcHeight = bx::min<uint32_t>(srcHeight, _srcY + _height) - _srcY;
		srcDepth  = bx::min<uint32_t>(srcDepth,  _srcZ + _depth ) - _srcZ;
		dstWidth  = bx::min<uint32_t>(dstWidth,  _dstX + _width ) - _dstX;
		dstHeight = bx::min<uint32_t>(dstHeight, _dstY + _height) - _dstY;
		dstDepth  = bx::min<uint32_t>(dstDepth,  _dstZ + _depth ) - _dstZ;

		const uint16_t width  = uint16_t(bx::min(srcWidth,  dstWidth ) );
		const uint16_t height = uint16_t(bx::min(srcHeight, dstHeight) );
		const uint16_t depth  = uint16_t(bx::min(srcDepth,  dstDepth ) );

		MAX_ENCODER(blit(_id, _dst, _dstMip, _dstX, _dstY, _dstZ, _src, _srcMip, _srcX, _srcY, _srcZ, width, height, depth) );
	}

#undef MAX_ENCODER

	void end(Encoder* _encoder)
	{
		s_ctx->end(_encoder);
	}

	void MeshQuery::alloc(uint32_t _num)
	{
		m_handleData = (MeshQuery::HandleData*)bx::alloc(g_allocator, sizeof(MeshQuery::HandleData) * _num);
		m_num = 0;
	}

	void MeshQuery::free()
	{
		bx::free(g_allocator, m_handleData);
		m_num = 0;
	}

	void EntityQuery::alloc(uint32_t _num)
	{
		m_entities = (EntityHandle*)bx::alloc(g_allocator, sizeof(EntityHandle) * _num);
		m_num = 0;
	}

	void EntityQuery::free()
	{
		bx::free(g_allocator, m_entities);
		m_num = 0;
	}

	void HashQuery::alloc(uint32_t _num)
	{
		m_data = (uint32_t*)bx::alloc(g_allocator, sizeof(uint32_t) * _num);
		m_num = 0;
	}

	void HashQuery::free()
	{
		bx::free(g_allocator, m_data);
		m_num = 0;
	}

	uint32_t frame(bool _capture)
	{
		MAX_CHECK_API_THREAD();
		return s_ctx->frame(_capture);
	}

	const Caps* getCaps()
	{
		return &g_caps;
	}

	const Stats* getStats()
	{
		return s_ctx->getPerfStats();
	}

	RendererType::Enum getRendererType()
	{
		return g_caps.rendererType;
	}

	const Memory* alloc(uint32_t _size)
	{
		BX_ASSERT(0 < _size, "Invalid memory operation. _size is 0.");
		Memory* mem = (Memory*)bx::alloc(g_allocator, sizeof(Memory) + _size);
		mem->size = _size;
		mem->data = (uint8_t*)mem + sizeof(Memory);
		return mem;
	}

	const Memory* copy(const void* _data, uint32_t _size)
	{
		BX_ASSERT(0 < _size, "Invalid memory operation. _size is 0.");
		const Memory* mem = alloc(_size);
		bx::memCopy(mem->data, _data, _size);
		return mem;
	}

	struct MemoryRef
	{
		Memory mem;
		ReleaseFn releaseFn;
		void* userData;
	};

	const Memory* makeRef(const void* _data, uint32_t _size, ReleaseFn _releaseFn, void* _userData)
	{
		MemoryRef* memRef = (MemoryRef*)bx::alloc(g_allocator, sizeof(MemoryRef) );
		memRef->mem.size  = _size;
		memRef->mem.data  = (uint8_t*)_data;
		memRef->releaseFn = _releaseFn;
		memRef->userData  = _userData;
		return &memRef->mem;
	}

	bool isMemoryRef(const Memory* _mem)
	{
		return _mem->data != (uint8_t*)_mem + sizeof(Memory);
	}

	void release(const Memory* _mem)
	{
		BX_ASSERT(NULL != _mem, "_mem can't be NULL");
		Memory* mem = const_cast<Memory*>(_mem);
		if (isMemoryRef(mem) )
		{
			MemoryRef* memRef = reinterpret_cast<MemoryRef*>(mem);
			if (NULL != memRef->releaseFn)
			{
				memRef->releaseFn(mem->data, memRef->userData);
			}
		}
		bx::free(g_allocator, mem);
	}

	void setDebug(uint32_t _debug)
	{
		MAX_CHECK_API_THREAD();
		s_ctx->setDebug(_debug);
	}

	void dbgTextClear(uint8_t _attr, bool _small)
	{
		MAX_CHECK_API_THREAD();
		s_ctx->dbgTextClear(_attr, _small);
	}

	void dbgTextPrintfVargs(uint16_t _x, uint16_t _y, uint8_t _attr, const char* _format, va_list _argList)
	{
		s_ctx->dbgTextPrintfVargs(_x, _y, _attr, _format, _argList);
	}

	void dbgTextPrintf(uint16_t _x, uint16_t _y, uint8_t _attr, const char* _format, ...)
	{
		MAX_CHECK_API_THREAD();
		va_list argList;
		va_start(argList, _format);
		s_ctx->dbgTextPrintfVargs(_x, _y, _attr, _format, argList);
		va_end(argList);
	}

	void dbgTextImage(uint16_t _x, uint16_t _y, uint16_t _width, uint16_t _height, const void* _data, uint16_t _pitch)
	{
		MAX_CHECK_API_THREAD();
		s_ctx->dbgTextImage(_x, _y, _width, _height, _data, _pitch);
	}

	void dbgDrawBegin(uint16_t _viewId, bool _depthTestLess, max::Encoder* _encoder)
	{
		MAX_CHECK_API_THREAD();
		s_dde.begin(_viewId, _depthTestLess, _encoder);
	}

	void dbgDrawEnd()
	{
		MAX_CHECK_API_THREAD();
		s_dde.end();
	}

	void dbgDrawPush()
	{
		MAX_CHECK_API_THREAD();
		s_dde.push();
	}

	void dbgDrawPop()
	{
		MAX_CHECK_API_THREAD();
		s_dde.pop();
	}

	void dbgDrawSetDepthTestLess(bool _depthTestLess)
	{
		MAX_CHECK_API_THREAD();
		s_dde.setDepthTestLess(_depthTestLess);
	}

	void dbgDrawSetState(bool _depthTest, bool _depthWrite, bool _clockwise)
	{
		MAX_CHECK_API_THREAD();
		s_dde.setState(_depthTest, _depthWrite, _clockwise);
	}

	void dbgDrawSetColor(uint32_t _abgr)
	{
		MAX_CHECK_API_THREAD();
		s_dde.setColor(_abgr);
	}

	void dbgDrawSetLod(uint8_t _lod)
	{
		MAX_CHECK_API_THREAD();
		s_dde.setLod(_lod);
	}

	void dbgDrawSetWireframe(bool _wireframe)
	{
		MAX_CHECK_API_THREAD();
		s_dde.setWireframe(_wireframe);
	}

	void dbgDrawSetStipple(bool _stipple, float _scale, float _offset)
	{
		MAX_CHECK_API_THREAD();
		s_dde.setStipple(_stipple, _scale, _offset);
	}

	void dbgDrawSetSpin(float _spin)
	{
		MAX_CHECK_API_THREAD();
		s_dde.setSpin(_spin);
	}

	void dbgDrawSetTransform(const void* _mtx)
	{
		MAX_CHECK_API_THREAD();
		s_dde.setTransform(_mtx);
	}

	void dbgDrawSetTranslate(float _x, float _y, float _z)
	{
		MAX_CHECK_API_THREAD();
		s_dde.setTranslate(_x, _y, _z);
	}

	void dbgDrawPushTransform(const void* _mtx)
	{
		MAX_CHECK_API_THREAD();
		s_dde.pushTransform(_mtx, 1);
	}

	void dbgDrawPopTransform()
	{
		MAX_CHECK_API_THREAD();
		s_dde.popTransform();
	}

	void dbgDrawMoveTo(float _x, float _y, float _z)
	{
		MAX_CHECK_API_THREAD();
		s_dde.moveTo(_x, _y, _z);
	}

	void dbgDrawMoveTo(const bx::Vec3& _pos)
	{
		MAX_CHECK_API_THREAD();
		s_dde.moveTo(_pos);
	}

	void dbgDrawLineTo(float _x, float _y, float _z)
	{
		MAX_CHECK_API_THREAD();
		s_dde.lineTo(_x, _y, _z);
	}

	void dbgDrawLineTo(const bx::Vec3& _pos)
	{
		MAX_CHECK_API_THREAD();
		s_dde.lineTo(_pos);
	}

	void dbgDrawClose()
	{
		MAX_CHECK_API_THREAD();
		s_dde.close();
	}

	void dbgDraw(const bx::Aabb& _aabb)
	{
		MAX_CHECK_API_THREAD();
		s_dde.draw(_aabb);
	}

	void dbgDraw(const bx::Cylinder& _cylinder)
	{
		MAX_CHECK_API_THREAD();
		s_dde.drawCylinder(_cylinder.pos, _cylinder.end, _cylinder.radius, false);
	}

	void dbgDraw(const bx::Capsule& _capsule)
	{
		MAX_CHECK_API_THREAD();
		s_dde.drawCylinder(_capsule.pos, _capsule.end, _capsule.radius, true);
	}

	void dbgDraw(const bx::Disk& _disk)
	{
		MAX_CHECK_API_THREAD();
		s_dde.draw(_disk);
	}

	void dbgDraw(const bx::Obb& _obb)
	{
		MAX_CHECK_API_THREAD();
		s_dde.draw(_obb);
	}

	void dbgDraw(const bx::Sphere& _sphere)
	{
		MAX_CHECK_API_THREAD();
		s_dde.draw(_sphere);
	}

	void dbgDraw(const bx::Triangle& _triangle)
	{
		MAX_CHECK_API_THREAD();
		s_dde.draw(_triangle);
	}

	void dbgDraw(const bx::Cone& _cone)
	{
		MAX_CHECK_API_THREAD();
		s_dde.drawCone(_cone.pos, _cone.end, _cone.radius);
	}

	void dbgDrawLineList(uint32_t _numVertices, const bx::Vec3* _vertices, uint32_t _numIndices, const uint16_t* _indices)
	{
		MAX_CHECK_API_THREAD();
		s_dde.draw(true, _numVertices, _vertices, _numIndices, _indices);
	}

	void dbgDrawTriList(uint32_t _numVertices, const bx::Vec3* _vertices, uint32_t _numIndices, const uint16_t* _indices)
	{
		MAX_CHECK_API_THREAD();
		s_dde.draw(false, _numVertices, _vertices, _numIndices, _indices);
	}

	void dbgDrawFrustum(const void* _viewProj)
	{
		MAX_CHECK_API_THREAD();
		s_dde.drawFrustum(_viewProj);
	}

	void dbgDrawArc(Axis::Enum _axis, float _x, float _y, float _z, float _radius, float _degrees)
	{
		MAX_CHECK_API_THREAD();
		s_dde.drawArc(_axis, _x, _y, _z, _radius, _degrees);
	}

	void dbgDrawCircle(const bx::Vec3& _normal, const bx::Vec3& _center, float _radius, float _weight)
	{
		MAX_CHECK_API_THREAD();
		s_dde.drawCircle(_normal, _center, _radius, _weight);
	}

	void dbgDrawCircle(Axis::Enum _axis, float _x, float _y, float _z, float _radius, float _weight)
	{
		MAX_CHECK_API_THREAD();
		s_dde.drawCircle(_axis, _x, _y, _z, _radius, _weight);
	}

	void dbgDrawQuad(const bx::Vec3& _normal, const bx::Vec3& _center, float _size)
	{
		MAX_CHECK_API_THREAD();
		s_dde.drawQuad(_normal, _center, _size);
	}

	void dbgDrawQuad(max::TextureHandle _handle, const bx::Vec3& _normal, const bx::Vec3& _center, float _size)
	{
		MAX_CHECK_API_THREAD();
		s_dde.drawQuad(_handle, _normal, _center, _size);
	}

	void dbgDrawCone(const bx::Vec3& _from, const bx::Vec3& _to, float _radius)
	{
		MAX_CHECK_API_THREAD();
		s_dde.drawCone(_from, _to, _radius);
	}

	void dbgDrawCylinder(const bx::Vec3& _from, const bx::Vec3& _to, float _radius)
	{
		MAX_CHECK_API_THREAD();
		s_dde.drawCylinder(_from, _to, _radius, false);
	}

	void dbgDrawCapsule(const bx::Vec3& _from, const bx::Vec3& _to, float _radius)
	{
		MAX_CHECK_API_THREAD();
		s_dde.drawCylinder(_from, _to, _radius, true);
	}

	void dbgDrawAxis(float _x, float _y, float _z, float _len, Axis::Enum _highlight, float _thickness)
	{
		MAX_CHECK_API_THREAD();
		s_dde.drawAxis(_x, _y, _z, _len, _highlight, _thickness);
	}

	void dbgDrawGrid(const bx::Vec3& _normal, const bx::Vec3& _center, uint32_t _size, float _step)
	{
		MAX_CHECK_API_THREAD();
		s_dde.drawGrid(_normal, _center, _size, _step);
	}

	void dbgDrawGrid(Axis::Enum _axis, const bx::Vec3& _center, uint32_t _size, float _step)
	{
		MAX_CHECK_API_THREAD();
		s_dde.drawGrid(_axis, _center, _size, _step);
	}

	void dbgDrawOrb(float _x, float _y, float _z, float _radius, Axis::Enum _highlight)
	{
		MAX_CHECK_API_THREAD();
		s_dde.drawOrb(_x, _y, _z, _radius, _highlight);
	}

	void inputAddBindings(const char* _name, const InputBinding* _bindings)
	{
		s_ctx->addBindings(_name, _bindings);
	}

	void inputRemoveBindings(const char* _name)
	{
		s_ctx->removeBindings(_name);
	}

	void inputAddMappings(uint32_t _id, InputMapping* _mappings)
	{
		s_ctx->addMappings(_id, _mappings);
	}

	void inputRemoveMappings(uint32_t _id)
	{
		s_ctx->removeMappings(_id);
	}

	void inputProcess()
	{
		s_ctx->processInput();
	}

	float inputGetValue(uint32_t _id, uint32_t _action)
	{
		return s_ctx->getValue(_id, _action);
	}

	void inputSetMouseResolution(uint16_t _width, uint16_t _height)
	{
		s_ctx->m_mouse.setResolution(_width, _height);
	}

	void inputSetKeyState(Key::Enum _key, uint8_t _modifiers, bool _down)
	{
		s_ctx->m_keyboard.setKeyState(_key, _modifiers, _down);
	}

	bool inputGetKeyState(Key::Enum _key, uint8_t* _modifiers)
	{
		return s_ctx->m_keyboard.getKeyState(_key, _modifiers);
	}

	uint8_t inputGetModifiersState()
	{
		return s_ctx->m_keyboard.getModifiersState();
	}

	void inputChar(uint8_t _len, const uint8_t _char[4])
	{
		s_ctx->m_keyboard.pushChar(_len, _char);
	}

	const uint8_t* inputGetChar()
	{
		return s_ctx->m_keyboard.popChar();
	}

	void inputCharFlush()
	{
		s_ctx->m_keyboard.charFlush();
	}

	void inputSetMousePos(int32_t _mx, int32_t _my, int32_t _mz)
	{
		s_ctx->m_mouse.setPos(_mx, _my, _mz);
	}

	void inputSetMouseButtonState(MouseButton::Enum _button, uint8_t _state)
	{
		s_ctx->m_mouse.setButtonState(_button, _state);
	}

	void inputGetMouse(float _mouse[3])
	{
		_mouse[0] = s_ctx->m_mouse.m_norm[0];
		_mouse[1] = s_ctx->m_mouse.m_norm[1];
		_mouse[2] = s_ctx->m_mouse.m_norm[2];
		s_ctx->m_mouse.m_norm[0] = 0.0f;
		s_ctx->m_mouse.m_norm[1] = 0.0f;
		s_ctx->m_mouse.m_norm[2] = 0.0f;
	}

	bool inputIsMouseLocked()
	{
		return s_ctx->m_mouse.m_lock;
	}

	void inputSetMouseLock(bool _lock)
	{
		if (s_ctx->m_mouse.m_lock != _lock)
		{
			s_ctx->m_mouse.m_lock = _lock;

			setMouseLock({0}, _lock);
			if (_lock)
			{
				s_ctx->m_mouse.m_norm[0] = 0.0f;
				s_ctx->m_mouse.m_norm[1] = 0.0f;
				s_ctx->m_mouse.m_norm[2] = 0.0f;
			}
		}
	}

	void inputSetGamepadAxis(GamepadHandle _handle, GamepadAxis::Enum _axis, int32_t _value)
	{
		s_ctx->m_gamepad[_handle.idx].setAxis(_axis, _value);
	}

	int32_t inputGetGamepadAxis(GamepadHandle _handle, GamepadAxis::Enum _axis)
	{
		return s_ctx->m_gamepad[_handle.idx].getAxis(_axis);
	}

	void* load(const char* _filePath, uint32_t* _size)
	{
		bx::FileReader reader;
		if (bx::open(&reader, _filePath))
		{
			uint32_t size = (uint32_t)bx::getSize(&reader);
			void* data = bx::alloc(g_allocator, size);
			bx::read(&reader, data, size, bx::ErrorAssert{});
			bx::close(&reader);
			if (NULL != _size)
			{
				*_size = size;
			}
			return data;
		}
		else
		{
			BX_TRACE("Failed to open: %s.", _filePath);
		}

		if (NULL != _size)
		{
			*_size = 0;
		}

		return NULL;
	}

	const max::Memory* loadMemory(const char* _filePath)
	{
		bx::FileReader reader;
		if (bx::open(&reader, _filePath))
		{
			uint32_t size = (uint32_t)bx::getSize(&reader);
			const max::Memory* mem = max::alloc(size + 1);
			bx::read(&reader, mem->data, size, bx::ErrorAssert{});
			bx::close(&reader);
			mem->data[mem->size - 1] = '\0';
			return mem;
		}

		BX_TRACE("Failed to load %s.", _filePath);
		return NULL;
	}

	IndexBufferHandle createIndexBuffer(const Memory* _mem, uint16_t _flags)
	{
		BX_ASSERT(
			  0 == (_flags & MAX_BUFFER_INDEX32) || 0 != (g_caps.supported & MAX_CAPS_INDEX32)
			, "32-bit indices are not supported. Use max::getCaps to check MAX_CAPS_INDEX32 backend renderer capabilities."
			);
		BX_ASSERT(NULL != _mem, "_mem can't be NULL");
		return s_ctx->createIndexBuffer(_mem, _flags);
	}

	void setName(IndexBufferHandle _handle, const char* _name, int32_t _len)
	{
		s_ctx->setName(_handle, bx::StringView(_name, _len) );
	}

	void destroy(IndexBufferHandle _handle)
	{
		s_ctx->destroyIndexBuffer(_handle);
	}

	VertexLayoutHandle createVertexLayout(const VertexLayout& _layout)
	{
		return s_ctx->createVertexLayout(_layout);
	}

	void destroy(VertexLayoutHandle _handle)
	{
		s_ctx->destroyVertexLayout(_handle);
	}

	VertexBufferHandle createVertexBuffer(const Memory* _mem, const VertexLayout& _layout, uint16_t _flags)
	{
		BX_ASSERT(NULL != _mem, "_mem can't be NULL");
		BX_ASSERT(isValid(_layout), "Invalid VertexLayout.");
		return s_ctx->createVertexBuffer(_mem, _layout, _flags);
	}

	void setName(VertexBufferHandle _handle, const char* _name, int32_t _len)
	{
		s_ctx->setName(_handle, bx::StringView(_name, _len) );
	}

	void destroy(VertexBufferHandle _handle)
	{
		s_ctx->destroyVertexBuffer(_handle);
	}

	DynamicIndexBufferHandle createDynamicIndexBuffer(uint32_t _num, uint16_t _flags)
	{
		return s_ctx->createDynamicIndexBuffer(_num, _flags);
	}

	DynamicIndexBufferHandle createDynamicIndexBuffer(const Memory* _mem, uint16_t _flags)
	{
		BX_ASSERT(
			  0 == (_flags & MAX_BUFFER_INDEX32) || 0 != (g_caps.supported & MAX_CAPS_INDEX32)
			, "32-bit indices are not supported. Use max::getCaps to check MAX_CAPS_INDEX32 backend renderer capabilities."
			);
		BX_ASSERT(NULL != _mem, "_mem can't be NULL");
		return s_ctx->createDynamicIndexBuffer(_mem, _flags);
	}

	void update(DynamicIndexBufferHandle _handle, uint32_t _startIndex, const Memory* _mem)
	{
		BX_ASSERT(NULL != _mem, "_mem can't be NULL");
		s_ctx->update(_handle, _startIndex, _mem);
	}

	void destroy(DynamicIndexBufferHandle _handle)
	{
		s_ctx->destroyDynamicIndexBuffer(_handle);
	}

	DynamicVertexBufferHandle createDynamicVertexBuffer(uint32_t _num, const VertexLayout& _layout, uint16_t _flags)
	{
		BX_ASSERT(isValid(_layout), "Invalid VertexLayout.");
		return s_ctx->createDynamicVertexBuffer(_num, _layout, _flags);
	}

	DynamicVertexBufferHandle createDynamicVertexBuffer(const Memory* _mem, const VertexLayout& _layout, uint16_t _flags)
	{
		BX_ASSERT(NULL != _mem, "_mem can't be NULL");
		BX_ASSERT(isValid(_layout), "Invalid VertexLayout.");
		return s_ctx->createDynamicVertexBuffer(_mem, _layout, _flags);
	}

	void update(DynamicVertexBufferHandle _handle, uint32_t _startVertex, const Memory* _mem)
	{
		BX_ASSERT(NULL != _mem, "_mem can't be NULL");
		s_ctx->update(_handle, _startVertex, _mem);
	}

	void destroy(DynamicVertexBufferHandle _handle)
	{
		s_ctx->destroyDynamicVertexBuffer(_handle);
	}

	uint32_t getAvailTransientIndexBuffer(uint32_t _num, bool _index32)
	{
		BX_ASSERT(0 < _num, "Requesting 0 indices.");
		return s_ctx->getAvailTransientIndexBuffer(_num, _index32);
	}

	uint32_t getAvailTransientVertexBuffer(uint32_t _num, const VertexLayout& _layout)
	{
		BX_ASSERT(0 < _num, "Requesting 0 vertices.");
		BX_ASSERT(isValid(_layout), "Invalid VertexLayout.");
		return s_ctx->getAvailTransientVertexBuffer(_num, _layout.m_stride);
	}

	uint32_t getAvailInstanceDataBuffer(uint32_t _num, uint16_t _stride)
	{
		BX_ASSERT(0 < _num, "Requesting 0 instances.");
		return s_ctx->getAvailTransientVertexBuffer(_num, _stride);
	}

	void allocTransientIndexBuffer(TransientIndexBuffer* _tib, uint32_t _num, bool _index32)
	{
		BX_ASSERT(NULL != _tib, "_tib can't be NULL");
		BX_ASSERT(0 < _num, "Requesting 0 indices.");
		BX_ASSERT(
			  !_index32 || 0 != (g_caps.supported & MAX_CAPS_INDEX32)
			, "32-bit indices are not supported. Use max::getCaps to check MAX_CAPS_INDEX32 backend renderer capabilities."
			);

		s_ctx->allocTransientIndexBuffer(_tib, _num, _index32);

		const uint32_t indexSize = _tib->isIndex16 ? 2 : 4;
		BX_ASSERT(_num == _tib->size/ indexSize
			, "Failed to allocate transient index buffer (requested %d, available %d). "
			  "Use max::getAvailTransient* functions to ensure availability."
			, _num
			, _tib->size/indexSize
			);
		BX_UNUSED(indexSize);
	}

	void allocTransientVertexBuffer(TransientVertexBuffer* _tvb, uint32_t _num, const VertexLayout& _layout)
	{
		BX_ASSERT(NULL != _tvb, "_tvb can't be NULL");
		BX_ASSERT(0 < _num, "Requesting 0 vertices.");
		BX_ASSERT(isValid(_layout), "Invalid VertexLayout.");

		VertexLayoutHandle layoutHandle;
		{
			MAX_MUTEX_SCOPE(s_ctx->m_resourceApiLock);
			layoutHandle = s_ctx->findOrCreateVertexLayout(_layout, true);
		}
		BX_ASSERT(isValid(layoutHandle), "Failed to allocate vertex layout handle (MAX_CONFIG_MAX_VERTEX_LAYOUTS, max: %d).", MAX_CONFIG_MAX_VERTEX_LAYOUTS);

		s_ctx->allocTransientVertexBuffer(_tvb, _num, layoutHandle, _layout.m_stride);

		BX_ASSERT(_num == _tvb->size / _layout.m_stride
			, "Failed to allocate transient vertex buffer (requested %d, available %d). "
			  "Use max::getAvailTransient* functions to ensure availability."
			, _num
			, _tvb->size / _layout.m_stride
			);
	}

	bool allocTransientBuffers(max::TransientVertexBuffer* _tvb, const max::VertexLayout& _layout, uint32_t _numVertices, max::TransientIndexBuffer* _tib, uint32_t _numIndices, bool _index32)
	{
		MAX_MUTEX_SCOPE(s_ctx->m_resourceApiLock);

		if (_numVertices == getAvailTransientVertexBuffer(_numVertices, _layout)
		&&  _numIndices  == getAvailTransientIndexBuffer(_numIndices, _index32) )
		{
			allocTransientVertexBuffer(_tvb, _numVertices, _layout);
			allocTransientIndexBuffer(_tib, _numIndices, _index32);
			return true;
		}

		return false;
	}

	void allocInstanceDataBuffer(InstanceDataBuffer* _idb, uint32_t _num, uint16_t _stride)
	{
		MAX_CHECK_CAPS(MAX_CAPS_INSTANCING, "Instancing is not supported!");
		BX_ASSERT(bx::isAligned(_stride, 16), "Stride must be multiple of 16.");
		BX_ASSERT(0 < _num, "Requesting 0 instanced data vertices.");
		s_ctx->allocInstanceDataBuffer(_idb, _num, _stride);
		BX_ASSERT(_num == _idb->size / _stride
			, "Failed to allocate instance data buffer (requested %d, available %d). "
			  "Use max::getAvailTransient* functions to ensure availability."
			, _num
			, _idb->size / _stride
			);
	}

	IndirectBufferHandle createIndirectBuffer(uint32_t _num)
	{
		return s_ctx->createIndirectBuffer(_num);
	}

	void destroy(IndirectBufferHandle _handle)
	{
		s_ctx->destroyIndirectBuffer(_handle);
	}

	ShaderHandle createShader(const Memory* _mem)
	{
		BX_ASSERT(NULL != _mem, "_mem can't be NULL");
		return s_ctx->createShader(_mem);
	}

	ShaderHandle loadShader(const char* _name)
	{
		BX_ASSERT(NULL != _name, "_name can't be NULL");

		bx::FilePath filePath("shaders/");

		switch (max::getRendererType())
		{
		case max::RendererType::Noop:
		case max::RendererType::Direct3D11:
		case max::RendererType::Direct3D12: filePath.join("dx11");  break;
		case max::RendererType::Agc:
		case max::RendererType::Gnm:        filePath.join("pssl");  break;
		case max::RendererType::Metal:      filePath.join("metal"); break;
		case max::RendererType::Nvn:        filePath.join("nvn");   break;
		case max::RendererType::OpenGL:     filePath.join("glsl");  break;
		case max::RendererType::OpenGLES:   filePath.join("essl");  break;
		case max::RendererType::Vulkan:     filePath.join("spirv"); break;

		case max::RendererType::Count:
			return MAX_INVALID_HANDLE;
		}

		char fileName[512];
		bx::strCopy(fileName, BX_COUNTOF(fileName), _name);
		bx::strCat(fileName, BX_COUNTOF(fileName), ".bin");

		filePath.join(fileName);

		const Memory* mem = loadMemory(filePath.getCPtr());
		return s_ctx->createShader(mem);
	}

	uint16_t getShaderUniforms(ShaderHandle _handle, UniformHandle* _uniforms, uint16_t _max)
	{
		BX_WARN(NULL == _uniforms || 0 != _max
			, "Passing uniforms array pointer, but array maximum capacity is set to 0."
			);

		uint16_t num = s_ctx->getShaderUniforms(_handle, _uniforms, _max);

		BX_WARN(0 == _max || num <= _max
			, "Shader has more uniforms that capacity of output array. Output is truncated (num %d, max %d)."
			, num
			, _max
			);

		return num;
	}

	void setName(ShaderHandle _handle, const char* _name, int32_t _len)
	{
		s_ctx->setName(_handle, bx::StringView(_name, _len) );
	}

	void destroy(ShaderHandle _handle)
	{
		s_ctx->destroyShader(_handle);
	}

	ProgramHandle createProgram(ShaderHandle _vsh, ShaderHandle _fsh, bool _destroyShaders)
	{
		if (!isValid(_fsh) )
		{
			return createProgram(_vsh, _destroyShaders);
		}

		return s_ctx->createProgram(_vsh, _fsh, _destroyShaders);
	}

	ProgramHandle createProgram(ShaderHandle _csh, bool _destroyShader)
	{
		return s_ctx->createProgram(_csh, _destroyShader);
	}

	ProgramHandle loadProgram(const char* _vsName, const char* _fsName)
	{
		return s_ctx->createProgram(loadShader(_vsName), loadShader(_fsName), true);
	}

	void destroy(ProgramHandle _handle)
	{
		s_ctx->destroyProgram(_handle);
	}

	void isFrameBufferValid(uint8_t _num, const Attachment* _attachment, bx::Error* _err)
	{
		BX_ERROR_SCOPE(_err, "Frame buffer validation");

		uint8_t color = 0;
		uint8_t depth = 0;

		const TextureRef& firstTexture = s_ctx->m_textureRef[_attachment[0].handle.idx];

		const uint16_t firstAttachmentWidth  = bx::max<uint16_t>(firstTexture.m_width  >> _attachment[0].mip, 1);
		const uint16_t firstAttachmentHeight = bx::max<uint16_t>(firstTexture.m_height >> _attachment[0].mip, 1);

		for (uint32_t ii = 0; ii < _num; ++ii)
		{
			const Attachment&   at = _attachment[ii];
			const TextureHandle texHandle = at.handle;
			const TextureRef&   tr = s_ctx->m_textureRef[texHandle.idx];

			MAX_ERROR_CHECK(true
				&& isValid(texHandle)
				&& s_ctx->m_textureHandle.isValid(texHandle.idx)
				, _err
				, MAX_ERROR_FRAME_BUFFER_VALIDATION
				, "Invalid texture attachment."
				, "Attachment %d, texture handle %d."
				, ii
				, texHandle.idx
				);

			MAX_ERROR_CHECK(true
				&& at.mip < tr.m_numMips
				, _err
				, MAX_ERROR_FRAME_BUFFER_VALIDATION
				, "Invalid texture mip level."
				, "Attachment %d, Mip %d, texture (handle %d) number of mips %d."
				, ii
				, at.mip
				, texHandle.idx
				, tr.m_numMips
				);

			{
				const uint16_t numLayers = tr.is3D()
					? bx::max<uint16_t>(tr.m_depth >> at.mip, 1)
					: tr.m_numLayers * (tr.isCubeMap() ? 6 : 1)
					;

				MAX_ERROR_CHECK(true
					&& (at.layer + at.numLayers) <= numLayers
					, _err
					, MAX_ERROR_FRAME_BUFFER_VALIDATION
					, "Invalid texture layer range."
					, "Attachment %d, Layer: %d, Num: %d, Max number of layers: %d."
					, ii
					, at.layer
					, at.numLayers
					, numLayers
					);
			}

			MAX_ERROR_CHECK(true
				&& _attachment[0].numLayers == at.numLayers
				, _err
				, MAX_ERROR_FRAME_BUFFER_VALIDATION
				, "Mismatch in attachment layer count."
				, "Attachment %d, Given: %d, Expected: %d."
				, ii
				, at.numLayers
				, _attachment[0].numLayers
				);

			MAX_ERROR_CHECK(true
				&& firstTexture.m_bbRatio == tr.m_bbRatio
				, _err
				, MAX_ERROR_FRAME_BUFFER_VALIDATION
				, "Mismatch in texture back-buffer ratio."
				, "Attachment %d, Given: %d, Expected: %d."
				, ii
				, tr.m_bbRatio
				, firstTexture.m_bbRatio
				);

			MAX_ERROR_CHECK(true
				&& firstTexture.m_numSamples == tr.m_numSamples
				, _err
				, MAX_ERROR_FRAME_BUFFER_VALIDATION
				, "Mismatch in texture sample count."
				, "Attachment %d, Given: %d, Expected: %d."
				, ii
				, tr.m_numSamples
				, firstTexture.m_numSamples
				);

			if (BackbufferRatio::Count == firstTexture.m_bbRatio)
			{
				const uint16_t width  = bx::max<uint16_t>(tr.m_width  >> at.mip, 1);
				const uint16_t height = bx::max<uint16_t>(tr.m_height >> at.mip, 1);

				MAX_ERROR_CHECK(true
					&& width  == firstAttachmentWidth
					&& height == firstAttachmentHeight
					, _err
					, MAX_ERROR_FRAME_BUFFER_VALIDATION
					, "Mismatch in texture size."
					, "Attachment %d, Given: %dx%d, Expected: %dx%d."
					, ii
					, width
					, height
					, firstAttachmentWidth
					, firstAttachmentHeight
					);
			}

			if (bimg::isDepth(bimg::TextureFormat::Enum(tr.m_format) ) )
			{
				++depth;

				MAX_ERROR_CHECK(
					// if MAX_TEXTURE_RT_MSAA_X2 or greater than MAX_TEXTURE_RT_WRITE_ONLY is required
					// if MAX_TEXTURE_RT with no MSSA then WRITE_ONLY is not required.
					(1 == ((tr.m_flags & MAX_TEXTURE_RT_MSAA_MASK) >> MAX_TEXTURE_RT_MSAA_SHIFT))
					|| (0 != (tr.m_flags & MAX_TEXTURE_RT_WRITE_ONLY))
					, _err
					, MAX_ERROR_FRAME_BUFFER_VALIDATION
					, "Frame buffer depth MSAA texture cannot be resolved. It must be created with `MAX_TEXTURE_RT_WRITE_ONLY` flag."
					, "Attachment %d, texture flags 0x%016" PRIx64 "."
					, ii
					, tr.m_flags
					);
			}
			else
			{
				++color;
			}

			MAX_ERROR_CHECK(true
				&& 0 == (tr.m_flags & MAX_TEXTURE_READ_BACK)
				, _err
				, MAX_ERROR_FRAME_BUFFER_VALIDATION
				, "Frame buffer texture cannot be created with `MAX_TEXTURE_READ_BACK`."
				, "Attachment %d, texture flags 0x%016" PRIx64 "."
				, ii
				, tr.m_flags
				);

			MAX_ERROR_CHECK(true
				&& 0 != (tr.m_flags & MAX_TEXTURE_RT_MASK)
				, _err
				, MAX_ERROR_FRAME_BUFFER_VALIDATION
				, "Frame buffer texture is not created with one of `MAX_TEXTURE_RT*` flags."
				, "Attachment %d, texture flags 0x%016" PRIx64 "."
				, ii
				, tr.m_flags
				);
		}

		MAX_ERROR_CHECK(true
			&& color <= g_caps.limits.maxFBAttachments
			, _err
			, MAX_ERROR_FRAME_BUFFER_VALIDATION
			, "Too many frame buffer color attachments."
			, "Num: %d, Max: %d."
			, _num
			, g_caps.limits.maxFBAttachments
			);

		MAX_ERROR_CHECK(true
			&& depth <= 1
			, _err
			, MAX_ERROR_FRAME_BUFFER_VALIDATION
			, "There can be only one depth texture attachment."
			, "Num depth attachments %d."
			, depth
			);
	}

	bool isFrameBufferValid(uint8_t _num, const Attachment* _attachment)
	{
		MAX_MUTEX_SCOPE(s_ctx->m_resourceApiLock);
		bx::Error err;
		isFrameBufferValid(_num, _attachment, &err);
		return err.isOk();
	}

	static void isTextureValid(uint16_t _width, uint16_t _height, uint16_t _depth, bool _cubeMap, uint16_t _numLayers, TextureFormat::Enum _format, uint64_t _flags, bx::Error* _err)
	{
		BX_ERROR_SCOPE(_err, "Texture validation");

		const bool is3DTexture = 1 < _depth;

		MAX_ERROR_CHECK(false
			|| !_cubeMap
			|| !is3DTexture
			, _err
			, MAX_ERROR_TEXTURE_VALIDATION
			, "Texture can't be 3D and cube map at the same time."
			, ""
			);

		MAX_ERROR_CHECK(false
			|| !is3DTexture
			|| 0 != (g_caps.supported & MAX_CAPS_TEXTURE_3D)
			, _err
			, MAX_ERROR_TEXTURE_VALIDATION
			, "Texture3D is not supported! "
			  "Use max::getCaps to check `MAX_CAPS_TEXTURE_3D` backend renderer capabilities."
			, ""
			);

		MAX_ERROR_CHECK(false
			|| _width  <= g_caps.limits.maxTextureSize
			|| _height <= g_caps.limits.maxTextureSize
			, _err
			, MAX_ERROR_TEXTURE_VALIDATION
			, "Requested texture width/height is above the `maxTextureSize` limit."
			, "Texture width x height requested %d x %d (Max: %d)."
			, _width
			, _height
			, g_caps.limits.maxTextureSize
			);

		MAX_ERROR_CHECK(false
			|| 0 == (_flags & MAX_TEXTURE_RT_MASK)
			|| 0 == (_flags & MAX_TEXTURE_READ_BACK)
			, _err
			, MAX_ERROR_TEXTURE_VALIDATION
			, "Can't create render target with `MAX_TEXTURE_READ_BACK` flag."
			, ""
			);

		MAX_ERROR_CHECK(false
			|| 0 == (_flags & MAX_TEXTURE_COMPUTE_WRITE)
			|| 0 == (_flags & MAX_TEXTURE_READ_BACK)
			, _err
			, MAX_ERROR_TEXTURE_VALIDATION
			, "Can't create compute texture with `MAX_TEXTURE_READ_BACK` flag."
			, ""
			);

		MAX_ERROR_CHECK(false
			|| 1 >= _numLayers
			|| 0 != (g_caps.supported & MAX_CAPS_TEXTURE_2D_ARRAY)
			, _err
			, MAX_ERROR_TEXTURE_VALIDATION
			, "Texture array is not supported! "
			  "Use max::getCaps to check `MAX_CAPS_TEXTURE_2D_ARRAY` backend renderer capabilities."
			, ""
			);

		MAX_ERROR_CHECK(false
			|| _numLayers <= g_caps.limits.maxTextureLayers
			, _err
			, MAX_ERROR_TEXTURE_VALIDATION
			, "Requested number of texture array layers is above the `maxTextureLayers` limit."
			, "Number of texture array layers requested %d (Max: %d)."
			, _numLayers
			, g_caps.limits.maxTextureLayers
			);

		bool formatSupported;
		if (0 != (_flags & (MAX_TEXTURE_RT | MAX_TEXTURE_RT_WRITE_ONLY)) )
		{
			formatSupported = 0 != (g_caps.formats[_format] & MAX_CAPS_FORMAT_TEXTURE_FRAMEBUFFER);
		}
		else
		{
			formatSupported = 0 != (g_caps.formats[_format] & (0
				| MAX_CAPS_FORMAT_TEXTURE_2D
				| MAX_CAPS_FORMAT_TEXTURE_2D_EMULATED
				| MAX_CAPS_FORMAT_TEXTURE_2D_SRGB
				) );
		}

		uint16_t srgbCaps = MAX_CAPS_FORMAT_TEXTURE_2D_SRGB;

		if (_cubeMap)
		{
			formatSupported = 0 != (g_caps.formats[_format] & (0
				| MAX_CAPS_FORMAT_TEXTURE_CUBE
				| MAX_CAPS_FORMAT_TEXTURE_CUBE_EMULATED
				| MAX_CAPS_FORMAT_TEXTURE_CUBE_SRGB
				) );
			srgbCaps = MAX_CAPS_FORMAT_TEXTURE_CUBE_SRGB;
		}
		else if (is3DTexture)
		{
			formatSupported = 0 != (g_caps.formats[_format] & (0
				| MAX_CAPS_FORMAT_TEXTURE_3D
				| MAX_CAPS_FORMAT_TEXTURE_3D_EMULATED
				| MAX_CAPS_FORMAT_TEXTURE_3D_SRGB
				) );
			srgbCaps = MAX_CAPS_FORMAT_TEXTURE_3D_SRGB;
		}

		if (formatSupported
		&&  0 != (_flags & MAX_TEXTURE_RT_MASK) )
		{
			formatSupported = 0 != (g_caps.formats[_format] & (0
				| MAX_CAPS_FORMAT_TEXTURE_FRAMEBUFFER
				) );
		}

		MAX_ERROR_CHECK(
			  formatSupported
			, _err
			, MAX_ERROR_TEXTURE_VALIDATION
			, "Texture format is not supported! "
			  "Use max::isTextureValid to check support for texture format before creating it."
			, "Texture format: %s."
			, getName(_format)
			);

		MAX_ERROR_CHECK(false
			|| 0 == (_flags & MAX_TEXTURE_MSAA_SAMPLE)
			|| 0 != (g_caps.formats[_format] & MAX_CAPS_FORMAT_TEXTURE_MSAA)
			, _err
			, MAX_ERROR_TEXTURE_VALIDATION
			, "MSAA sampling for this texture format is not supported."
			, "Texture format: %s."
			, getName(_format)
			);

		MAX_ERROR_CHECK(false
			|| 0 == (_flags & MAX_TEXTURE_SRGB)
			|| 0 != (g_caps.formats[_format] & srgbCaps & (0
					| MAX_CAPS_FORMAT_TEXTURE_2D_SRGB
					| MAX_CAPS_FORMAT_TEXTURE_3D_SRGB
					| MAX_CAPS_FORMAT_TEXTURE_CUBE_SRGB
					) )
			, _err
			, MAX_ERROR_TEXTURE_VALIDATION
			, "sRGB sampling for this texture format is not supported."
			, "Texture format: %s."
			, getName(_format)
			);
	}

	bool isTextureValid(uint16_t _depth, bool _cubeMap, uint16_t _numLayers, TextureFormat::Enum _format, uint64_t _flags)
	{
		bx::Error err;
		isTextureValid(0, 0, _depth, _cubeMap, _numLayers, _format, _flags, &err);
		return err.isOk();
	}

	void isIdentifierValid(const bx::StringView& _name, bx::Error* _err)
	{
		BX_ERROR_SCOPE(_err, "Uniform identifier validation");

		MAX_ERROR_CHECK(false
			|| !_name.isEmpty()
			, _err
			, MAX_ERROR_IDENTIFIER_VALIDATION
			, "Identifier can't be empty."
			, ""
			);

		MAX_ERROR_CHECK(false
			|| PredefinedUniform::Count == nameToPredefinedUniformEnum(_name)
			, _err
			, MAX_ERROR_IDENTIFIER_VALIDATION
			, "Identifier can't use predefined uniform name."
			, ""
			);

		const char ch = *_name.getPtr();
		MAX_ERROR_CHECK(false
			|| bx::isAlpha(ch)
			|| '_' == ch
			, _err
			, MAX_ERROR_IDENTIFIER_VALIDATION
			, "The first character of an identifier should be either an alphabet character or an underscore."
			, ""
			);

		bool result = true;

		for (const char* ptr = _name.getPtr() + 1, *term = _name.getTerm()
			; ptr != term && result
			; ++ptr
			)
		{
			result &= bx::isAlphaNum(*ptr) || '_' == *ptr;
		}

		MAX_ERROR_CHECK(false
			|| result
			, _err
			, MAX_ERROR_IDENTIFIER_VALIDATION
			, "Identifier contains invalid characters. Identifier must be the alphabet character, number, or underscore."
			, ""
			);
	}

	void calcTextureSize(TextureInfo& _info, uint16_t _width, uint16_t _height, uint16_t _depth, bool _cubeMap, bool _hasMips, uint16_t _numLayers, TextureFormat::Enum _format)
	{
		bimg::imageGetSize( (bimg::TextureInfo*)&_info, _width, _height, _depth, _cubeMap, _hasMips, _numLayers, bimg::TextureFormat::Enum(_format) );
	}

	TextureHandle createTexture(const Memory* _mem, uint64_t _flags, uint8_t _skip, TextureInfo* _info)
	{
		BX_ASSERT(NULL != _mem, "_mem can't be NULL");
		return s_ctx->createTexture(_mem, _flags, _skip, _info, BackbufferRatio::Count, false);
	}

	void getTextureSizeFromRatio(BackbufferRatio::Enum _ratio, uint16_t& _width, uint16_t& _height)
	{
		switch (_ratio)
		{
		case BackbufferRatio::Half:      _width /=  2; _height /=  2; break;
		case BackbufferRatio::Quarter:   _width /=  4; _height /=  4; break;
		case BackbufferRatio::Eighth:    _width /=  8; _height /=  8; break;
		case BackbufferRatio::Sixteenth: _width /= 16; _height /= 16; break;
		case BackbufferRatio::Double:    _width *=  2; _height *=  2; break;

		default:
			break;
		}

		_width  = bx::max<uint16_t>(1, _width);
		_height = bx::max<uint16_t>(1, _height);
	}

	static TextureHandle createTexture2D(BackbufferRatio::Enum _ratio, uint16_t _width, uint16_t _height, bool _hasMips, uint16_t _numLayers, TextureFormat::Enum _format, uint64_t _flags, const Memory* _mem)
	{
		if (BackbufferRatio::Count != _ratio)
		{
			_width  = uint16_t(s_ctx->m_init.resolution.width);
			_height = uint16_t(s_ctx->m_init.resolution.height);
			getTextureSizeFromRatio(_ratio, _width, _height);
		}

		bx::ErrorAssert err;
		isTextureValid(_width, _height, 0, false, _numLayers, _format, _flags, &err);

		if (!err.isOk() )
		{
			return MAX_INVALID_HANDLE;
		}

		const uint8_t numMips = calcNumMips(_hasMips, _width, _height);
		_numLayers = bx::max<uint16_t>(_numLayers, 1);

		if (BX_ENABLED(MAX_CONFIG_DEBUG)
		&&  NULL != _mem)
		{
			TextureInfo ti;
			calcTextureSize(ti, _width, _height, 1, false, _hasMips, _numLayers, _format);
			BX_ASSERT(ti.storageSize == _mem->size
				, "createTexture2D: Texture storage size doesn't match passed memory size (storage size: %d, memory size: %d)"
				, ti.storageSize
				, _mem->size
				);
		}

		uint32_t size = sizeof(uint32_t)+sizeof(TextureCreate);
		const Memory* mem = alloc(size);

		bx::StaticMemoryBlockWriter writer(mem->data, mem->size);
		uint32_t magic = MAX_CHUNK_MAGIC_TEX;
		bx::write(&writer, magic, bx::ErrorAssert{});

		TextureCreate tc;
		tc.m_width     = _width;
		tc.m_height    = _height;
		tc.m_depth     = 0;
		tc.m_numLayers = _numLayers;
		tc.m_numMips   = numMips;
		tc.m_format    = _format;
		tc.m_cubeMap   = false;
		tc.m_mem       = _mem;
		bx::write(&writer, tc, bx::ErrorAssert{});

		return s_ctx->createTexture(mem, _flags, 0, NULL, _ratio, NULL != _mem);
	}

	TextureHandle createTexture2D(uint16_t _width, uint16_t _height, bool _hasMips, uint16_t _numLayers, TextureFormat::Enum _format, uint64_t _flags, const Memory* _mem)
	{
		BX_ASSERT(_width > 0 && _height > 0, "Invalid texture size (width %d, height %d).", _width, _height);
		return createTexture2D(BackbufferRatio::Count, _width, _height, _hasMips, _numLayers, _format, _flags, _mem);
	}

	TextureHandle createTexture2D(BackbufferRatio::Enum _ratio, bool _hasMips, uint16_t _numLayers, TextureFormat::Enum _format, uint64_t _flags)
	{
		BX_ASSERT(_ratio < BackbufferRatio::Count, "Invalid back buffer ratio.");
		return createTexture2D(_ratio, 0, 0, _hasMips, _numLayers, _format, _flags, NULL);
	}

	TextureHandle createTexture3D(uint16_t _width, uint16_t _height, uint16_t _depth, bool _hasMips, TextureFormat::Enum _format, uint64_t _flags, const Memory* _mem)
	{
		bx::ErrorAssert err;
		isTextureValid(_width, _height, _depth, false, 1, _format, _flags, &err);

		if (!err.isOk() )
		{
			return MAX_INVALID_HANDLE;
		}

		const uint8_t numMips = calcNumMips(_hasMips, _width, _height, _depth);

		if (BX_ENABLED(MAX_CONFIG_DEBUG)
		&&  NULL != _mem)
		{
			TextureInfo ti;
			calcTextureSize(ti, _width, _height, _depth, false, _hasMips, 1, _format);
			BX_ASSERT(ti.storageSize == _mem->size
				, "createTexture3D: Texture storage size doesn't match passed memory size (storage size: %d, memory size: %d)"
				, ti.storageSize
				, _mem->size
				);
		}

		uint32_t size = sizeof(uint32_t)+sizeof(TextureCreate);
		const Memory* mem = alloc(size);

		bx::StaticMemoryBlockWriter writer(mem->data, mem->size);
		uint32_t magic = MAX_CHUNK_MAGIC_TEX;
		bx::write(&writer, magic, bx::ErrorAssert{});

		TextureCreate tc;
		tc.m_width     = _width;
		tc.m_height    = _height;
		tc.m_depth     = _depth;
		tc.m_numLayers = 1;
		tc.m_numMips   = numMips;
		tc.m_format    = _format;
		tc.m_cubeMap   = false;
		tc.m_mem       = _mem;
		bx::write(&writer, tc, bx::ErrorAssert{});

		return s_ctx->createTexture(mem, _flags, 0, NULL, BackbufferRatio::Count, NULL != _mem);
	}

	TextureHandle createTextureCube(uint16_t _size, bool _hasMips, uint16_t _numLayers, TextureFormat::Enum _format, uint64_t _flags, const Memory* _mem)
	{
		bx::ErrorAssert err;
		isTextureValid(_size, _size, 0, true, _numLayers, _format, _flags, &err);

		if (!err.isOk() )
		{
			return MAX_INVALID_HANDLE;
		}

		const uint8_t numMips = calcNumMips(_hasMips, _size, _size);
		_numLayers = bx::max<uint16_t>(_numLayers, 1);

		if (BX_ENABLED(MAX_CONFIG_DEBUG)
		&&  NULL != _mem)
		{
			TextureInfo ti;
			calcTextureSize(ti, _size, _size, 1, true, _hasMips, _numLayers, _format);
			BX_ASSERT(ti.storageSize == _mem->size
				, "createTextureCube: Texture storage size doesn't match passed memory size (storage size: %d, memory size: %d)"
				, ti.storageSize
				, _mem->size
				);
		}

		uint32_t size = sizeof(uint32_t)+sizeof(TextureCreate);
		const Memory* mem = alloc(size);

		bx::StaticMemoryBlockWriter writer(mem->data, mem->size);
		uint32_t magic = MAX_CHUNK_MAGIC_TEX;
		bx::write(&writer, magic, bx::ErrorAssert{});

		TextureCreate tc;
		tc.m_width     = _size;
		tc.m_height    = _size;
		tc.m_depth     = 0;
		tc.m_numLayers = _numLayers;
		tc.m_numMips   = numMips;
		tc.m_format    = _format;
		tc.m_cubeMap   = true;
		tc.m_mem       = _mem;
		bx::write(&writer, tc, bx::ErrorAssert{});

		return s_ctx->createTexture(mem, _flags, 0, NULL, BackbufferRatio::Count, NULL != _mem);
	}

	static void imageReleaseCb(void* _ptr, void* _userData)
	{
		BX_UNUSED(_ptr);
		bimg::ImageContainer* imageContainer = (bimg::ImageContainer*)_userData;
		bimg::imageFree(imageContainer);
	}

	TextureHandle loadTexture(
		const char* _filePath,
		uint64_t _flags,
		uint8_t _skip,
		TextureInfo* _info,
		Orientation::Enum* _orientation
	)
	{
		BX_UNUSED(_skip);
		max::TextureHandle handle = MAX_INVALID_HANDLE;

		uint32_t size;
		void* data = load(_filePath, &size);
		if (NULL != data)
		{
			bimg::ImageContainer* imageContainer = bimg::imageParse(g_allocator, data, size);

			if (NULL != imageContainer)
			{
				if (NULL != _orientation)
				{
					*_orientation = (Orientation::Enum)imageContainer->m_orientation;
				}

				const max::Memory* mem = max::makeRef(
					imageContainer->m_data
					, imageContainer->m_size
					, imageReleaseCb
					, imageContainer
				);
				bx::free(g_allocator, data);

				if (NULL != _info)
				{
					max::calcTextureSize(
						*_info
						, uint16_t(imageContainer->m_width)
						, uint16_t(imageContainer->m_height)
						, uint16_t(imageContainer->m_depth)
						, imageContainer->m_cubeMap
						, 1 < imageContainer->m_numMips
						, imageContainer->m_numLayers
						, max::TextureFormat::Enum(imageContainer->m_format)
					);
				}

				if (imageContainer->m_cubeMap)
				{
					handle = max::createTextureCube(
						uint16_t(imageContainer->m_width)
						, 1 < imageContainer->m_numMips
						, imageContainer->m_numLayers
						, max::TextureFormat::Enum(imageContainer->m_format)
						, _flags
						, mem
					);
				}
				else if (1 < imageContainer->m_depth)
				{
					handle = max::createTexture3D(
						uint16_t(imageContainer->m_width)
						, uint16_t(imageContainer->m_height)
						, uint16_t(imageContainer->m_depth)
						, 1 < imageContainer->m_numMips
						, max::TextureFormat::Enum(imageContainer->m_format)
						, _flags
						, mem
					);
				}
				else if (max::isTextureValid(0, false, imageContainer->m_numLayers, max::TextureFormat::Enum(imageContainer->m_format), _flags))
				{
					handle = max::createTexture2D(
						uint16_t(imageContainer->m_width)
						, uint16_t(imageContainer->m_height)
						, 1 < imageContainer->m_numMips
						, imageContainer->m_numLayers
						, max::TextureFormat::Enum(imageContainer->m_format)
						, _flags
						, mem
					);
				}

				if (max::isValid(handle))
				{
					const bx::StringView name(_filePath);
					max::setName(handle, name.getPtr(), name.getLength());
				}
			}
		}

		return handle;
	}

	bimg::ImageContainer* loadImage(const char* _filePath, TextureFormat::Enum _dstFormat)
	{
		uint32_t size = 0;
		void* data = load(_filePath, &size);
		return bimg::imageParse(g_allocator, data, size, bimg::TextureFormat::Enum(_dstFormat));
	}

	void setName(TextureHandle _handle, const char* _name, int32_t _len)
	{
		s_ctx->setName(_handle, bx::StringView(_name, _len) );
	}

	void* getDirectAccessPtr(TextureHandle _handle)
	{
		return s_ctx->getDirectAccessPtr(_handle);
	}

	void destroy(TextureHandle _handle)
	{
		s_ctx->destroyTexture(_handle);
	}

	void updateTexture2D(TextureHandle _handle, uint16_t _layer, uint8_t _mip, uint16_t _x, uint16_t _y, uint16_t _width, uint16_t _height, const Memory* _mem, uint16_t _pitch)
	{
		BX_ASSERT(NULL != _mem, "_mem can't be NULL");
		if (_width  == 0
		||  _height == 0)
		{
			release(_mem);
		}
		else
		{
			s_ctx->updateTexture(_handle, 0, _mip, _x, _y, _layer, _width, _height, 1, _pitch, _mem);
		}
	}

	void updateTexture3D(TextureHandle _handle, uint8_t _mip, uint16_t _x, uint16_t _y, uint16_t _z, uint16_t _width, uint16_t _height, uint16_t _depth, const Memory* _mem)
	{
		BX_ASSERT(NULL != _mem, "_mem can't be NULL");
		MAX_CHECK_CAPS(MAX_CAPS_TEXTURE_3D, "Texture3D is not supported!");

		if (0 == _width
		||  0 == _height
		||  0 == _depth)
		{
			release(_mem);
		}
		else
		{
			s_ctx->updateTexture(_handle, 0, _mip, _x, _y, _z, _width, _height, _depth, UINT16_MAX, _mem);
		}
	}

	void updateTextureCube(TextureHandle _handle, uint16_t _layer, uint8_t _side, uint8_t _mip, uint16_t _x, uint16_t _y, uint16_t _width, uint16_t _height, const Memory* _mem, uint16_t _pitch)
	{
		BX_ASSERT(NULL != _mem, "_mem can't be NULL");
		BX_ASSERT(_side <= 5, "Invalid side %d.", _side);
		if (0 == _width
		||  0 == _height)
		{
			release(_mem);
		}
		else
		{
			s_ctx->updateTexture(_handle, _side, _mip, _x, _y, _layer, _width, _height, 1, _pitch, _mem);
		}
	}

	uint32_t readTexture(TextureHandle _handle, void* _data, uint8_t _mip)
	{
		BX_ASSERT(NULL != _data, "_data can't be NULL");
		MAX_CHECK_CAPS(MAX_CAPS_TEXTURE_READ_BACK, "Texture read-back is not supported!");
		return s_ctx->readTexture(_handle, _data, _mip);
	}

	FrameBufferHandle createFrameBuffer(uint16_t _width, uint16_t _height, TextureFormat::Enum _format, uint64_t _textureFlags)
	{
		_textureFlags |= _textureFlags&MAX_TEXTURE_RT_MSAA_MASK ? 0 : MAX_TEXTURE_RT;
		TextureHandle th = createTexture2D(_width, _height, false, 1, _format, _textureFlags);
		return createFrameBuffer(1, &th, true);
	}

	FrameBufferHandle createFrameBuffer(BackbufferRatio::Enum _ratio, TextureFormat::Enum _format, uint64_t _textureFlags)
	{
		BX_ASSERT(_ratio < BackbufferRatio::Count, "Invalid back buffer ratio.");
		_textureFlags |= _textureFlags&MAX_TEXTURE_RT_MSAA_MASK ? 0 : MAX_TEXTURE_RT;
		TextureHandle th = createTexture2D(_ratio, false, 1, _format, _textureFlags);
		return createFrameBuffer(1, &th, true);
	}

	FrameBufferHandle createFrameBuffer(uint8_t _num, const TextureHandle* _handles, bool _destroyTextures)
	{
		Attachment attachment[MAX_CONFIG_MAX_FRAME_BUFFER_ATTACHMENTS];
		for (uint8_t ii = 0; ii < _num; ++ii)
		{
			Attachment& at = attachment[ii];
			at.init(_handles[ii], Access::Write, 0, 1, 0, MAX_RESOLVE_AUTO_GEN_MIPS);
		}
		return createFrameBuffer(_num, attachment, _destroyTextures);
	}

	FrameBufferHandle createFrameBuffer(uint8_t _num, const Attachment* _attachment, bool _destroyTextures)
	{
		BX_ASSERT(_num != 0, "Number of frame buffer attachments can't be 0.");
		BX_ASSERT(_num <= MAX_CONFIG_MAX_FRAME_BUFFER_ATTACHMENTS
			, "Number of frame buffer attachments is larger than allowed %d (max: %d)."
			, _num
			, MAX_CONFIG_MAX_FRAME_BUFFER_ATTACHMENTS
			);
		BX_ASSERT(NULL != _attachment, "_attachment can't be NULL");
		return s_ctx->createFrameBuffer(_num, _attachment, _destroyTextures);
	}

	FrameBufferHandle createFrameBuffer(void* _nwh, uint16_t _width, uint16_t _height, TextureFormat::Enum _format, TextureFormat::Enum _depthFormat)
	{
		MAX_CHECK_CAPS(MAX_CAPS_SWAP_CHAIN, "Swap chain is not supported!");
		BX_WARN(_width > 0 && _height > 0
			, "Invalid frame buffer dimensions (width %d, height %d)."
			, _width
			, _height
			);
		BX_ASSERT(_format == TextureFormat::Count || bimg::isColor(bimg::TextureFormat::Enum(_format) )
			, "Invalid texture format for color (%s)."
			, bimg::getName(bimg::TextureFormat::Enum(_format) )
			);
		BX_ASSERT(_depthFormat == TextureFormat::Count || bimg::isDepth(bimg::TextureFormat::Enum(_depthFormat) )
			, "Invalid texture format for depth (%s)."
			, bimg::getName(bimg::TextureFormat::Enum(_depthFormat) )
			);
		return s_ctx->createFrameBuffer(
			  _nwh
			, bx::max<uint16_t>(_width, 1)
			, bx::max<uint16_t>(_height, 1)
			, _format
			, _depthFormat
			);
	}

	void setName(FrameBufferHandle _handle, const char* _name, int32_t _len)
	{
		s_ctx->setName(_handle, bx::StringView(_name, _len) );
	}

	TextureHandle getTexture(FrameBufferHandle _handle, uint8_t _attachment)
	{
		return s_ctx->getTexture(_handle, _attachment);
	}

	void destroy(FrameBufferHandle _handle)
	{
		s_ctx->destroyFrameBuffer(_handle);
	}

	UniformHandle createUniform(const char* _name, UniformType::Enum _type, uint16_t _num)
	{
		return s_ctx->createUniform(_name, _type, _num);
	}

	void getUniformInfo(UniformHandle _handle, UniformInfo& _info)
	{
		s_ctx->getUniformInfo(_handle, _info);
	}

	void destroy(UniformHandle _handle)
	{
		s_ctx->destroyUniform(_handle);
	}


	MaterialHandle createMaterial(ProgramHandle _program)
	{
		return s_ctx->createMaterial(_program);
	}
	
	void setMaterial(MaterialHandle _material)
	{
		s_ctx->setMaterial(_material);
	}

	void addParameter(MaterialHandle _material, const char* _name, float* _value, uint32_t _num)
	{
		s_ctx->addParameter(_material, _name, _value, _num);
	}

	void addParameter(MaterialHandle _material, const char* _name, uint32_t _stage, TextureHandle _texture)
	{
		s_ctx->addParameter(_material, _name, _stage, _texture);
	}

	void destroy(MaterialHandle _handle)
	{
		s_ctx->destroyMaterial(_handle);
	}

	MeshHandle createMesh(const Memory* _mem, bool _ramcopy)
	{
		return s_ctx->createMesh(_mem, _ramcopy);
	}

	MeshHandle createMesh(const Memory* _vertices, const Memory* _indices, const VertexLayout& _layout, bool _dynamic)
	{
		return s_ctx->createMesh(_vertices, _indices, _layout, _dynamic);
	}

	void update(MeshHandle _mesh, const Memory* _vertices, const Memory* _indices)
	{
		s_ctx->update(_mesh, _vertices, _indices);
	}

	MeshHandle loadMesh(const char* _filePath, bool _ramcopy)
	{
		const Memory* mem = loadMemory(_filePath);
		MeshHandle handle = s_ctx->createMesh(mem, _ramcopy);
		return handle;
	}

	MeshQuery* queryMesh(MeshHandle _handle)
	{
		return s_ctx->queryMesh(_handle);
	}

	void destroy(MeshHandle _handle)
	{
		s_ctx->destroyMesh(_handle);
	}

	ComponentHandle createComponent(void* _data, uint32_t _size)
	{
		return s_ctx->createComponent(_data, _size);
	}

	void destroy(ComponentHandle _component)
	{
		s_ctx->destroyComponent(_component);
	}

	EntityHandle createEntity(bool _destroyComponents)
	{
		return s_ctx->createEntity(_destroyComponents);
	}

	void addComponent(EntityHandle _entity, ComponentHandle _component, uint32_t _hash)
	{
		s_ctx->addComponent(_entity, _component, _hash);
	}

	void* getComponent(EntityHandle _handle, uint32_t _hash)
	{
		return s_ctx->getComponent(_handle, _hash);
	}

	EntityQuery* queryEntities(const HashQuery& _hashes)
	{
		return s_ctx->queryEntities(_hashes);
	}

	void destroy(EntityHandle _entity)
	{
		s_ctx->destroyEntity(_entity);
	}

	BodyHandle createBody(
		CollisionShape::Enum _shape,
		const bx::Vec3& _pos,
		const bx::Quaternion& _quat,
		const bx::Vec3& _scale,
		LayerType::Enum _layer,
		MotionType::Enum _motion,
		Activation::Enum _activation,
		float _maxVelocity,
		uint8_t _flags
	)
	{
		return s_ctx->createBody(_shape, _pos, _quat, _scale, _layer, _motion, _activation, _maxVelocity, _flags);
	}

	BodyHandle createBodySphere(
		const bx::Vec3& _pos,
		const bx::Quaternion& _quat,
		const float _radius,
		LayerType::Enum _layer,
		MotionType::Enum _motion,
		Activation::Enum _activation,
		float _maxVelocity,
		uint8_t _flags
	)
	{
		return s_ctx->createBody(CollisionShape::Sphere, _pos, _quat, { _radius , 0.0f, 0.0f }, _layer, _motion, _activation, _maxVelocity, _flags);
	}

	BodyHandle createBodyBox(
		const bx::Vec3& _pos,
		const bx::Quaternion& _quat,
		const bx::Vec3& _scale,
		LayerType::Enum _layer,
		MotionType::Enum _motion,
		Activation::Enum _activation,
		float _maxVelocity,
		uint8_t _flags
	)
	{
		return s_ctx->createBody(CollisionShape::Box, _pos, _quat, _scale, _layer, _motion, _activation, _maxVelocity, _flags);
	}

	BodyHandle createBodyCapsule(
		const bx::Vec3& _pos,
		const bx::Quaternion& _quat,
		const float _radius,
		const float _halfHeight,
		LayerType::Enum _layer,
		MotionType::Enum _motion,
		Activation::Enum _activation,
		float _maxVelocity,
		uint8_t _flags
	)
	{
		return s_ctx->createBody(CollisionShape::Capsule, _pos, _quat, { _radius, _halfHeight, 0.0f}, _layer, _motion, _activation, _maxVelocity, _flags);
	}

	void setPosition(BodyHandle _handle, const bx::Vec3& _pos, Activation::Enum _activation)
	{
		s_ctx->setPosition(_handle, _pos, _activation);
	}

	bx::Vec3 getPosition(BodyHandle _handle)
	{
		return s_ctx->getPosition(_handle);
	}

	void setRotation(BodyHandle _handle, const bx::Quaternion& _rot, Activation::Enum _activation)
	{
		s_ctx->setRotation(_handle, _rot, _activation);
	}

	bx::Quaternion getRotation(BodyHandle _handle)
	{
		return s_ctx->getRotation(_handle);
	}

	void setLinearVelocity(BodyHandle _handle, const bx::Vec3& _velocity)
	{
		s_ctx->setLinearVelocity(_handle, _velocity);
	}

	bx::Vec3 getLinearVelocity(BodyHandle _handle)
	{
		return s_ctx->getLinearVelocity(_handle);
	}

	void setAngularVelocity(BodyHandle _handle, const bx::Vec3& _angularVelocity)
	{
		s_ctx->setAngularVelocity(_handle, _angularVelocity);
	}

	bx::Vec3 getAngularVelocity(BodyHandle _handle)
	{
		return s_ctx->getAngularVelocity(_handle);
	}

	void addLinearAndAngularVelocity(BodyHandle _handle, const bx::Vec3& _linearVelocity, const bx::Vec3& _angularVelocity)
	{
		s_ctx->addLinearAndAngularVelocity(_handle, _linearVelocity, _angularVelocity);
	}

	void addLinearImpulse(BodyHandle _handle, const bx::Vec3& _impulse)
	{
		s_ctx->addLinearImpulse(_handle, _impulse);
	}

	void addAngularImpulse(BodyHandle _handle, const bx::Vec3& _impulse)
	{
		s_ctx->addAngularImpulse(_handle, _impulse);
	}

	void addBuoyancyImpulse(
		BodyHandle _handle,
		const bx::Vec3& _surfacePosition,
		const bx::Vec3& _surfaceNormal,
		float _buoyancy,
		float _linearDrag,
		float _angularDrag,
		const bx::Vec3& _fluidVelocity,
		const bx::Vec3& _gravity,
		float _deltaTime
	)
	{
		s_ctx->addBuoyancyImpulse(
			_handle,
			_surfacePosition,
			_surfaceNormal,
			_buoyancy,
			_linearDrag,
			_angularDrag,
			_fluidVelocity,
			_gravity,
			_deltaTime
		);
	}

	void addForce(BodyHandle _handle, const bx::Vec3& _force, Activation::Enum _activation)
	{
		s_ctx->addForce(_handle, _force, _activation);
	}

	void addTorque(BodyHandle _handle, const bx::Vec3& _torque, Activation::Enum _activation)
	{
		s_ctx->addTorque(_handle, _torque, _activation);
	}

	void addMovement(BodyHandle _handle, const bx::Vec3& _position, const bx::Quaternion& _rotation, const float _deltaTime)
	{
		s_ctx->addMovement(_handle, _position, _rotation, _deltaTime);
	}

	void setFriction(BodyHandle _handle, float _friction)
	{
		s_ctx->setFriction(_handle, _friction);
	}

	float getFriction(BodyHandle _handle)
	{
		return s_ctx->getFriction(_handle);
	}

	void getGroundInfo(BodyHandle _handle, GroundInfo& _info)
	{
		s_ctx->getGroundInfo(_handle, _info);
	}

	void destroy(BodyHandle _body)
	{
		s_ctx->destroyBody(_body);
	}

	const bx::Vec3 getGravity()
	{
		return s_ctx->getGravity();
	}

	// @todo Collapse all body getters above into a BodyInfo struct.

	OcclusionQueryHandle createOcclusionQuery()
	{
		MAX_CHECK_CAPS(MAX_CAPS_OCCLUSION_QUERY, "Occlusion query is not supported!");
		return s_ctx->createOcclusionQuery();
	}

	OcclusionQueryResult::Enum getResult(OcclusionQueryHandle _handle, int32_t* _result)
	{
		MAX_CHECK_CAPS(MAX_CAPS_OCCLUSION_QUERY, "Occlusion query is not supported!");
		return s_ctx->getResult(_handle, _result);
	}

	void destroy(OcclusionQueryHandle _handle)
	{
		MAX_CHECK_CAPS(MAX_CAPS_OCCLUSION_QUERY, "Occlusion query is not supported!");
		s_ctx->destroyOcclusionQuery(_handle);
	}

	void setPaletteColor(uint8_t _index, uint32_t _rgba)
	{
		const uint8_t rr = uint8_t(_rgba>>24);
		const uint8_t gg = uint8_t(_rgba>>16);
		const uint8_t bb = uint8_t(_rgba>> 8);
		const uint8_t aa = uint8_t(_rgba>> 0);

		const float rgba[4] =
		{
			rr * 1.0f/255.0f,
			gg * 1.0f/255.0f,
			bb * 1.0f/255.0f,
			aa * 1.0f/255.0f,
		};

		s_ctx->setPaletteColor(_index, rgba);
	}

	void setPaletteColor(uint8_t _index, float _r, float _g, float _b, float _a)
	{
		float rgba[4] = { _r, _g, _b, _a };
		s_ctx->setPaletteColor(_index, rgba);
	}

	void setPaletteColor(uint8_t _index, const float _rgba[4])
	{
		s_ctx->setPaletteColor(_index, _rgba);
	}

	bool checkView(ViewId _id)
	{
		// workaround GCC 4.9 type-limit check.
		const uint32_t id = _id;
		return id < MAX_CONFIG_MAX_VIEWS;
	}

	void setViewName(ViewId _id, const char* _name, int32_t _len)
	{
		BX_ASSERT(checkView(_id), "Invalid view id: %d", _id);
		s_ctx->setViewName(_id, bx::StringView(_name, _len) );
	}

	void setViewRect(ViewId _id, uint16_t _x, uint16_t _y, uint16_t _width, uint16_t _height)
	{
		BX_ASSERT(checkView(_id), "Invalid view id: %d", _id);
		s_ctx->setViewRect(_id, _x, _y, _width, _height);
	}

	void setViewRect(ViewId _id, uint16_t _x, uint16_t _y, BackbufferRatio::Enum _ratio)
	{
		BX_ASSERT(checkView(_id), "Invalid view id: %d", _id);

		uint16_t width  = uint16_t(s_ctx->m_init.resolution.width);
		uint16_t height = uint16_t(s_ctx->m_init.resolution.height);
		getTextureSizeFromRatio(_ratio, width, height);
		setViewRect(_id, _x, _y, width, height);
	}

	void setViewScissor(ViewId _id, uint16_t _x, uint16_t _y, uint16_t _width, uint16_t _height)
	{
		BX_ASSERT(checkView(_id), "Invalid view id: %d", _id);
		s_ctx->setViewScissor(_id, _x, _y, _width, _height);
	}

	void setViewClear(ViewId _id, uint16_t _flags, uint32_t _rgba, float _depth, uint8_t _stencil)
	{
		BX_ASSERT(checkView(_id), "Invalid view id: %d", _id);
		s_ctx->setViewClear(_id, _flags, _rgba, _depth, _stencil);
	}

	void setViewClear(ViewId _id, uint16_t _flags, float _depth, uint8_t _stencil, uint8_t _0, uint8_t _1, uint8_t _2, uint8_t _3, uint8_t _4, uint8_t _5, uint8_t _6, uint8_t _7)
	{
		BX_ASSERT(checkView(_id), "Invalid view id: %d", _id);
		s_ctx->setViewClear(_id, _flags, _depth, _stencil, _0, _1, _2, _3, _4, _5, _6, _7);
	}

	void setViewMode(ViewId _id, ViewMode::Enum _mode)
	{
		BX_ASSERT(checkView(_id), "Invalid view id: %d", _id);
		s_ctx->setViewMode(_id, _mode);
	}

	void setViewFrameBuffer(ViewId _id, FrameBufferHandle _handle)
	{
		BX_ASSERT(checkView(_id), "Invalid view id: %d", _id);
		s_ctx->setViewFrameBuffer(_id, _handle);
	}

	void setViewTransform(ViewId _id, const void* _view, const void* _proj)
	{
		BX_ASSERT(checkView(_id), "Invalid view id: %d", _id);
		s_ctx->setViewTransform(_id, _view, _proj);
	}

	void setViewOrder(ViewId _id, uint16_t _num, const ViewId* _order)
	{
		BX_ASSERT(checkView(_id), "Invalid view id: %d", _id);
		s_ctx->setViewOrder(_id, _num, _order);
	}

	void resetView(ViewId _id)
	{
		BX_ASSERT(checkView(_id), "Invalid view id: %d", _id);
		s_ctx->resetView(_id);
	}

#define MAX_CHECK_ENCODER0()                               \
	MAX_CHECK_API_THREAD();                                \
	MAX_FATAL(NULL != s_ctx->m_encoder0, Fatal::DebugCheck \
		, "max is configured to allow only encoder API. See: `MAX_CONFIG_ENCODER_API_ONLY`.")

	void setMarker(const char* _name, int32_t _len)
	{
		MAX_CHECK_ENCODER0();
		s_ctx->m_encoder0->setMarker(_name, _len);
	}

	void setState(uint64_t _state, uint32_t _rgba)
	{
		MAX_CHECK_ENCODER0();
		s_ctx->m_encoder0->setState(_state, _rgba);
	}

	void setCondition(OcclusionQueryHandle _handle, bool _visible)
	{
		MAX_CHECK_ENCODER0();
		s_ctx->m_encoder0->setCondition(_handle, _visible);
	}

	void setStencil(uint32_t _fstencil, uint32_t _bstencil)
	{
		MAX_CHECK_ENCODER0();
		s_ctx->m_encoder0->setStencil(_fstencil, _bstencil);
	}

	uint16_t setScissor(uint16_t _x, uint16_t _y, uint16_t _width, uint16_t _height)
	{
		MAX_CHECK_ENCODER0();
		return s_ctx->m_encoder0->setScissor(_x, _y, _width, _height);
	}

	void setScissor(uint16_t _cache)
	{
		MAX_CHECK_ENCODER0();
		s_ctx->m_encoder0->setScissor(_cache);
	}

	uint32_t setTransform(const void* _mtx, uint16_t _num)
	{
		MAX_CHECK_ENCODER0();
		return s_ctx->m_encoder0->setTransform(_mtx, _num);
	}

	uint32_t allocTransform(Transform* _transform, uint16_t _num)
	{
		MAX_CHECK_ENCODER0();
		return s_ctx->m_encoder0->allocTransform(_transform, _num);
	}

	void setTransform(uint32_t _cache, uint16_t _num)
	{
		MAX_CHECK_ENCODER0();
		s_ctx->m_encoder0->setTransform(_cache, _num);
	}

	void setUniform(UniformHandle _handle, const void* _value, uint16_t _num)
	{
		MAX_CHECK_ENCODER0();
		s_ctx->m_encoder0->setUniform(_handle, _value, _num);
	}

	void setIndexBuffer(IndexBufferHandle _handle)
	{
		MAX_CHECK_ENCODER0();
		s_ctx->m_encoder0->setIndexBuffer(_handle);
	}

	void setIndexBuffer(IndexBufferHandle _handle, uint32_t _firstIndex, uint32_t _numIndices)
	{
		MAX_CHECK_ENCODER0();
		s_ctx->m_encoder0->setIndexBuffer(_handle, _firstIndex, _numIndices);
	}

	void setIndexBuffer(DynamicIndexBufferHandle _handle)
	{
		MAX_CHECK_ENCODER0();
		s_ctx->m_encoder0->setIndexBuffer(_handle);
	}

	void setIndexBuffer(DynamicIndexBufferHandle _handle, uint32_t _firstIndex, uint32_t _numIndices)
	{
		MAX_CHECK_ENCODER0();
		s_ctx->m_encoder0->setIndexBuffer(_handle, _firstIndex, _numIndices);
	}

	void setIndexBuffer(const TransientIndexBuffer* _tib)
	{
		MAX_CHECK_ENCODER0();
		s_ctx->m_encoder0->setIndexBuffer(_tib);
	}

	void setIndexBuffer(const TransientIndexBuffer* _tib, uint32_t _firstIndex, uint32_t _numIndices)
	{
		MAX_CHECK_ENCODER0();
		s_ctx->m_encoder0->setIndexBuffer(_tib, _firstIndex, _numIndices);
	}

	void setVertexBuffer(
		  uint8_t _stream
		, VertexBufferHandle _handle
		, uint32_t _startVertex
		, uint32_t _numVertices
		, VertexLayoutHandle _layoutHandle
		)
	{
		MAX_CHECK_ENCODER0();
		s_ctx->m_encoder0->setVertexBuffer(_stream, _handle, _startVertex, _numVertices, _layoutHandle);
	}

	void setVertexBuffer(uint8_t _stream, VertexBufferHandle _handle)
	{
		MAX_CHECK_ENCODER0();
		s_ctx->m_encoder0->setVertexBuffer(_stream, _handle);
	}

	void setVertexBuffer(
		  uint8_t _stream
		, DynamicVertexBufferHandle _handle
		, uint32_t _startVertex
		, uint32_t _numVertices
		, VertexLayoutHandle _layoutHandle
		)
	{
		MAX_CHECK_ENCODER0();
		s_ctx->m_encoder0->setVertexBuffer(_stream, _handle, _startVertex, _numVertices, _layoutHandle);
	}

	void setVertexBuffer(uint8_t _stream, DynamicVertexBufferHandle _handle)
	{
		MAX_CHECK_ENCODER0();
		s_ctx->m_encoder0->setVertexBuffer(_stream, _handle);
	}

	void setVertexBuffer(
		  uint8_t _stream
		, const TransientVertexBuffer* _tvb
		, uint32_t _startVertex
		, uint32_t _numVertices
		, VertexLayoutHandle _layoutHandle
		)
	{
		MAX_CHECK_ENCODER0();
		s_ctx->m_encoder0->setVertexBuffer(_stream, _tvb, _startVertex, _numVertices, _layoutHandle);
	}

	void setVertexBuffer(uint8_t _stream, const TransientVertexBuffer* _tvb)
	{
		MAX_CHECK_ENCODER0();
		s_ctx->m_encoder0->setVertexBuffer(_stream, _tvb);
	}

	void setVertexCount(uint32_t _numVertices)
	{
		MAX_CHECK_ENCODER0();
		s_ctx->m_encoder0->setVertexCount(_numVertices);
	}

	void setInstanceDataBuffer(const InstanceDataBuffer* _idb)
	{
		MAX_CHECK_ENCODER0();
		s_ctx->m_encoder0->setInstanceDataBuffer(_idb);
	}

	void setInstanceDataBuffer(const InstanceDataBuffer* _idb, uint32_t _start, uint32_t _num)
	{
		MAX_CHECK_ENCODER0();
		s_ctx->m_encoder0->setInstanceDataBuffer(_idb, _start, _num);
	}

	void setInstanceDataBuffer(VertexBufferHandle _handle, uint32_t _startVertex, uint32_t _num)
	{
		MAX_CHECK_ENCODER0();
		s_ctx->m_encoder0->setInstanceDataBuffer(_handle, _startVertex, _num);
	}

	void setInstanceDataBuffer(DynamicVertexBufferHandle _handle, uint32_t _startVertex, uint32_t _num)
	{
		MAX_CHECK_ENCODER0();
		s_ctx->m_encoder0->setInstanceDataBuffer(_handle, _startVertex, _num);
	}

	void setInstanceCount(uint32_t _numInstances)
	{
		MAX_CHECK_ENCODER0();
		s_ctx->m_encoder0->setInstanceCount(_numInstances);
	}

	void setTexture(uint8_t _stage, UniformHandle _sampler, TextureHandle _handle, uint32_t _flags)
	{
		MAX_CHECK_ENCODER0();
		s_ctx->m_encoder0->setTexture(_stage, _sampler, _handle, _flags);
	}

	void touch(ViewId _id)
	{
		MAX_CHECK_ENCODER0();
		s_ctx->m_encoder0->touch(_id);
	}

	void submit(ViewId _id, ProgramHandle _program, uint32_t _depth, uint8_t _flags)
	{
		MAX_CHECK_ENCODER0();
		s_ctx->m_encoder0->submit(_id, _program, _depth, _flags);
	}

	void submit(ViewId _id, MaterialHandle _material, uint32_t _depth, uint8_t _flags)
	{
		MaterialRef& mr = s_ctx->m_materialRef[_material.idx];

		MAX_CHECK_ENCODER0();
		s_ctx->m_encoder0->submit(_id, mr.m_program, _depth, _flags);
	}

	void submit(ViewId _id, ProgramHandle _program, OcclusionQueryHandle _occlusionQuery, uint32_t _depth, uint8_t _flags)
	{
		MAX_CHECK_ENCODER0();
		s_ctx->m_encoder0->submit(_id, _program, _occlusionQuery, _depth, _flags);
	}

	void submit(ViewId _id, ProgramHandle _program, IndirectBufferHandle _indirectHandle, uint32_t _start, uint32_t _num, uint32_t _depth, uint8_t _flags)
	{
		MAX_CHECK_ENCODER0();
		s_ctx->m_encoder0->submit(_id, _program, _indirectHandle, _start, _num, _depth, _flags);
	}

	void submit(ViewId _id, ProgramHandle _program, IndirectBufferHandle _indirectHandle, uint32_t _start, IndexBufferHandle _numHandle, uint32_t _numIndex, uint32_t _numMax, uint32_t _depth, uint8_t _flags)
	{
		MAX_CHECK_ENCODER0();
		s_ctx->m_encoder0->submit(_id, _program, _indirectHandle, _start, _numHandle, _numIndex, _numMax, _depth, _flags);
	}

	void setBuffer(uint8_t _stage, IndexBufferHandle _handle, Access::Enum _access)
	{
		MAX_CHECK_ENCODER0();
		s_ctx->m_encoder0->setBuffer(_stage, _handle, _access);
	}

	void setBuffer(uint8_t _stage, VertexBufferHandle _handle, Access::Enum _access)
	{
		MAX_CHECK_ENCODER0();
		s_ctx->m_encoder0->setBuffer(_stage, _handle, _access);
	}

	void setBuffer(uint8_t _stage, DynamicIndexBufferHandle _handle, Access::Enum _access)
	{
		MAX_CHECK_ENCODER0();
		s_ctx->m_encoder0->setBuffer(_stage, _handle, _access);
	}

	void setBuffer(uint8_t _stage, DynamicVertexBufferHandle _handle, Access::Enum _access)
	{
		MAX_CHECK_ENCODER0();
		s_ctx->m_encoder0->setBuffer(_stage, _handle, _access);
	}

	void setBuffer(uint8_t _stage, IndirectBufferHandle _handle, Access::Enum _access)
	{
		MAX_CHECK_ENCODER0();
		s_ctx->m_encoder0->setBuffer(_stage, _handle, _access);
	}

	void setImage(uint8_t _stage, TextureHandle _handle, uint8_t _mip, Access::Enum _access, TextureFormat::Enum _format)
	{
		MAX_CHECK_ENCODER0();
		s_ctx->m_encoder0->setImage(_stage, _handle, _mip, _access, _format);
	}

	void dispatch(ViewId _id, ProgramHandle _handle, uint32_t _numX, uint32_t _numY, uint32_t _numZ, uint8_t _flags)
	{
		MAX_CHECK_ENCODER0();
		s_ctx->m_encoder0->dispatch(_id, _handle, _numX, _numY, _numZ, _flags);
	}

	void dispatch(ViewId _id, ProgramHandle _handle, IndirectBufferHandle _indirectHandle, uint32_t _start, uint32_t _num, uint8_t _flags)
	{
		MAX_CHECK_ENCODER0();
		s_ctx->m_encoder0->dispatch(_id, _handle, _indirectHandle, _start, _num, _flags);
	}

	void discard(uint8_t _flags)
	{
		MAX_CHECK_ENCODER0();
		s_ctx->m_encoder0->discard(_flags);
	}

	void blit(ViewId _id, TextureHandle _dst, uint16_t _dstX, uint16_t _dstY, TextureHandle _src, uint16_t _srcX, uint16_t _srcY, uint16_t _width, uint16_t _height)
	{
		MAX_CHECK_ENCODER0();
		s_ctx->m_encoder0->blit(_id, _dst, _dstX, _dstY, _src, _srcX, _srcY, _width, _height);
	}

	void blit(ViewId _id, TextureHandle _dst, uint8_t _dstMip, uint16_t _dstX, uint16_t _dstY, uint16_t _dstZ, TextureHandle _src, uint8_t _srcMip, uint16_t _srcX, uint16_t _srcY, uint16_t _srcZ, uint16_t _width, uint16_t _height, uint16_t _depth)
	{
		MAX_CHECK_ENCODER0();
		s_ctx->m_encoder0->blit(_id, _dst, _dstMip, _dstX, _dstY, _dstZ, _src, _srcMip, _srcX, _srcY, _srcZ, _width, _height, _depth);
	}

	void requestScreenShot(FrameBufferHandle _handle, const char* _filePath)
	{
		MAX_CHECK_API_THREAD();
		s_ctx->requestScreenShot(_handle, _filePath);
	}

	void cmdAdd(const char* _name, ConsoleFn _fn, void* _userData)
	{
		s_ctx->cmdAdd(_name, _fn, _userData);
	}

	void cmdRemove(const char* _name)
	{
		s_ctx->cmdRemove(_name);
	}

	void cmdExec(const char* _format, ...)
	{
		char tmp[2048];

		va_list argList;
		va_start(argList, _format);
		bx::vsnprintf(tmp, BX_COUNTOF(tmp), _format, argList);
		va_end(argList);

		s_ctx->cmdExec(tmp);
	}

#undef MAX_CHECK_ENCODER0

} // namespace max

#ifdef MAX_BUILD_MINK

namespace mink
{
	const Memory* loadMemory(const bx::FilePath& _filePath)
	{
		bx::FileReader reader;
		if (bx::open(&reader, _filePath))
		{
			uint32_t size = (uint32_t)bx::getSize(&reader);
			const Memory* mem = alloc(size + 1);
			bx::read(&reader, mem->data, size, bx::ErrorAssert{});
			bx::close(&reader);
			mem->data[mem->size - 1] = '\0';
			return mem;
		}

		BX_TRACE("Failed to load %s.", _filePath);
		return NULL;
	}

	MotionHandle loadMotion(const char* _filePath)
	{
		const Memory* mem = loadMemory(_filePath);
		return mink::createMotion(mem);
	}

} // namespace mink

#endif // MAX_BUILD_MINK

#if MAX_CONFIG_PREFER_DISCRETE_GPU
extern "C"
{
	// When laptop setup has integrated and discrete GPU, following driver workarounds will
	// select discrete GPU:

	// Reference(s):
	// - https://web.archive.org/web/20180722051003/https://docs.nvidia.com/gameworks/content/technologies/desktop/optimus.htm
	//
	__declspec(dllexport) uint32_t NvOptimusEnablement = UINT32_C(1);

	// Reference(s):
	// - https://web.archive.org/web/20180722051032/https://gpuopen.com/amdpowerxpressrequesthighperformance/
	//
	__declspec(dllexport) uint32_t AmdPowerXpressRequestHighPerformance = UINT32_C(1);
}
#endif // MAX_CONFIG_PREFER_DISCRETE_GPU

#define MAX_TEXTURE_FORMAT_BIMG(_fmt) \
	BX_STATIC_ASSERT(uint32_t(max::TextureFormat::_fmt) == uint32_t(bimg::TextureFormat::_fmt) )

MAX_TEXTURE_FORMAT_BIMG(BC1);
MAX_TEXTURE_FORMAT_BIMG(BC2);
MAX_TEXTURE_FORMAT_BIMG(BC3);
MAX_TEXTURE_FORMAT_BIMG(BC4);
MAX_TEXTURE_FORMAT_BIMG(BC5);
MAX_TEXTURE_FORMAT_BIMG(BC6H);
MAX_TEXTURE_FORMAT_BIMG(BC7);
MAX_TEXTURE_FORMAT_BIMG(ETC1);
MAX_TEXTURE_FORMAT_BIMG(ETC2);
MAX_TEXTURE_FORMAT_BIMG(ETC2A);
MAX_TEXTURE_FORMAT_BIMG(ETC2A1);
MAX_TEXTURE_FORMAT_BIMG(PTC12);
MAX_TEXTURE_FORMAT_BIMG(PTC14);
MAX_TEXTURE_FORMAT_BIMG(PTC12A);
MAX_TEXTURE_FORMAT_BIMG(PTC14A);
MAX_TEXTURE_FORMAT_BIMG(PTC22);
MAX_TEXTURE_FORMAT_BIMG(PTC24);
MAX_TEXTURE_FORMAT_BIMG(ATC);
MAX_TEXTURE_FORMAT_BIMG(ATCE);
MAX_TEXTURE_FORMAT_BIMG(ATCI);
MAX_TEXTURE_FORMAT_BIMG(ASTC4x4);
MAX_TEXTURE_FORMAT_BIMG(ASTC5x4);
MAX_TEXTURE_FORMAT_BIMG(ASTC5x5);
MAX_TEXTURE_FORMAT_BIMG(ASTC6x5);
MAX_TEXTURE_FORMAT_BIMG(ASTC6x6);
MAX_TEXTURE_FORMAT_BIMG(ASTC8x5);
MAX_TEXTURE_FORMAT_BIMG(ASTC8x6);
MAX_TEXTURE_FORMAT_BIMG(ASTC8x8);
MAX_TEXTURE_FORMAT_BIMG(ASTC10x5);
MAX_TEXTURE_FORMAT_BIMG(ASTC10x6);
MAX_TEXTURE_FORMAT_BIMG(ASTC10x8);
MAX_TEXTURE_FORMAT_BIMG(ASTC10x10);
MAX_TEXTURE_FORMAT_BIMG(ASTC12x10);
MAX_TEXTURE_FORMAT_BIMG(ASTC12x12);
MAX_TEXTURE_FORMAT_BIMG(Unknown);
MAX_TEXTURE_FORMAT_BIMG(R1);
MAX_TEXTURE_FORMAT_BIMG(A8);
MAX_TEXTURE_FORMAT_BIMG(R8);
MAX_TEXTURE_FORMAT_BIMG(R8I);
MAX_TEXTURE_FORMAT_BIMG(R8U);
MAX_TEXTURE_FORMAT_BIMG(R8S);
MAX_TEXTURE_FORMAT_BIMG(R16);
MAX_TEXTURE_FORMAT_BIMG(R16I);
MAX_TEXTURE_FORMAT_BIMG(R16U);
MAX_TEXTURE_FORMAT_BIMG(R16F);
MAX_TEXTURE_FORMAT_BIMG(R16S);
MAX_TEXTURE_FORMAT_BIMG(R32I);
MAX_TEXTURE_FORMAT_BIMG(R32U);
MAX_TEXTURE_FORMAT_BIMG(R32F);
MAX_TEXTURE_FORMAT_BIMG(RG8);
MAX_TEXTURE_FORMAT_BIMG(RG8I);
MAX_TEXTURE_FORMAT_BIMG(RG8U);
MAX_TEXTURE_FORMAT_BIMG(RG8S);
MAX_TEXTURE_FORMAT_BIMG(RG16);
MAX_TEXTURE_FORMAT_BIMG(RG16I);
MAX_TEXTURE_FORMAT_BIMG(RG16U);
MAX_TEXTURE_FORMAT_BIMG(RG16F);
MAX_TEXTURE_FORMAT_BIMG(RG16S);
MAX_TEXTURE_FORMAT_BIMG(RG32I);
MAX_TEXTURE_FORMAT_BIMG(RG32U);
MAX_TEXTURE_FORMAT_BIMG(RG32F);
MAX_TEXTURE_FORMAT_BIMG(RGB8);
MAX_TEXTURE_FORMAT_BIMG(RGB8I);
MAX_TEXTURE_FORMAT_BIMG(RGB8U);
MAX_TEXTURE_FORMAT_BIMG(RGB8S);
MAX_TEXTURE_FORMAT_BIMG(RGB9E5F);
MAX_TEXTURE_FORMAT_BIMG(BGRA8);
MAX_TEXTURE_FORMAT_BIMG(RGBA8);
MAX_TEXTURE_FORMAT_BIMG(RGBA8I);
MAX_TEXTURE_FORMAT_BIMG(RGBA8U);
MAX_TEXTURE_FORMAT_BIMG(RGBA8S);
MAX_TEXTURE_FORMAT_BIMG(RGBA16);
MAX_TEXTURE_FORMAT_BIMG(RGBA16I);
MAX_TEXTURE_FORMAT_BIMG(RGBA16U);
MAX_TEXTURE_FORMAT_BIMG(RGBA16F);
MAX_TEXTURE_FORMAT_BIMG(RGBA16S);
MAX_TEXTURE_FORMAT_BIMG(RGBA32I);
MAX_TEXTURE_FORMAT_BIMG(RGBA32U);
MAX_TEXTURE_FORMAT_BIMG(RGBA32F);
MAX_TEXTURE_FORMAT_BIMG(B5G6R5);
MAX_TEXTURE_FORMAT_BIMG(R5G6B5);
MAX_TEXTURE_FORMAT_BIMG(BGRA4);
MAX_TEXTURE_FORMAT_BIMG(RGBA4);
MAX_TEXTURE_FORMAT_BIMG(BGR5A1);
MAX_TEXTURE_FORMAT_BIMG(RGB5A1);
MAX_TEXTURE_FORMAT_BIMG(RGB10A2);
MAX_TEXTURE_FORMAT_BIMG(RG11B10F);
MAX_TEXTURE_FORMAT_BIMG(UnknownDepth);
MAX_TEXTURE_FORMAT_BIMG(D16);
MAX_TEXTURE_FORMAT_BIMG(D24);
MAX_TEXTURE_FORMAT_BIMG(D24S8);
MAX_TEXTURE_FORMAT_BIMG(D32);
MAX_TEXTURE_FORMAT_BIMG(D16F);
MAX_TEXTURE_FORMAT_BIMG(D24F);
MAX_TEXTURE_FORMAT_BIMG(D32F);
MAX_TEXTURE_FORMAT_BIMG(D0S8);
MAX_TEXTURE_FORMAT_BIMG(Count);

#undef MAX_TEXTURE_FORMAT_BIMG

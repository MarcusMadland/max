/*
 * Copyright 2024 Marcus Madland. All rights reserved.
 * License: https://github.com/marcusmadland/max/blob/main/LICENSE
 */

#ifndef MAX_P_H_HEADER_GUARD
#define MAX_P_H_HEADER_GUARD

#include <bx/platform.h>

#ifndef BX_CONFIG_DEBUG
#	error "BX_CONFIG_DEBUG must be defined in build script!"
#endif // BX_CONFIG_DEBUG

#define MAX_CONFIG_DEBUG BX_CONFIG_DEBUG

#if BX_CONFIG_DEBUG
#	define BX_TRACE  _MAX_TRACE
#	define BX_WARN   _MAX_WARN
#	define BX_ASSERT _MAX_ASSERT
#endif // BX_CONFIG_DEBUG

#include <max/max.h>
#include "config.h"

#include <inttypes.h>

// Check handle, cannot be max::kInvalidHandle and must be valid.
#define MAX_CHECK_HANDLE(_desc, _handleAlloc, _handle) \
	BX_ASSERT(isValid(_handle)                          \
		&& _handleAlloc.isValid(_handle.idx)            \
		, "Invalid handle. %s handle: %d (max %d)"      \
		, _desc                                         \
		, _handle.idx                                   \
		, _handleAlloc.getMaxHandles()                  \
		)

// Check handle, it's ok to be max::kInvalidHandle or must be valid.
#define MAX_CHECK_HANDLE_INVALID_OK(_desc, _handleAlloc, _handle) \
	BX_ASSERT(!isValid(_handle)                                    \
		|| _handleAlloc.isValid(_handle.idx)                       \
		, "Invalid handle. %s handle: %d (max %d)"                 \
		, _desc                                                    \
		, _handle.idx                                              \
		, _handleAlloc.getMaxHandles()                             \
		)

#if MAX_CONFIG_MULTITHREADED
#	define MAX_MUTEX_SCOPE(_mutex) bx::MutexScope BX_CONCATENATE(mutexScope, __LINE__)(_mutex)
#else
#	define MAX_MUTEX_SCOPE(_mutex) BX_NOOP()
#endif // MAX_CONFIG_MULTITHREADED

#if MAX_CONFIG_PROFILER
#	define MAX_PROFILER_SCOPE(_name, _abgr)            ProfilerScope BX_CONCATENATE(profilerScope, __LINE__)(_name, _abgr, __FILE__, uint16_t(__LINE__) )
#	define MAX_PROFILER_BEGIN(_name, _abgr)            g_callback->profilerBegin(_name, _abgr, __FILE__, uint16_t(__LINE__) )
#	define MAX_PROFILER_BEGIN_LITERAL(_name, _abgr)    g_callback->profilerBeginLiteral(_name, _abgr, __FILE__, uint16_t(__LINE__) )
#	define MAX_PROFILER_END()                          g_callback->profilerEnd()
#	define MAX_PROFILER_SET_CURRENT_THREAD_NAME(_name) BX_NOOP()
#else
#	define MAX_PROFILER_SCOPE(_name, _abgr)            BX_NOOP()
#	define MAX_PROFILER_BEGIN(_name, _abgr)            BX_NOOP()
#	define MAX_PROFILER_BEGIN_LITERAL(_name, _abgr)    BX_NOOP()
#	define MAX_PROFILER_END()                          BX_NOOP()
#	define MAX_PROFILER_SET_CURRENT_THREAD_NAME(_name) BX_NOOP()
#endif // MAX_PROFILER_SCOPE

namespace max
{
#if BX_COMPILER_CLANG_ANALYZER
	void __attribute__( (analyzer_noreturn) ) fatal(const char* _filePath, uint16_t _line, Fatal::Enum _code, const char* _format, ...);
#else
	void fatal(const char* _filePath, uint16_t _line, Fatal::Enum _code, const char* _format, ...);
#endif // BX_COMPILER_CLANG_ANALYZER

	void trace(const char* _filePath, uint16_t _line, const char* _format, ...);

	inline bool operator==(const VertexLayoutHandle& _lhs, const VertexLayoutHandle& _rhs) { return _lhs.idx == _rhs.idx; }
	inline bool operator==(const UniformHandle& _lhs,    const UniformHandle&    _rhs) { return _lhs.idx == _rhs.idx; }
}

#define _MAX_TRACE(_format, ...)                                                       \
	BX_MACRO_BLOCK_BEGIN                                                                \
		max::trace(__FILE__, uint16_t(__LINE__), "MAX " _format "\n", ##__VA_ARGS__); \
	BX_MACRO_BLOCK_END

#define _MAX_WARN(_condition, _format, ...)          \
	BX_MACRO_BLOCK_BEGIN                              \
		if (!BX_IGNORE_C4127(_condition) )            \
		{                                             \
			BX_TRACE("WARN " _format, ##__VA_ARGS__); \
		}                                             \
	BX_MACRO_BLOCK_END

#define _MAX_ASSERT(_condition, _format, ...)                                                                 \
	BX_MACRO_BLOCK_BEGIN                                                                                       \
		if (!BX_IGNORE_C4127(_condition)                                                                       \
		&&  bx::assertFunction(bx::Location::current(), "ASSERT " #_condition " -> " _format, ##__VA_ARGS__) ) \
		{                                                                                                      \
			max::fatal(__FILE__, uint16_t(__LINE__), max::Fatal::DebugCheck, _format, ##__VA_ARGS__);        \
		}                                                                                                      \
	BX_MACRO_BLOCK_END

#define MAX_FATAL(_condition, _err, _format, ...)                             \
	BX_MACRO_BLOCK_BEGIN                                                       \
		if (!BX_IGNORE_C4127(_condition) )                                     \
		{                                                                      \
			fatal(__FILE__, uint16_t(__LINE__), _err, _format, ##__VA_ARGS__); \
		}                                                                      \
	BX_MACRO_BLOCK_END

#define MAX_ERROR_CHECK(_condition, _err, _result, _msg, _format, ...) \
	if (!BX_IGNORE_C4127(_condition) )                                  \
	{                                                                   \
		BX_ERROR_SET(_err, _result, _msg);                              \
		BX_TRACE("%S: 0x%08x '%S' - " _format                           \
			, &bxErrorScope.getName()                                   \
			, _err->get().code                                          \
			, &_err->getMessage()                                       \
			, ##__VA_ARGS__                                             \
			);                                                          \
		return;                                                         \
	}

#include <bx/allocator.h>
#include <bx/bx.h>
#include <bx/cpu.h>
#include <bx/debug.h>
#include <bx/endian.h>
#include <bx/error.h>
#include <bx/float4x4_t.h>
#include <bx/handlealloc.h>
#include <bx/hash.h>
#include <bx/math.h>
#include <bx/mutex.h>
#include <bx/os.h>
#include <bx/readerwriter.h>
#include <bx/ringbuffer.h>
#include <bx/sort.h>
#include <bx/string.h>
#include <bx/thread.h>
#include <bx/timer.h>
#include <bx/uint32_t.h>
#include <bx/spscqueue.h>
#include <bx/filepath.h>
#include <bx/commandline.h>
#include <bx/bounds.h>

#include <max/platform.h>
#include <bimg/bimg.h>
#include "shader.h"
#include "vertexlayout.h"
#include "version.h"

#if !defined(MAX_DEFAULT_WIDTH) && !defined(MAX_DEFAULT_HEIGHT)
#	define MAX_DEFAULT_WIDTH  1280
#	define MAX_DEFAULT_HEIGHT 720
#elif !defined(MAX_DEFAULT_WIDTH) || !defined(MAX_DEFAULT_HEIGHT)
#	error "Both MAX_DEFAULT_WIDTH and MAX_DEFAULT_HEIGHT must be defined."
#endif // MAX_DEFAULT_WIDTH && MAX_DEFAULT_HEIGHT

#define MAX_IMPLEMENT_EVENT(_class, _type) \
			_class(WindowHandle _handle) : Event(_type, _handle) {}

#define MAX_CHUNK_MAGIC_TEX BX_MAKEFOURCC('T', 'E', 'X', 0x0)

#define MAX_CLEAR_COLOR_USE_PALETTE UINT16_C(0x8000)
#define MAX_CLEAR_MASK (0                 \
			| MAX_CLEAR_COLOR             \
			| MAX_CLEAR_DEPTH             \
			| MAX_CLEAR_STENCIL           \
			| MAX_CLEAR_COLOR_USE_PALETTE \
			)

#if MAX_CONFIG_USE_TINYSTL
namespace max
{
	struct TinyStlAllocator
	{
		static void* static_allocate(size_t _bytes);
		static void static_deallocate(void* _ptr, size_t /*_bytes*/);
	};
} // namespace max
#	define TINYSTL_ALLOCATOR max::TinyStlAllocator
#	include <tinystl/string.h>
#	include <tinystl/unordered_map.h>
#	include <tinystl/unordered_set.h>
#	include <tinystl/vector.h>

namespace tinystl
{
	template<typename T, typename Alloc = TINYSTL_ALLOCATOR>
	class list : public vector<T, Alloc>
	{
	public:
		void push_front(const T& _value)
		{
			this->insert(this->begin(), _value);
		}

		void pop_front()
		{
			this->erase(this->begin() );
		}

		void sort()
		{
			bx::quickSort(
				  this->begin()
				, uint32_t(this->end() - this->begin() )
				, sizeof(T)
				, [](const void* _a, const void* _b) -> int32_t {
					const T& lhs = *(const T*)(_a);
					const T& rhs = *(const T*)(_b);
					return lhs < rhs ? -1 : 1;
				});
		}
	};

} // namespace tinystl

namespace stl = tinystl;
#else
#	include <list>
#	include <string>
#	include <unordered_map>
#	include <unordered_set>
#	include <vector>
namespace stl = std;
#endif // MAX_CONFIG_USE_TINYSTL

#include <meshoptimizer/src/meshoptimizer.h> 

#if BX_PLATFORM_ANDROID
#	include <android/native_window.h>
#endif // BX_PLATFORM_*

#define MAX_MAX_COMPUTE_BINDINGS MAX_CONFIG_MAX_TEXTURE_SAMPLERS

#define MAX_SAMPLER_INTERNAL_DEFAULT       UINT32_C(0x10000000)
#define MAX_SAMPLER_INTERNAL_SHARED        UINT32_C(0x20000000)

#define MAX_RESET_INTERNAL_FORCE           UINT32_C(0x80000000)

#define MAX_STATE_INTERNAL_SCISSOR         UINT64_C(0x2000000000000000)
#define MAX_STATE_INTERNAL_OCCLUSION_QUERY UINT64_C(0x4000000000000000)

#define MAX_SUBMIT_INTERNAL_NONE              UINT8_C(0x00)
#define MAX_SUBMIT_INTERNAL_INDEX32           UINT8_C(0x40)
#define MAX_SUBMIT_INTERNAL_OCCLUSION_VISIBLE UINT8_C(0x80)
#define MAX_SUBMIT_INTERNAL_RESERVED_MASK     UINT8_C(0xff)

#define MAX_PHYSICS_NOOP_NAME		  "Noop"
#define MAX_PHYSICS_JOLT_NAME		  "Jolt"

#define MAX_RENDERER_NOOP_NAME       "Noop"
#define MAX_RENDERER_AGC_NAME        "AGC"
#define MAX_RENDERER_DIRECT3D11_NAME "Direct3D 11"
#define MAX_RENDERER_DIRECT3D12_NAME "Direct3D 12"
#define MAX_RENDERER_GNM_NAME        "GNM"
#define MAX_RENDERER_METAL_NAME      "Metal"
#define MAX_RENDERER_NVN_NAME        "NVN"
#define MAX_RENDERER_VULKAN_NAME     "Vulkan"

#if MAX_CONFIG_RENDERER_OPENGL
#	if MAX_CONFIG_RENDERER_OPENGL >= 31 && MAX_CONFIG_RENDERER_OPENGL <= 33
#		if MAX_CONFIG_RENDERER_OPENGL == 31
#			define MAX_RENDERER_OPENGL_NAME "OpenGL 3.1"
#		elif MAX_CONFIG_RENDERER_OPENGL == 32
#			define MAX_RENDERER_OPENGL_NAME "OpenGL 3.2"
#		else
#			define MAX_RENDERER_OPENGL_NAME "OpenGL 3.3"
#		endif // 31+
#	elif MAX_CONFIG_RENDERER_OPENGL >= 40 && MAX_CONFIG_RENDERER_OPENGL <= 46
#		if MAX_CONFIG_RENDERER_OPENGL == 40
#			define MAX_RENDERER_OPENGL_NAME "OpenGL 4.0"
#		elif MAX_CONFIG_RENDERER_OPENGL == 41
#			define MAX_RENDERER_OPENGL_NAME "OpenGL 4.1"
#		elif MAX_CONFIG_RENDERER_OPENGL == 42
#			define MAX_RENDERER_OPENGL_NAME "OpenGL 4.2"
#		elif MAX_CONFIG_RENDERER_OPENGL == 43
#			define MAX_RENDERER_OPENGL_NAME "OpenGL 4.3"
#		elif MAX_CONFIG_RENDERER_OPENGL == 44
#			define MAX_RENDERER_OPENGL_NAME "OpenGL 4.4"
#		elif MAX_CONFIG_RENDERER_OPENGL == 45
#			define MAX_RENDERER_OPENGL_NAME "OpenGL 4.5"
#		else
#			define MAX_RENDERER_OPENGL_NAME "OpenGL 4.6"
#		endif // 40+
#	else
#		define MAX_RENDERER_OPENGL_NAME "OpenGL 2.1"
#	endif // MAX_CONFIG_RENDERER_OPENGL
#elif MAX_CONFIG_RENDERER_OPENGLES
#	if MAX_CONFIG_RENDERER_OPENGLES == 30
#		define MAX_RENDERER_OPENGL_NAME "OpenGL ES 3.0"
#	elif MAX_CONFIG_RENDERER_OPENGLES == 31
#		define MAX_RENDERER_OPENGL_NAME "OpenGL ES 3.1"
#	elif MAX_CONFIG_RENDERER_OPENGLES >= 32
#		define MAX_RENDERER_OPENGL_NAME "OpenGL ES 3.2"
#	else
#		define MAX_RENDERER_OPENGL_NAME "OpenGL ES 2.0"
#	endif // MAX_CONFIG_RENDERER_OPENGLES
#else
#	define MAX_RENDERER_OPENGL_NAME "OpenGL"
#endif //

namespace max
{
	int main(int _argc, const char* const* _argv);

	extern InternalData g_internalData;
	extern PlatformData g_platformData;
	extern bool g_platformDataChangedSinceReset;
	extern void isFrameBufferValid(uint8_t _num, const Attachment* _attachment, bx::Error* _err);
	extern void isIdentifierValid(const bx::StringView& _name, bx::Error* _err);

#if MAX_CONFIG_MAX_DRAW_CALLS < (64<<10)
	typedef uint16_t RenderItemCount;
#else
	typedef uint32_t RenderItemCount;
#endif // MAX_CONFIG_MAX_DRAW_CALLS < (64<<10)

	///
	struct Handle
	{
		///
		struct TypeName
		{
			const char* abrvName;
			const char* fullName;
		};

		///
		enum Enum
		{
			DynamicIndexBuffer,
			DynamicVertexBuffer,
			FrameBuffer,
			IndexBuffer,
			IndirectBuffer,
			OcclusionQuery,
			Program,
			Shader,
			Texture,
			Uniform,
			VertexBuffer,
			VertexLayout,

			Count
		};

		template<typename Ty>
		static constexpr Enum toEnum();

		constexpr Handle()
			: idx(kInvalidHandle)
			, type(Count)
		{
		}

		template<typename Ty>
		constexpr Handle(Ty _handle)
			: idx(_handle.idx)
			, type(uint16_t(toEnum<Ty>() ) )
		{
		}

		template<typename Ty>
		constexpr Ty to() const
		{
			if (type == toEnum<Ty>() )
			{
				return Ty{ idx };
			}

			BX_ASSERT(type == toEnum<Ty>(), "Handle type %s, cannot be converted to %s."
				, getTypeName().fullName
				, getTypeName(toEnum<Ty>() ).fullName
				);
			return { kInvalidHandle };
		}

		Enum getType() const
		{
			return Enum(type);
		}

		static const TypeName& getTypeName(Handle::Enum _enum);

		const TypeName& getTypeName() const
		{
			return getTypeName(getType() );
		}

		bool isBuffer() const
		{
			return false
				|| type == DynamicIndexBuffer
				|| type == DynamicVertexBuffer
				|| type == IndexBuffer
				|| type == IndirectBuffer
				|| type == VertexBuffer
				;
		}

		bool isTexture() const
		{
			return type == Texture;
		}

		uint16_t idx;
		uint16_t type;
	};

#define IMPLEMENT_HANDLE(_name)                                   \
	template<>                                                    \
	inline constexpr Handle::Enum Handle::toEnum<_name##Handle>() \
	{                                                             \
		return Handle::_name;                                     \
	}                                                             \

	IMPLEMENT_HANDLE(DynamicIndexBuffer);
	IMPLEMENT_HANDLE(DynamicVertexBuffer);
	IMPLEMENT_HANDLE(FrameBuffer);
	IMPLEMENT_HANDLE(IndexBuffer);
	IMPLEMENT_HANDLE(IndirectBuffer);
	IMPLEMENT_HANDLE(OcclusionQuery);
	IMPLEMENT_HANDLE(Program);
	IMPLEMENT_HANDLE(Shader);
	IMPLEMENT_HANDLE(Texture);
	IMPLEMENT_HANDLE(Uniform);
	IMPLEMENT_HANDLE(VertexBuffer);
	IMPLEMENT_HANDLE(VertexLayout);

#undef IMPLEMENT_HANDLE

	struct Event
	{
		enum Enum
		{
			Axis,
			Char,
			Exit,
			Gamepad,
			Key,
			Mouse,
			Size,
			Window,
			Suspend,
			DropFile,
		};

		Event(Enum _type)
			: m_type(_type)
		{
			m_handle.idx = UINT16_MAX;
		}

		Event(Enum _type, WindowHandle _handle)
			: m_type(_type)
			, m_handle(_handle)
		{
		}

		Event::Enum m_type;
		WindowHandle m_handle;
	};

	struct AxisEvent : public Event
	{
		MAX_IMPLEMENT_EVENT(AxisEvent, Event::Axis);

		GamepadAxis::Enum m_axis;
		int32_t m_value;
		GamepadHandle m_gamepad;
	};

	struct CharEvent : public Event
	{
		MAX_IMPLEMENT_EVENT(CharEvent, Event::Char);

		uint8_t m_len;
		uint8_t m_char[4];
	};

	struct GamepadEvent : public Event
	{
		MAX_IMPLEMENT_EVENT(GamepadEvent, Event::Gamepad);

		GamepadHandle m_gamepad;
		bool m_connected;
	};

	struct KeyEvent : public Event
	{
		MAX_IMPLEMENT_EVENT(KeyEvent, Event::Key);

		Key::Enum m_key;
		uint8_t m_modifiers;
		bool m_down;
	};

	struct MouseEvent : public Event
	{
		MAX_IMPLEMENT_EVENT(MouseEvent, Event::Mouse);

		int32_t m_mx;
		int32_t m_my;
		int32_t m_mz;
		MouseButton::Enum m_button;
		bool m_down;
		bool m_move;
	};

	struct SizeEvent : public Event
	{
		MAX_IMPLEMENT_EVENT(SizeEvent, Event::Size);

		uint32_t m_width;
		uint32_t m_height;
	};

	struct WindowEvent : public Event
	{
		MAX_IMPLEMENT_EVENT(WindowEvent, Event::Window);

		void* m_nwh;
	};

	struct SuspendEvent : public Event
	{
		MAX_IMPLEMENT_EVENT(SuspendEvent, Event::Suspend);

		Suspend::Enum m_state;
	};

	struct DropFileEvent : public Event
	{
		MAX_IMPLEMENT_EVENT(DropFileEvent, Event::DropFile);

		bx::FilePath m_filePath;
	};

	const Event* poll();
	const Event* poll(WindowHandle _handle);
	void release(const Event* _event);

	class EventQueue
	{
	public:
		EventQueue()
			: m_queue(getAllocator())
		{
		}

		~EventQueue()
		{
			for (const Event* ev = poll(); NULL != ev; ev = poll())
			{
				release(ev);
			}
		}

		void postAxisEvent(WindowHandle _handle, GamepadHandle _gamepad, GamepadAxis::Enum _axis, int32_t _value)
		{
			AxisEvent* ev = BX_NEW(getAllocator(), AxisEvent)(_handle);
			ev->m_gamepad = _gamepad;
			ev->m_axis = _axis;
			ev->m_value = _value;
			m_queue.push(ev);
		}

		void postCharEvent(WindowHandle _handle, uint8_t _len, const uint8_t _char[4])
		{
			CharEvent* ev = BX_NEW(getAllocator(), CharEvent)(_handle);
			ev->m_len = _len;
			bx::memCopy(ev->m_char, _char, 4);
			m_queue.push(ev);
		}

		void postExitEvent()
		{
			Event* ev = BX_NEW(getAllocator(), Event)(Event::Exit);
			m_queue.push(ev);
		}

		void postGamepadEvent(WindowHandle _handle, GamepadHandle _gamepad, bool _connected)
		{
			GamepadEvent* ev = BX_NEW(getAllocator(), GamepadEvent)(_handle);
			ev->m_gamepad = _gamepad;
			ev->m_connected = _connected;
			m_queue.push(ev);
		}

		void postKeyEvent(WindowHandle _handle, Key::Enum _key, uint8_t _modifiers, bool _down)
		{
			KeyEvent* ev = BX_NEW(getAllocator(), KeyEvent)(_handle);
			ev->m_key = _key;
			ev->m_modifiers = _modifiers;
			ev->m_down = _down;
			m_queue.push(ev);
		}

		void postMouseEvent(WindowHandle _handle, int32_t _mx, int32_t _my, int32_t _mz)
		{
			MouseEvent* ev = BX_NEW(getAllocator(), MouseEvent)(_handle);
			ev->m_mx = _mx;
			ev->m_my = _my;
			ev->m_mz = _mz;
			ev->m_button = MouseButton::None;
			ev->m_down = false;
			ev->m_move = true;
			m_queue.push(ev);
		}

		void postMouseEvent(WindowHandle _handle, int32_t _mx, int32_t _my, int32_t _mz, MouseButton::Enum _button, bool _down)
		{
			MouseEvent* ev = BX_NEW(getAllocator(), MouseEvent)(_handle);
			ev->m_mx = _mx;
			ev->m_my = _my;
			ev->m_mz = _mz;
			ev->m_button = _button;
			ev->m_down = _down;
			ev->m_move = false;
			m_queue.push(ev);
		}

		void postSizeEvent(WindowHandle _handle, uint32_t _width, uint32_t _height)
		{
			SizeEvent* ev = BX_NEW(getAllocator(), SizeEvent)(_handle);
			ev->m_width = _width;
			ev->m_height = _height;
			m_queue.push(ev);
		}

		void postWindowEvent(WindowHandle _handle, void* _nwh = NULL)
		{
			WindowEvent* ev = BX_NEW(getAllocator(), WindowEvent)(_handle);
			ev->m_nwh = _nwh;
			m_queue.push(ev);
		}

		void postSuspendEvent(WindowHandle _handle, Suspend::Enum _state)
		{
			SuspendEvent* ev = BX_NEW(getAllocator(), SuspendEvent)(_handle);
			ev->m_state = _state;
			m_queue.push(ev);
		}

		void postDropFileEvent(WindowHandle _handle, const bx::FilePath& _filePath)
		{
			DropFileEvent* ev = BX_NEW(getAllocator(), DropFileEvent)(_handle);
			ev->m_filePath = _filePath;
			m_queue.push(ev);
		}

		const Event* poll()
		{
			return m_queue.pop();
		}

		const Event* poll(WindowHandle _handle)
		{
			if (isValid(_handle))
			{
				Event* ev = m_queue.peek();
				if (NULL == ev
					|| ev->m_handle.idx != _handle.idx)
				{
					return NULL;
				}
			}

			return poll();
		}

		void release(const Event* _event) const
		{
			bx::deleteObject(getAllocator(), const_cast<Event*>(_event));
		}

	private:
		bx::SpScUnboundedQueueT<Event> m_queue;
	};

	inline bool isValid(const VertexLayout& _layout)
	{
		return 0 != _layout.m_stride;
	}

	struct Condition
	{
		enum Enum
		{
			LessEqual,
			GreaterEqual,
		};
	};

	bool windowsVersionIs(Condition::Enum _op, uint32_t _version, uint32_t _build = UINT32_MAX);

	constexpr bool isShaderType(uint32_t _magic, char _type)
	{
		return uint32_t(_type) == (_magic & BX_MAKEFOURCC(0xff, 0, 0, 0) );
	}

	inline bool isShaderBin(uint32_t _magic)
	{
		return BX_MAKEFOURCC(0, 'S', 'H', 0) == (_magic & BX_MAKEFOURCC(0, 0xff, 0xff, 0) )
			&& (isShaderType(_magic, 'C') || isShaderType(_magic, 'F') || isShaderType(_magic, 'V') )
			;
	}

	inline bool isShaderVerLess(uint32_t _magic, uint8_t _version)
	{
		return (_magic & BX_MAKEFOURCC(0, 0, 0, 0xff) ) < BX_MAKEFOURCC(0, 0, 0, _version);
	}

	const char* getShaderTypeName(uint32_t _magic);

	struct Clear
	{
		void set(uint16_t _flags, uint32_t _rgba, float _depth, uint8_t _stencil)
		{
			m_flags    = _flags;
			m_index[0] = uint8_t(_rgba>>24);
			m_index[1] = uint8_t(_rgba>>16);
			m_index[2] = uint8_t(_rgba>> 8);
			m_index[3] = uint8_t(_rgba>> 0);
			m_depth    = _depth;
			m_stencil  = _stencil;
		}

		void set(uint16_t _flags, float _depth, uint8_t _stencil, uint8_t _0, uint8_t _1, uint8_t _2, uint8_t _3, uint8_t _4, uint8_t _5, uint8_t _6, uint8_t _7)
		{
			m_flags = (_flags & ~MAX_CLEAR_COLOR)
				| (0xff != (_0&_1&_2&_3&_4&_5&_6&_7) ? MAX_CLEAR_COLOR|MAX_CLEAR_COLOR_USE_PALETTE : 0)
				;
			m_index[0] = _0;
			m_index[1] = _1;
			m_index[2] = _2;
			m_index[3] = _3;
			m_index[4] = _4;
			m_index[5] = _5;
			m_index[6] = _6;
			m_index[7] = _7;
			m_depth    = _depth;
			m_stencil  = _stencil;
		}

		uint8_t  m_index[8];
		float    m_depth;
		uint8_t  m_stencil;
		uint16_t m_flags;
	};

	struct Rect
	{
		Rect()
		{
		}

		Rect(uint16_t _x, uint16_t _y, uint16_t _width, uint16_t _height)
			: m_x(_x)
			, m_y(_y)
			, m_width(_width)
			, m_height(_height)
		{
		}

		void clear()
		{
			m_x      = 0;
			m_y      = 0;
			m_width  = 0;
			m_height = 0;
		}

		bool isZero() const
		{
			uint64_t ui64 = *( (uint64_t*)this);
			return UINT64_C(0) == ui64;
		}

		bool isZeroArea() const
		{
			return 0 == m_width
				|| 0 == m_height
				;
		}

		void set(uint16_t _x, uint16_t _y, uint16_t _width, uint16_t _height)
		{
			m_x = _x;
			m_y = _y;
			m_width  = _width;
			m_height = _height;
		}

		void setIntersect(const Rect& _a, const Rect& _b)
		{
			const uint16_t sx = bx::max<uint16_t>(_a.m_x, _b.m_x);
			const uint16_t sy = bx::max<uint16_t>(_a.m_y, _b.m_y);
			const uint16_t ex = bx::min<uint16_t>(_a.m_x + _a.m_width,  _b.m_x + _b.m_width );
			const uint16_t ey = bx::min<uint16_t>(_a.m_y + _a.m_height, _b.m_y + _b.m_height);
			m_x = sx;
			m_y = sy;
			m_width  = (uint16_t)bx::uint32_satsub(ex, sx);
			m_height = (uint16_t)bx::uint32_satsub(ey, sy);
		}

		void intersect(const Rect& _a)
		{
			setIntersect(*this, _a);
		}

		uint16_t m_x;
		uint16_t m_y;
		uint16_t m_width;
		uint16_t m_height;
	};

	struct TextureCreate
	{
		TextureFormat::Enum m_format;
		uint16_t m_width;
		uint16_t m_height;
		uint16_t m_depth;
		uint16_t m_numLayers;
		uint8_t  m_numMips;
		bool     m_cubeMap;
		const Memory* m_mem;
	};

	extern const uint32_t g_uniformTypeSize[UniformType::Count+1];
	extern CallbackI* g_callback;
	extern bx::AllocatorI* g_allocator;
	extern Caps g_caps;

	typedef bx::StringT<&g_allocator> String;

	struct ProfilerScope
	{
		ProfilerScope(const char* _name, uint32_t _abgr, const char* _filePath, uint16_t _line)
		{
			g_callback->profilerBeginLiteral(_name, _abgr, _filePath, _line);
		}

		~ProfilerScope()
		{
			g_callback->profilerEnd();
		}
	};

	void setGraphicsDebuggerPresent(bool _present);
	bool isGraphicsDebuggerPresent();
	void release(const Memory* _mem);
	const char* getAttribName(Attrib::Enum _attr);
	const char* getAttribNameShort(Attrib::Enum _attr);
	void getTextureSizeFromRatio(BackbufferRatio::Enum _ratio, uint16_t& _width, uint16_t& _height);
	TextureFormat::Enum getViableTextureFormat(const bimg::ImageContainer& _imageContainer);
	const char* getName(TextureFormat::Enum _fmt);
	const char* getName(UniformHandle _handle);
	const char* getName(ShaderHandle _handle);
	const char* getName(Topology::Enum _topology);

	template<typename Ty>
	inline void release(Ty)
	{
	}

	template<>
	inline void release(Memory* _mem)
	{
		release( (const Memory*)_mem);
	}

	inline uint64_t packStencil(uint32_t _fstencil, uint32_t _bstencil)
	{
		return (uint64_t(_bstencil)<<32)|uint64_t(_fstencil);
	}

	inline uint32_t unpackStencil(uint8_t _0or1, uint64_t _stencil)
	{
		return uint32_t( (_stencil >> (32*_0or1) ) );
	}

	inline bool needBorderColor(uint64_t _flags)
	{
		return MAX_SAMPLER_U_BORDER == (_flags & MAX_SAMPLER_U_BORDER)
			|| MAX_SAMPLER_V_BORDER == (_flags & MAX_SAMPLER_V_BORDER)
			|| MAX_SAMPLER_W_BORDER == (_flags & MAX_SAMPLER_W_BORDER)
			;
	}

	inline uint8_t calcNumMips(bool _hasMips, uint16_t _width, uint16_t _height, uint16_t _depth = 1)
	{
		if (_hasMips)
		{
			const uint32_t max = bx::max(_width, _height, _depth);
			const uint32_t num = 1 + bx::floorLog2(max);

			return uint8_t(num);
		}

		return 1;
	}

	/// Dump vertex layout info into debug output.
	void dump(const VertexLayout& _layout);

	/// Dump resolution and reset info into debug output.
	void dump(const Resolution& _resolution);

	struct TextVideoMem
	{
		TextVideoMem()
			: m_mem(NULL)
			, m_size(0)
			, m_width(0)
			, m_height(0)
			, m_small(false)
		{
			resize(false, 1, 1);
			clear();
		}

		~TextVideoMem()
		{
			bx::free(g_allocator, m_mem);
		}

		void resize(bool _small, uint32_t _width, uint32_t _height)
		{
			uint32_t width  = bx::uint32_imax(1, _width/8);
			uint32_t height = bx::uint32_imax(1, _height/(_small ? 8 : 16) );

			if (NULL == m_mem
			||  m_width  != width
			||  m_height != height
			||  m_small  != _small)
			{
				m_small  = _small;
				m_width  = bx::narrowCast<uint16_t>(width);
				m_height = bx::narrowCast<uint16_t>(height);

				uint32_t size = m_size;
				m_size = m_width * m_height;

				m_mem = (MemSlot*)bx::realloc(g_allocator, m_mem, m_size * sizeof(MemSlot) );

				if (size < m_size)
				{
					bx::memSet(&m_mem[size], 0, (m_size-size) * sizeof(MemSlot) );
				}
			}
		}

		void clear(uint8_t _attr = 0)
		{
			MemSlot* mem = m_mem;
			bx::memSet(mem, 0, m_size * sizeof(MemSlot) );
			if (_attr != 0)
			{
				for (uint32_t ii = 0, num = m_size; ii < num; ++ii)
				{
					mem[ii].attribute = _attr;
				}
			}
		}

		void printfVargs(uint16_t _x, uint16_t _y, uint8_t _attr, const char* _format, va_list _argList);

		void printf(uint16_t _x, uint16_t _y, uint8_t _attr, const char* _format, ...)
		{
			va_list argList;
			va_start(argList, _format);
			printfVargs(_x, _y, _attr, _format, argList);
			va_end(argList);
		}

		void image(uint16_t _x, uint16_t _y, uint16_t _width, uint16_t _height, const void* _data, uint16_t _pitch)
		{
			if (_x < m_width && _y < m_height)
			{
				MemSlot* dst = &m_mem[_y*m_width+_x];
				const uint8_t* src = (const uint8_t*)_data;
				const uint32_t width  =  bx::min<uint32_t>(m_width,  _width +_x)-_x;
				const uint32_t height =  bx::min<uint32_t>(m_height, _height+_y)-_y;
				const uint32_t dstPitch = m_width;

				for (uint32_t ii = 0; ii < height; ++ii)
				{
					for (uint32_t jj = 0; jj < width; ++jj)
					{
						dst[jj].character = src[jj*2];
						dst[jj].attribute = src[jj*2+1];
					}

					src += _pitch;
					dst += dstPitch;
				}
			}
		}

		struct MemSlot
		{
			uint8_t attribute;
			uint8_t character;
		};

		MemSlot* m_mem;
		uint32_t m_size;
		uint16_t m_width;
		uint16_t m_height;
		bool m_small;
	};

	struct TextVideoMemBlitter
	{
		void init(uint8_t scale);
		void shutdown();

		TextureHandle m_texture;
		TransientVertexBuffer* m_vb;
		TransientIndexBuffer* m_ib;
		VertexLayout m_layout;
		ProgramHandle m_program;
		uint8_t m_scale;
	};

	struct RendererContextI;

	extern void blit(RendererContextI* _renderCtx, TextVideoMemBlitter& _blitter, const TextVideoMem& _mem);

	inline void blit(RendererContextI* _renderCtx, TextVideoMemBlitter& _blitter, const TextVideoMem* _mem)
	{
		blit(_renderCtx, _blitter, *_mem);
	}

	template <uint32_t maxKeys>
	struct UpdateBatchT
	{
		UpdateBatchT()
			: m_num(0)
		{
		}

		void add(uint32_t _key, uint32_t _value)
		{
			uint32_t num = m_num++;
			m_keys[num] = _key;
			m_values[num] = _value;
		}

		bool sort()
		{
			if (0 < m_num)
			{
				uint32_t* tempKeys = (uint32_t*)alloca(sizeof(m_keys) );
				uint32_t* tempValues = (uint32_t*)alloca(sizeof(m_values) );
				bx::radixSort(m_keys, tempKeys, m_values, tempValues, m_num);
				return true;
			}

			return false;
		}

		bool isFull() const
		{
			return m_num >= maxKeys;
		}

		void reset()
		{
			m_num = 0;
		}

		uint32_t m_num;
		uint32_t m_keys[maxKeys];
		uint32_t m_values[maxKeys];
	};

	struct ClearQuad
	{
		ClearQuad()
		{
			for (uint32_t ii = 0; ii < BX_COUNTOF(m_program); ++ii)
			{
				m_program[ii].idx = kInvalidHandle;
			}
		}

		void init();
		void shutdown();

		VertexBufferHandle m_vb;
		VertexLayout m_layout;
		ProgramHandle m_program[MAX_CONFIG_MAX_FRAME_BUFFER_ATTACHMENTS];
	};

	struct PredefinedUniform
	{
		enum Enum
		{
			ViewRect,
			ViewTexel,
			View,
			InvView,
			Proj,
			InvProj,
			ViewProj,
			InvViewProj,
			Model,
			ModelView,
			ModelViewProj,
			AlphaRef,
			Count
		};

		uint32_t m_loc;
		uint16_t m_count;
		uint8_t m_type;
	};

	const char* getUniformTypeName(UniformType::Enum _enum);
	UniformType::Enum nameToUniformTypeEnum(const char* _name);
	const char* getPredefinedUniformName(PredefinedUniform::Enum _enum);
	PredefinedUniform::Enum nameToPredefinedUniformEnum(const bx::StringView& _name);

	class CommandBuffer
	{
		BX_CLASS(CommandBuffer
			, NO_COPY
			);

	public:
		CommandBuffer()
			: m_buffer(NULL)
			, m_pos(0)
			, m_size(0)
			, m_minCapacity(0)
		{
			resize();
			finish();
		}

		~CommandBuffer()
		{
			bx::free(g_allocator, m_buffer);
		}

		void init(uint32_t _minCapacity)
		{
			m_minCapacity = bx::alignUp(_minCapacity, 1024);
			resize();
		}

		enum Enum
		{
			RendererInit,
			RendererShutdownBegin,
			CreateVertexLayout,
			CreateIndexBuffer,
			CreateVertexBuffer,
			CreateDynamicIndexBuffer,
			UpdateDynamicIndexBuffer,
			CreateDynamicVertexBuffer,
			UpdateDynamicVertexBuffer,
			CreateShader,
			CreateProgram,
			CreateTexture,
			UpdateTexture,
			ResizeTexture,
			CreateFrameBuffer,
			CreateUniform,
			UpdateViewName,
			InvalidateOcclusionQuery,
			SetName,
			End,
			RendererShutdownEnd,
			DestroyVertexLayout,
			DestroyIndexBuffer,
			DestroyVertexBuffer,
			DestroyDynamicIndexBuffer,
			DestroyDynamicVertexBuffer,
			DestroyShader,
			DestroyProgram,
			DestroyTexture,
			DestroyFrameBuffer,
			DestroyUniform,
			ReadTexture,
		};

		void resize(uint32_t _capacity = 0)
		{
			m_capacity = bx::alignUp(bx::max(_capacity, m_minCapacity), 1024);
			m_buffer = (uint8_t*)bx::realloc(g_allocator, m_buffer, m_capacity);
		}

		void write(const void* _data, uint32_t _size)
		{
			BX_ASSERT(m_size == 0, "Called write outside start/finish (m_size: %d)?", m_size);
			if (m_pos + _size > m_capacity)
			{
				resize(m_capacity + (16<<10) );
			}

			bx::memCopy(&m_buffer[m_pos], _data, _size);
			m_pos += _size;
		}

		template<typename Type>
		void write(const Type& _in)
		{
			align(BX_ALIGNOF(Type) );
			write(reinterpret_cast<const uint8_t*>(&_in), sizeof(Type) );
		}

		void write(const bx::StringView& _str)
		{
			const uint16_t len = bx::narrowCast<uint16_t>(_str.getLength()+1);
			write(len);
			write(_str.getPtr(), len-1);
			write('\0');
		}

		void read(void* _data, uint32_t _size)
		{
			BX_ASSERT(m_pos + _size <= m_size
				, "CommandBuffer::read error (pos: %d-%d, size: %d)."
				, m_pos
				, m_pos + _size
				, m_size
				);
			bx::memCopy(_data, &m_buffer[m_pos], _size);
			m_pos += _size;
		}

		template<typename Type>
		void read(Type& _in)
		{
			align(BX_ALIGNOF(Type) );
			read(reinterpret_cast<uint8_t*>(&_in), sizeof(Type) );
		}

		const uint8_t* skip(uint32_t _size)
		{
			BX_ASSERT(m_pos + _size <= m_size
				, "CommandBuffer::skip error (pos: %d-%d, size: %d)."
				, m_pos
				, m_pos + _size
				, m_size
				);
			const uint8_t* result = &m_buffer[m_pos];
			m_pos += _size;
			return result;
		}

		template<typename Type>
		void skip()
		{
			align(BX_ALIGNOF(Type) );
			skip(sizeof(Type) );
		}

		void align(uint32_t _alignment)
		{
			const uint32_t mask = _alignment-1;
			const uint32_t pos = (m_pos+mask) & (~mask);
			m_pos = pos;
		}

		void reset()
		{
			m_pos = 0;
		}

		void start()
		{
			m_pos = 0;
			m_size = 0;
		}

		void finish()
		{
			uint8_t cmd = End;
			write(cmd);
			m_size = m_pos;
			m_pos = 0;

			if (m_size < m_minCapacity
			&&  m_capacity != m_minCapacity)
			{
				resize();
			}
		}

		uint8_t* m_buffer;
		uint32_t m_pos;
		uint32_t m_size;
		uint32_t m_capacity;
		uint32_t m_minCapacity;
	};

	//
	constexpr uint8_t  kSortKeyViewNumBits         = uint8_t(31 - bx::uint32_cntlz(MAX_CONFIG_MAX_VIEWS) );
	constexpr uint8_t  kSortKeyViewBitShift        = 64-kSortKeyViewNumBits;
	constexpr uint64_t kSortKeyViewMask            = uint64_t(MAX_CONFIG_MAX_VIEWS-1)<<kSortKeyViewBitShift;

	constexpr uint8_t  kSortKeyDrawBitShift        = kSortKeyViewBitShift - 1;
	constexpr uint64_t kSortKeyDrawBit             = uint64_t(1)<<kSortKeyDrawBitShift;

	//
	constexpr uint8_t  kSortKeyDrawTypeNumBits     = 2;
	constexpr uint8_t  kSortKeyDrawTypeBitShift    = kSortKeyDrawBitShift - kSortKeyDrawTypeNumBits;
	constexpr uint64_t kSortKeyDrawTypeMask        = uint64_t(3)<<kSortKeyDrawTypeBitShift;

	constexpr uint64_t kSortKeyDrawTypeProgram     = uint64_t(0)<<kSortKeyDrawTypeBitShift;
	constexpr uint64_t kSortKeyDrawTypeDepth       = uint64_t(1)<<kSortKeyDrawTypeBitShift;
	constexpr uint64_t kSortKeyDrawTypeSequence    = uint64_t(2)<<kSortKeyDrawTypeBitShift;

	//
	constexpr uint8_t  kSortKeyTransNumBits        = 2;

	constexpr uint8_t  kSortKeyDraw0BlendShift     = kSortKeyDrawTypeBitShift - kSortKeyTransNumBits;
	constexpr uint64_t kSortKeyDraw0BlendMask      = uint64_t(0x3)<<kSortKeyDraw0BlendShift;

	constexpr uint8_t  kSortKeyDraw0ProgramShift   = kSortKeyDraw0BlendShift - MAX_CONFIG_SORT_KEY_NUM_BITS_PROGRAM;
	constexpr uint64_t kSortKeyDraw0ProgramMask    = uint64_t(MAX_CONFIG_MAX_PROGRAMS-1)<<kSortKeyDraw0ProgramShift;

	constexpr uint8_t  kSortKeyDraw0DepthShift     = kSortKeyDraw0ProgramShift - MAX_CONFIG_SORT_KEY_NUM_BITS_DEPTH;
	constexpr uint64_t kSortKeyDraw0DepthMask      = ( (uint64_t(1)<<MAX_CONFIG_SORT_KEY_NUM_BITS_DEPTH)-1)<<kSortKeyDraw0DepthShift;

	//
	constexpr uint8_t  kSortKeyDraw1DepthShift     = kSortKeyDrawTypeBitShift - MAX_CONFIG_SORT_KEY_NUM_BITS_DEPTH;
	constexpr uint64_t kSortKeyDraw1DepthMask      = ( (uint64_t(1)<<MAX_CONFIG_SORT_KEY_NUM_BITS_DEPTH)-1)<<kSortKeyDraw1DepthShift;

	constexpr uint8_t  kSortKeyDraw1BlendShift     = kSortKeyDraw1DepthShift - kSortKeyTransNumBits;
	constexpr uint64_t kSortKeyDraw1BlendMask      = uint64_t(0x3)<<kSortKeyDraw1BlendShift;

	constexpr uint8_t  kSortKeyDraw1ProgramShift   = kSortKeyDraw1BlendShift - MAX_CONFIG_SORT_KEY_NUM_BITS_PROGRAM;
	constexpr uint64_t kSortKeyDraw1ProgramMask    = uint64_t(MAX_CONFIG_MAX_PROGRAMS-1)<<kSortKeyDraw1ProgramShift;

	//
	constexpr uint8_t  kSortKeyDraw2SeqShift       = kSortKeyDrawTypeBitShift - MAX_CONFIG_SORT_KEY_NUM_BITS_SEQ;
	constexpr uint64_t kSortKeyDraw2SeqMask        = ( (uint64_t(1)<<MAX_CONFIG_SORT_KEY_NUM_BITS_SEQ)-1)<<kSortKeyDraw2SeqShift;

	constexpr uint8_t  kSortKeyDraw2BlendShift     = kSortKeyDraw2SeqShift - kSortKeyTransNumBits;
	constexpr uint64_t kSortKeyDraw2BlendMask      = uint64_t(0x3)<<kSortKeyDraw2BlendShift;

	constexpr uint8_t  kSortKeyDraw2ProgramShift   = kSortKeyDraw2BlendShift - MAX_CONFIG_SORT_KEY_NUM_BITS_PROGRAM;
	constexpr uint64_t kSortKeyDraw2ProgramMask    = uint64_t(MAX_CONFIG_MAX_PROGRAMS-1)<<kSortKeyDraw2ProgramShift;

	//
	constexpr uint8_t  kSortKeyComputeSeqShift     = kSortKeyDrawBitShift - MAX_CONFIG_SORT_KEY_NUM_BITS_SEQ;
	constexpr uint64_t kSortKeyComputeSeqMask      = ( (uint64_t(1)<<MAX_CONFIG_SORT_KEY_NUM_BITS_SEQ)-1)<<kSortKeyComputeSeqShift;

	constexpr uint8_t  kSortKeyComputeProgramShift = kSortKeyComputeSeqShift - MAX_CONFIG_SORT_KEY_NUM_BITS_PROGRAM;
	constexpr uint64_t kSortKeyComputeProgramMask  = uint64_t(MAX_CONFIG_MAX_PROGRAMS-1)<<kSortKeyComputeProgramShift;

	BX_STATIC_ASSERT(MAX_CONFIG_MAX_VIEWS <= (1<<kSortKeyViewNumBits) );
	BX_STATIC_ASSERT( (MAX_CONFIG_MAX_PROGRAMS & (MAX_CONFIG_MAX_PROGRAMS-1) ) == 0); // Must be power of 2.
	BX_STATIC_ASSERT( (0 // Render key mask shouldn't overlap.
		| kSortKeyViewMask
		| kSortKeyDrawBit
		| kSortKeyDrawTypeMask
		| kSortKeyDraw0BlendMask
		| kSortKeyDraw0ProgramMask
		| kSortKeyDraw0DepthMask
		) == (0
		^ kSortKeyViewMask
		^ kSortKeyDrawBit
		^ kSortKeyDrawTypeMask
		^ kSortKeyDraw0BlendMask
		^ kSortKeyDraw0ProgramMask
		^ kSortKeyDraw0DepthMask
		) );
	BX_STATIC_ASSERT( (0 // Render key mask shouldn't overlap.
		| kSortKeyViewMask
		| kSortKeyDrawBit
		| kSortKeyDrawTypeMask
		| kSortKeyDraw1DepthMask
		| kSortKeyDraw1BlendMask
		| kSortKeyDraw1ProgramMask
		) == (0
		^ kSortKeyViewMask
		^ kSortKeyDrawBit
		^ kSortKeyDrawTypeMask
		^ kSortKeyDraw1DepthMask
		^ kSortKeyDraw1BlendMask
		^ kSortKeyDraw1ProgramMask
		) );
	BX_STATIC_ASSERT( (0 // Render key mask shouldn't overlap.
		| kSortKeyViewMask
		| kSortKeyDrawBit
		| kSortKeyDrawTypeMask
		| kSortKeyDraw2SeqMask
		| kSortKeyDraw2BlendMask
		| kSortKeyDraw2ProgramMask
		) == (0
		^ kSortKeyViewMask
		^ kSortKeyDrawBit
		^ kSortKeyDrawTypeMask
		^ kSortKeyDraw2SeqMask
		^ kSortKeyDraw2BlendMask
		^ kSortKeyDraw2ProgramMask
		) );
	BX_STATIC_ASSERT( (0 // Compute key mask shouldn't overlap.
		| kSortKeyViewMask
		| kSortKeyDrawBit
		| kSortKeyComputeSeqShift
		| kSortKeyComputeProgramMask
		) == (0
		^ kSortKeyViewMask
		^ kSortKeyDrawBit
		^ kSortKeyComputeSeqShift
		^ kSortKeyComputeProgramMask
		) );

	// |               3               2               1               0|
	// |fedcba9876543210fedcba9876543210fedcba9876543210fedcba9876543210| Common
	// |vvvvvvvvd                                                       |
	// |       ^^                                                       |
	// |       ||                                                       |
	// |  view-+|                                                       |
	// |        +-draw                                                  |
	// |----------------------------------------------------------------| Draw Key 0 - Sort by program
	// |        |kkttpppppppppdddddddddddddddddddddddddddddddd          |
	// |        | ^ ^        ^                               ^          |
	// |        | | |        |                               |          |
	// |        | | +-blend  +-program                 depth-+          |
	// |        | +-key type                                            |
	// |----------------------------------------------------------------| Draw Key 1 - Sort by depth
	// |        |kkddddddddddddddddddddddddddddddddttppppppppp          |
	// |        | ^                               ^ ^        ^          |
	// |        | |                               | +-blend  |          |
	// |        | +-key type                depth-+  program-+          |
	// |        |                                                       |
	// |----------------------------------------------------------------| Draw Key 2 - Sequential
	// |        |kkssssssssssssssssssssttppppppppp                      |
	// |        | ^                   ^ ^        ^                      |
	// |        | |                   | |        |                      |
	// |        | +-key type      seq-+ +-blend  +-program              |
	// |        |                                                       |
	// |----------------------------------------------------------------| Compute Key
	// |        |ssssssssssssssssssssppppppppp                          |
	// |        |                   ^        ^                          |
	// |        |                   |        |                          |
	// |        |               seq-+        +-program                  |
	// |        |                                                       |
	// |--------+-------------------------------------------------------|
	//
	struct SortKey
	{
		enum Enum
		{
			SortProgram,
			SortDepth,
			SortSequence,
		};

		uint64_t encodeDraw(Enum _type)
		{
			switch (_type)
			{
			case SortProgram:
				{
					const uint64_t depth   = (uint64_t(m_depth      ) << kSortKeyDraw0DepthShift  ) & kSortKeyDraw0DepthMask;
					const uint64_t program = (uint64_t(m_program.idx) << kSortKeyDraw0ProgramShift) & kSortKeyDraw0ProgramMask;
					const uint64_t blend   = (uint64_t(m_blend      ) << kSortKeyDraw0BlendShift  ) & kSortKeyDraw0BlendMask;
					const uint64_t view    = (uint64_t(m_view       ) << kSortKeyViewBitShift     ) & kSortKeyViewMask;
					const uint64_t key     = view|kSortKeyDrawBit|kSortKeyDrawTypeProgram|blend|program|depth;

					return key;
				}
				break;

			case SortDepth:
				{
					const uint64_t depth   = (uint64_t(m_depth      ) << kSortKeyDraw1DepthShift  ) & kSortKeyDraw1DepthMask;
					const uint64_t program = (uint64_t(m_program.idx) << kSortKeyDraw1ProgramShift) & kSortKeyDraw1ProgramMask;
					const uint64_t blend   = (uint64_t(m_blend      ) << kSortKeyDraw1BlendShift) & kSortKeyDraw1BlendMask;
					const uint64_t view    = (uint64_t(m_view       ) << kSortKeyViewBitShift     ) & kSortKeyViewMask;
					const uint64_t key     = view|kSortKeyDrawBit|kSortKeyDrawTypeDepth|depth|blend|program;
					return key;
				}
				break;

			case SortSequence:
				{
					const uint64_t seq     = (uint64_t(m_seq        ) << kSortKeyDraw2SeqShift    ) & kSortKeyDraw2SeqMask;
					const uint64_t program = (uint64_t(m_program.idx) << kSortKeyDraw2ProgramShift) & kSortKeyDraw2ProgramMask;
					const uint64_t blend   = (uint64_t(m_blend      ) << kSortKeyDraw2BlendShift  ) & kSortKeyDraw2BlendMask;
					const uint64_t view    = (uint64_t(m_view       ) << kSortKeyViewBitShift     ) & kSortKeyViewMask;
					const uint64_t key     = view|kSortKeyDrawBit|kSortKeyDrawTypeSequence|seq|blend|program;

					BX_ASSERT(seq == (uint64_t(m_seq) << kSortKeyDraw2SeqShift)
						, "SortKey error, sequence is truncated (m_seq: %d)."
						, m_seq
						);

					return key;
				}
				break;
			}

			BX_ASSERT(false, "You should not be here.");
			return 0;
		}

		uint64_t encodeCompute()
		{
			const uint64_t program = (uint64_t(m_program.idx) << kSortKeyComputeProgramShift) & kSortKeyComputeProgramMask;
			const uint64_t seq     = (uint64_t(m_seq        ) << kSortKeyComputeSeqShift    ) & kSortKeyComputeSeqMask;
			const uint64_t view    = (uint64_t(m_view       ) << kSortKeyViewBitShift       ) & kSortKeyViewMask;
			const uint64_t key     = program|seq|view;

			BX_ASSERT(seq == (uint64_t(m_seq) << kSortKeyComputeSeqShift)
				, "SortKey error, sequence is truncated (m_seq: %d)."
				, m_seq
				);

			return key;
		}

		/// Returns true if item is compute command.
		bool decode(uint64_t _key, ViewId _viewRemap[MAX_CONFIG_MAX_VIEWS])
		{
			m_view = _viewRemap[(_key & kSortKeyViewMask) >> kSortKeyViewBitShift];

			if (_key & kSortKeyDrawBit)
			{
				uint64_t type = _key & kSortKeyDrawTypeMask;

				if (type == kSortKeyDrawTypeDepth)
				{
					m_program.idx = uint16_t( (_key & kSortKeyDraw1ProgramMask) >> kSortKeyDraw1ProgramShift);
					return false;
				}

				if (type == kSortKeyDrawTypeSequence)
				{
					m_program.idx = uint16_t( (_key & kSortKeyDraw2ProgramMask) >> kSortKeyDraw2ProgramShift);
					return false;
				}

				m_program.idx = uint16_t( (_key & kSortKeyDraw0ProgramMask) >> kSortKeyDraw0ProgramShift);
				return false; // draw
			}

			m_program.idx = uint16_t( (_key & kSortKeyComputeProgramMask) >> kSortKeyComputeProgramShift);
			return true; // compute
		}

		static ViewId decodeView(uint64_t _key)
		{
			return ViewId( (_key & kSortKeyViewMask) >> kSortKeyViewBitShift);
		}

		static uint64_t remapView(uint64_t _key, ViewId _viewRemap[MAX_CONFIG_MAX_VIEWS])
		{
			const ViewId   oldView = decodeView(_key);
			const uint64_t view    = uint64_t(_viewRemap[oldView]) << kSortKeyViewBitShift;
			const uint64_t key     = (_key & ~kSortKeyViewMask) | view;
			return key;
		}

		void reset()
		{
			m_depth   = 0;
			m_seq     = 0;
			m_program = {0};
			m_view    = 0;
			m_blend   = 0;
		}

		uint32_t      m_depth;
		uint32_t      m_seq;
		ProgramHandle m_program;
		ViewId        m_view;
		uint8_t       m_blend;
	};
#undef SORT_KEY_RENDER_DRAW

	constexpr uint8_t  kBlitKeyViewShift = 32-kSortKeyViewNumBits;
	constexpr uint32_t kBlitKeyViewMask  = uint32_t(MAX_CONFIG_MAX_VIEWS-1)<<kBlitKeyViewShift;
	constexpr uint8_t  kBlitKeyItemShift = 0;
	constexpr uint32_t kBlitKeyItemMask  = UINT16_MAX;

	struct BlitKey
	{
		uint32_t encode()
		{
			const uint32_t view = (uint32_t(m_view) << kBlitKeyViewShift) & kBlitKeyViewMask;
			const uint32_t item = (uint32_t(m_item) << kBlitKeyItemShift) & kBlitKeyItemMask;
			const uint32_t key  = view|item;

			return key;
		}

		void decode(uint32_t _key)
		{
			m_item = uint16_t( (_key & kBlitKeyItemMask) >> kBlitKeyItemShift);
			m_view =   ViewId( (_key & kBlitKeyViewMask) >> kBlitKeyViewShift);
		}

		static uint32_t remapView(uint32_t _key, ViewId _viewRemap[MAX_CONFIG_MAX_VIEWS])
		{
			const ViewId   oldView = ViewId( (_key & kBlitKeyViewMask) >> kBlitKeyViewShift);
			const uint32_t view    = uint32_t( (_viewRemap[oldView] << kBlitKeyViewShift) & kBlitKeyViewMask);
			const uint32_t key     = (_key & ~kBlitKeyViewMask) | view;
			return key;
		}

		uint16_t m_item;
		ViewId   m_view;
	};

	BX_ALIGN_DECL_16(struct) Srt
	{
		float rotate[4];
		float translate[3];
		float pad0;
		float scale[3];
		float pad1;
	};

	BX_ALIGN_DECL_16(struct) Matrix4
	{
		union
		{
			float val[16];
			bx::float4x4_t f4x4;
		} un;

		void setIdentity()
		{
			bx::memSet(un.val, 0, sizeof(un.val) );
			un.val[0] = un.val[5] = un.val[10] = un.val[15] = 1.0f;
		}
	};

	struct MatrixCache
	{
		MatrixCache()
			: m_num(1)
		{
			m_cache[0].setIdentity();
		}

		void reset()
		{
			m_num = 1;
		}

		uint32_t reserve(uint16_t* _num)
		{
			uint32_t num = *_num;
			uint32_t first = bx::atomicFetchAndAddsat<uint32_t>(&m_num, num, MAX_CONFIG_MAX_MATRIX_CACHE - 1);
			BX_WARN(first+num < MAX_CONFIG_MAX_MATRIX_CACHE, "Matrix cache overflow. %d (max: %d)", first+num, MAX_CONFIG_MAX_MATRIX_CACHE);
			num = bx::min(num, MAX_CONFIG_MAX_MATRIX_CACHE-1-first);
			*_num = bx::narrowCast<uint16_t>(num);
			return first;
		}

		uint32_t add(const void* _mtx, uint16_t _num)
		{
			if (NULL != _mtx)
			{
				uint32_t first = reserve(&_num);
				bx::memCopy(&m_cache[first], _mtx, sizeof(Matrix4)*_num);
				return first;
			}

			return 0;
		}

		float* toPtr(uint32_t _cacheIdx)
		{
			BX_ASSERT(_cacheIdx < MAX_CONFIG_MAX_MATRIX_CACHE, "Matrix cache out of bounds index %d (max: %d)"
				, _cacheIdx
				, MAX_CONFIG_MAX_MATRIX_CACHE
				);
			return m_cache[_cacheIdx].un.val;
		}

		uint32_t fromPtr(const void* _ptr) const
		{
			return uint32_t( (const Matrix4*)_ptr - m_cache);
		}

		Matrix4 m_cache[MAX_CONFIG_MAX_MATRIX_CACHE];
		uint32_t m_num;
	};

	struct RectCache
	{
		RectCache()
			: m_num(0)
		{
		}

		void reset()
		{
			m_num = 0;
		}

		uint32_t add(uint16_t _x, uint16_t _y, uint16_t _width, uint16_t _height)
		{
			const uint32_t first = bx::atomicFetchAndAddsat<uint32_t>(&m_num, 1, MAX_CONFIG_MAX_RECT_CACHE-1);
			BX_ASSERT(first+1 < MAX_CONFIG_MAX_RECT_CACHE, "Rect cache overflow. %d (max: %d)", first, MAX_CONFIG_MAX_RECT_CACHE);

			Rect& rect = m_cache[first];

			rect.m_x = _x;
			rect.m_y = _y;
			rect.m_width = _width;
			rect.m_height = _height;

			return first;
		}

		Rect     m_cache[MAX_CONFIG_MAX_RECT_CACHE];
		uint32_t m_num;
	};

	constexpr uint8_t  kConstantOpcodeTypeShift = 27;
	constexpr uint32_t kConstantOpcodeTypeMask  = UINT32_C(0xf8000000);
	constexpr uint8_t  kConstantOpcodeLocShift  = 11;
	constexpr uint32_t kConstantOpcodeLocMask   = UINT32_C(0x07fff800);
	constexpr uint8_t  kConstantOpcodeNumShift  = 1;
	constexpr uint32_t kConstantOpcodeNumMask   = UINT32_C(0x000007fe);
	constexpr uint8_t  kConstantOpcodeCopyShift = 0;
	constexpr uint32_t kConstantOpcodeCopyMask  = UINT32_C(0x00000001);

	constexpr uint8_t kUniformFragmentBit  = 0x10;
	constexpr uint8_t kUniformSamplerBit   = 0x20;
	constexpr uint8_t kUniformReadOnlyBit  = 0x40;
	constexpr uint8_t kUniformCompareBit   = 0x80;
	constexpr uint8_t kUniformMask = 0
		| kUniformFragmentBit
		| kUniformSamplerBit
		| kUniformReadOnlyBit
		| kUniformCompareBit
		;

	class UniformBuffer
	{
	public:
		static UniformBuffer* create(uint32_t _size = 1<<20)
		{
			const uint32_t structSize = sizeof(UniformBuffer)-sizeof(UniformBuffer::m_buffer);

			uint32_t size = bx::alignUp(_size, 16);
			void*    data = bx::alloc(g_allocator, size+structSize);
			return BX_PLACEMENT_NEW(data, UniformBuffer)(size);
		}

		static void destroy(UniformBuffer* _uniformBuffer)
		{
			_uniformBuffer->~UniformBuffer();
			bx::free(g_allocator, _uniformBuffer);
		}

		static void update(UniformBuffer** _uniformBuffer, uint32_t _threshold = 64<<10, uint32_t _grow = 1<<20)
		{
			UniformBuffer* uniformBuffer = *_uniformBuffer;
			if (_threshold >= uniformBuffer->m_size - uniformBuffer->m_pos)
			{
				const uint32_t structSize = sizeof(UniformBuffer)-sizeof(UniformBuffer::m_buffer);
				uint32_t size = bx::alignUp(uniformBuffer->m_size + _grow, 16);
				void*    data = bx::realloc(g_allocator, uniformBuffer, size+structSize);
				uniformBuffer = reinterpret_cast<UniformBuffer*>(data);
				uniformBuffer->m_size = size;

				*_uniformBuffer = uniformBuffer;
			}
		}

		static uint32_t encodeOpcode(UniformType::Enum _type, uint16_t _loc, uint16_t _num, uint16_t _copy)
		{
			const uint32_t type = _type << kConstantOpcodeTypeShift;
			const uint32_t loc  = _loc  << kConstantOpcodeLocShift;
			const uint32_t num  = _num  << kConstantOpcodeNumShift;
			const uint32_t copy = _copy << kConstantOpcodeCopyShift;
			return type|loc|num|copy;
		}

		static void decodeOpcode(uint32_t _opcode, UniformType::Enum& _type, uint16_t& _loc, uint16_t& _num, uint16_t& _copy)
		{
			const uint32_t type = (_opcode&kConstantOpcodeTypeMask) >> kConstantOpcodeTypeShift;
			const uint32_t loc  = (_opcode&kConstantOpcodeLocMask ) >> kConstantOpcodeLocShift;
			const uint32_t num  = (_opcode&kConstantOpcodeNumMask ) >> kConstantOpcodeNumShift;
			const uint32_t copy = (_opcode&kConstantOpcodeCopyMask); // >> kConstantOpcodeCopyShift;

			_type = (UniformType::Enum)(type);
			_copy = (uint16_t)copy;
			_num  = (uint16_t)num;
			_loc  = (uint16_t)loc;
		}

		void write(const void* _data, uint32_t _size)
		{
			BX_ASSERT(m_pos + _size < m_size, "Write would go out of bounds. pos %d + size %d > max size: %d).", m_pos, _size, m_size);

			if (m_pos + _size < m_size)
			{
				bx::memCopy(&m_buffer[m_pos], _data, _size);
				m_pos += _size;
			}
		}

		void write(uint32_t _value)
		{
			write(&_value, sizeof(uint32_t) );
		}

		const char* read(uint32_t _size)
		{
			BX_ASSERT(m_pos < m_size, "Out of bounds %d (size: %d).", m_pos, m_size);
			const char* result = &m_buffer[m_pos];
			m_pos += _size;
			return result;
		}

		uint32_t read()
		{
			uint32_t result;
			bx::memCopy(&result, read(sizeof(uint32_t) ), sizeof(uint32_t) );
			return result;
		}

		bool isEmpty() const
		{
			return 0 == m_pos;
		}

		uint32_t getPos() const
		{
			return m_pos;
		}

		void reset(uint32_t _pos = 0)
		{
			m_pos = _pos;
		}

		void finish()
		{
			write(UniformType::End);
			m_pos = 0;
		}

		void writeUniform(UniformType::Enum _type, uint16_t _loc, const void* _value, uint16_t _num = 1);
		void writeUniformHandle(UniformType::Enum _type, uint16_t _loc, UniformHandle _handle, uint16_t _num = 1);
		void writeMarker(const bx::StringView& _name);

	private:
		UniformBuffer(uint32_t _size)
			: m_size(_size)
			, m_pos(0)
		{
			finish();
		}

		~UniformBuffer()
		{
		}

		uint32_t m_size;
		uint32_t m_pos;
		char     m_buffer[256<<20];
	};

	struct UniformRegInfo
	{
		UniformHandle m_handle;
	};

	class UniformRegistry
	{
	public:
		UniformRegistry()
		{
		}

		~UniformRegistry()
		{
		}

		const UniformRegInfo* find(const char* _name) const
		{
			uint16_t handle = m_uniforms.find(bx::hash<bx::HashMurmur2A>(_name) );
			if (kInvalidHandle != handle)
			{
				return &m_info[handle];
			}

			return NULL;
		}

		const UniformRegInfo& add(UniformHandle _handle, const char* _name)
		{
			BX_ASSERT(isValid(_handle), "Uniform handle is invalid (name: %s)!", _name);
			const uint32_t key = bx::hash<bx::HashMurmur2A>(_name);
			m_uniforms.removeByKey(key);
			m_uniforms.insert(key, _handle.idx);

			UniformRegInfo& info = m_info[_handle.idx];
			info.m_handle = _handle;

			return info;
		}

		void remove(UniformHandle _handle)
		{
			m_uniforms.removeByHandle(_handle.idx);
		}

	private:
		typedef bx::HandleHashMapT<MAX_CONFIG_MAX_UNIFORMS*2> UniformHashMap;
		UniformHashMap m_uniforms;
		UniformRegInfo m_info[MAX_CONFIG_MAX_UNIFORMS];
	};

	struct Binding
	{
		enum Enum
		{
			Image,
			IndexBuffer,
			VertexBuffer,
			Texture,

			Count
		};

		uint32_t m_samplerFlags;
		uint16_t m_idx;
		uint8_t  m_type;
		uint8_t  m_format;
		uint8_t  m_access;
		uint8_t  m_mip;
	};

	struct Stream
	{
		void clear()
		{
			m_startVertex      = 0;
			m_handle.idx       = kInvalidHandle;
			m_layoutHandle.idx = kInvalidHandle;
		}

		uint32_t           m_startVertex;
		VertexBufferHandle m_handle;
		VertexLayoutHandle m_layoutHandle;
	};

	BX_ALIGN_DECL_CACHE_LINE(struct) RenderBind
	{
		void clear(uint8_t _flags = MAX_DISCARD_ALL)
		{
			if (0 != (_flags & MAX_DISCARD_BINDINGS) )
			{
				for (uint32_t ii = 0; ii < MAX_CONFIG_MAX_TEXTURE_SAMPLERS; ++ii)
				{
					Binding& bind = m_bind[ii];
					bind.m_idx = kInvalidHandle;
					bind.m_type = 0;
					bind.m_samplerFlags = 0;
					bind.m_format = 0;
					bind.m_access = 0;
					bind.m_mip = 0;
				}
			}
		};

		Binding m_bind[MAX_CONFIG_MAX_TEXTURE_SAMPLERS];
	};

	BX_ALIGN_DECL_CACHE_LINE(struct) RenderDraw
	{
		void clear(uint8_t _flags = MAX_DISCARD_ALL)
		{
			if (0 != (_flags & MAX_DISCARD_STATE) )
			{
				m_uniformBegin  = 0;
				m_uniformEnd    = 0;
				m_uniformIdx    = UINT8_MAX;

				m_stateFlags    = MAX_STATE_DEFAULT;
				m_stencil       = packStencil(MAX_STENCIL_DEFAULT, MAX_STENCIL_DEFAULT);
				m_rgba          = 0;
				m_scissor       = UINT16_MAX;
			}

			if (0 != (_flags & MAX_DISCARD_TRANSFORM) )
			{
				m_startMatrix = 0;
				m_numMatrices = 1;
			}

			if (0 != (_flags & MAX_DISCARD_INSTANCE_DATA) )
			{
				m_instanceDataOffset = 0;
				m_instanceDataStride = 0;
				m_numInstances       = 1;
				m_instanceDataBuffer.idx = kInvalidHandle;
			}

			if (0 != (_flags & MAX_DISCARD_VERTEX_STREAMS) )
			{
				m_numVertices = UINT32_MAX;
				m_streamMask  = 0;
				m_stream[0].clear();
			}

			if (0 != (_flags & MAX_DISCARD_INDEX_BUFFER) )
			{
				m_startIndex      = 0;
				m_numIndices      = UINT32_MAX;
				m_indexBuffer.idx = kInvalidHandle;
				m_submitFlags     = 0;
			}
			else
			{
				m_submitFlags = isIndex16() ? 0 : MAX_SUBMIT_INTERNAL_INDEX32;
			}

			m_startIndirect    = 0;
			m_numIndirect      = UINT32_MAX;
			m_numIndirectIndex = 0;
			m_indirectBuffer.idx    = kInvalidHandle;
			m_numIndirectBuffer.idx = kInvalidHandle;
			m_occlusionQuery.idx    = kInvalidHandle;
		}

		bool setStreamBit(uint8_t _stream, VertexBufferHandle _handle)
		{
			const uint8_t bit  = 1<<_stream;
			const uint8_t mask = m_streamMask & ~bit;
			const uint8_t tmp  = isValid(_handle) ? bit : 0;
			m_streamMask = mask | tmp;
			return 0 != tmp;
		}

		bool isIndex16() const
		{
			return 0 == (m_submitFlags & MAX_SUBMIT_INTERNAL_INDEX32);
		}

		Stream   m_stream[MAX_CONFIG_MAX_VERTEX_STREAMS];
		uint64_t m_stateFlags;
		uint64_t m_stencil;
		uint32_t m_rgba;
		uint32_t m_uniformBegin;
		uint32_t m_uniformEnd;
		uint32_t m_startMatrix;
		uint32_t m_startIndex;
		uint32_t m_numIndices;
		uint32_t m_numVertices;
		uint32_t m_instanceDataOffset;
		uint32_t m_numInstances;
		uint32_t m_startIndirect;
		uint32_t m_numIndirect;
		uint32_t m_numIndirectIndex;
		uint16_t m_instanceDataStride;
		uint16_t m_numMatrices;
		uint16_t m_scissor;
		uint8_t  m_submitFlags;
		uint8_t  m_streamMask;
		uint8_t  m_uniformIdx;

		IndexBufferHandle    m_indexBuffer;
		VertexBufferHandle   m_instanceDataBuffer;
		IndirectBufferHandle m_indirectBuffer;
		IndexBufferHandle    m_numIndirectBuffer;
		OcclusionQueryHandle m_occlusionQuery;
	};

	BX_ALIGN_DECL_CACHE_LINE(struct) RenderCompute
	{
		void clear(uint8_t _flags)
		{
			if (0 != (_flags & MAX_DISCARD_STATE) )
			{
				m_uniformBegin = 0;
				m_uniformEnd   = 0;
				m_uniformIdx   = UINT8_MAX;
			}

			if (0 != (_flags & MAX_DISCARD_TRANSFORM) )
			{
				m_startMatrix = 0;
				m_numMatrices = 0;
			}

			m_numX               = 0;
			m_numY               = 0;
			m_numZ               = 0;
			m_submitFlags        = 0;
			m_indirectBuffer.idx = kInvalidHandle;
			m_startIndirect      = 0;
			m_numIndirect        = UINT32_MAX;
		}

		uint32_t m_uniformBegin;
		uint32_t m_uniformEnd;
		uint32_t m_startMatrix;
		IndirectBufferHandle m_indirectBuffer;

		uint32_t m_numX;
		uint32_t m_numY;
		uint32_t m_numZ;
		uint32_t m_startIndirect;
		uint32_t m_numIndirect;
		uint16_t m_numMatrices;
		uint8_t  m_submitFlags;
		uint8_t  m_uniformIdx;
	};

	union RenderItem
	{
		RenderDraw    draw;
		RenderCompute compute;
	};

	BX_ALIGN_DECL_CACHE_LINE(struct) BlitItem
	{
		uint16_t m_srcX;
		uint16_t m_srcY;
		uint16_t m_srcZ;
		uint16_t m_dstX;
		uint16_t m_dstY;
		uint16_t m_dstZ;
		uint16_t m_width;
		uint16_t m_height;
		uint16_t m_depth;
		uint8_t  m_srcMip;
		uint8_t  m_dstMip;
		Handle m_src;
		Handle m_dst;
	};

	struct IndexBuffer
	{
		String   m_name;
		uint32_t m_size;
		uint16_t m_flags;
	};

	struct VertexBuffer
	{
		String   m_name;
		uint32_t m_size;
		uint16_t m_stride;
	};

	struct DynamicIndexBuffer
	{
		void reset()
		{
			m_handle     = MAX_INVALID_HANDLE;
			m_offset     = 0;
			m_size       = 0;
			m_startIndex = 0;
			m_flags      = 0;
		}

		IndexBufferHandle m_handle;
		uint32_t m_offset;
		uint32_t m_size;
		uint32_t m_startIndex;
		uint16_t m_flags;
	};

	struct DynamicVertexBuffer
	{
		void reset()
		{
			m_handle       = MAX_INVALID_HANDLE;
			m_offset       = 0;
			m_size         = 0;
			m_startVertex  = 0;
			m_numVertices  = 0;
			m_stride       = 0;
			m_layoutHandle = MAX_INVALID_HANDLE;
			m_flags        = 0;
		}

		VertexBufferHandle m_handle;
		uint32_t m_offset;
		uint32_t m_size;
		uint32_t m_startVertex;
		uint32_t m_numVertices;
		uint16_t m_stride;
		VertexLayoutHandle m_layoutHandle;
		uint16_t m_flags;
	};

	struct ShaderRef
	{
		UniformHandle* m_uniforms;
		String   m_name;
		uint32_t m_hashIn;
		uint32_t m_hashOut;
		uint16_t m_num;
		int16_t  m_refCount;
	};

	struct ProgramRef
	{
		ShaderHandle m_vsh;
		ShaderHandle m_fsh;
		int16_t      m_refCount;
	};

	struct UniformRef
	{
		String            m_name;
		UniformType::Enum m_type;
		uint16_t          m_num;
		int16_t           m_refCount;
	};

	struct TextureRef
	{
		void init(
			  BackbufferRatio::Enum _ratio
			, uint16_t _width
			, uint16_t _height
			, uint16_t _depth
			, TextureFormat::Enum _format
			, uint32_t _storageSize
			, uint8_t _numMips
			, uint16_t _numLayers
			, bool _ptrPending
			, bool _immutable
			, bool _cubeMap
			, uint64_t _flags
			)
		{
			m_ptr         = _ptrPending ? (void*)UINTPTR_MAX : NULL;
			m_storageSize = _storageSize;
			m_refCount    = 1;
			m_bbRatio     = uint8_t(_ratio);
			m_width       = _width;
			m_height      = _height;
			m_depth       = _depth;
			m_format      = uint8_t(_format);
			m_numSamples  = 1 << bx::uint32_satsub( (_flags & MAX_TEXTURE_RT_MSAA_MASK) >> MAX_TEXTURE_RT_MSAA_SHIFT, 1);
			m_numMips     = _numMips;
			m_numLayers   = _numLayers;
			m_owned       = false;
			m_immutable   = _immutable;
			m_cubeMap     = _cubeMap;
			m_flags       = _flags;
		}

		bool isRt() const
		{
			return 0 != (m_flags & MAX_TEXTURE_RT_MASK);
		}

		bool isReadBack() const
		{
			return 0 != (m_flags & MAX_TEXTURE_READ_BACK);
		}

		bool isBlitDst() const
		{
			return 0 != (m_flags & MAX_TEXTURE_BLIT_DST);
		}

		bool isCubeMap() const
		{
			return m_cubeMap;
		}

		bool is3D() const
		{
			return 0 < m_depth;
		}

		String   m_name;
		void*    m_ptr;
		uint64_t m_flags;
		uint32_t m_storageSize;
		int16_t  m_refCount;
		uint8_t  m_bbRatio;
		uint16_t m_width;
		uint16_t m_height;
		uint16_t m_depth;
		uint8_t  m_format;
		uint8_t  m_numSamples;
		uint8_t  m_numMips;
		uint16_t m_numLayers;
		bool     m_owned;
		bool     m_immutable;
		bool     m_cubeMap;
	};

	struct FrameBufferRef
	{
		String m_name;
		uint16_t m_width;
		uint16_t m_height;

		union un
		{
			TextureHandle m_th[MAX_CONFIG_MAX_FRAME_BUFFER_ATTACHMENTS];
			void* m_nwh;
		} un;

		bool m_window;
	};

	struct Primitive
	{
		uint32_t m_startIndex;
		uint32_t m_numIndices;
		uint32_t m_startVertex;
		uint32_t m_numVertices;

		bx::Sphere m_sphere;
		bx::Aabb   m_aabb;
		bx::Obb    m_obb;
	};
	typedef stl::vector<Primitive> PrimitiveArray;

	struct Group
	{
		Group()
		{
			reset();
		}

		void reset()
		{
			m_vbh.idx = kInvalidHandle;
			m_ibh.idx = kInvalidHandle;
			m_numVertices = 0;
			m_vertices = NULL;
			m_numIndices = 0;
			m_indices = NULL;
			m_prims.clear();
		}

		VertexBufferHandle m_vbh;
		IndexBufferHandle m_ibh;
		uint32_t m_numVertices;
		uint8_t* m_vertices;
		uint32_t m_numIndices;
		uint32_t* m_indices;
		bx::Sphere m_sphere;
		bx::Aabb   m_aabb;
		bx::Obb    m_obb;
		PrimitiveArray m_prims;
	};
	typedef stl::vector<Group> GroupArray;

	struct MeshRef
	{
		const Memory* m_data;
		VertexLayout  m_layout;
		GroupArray	  m_groups;
		uint32_t	  m_refCount;
	};

	struct EntityRef
	{
		bx::HandleHashMapT<MAX_CONFIG_MAX_COMPONENTS_PER_ENTITY * 2> m_components;

		bool m_destroyComponents;
		uint16_t m_refCount;
	};

	struct ComponentRef
	{
		void* m_data;
		uint32_t m_size;
		uint16_t m_refCount;
	};

	BX_ALIGN_DECL_CACHE_LINE(struct) View
	{
		void reset()
		{
			setRect(0, 0, 1, 1);
			setScissor(0, 0, 0, 0);
			setClear(MAX_CLEAR_NONE, 0, 0.0f, 0);
			setMode(ViewMode::Default);
			setFrameBuffer(MAX_INVALID_HANDLE);
			setTransform(NULL, NULL);
		}

		void setRect(uint16_t _x, uint16_t _y, uint16_t _width, uint16_t _height)
		{
			m_rect.m_x      = uint16_t(bx::max<int16_t>(int16_t(_x), 0) );
			m_rect.m_y      = uint16_t(bx::max<int16_t>(int16_t(_y), 0) );
			m_rect.m_width  = bx::max<uint16_t>(_width,  1);
			m_rect.m_height = bx::max<uint16_t>(_height, 1);
		}

		void setScissor(uint16_t _x, uint16_t _y, uint16_t _width, uint16_t _height)
		{
			m_scissor.m_x = _x;
			m_scissor.m_y = _y;
			m_scissor.m_width  = _width;
			m_scissor.m_height = _height;
		}

		void setClear(uint16_t _flags, uint32_t _rgba, float _depth, uint8_t _stencil)
		{
			m_clear.set(_flags, _rgba, _depth, _stencil);
		}

		void setClear(uint16_t _flags, float _depth, uint8_t _stencil, uint8_t _0, uint8_t _1, uint8_t _2, uint8_t _3, uint8_t _4, uint8_t _5, uint8_t _6, uint8_t _7)
		{
			m_clear.set(_flags, _depth, _stencil, _0, _1, _2, _3, _4, _5, _6, _7);
		}

		void setMode(ViewMode::Enum _mode)
		{
			m_mode = uint8_t(_mode);
		}

		void setFrameBuffer(FrameBufferHandle _handle)
		{
			m_fbh = _handle;
		}

		void setTransform(const void* _view, const void* _proj)
		{
			if (NULL != _view)
			{
				bx::memCopy(m_view.un.val, _view, sizeof(Matrix4) );
			}
			else
			{
				m_view.setIdentity();
			}

			if (NULL != _proj)
			{
				bx::memCopy(m_proj.un.val, _proj, sizeof(Matrix4) );
			}
			else
			{
				m_proj.setIdentity();
			}
		}

		Clear   m_clear;
		Rect    m_rect;
		Rect    m_scissor;
		Matrix4 m_view;
		Matrix4 m_proj;
		FrameBufferHandle m_fbh;
		uint8_t m_mode;
	};

	struct FrameCache
	{
		void reset()
		{
			m_matrixCache.reset();
			m_rectCache.reset();
		}

		bool isZeroArea(const Rect& _rect, uint16_t _scissor) const
		{
			if (UINT16_MAX != _scissor)
			{
				Rect scissorRect;
				scissorRect.setIntersect(_rect, m_rectCache.m_cache[_scissor]);
				return scissorRect.isZeroArea();
			}

			return false;
		}

		MatrixCache m_matrixCache;
		RectCache m_rectCache;
	};

	struct ScreenShot
	{
		bx::FilePath filePath;
		FrameBufferHandle handle;
	};

	BX_ALIGN_DECL_CACHE_LINE(struct) Frame
	{
		Frame()
			: m_waitSubmit(0)
			, m_waitRender(0)
			, m_frameNum(0)
			, m_capture(false)
		{
			SortKey term;
			term.reset();
			term.m_program = MAX_INVALID_HANDLE;
			m_sortKeys[MAX_CONFIG_MAX_DRAW_CALLS]   = term.encodeDraw(SortKey::SortProgram);
			m_sortValues[MAX_CONFIG_MAX_DRAW_CALLS] = MAX_CONFIG_MAX_DRAW_CALLS;
			bx::memSet(m_occlusion, 0xff, sizeof(m_occlusion) );

			m_perfStats.viewStats = m_viewStats;

			bx::memSet(&m_renderItemBind[0], 0, sizeof(m_renderItemBind));
		}

		~Frame()
		{
		}

		void create(uint32_t _minResourceCbSize)
		{
			m_cmdPre.init(_minResourceCbSize);
			m_cmdPost.init(_minResourceCbSize);

			{
				const uint32_t num = g_caps.limits.maxEncoders;

				m_uniformBuffer = (UniformBuffer**)bx::alloc(g_allocator, sizeof(UniformBuffer*)*num);

				for (uint32_t ii = 0; ii < num; ++ii)
				{
					m_uniformBuffer[ii] = UniformBuffer::create();
				}
			}

			reset();
			start(0);
			m_textVideoMem = BX_NEW(g_allocator, TextVideoMem);
		}

		void destroy()
		{
			for (uint32_t ii = 0, num = g_caps.limits.maxEncoders; ii < num; ++ii)
			{
				UniformBuffer::destroy(m_uniformBuffer[ii]);
			}

			bx::free(g_allocator, m_uniformBuffer);
			bx::deleteObject(g_allocator, m_textVideoMem);
		}

		void reset()
		{
			start(0);
			finish();
			resetFreeHandles();
		}

		void start(uint32_t frameNum)
		{
			m_perfStats.transientVbUsed = m_vboffset;
			m_perfStats.transientIbUsed = m_iboffset;

			m_frameCache.reset();
			m_numRenderItems = 0;
			m_numBlitItems   = 0;
			m_iboffset = 0;
			m_vboffset = 0;
			m_cmdPre.start();
			m_cmdPost.start();
			m_capture = false;
			m_numScreenShots = 0;
			m_frameNum = frameNum;
		}

		void finish()
		{
			m_cmdPre.finish();
			m_cmdPost.finish();
		}

		void sort();

		uint32_t getAvailTransientIndexBuffer(uint32_t _num, uint16_t _indexSize)
		{
			const uint32_t offset = bx::strideAlign(m_iboffset, _indexSize);
			uint32_t iboffset = offset + _num*_indexSize;
			iboffset = bx::min<uint32_t>(iboffset, g_caps.limits.transientIbSize);
			const uint32_t num = (iboffset-offset)/_indexSize;
			return num;
		}

		uint32_t allocTransientIndexBuffer(uint32_t& _num, uint16_t _indexSize)
		{
			uint32_t offset = bx::strideAlign(m_iboffset, _indexSize);
			uint32_t num    = getAvailTransientIndexBuffer(_num, _indexSize);
			m_iboffset = offset + num*_indexSize;
			_num = num;

			return offset;
		}

		uint32_t getAvailTransientVertexBuffer(uint32_t _num, uint16_t _stride)
		{
			uint32_t offset   = bx::strideAlign(m_vboffset, _stride);
			uint32_t vboffset = offset + _num * _stride;
			vboffset = bx::min<uint32_t>(vboffset, g_caps.limits.transientVbSize);
			uint32_t num = (vboffset-offset)/_stride;
			return num;
		}

		uint32_t allocTransientVertexBuffer(uint32_t& _num, uint16_t _stride)
		{
			uint32_t offset = bx::strideAlign(m_vboffset, _stride);
			uint32_t num    = getAvailTransientVertexBuffer(_num, _stride);
			m_vboffset = offset + num * _stride;
			_num = num;

			return offset;
		}

		bool free(IndexBufferHandle _handle)
		{
			return m_freeIndexBuffer.queue(_handle);
		}

		bool free(VertexLayoutHandle _handle)
		{
			return m_freeVertexLayout.queue(_handle);
		}

		bool free(VertexBufferHandle _handle)
		{
			return m_freeVertexBuffer.queue(_handle);
		}

		bool free(ShaderHandle _handle)
		{
			return m_freeShader.queue(_handle);
		}

		bool free(ProgramHandle _handle)
		{
			return m_freeProgram.queue(_handle);
		}

		bool free(TextureHandle _handle)
		{
			return m_freeTexture.queue(_handle);
		}

		bool free(FrameBufferHandle _handle)
		{
			return m_freeFrameBuffer.queue(_handle);
		}

		bool free(UniformHandle _handle)
		{
			return m_freeUniform.queue(_handle);
		}

		bool free(MeshHandle _handle)
		{
			return m_freeMesh.queue(_handle);
		}

		bool free(ComponentHandle _handle)
		{
			return m_freeComponent.queue(_handle);
		}

		bool free(EntityHandle _handle)
		{
			return m_freeEntity.queue(_handle);
		}

		void resetFreeHandles()
		{
			m_freeIndexBuffer.reset();
			m_freeVertexLayout.reset();
			m_freeVertexBuffer.reset();
			m_freeShader.reset();
			m_freeProgram.reset();
			m_freeTexture.reset();
			m_freeFrameBuffer.reset();
			m_freeUniform.reset();
			m_freeMesh.reset();
			m_freeComponent.reset();
			m_freeEntity.reset();
		}

		ViewId m_viewRemap[MAX_CONFIG_MAX_VIEWS];
		float m_colorPalette[MAX_CONFIG_MAX_COLOR_PALETTE][4];

		View m_view[MAX_CONFIG_MAX_VIEWS];

		int32_t m_occlusion[MAX_CONFIG_MAX_OCCLUSION_QUERIES];

		uint64_t m_sortKeys[MAX_CONFIG_MAX_DRAW_CALLS+1];
		RenderItemCount m_sortValues[MAX_CONFIG_MAX_DRAW_CALLS+1];
		RenderItem m_renderItem[MAX_CONFIG_MAX_DRAW_CALLS+1];
		RenderBind m_renderItemBind[MAX_CONFIG_MAX_DRAW_CALLS + 1];

		uint32_t m_blitKeys[MAX_CONFIG_MAX_BLIT_ITEMS+1];
		BlitItem m_blitItem[MAX_CONFIG_MAX_BLIT_ITEMS+1];

		FrameCache m_frameCache;
		UniformBuffer** m_uniformBuffer;

		uint32_t m_numRenderItems;
		uint16_t m_numBlitItems;

		uint32_t m_iboffset;
		uint32_t m_vboffset;
		TransientIndexBuffer* m_transientIb;
		TransientVertexBuffer* m_transientVb;

		Resolution m_resolution;
		uint32_t m_debug;

		ScreenShot m_screenShot[MAX_CONFIG_MAX_SCREENSHOTS];
		uint8_t m_numScreenShots;

		CommandBuffer m_cmdPre;
		CommandBuffer m_cmdPost;

		template<typename Ty, uint32_t Max>
		struct FreeHandle
		{
			FreeHandle()
				: m_num(0)
			{
			}

			bool isQueued(Ty _handle)
			{
				for (uint32_t ii = 0, num = m_num; ii < num; ++ii)
				{
					if (m_queue[ii].idx == _handle.idx)
					{
						return true;
					}
				}

				return false;
			}

			bool queue(Ty _handle)
			{
				if (BX_ENABLED(MAX_CONFIG_DEBUG) )
				{
					if (isQueued(_handle) )
					{
						return false;
					}
				}

				m_queue[m_num] = _handle;
				++m_num;

				return true;
			}

			void reset()
			{
				m_num = 0;
			}

			Ty get(uint16_t _idx) const
			{
				return m_queue[_idx];
			}

			uint16_t getNumQueued() const
			{
				return m_num;
			}

			Ty m_queue[Max];
			uint16_t m_num;
		};

		FreeHandle<IndexBufferHandle,  MAX_CONFIG_MAX_INDEX_BUFFERS>  m_freeIndexBuffer;
		FreeHandle<VertexLayoutHandle, MAX_CONFIG_MAX_VERTEX_LAYOUTS> m_freeVertexLayout;
		FreeHandle<VertexBufferHandle, MAX_CONFIG_MAX_VERTEX_BUFFERS> m_freeVertexBuffer;
		FreeHandle<ShaderHandle,       MAX_CONFIG_MAX_SHADERS>        m_freeShader;
		FreeHandle<ProgramHandle,      MAX_CONFIG_MAX_PROGRAMS>       m_freeProgram;
		FreeHandle<TextureHandle,      MAX_CONFIG_MAX_TEXTURES>       m_freeTexture;
		FreeHandle<FrameBufferHandle,  MAX_CONFIG_MAX_FRAME_BUFFERS>  m_freeFrameBuffer;
		FreeHandle<UniformHandle,      MAX_CONFIG_MAX_UNIFORMS>       m_freeUniform;
		FreeHandle<MeshHandle,		   MAX_CONFIG_MAX_MESHES>         m_freeMesh;
		FreeHandle<ComponentHandle,	   MAX_CONFIG_MAX_COMPONENTS>     m_freeComponent;
		FreeHandle<EntityHandle,	   MAX_CONFIG_MAX_ENTITIES>       m_freeEntity;

		TextVideoMem* m_textVideoMem;

		Stats     m_perfStats;
		ViewStats m_viewStats[MAX_CONFIG_MAX_VIEWS];

		int64_t m_waitSubmit;
		int64_t m_waitRender;

		uint32_t m_frameNum;

		bool m_capture;
	};

	BX_ALIGN_DECL_CACHE_LINE(struct) EncoderImpl
	{
		EncoderImpl()
		{
			// Although it will be cleared by the discard(), the fact that the
			// struct is padded to have a size equal to the cache line size,
			// will leaves bytes uninitialized. This will influence the hashing
			// as it reads those bytes too. To make this deterministic, we will
			// clear all bytes (inclusively the padding) before we start.
			bx::memSet(&m_bind, 0, sizeof(m_bind));

			discard(MAX_DISCARD_ALL);
		}

		void begin(Frame* _frame, uint8_t _idx)
		{
			m_frame = _frame;

			m_cpuTimeBegin = bx::getHPCounter();

			m_uniformIdx   = _idx;
			m_uniformBegin = 0;
			m_uniformEnd   = 0;

			UniformBuffer* uniformBuffer = m_frame->m_uniformBuffer[m_uniformIdx];
			uniformBuffer->reset();

			m_numSubmitted = 0;
			m_numDropped   = 0;
		}

		void end(bool _finalize)
		{
			if (_finalize)
			{
				UniformBuffer* uniformBuffer = m_frame->m_uniformBuffer[m_uniformIdx];
				uniformBuffer->finish();

				m_cpuTimeEnd = bx::getHPCounter();
			}

			if (BX_ENABLED(MAX_CONFIG_DEBUG_OCCLUSION) )
			{
				m_occlusionQuerySet.clear();
			}

			if (BX_ENABLED(MAX_CONFIG_DEBUG_UNIFORM) )
			{
				m_uniformSet.clear();
			}
		}

		void setMarker(const bx::StringView& _name)
		{
			UniformBuffer::update(&m_frame->m_uniformBuffer[m_uniformIdx]);
			UniformBuffer* uniformBuffer = m_frame->m_uniformBuffer[m_uniformIdx];
			uniformBuffer->writeMarker(_name);
		}

		void setUniform(UniformType::Enum _type, UniformHandle _handle, const void* _value, uint16_t _num)
		{
			if (BX_ENABLED(MAX_CONFIG_DEBUG_UNIFORM) )
			{
				BX_ASSERT(m_uniformSet.end() == m_uniformSet.find(_handle.idx)
					, "Uniform %d (%s) was already set for this draw call."
					, _handle.idx
					, getName(_handle)
					);
//				m_uniformSet.insert(_handle.idx);
			}

			UniformBuffer::update(&m_frame->m_uniformBuffer[m_uniformIdx]);
			UniformBuffer* uniformBuffer = m_frame->m_uniformBuffer[m_uniformIdx];
			uniformBuffer->writeUniform(_type, _handle.idx, _value, _num);
		}

		void setState(uint64_t _state, uint32_t _rgba)
		{
			const uint8_t blend    = ( (_state&MAX_STATE_BLEND_MASK    )>>MAX_STATE_BLEND_SHIFT    )&0xff;
			const uint8_t alphaRef = ( (_state&MAX_STATE_ALPHA_REF_MASK)>>MAX_STATE_ALPHA_REF_SHIFT)&0xff;

			// Transparency sort order table:
			//
			//                    +----------------------------------------- MAX_STATE_BLEND_ZERO
			//                    |  +-------------------------------------- MAX_STATE_BLEND_ONE
			//                    |  |  +----------------------------------- MAX_STATE_BLEND_SRC_COLOR
			//                    |  |  |  +-------------------------------- MAX_STATE_BLEND_INV_SRC_COLOR
			//                    |  |  |  |  +----------------------------- MAX_STATE_BLEND_SRC_ALPHA
			//                    |  |  |  |  |  +-------------------------- MAX_STATE_BLEND_INV_SRC_ALPHA
			//                    |  |  |  |  |  |  +----------------------- MAX_STATE_BLEND_DST_ALPHA
			//                    |  |  |  |  |  |  |  +-------------------- MAX_STATE_BLEND_INV_DST_ALPHA
			//                    |  |  |  |  |  |  |  |  +----------------- MAX_STATE_BLEND_DST_COLOR
			//                    |  |  |  |  |  |  |  |  |  +-------------- MAX_STATE_BLEND_INV_DST_COLOR
			//                    |  |  |  |  |  |  |  |  |  |  +----------- MAX_STATE_BLEND_SRC_ALPHA_SAT
			//                    |  |  |  |  |  |  |  |  |  |  |  +-------- MAX_STATE_BLEND_FACTOR
			//                    |  |  |  |  |  |  |  |  |  |  |  |  +----- MAX_STATE_BLEND_INV_FACTOR
			//                    |  |  |  |  |  |  |  |  |  |  |  |  |
			//                 x  |  |  |  |  |  |  |  |  |  |  |  |  |  x  x  x  x  x
			m_key.m_blend = "\x0\x2\x2\x3\x3\x2\x3\x2\x3\x2\x2\x2\x2\x2\x2\x2\x2\x2\x2"[( (blend)&0xf) + (!!blend)] + !!alphaRef;

			m_draw.m_stateFlags = _state;
			m_draw.m_rgba       = _rgba;
		}

		void setCondition(OcclusionQueryHandle _handle, bool _visible)
		{
			m_draw.m_occlusionQuery = _handle;
			m_draw.m_submitFlags   |= _visible ? MAX_SUBMIT_INTERNAL_OCCLUSION_VISIBLE : 0;
		}

		void setStencil(uint32_t _fstencil, uint32_t _bstencil)
		{
			m_draw.m_stencil = packStencil(_fstencil, _bstencil);
		}

		uint16_t setScissor(uint16_t _x, uint16_t _y, uint16_t _width, uint16_t _height)
		{
			uint16_t scissor = bx::narrowCast<uint16_t>(m_frame->m_frameCache.m_rectCache.add(_x, _y, _width, _height) );
			m_draw.m_scissor = scissor;
			return scissor;
		}

		void setScissor(uint16_t _cache)
		{
			m_draw.m_scissor = _cache;
		}

		uint32_t setTransform(const void* _mtx, uint16_t _num)
		{
			m_draw.m_startMatrix = m_frame->m_frameCache.m_matrixCache.add(_mtx, _num);
			m_draw.m_numMatrices = _num;

			return m_draw.m_startMatrix;
		}

		uint32_t allocTransform(Transform* _transform, uint16_t _num)
		{
			uint32_t first   = m_frame->m_frameCache.m_matrixCache.reserve(&_num);
			_transform->data = m_frame->m_frameCache.m_matrixCache.toPtr(first);
			_transform->num  = _num;

			return first;
		}

		void setTransform(uint32_t _cache, uint16_t _num)
		{
			BX_ASSERT(_cache < MAX_CONFIG_MAX_MATRIX_CACHE, "Matrix cache out of bounds index %d (max: %d)"
				, _cache
				, MAX_CONFIG_MAX_MATRIX_CACHE
				);
			m_draw.m_startMatrix = _cache;
			m_draw.m_numMatrices = uint16_t(bx::min<uint32_t>(_cache+_num, MAX_CONFIG_MAX_MATRIX_CACHE-1) - _cache);
		}

		void setIndexBuffer(IndexBufferHandle _handle, const IndexBuffer& _ib, uint32_t _firstIndex, uint32_t _numIndices)
		{
			BX_ASSERT(UINT8_MAX != m_draw.m_streamMask, "max::setVertexCount was already called for this draw call.");
			m_draw.m_startIndex  = _firstIndex;
			m_draw.m_numIndices  = _numIndices;
			m_draw.m_indexBuffer = _handle;
			m_draw.m_submitFlags |= 0 == (_ib.m_flags & MAX_BUFFER_INDEX32) ? MAX_SUBMIT_INTERNAL_NONE : MAX_SUBMIT_INTERNAL_INDEX32;
		}

		void setIndexBuffer(const DynamicIndexBuffer& _dib, uint32_t _firstIndex, uint32_t _numIndices)
		{
			BX_ASSERT(UINT8_MAX != m_draw.m_streamMask, "max::setVertexCount was already called for this draw call.");
			const uint32_t indexSize = 0 == (_dib.m_flags & MAX_BUFFER_INDEX32) ? 2 : 4;
			m_draw.m_startIndex  = _dib.m_startIndex + _firstIndex;
			m_draw.m_numIndices  = bx::min(_numIndices, _dib.m_size/indexSize);
			m_draw.m_indexBuffer = _dib.m_handle;
			m_draw.m_submitFlags |= 0 == (_dib.m_flags & MAX_BUFFER_INDEX32) ? MAX_SUBMIT_INTERNAL_NONE : MAX_SUBMIT_INTERNAL_INDEX32;
		}

		void setIndexBuffer(const TransientIndexBuffer* _tib, uint32_t _firstIndex, uint32_t _numIndices)
		{
			BX_ASSERT(UINT8_MAX != m_draw.m_streamMask, "max::setVertexCount was already called for this draw call.");
			const uint32_t indexSize  = _tib->isIndex16 ? 2 : 4;
			const uint32_t numIndices = bx::min(_numIndices, _tib->size/indexSize);
			m_draw.m_indexBuffer = _tib->handle;
			m_draw.m_startIndex  = _tib->startIndex + _firstIndex;
			m_draw.m_numIndices  = numIndices;
			m_draw.m_submitFlags |= _tib->isIndex16 ? MAX_SUBMIT_INTERNAL_NONE : MAX_SUBMIT_INTERNAL_INDEX32;
			m_discard            = 0 == numIndices;
		}

		void setVertexBuffer(
			  uint8_t _stream
			, VertexBufferHandle _handle
			, uint32_t _startVertex
			, uint32_t _numVertices
			, VertexLayoutHandle _layoutHandle
			)
		{
			BX_ASSERT(UINT8_MAX != m_draw.m_streamMask, "max::setVertexCount was already called for this draw call.");
			BX_ASSERT(_stream < MAX_CONFIG_MAX_VERTEX_STREAMS, "Invalid stream %d (max %d).", _stream, MAX_CONFIG_MAX_VERTEX_STREAMS);
			if (m_draw.setStreamBit(_stream, _handle) )
			{
				Stream& stream = m_draw.m_stream[_stream];
				stream.m_startVertex   = _startVertex;
				stream.m_handle        = _handle;
				stream.m_layoutHandle  = _layoutHandle;
				m_numVertices[_stream] = _numVertices;
			}
		}

		void setVertexBuffer(
			  uint8_t _stream
			, const DynamicVertexBuffer& _dvb
			, uint32_t _startVertex
			, uint32_t _numVertices
			, VertexLayoutHandle _layoutHandle
			)
		{
			BX_ASSERT(UINT8_MAX != m_draw.m_streamMask, "max::setVertexCount was already called for this draw call.");
			BX_ASSERT(_stream < MAX_CONFIG_MAX_VERTEX_STREAMS, "Invalid stream %d (max %d).", _stream, MAX_CONFIG_MAX_VERTEX_STREAMS);
			if (m_draw.setStreamBit(_stream, _dvb.m_handle) )
			{
				Stream& stream = m_draw.m_stream[_stream];
				stream.m_startVertex   = _dvb.m_startVertex + _startVertex;
				stream.m_handle        = _dvb.m_handle;
				stream.m_layoutHandle  = isValid(_layoutHandle) ? _layoutHandle : _dvb.m_layoutHandle;
				m_numVertices[_stream] =
					bx::min(bx::uint32_imax(0, _dvb.m_numVertices - _startVertex), _numVertices)
					;
			}
		}

		void setVertexBuffer(
			  uint8_t _stream
			, const TransientVertexBuffer* _tvb
			, uint32_t _startVertex
			, uint32_t _numVertices
			, VertexLayoutHandle _layoutHandle
			)
		{
			BX_ASSERT(UINT8_MAX != m_draw.m_streamMask, "max::setVertexCount was already called for this draw call.");
			BX_ASSERT(_stream < MAX_CONFIG_MAX_VERTEX_STREAMS, "Invalid stream %d (max %d).", _stream, MAX_CONFIG_MAX_VERTEX_STREAMS);
			if (m_draw.setStreamBit(_stream, _tvb->handle) )
			{
				Stream& stream = m_draw.m_stream[_stream];
				stream.m_startVertex   = _tvb->startVertex + _startVertex;
				stream.m_handle        = _tvb->handle;
				stream.m_layoutHandle  = isValid(_layoutHandle) ? _layoutHandle : _tvb->layoutHandle;
				m_numVertices[_stream] = bx::min(bx::uint32_imax(0, _tvb->size/_tvb->stride - _startVertex), _numVertices);
			}
		}

		void setVertexCount(uint32_t _numVertices)
		{
			BX_ASSERT(0 == m_draw.m_streamMask, "Vertex buffer already set.");
			m_draw.m_streamMask  = UINT8_MAX;
			Stream& stream = m_draw.m_stream[0];
			stream.m_startVertex        = 0;
			stream.m_handle.idx         = kInvalidHandle;
			stream.m_layoutHandle.idx   = kInvalidHandle;
			m_numVertices[0]            = _numVertices;
		}

		void setInstanceDataBuffer(const InstanceDataBuffer* _idb, uint32_t _start, uint32_t _num)
		{
			const uint32_t start = bx::min(_start, _idb->num);
			const uint32_t num   = bx::min(_idb->num - start, _num);
			m_draw.m_instanceDataOffset = _idb->offset + start*_idb->stride;
			m_draw.m_instanceDataStride = _idb->stride;
			m_draw.m_numInstances       = num;
			m_draw.m_instanceDataBuffer = _idb->handle;
		}

		void setInstanceDataBuffer(VertexBufferHandle _handle, uint32_t _startVertex, uint32_t _num, uint16_t _stride)
		{
			m_draw.m_instanceDataOffset = _startVertex * _stride;
			m_draw.m_instanceDataStride = _stride;
			m_draw.m_numInstances       = _num;
			m_draw.m_instanceDataBuffer = _handle;
		}

		void setInstanceCount(uint32_t _numInstances)
		{
			BX_ASSERT(!isValid(m_draw.m_instanceDataBuffer), "Instance buffer already set.");
			m_draw.m_numInstances = _numInstances;
		}

		void setTexture(uint8_t _stage, UniformHandle _sampler, TextureHandle _handle, uint32_t _flags)
		{
			Binding& bind = m_bind.m_bind[_stage];
			bind.m_idx    = _handle.idx;
			bind.m_type   = uint8_t(Binding::Texture);
			bind.m_samplerFlags = (_flags&MAX_SAMPLER_INTERNAL_DEFAULT)
				? MAX_SAMPLER_INTERNAL_DEFAULT
				: _flags
				;
			bind.m_format = 0;
			bind.m_access = 0;
			bind.m_mip    = 0;

			if (isValid(_sampler) )
			{
				uint32_t stage = _stage;
				setUniform(UniformType::Sampler, _sampler, &stage, 1);
			}
		}

		void setBuffer(uint8_t _stage, IndexBufferHandle _handle, Access::Enum _access)
		{
			Binding& bind = m_bind.m_bind[_stage];
			bind.m_idx    = _handle.idx;
			bind.m_type   = uint8_t(Binding::IndexBuffer);
			bind.m_format = 0;
			bind.m_access = uint8_t(_access);
			bind.m_mip    = 0;
		}

		void setBuffer(uint8_t _stage, VertexBufferHandle _handle, Access::Enum _access)
		{
			Binding& bind = m_bind.m_bind[_stage];
			bind.m_idx    = _handle.idx;
			bind.m_type   = uint8_t(Binding::VertexBuffer);
			bind.m_format = 0;
			bind.m_access = uint8_t(_access);
			bind.m_mip    = 0;
		}

		void setImage(uint8_t _stage, TextureHandle _handle, uint8_t _mip, Access::Enum _access, TextureFormat::Enum _format)
		{
			Binding& bind = m_bind.m_bind[_stage];
			bind.m_idx    = _handle.idx;
			bind.m_type   = uint8_t(Binding::Image);
			bind.m_format = uint8_t(_format);
			bind.m_access = uint8_t(_access);
			bind.m_mip    = _mip;
		}

		void discard(uint8_t _flags)
		{
			if (BX_ENABLED(MAX_CONFIG_DEBUG_UNIFORM) )
			{
				m_uniformSet.clear();
			}

			m_discard = false;
			m_draw.clear(_flags);
			m_compute.clear(_flags);
			m_bind.clear(_flags);
		}

		void submit(ViewId _id, ProgramHandle _program, OcclusionQueryHandle _occlusionQuery, uint32_t _depth, uint8_t _flags);

		void submit(ViewId _id, ProgramHandle _program, IndirectBufferHandle _indirectHandle, uint32_t _start, uint32_t _num, uint32_t _depth, uint8_t _flags)
		{
			m_draw.m_startIndirect  = _start;
			m_draw.m_numIndirect    = _num;
			m_draw.m_indirectBuffer = _indirectHandle;
			OcclusionQueryHandle handle = MAX_INVALID_HANDLE;
			submit(_id, _program, handle, _depth, _flags);
		}

		void submit(ViewId _id, ProgramHandle _program, IndirectBufferHandle _indirectHandle, uint32_t _start, IndexBufferHandle _numHandle, uint32_t _numIndex, uint32_t _numMax, uint32_t _depth, uint8_t _flags)
		{
			m_draw.m_numIndirectIndex  = _numIndex;
			m_draw.m_numIndirectBuffer = _numHandle;
			submit(_id, _program, _indirectHandle, _start, _numMax, _depth, _flags);
		}

		void dispatch(ViewId _id, ProgramHandle _handle, uint32_t _ngx, uint32_t _ngy, uint32_t _ngz, uint8_t _flags);

		void dispatch(ViewId _id, ProgramHandle _handle, IndirectBufferHandle _indirectHandle, uint32_t _start, uint32_t _num, uint8_t _flags)
		{
			m_compute.m_indirectBuffer = _indirectHandle;
			m_compute.m_startIndirect  = _start;
			m_compute.m_numIndirect    = _num;
			dispatch(_id, _handle, 0, 0, 0, _flags);
		}

		void blit(ViewId _id, TextureHandle _dst, uint8_t _dstMip, uint16_t _dstX, uint16_t _dstY, uint16_t _dstZ, TextureHandle _src, uint8_t _srcMip, uint16_t _srcX, uint16_t _srcY, uint16_t _srcZ, uint16_t _width, uint16_t _height, uint16_t _depth);

		Frame* m_frame;

		SortKey m_key;

		RenderDraw    m_draw;
		RenderCompute m_compute;
		RenderBind    m_bind;

		uint32_t m_numSubmitted;
		uint32_t m_numDropped;

		uint32_t m_uniformBegin;
		uint32_t m_uniformEnd;
		uint32_t m_numVertices[MAX_CONFIG_MAX_VERTEX_STREAMS];
		uint8_t  m_uniformIdx;
		bool     m_discard;

		typedef stl::unordered_set<uint16_t> HandleSet;
		HandleSet m_uniformSet;
		HandleSet m_occlusionQuerySet;

		int64_t m_cpuTimeBegin;
		int64_t m_cpuTimeEnd;
	};

	struct DebugProgram
	{
		enum Enum
		{
			Lines,
			LinesStipple,
			Fill,
			FillMesh,
			FillLit,
			FillLitMesh,
			FillTexture,

			Count
		};
	};

	struct DebugMesh
	{
		enum Enum
		{
			Sphere0,
			Sphere1,
			Sphere2,
			Sphere3,

			Cone0,
			Cone1,
			Cone2,
			Cone3,

			Cylinder0,
			Cylinder1,
			Cylinder2,
			Cylinder3,

			Capsule0,
			Capsule1,
			Capsule2,
			Capsule3,

			Quad,

			Cube,

			Count,

			SphereMaxLod = Sphere3 - Sphere0,
			ConeMaxLod = Cone3 - Cone0,
			CylinderMaxLod = Cylinder3 - Cylinder0,
			CapsuleMaxLod = Capsule3 - Capsule0,
		};

		uint32_t m_startVertex;
		uint32_t m_numVertices;
		uint32_t m_startIndex[2];
		uint32_t m_numIndices[2];
	};

	struct DebugAttrib
	{
		uint64_t m_state;
		float    m_offset;
		float    m_scale;
		float    m_spin;
		uint32_t m_abgr;
		bool     m_stipple;
		bool     m_wireframe;
		uint8_t  m_lod;
	};

	struct DebugPosVertex
	{
		float m_x;
		float m_y;
		float m_z;
		float m_len;
		uint32_t m_abgr;

		static void init()
		{
			ms_layout
				.begin()
				.add(max::Attrib::Position, 3, max::AttribType::Float)
				.add(max::Attrib::TexCoord0, 1, max::AttribType::Float)
				.add(max::Attrib::Color0, 4, max::AttribType::Uint8, true)
				.end();
		}

		static max::VertexLayout ms_layout;
	};

	struct DebugUvVertex
	{
		float m_x;
		float m_y;
		float m_z;
		float m_u;
		float m_v;
		uint32_t m_abgr;

		static void init()
		{
			ms_layout
				.begin()
				.add(max::Attrib::Position, 3, max::AttribType::Float)
				.add(max::Attrib::TexCoord0, 2, max::AttribType::Float)
				.add(max::Attrib::Color0, 4, max::AttribType::Uint8, true)
				.end();
		}

		static max::VertexLayout ms_layout;
	};

	struct DebugShapeVertex
	{
		float m_x;
		float m_y;
		float m_z;
		uint8_t m_indices[4];

		static void init()
		{
			ms_layout
				.begin()
				.add(max::Attrib::Position, 3, max::AttribType::Float)
				.add(max::Attrib::Indices, 4, max::AttribType::Uint8)
				.end();
		}

		static max::VertexLayout ms_layout;
	};

	struct DebugMeshVertex
	{
		float m_x;
		float m_y;
		float m_z;

		static void init()
		{
			ms_layout
				.begin()
				.add(max::Attrib::Position, 3, max::AttribType::Float)
				.end();
		}

		static max::VertexLayout ms_layout;
	};

	static DebugShapeVertex s_quadVertices[4] =
	{
		{-1.0f, 0.0f,  1.0f, { 0, 0, 0, 0 } },
		{ 1.0f, 0.0f,  1.0f, { 0, 0, 0, 0 } },
		{-1.0f, 0.0f, -1.0f, { 0, 0, 0, 0 } },
		{ 1.0f, 0.0f, -1.0f, { 0, 0, 0, 0 } },

	};

	static const uint16_t s_quadIndices[6] =
	{
		0, 1, 2,
		1, 3, 2,
	};

	static DebugShapeVertex s_cubeVertices[8] =
	{
		{-1.0f,  1.0f,  1.0f, { 0, 0, 0, 0 } },
		{ 1.0f,  1.0f,  1.0f, { 0, 0, 0, 0 } },
		{-1.0f, -1.0f,  1.0f, { 0, 0, 0, 0 } },
		{ 1.0f, -1.0f,  1.0f, { 0, 0, 0, 0 } },
		{-1.0f,  1.0f, -1.0f, { 0, 0, 0, 0 } },
		{ 1.0f,  1.0f, -1.0f, { 0, 0, 0, 0 } },
		{-1.0f, -1.0f, -1.0f, { 0, 0, 0, 0 } },
		{ 1.0f, -1.0f, -1.0f, { 0, 0, 0, 0 } },
	};

	static const uint16_t s_cubeIndices[36] =
	{
		0, 1, 2, // 0
		1, 3, 2,
		4, 6, 5, // 2
		5, 6, 7,
		0, 2, 4, // 4
		4, 2, 6,
		1, 5, 3, // 6
		5, 7, 3,
		0, 4, 1, // 8
		4, 5, 1,
		2, 3, 6, // 10
		6, 3, 7,
	};

	static const uint8_t s_circleLod[] =
	{
		37,
		29,
		23,
		17,
		11,
	};

	static uint8_t getCircleLod(uint8_t _lod)
	{
		_lod = _lod > BX_COUNTOF(s_circleLod) - 1 ? BX_COUNTOF(s_circleLod) - 1 : _lod;
		return s_circleLod[_lod];
	}

	static void circle(float* _out, float _angle)
	{
		float sa = bx::sin(_angle);
		float ca = bx::cos(_angle);
		_out[0] = sa;
		_out[1] = ca;
	}

	static void squircle(float* _out, float _angle)
	{
		float sa = bx::sin(_angle);
		float ca = bx::cos(_angle);
		_out[0] = bx::sqrt(bx::abs(sa)) * bx::sign(sa);
		_out[1] = bx::sqrt(bx::abs(ca)) * bx::sign(ca);
	}

	static uint32_t genSphere(uint8_t _subdiv0, void* _pos0 = NULL, uint16_t _posStride0 = 0, void* _normals0 = NULL, uint16_t _normalStride0 = 0)
	{
		if (NULL != _pos0)
		{
			struct Gen
			{
				Gen(void* _pos, uint16_t _posStride, void* _normals, uint16_t _normalStride, uint8_t _subdiv)
					: m_pos((uint8_t*)_pos)
					, m_normals((uint8_t*)_normals)
					, m_posStride(_posStride)
					, m_normalStride(_normalStride)
				{
					static const float scale = 1.0f;
					static const float golden = 1.6180339887f;
					static const float len = bx::sqrt(golden * golden + 1.0f);
					static const float ss = 1.0f / len * scale;
					static const float ll = ss * golden;

					static const bx::Vec3 vv[] =
					{
						{ -ll, 0.0f, -ss },
						{  ll, 0.0f, -ss },
						{  ll, 0.0f,  ss },
						{ -ll, 0.0f,  ss },

						{ -ss,  ll, 0.0f },
						{  ss,  ll, 0.0f },
						{  ss, -ll, 0.0f },
						{ -ss, -ll, 0.0f },

						{ 0.0f, -ss,  ll },
						{ 0.0f,  ss,  ll },
						{ 0.0f,  ss, -ll },
						{ 0.0f, -ss, -ll },
					};

					m_numVertices = 0;

					triangle(vv[0], vv[4], vv[3], scale, _subdiv);
					triangle(vv[0], vv[10], vv[4], scale, _subdiv);
					triangle(vv[4], vv[10], vv[5], scale, _subdiv);
					triangle(vv[5], vv[10], vv[1], scale, _subdiv);
					triangle(vv[5], vv[1], vv[2], scale, _subdiv);
					triangle(vv[5], vv[2], vv[9], scale, _subdiv);
					triangle(vv[5], vv[9], vv[4], scale, _subdiv);
					triangle(vv[3], vv[4], vv[9], scale, _subdiv);

					triangle(vv[0], vv[3], vv[7], scale, _subdiv);
					triangle(vv[0], vv[7], vv[11], scale, _subdiv);
					triangle(vv[11], vv[7], vv[6], scale, _subdiv);
					triangle(vv[11], vv[6], vv[1], scale, _subdiv);
					triangle(vv[1], vv[6], vv[2], scale, _subdiv);
					triangle(vv[2], vv[6], vv[8], scale, _subdiv);
					triangle(vv[8], vv[6], vv[7], scale, _subdiv);
					triangle(vv[8], vv[7], vv[3], scale, _subdiv);

					triangle(vv[0], vv[11], vv[10], scale, _subdiv);
					triangle(vv[1], vv[10], vv[11], scale, _subdiv);
					triangle(vv[2], vv[8], vv[9], scale, _subdiv);
					triangle(vv[3], vv[9], vv[8], scale, _subdiv);
				}

				void addVert(const bx::Vec3& _v)
				{
					bx::store(m_pos, _v);
					m_pos += m_posStride;

					if (NULL != m_normals)
					{
						const bx::Vec3 normal = bx::normalize(_v);
						bx::store(m_normals, normal);

						m_normals += m_normalStride;
					}

					m_numVertices++;
				}

				void triangle(const bx::Vec3& _v0, const bx::Vec3& _v1, const bx::Vec3& _v2, float _scale, uint8_t _subdiv)
				{
					if (0 == _subdiv)
					{
						addVert(_v0);
						addVert(_v1);
						addVert(_v2);
					}
					else
					{
						const bx::Vec3 v01 = bx::mul(bx::normalize(bx::add(_v0, _v1)), _scale);
						const bx::Vec3 v12 = bx::mul(bx::normalize(bx::add(_v1, _v2)), _scale);
						const bx::Vec3 v20 = bx::mul(bx::normalize(bx::add(_v2, _v0)), _scale);

						--_subdiv;
						triangle(_v0, v01, v20, _scale, _subdiv);
						triangle(_v1, v12, v01, _scale, _subdiv);
						triangle(_v2, v20, v12, _scale, _subdiv);
						triangle(v01, v12, v20, _scale, _subdiv);
					}
				}

				uint8_t* m_pos;
				uint8_t* m_normals;
				uint16_t m_posStride;
				uint16_t m_normalStride;
				uint32_t m_numVertices;

			} gen(_pos0, _posStride0, _normals0, _normalStride0, _subdiv0);
		}

		uint32_t numVertices = 20 * 3 * bx::uint32_max(1, (uint32_t)bx::pow(4.0f, _subdiv0));
		return numVertices;
	}

	static bx::Vec3 getPoint(Axis::Enum _axis, float _x, float _y)
	{
		switch (_axis)
		{
		case Axis::X: return { 0.0f,   _x,   _y };
		case Axis::Y: return { _y, 0.0f,   _x };
		default: break;
		}

		return { _x, _y, 0.0f };
	}

	static bool checkAvailTransientBuffers(uint32_t _numVertices, const max::VertexLayout& _layout, uint32_t _numIndices)
	{
		return _numVertices == max::getAvailTransientVertexBuffer(_numVertices, _layout)
			&& (0 == _numIndices || _numIndices == max::getAvailTransientIndexBuffer(_numIndices))
			;
	}

	struct DebugDrawShared
	{
		void init();
		void shutdown();

		DebugMesh m_mesh[DebugMesh::Count];

		max::UniformHandle s_texColor;
		max::ProgramHandle m_program[DebugProgram::Count];
		max::UniformHandle u_params;

		max::VertexBufferHandle m_vbh;
		max::IndexBufferHandle  m_ibh;
	};

	BX_ALIGN_DECL_CACHE_LINE(struct) DebugDrawEncoderImpl
	{
		DebugDrawEncoderImpl()
			: m_depthTestLess(true)
			, m_state(State::Count)
			, m_defaultEncoder(NULL)
		{
		}

		void init(max::Encoder* _encoder)
		{
			m_defaultEncoder = _encoder;
			m_state = State::Count;
		}

		void shutdown()
		{
		}

		void begin(max::ViewId _viewId, bool _depthTestLess, max::Encoder* _encoder)
		{
			BX_ASSERT(State::Count == m_state, "");

			m_viewId = _viewId;
			m_encoder = _encoder == NULL ? m_defaultEncoder : _encoder;
			m_state = State::None;
			m_stack = 0;
			m_depthTestLess = _depthTestLess;

			m_pos = 0;
			m_indexPos = 0;
			m_vertexPos = 0;
			m_posQuad = 0;

			DebugAttrib& attrib = m_attrib[0];
			attrib.m_state = 0
				| MAX_STATE_WRITE_RGB
				| (m_depthTestLess ? MAX_STATE_DEPTH_TEST_LESS : MAX_STATE_DEPTH_TEST_GREATER)
				| MAX_STATE_CULL_CW
				| MAX_STATE_WRITE_Z
				;
			attrib.m_scale = 1.0f;
			attrib.m_spin = 0.0f;
			attrib.m_offset = 0.0f;
			attrib.m_abgr = UINT32_MAX;
			attrib.m_stipple = false;
			attrib.m_wireframe = false;
			attrib.m_lod = 0;

			m_mtxStackCurrent = 0;
			m_mtxStack[m_mtxStackCurrent].reset();
		}

		void end()
		{
			BX_ASSERT(0 == m_stack, "Invalid stack %d.", m_stack);

			flushQuad();
			flush();

			m_encoder = NULL;
			m_state = State::Count;
		}

		void push()
		{
			BX_ASSERT(State::Count != m_state, "");
			++m_stack;
			m_attrib[m_stack] = m_attrib[m_stack - 1];
		}

		void pop()
		{
			BX_ASSERT(State::Count != m_state, "");
			const DebugAttrib& curr = m_attrib[m_stack];
			const  DebugAttrib& prev = m_attrib[m_stack - 1];
			if (curr.m_stipple != prev.m_stipple
				|| curr.m_state != prev.m_state)
			{
				flush();
			}
			--m_stack;
		}

		void setDepthTestLess(bool _depthTestLess)
		{
			BX_ASSERT(State::Count != m_state, "");
			if (m_depthTestLess != _depthTestLess)
			{
				m_depthTestLess = _depthTestLess;
				DebugAttrib& attrib = m_attrib[m_stack];
				if (attrib.m_state & MAX_STATE_DEPTH_TEST_MASK)
				{
					flush();
					attrib.m_state &= ~MAX_STATE_DEPTH_TEST_MASK;
					attrib.m_state |= _depthTestLess ? MAX_STATE_DEPTH_TEST_LESS : MAX_STATE_DEPTH_TEST_GREATER;
				}
			}
		}

		void setTransform(const void* _mtx, uint16_t _num = 1, bool _flush = true)
		{
			BX_ASSERT(State::Count != m_state, "");
			if (_flush)
			{
				flush();
			}

			MatrixStack& stack = m_mtxStack[m_mtxStackCurrent];

			if (NULL == _mtx)
			{
				stack.reset();
				return;
			}

			max::Transform transform;
			stack.mtx = m_encoder->allocTransform(&transform, _num);
			stack.num = _num;
			stack.data = transform.data;
			bx::memCopy(transform.data, _mtx, _num * 64);
		}

		void setTranslate(float _x, float _y, float _z)
		{
			float mtx[16];
			bx::mtxTranslate(mtx, _x, _y, _z);
			setTransform(mtx);
		}

		void setTranslate(const float* _pos)
		{
			setTranslate(_pos[0], _pos[1], _pos[2]);
		}

		void pushTransform(const void* _mtx, uint16_t _num, bool _flush = true)
		{
			BX_ASSERT(m_mtxStackCurrent < BX_COUNTOF(m_mtxStack), "Out of matrix stack!");
			BX_ASSERT(State::Count != m_state, "");
			if (_flush)
			{
				flush();
			}

			float* mtx = NULL;

			const MatrixStack& stack = m_mtxStack[m_mtxStackCurrent];

			if (NULL == stack.data)
			{
				mtx = (float*)_mtx;
			}
			else
			{
				mtx = (float*)alloca(_num * 64);
				for (uint16_t ii = 0; ii < _num; ++ii)
				{
					const float* mtxTransform = (const float*)_mtx;
					bx::mtxMul(&mtx[ii * 16], &mtxTransform[ii * 16], stack.data);
				}
			}

			m_mtxStackCurrent++;
			setTransform(mtx, _num, _flush);
		}

		void popTransform(bool _flush = true)
		{
			BX_ASSERT(State::Count != m_state, "");
			if (_flush)
			{
				flush();
			}

			m_mtxStackCurrent--;
		}

		void pushTranslate(float _x, float _y, float _z)
		{
			float mtx[16];
			bx::mtxTranslate(mtx, _x, _y, _z);
			pushTransform(mtx, 1);
		}

		void pushTranslate(const bx::Vec3& _pos)
		{
			pushTranslate(_pos.x, _pos.y, _pos.z);
		}

		void setState(bool _depthTest, bool _depthWrite, bool _clockwise)
		{
			const uint64_t depthTest = m_depthTestLess
				? MAX_STATE_DEPTH_TEST_LESS
				: MAX_STATE_DEPTH_TEST_GREATER
				;

			uint64_t state = m_attrib[m_stack].m_state & ~(0
				| MAX_STATE_DEPTH_TEST_MASK
				| MAX_STATE_WRITE_Z
				| MAX_STATE_CULL_CW
				| MAX_STATE_CULL_CCW
				);

			state |= _depthTest
				? depthTest
				: 0
				;

			state |= _depthWrite
				? MAX_STATE_WRITE_Z
				: 0
				;

			state |= _clockwise
				? MAX_STATE_CULL_CW
				: MAX_STATE_CULL_CCW
				;

			if (m_attrib[m_stack].m_state != state)
			{
				flush();
			}

			m_attrib[m_stack].m_state = state;
		}

		void setColor(uint32_t _abgr)
		{
			BX_ASSERT(State::Count != m_state, "");
			m_attrib[m_stack].m_abgr = _abgr;
		}

		void setLod(uint8_t _lod)
		{
			BX_ASSERT(State::Count != m_state, "");
			m_attrib[m_stack].m_lod = _lod;
		}

		void setWireframe(bool _wireframe)
		{
			BX_ASSERT(State::Count != m_state, "");
			m_attrib[m_stack].m_wireframe = _wireframe;
		}

		void setStipple(bool _stipple, float _scale = 1.0f, float _offset = 0.0f)
		{
			BX_ASSERT(State::Count != m_state, "");

			DebugAttrib& attrib = m_attrib[m_stack];

			if (attrib.m_stipple != _stipple)
			{
				flush();
			}

			attrib.m_stipple = _stipple;
			attrib.m_offset = _offset;
			attrib.m_scale = _scale;
		}

		void setSpin(float _spin)
		{
			DebugAttrib& attrib = m_attrib[m_stack];
			attrib.m_spin = _spin;
		}

		void moveTo(float _x, float _y, float _z = 0.0f)
		{
			BX_ASSERT(State::Count != m_state, "");

			softFlush();

			m_state = State::MoveTo;

			DebugPosVertex& vertex = m_cache[m_pos];
			vertex.m_x = _x;
			vertex.m_y = _y;
			vertex.m_z = _z;

			DebugAttrib& attrib = m_attrib[m_stack];
			vertex.m_abgr = attrib.m_abgr;
			vertex.m_len = attrib.m_offset;

			m_vertexPos = m_pos;
		}

		void moveTo(const bx::Vec3& _pos)
		{
			BX_ASSERT(State::Count != m_state, "");
			moveTo(_pos.x, _pos.y, _pos.z);
		}

		void moveTo(Axis::Enum _axis, float _x, float _y)
		{
			moveTo(getPoint(_axis, _x, _y));
		}

		void lineTo(float _x, float _y, float _z = 0.0f)
		{
			BX_ASSERT(State::Count != m_state, "");
			if (State::None == m_state)
			{
				moveTo(_x, _y, _z);
				return;
			}

			if (m_pos + 2 > uint16_t(BX_COUNTOF(m_cache)))
			{
				uint32_t pos = m_pos;
				uint32_t vertexPos = m_vertexPos;

				flush();

				bx::memCopy(&m_cache[0], &m_cache[vertexPos], sizeof(DebugPosVertex));
				if (vertexPos == pos)
				{
					m_pos = 1;
				}
				else
				{
					bx::memCopy(&m_cache[1], &m_cache[pos - 1], sizeof(DebugPosVertex));
					m_pos = 2;
				}

				m_state = State::LineTo;
			}
			else if (State::MoveTo == m_state)
			{
				++m_pos;
				m_state = State::LineTo;
			}

			uint16_t prev = m_pos - 1;
			uint16_t curr = m_pos++;
			DebugPosVertex& vertex = m_cache[curr];
			vertex.m_x = _x;
			vertex.m_y = _y;
			vertex.m_z = _z;

			DebugAttrib& attrib = m_attrib[m_stack];
			vertex.m_abgr = attrib.m_abgr;
			vertex.m_len = attrib.m_offset;

			float len = bx::length(bx::sub(bx::load<bx::Vec3>(&vertex.m_x), bx::load<bx::Vec3>(&m_cache[prev].m_x))) * attrib.m_scale;
			vertex.m_len = m_cache[prev].m_len + len;

			m_indices[m_indexPos++] = prev;
			m_indices[m_indexPos++] = curr;
		}

		void lineTo(const bx::Vec3& _pos)
		{
			BX_ASSERT(State::Count != m_state, "");
			lineTo(_pos.x, _pos.y, _pos.z);
		}

		void lineTo(Axis::Enum _axis, float _x, float _y)
		{
			lineTo(getPoint(_axis, _x, _y));
		}

		void close()
		{
			BX_ASSERT(State::Count != m_state, "");
			DebugPosVertex& vertex = m_cache[m_vertexPos];
			lineTo(vertex.m_x, vertex.m_y, vertex.m_z);

			m_state = State::None;
		}

		void draw(const bx::Aabb& _aabb)
		{
			const DebugAttrib& attrib = m_attrib[m_stack];
			if (attrib.m_wireframe)
			{
				moveTo(_aabb.min.x, _aabb.min.y, _aabb.min.z);
				lineTo(_aabb.max.x, _aabb.min.y, _aabb.min.z);
				lineTo(_aabb.max.x, _aabb.max.y, _aabb.min.z);
				lineTo(_aabb.min.x, _aabb.max.y, _aabb.min.z);
				close();

				moveTo(_aabb.min.x, _aabb.min.y, _aabb.max.z);
				lineTo(_aabb.max.x, _aabb.min.y, _aabb.max.z);
				lineTo(_aabb.max.x, _aabb.max.y, _aabb.max.z);
				lineTo(_aabb.min.x, _aabb.max.y, _aabb.max.z);
				close();

				moveTo(_aabb.min.x, _aabb.min.y, _aabb.min.z);
				lineTo(_aabb.min.x, _aabb.min.y, _aabb.max.z);

				moveTo(_aabb.max.x, _aabb.min.y, _aabb.min.z);
				lineTo(_aabb.max.x, _aabb.min.y, _aabb.max.z);

				moveTo(_aabb.min.x, _aabb.max.y, _aabb.min.z);
				lineTo(_aabb.min.x, _aabb.max.y, _aabb.max.z);

				moveTo(_aabb.max.x, _aabb.max.y, _aabb.min.z);
				lineTo(_aabb.max.x, _aabb.max.y, _aabb.max.z);
			}
			else
			{
				bx::Obb obb;
				toObb(obb, _aabb);
				draw(DebugMesh::Cube, obb.mtx, 1, false);
			}
		}

		void draw(const bx::Cylinder& _cylinder, bool _capsule)
		{
			drawCylinder(_cylinder.pos, _cylinder.end, _cylinder.radius, _capsule);
		}

		void draw(const bx::Disk& _disk)
		{
			drawCircle(_disk.normal, _disk.center, _disk.radius, 0.0f);
		}

		void draw(const bx::Obb& _obb)
		{
			const DebugAttrib& attrib = m_attrib[m_stack];
			if (attrib.m_wireframe)
			{
				pushTransform(_obb.mtx, 1);

				moveTo(-1.0f, -1.0f, -1.0f);
				lineTo(1.0f, -1.0f, -1.0f);
				lineTo(1.0f, 1.0f, -1.0f);
				lineTo(-1.0f, 1.0f, -1.0f);
				close();

				moveTo(-1.0f, 1.0f, 1.0f);
				lineTo(1.0f, 1.0f, 1.0f);
				lineTo(1.0f, -1.0f, 1.0f);
				lineTo(-1.0f, -1.0f, 1.0f);
				close();

				moveTo(1.0f, -1.0f, -1.0f);
				lineTo(1.0f, -1.0f, 1.0f);

				moveTo(1.0f, 1.0f, -1.0f);
				lineTo(1.0f, 1.0f, 1.0f);

				moveTo(-1.0f, 1.0f, -1.0f);
				lineTo(-1.0f, 1.0f, 1.0f);

				moveTo(-1.0f, -1.0f, -1.0f);
				lineTo(-1.0f, -1.0f, 1.0f);

				popTransform();
			}
			else
			{
				draw(DebugMesh::Cube, _obb.mtx, 1, false);
			}
		}

		void draw(const bx::Sphere& _sphere)
		{
			const DebugAttrib& attrib = m_attrib[m_stack];
			float mtx[16];
			bx::mtxSRT(mtx
				, _sphere.radius
				, _sphere.radius
				, _sphere.radius
				, 0.0f
				, 0.0f
				, 0.0f
				, _sphere.center.x
				, _sphere.center.y
				, _sphere.center.z
			);
			uint8_t lod = attrib.m_lod > DebugMesh::SphereMaxLod
				? uint8_t(DebugMesh::SphereMaxLod)
				: attrib.m_lod
				;
			draw(DebugMesh::Enum(DebugMesh::Sphere0 + lod), mtx, 1, attrib.m_wireframe);
		}

		void draw(const bx::Triangle& _triangle)
		{
			DebugAttrib& attrib = m_attrib[m_stack];
			if (attrib.m_wireframe)
			{
				moveTo(_triangle.v0);
				lineTo(_triangle.v1);
				lineTo(_triangle.v2);
				close();
			}
			else
			{
				uint64_t old = attrib.m_state;
				attrib.m_state &= ~MAX_STATE_CULL_MASK;

				draw(false, 3, reinterpret_cast<const bx::Vec3*>(&_triangle.v0.x), 0, NULL);

				attrib.m_state = old;
			}
		}

		void setUParams(const DebugAttrib& _attrib, bool _wireframe);

		void draw(bool _lineList, uint32_t _numVertices, const bx::Vec3* _vertices, uint32_t _numIndices, const uint16_t* _indices);

		void drawFrustum(const float* _viewProj)
		{
			bx::Plane planes[6] = { bx::InitNone, bx::InitNone, bx::InitNone, bx::InitNone, bx::InitNone, bx::InitNone };
			buildFrustumPlanes(planes, _viewProj);

			const bx::Vec3 points[8] =
			{
				intersectPlanes(planes[0], planes[2], planes[4]),
				intersectPlanes(planes[0], planes[3], planes[4]),
				intersectPlanes(planes[0], planes[3], planes[5]),
				intersectPlanes(planes[0], planes[2], planes[5]),
				intersectPlanes(planes[1], planes[2], planes[4]),
				intersectPlanes(planes[1], planes[3], planes[4]),
				intersectPlanes(planes[1], planes[3], planes[5]),
				intersectPlanes(planes[1], planes[2], planes[5]),
			};

			moveTo(points[0]);
			lineTo(points[1]);
			lineTo(points[2]);
			lineTo(points[3]);
			close();

			moveTo(points[4]);
			lineTo(points[5]);
			lineTo(points[6]);
			lineTo(points[7]);
			close();

			moveTo(points[0]);
			lineTo(points[4]);

			moveTo(points[1]);
			lineTo(points[5]);

			moveTo(points[2]);
			lineTo(points[6]);

			moveTo(points[3]);
			lineTo(points[7]);
		}

		void drawFrustum(const void* _viewProj)
		{
			drawFrustum((const float*)_viewProj);
		}

		void drawArc(Axis::Enum _axis, float _x, float _y, float _z, float _radius, float _degrees)
		{
			const DebugAttrib& attrib = m_attrib[m_stack];
			const uint32_t num = getCircleLod(attrib.m_lod);
			const float step = bx::kPi * 2.0f / num;

			_degrees = bx::wrap(_degrees, 360.0f);

			bx::Vec3 pos = getPoint(
				_axis
				, bx::sin(step * 0) * _radius
				, bx::cos(step * 0) * _radius
			);

			moveTo({ pos.x + _x, pos.y + _y, pos.z + _z });

			uint32_t n = uint32_t(num * _degrees / 360.0f);

			for (uint32_t ii = 1; ii < n + 1; ++ii)
			{
				pos = getPoint(
					_axis
					, bx::sin(step * ii) * _radius
					, bx::cos(step * ii) * _radius
				);
				lineTo({ pos.x + _x, pos.y + _y, pos.z + _z });
			}

			moveTo(_x, _y, _z);
			pos = getPoint(
				_axis
				, bx::sin(step * 0) * _radius
				, bx::cos(step * 0) * _radius
			);
			lineTo({ pos.x + _x, pos.y + _y, pos.z + _z });

			pos = getPoint(
				_axis
				, bx::sin(step * n) * _radius
				, bx::cos(step * n) * _radius
			);
			moveTo({ pos.x + _x, pos.y + _y, pos.z + _z });
			lineTo(_x, _y, _z);
		}

		void drawCircle(const bx::Vec3& _normal, const bx::Vec3& _center, float _radius, float _weight)
		{
			const DebugAttrib& attrib = m_attrib[m_stack];
			const uint32_t num = getCircleLod(attrib.m_lod);
			const float step = bx::kPi * 2.0f / num;
			_weight = bx::clamp(_weight, 0.0f, 2.0f);

			bx::Vec3 udir(bx::InitNone);
			bx::Vec3 vdir(bx::InitNone);
			bx::calcTangentFrame(udir, vdir, _normal, attrib.m_spin);

			float xy0[2];
			float xy1[2];
			circle(xy0, 0.0f);
			squircle(xy1, 0.0f);

			bx::Vec3 pos = bx::mul(udir, bx::lerp(xy0[0], xy1[0], _weight) * _radius);
			bx::Vec3 tmp0 = bx::mul(vdir, bx::lerp(xy0[1], xy1[1], _weight) * _radius);
			bx::Vec3 tmp1 = bx::add(pos, tmp0);
			bx::Vec3 tmp2 = bx::add(tmp1, _center);
			moveTo(tmp2);

			for (uint32_t ii = 1; ii < num; ++ii)
			{
				float angle = step * ii;
				circle(xy0, angle);
				squircle(xy1, angle);

				pos = bx::mul(udir, bx::lerp(xy0[0], xy1[0], _weight) * _radius);
				tmp0 = bx::mul(vdir, bx::lerp(xy0[1], xy1[1], _weight) * _radius);
				tmp1 = bx::add(pos, tmp0);
				tmp2 = bx::add(tmp1, _center);
				lineTo(tmp2);
			}

			close();
		}

		void drawCircle(Axis::Enum _axis, float _x, float _y, float _z, float _radius, float _weight)
		{
			const DebugAttrib& attrib = m_attrib[m_stack];
			const uint32_t num = getCircleLod(attrib.m_lod);
			const float step = bx::kPi * 2.0f / num;
			_weight = bx::clamp(_weight, 0.0f, 2.0f);

			float xy0[2];
			float xy1[2];
			circle(xy0, 0.0f);
			squircle(xy1, 0.0f);

			bx::Vec3 pos = getPoint(
				_axis
				, bx::lerp(xy0[0], xy1[0], _weight) * _radius
				, bx::lerp(xy0[1], xy1[1], _weight) * _radius
			);

			moveTo({ pos.x + _x, pos.y + _y, pos.z + _z });

			for (uint32_t ii = 1; ii < num; ++ii)
			{
				float angle = step * ii;
				circle(xy0, angle);
				squircle(xy1, angle);

				pos = getPoint(
					_axis
					, bx::lerp(xy0[0], xy1[0], _weight) * _radius
					, bx::lerp(xy0[1], xy1[1], _weight) * _radius
				);
				lineTo({ pos.x + _x, pos.y + _y, pos.z + _z });
			}
			close();
		}

		void drawQuad(const bx::Vec3& _normal, const bx::Vec3& _center, float _size)
		{
			const DebugAttrib& attrib = m_attrib[m_stack];
			if (attrib.m_wireframe)
			{
				bx::Vec3 udir(bx::InitNone);
				bx::Vec3 vdir(bx::InitNone);
				bx::calcTangentFrame(udir, vdir, _normal, attrib.m_spin);

				const float halfExtent = _size * 0.5f;

				const bx::Vec3 umin = bx::mul(udir, -halfExtent);
				const bx::Vec3 umax = bx::mul(udir, halfExtent);
				const bx::Vec3 vmin = bx::mul(vdir, -halfExtent);
				const bx::Vec3 vmax = bx::mul(vdir, halfExtent);
				const bx::Vec3 center = _center;

				moveTo(bx::add(center, bx::add(umin, vmin)));
				lineTo(bx::add(center, bx::add(umax, vmin)));
				lineTo(bx::add(center, bx::add(umax, vmax)));
				lineTo(bx::add(center, bx::add(umin, vmax)));

				close();
			}
			else
			{
				float mtx[16];
				bx::mtxFromNormal(mtx, _normal, _size * 0.5f, _center, attrib.m_spin);
				draw(DebugMesh::Quad, mtx, 1, false);
			}
		}

		void drawQuad(max::TextureHandle _handle, const bx::Vec3& _normal, const bx::Vec3& _center, float _size)
		{
			BX_UNUSED(_handle, _normal, _center, _size);
		}

		void drawCone(const bx::Vec3& _from, const bx::Vec3& _to, float _radius)
		{
			const DebugAttrib& attrib = m_attrib[m_stack];

			const bx::Vec3 normal = bx::normalize(bx::sub(_from, _to));

			float mtx[2][16];
			bx::mtxFromNormal(mtx[0], normal, _radius, _from, attrib.m_spin);

			bx::memCopy(mtx[1], mtx[0], 64);
			mtx[1][12] = _to.x;
			mtx[1][13] = _to.y;
			mtx[1][14] = _to.z;

			uint8_t lod = attrib.m_lod > DebugMesh::ConeMaxLod
				? uint8_t(DebugMesh::ConeMaxLod)
				: attrib.m_lod
				;
			draw(DebugMesh::Enum(DebugMesh::Cone0 + lod), mtx[0], 2, attrib.m_wireframe);
		}

		void drawCylinder(const bx::Vec3& _from, const bx::Vec3& _to, float _radius, bool _capsule)
		{
			const DebugAttrib& attrib = m_attrib[m_stack];
			const bx::Vec3 normal = bx::normalize(bx::sub(_from, _to));

			float mtx[2][16];
			bx::mtxFromNormal(mtx[0], normal, _radius, _from, attrib.m_spin);

			bx::memCopy(mtx[1], mtx[0], 64);
			mtx[1][12] = _to.x;
			mtx[1][13] = _to.y;
			mtx[1][14] = _to.z;

			if (_capsule)
			{
				uint8_t lod = attrib.m_lod > DebugMesh::CapsuleMaxLod
					? uint8_t(DebugMesh::CapsuleMaxLod)
					: attrib.m_lod
					;
				draw(DebugMesh::Enum(DebugMesh::Capsule0 + lod), mtx[0], 2, attrib.m_wireframe);

				bx::Sphere sphere;
				sphere.center = _from;
				sphere.radius = _radius;
				draw(sphere);

				sphere.center = _to;
				draw(sphere);
			}
			else
			{
				uint8_t lod = attrib.m_lod > DebugMesh::CylinderMaxLod
					? uint8_t(DebugMesh::CylinderMaxLod)
					: attrib.m_lod
					;
				draw(DebugMesh::Enum(DebugMesh::Cylinder0 + lod), mtx[0], 2, attrib.m_wireframe);
			}
		}

		void drawAxis(float _x, float _y, float _z, float _len, Axis::Enum _highlight, float _thickness)
		{
			push();

			if (_thickness > 0.0f)
			{
				const bx::Vec3 from = { _x, _y, _z };
				bx::Vec3 mid(bx::InitNone);
				bx::Vec3 to(bx::InitNone);

				setColor(Axis::X == _highlight ? 0xff00ffff : 0xff0000ff);
				mid = { _x + _len - _thickness, _y, _z };
				to = { _x + _len,              _y, _z };
				drawCylinder(from, mid, _thickness, false);
				drawCone(mid, to, _thickness);

				setColor(Axis::Y == _highlight ? 0xff00ffff : 0xff00ff00);
				mid = { _x, _y + _len - _thickness, _z };
				to = { _x, _y + _len,              _z };
				drawCylinder(from, mid, _thickness, false);
				drawCone(mid, to, _thickness);

				setColor(Axis::Z == _highlight ? 0xff00ffff : 0xffff0000);
				mid = { _x, _y, _z + _len - _thickness };
				to = { _x, _y, _z + _len };
				drawCylinder(from, mid, _thickness, false);
				drawCone(mid, to, _thickness);
			}
			else
			{
				setColor(Axis::X == _highlight ? 0xff00ffff : 0xff0000ff);
				moveTo(_x, _y, _z);
				lineTo(_x + _len, _y, _z);

				setColor(Axis::Y == _highlight ? 0xff00ffff : 0xff00ff00);
				moveTo(_x, _y, _z);
				lineTo(_x, _y + _len, _z);

				setColor(Axis::Z == _highlight ? 0xff00ffff : 0xffff0000);
				moveTo(_x, _y, _z);
				lineTo(_x, _y, _z + _len);
			}

			pop();
		}

		void drawGrid(const bx::Vec3& _normal, const bx::Vec3& _center, uint32_t _size, float _step)
		{
			const DebugAttrib& attrib = m_attrib[m_stack];

			bx::Vec3 udir(bx::InitNone);
			bx::Vec3 vdir(bx::InitNone);
			bx::calcTangentFrame(udir, vdir, _normal, attrib.m_spin);

			udir = bx::mul(udir, _step);
			vdir = bx::mul(vdir, _step);

			const uint32_t num = (_size / 2) * 2 + 1;
			const float halfExtent = float(_size / 2);

			const bx::Vec3 umin = bx::mul(udir, -halfExtent);
			const bx::Vec3 umax = bx::mul(udir, halfExtent);
			const bx::Vec3 vmin = bx::mul(vdir, -halfExtent);
			const bx::Vec3 vmax = bx::mul(vdir, halfExtent);

			bx::Vec3 xs = bx::add(_center, bx::add(umin, vmin));
			bx::Vec3 xe = bx::add(_center, bx::add(umax, vmin));
			bx::Vec3 ys = bx::add(_center, bx::add(umin, vmin));
			bx::Vec3 ye = bx::add(_center, bx::add(umin, vmax));

			for (uint32_t ii = 0; ii < num; ++ii)
			{
				moveTo(xs);
				lineTo(xe);
				xs = bx::add(xs, vdir);
				xe = bx::add(xe, vdir);

				moveTo(ys);
				lineTo(ye);
				ys = bx::add(ys, udir);
				ye = bx::add(ye, udir);
			}
		}

		void drawGrid(Axis::Enum _axis, const bx::Vec3& _center, uint32_t _size, float _step)
		{
			push();
			pushTranslate(_center);

			const uint32_t num = (_size / 2) * 2 - 1;
			const float halfExtent = float(_size / 2) * _step;

			setColor(0xff606060);
			float yy = -halfExtent + _step;
			for (uint32_t ii = 0; ii < num; ++ii)
			{
				moveTo(_axis, -halfExtent, yy);
				lineTo(_axis, halfExtent, yy);

				moveTo(_axis, yy, -halfExtent);
				lineTo(_axis, yy, halfExtent);

				yy += _step;
			}

			setColor(0xff101010);
			moveTo(_axis, -halfExtent, -halfExtent);
			lineTo(_axis, -halfExtent, halfExtent);
			lineTo(_axis, halfExtent, halfExtent);
			lineTo(_axis, halfExtent, -halfExtent);
			close();

			moveTo(_axis, -halfExtent, 0.0f);
			lineTo(_axis, halfExtent, 0.0f);

			moveTo(_axis, 0.0f, -halfExtent);
			lineTo(_axis, 0.0f, halfExtent);

			popTransform();
			pop();
		}

		void drawOrb(float _x, float _y, float _z, float _radius, Axis::Enum _hightlight)
		{
			push();

			setColor(Axis::X == _hightlight ? 0xff00ffff : 0xff0000ff);
			drawCircle(Axis::X, _x, _y, _z, _radius, 0.0f);

			setColor(Axis::Y == _hightlight ? 0xff00ffff : 0xff00ff00);
			drawCircle(Axis::Y, _x, _y, _z, _radius, 0.0f);

			setColor(Axis::Z == _hightlight ? 0xff00ffff : 0xffff0000);
			drawCircle(Axis::Z, _x, _y, _z, _radius, 0.0f);

			pop();
		}

		void draw(DebugMesh::Enum _mesh, const float* _mtx, uint16_t _num, bool _wireframe);

		void softFlush()
		{
			if (m_pos == uint16_t(BX_COUNTOF(m_cache)))
			{
				flush();
			}
		}

		void flush();

		void flushQuad();

		struct State
		{
			enum Enum
			{
				None,
				MoveTo,
				LineTo,

				Count
			};
		};

		static const uint32_t kCacheSize = 1024;
		static const uint32_t kStackSize = 16;
		static const uint32_t kCacheQuadSize = 1024;
		BX_STATIC_ASSERT(kCacheSize >= 3, "Cache must be at least 3 elements.");

		DebugPosVertex   m_cache[kCacheSize + 1];
		DebugUvVertex m_cacheQuad[kCacheQuadSize];
		uint16_t m_indices[kCacheSize * 2];
		uint16_t m_pos;
		uint16_t m_posQuad;
		uint16_t m_indexPos;
		uint16_t m_vertexPos;
		uint32_t m_mtxStackCurrent;

		struct MatrixStack
		{
			void reset()
			{
				mtx = 0;
				num = 1;
				data = NULL;
			}

			uint32_t mtx;
			uint16_t num;
			float* data;
		};

		MatrixStack m_mtxStack[32];

		max::ViewId m_viewId;
		uint8_t m_stack;
		bool    m_depthTestLess;

		DebugAttrib m_attrib[kStackSize];

		State::Enum m_state;

		max::Encoder* m_encoder;
		max::Encoder* m_defaultEncoder;
	};

	struct VertexLayoutRef
	{
		VertexLayoutRef()
		{
		}

		void init()
		{
			bx::memSet(m_refCount,                  0, sizeof(m_refCount)               );
			bx::memSet(m_vertexBufferRef,        0xff, sizeof(m_vertexBufferRef)        );
			bx::memSet(m_dynamicVertexBufferRef, 0xff, sizeof(m_dynamicVertexBufferRef) );
		}

		template <uint16_t MaxHandlesT>
		void shutdown(bx::HandleAllocT<MaxHandlesT>& _handleAlloc)
		{
			for (uint16_t ii = 0, num = _handleAlloc.getNumHandles(); ii < num; ++ii)
			{
				VertexLayoutHandle handle = { _handleAlloc.getHandleAt(ii) };
				m_refCount[handle.idx] = 0;
				m_vertexLayoutMap.removeByHandle(handle.idx);
				_handleAlloc.free(handle.idx);
			}

			m_vertexLayoutMap.reset();
		}

		VertexLayoutHandle find(uint32_t _hash)
		{
			VertexLayoutHandle handle = { m_vertexLayoutMap.find(_hash) };
			return handle;
		}

		void add(VertexLayoutHandle _layoutHandle, uint32_t _hash)
		{
			m_refCount[_layoutHandle.idx]++;
			m_vertexLayoutMap.insert(_hash, _layoutHandle.idx);
		}

		void add(VertexBufferHandle _handle, VertexLayoutHandle _layoutHandle, uint32_t _hash)
		{
			BX_ASSERT(m_vertexBufferRef[_handle.idx].idx == kInvalidHandle, "");
			m_vertexBufferRef[_handle.idx] = _layoutHandle;
			m_refCount[_layoutHandle.idx]++;
			m_vertexLayoutMap.insert(_hash, _layoutHandle.idx);
		}

		void add(DynamicVertexBufferHandle _handle, VertexLayoutHandle _layoutHandle, uint32_t _hash)
		{
			BX_ASSERT(m_dynamicVertexBufferRef[_handle.idx].idx == kInvalidHandle, "");
			m_dynamicVertexBufferRef[_handle.idx] = _layoutHandle;
			m_refCount[_layoutHandle.idx]++;
			m_vertexLayoutMap.insert(_hash, _layoutHandle.idx);
		}

		VertexLayoutHandle release(VertexLayoutHandle _layoutHandle)
		{
			if (isValid(_layoutHandle) )
			{
				m_refCount[_layoutHandle.idx]--;

				if (0 == m_refCount[_layoutHandle.idx])
				{
					m_vertexLayoutMap.removeByHandle(_layoutHandle.idx);
					return _layoutHandle;
				}
			}

			return MAX_INVALID_HANDLE;
		}

		VertexLayoutHandle release(VertexBufferHandle _handle)
		{
			VertexLayoutHandle layoutHandle = m_vertexBufferRef[_handle.idx];
			layoutHandle = release(layoutHandle);
			m_vertexBufferRef[_handle.idx].idx = kInvalidHandle;

			return layoutHandle;
		}

		VertexLayoutHandle release(DynamicVertexBufferHandle _handle)
		{
			VertexLayoutHandle layoutHandle = m_dynamicVertexBufferRef[_handle.idx];
			layoutHandle = release(layoutHandle);
			m_dynamicVertexBufferRef[_handle.idx].idx = kInvalidHandle;

			return layoutHandle;
		}

		typedef bx::HandleHashMapT<MAX_CONFIG_MAX_VERTEX_LAYOUTS*2> VertexLayoutMap;
		VertexLayoutMap m_vertexLayoutMap;

		uint16_t m_refCount[MAX_CONFIG_MAX_VERTEX_LAYOUTS];
		VertexLayoutHandle m_vertexBufferRef[MAX_CONFIG_MAX_VERTEX_BUFFERS];
		VertexLayoutHandle m_dynamicVertexBufferRef[MAX_CONFIG_MAX_DYNAMIC_VERTEX_BUFFERS];
	};

	// First-fit non-local allocator.
	class NonLocalAllocator
	{
	public:
		static const uint64_t kInvalidBlock = UINT64_MAX;

		NonLocalAllocator()
		{
		}

		~NonLocalAllocator()
		{
		}

		void reset()
		{
			m_free.clear();
			m_used.clear();
		}

		void add(uint64_t _ptr, uint32_t _size)
		{
			m_free.push_back(Free(_ptr, _size) );
		}

		uint64_t remove()
		{
			BX_ASSERT(0 == m_used.size(), "");

			if (0 < m_free.size() )
			{
				Free freeBlock = m_free.front();
				m_free.pop_front();
				return freeBlock.m_ptr;
			}

			return 0;
		}

		uint64_t alloc(uint32_t _size)
		{
			_size = bx::max(_size, 16u);

			for (FreeList::iterator it = m_free.begin(), itEnd = m_free.end(); it != itEnd; ++it)
			{
				if (it->m_size >= _size)
				{
					uint64_t ptr = it->m_ptr;

					m_used.insert(stl::make_pair(ptr, _size) );

					if (it->m_size != _size)
					{
						it->m_size -= _size;
						it->m_ptr  += _size;
					}
					else
					{
						m_free.erase(it);
					}

					return ptr;
				}
			}

			// there is no block large enough.
			return kInvalidBlock;
		}

		void free(uint64_t _block)
		{
			UsedList::iterator it = m_used.find(_block);
			if (it != m_used.end() )
			{
				m_free.push_front(Free(it->first, it->second) );
				m_used.erase(it);
			}
		}

		bool compact()
		{
			m_free.sort();

			for (FreeList::iterator it = m_free.begin(), next = it, itEnd = m_free.end(); next != itEnd;)
			{
				if ( (it->m_ptr + it->m_size) == next->m_ptr)
				{
					it->m_size += next->m_size;
					next = m_free.erase(next);
				}
				else
				{
					it = next;
					++next;
				}
			}

			return 0 == m_used.size();
		}

	private:
		struct Free
		{
			Free(uint64_t _ptr, uint32_t _size)
				: m_ptr(_ptr)
				, m_size(_size)
			{
			}

			bool operator<(const Free& rhs) const
			{
				return m_ptr < rhs.m_ptr;
			}

			uint64_t m_ptr;
			uint32_t m_size;
		};

		typedef stl::list<Free> FreeList;
		FreeList m_free;

		typedef stl::unordered_map<uint64_t, uint32_t> UsedList;
		UsedList m_used;
	};

	struct BX_NO_VTABLE RendererContextI
	{
		virtual ~RendererContextI() = 0;
		virtual RendererType::Enum getRendererType() const = 0;
		virtual const char* getRendererName() const = 0;
		virtual bool isDeviceRemoved() = 0;
		virtual void flip() = 0;
		virtual void createIndexBuffer(IndexBufferHandle _handle, const Memory* _mem, uint16_t _flags) = 0;
		virtual void destroyIndexBuffer(IndexBufferHandle _handle) = 0;
		virtual void createVertexLayout(VertexLayoutHandle _handle, const VertexLayout& _layout) = 0;
		virtual void destroyVertexLayout(VertexLayoutHandle _handle) = 0;
		virtual void createVertexBuffer(VertexBufferHandle _handle, const Memory* _mem, VertexLayoutHandle _layoutHandle, uint16_t _flags) = 0;
		virtual void destroyVertexBuffer(VertexBufferHandle _handle) = 0;
		virtual void createDynamicIndexBuffer(IndexBufferHandle _handle, uint32_t _size, uint16_t _flags) = 0;
		virtual void updateDynamicIndexBuffer(IndexBufferHandle _handle, uint32_t _offset, uint32_t _size, const Memory* _mem) = 0;
		virtual void destroyDynamicIndexBuffer(IndexBufferHandle _handle) = 0;
		virtual void createDynamicVertexBuffer(VertexBufferHandle _handle, uint32_t _size, uint16_t _flags) = 0;
		virtual void updateDynamicVertexBuffer(VertexBufferHandle _handle, uint32_t _offset, uint32_t _size, const Memory* _mem) = 0;
		virtual void destroyDynamicVertexBuffer(VertexBufferHandle _handle) = 0;
		virtual void createShader(ShaderHandle _handle, const Memory* _mem) = 0;
		virtual void destroyShader(ShaderHandle _handle) = 0;
		virtual void createProgram(ProgramHandle _handle, ShaderHandle _vsh, ShaderHandle _fsh) = 0;
		virtual void destroyProgram(ProgramHandle _handle) = 0;
		virtual void* createTexture(TextureHandle _handle, const Memory* _mem, uint64_t _flags, uint8_t _skip) = 0;
		virtual void updateTextureBegin(TextureHandle _handle, uint8_t _side, uint8_t _mip) = 0;
		virtual void updateTexture(TextureHandle _handle, uint8_t _side, uint8_t _mip, const Rect& _rect, uint16_t _z, uint16_t _depth, uint16_t _pitch, const Memory* _mem) = 0;
		virtual void updateTextureEnd() = 0;
		virtual void readTexture(TextureHandle _handle, void* _data, uint8_t _mip) = 0;
		virtual void resizeTexture(TextureHandle _handle, uint16_t _width, uint16_t _height, uint8_t _numMips, uint16_t _numLayers) = 0;
		virtual void overrideInternal(TextureHandle _handle, uintptr_t _ptr) = 0;
		virtual uintptr_t getInternal(TextureHandle _handle) = 0;
		virtual void destroyTexture(TextureHandle _handle) = 0;
		virtual void createFrameBuffer(FrameBufferHandle _handle, uint8_t _num, const Attachment* _attachment) = 0;
		virtual void createFrameBuffer(FrameBufferHandle _handle, void* _nwh, uint32_t _width, uint32_t _height, TextureFormat::Enum _format, TextureFormat::Enum _depthFormat) = 0;
		virtual void destroyFrameBuffer(FrameBufferHandle _handle) = 0;
		virtual void createUniform(UniformHandle _handle, UniformType::Enum _type, uint16_t _num, const char* _name) = 0;
		virtual void destroyUniform(UniformHandle _handle) = 0;
		virtual void requestScreenShot(FrameBufferHandle _handle, const char* _filePath) = 0;
		virtual void updateViewName(ViewId _id, const char* _name) = 0;
		virtual void updateUniform(uint16_t _loc, const void* _data, uint32_t _size) = 0;
		virtual void invalidateOcclusionQuery(OcclusionQueryHandle _handle) = 0;
		virtual void setMarker(const char* _name, uint16_t _len) = 0;
		virtual void setName(Handle _handle, const char* _name, uint16_t _len) = 0;
		virtual void submit(Frame* _render, ClearQuad& _clearQuad, TextVideoMemBlitter& _textVideoMemBlitter) = 0;
		virtual void blitSetup(TextVideoMemBlitter& _blitter) = 0;
		virtual void blitRender(TextVideoMemBlitter& _blitter, uint32_t _numIndices) = 0;
	};

	inline RendererContextI::~RendererContextI()
	{
	}

	void rendererUpdateUniforms(RendererContextI* _renderCtx, UniformBuffer* _uniformBuffer, uint32_t _begin, uint32_t _end);

	struct BX_NO_VTABLE PhysicsContextI
	{
		virtual ~PhysicsContextI() = 0;
		virtual PhysicsType::Enum getPhysicsType() const = 0;
		virtual const char* getPhysicsName() const = 0;
		virtual void simulate(const float _dt) = 0;
		virtual void createBody(BodyHandle _handle, CollisionShape::Enum _shape, const bx::Vec3& _pos, const bx::Quaternion& _quat, const bx::Vec3& _scale, LayerType::Enum _layer, MotionType::Enum _motion, Activation::Enum _activation, float _maxVelocity, uint8_t _flags) = 0;
		virtual void destroyBody(BodyHandle _handle) = 0;
		virtual void setPosition(BodyHandle _handle, const bx::Vec3& _pos, Activation::Enum _activation) = 0;
		virtual bx::Vec3 getPosition(BodyHandle _handle) = 0;
		virtual void setRotation(BodyHandle _handle, const bx::Quaternion& _rot, Activation::Enum _activation) = 0;
		virtual bx::Quaternion getRotation(BodyHandle _handle) = 0;
		virtual void setLinearVelocity(BodyHandle _handle, const bx::Vec3& _velocity) = 0;
		virtual bx::Vec3 getLinearVelocity(BodyHandle _handle) = 0;
		virtual void setAngularVelocity(BodyHandle _handle, const bx::Vec3& _angularVelocity) = 0;
		virtual bx::Vec3 getAngularVelocity(BodyHandle _handle) = 0;
		virtual void addLinearAndAngularVelocity(BodyHandle _handle, const bx::Vec3& _linearVelocity, const bx::Vec3& _angularVelocity) = 0;
		virtual void addLinearImpulse(BodyHandle _handle, const bx::Vec3& _impulse) = 0;
		virtual void addAngularImpulse(BodyHandle _handle, const bx::Vec3& _impulse) = 0;
		virtual void addBuoyancyImpulse(BodyHandle _handle, const bx::Vec3& _surfacePosition, const bx::Vec3& _surfaceNormal, float _buoyancy, float _linearDrag, float _angularDrag, const bx::Vec3& _fluidVelocity, const bx::Vec3& _gravity, float _deltaTime) = 0;
		virtual void addForce(BodyHandle _handle, const bx::Vec3& _force, Activation::Enum _activation) = 0;
		virtual void addTorque(BodyHandle _handle, const bx::Vec3& _torque, Activation::Enum _activation) = 0;
		virtual void addMovement(BodyHandle _handle, const bx::Vec3& _position, const bx::Quaternion& _rotation, const float _deltaTime) = 0;
		virtual void setFriction(BodyHandle _handle, float _friction) = 0;
		virtual float getFriction(BodyHandle _handle) = 0;
		virtual void getGroundInfo(BodyHandle _handle, GroundInfo& _info) = 0;
		virtual const bx::Vec3 getGravity() = 0;
	};

	inline PhysicsContextI::~PhysicsContextI()
	{
	}

	struct InputMouse
	{
		InputMouse()
			: m_width(1280)
			, m_height(720)
			, m_wheelDelta(120)
			, m_lock(false)
		{
		}

		void reset()
		{
			if (m_lock)
			{
				m_norm[0] = 0.0f;
				m_norm[1] = 0.0f;
				m_norm[2] = 0.0f;
			}

			bx::memSet(m_buttons, 0, sizeof(m_buttons));
		}

		void setResolution(uint16_t _width, uint16_t _height)
		{
			m_width = _width;
			m_height = _height;
		}

		void setPos(int32_t _mx, int32_t _my, int32_t _mz)
		{
			m_absolute[0] = _mx;
			m_absolute[1] = _my;
			m_absolute[2] = _mz;
			m_norm[0] = float(_mx) / float(m_width);
			m_norm[1] = float(_my) / float(m_height);
			m_norm[2] = float(_mz) / float(m_wheelDelta);
		}

		void setButtonState(MouseButton::Enum _button, uint8_t _state)
		{
			m_buttons[_button] = _state;
		}

		int32_t m_absolute[3];
		float m_norm[3];
		int32_t m_wheel;
		uint8_t m_buttons[MouseButton::Count];
		uint16_t m_width;
		uint16_t m_height;
		uint16_t m_wheelDelta;
		bool m_lock;
	};

	struct InputKeyboard
	{
		InputKeyboard()
			: m_ring(BX_COUNTOF(m_char) - 4)
		{
		}

		void reset()
		{
			bx::memSet(m_key, 0, sizeof(m_key));
			bx::memSet(m_once, 0xff, sizeof(m_once));
		}

		static uint32_t encodeKeyState(uint8_t _modifiers, bool _down)
		{
			uint32_t state = 0;
			state |= uint32_t(_down ? _modifiers : 0) << 16;
			state |= uint32_t(_down) << 8;
			return state;
		}

		static bool decodeKeyState(uint32_t _state, uint8_t& _modifiers)
		{
			_modifiers = (_state >> 16) & 0xff;
			return 0 != ((_state >> 8) & 0xff);
		}

		void setKeyState(Key::Enum _key, uint8_t _modifiers, bool _down)
		{
			m_key[_key] = encodeKeyState(_modifiers, _down);
			m_once[_key] = false;
		}

		bool getKeyState(Key::Enum _key, uint8_t* _modifiers)
		{
			uint8_t modifiers;
			_modifiers = NULL == _modifiers ? &modifiers : _modifiers;

			return decodeKeyState(m_key[_key], *_modifiers);
		}

		uint8_t getModifiersState()
		{
			uint8_t modifiers = 0;
			for (uint32_t ii = 0; ii < Key::Count; ++ii)
			{
				modifiers |= (m_key[ii] >> 16) & 0xff;
			}
			return modifiers;
		}

		void pushChar(uint8_t _len, const uint8_t _char[4])
		{
			for (uint32_t len = m_ring.reserve(4)
				; len < _len
				; len = m_ring.reserve(4)
				)
			{
				popChar();
			}

			bx::memCopy(&m_char[m_ring.m_current], _char, 4);
			m_ring.commit(4);
		}

		const uint8_t* popChar()
		{
			if (0 < m_ring.available())
			{
				uint8_t* utf8 = &m_char[m_ring.m_read];
				m_ring.consume(4);
				return utf8;
			}

			return NULL;
		}

		void charFlush()
		{
			m_ring.m_current = 0;
			m_ring.m_write = 0;
			m_ring.m_read = 0;
		}

		uint32_t m_key[256];
		bool m_once[256];

		bx::RingBufferControl m_ring;
		uint8_t m_char[256];
	};

	struct Gamepad
	{
		Gamepad()
		{
			reset();
		}

		void reset()
		{
			bx::memSet(m_axis, 0, sizeof(m_axis));
		}

		void setAxis(GamepadAxis::Enum _axis, int32_t _value)
		{
			m_axis[_axis] = _value;
		}

		int32_t getAxis(GamepadAxis::Enum _axis)
		{
			return m_axis[_axis];
		}

		int32_t m_axis[GamepadAxis::Count];
	};

#if MAX_CONFIG_DEBUG
#	define MAX_API_FUNC(_func) BX_NO_INLINE _func
#else
#	define MAX_API_FUNC(_func) _func
#endif // MAX_CONFIG_DEBUG

	struct Context
	{
		static constexpr uint32_t kAlignment = 64;

		Context()
			: m_render(&m_frame[0])
			, m_submit(&m_frame[MAX_CONFIG_MULTITHREADED ? 1 : 0])
			, m_numFreeDynamicIndexBufferHandles(0)
			, m_numFreeDynamicVertexBufferHandles(0)
			, m_numFreeBodyHandles(0)
			, m_numFreeOcclusionQueryHandles(0)
			, m_colorPaletteDirty(0)
			, m_frames(0)
			, m_debug(MAX_DEBUG_NONE)
			, m_rtMemoryUsed(0)
			, m_textureMemoryUsed(0)
			, m_renderCtx(NULL)
			, m_physicsCtx(NULL)
			, m_headless(false)
			, m_rendererInitialized(false)
			, m_exit(false)
			, m_flipAfterRender(false)
			, m_singleThreaded(false)
		{
		}

		~Context()
		{
		}

#if BX_CONFIG_SUPPORTS_THREADING
		static int32_t renderThread(bx::Thread* /*_self*/, void* /*_userData*/)
		{
			BX_TRACE("render thread start");
			MAX_PROFILER_SET_CURRENT_THREAD_NAME("max - Render Thread");
			while (RenderFrame::Exiting != max::renderFrame() ) {};
			BX_TRACE("render thread exit");
			return bx::kExitSuccess;
		}
#endif

		// game thread
		bool init(const Init& _init);
		void shutdown();

		CommandBuffer& getCommandBuffer(CommandBuffer::Enum _cmd)
		{
			CommandBuffer& cmdbuf = _cmd < CommandBuffer::End ? m_submit->m_cmdPre : m_submit->m_cmdPost;
			uint8_t cmd = (uint8_t)_cmd;
			cmdbuf.write(cmd);
			return cmdbuf;
		}

		MAX_API_FUNC(void reset(uint32_t _width, uint32_t _height, uint32_t _flags, TextureFormat::Enum _format) )
		{
			MAX_MUTEX_SCOPE(m_resourceApiLock);

			BX_ASSERT(false
				|| !m_headless
				|| 0 == _width
				|| 0 == _height
				, "Running in headless mode, resolution of non-existing backbuffer can't be larger than 0x0!"
				);

			const TextureFormat::Enum format = TextureFormat::Count != _format ? _format : m_init.resolution.format;

			if (!g_platformDataChangedSinceReset
			&&  m_init.resolution.format == format
			&&  m_init.resolution.width  == _width
			&&  m_init.resolution.height == _height
			&&  m_init.resolution.reset  == _flags)
			{
				// Nothing changed, ignore request.
				return;
			}

			const uint32_t maskFlags = ~(0
				| (0 != (g_caps.supported & MAX_CAPS_TRANSPARENT_BACKBUFFER) ? 0 : MAX_RESET_TRANSPARENT_BACKBUFFER)
				| (0 != (g_caps.supported & MAX_CAPS_HDR10)                  ? 0 : MAX_RESET_HDR10)
				| (0 != (g_caps.supported & MAX_CAPS_HIDPI)                  ? 0 : MAX_RESET_HIDPI)
				);
			const uint32_t oldFlags = _flags;
			_flags &= maskFlags;

#define WARN_RESET_CAPS_FLAGS(_name) \
	BX_WARN( (oldFlags&(MAX_RESET_##_name) ) == (_flags&(MAX_RESET_##_name) ) \
		, "Reset flag `MAX_RESET_" #_name "` will be ignored, because `MAX_CAPS_" #_name "` is not supported." \
		)
			WARN_RESET_CAPS_FLAGS(TRANSPARENT_BACKBUFFER);
			WARN_RESET_CAPS_FLAGS(HDR10);
			WARN_RESET_CAPS_FLAGS(HIDPI);

#undef WARN_RESET_CAPS_FLAGS
			BX_UNUSED(oldFlags);

			BX_WARN(g_caps.limits.maxTextureSize >= _width
				&&  g_caps.limits.maxTextureSize >= _height
				, "Frame buffer resolution width or height can't be larger than limits.maxTextureSize %d (width %d, height %d)."
				, g_caps.limits.maxTextureSize
				, _width
				, _height
				);
			m_init.resolution.format = format;
			m_init.resolution.width  = bx::clamp(_width,  1u, g_caps.limits.maxTextureSize);
			m_init.resolution.height = bx::clamp(_height, 1u, g_caps.limits.maxTextureSize);
			m_init.resolution.reset  = 0
				| _flags
				| (g_platformDataChangedSinceReset ? MAX_RESET_INTERNAL_FORCE : 0)
				;
			dump(m_init.resolution);
			g_platformDataChangedSinceReset = false;

			m_flipAfterRender = !!(_flags & MAX_RESET_FLIP_AFTER_RENDER);

			for (uint32_t ii = 0; ii < MAX_CONFIG_MAX_VIEWS; ++ii)
			{
				m_view[ii].setFrameBuffer(MAX_INVALID_HANDLE);
			}

			for (uint16_t ii = 0, num = m_textureHandle.getNumHandles(); ii < num; ++ii)
			{
				uint16_t textureIdx = m_textureHandle.getHandleAt(ii);
				const TextureRef& ref = m_textureRef[textureIdx];
				if (BackbufferRatio::Count != ref.m_bbRatio)
				{
					TextureHandle handle = { textureIdx };
					resizeTexture(handle
						, uint16_t(m_init.resolution.width)
						, uint16_t(m_init.resolution.height)
						, ref.m_numMips
						, ref.m_numLayers
						);
					m_init.resolution.reset |= MAX_RESET_INTERNAL_FORCE;
				}
			}
		}

		MAX_API_FUNC(void setDebug(uint32_t _debug) )
		{
			m_debug = _debug;
		}

		MAX_API_FUNC(void dbgTextClear(uint8_t _attr, bool _small) )
		{
			MAX_MUTEX_SCOPE(m_resourceApiLock);

			const uint8_t debugTextScale = m_init.resolution.debugTextScale;
			m_submit->m_textVideoMem->resize(
				  _small
				, (uint16_t)m_init.resolution.width  / debugTextScale
				, (uint16_t)m_init.resolution.height / debugTextScale
				);
			m_submit->m_textVideoMem->clear(_attr);
		}

		MAX_API_FUNC(void dbgTextPrintfVargs(uint16_t _x, uint16_t _y, uint8_t _attr, const char* _format, va_list _argList) )
		{
			MAX_MUTEX_SCOPE(m_resourceApiLock);

			m_submit->m_textVideoMem->printfVargs(_x, _y, _attr, _format, _argList);
		}

		MAX_API_FUNC(void dbgTextImage(uint16_t _x, uint16_t _y, uint16_t _width, uint16_t _height, const void* _data, uint16_t _pitch) )
		{
			MAX_MUTEX_SCOPE(m_resourceApiLock);

			m_submit->m_textVideoMem->image(_x, _y, _width, _height, _data, _pitch);
		}

		MAX_API_FUNC(const Stats* getPerfStats() )
		{
			MAX_MUTEX_SCOPE(m_resourceApiLock);

			Stats& stats = m_submit->m_perfStats;
			const Resolution& resolution = m_submit->m_resolution;
			stats.width  = uint16_t(resolution.width);
			stats.height = uint16_t(resolution.height);
			const TextVideoMem* tvm = m_submit->m_textVideoMem;
			stats.textWidth  = tvm->m_width;
			stats.textHeight = tvm->m_height;
			stats.encoderStats = m_encoderStats;

			stats.numDynamicIndexBuffers  = m_dynamicIndexBufferHandle.getNumHandles();
			stats.numDynamicVertexBuffers = m_dynamicVertexBufferHandle.getNumHandles();
			stats.numFrameBuffers         = m_frameBufferHandle.getNumHandles();
			stats.numIndexBuffers         = m_indexBufferHandle.getNumHandles();
			stats.numOcclusionQueries     = m_occlusionQueryHandle.getNumHandles();
			stats.numPrograms             = m_programHandle.getNumHandles();
			stats.numShaders              = m_shaderHandle.getNumHandles();
			stats.numTextures             = m_textureHandle.getNumHandles();
			stats.numUniforms             = m_uniformHandle.getNumHandles();
			stats.numVertexBuffers        = m_vertexBufferHandle.getNumHandles();
			stats.numVertexLayouts        = m_layoutHandle.getNumHandles();

			stats.textureMemoryUsed = m_textureMemoryUsed;
			stats.rtMemoryUsed      = m_rtMemoryUsed;

			return &stats;
		}

		void resetInput()
		{
			m_mouse.reset();
			m_keyboard.reset();
			for (uint32_t ii = 0; ii < BX_COUNTOF(m_gamepad); ++ii)
			{
				m_gamepad[ii].reset();
			}
		}

		void processInput(const InputBinding* _bindings)
		{
			for (const InputBinding* binding = _bindings; binding->m_key != Key::None; ++binding)
			{
				uint8_t modifiers;
				bool down = InputKeyboard::decodeKeyState(m_keyboard.m_key[binding->m_key], modifiers);

				if (binding->m_flags == 1)
				{
					if (down)
					{
						if (modifiers == binding->m_modifiers
							&& !m_keyboard.m_once[binding->m_key])
						{
							if (NULL == binding->m_fn)
							{
								cmdExec((const char*)binding->m_userData);
							}
							else
							{
								binding->m_fn(binding->m_userData);
							}
							m_keyboard.m_once[binding->m_key] = true;
						}
					}
					else
					{
						m_keyboard.m_once[binding->m_key] = false;
					}
				}
				else
				{
					if (down
						&& modifiers == binding->m_modifiers)
					{
						if (NULL == binding->m_fn)
						{
							cmdExec((const char*)binding->m_userData);
						}
						else
						{
							binding->m_fn(binding->m_userData);
						}
					}
				}
			}
		}

		MAX_API_FUNC(void processInput())
		{
			for (InputBindingMap::const_iterator it = m_inputBindingsMap.begin(); it != m_inputBindingsMap.end(); ++it)
			{
				processInput(it->second);
			}
		}

		MAX_API_FUNC(void addBindings(const char* _name, const InputBinding* _bindings))
		{
			m_inputBindingsMap.insert(stl::make_pair(stl::string(_name), _bindings));
		}

		MAX_API_FUNC(void removeBindings(const char* _name))
		{
			InputBindingMap::iterator it = m_inputBindingsMap.find(stl::string(_name));
			if (it != m_inputBindingsMap.end())
			{
				m_inputBindingsMap.erase(it);
			}
		}

		MAX_API_FUNC(void addMappings(uint32_t _id, const InputMapping* _mappings))
		{
			m_inputMappingsMap.insert(stl::make_pair(_id, _mappings));
		}

		MAX_API_FUNC(void removeMappings(uint32_t _id))
		{
			InputMappingMap::iterator it = m_inputMappingsMap.find(_id);
			if (it != m_inputMappingsMap.end())
			{
				m_inputMappingsMap.erase(it);
			}
		}

		MAX_API_FUNC(float getValue(uint32_t _id, uint32_t _action))
		{
			InputMappingMap::iterator it = m_inputMappingsMap.find(_id);
			if (it != m_inputMappingsMap.end())
			{
				for (const InputMapping* mapping = it->second; mapping->m_action != UINT32_MAX; ++mapping)
				{
					if (mapping->m_action == _action)
					{
						return mapping->m_fn(mapping->m_userData);
					}
				}
			}

			return 0.0f;
		}

		MAX_API_FUNC(IndexBufferHandle createIndexBuffer(const Memory* _mem, uint16_t _flags) )
		{
			MAX_MUTEX_SCOPE(m_resourceApiLock);

			IndexBufferHandle handle = { m_indexBufferHandle.alloc() };

			BX_WARN(isValid(handle), "Failed to allocate index buffer handle.");
			if (isValid(handle) )
			{
				IndexBuffer& ib = m_indexBuffers[handle.idx];
				ib.m_size  = _mem->size;
				ib.m_flags = _flags;

				CommandBuffer& cmdbuf = getCommandBuffer(CommandBuffer::CreateIndexBuffer);
				cmdbuf.write(handle);
				cmdbuf.write(_mem);
				cmdbuf.write(_flags);

				setDebugNameForHandle(handle);
			}
			else
			{
				release(_mem);
			}

			return handle;
		}

		MAX_API_FUNC(void setName(IndexBufferHandle _handle, const bx::StringView& _name) )
		{
			MAX_MUTEX_SCOPE(m_resourceApiLock);

			MAX_CHECK_HANDLE("setName", m_indexBufferHandle, _handle);

			IndexBuffer& ref = m_indexBuffers[_handle.idx];
			ref.m_name.set(_name);

			setNameForHandle(_handle, _name);
		}

		MAX_API_FUNC(void destroyIndexBuffer(IndexBufferHandle _handle) )
		{
			MAX_MUTEX_SCOPE(m_resourceApiLock);

			MAX_CHECK_HANDLE("destroyIndexBuffer", m_indexBufferHandle, _handle);
			bool ok = m_submit->free(_handle); BX_UNUSED(ok);
			BX_ASSERT(ok, "Index buffer handle %d is already destroyed!", _handle.idx);

			IndexBuffer& ref = m_indexBuffers[_handle.idx];
			ref.m_name.clear();

			CommandBuffer& cmdbuf = getCommandBuffer(CommandBuffer::DestroyIndexBuffer);
			cmdbuf.write(_handle);
		}

		VertexLayoutHandle findOrCreateVertexLayout(const VertexLayout& _layout, bool _refCountOnCreation = false)
		{
			VertexLayoutHandle layoutHandle = m_vertexLayoutRef.find(_layout.m_hash);

			if (isValid(layoutHandle) )
			{
				return layoutHandle;
			}

			layoutHandle = { m_layoutHandle.alloc() };
			if (!isValid(layoutHandle) )
			{
				BX_TRACE("WARNING: Failed to allocate vertex layout handle (MAX_CONFIG_MAX_VERTEX_LAYOUTS, max: %d).", MAX_CONFIG_MAX_VERTEX_LAYOUTS);
				return MAX_INVALID_HANDLE;
			}

			CommandBuffer& cmdbuf = getCommandBuffer(CommandBuffer::CreateVertexLayout);
			cmdbuf.write(layoutHandle);
			cmdbuf.write(_layout);

			if (_refCountOnCreation)
			{
				m_vertexLayoutRef.add(layoutHandle, _layout.m_hash);
			}

			return layoutHandle;
		}

		MAX_API_FUNC(VertexLayoutHandle createVertexLayout(const VertexLayout& _layout) )
		{
			MAX_MUTEX_SCOPE(m_resourceApiLock);

			VertexLayoutHandle handle = findOrCreateVertexLayout(_layout);
			if (!isValid(handle) )
			{
				return MAX_INVALID_HANDLE;
			}

			m_vertexLayoutRef.add(handle, _layout.m_hash);

			return handle;
		}

		MAX_API_FUNC(void destroyVertexLayout(VertexLayoutHandle _handle) )
		{
			MAX_MUTEX_SCOPE(m_resourceApiLock);
			if (isValid(m_vertexLayoutRef.release(_handle) ) )
			{
				m_submit->free(_handle);
			}
		}

		MAX_API_FUNC(VertexBufferHandle createVertexBuffer(const Memory* _mem, const VertexLayout& _layout, uint16_t _flags) )
		{
			MAX_MUTEX_SCOPE(m_resourceApiLock);

			VertexBufferHandle handle = { m_vertexBufferHandle.alloc() };

			if (isValid(handle) )
			{
				VertexLayoutHandle layoutHandle = findOrCreateVertexLayout(_layout);
				if (!isValid(layoutHandle) )
				{
					BX_TRACE("WARNING: Failed to allocate vertex layout handle (MAX_CONFIG_MAX_VERTEX_LAYOUTS, max: %d).", MAX_CONFIG_MAX_VERTEX_LAYOUTS);
					m_vertexBufferHandle.free(handle.idx);
					return MAX_INVALID_HANDLE;
				}

				m_vertexLayoutRef.add(handle, layoutHandle, _layout.m_hash);

				VertexBuffer& vb = m_vertexBuffers[handle.idx];
				vb.m_size   = _mem->size;
				vb.m_stride = _layout.m_stride;

				CommandBuffer& cmdbuf = getCommandBuffer(CommandBuffer::CreateVertexBuffer);
				cmdbuf.write(handle);
				cmdbuf.write(_mem);
				cmdbuf.write(layoutHandle);
				cmdbuf.write(_flags);

				setDebugNameForHandle(handle);

				return handle;
			}

			BX_TRACE("WARNING: Failed to allocate vertex buffer handle (MAX_CONFIG_MAX_VERTEX_BUFFERS, max: %d).", MAX_CONFIG_MAX_VERTEX_BUFFERS);
			release(_mem);

			return MAX_INVALID_HANDLE;
		}

		MAX_API_FUNC(void setName(VertexBufferHandle _handle, const bx::StringView& _name) )
		{
			MAX_MUTEX_SCOPE(m_resourceApiLock);

			MAX_CHECK_HANDLE("setName", m_vertexBufferHandle, _handle);

			VertexBuffer& ref = m_vertexBuffers[_handle.idx];
			ref.m_name.set(_name);

			setNameForHandle(_handle, _name);
		}

		MAX_API_FUNC(void destroyVertexBuffer(VertexBufferHandle _handle) )
		{
			MAX_MUTEX_SCOPE(m_resourceApiLock);

			MAX_CHECK_HANDLE("destroyVertexBuffer", m_vertexBufferHandle, _handle);
			bool ok = m_submit->free(_handle); BX_UNUSED(ok);
			BX_ASSERT(ok, "Vertex buffer handle %d is already destroyed!", _handle.idx);

			VertexBuffer& ref = m_vertexBuffers[_handle.idx];
			ref.m_name.clear();

			CommandBuffer& cmdbuf = getCommandBuffer(CommandBuffer::DestroyVertexBuffer);
			cmdbuf.write(_handle);
		}

		void destroyVertexBufferInternal(VertexBufferHandle _handle)
		{
			VertexLayoutHandle layoutHandle = m_vertexLayoutRef.release(_handle);
			if (isValid(layoutHandle) )
			{
				CommandBuffer& cmdbuf = getCommandBuffer(CommandBuffer::DestroyVertexLayout);
				cmdbuf.write(layoutHandle);
				m_render->free(layoutHandle);
			}

			m_vertexBufferHandle.free(_handle.idx);
		}

		uint64_t allocDynamicIndexBuffer(uint32_t _size, uint16_t _flags)
		{
			uint64_t ptr = m_dynIndexBufferAllocator.alloc(_size);
			if (ptr == NonLocalAllocator::kInvalidBlock)
			{
				IndexBufferHandle indexBufferHandle = { m_indexBufferHandle.alloc() };
				if (!isValid(indexBufferHandle) )
				{
					BX_TRACE("Failed to allocate index buffer handle.");
					return NonLocalAllocator::kInvalidBlock;
				}

				const uint32_t allocSize = bx::max<uint32_t>(MAX_CONFIG_DYNAMIC_INDEX_BUFFER_SIZE, bx::alignUp(_size, 1<<20) );

				IndexBuffer& ib = m_indexBuffers[indexBufferHandle.idx];
				ib.m_size = allocSize;

				CommandBuffer& cmdbuf = getCommandBuffer(CommandBuffer::CreateDynamicIndexBuffer);
				cmdbuf.write(indexBufferHandle);
				cmdbuf.write(allocSize);
				cmdbuf.write(_flags);

				m_dynIndexBufferAllocator.add(uint64_t(indexBufferHandle.idx) << 32, allocSize);
				ptr = m_dynIndexBufferAllocator.alloc(_size);
			}

			return ptr;
		}

		uint64_t allocIndexBuffer(uint32_t _size, uint16_t _flags)
		{
			IndexBufferHandle indexBufferHandle = { m_indexBufferHandle.alloc() };
			if (!isValid(indexBufferHandle) )
			{
				BX_TRACE("Failed to allocate index buffer handle.");
				return NonLocalAllocator::kInvalidBlock;
			}

			IndexBuffer& ib = m_indexBuffers[indexBufferHandle.idx];
			ib.m_size = _size;

			CommandBuffer& cmdbuf = getCommandBuffer(CommandBuffer::CreateDynamicIndexBuffer);
			cmdbuf.write(indexBufferHandle);
			cmdbuf.write(_size);
			cmdbuf.write(_flags);

			setDebugNameForHandle(indexBufferHandle, "Dynamic Index Buffer");

			return uint64_t(indexBufferHandle.idx) << 32;
		}

		MAX_API_FUNC(DynamicIndexBufferHandle createDynamicIndexBuffer(uint32_t _num, uint16_t _flags) )
		{
			MAX_MUTEX_SCOPE(m_resourceApiLock);

			DynamicIndexBufferHandle handle = { m_dynamicIndexBufferHandle.alloc() };
			if (!isValid(handle) )
			{
				BX_TRACE("Failed to allocate dynamic index buffer handle.");
				return handle;
			}

			const uint32_t indexSize = 0 == (_flags & MAX_BUFFER_INDEX32) ? 2 : 4;
			const uint32_t size = bx::alignUp(_num*indexSize, 16);

			const uint64_t ptr = (0 != (_flags & MAX_BUFFER_COMPUTE_READ_WRITE) )
				? allocIndexBuffer(size, _flags)
				: allocDynamicIndexBuffer(size, _flags)
				;

			if (ptr == NonLocalAllocator::kInvalidBlock)
			{
				m_dynamicIndexBufferHandle.free(handle.idx);
				return MAX_INVALID_HANDLE;
			}

			DynamicIndexBuffer& dib = m_dynamicIndexBuffers[handle.idx];
			dib.m_handle.idx = uint16_t(ptr>>32);
			dib.m_offset     = uint32_t(ptr);
			dib.m_size       = _num * indexSize;
			dib.m_startIndex = bx::strideAlign(dib.m_offset, indexSize)/indexSize;
			dib.m_flags      = _flags;

			return handle;
		}

		MAX_API_FUNC(DynamicIndexBufferHandle createDynamicIndexBuffer(const Memory* _mem, uint16_t _flags) )
		{
			MAX_MUTEX_SCOPE(m_resourceApiLock);

			BX_ASSERT(0 == (_flags & MAX_BUFFER_COMPUTE_WRITE), "Can't initialize compute write buffer from CPU.");

			const uint32_t indexSize = 0 == (_flags & MAX_BUFFER_INDEX32) ? 2 : 4;
			DynamicIndexBufferHandle handle = createDynamicIndexBuffer(_mem->size/indexSize, _flags);

			if (!isValid(handle) )
			{
				release(_mem);
				return MAX_INVALID_HANDLE;
			}

			update(handle, 0, _mem);

			return handle;
		}

		MAX_API_FUNC(void update(DynamicIndexBufferHandle _handle, uint32_t _startIndex, const Memory* _mem) )
		{
			MAX_MUTEX_SCOPE(m_resourceApiLock);

			MAX_CHECK_HANDLE("updateDynamicIndexBuffer", m_dynamicIndexBufferHandle, _handle);

			DynamicIndexBuffer& dib = m_dynamicIndexBuffers[_handle.idx];
			BX_ASSERT(0 == (dib.m_flags & MAX_BUFFER_COMPUTE_WRITE), "Can't update GPU write buffer from CPU.");
			const uint32_t indexSize = 0 == (dib.m_flags & MAX_BUFFER_INDEX32) ? 2 : 4;

			if (dib.m_size < _mem->size
			&&  0 != (dib.m_flags & MAX_BUFFER_ALLOW_RESIZE) )
			{
				destroy(dib);

				const uint64_t ptr = (0 != (dib.m_flags & MAX_BUFFER_COMPUTE_READ) )
					? allocIndexBuffer(_mem->size, dib.m_flags)
					: allocDynamicIndexBuffer(_mem->size, dib.m_flags)
					;

				dib.m_handle.idx = uint16_t(ptr>>32);
				dib.m_offset     = uint32_t(ptr);
				dib.m_size       = _mem->size;
				dib.m_startIndex = bx::strideAlign(dib.m_offset, indexSize)/indexSize;
			}

			const uint32_t offset = (dib.m_startIndex + _startIndex)*indexSize;
			const uint32_t size   = bx::min<uint32_t>(offset
				+ bx::min(bx::uint32_satsub(dib.m_size, _startIndex*indexSize), _mem->size)
				, m_indexBuffers[dib.m_handle.idx].m_size) - offset
				;
			BX_ASSERT(_mem->size <= size, "Truncating dynamic index buffer update (size %d, mem size %d)."
				, size
				, _mem->size
				);
			CommandBuffer& cmdbuf = getCommandBuffer(CommandBuffer::UpdateDynamicIndexBuffer);
			cmdbuf.write(dib.m_handle);
			cmdbuf.write(offset);
			cmdbuf.write(size);
			cmdbuf.write(_mem);
		}

		MAX_API_FUNC(void destroyDynamicIndexBuffer(DynamicIndexBufferHandle _handle) )
		{
			MAX_MUTEX_SCOPE(m_resourceApiLock);

			MAX_CHECK_HANDLE("destroyDynamicIndexBuffer", m_dynamicIndexBufferHandle, _handle);

			m_freeDynamicIndexBufferHandle[m_numFreeDynamicIndexBufferHandles++] = _handle;
		}

		void destroy(const DynamicIndexBuffer& _dib)
		{
			if (0 != (_dib.m_flags & MAX_BUFFER_COMPUTE_READ_WRITE) )
			{
				destroyIndexBuffer(_dib.m_handle);
			}
			else
			{
				m_dynIndexBufferAllocator.free(uint64_t(_dib.m_handle.idx) << 32 | _dib.m_offset);
				if (m_dynIndexBufferAllocator.compact() )
				{
					for (uint64_t ptr = m_dynIndexBufferAllocator.remove(); 0 != ptr; ptr = m_dynIndexBufferAllocator.remove() )
					{
						IndexBufferHandle handle = { uint16_t(ptr >> 32) };
						destroyIndexBuffer(handle);
					}
				}
			}
		}

		void destroyDynamicIndexBufferInternal(DynamicIndexBufferHandle _handle)
		{
			DynamicIndexBuffer& dib = m_dynamicIndexBuffers[_handle.idx];
			destroy(dib);
			dib.reset();
			m_dynamicIndexBufferHandle.free(_handle.idx);
		}

		uint64_t allocDynamicVertexBuffer(uint32_t _size, uint16_t _flags)
		{
			uint64_t ptr = m_dynVertexBufferAllocator.alloc(_size);
			if (ptr == NonLocalAllocator::kInvalidBlock)
			{
				VertexBufferHandle vertexBufferHandle = { m_vertexBufferHandle.alloc() };
				if (!isValid(vertexBufferHandle) )
				{
					BX_TRACE("Failed to allocate dynamic vertex buffer handle.");
					return NonLocalAllocator::kInvalidBlock;
				}

				const uint32_t allocSize = bx::max<uint32_t>(MAX_CONFIG_DYNAMIC_VERTEX_BUFFER_SIZE, bx::alignUp(_size, 1<<20) );

				VertexBuffer& vb = m_vertexBuffers[vertexBufferHandle.idx];
				vb.m_size   = allocSize;
				vb.m_stride = 0;

				CommandBuffer& cmdbuf = getCommandBuffer(CommandBuffer::CreateDynamicVertexBuffer);
				cmdbuf.write(vertexBufferHandle);
				cmdbuf.write(allocSize);
				cmdbuf.write(_flags);

				m_dynVertexBufferAllocator.add(uint64_t(vertexBufferHandle.idx) << 32, allocSize);
				ptr = m_dynVertexBufferAllocator.alloc(_size);
			}

			return ptr;
		}

		uint64_t allocVertexBuffer(uint32_t _size, uint16_t _flags)
		{
			VertexBufferHandle vertexBufferHandle = { m_vertexBufferHandle.alloc() };

			if (!isValid(vertexBufferHandle) )
			{
				BX_TRACE("WARNING: Failed to allocate vertex buffer handle (MAX_CONFIG_MAX_VERTEX_BUFFERS, max: %d).", MAX_CONFIG_MAX_VERTEX_BUFFERS);
				return NonLocalAllocator::kInvalidBlock;
			}

			VertexBuffer& vb = m_vertexBuffers[vertexBufferHandle.idx];
			vb.m_size   = _size;
			vb.m_stride = 0;

			CommandBuffer& cmdbuf = getCommandBuffer(CommandBuffer::CreateDynamicVertexBuffer);
			cmdbuf.write(vertexBufferHandle);
			cmdbuf.write(_size);
			cmdbuf.write(_flags);

			setDebugNameForHandle(vertexBufferHandle, "Dynamic Vertex Buffer");

			return uint64_t(vertexBufferHandle.idx)<<32;
		}

		MAX_API_FUNC(DynamicVertexBufferHandle createDynamicVertexBuffer(uint32_t _num, const VertexLayout& _layout, uint16_t _flags) )
		{
			MAX_MUTEX_SCOPE(m_resourceApiLock);

			VertexLayoutHandle layoutHandle = findOrCreateVertexLayout(_layout);
			if (!isValid(layoutHandle) )
			{
				BX_TRACE("WARNING: Failed to allocate vertex layout handle (MAX_CONFIG_MAX_VERTEX_LAYOUTS, max: %d).", MAX_CONFIG_MAX_VERTEX_LAYOUTS);
				return MAX_INVALID_HANDLE;
			}

			DynamicVertexBufferHandle handle = { m_dynamicVertexBufferHandle.alloc() };
			if (!isValid(handle) )
			{
				BX_TRACE("WARNING: Failed to allocate dynamic vertex buffer handle (MAX_CONFIG_MAX_DYNAMIC_VERTEX_BUFFERS, max: %d).", MAX_CONFIG_MAX_DYNAMIC_VERTEX_BUFFERS);
				return MAX_INVALID_HANDLE;
			}

			const uint32_t size = bx::strideAlign<16>(_num*_layout.m_stride, _layout.m_stride)+_layout.m_stride;

			const uint64_t ptr = (0 != (_flags & MAX_BUFFER_COMPUTE_READ_WRITE) )
				? allocVertexBuffer(size, _flags)
				: allocDynamicVertexBuffer(size, _flags)
				;

			if (ptr == NonLocalAllocator::kInvalidBlock)
			{
				m_dynamicVertexBufferHandle.free(handle.idx);
				return MAX_INVALID_HANDLE;
			}

			DynamicVertexBuffer& dvb = m_dynamicVertexBuffers[handle.idx];
			dvb.m_handle.idx    = uint16_t(ptr>>32);
			dvb.m_offset        = uint32_t(ptr);
			dvb.m_size          = _num * _layout.m_stride;
			dvb.m_startVertex   = bx::strideAlign(dvb.m_offset, _layout.m_stride)/_layout.m_stride;
			dvb.m_numVertices   = _num;
			dvb.m_stride        = _layout.m_stride;
			dvb.m_layoutHandle  = layoutHandle;
			dvb.m_flags         = _flags;
			m_vertexLayoutRef.add(handle, layoutHandle, _layout.m_hash);

			return handle;
		}

		MAX_API_FUNC(DynamicVertexBufferHandle createDynamicVertexBuffer(const Memory* _mem, const VertexLayout& _layout, uint16_t _flags) )
		{
			MAX_MUTEX_SCOPE(m_resourceApiLock);

			BX_ASSERT(0 == (_flags & MAX_BUFFER_COMPUTE_WRITE), "Can't initialize compute write buffer from CPU.");

			uint32_t numVertices = _mem->size/_layout.m_stride;
			DynamicVertexBufferHandle handle = createDynamicVertexBuffer(numVertices, _layout, _flags);

			if (!isValid(handle) )
			{
				release(_mem);
				return MAX_INVALID_HANDLE;
			}

			update(handle, 0, _mem);

			return handle;
		}

		MAX_API_FUNC(void update(DynamicVertexBufferHandle _handle, uint32_t _startVertex, const Memory* _mem) )
		{
			MAX_MUTEX_SCOPE(m_resourceApiLock);

			MAX_CHECK_HANDLE("updateDynamicVertexBuffer", m_dynamicVertexBufferHandle, _handle);

			DynamicVertexBuffer& dvb = m_dynamicVertexBuffers[_handle.idx];
			BX_ASSERT(0 == (dvb.m_flags & MAX_BUFFER_COMPUTE_WRITE), "Can't update GPU write buffer from CPU.");

			if (dvb.m_size < _mem->size
			&&  0 != (dvb.m_flags & MAX_BUFFER_ALLOW_RESIZE) )
			{
				destroy(dvb);

				const uint32_t size = bx::strideAlign<16>(_mem->size, dvb.m_stride)+dvb.m_stride;

				const uint64_t ptr = (0 != (dvb.m_flags & MAX_BUFFER_COMPUTE_READ) )
					? allocVertexBuffer(size, dvb.m_flags)
					: allocDynamicVertexBuffer(size, dvb.m_flags)
					;

				dvb.m_handle.idx  = uint16_t(ptr>>32);
				dvb.m_offset      = uint32_t(ptr);
				dvb.m_size        = size;
				dvb.m_numVertices = _mem->size / dvb.m_stride;
				dvb.m_startVertex = bx::strideAlign(dvb.m_offset, dvb.m_stride)/dvb.m_stride;
			}

			const uint32_t offset = (dvb.m_startVertex + _startVertex)*dvb.m_stride;
			const uint32_t size   = bx::min<uint32_t>(offset
				+ bx::min(bx::uint32_satsub(dvb.m_size, _startVertex*dvb.m_stride), _mem->size)
				, m_vertexBuffers[dvb.m_handle.idx].m_size) - offset
				;
			BX_ASSERT(_mem->size <= size, "Truncating dynamic vertex buffer update (size %d, mem size %d)."
				, size
				, _mem->size
				);

			CommandBuffer& cmdbuf = getCommandBuffer(CommandBuffer::UpdateDynamicVertexBuffer);
			cmdbuf.write(dvb.m_handle);
			cmdbuf.write(offset);
			cmdbuf.write(size);
			cmdbuf.write(_mem);
		}

		MAX_API_FUNC(void destroyDynamicVertexBuffer(DynamicVertexBufferHandle _handle) )
		{
			MAX_MUTEX_SCOPE(m_resourceApiLock);

			MAX_CHECK_HANDLE("destroyDynamicVertexBuffer", m_dynamicVertexBufferHandle, _handle);

			m_freeDynamicVertexBufferHandle[m_numFreeDynamicVertexBufferHandles++] = _handle;
		}

		void destroy(const DynamicVertexBuffer& _dvb)
		{
			if (0 != (_dvb.m_flags & MAX_BUFFER_COMPUTE_READ_WRITE) )
			{
				destroyVertexBuffer(_dvb.m_handle);
			}
			else
			{
				m_dynVertexBufferAllocator.free(uint64_t(_dvb.m_handle.idx) << 32 | _dvb.m_offset);
				if (m_dynVertexBufferAllocator.compact() )
				{
					for (uint64_t ptr = m_dynVertexBufferAllocator.remove(); 0 != ptr; ptr = m_dynVertexBufferAllocator.remove() )
					{
						VertexBufferHandle handle = { uint16_t(ptr >> 32) };
						destroyVertexBuffer(handle);
					}
				}
			}
		}

		void destroyDynamicVertexBufferInternal(DynamicVertexBufferHandle _handle)
		{
			VertexLayoutHandle layoutHandle = m_vertexLayoutRef.release(_handle);
			MAX_CHECK_HANDLE_INVALID_OK("destroyDynamicVertexBufferInternal", m_layoutHandle, layoutHandle);

			if (isValid(layoutHandle) )
			{
				CommandBuffer& cmdbuf = getCommandBuffer(CommandBuffer::DestroyVertexLayout);
				cmdbuf.write(layoutHandle);
				m_render->free(layoutHandle);
			}

			DynamicVertexBuffer& dvb = m_dynamicVertexBuffers[_handle.idx];
			destroy(dvb);
			dvb.reset();
			m_dynamicVertexBufferHandle.free(_handle.idx);
		}

		MAX_API_FUNC(uint32_t getAvailTransientIndexBuffer(uint32_t _num, bool _index32) )
		{
			MAX_MUTEX_SCOPE(m_resourceApiLock);

			const bool isIndex16     = !_index32;
			const uint16_t indexSize = isIndex16 ? 2 : 4;
			return m_submit->getAvailTransientIndexBuffer(_num, indexSize);
		}

		MAX_API_FUNC(uint32_t getAvailTransientVertexBuffer(uint32_t _num, uint16_t _stride) )
		{
			MAX_MUTEX_SCOPE(m_resourceApiLock);

			return m_submit->getAvailTransientVertexBuffer(_num, _stride);
		}

		TransientIndexBuffer* createTransientIndexBuffer(uint32_t _size)
		{
			TransientIndexBuffer* tib = NULL;

			IndexBufferHandle handle = { m_indexBufferHandle.alloc() };
			BX_WARN(isValid(handle), "Failed to allocate transient index buffer handle.");
			if (isValid(handle) )
			{
				CommandBuffer& cmdbuf = getCommandBuffer(CommandBuffer::CreateDynamicIndexBuffer);
				cmdbuf.write(handle);
				cmdbuf.write(_size);
				uint16_t flags = MAX_BUFFER_NONE;
				cmdbuf.write(flags);

				const uint32_t size = 0
					+ bx::alignUp<uint32_t>(sizeof(TransientIndexBuffer), 16)
					+ bx::alignUp(_size, 16)
					;
				tib = (TransientIndexBuffer*)bx::alignedAlloc(g_allocator, size, 16);
				tib->data   = (uint8_t *)tib + bx::alignUp(sizeof(TransientIndexBuffer), 16);
				tib->size   = _size;
				tib->handle = handle;

				setDebugNameForHandle(handle, "Transient Index Buffer");
			}

			return tib;
		}

		void destroyTransientIndexBuffer(TransientIndexBuffer* _tib)
		{
			CommandBuffer& cmdbuf = getCommandBuffer(CommandBuffer::DestroyDynamicIndexBuffer);
			cmdbuf.write(_tib->handle);

			m_submit->free(_tib->handle);
			bx::alignedFree(g_allocator, _tib, 16);
		}

		MAX_API_FUNC(void allocTransientIndexBuffer(TransientIndexBuffer* _tib, uint32_t _num, bool _index32) )
		{
			MAX_MUTEX_SCOPE(m_resourceApiLock);

			const bool isIndex16     = !_index32;
			const uint16_t indexSize = isIndex16 ? 2 : 4;
			const uint32_t offset    = m_submit->allocTransientIndexBuffer(_num, indexSize);

			TransientIndexBuffer& tib = *m_submit->m_transientIb;

			_tib->data       = &tib.data[offset];
			_tib->size       = _num * indexSize;
			_tib->handle     = tib.handle;
			_tib->startIndex = bx::strideAlign(offset, indexSize) / indexSize;
			_tib->isIndex16  = isIndex16;
		}

		TransientVertexBuffer* createTransientVertexBuffer(uint32_t _size, const VertexLayout* _layout = NULL)
		{
			TransientVertexBuffer* tvb = NULL;

			VertexBufferHandle handle = { m_vertexBufferHandle.alloc() };

			BX_WARN(isValid(handle), "Failed to allocate transient vertex buffer handle.");
			if (isValid(handle) )
			{
				uint16_t stride = 0;
				VertexLayoutHandle layoutHandle = MAX_INVALID_HANDLE;

				if (NULL != _layout)
				{
					layoutHandle = findOrCreateVertexLayout(*_layout);
					m_vertexLayoutRef.add(handle, layoutHandle, _layout->m_hash);

					stride = _layout->m_stride;
				}

				CommandBuffer& cmdbuf = getCommandBuffer(CommandBuffer::CreateDynamicVertexBuffer);
				cmdbuf.write(handle);
				cmdbuf.write(_size);
				uint16_t flags = MAX_BUFFER_NONE;
				cmdbuf.write(flags);

				const uint32_t size = 0
					+ bx::alignUp<uint32_t>(sizeof(TransientVertexBuffer), 16)
					+ bx::alignUp(_size, 16)
					;
				tvb = (TransientVertexBuffer*)bx::alignedAlloc(g_allocator, size, 16);
				tvb->data = (uint8_t *)tvb + bx::alignUp(sizeof(TransientVertexBuffer), 16);
				tvb->size = _size;
				tvb->startVertex = 0;
				tvb->stride = stride;
				tvb->handle = handle;
				tvb->layoutHandle = layoutHandle;

				setDebugNameForHandle(handle, "Transient Vertex Buffer");
			}

			return tvb;
		}

		void destroyTransientVertexBuffer(TransientVertexBuffer* _tvb)
		{
			CommandBuffer& cmdbuf = getCommandBuffer(CommandBuffer::DestroyDynamicVertexBuffer);
			cmdbuf.write(_tvb->handle);

			m_submit->free(_tvb->handle);
			bx::alignedFree(g_allocator, _tvb, 16);
		}

		MAX_API_FUNC(void allocTransientVertexBuffer(TransientVertexBuffer* _tvb, uint32_t _num, VertexLayoutHandle _layoutHandle, uint16_t _stride) )
		{
			MAX_MUTEX_SCOPE(m_resourceApiLock);

			const uint32_t offset = m_submit->allocTransientVertexBuffer(_num, _stride);
			const TransientVertexBuffer& dvb = *m_submit->m_transientVb;

			_tvb->data         = &dvb.data[offset];
			_tvb->size         = _num * _stride;
			_tvb->startVertex  = bx::strideAlign(offset, _stride)/_stride;
			_tvb->stride       = _stride;
			_tvb->handle       = dvb.handle;
			_tvb->layoutHandle = _layoutHandle;
		}

		MAX_API_FUNC(void allocInstanceDataBuffer(InstanceDataBuffer* _idb, uint32_t _num, uint16_t _stride) )
		{
			MAX_MUTEX_SCOPE(m_resourceApiLock);

			const uint16_t stride = bx::alignUp(_stride, 16);
			const uint32_t offset = m_submit->allocTransientVertexBuffer(_num, stride);

			TransientVertexBuffer& dvb = *m_submit->m_transientVb;
			_idb->data   = &dvb.data[offset];
			_idb->size   = _num * stride;
			_idb->offset = offset;
			_idb->num    = _num;
			_idb->stride = stride;
			_idb->handle = dvb.handle;
		}

		IndirectBufferHandle createIndirectBuffer(uint32_t _num)
		{
			BX_UNUSED(_num);
			IndirectBufferHandle handle = { m_vertexBufferHandle.alloc() };

			BX_WARN(isValid(handle), "Failed to allocate draw indirect buffer handle.");
			if (isValid(handle) )
			{
				uint32_t size  = _num * MAX_CONFIG_DRAW_INDIRECT_STRIDE;
				uint16_t flags = MAX_BUFFER_DRAW_INDIRECT;

				CommandBuffer& cmdbuf = getCommandBuffer(CommandBuffer::CreateDynamicVertexBuffer);
				cmdbuf.write(handle);
				cmdbuf.write(size);
				cmdbuf.write(flags);
			}

			return handle;
		}

		void destroyIndirectBuffer(IndirectBufferHandle _handle)
		{
			VertexBufferHandle handle = { _handle.idx };
			MAX_CHECK_HANDLE("destroyDrawIndirectBuffer", m_vertexBufferHandle, handle);

			CommandBuffer& cmdbuf = getCommandBuffer(CommandBuffer::DestroyDynamicVertexBuffer);
			cmdbuf.write(handle);
			m_submit->free(handle);
		}

		MAX_API_FUNC(ShaderHandle createShader(const Memory* _mem) )
		{
			MAX_MUTEX_SCOPE(m_resourceApiLock);

			bx::MemoryReader reader(_mem->data, _mem->size);

			bx::Error err;

			uint32_t magic;
			bx::read(&reader, magic, &err);

			if (!err.isOk() )
			{
				BX_TRACE("Couldn't read shader signature!");
				release(_mem);
				return MAX_INVALID_HANDLE;
			}

			if (!isShaderBin(magic) )
			{
				BX_TRACE("Invalid shader signature! %c%c%c%d."
					, ( (uint8_t*)&magic)[0]
					, ( (uint8_t*)&magic)[1]
					, ( (uint8_t*)&magic)[2]
					, ( (uint8_t*)&magic)[3]
					);
				release(_mem);
				return MAX_INVALID_HANDLE;
			}

			if (isShaderType(magic, 'C')
			&&  0 == (g_caps.supported & MAX_CAPS_COMPUTE) )
			{
				BX_TRACE("Creating compute shader but compute is not supported!");
				release(_mem);
				return MAX_INVALID_HANDLE;
			}

			if ( (isShaderType(magic, 'C') && isShaderVerLess(magic, 3) )
			||   (isShaderType(magic, 'F') && isShaderVerLess(magic, 5) )
			||   (isShaderType(magic, 'V') && isShaderVerLess(magic, 5) ) )
			{
				BX_TRACE("Unsupported shader binary version.");
				release(_mem);
				return MAX_INVALID_HANDLE;
			}

			const uint32_t shaderHash = bx::hash<bx::HashMurmur2A>(_mem->data, _mem->size);
			const uint16_t idx = m_shaderHashMap.find(shaderHash);
			if (kInvalidHandle != idx)
			{
				ShaderHandle handle = { idx };
				shaderIncRef(handle);
				release(_mem);
				return handle;
			}

			uint32_t hashIn;
			bx::read(&reader, hashIn, &err);

			uint32_t hashOut;

			if (isShaderVerLess(magic, 6) )
			{
				hashOut = hashIn;
			}
			else
			{
				bx::read(&reader, hashOut, &err);
			}

			uint16_t count;
			bx::read(&reader, count, &err);

			if (!err.isOk() )
			{
				BX_TRACE("Corrupted shader binary!");
				release(_mem);
				return MAX_INVALID_HANDLE;
			}

			ShaderHandle handle = { m_shaderHandle.alloc() };

			if (!isValid(handle) )
			{
				BX_TRACE("Failed to allocate shader handle.");
				release(_mem);
				return MAX_INVALID_HANDLE;
			}

			bool ok = m_shaderHashMap.insert(shaderHash, handle.idx);
			BX_ASSERT(ok, "Shader already exists!"); BX_UNUSED(ok);

			ShaderRef& sr = m_shaderRef[handle.idx];
			sr.m_refCount = 1;
			sr.m_hashIn   = hashIn;
			sr.m_hashOut  = hashOut;
			sr.m_num      = 0;
			sr.m_uniforms = NULL;

			UniformHandle* uniforms = (UniformHandle*)alloca(count*sizeof(UniformHandle) );

			for (uint32_t ii = 0; ii < count; ++ii)
			{
				uint8_t nameSize = 0;
				bx::read(&reader, nameSize, &err);

				char name[256];
				bx::read(&reader, &name, nameSize, &err);
				name[nameSize] = '\0';

				uint8_t type = 0;
				bx::read(&reader, type, &err);
				type &= ~kUniformMask;

				uint8_t num;
				bx::read(&reader, num, &err);

				uint16_t regIndex;
				bx::read(&reader, regIndex, &err);

				uint16_t regCount;
				bx::read(&reader, regCount, &err);

				if (!isShaderVerLess(magic, 8) )
				{
					uint16_t texInfo;
					bx::read(&reader, texInfo, &err);
				}

				if (!isShaderVerLess(magic, 10) )
				{
					uint16_t texFormat = 0;
					bx::read(&reader, texFormat, &err);
				}

				PredefinedUniform::Enum predefined = nameToPredefinedUniformEnum(name);
				if (PredefinedUniform::Count == predefined && UniformType::End != UniformType::Enum(type) )
				{
					uniforms[sr.m_num] = createUniform(name, UniformType::Enum(type), num);
					sr.m_num++;
				}
			}

			if (0 != sr.m_num)
			{
				uint32_t size = sr.m_num*sizeof(UniformHandle);
				sr.m_uniforms = (UniformHandle*)bx::alloc(g_allocator, size);
				bx::memCopy(sr.m_uniforms, uniforms, size);
			}

			CommandBuffer& cmdbuf = getCommandBuffer(CommandBuffer::CreateShader);
			cmdbuf.write(handle);
			cmdbuf.write(_mem);

			setDebugNameForHandle(handle);

			return handle;
		}

		MAX_API_FUNC(uint16_t getShaderUniforms(ShaderHandle _handle, UniformHandle* _uniforms, uint16_t _max) )
		{
			MAX_MUTEX_SCOPE(m_resourceApiLock);

			if (!isValid(_handle) )
			{
				BX_WARN(false, "Passing invalid shader handle to max::getShaderUniforms.");
				return 0;
			}

			ShaderRef& sr = m_shaderRef[_handle.idx];
			if (NULL != _uniforms)
			{
				bx::memCopy(_uniforms, sr.m_uniforms, bx::min<uint16_t>(_max, sr.m_num)*sizeof(UniformHandle) );
			}

			return sr.m_num;
		}

		void setNameForHandle(Handle _handle, const bx::StringView& _name)
		{
			char tmp[1024];
			uint16_t len = 1+(uint16_t)bx::snprintf(tmp, BX_COUNTOF(tmp)
				, "%sH %d: %S"
				, _handle.getTypeName().abrvName
				, _handle.idx
				, &_name
				);

			CommandBuffer& cmdbuf = getCommandBuffer(CommandBuffer::SetName);
			cmdbuf.write(_handle);
			cmdbuf.write(len);
			cmdbuf.write(tmp, len);
		}

		void setDebugNameForHandle(Handle _handle, const bx::StringView& _name = "")
		{
			if (BX_ENABLED(MAX_CONFIG_DEBUG) )
			{
				setNameForHandle(_handle, _name);
			}
		}

		MAX_API_FUNC(void setName(ShaderHandle _handle, const bx::StringView& _name) )
		{
			MAX_MUTEX_SCOPE(m_resourceApiLock);

			MAX_CHECK_HANDLE("setName", m_shaderHandle, _handle);

			ShaderRef& sr = m_shaderRef[_handle.idx];
			sr.m_name.set(_name);

			setNameForHandle(_handle, _name);
		}

		MAX_API_FUNC(void destroyShader(ShaderHandle _handle) )
		{
			MAX_MUTEX_SCOPE(m_resourceApiLock);

			MAX_CHECK_HANDLE("destroyShader", m_shaderHandle, _handle);

			if (!isValid(_handle) )
			{
				BX_WARN(false, "Passing invalid shader handle to max::destroyShader.");
				return;
			}

			shaderDecRef(_handle);
		}

		void shaderTakeOwnership(ShaderHandle _handle)
		{
			shaderDecRef(_handle);
		}

		void shaderIncRef(ShaderHandle _handle)
		{
			ShaderRef& sr = m_shaderRef[_handle.idx];
			++sr.m_refCount;
		}

		void shaderDecRef(ShaderHandle _handle)
		{
			ShaderRef& sr = m_shaderRef[_handle.idx];
			int32_t refs = --sr.m_refCount;
			if (0 == refs)
			{
				bool ok = m_submit->free(_handle); BX_UNUSED(ok);
				BX_ASSERT(ok, "Shader handle %d is already destroyed!", _handle.idx);

				CommandBuffer& cmdbuf = getCommandBuffer(CommandBuffer::DestroyShader);
				cmdbuf.write(_handle);

				if (0 != sr.m_num)
				{
					for (uint32_t ii = 0, num = sr.m_num; ii < num; ++ii)
					{
						destroyUniform(sr.m_uniforms[ii]);
					}

					bx::free(g_allocator, sr.m_uniforms);
					sr.m_uniforms = NULL;
					sr.m_num = 0;
				}

				m_shaderHashMap.removeByHandle(_handle.idx);
			}
		}

		MAX_API_FUNC(ProgramHandle createProgram(ShaderHandle _vsh, ShaderHandle _fsh, bool _destroyShaders) )
		{
			MAX_MUTEX_SCOPE(m_resourceApiLock);

			if (!isValid(_vsh)
			||  !isValid(_fsh) )
			{
				BX_TRACE("Vertex/fragment shader is invalid (vsh %d, fsh %d).", _vsh.idx, _fsh.idx);
				return MAX_INVALID_HANDLE;
			}

			ProgramHandle handle = { m_programHashMap.find(uint32_t(_fsh.idx<<16)|_vsh.idx) };
			if (isValid(handle) )
			{
				ProgramRef& pr = m_programRef[handle.idx];
				++pr.m_refCount;
				shaderIncRef(pr.m_vsh);
				shaderIncRef(pr.m_fsh);
			}
			else
			{
				const ShaderRef& vsr = m_shaderRef[_vsh.idx];
				const ShaderRef& fsr = m_shaderRef[_fsh.idx];
				if (vsr.m_hashOut != fsr.m_hashIn)
				{
					BX_TRACE("Vertex shader output doesn't match fragment shader input.");
					return MAX_INVALID_HANDLE;
				}

				handle.idx = m_programHandle.alloc();

				BX_WARN(isValid(handle), "Failed to allocate program handle.");
				if (isValid(handle) )
				{
					shaderIncRef(_vsh);
					shaderIncRef(_fsh);
					ProgramRef& pr = m_programRef[handle.idx];
					pr.m_vsh = _vsh;
					pr.m_fsh = _fsh;
					pr.m_refCount = 1;

					const uint32_t key = uint32_t(_fsh.idx<<16)|_vsh.idx;
					bool ok = m_programHashMap.insert(key, handle.idx);
					BX_ASSERT(ok, "Program already exists (key: %x, handle: %3d)!", key, handle.idx); BX_UNUSED(ok);

					CommandBuffer& cmdbuf = getCommandBuffer(CommandBuffer::CreateProgram);
					cmdbuf.write(handle);
					cmdbuf.write(_vsh);
					cmdbuf.write(_fsh);
				}
			}

			if (_destroyShaders)
			{
				shaderTakeOwnership(_vsh);
				shaderTakeOwnership(_fsh);
			}

			return handle;
		}

		MAX_API_FUNC(ProgramHandle createProgram(ShaderHandle _vsh, bool _destroyShader) )
		{
			MAX_MUTEX_SCOPE(m_resourceApiLock);

			if (!isValid(_vsh) )
			{
				BX_WARN(false, "Compute shader is invalid (vsh %d).", _vsh.idx);
				return MAX_INVALID_HANDLE;
			}

			ProgramHandle handle = { m_programHashMap.find(_vsh.idx) };

			if (isValid(handle) )
			{
				ProgramRef& pr = m_programRef[handle.idx];
				++pr.m_refCount;
				shaderIncRef(pr.m_vsh);
			}
			else
			{
				handle.idx = m_programHandle.alloc();

				BX_WARN(isValid(handle), "Failed to allocate program handle.");
				if (isValid(handle) )
				{
					shaderIncRef(_vsh);
					ProgramRef& pr = m_programRef[handle.idx];
					pr.m_vsh = _vsh;
					ShaderHandle fsh = MAX_INVALID_HANDLE;
					pr.m_fsh = fsh;
					pr.m_refCount = 1;

					const uint32_t key = uint32_t(_vsh.idx);
					bool ok = m_programHashMap.insert(key, handle.idx);
					BX_ASSERT(ok, "Program already exists (key: %x, handle: %3d)!", key, handle.idx); BX_UNUSED(ok);

					CommandBuffer& cmdbuf = getCommandBuffer(CommandBuffer::CreateProgram);
					cmdbuf.write(handle);
					cmdbuf.write(_vsh);
					cmdbuf.write(fsh);
				}
			}

			if (_destroyShader)
			{
				shaderTakeOwnership(_vsh);
			}

			return handle;
		}

		MAX_API_FUNC(void destroyProgram(ProgramHandle _handle) )
		{
			MAX_MUTEX_SCOPE(m_resourceApiLock);

			MAX_CHECK_HANDLE("destroyProgram", m_programHandle, _handle);

			ProgramRef& pr = m_programRef[_handle.idx];
			shaderDecRef(pr.m_vsh);

			if (isValid(pr.m_fsh) )
			{
				shaderDecRef(pr.m_fsh);
			}

			int32_t refs = --pr.m_refCount;
			if (0 == refs)
			{
				bool ok = m_submit->free(_handle); BX_UNUSED(ok);
				BX_ASSERT(ok, "Program handle %d is already destroyed!", _handle.idx);

				CommandBuffer& cmdbuf = getCommandBuffer(CommandBuffer::DestroyProgram);
				cmdbuf.write(_handle);

				m_programHashMap.removeByHandle(_handle.idx);
			}
		}

		MAX_API_FUNC(TextureHandle createTexture(const Memory* _mem, uint64_t _flags, uint8_t _skip, TextureInfo* _info, BackbufferRatio::Enum _ratio, bool _immutable) )
		{
			MAX_MUTEX_SCOPE(m_resourceApiLock);

			TextureInfo ti;
			if (NULL == _info)
			{
				_info = &ti;
			}

			bimg::ImageContainer imageContainer;
			if (bimg::imageParse(imageContainer, _mem->data, _mem->size) )
			{
				calcTextureSize(*_info
					, (uint16_t)imageContainer.m_width
					, (uint16_t)imageContainer.m_height
					, (uint16_t)imageContainer.m_depth
					, imageContainer.m_cubeMap
					, imageContainer.m_numMips > 1
					, imageContainer.m_numLayers
					, TextureFormat::Enum(imageContainer.m_format)
					);
			}
			else
			{
				_info->format = TextureFormat::Unknown;
				_info->storageSize = 0;
				_info->width   = 0;
				_info->height  = 0;
				_info->depth   = 0;
				_info->numMips = 0;
				_info->bitsPerPixel = 0;
				_info->cubeMap = false;

				return MAX_INVALID_HANDLE;
			}

			_flags |= imageContainer.m_srgb ? MAX_TEXTURE_SRGB : 0;

			TextureHandle handle = { m_textureHandle.alloc() };
			BX_WARN(isValid(handle), "Failed to allocate texture handle.");

			if (!isValid(handle) )
			{
				release(_mem);
				return MAX_INVALID_HANDLE;
			}

			TextureRef& ref = m_textureRef[handle.idx];
			ref.init(
				  _ratio
				, uint16_t(imageContainer.m_width)
				, uint16_t(imageContainer.m_height)
				, uint16_t(imageContainer.m_depth)
				, _info->format
				, _info->storageSize
				, imageContainer.m_numMips
				, imageContainer.m_numLayers
				, 0 != (g_caps.supported & MAX_CAPS_TEXTURE_DIRECT_ACCESS)
				, _immutable
				, imageContainer.m_cubeMap
				, _flags
				);

			if (ref.isRt() )
			{
				m_rtMemoryUsed += int64_t(ref.m_storageSize);
			}
			else
			{
				m_textureMemoryUsed += int64_t(ref.m_storageSize);
			}

			CommandBuffer& cmdbuf = getCommandBuffer(CommandBuffer::CreateTexture);
			cmdbuf.write(handle);
			cmdbuf.write(_mem);
			cmdbuf.write(_flags);
			cmdbuf.write(_skip);

			setDebugNameForHandle(handle);

			return handle;
		}

		MAX_API_FUNC(void setName(TextureHandle _handle, const bx::StringView& _name) )
		{
			MAX_MUTEX_SCOPE(m_resourceApiLock);
			MAX_CHECK_HANDLE("setName", m_textureHandle, _handle);

			TextureRef& ref = m_textureRef[_handle.idx];
			ref.m_name.set(_name);

			setNameForHandle(_handle, _name);
		}

		void setDirectAccessPtr(TextureHandle _handle, void* _ptr)
		{
			TextureRef& ref = m_textureRef[_handle.idx];
			ref.m_ptr = _ptr;
		}

		MAX_API_FUNC(void* getDirectAccessPtr(TextureHandle _handle) )
		{
			MAX_MUTEX_SCOPE(m_resourceApiLock);
			MAX_CHECK_HANDLE("getDirectAccessPtr", m_textureHandle, _handle);

			TextureRef& ref = m_textureRef[_handle.idx];
			return ref.m_ptr;
		}

		MAX_API_FUNC(void destroyTexture(TextureHandle _handle) )
		{
			MAX_MUTEX_SCOPE(m_resourceApiLock);

			MAX_CHECK_HANDLE("destroyTexture", m_textureHandle, _handle);

			if (!isValid(_handle) )
			{
				BX_WARN(false, "Passing invalid texture handle to max::destroyTexture");
				return;
			}

			textureDecRef(_handle);
		}

		MAX_API_FUNC(uint32_t readTexture(TextureHandle _handle, void* _data, uint8_t _mip) )
		{
			MAX_MUTEX_SCOPE(m_resourceApiLock);

			MAX_CHECK_HANDLE("readTexture", m_textureHandle, _handle);

			const TextureRef& ref = m_textureRef[_handle.idx];
			BX_ASSERT(ref.isReadBack(), "Can't read from texture which was not created with MAX_TEXTURE_READ_BACK.");
			BX_ASSERT(_mip < ref.m_numMips, "Invalid mip: %d num mips:", _mip, ref.m_numMips);
			BX_UNUSED(ref);

			CommandBuffer& cmdbuf = getCommandBuffer(CommandBuffer::ReadTexture);
			cmdbuf.write(_handle);
			cmdbuf.write(_data);
			cmdbuf.write(_mip);
			return m_submit->m_frameNum + 2;
		}

		void resizeTexture(TextureHandle _handle, uint16_t _width, uint16_t _height, uint8_t _numMips, uint16_t _numLayers)
		{
			const TextureRef& ref = m_textureRef[_handle.idx];
			BX_ASSERT(BackbufferRatio::Count != ref.m_bbRatio, "");

			getTextureSizeFromRatio(BackbufferRatio::Enum(ref.m_bbRatio), _width, _height);
			_numMips = calcNumMips(1 < _numMips, _width, _height);

			BX_TRACE("Resize %3d: %4dx%d %s"
				, _handle.idx
				, _width
				, _height
				, bimg::getName(bimg::TextureFormat::Enum(ref.m_format) )
				);

			CommandBuffer& cmdbuf = getCommandBuffer(CommandBuffer::ResizeTexture);
			cmdbuf.write(_handle);
			cmdbuf.write(_width);
			cmdbuf.write(_height);
			cmdbuf.write(_numMips);
			cmdbuf.write(_numLayers);
		}

		void textureTakeOwnership(TextureHandle _handle)
		{
			TextureRef& ref = m_textureRef[_handle.idx];
			if (!ref.m_owned)
			{
				ref.m_owned = true;
				textureDecRef(_handle);
			}
		}

		void textureIncRef(TextureHandle _handle)
		{
			TextureRef& ref = m_textureRef[_handle.idx];
			++ref.m_refCount;
		}

		void textureDecRef(TextureHandle _handle)
		{
			TextureRef& ref = m_textureRef[_handle.idx];
			int32_t refs = --ref.m_refCount;
			if (0 == refs)
			{
				ref.m_name.clear();

				if (ref.isRt() )
				{
					m_rtMemoryUsed -= int64_t(ref.m_storageSize);
				}
				else
				{
					m_textureMemoryUsed -= int64_t(ref.m_storageSize);
				}

				bool ok = m_submit->free(_handle); BX_UNUSED(ok);
				BX_ASSERT(ok, "Texture handle %d is already destroyed!", _handle.idx);

				CommandBuffer& cmdbuf = getCommandBuffer(CommandBuffer::DestroyTexture);
				cmdbuf.write(_handle);
			}
		}

		MAX_API_FUNC(void updateTexture(
			  TextureHandle _handle
			, uint8_t _side
			, uint8_t _mip
			, uint16_t _x
			, uint16_t _y
			, uint16_t _z
			, uint16_t _width
			, uint16_t _height
			, uint16_t _depth
			, uint16_t _pitch
			, const Memory* _mem
		) )
		{
			MAX_MUTEX_SCOPE(m_resourceApiLock);

			const TextureRef& ref = m_textureRef[_handle.idx];
			if (ref.m_immutable)
			{
				BX_WARN(false, "Can't update immutable texture.");
				release(_mem);
				return;
			}

			CommandBuffer& cmdbuf = getCommandBuffer(CommandBuffer::UpdateTexture);
			cmdbuf.write(_handle);
			cmdbuf.write(_side);
			cmdbuf.write(_mip);
			Rect rect;
			rect.m_x = _x;
			rect.m_y = _y;
			rect.m_width  = _width;
			rect.m_height = _height;
			cmdbuf.write(rect);
			cmdbuf.write(_z);
			cmdbuf.write(_depth);
			cmdbuf.write(_pitch);
			cmdbuf.write(_mem);
		}

		MAX_API_FUNC(FrameBufferHandle createFrameBuffer(uint8_t _num, const Attachment* _attachment, bool _destroyTextures) )
		{
			MAX_MUTEX_SCOPE(m_resourceApiLock);

			bx::ErrorAssert err;
			isFrameBufferValid(_num, _attachment, &err);

			if (!err.isOk() )
			{
				return MAX_INVALID_HANDLE;
			}

			FrameBufferHandle handle = { m_frameBufferHandle.alloc() };
			BX_WARN(isValid(handle), "Failed to allocate frame buffer handle.");

			if (isValid(handle) )
			{
				CommandBuffer& cmdbuf = getCommandBuffer(CommandBuffer::CreateFrameBuffer);
				cmdbuf.write(handle);
				cmdbuf.write(false);
				cmdbuf.write(_num);

				const TextureRef& firstTexture = m_textureRef[_attachment[0].handle.idx];
				const BackbufferRatio::Enum bbRatio = BackbufferRatio::Enum(firstTexture.m_bbRatio);

				FrameBufferRef& fbr = m_frameBufferRef[handle.idx];
				if (BackbufferRatio::Count == bbRatio)
				{
					fbr.m_width  = bx::max<uint16_t>(firstTexture.m_width  >> _attachment[0].mip, 1);
					fbr.m_height = bx::max<uint16_t>(firstTexture.m_height >> _attachment[0].mip, 1);
				}

				fbr.m_window = false;
				bx::memSet(fbr.un.m_th, 0xff, sizeof(fbr.un.m_th) );

				for (uint32_t ii = 0; ii < _num; ++ii)
				{
					TextureHandle texHandle = _attachment[ii].handle;
					fbr.un.m_th[ii] = texHandle;
					textureIncRef(texHandle);
				}

				cmdbuf.write(_attachment, sizeof(Attachment) * _num);
			}

			if (_destroyTextures)
			{
				for (uint32_t ii = 0; ii < _num; ++ii)
				{
					textureTakeOwnership(_attachment[ii].handle);
				}
			}

			return handle;
		}

		MAX_API_FUNC(FrameBufferHandle createFrameBuffer(void* _nwh, uint16_t _width, uint16_t _height, TextureFormat::Enum _format, TextureFormat::Enum _depthFormat) )
		{
			MAX_MUTEX_SCOPE(m_resourceApiLock);

			FrameBufferHandle handle = { m_frameBufferHandle.alloc() };
			BX_WARN(isValid(handle), "Failed to allocate frame buffer handle.");

			if (isValid(handle) )
			{
				CommandBuffer& cmdbuf = getCommandBuffer(CommandBuffer::CreateFrameBuffer);
				cmdbuf.write(handle);
				cmdbuf.write(true);
				cmdbuf.write(_nwh);
				cmdbuf.write(_width);
				cmdbuf.write(_height);
				cmdbuf.write(_format);
				cmdbuf.write(_depthFormat);

				FrameBufferRef& fbr = m_frameBufferRef[handle.idx];
				fbr.m_width  = _width;
				fbr.m_height = _height;
				fbr.m_window = true;
				fbr.un.m_nwh = _nwh;
			}

			return handle;
		}

		MAX_API_FUNC(void setName(FrameBufferHandle _handle, const bx::StringView& _name) )
		{
			MAX_MUTEX_SCOPE(m_resourceApiLock);

			MAX_CHECK_HANDLE("setName", m_frameBufferHandle, _handle);

			FrameBufferRef& fbr = m_frameBufferRef[_handle.idx];
			fbr.m_name.set(_name);

//			setNameForHandle(_handle, _name);
		}

		MAX_API_FUNC(TextureHandle getTexture(FrameBufferHandle _handle, uint8_t _attachment) )
		{
			MAX_MUTEX_SCOPE(m_resourceApiLock);

			MAX_CHECK_HANDLE("getTexture", m_frameBufferHandle, _handle);

			const FrameBufferRef& fbr = m_frameBufferRef[_handle.idx];
			if (!fbr.m_window)
			{
				const uint32_t attachment = bx::min<uint32_t>(_attachment, MAX_CONFIG_MAX_FRAME_BUFFER_ATTACHMENTS);
				return fbr.un.m_th[attachment];
			}

			return MAX_INVALID_HANDLE;
		}

		MAX_API_FUNC(void destroyFrameBuffer(FrameBufferHandle _handle) )
		{
			MAX_MUTEX_SCOPE(m_resourceApiLock);

			MAX_CHECK_HANDLE("destroyFrameBuffer", m_frameBufferHandle, _handle);
			bool ok = m_submit->free(_handle); BX_UNUSED(ok);
			BX_ASSERT(ok, "Frame buffer handle %d is already destroyed!", _handle.idx);

			CommandBuffer& cmdbuf = getCommandBuffer(CommandBuffer::DestroyFrameBuffer);
			cmdbuf.write(_handle);

			FrameBufferRef& fbr = m_frameBufferRef[_handle.idx];
			fbr.m_name.clear();

			if (!fbr.m_window)
			{
				for (uint32_t ii = 0; ii < BX_COUNTOF(fbr.un.m_th); ++ii)
				{
					TextureHandle th = fbr.un.m_th[ii];
					if (isValid(th) )
					{
						textureDecRef(th);
					}
				}
			}
		}

		MAX_API_FUNC(UniformHandle createUniform(const char* _name, UniformType::Enum _type, uint16_t _num) )
		{
			MAX_MUTEX_SCOPE(m_resourceApiLock);

			{
				bx::ErrorAssert err;
				isIdentifierValid(_name, &err);

				if (!err.isOk() )
				{
					return MAX_INVALID_HANDLE;
				}
			}

			_num = bx::max<uint16_t>(1, _num);

			uint16_t idx = m_uniformHashMap.find(bx::hash<bx::HashMurmur2A>(_name) );
			if (kInvalidHandle != idx)
			{
				UniformHandle handle = { idx };
				UniformRef& uniform = m_uniformRef[handle.idx];
				BX_ASSERT(uniform.m_type == _type
					, "Uniform type mismatch (type: %d, expected %d)."
					, _type
					, uniform.m_type
					);

				uint32_t oldsize = g_uniformTypeSize[uniform.m_type];
				uint32_t newsize = g_uniformTypeSize[_type];

				if (oldsize < newsize
				||  uniform.m_num < _num)
				{
					uniform.m_type = oldsize < newsize ? _type : uniform.m_type;
					uniform.m_num  = bx::max<uint16_t>(uniform.m_num, _num);

					BX_TRACE("  Resize uniform (handle %3d) `%s`, num %d", handle.idx, _name, _num);

					CommandBuffer& cmdbuf = getCommandBuffer(CommandBuffer::CreateUniform);
					cmdbuf.write(handle);
					cmdbuf.write(uniform.m_type);
					cmdbuf.write(uniform.m_num);
					uint8_t len = bx::narrowCast<uint8_t>(bx::strLen(_name)+1);
					cmdbuf.write(len);
					cmdbuf.write(_name, len);
				}

				++uniform.m_refCount;
				return handle;
			}

			UniformHandle handle = { m_uniformHandle.alloc() };

			if (!isValid(handle) )
			{
				BX_TRACE("Failed to allocate uniform handle.");
				return MAX_INVALID_HANDLE;
			}

			BX_TRACE("Creating uniform (handle %3d) `%s`, num %d", handle.idx, _name, _num);

			UniformRef& uniform = m_uniformRef[handle.idx];
			uniform.m_name.set(_name);
			uniform.m_refCount = 1;
			uniform.m_type = _type;
			uniform.m_num  = _num;

			bool ok = m_uniformHashMap.insert(bx::hash<bx::HashMurmur2A>(_name), handle.idx);
			BX_ASSERT(ok, "Uniform already exists (name: %s)!", _name); BX_UNUSED(ok);

			CommandBuffer& cmdbuf = getCommandBuffer(CommandBuffer::CreateUniform);
			cmdbuf.write(handle);
			cmdbuf.write(_type);
			cmdbuf.write(_num);
			uint8_t len = bx::narrowCast<uint8_t>(bx::strLen(_name)+1);
			cmdbuf.write(len);
			cmdbuf.write(_name, len);

			return handle;
		}

		MAX_API_FUNC(void getUniformInfo(UniformHandle _handle, UniformInfo& _info) )
		{
			MAX_MUTEX_SCOPE(m_resourceApiLock);

			MAX_CHECK_HANDLE("getUniformInfo", m_uniformHandle, _handle);

			UniformRef& uniform = m_uniformRef[_handle.idx];
			bx::strCopy(_info.name, sizeof(_info.name), uniform.m_name.getPtr() );
			_info.type = uniform.m_type;
			_info.num  = uniform.m_num;
		}

		MAX_API_FUNC(void destroyUniform(UniformHandle _handle) )
		{
			MAX_MUTEX_SCOPE(m_resourceApiLock);

			MAX_CHECK_HANDLE("destroyUniform", m_uniformHandle, _handle);

			UniformRef& uniform = m_uniformRef[_handle.idx];
			BX_ASSERT(uniform.m_refCount > 0, "Destroying already destroyed uniform %d.", _handle.idx);
			int32_t refs = --uniform.m_refCount;

			if (0 == refs)
			{
				bool ok = m_submit->free(_handle); BX_UNUSED(ok);
				BX_ASSERT(ok, "Uniform handle %d is already destroyed!", _handle.idx);

				uniform.m_name.clear();
				m_uniformHashMap.removeByHandle(_handle.idx);

				CommandBuffer& cmdbuf = getCommandBuffer(CommandBuffer::DestroyUniform);
				cmdbuf.write(_handle);
			}
		}

		void meshTakeOwnership(MeshHandle _handle)
		{
			meshDecRef(_handle);
		}

		void meshIncRef(MeshHandle _handle)
		{
			MeshRef& mr = m_meshRef[_handle.idx];
			++mr.m_refCount;
		}

		void meshDecRef(MeshHandle _handle)
		{
			MeshRef& mr = m_meshRef[_handle.idx];
			int32_t refs = --mr.m_refCount;
			if (0 == refs)
			{
				bool ok = m_submit->free(_handle); BX_UNUSED(ok);
				BX_ASSERT(ok, "Mesh handle %d is already destroyed!", _handle.idx);

				for (GroupArray::const_iterator it = mr.m_groups.begin(), itEnd = mr.m_groups.end(); it != itEnd; ++it)
				{
					const Group& group = *it;
					
					destroyVertexBuffer(group.m_vbh);

					if (isValid(group.m_ibh))
					{
						destroyIndexBuffer(group.m_ibh);
					}

					if (NULL != group.m_vertices)
					{
						bx::free(g_allocator, group.m_vertices);
					}

					if (NULL != group.m_indices)
					{
						bx::free(g_allocator, group.m_indices);
					}
				}
				mr.m_groups.clear();

				m_meshHashMap.removeByHandle(_handle.idx);
			}
		}

		// @todo Rewrite mesh compiler to use uint32_t for indices instead of uint16_t.
		MAX_API_FUNC(MeshHandle createMesh(const Memory* _mem, bool _ramcopy))
		{
			MAX_MUTEX_SCOPE(m_resourceApiLock);

			bx::MemoryReader reader(_mem->data, _mem->size);

			const uint32_t meshHash = bx::hash<bx::HashMurmur2A>(_mem->data, _mem->size);
			const uint16_t idx = m_meshHashMap.find(meshHash);
			if (kInvalidHandle != idx)
			{
				MeshHandle handle = { idx };
				meshIncRef(handle);
				release(_mem);
				return handle;
			}

			MeshHandle handle = { m_meshHandle.alloc() };

			if (!isValid(handle))
			{
				BX_TRACE("Failed to allocate mesh handle.");
				return MAX_INVALID_HANDLE;
			}

			bool ok = m_meshHashMap.insert(meshHash, handle.idx);
			BX_ASSERT(ok, "Mesh already exists!"); BX_UNUSED(ok);

			MeshRef& mr = m_meshRef[handle.idx];
			mr.m_refCount = 1;
			mr.m_data = _mem;

			constexpr uint32_t kChunkVertexBuffer = BX_MAKEFOURCC('V', 'B', ' ', 0x1);
			constexpr uint32_t kChunkVertexBufferCompressed = BX_MAKEFOURCC('V', 'B', 'C', 0x0);
			constexpr uint32_t kChunkIndexBuffer = BX_MAKEFOURCC('I', 'B', ' ', 0x0);
			constexpr uint32_t kChunkIndexBufferCompressed = BX_MAKEFOURCC('I', 'B', 'C', 0x1);
			constexpr uint32_t kChunkPrimitive = BX_MAKEFOURCC('P', 'R', 'I', 0x0);

			Group group;

			uint32_t chunk;
			bx::Error err;
			while (4 == bx::read(&reader, chunk, &err) && err.isOk())
			{
				switch (chunk)
				{
				case kChunkVertexBuffer:
				{
					bx::read(&reader, group.m_sphere, &err);
					bx::read(&reader, group.m_aabb, &err);
					bx::read(&reader, group.m_obb, &err);
					read(&reader, mr.m_layout, &err);

					uint16_t stride = mr.m_layout.getStride();

					uint16_t numVertices = 0;
					bx::read(&reader, numVertices, &err);
					group.m_numVertices = (uint32_t)numVertices;

					const Memory* mem = alloc(group.m_numVertices * stride);
					bx::read(&reader, mem->data, mem->size, &err);

					if (_ramcopy)
					{
						group.m_vertices = (uint8_t*)bx::alloc(g_allocator, group.m_numVertices * stride);
						bx::memCopy(group.m_vertices, mem->data, mem->size);
					}

					group.m_vbh = createVertexBuffer(mem, mr.m_layout, MAX_BUFFER_NONE);
				}
				break;

				case kChunkVertexBufferCompressed:
				{
					bx::read(&reader, group.m_sphere, &err);
					bx::read(&reader, group.m_aabb, &err);
					bx::read(&reader, group.m_obb, &err);

					read(&reader, mr.m_layout, &err);

					uint16_t stride = mr.m_layout.getStride();

					uint16_t numVertices = 0;
					bx::read(&reader, numVertices, &err);
					group.m_numVertices = (uint32_t)numVertices;

					const Memory* mem = alloc(group.m_numVertices * stride);

					uint32_t compressedSize;
					bx::read(&reader, compressedSize, &err);

					void* compressedVertices = bx::alloc(g_allocator, compressedSize);
					bx::read(&reader, compressedVertices, compressedSize, &err);

					meshopt_decodeVertexBuffer(mem->data, group.m_numVertices, stride, (uint8_t*)compressedVertices, compressedSize);

					bx::free(g_allocator, compressedVertices);

					if (_ramcopy)
					{
						group.m_vertices = (uint8_t*)bx::alloc(g_allocator, group.m_numVertices * stride);
						bx::memCopy(group.m_vertices, mem->data, mem->size);
					}

					group.m_vbh = createVertexBuffer(mem, mr.m_layout, MAX_BUFFER_NONE);
				}
				break;

				case kChunkIndexBuffer:
				{
					bx::read(&reader, group.m_numIndices, &err);

					const Memory* mem = alloc(group.m_numIndices * 2);
					bx::read(&reader, mem->data, mem->size, &err);

					if (_ramcopy)
					{
						group.m_indices = (uint32_t*)bx::alloc(g_allocator, group.m_numIndices * sizeof(uint32_t));
						const uint16_t* src = (const uint16_t*)mem->data;
						for (uint32_t i = 0; i < group.m_numIndices; ++i)
						{
							group.m_indices[i] = (uint32_t)src[i];
						}
					}

					group.m_ibh = createIndexBuffer(mem, MAX_BUFFER_NONE);
				}
				break;

				case kChunkIndexBufferCompressed:
				{
					bx::read(&reader, group.m_numIndices, &err);

					const Memory* mem = alloc(group.m_numIndices * 2);

					uint32_t compressedSize;
					bx::read(&reader, compressedSize, &err);

					void* compressedIndices = bx::alloc(g_allocator, compressedSize);

					bx::read(&reader, compressedIndices, compressedSize, &err);

					meshopt_decodeIndexBuffer(mem->data, group.m_numIndices, 2, (uint8_t*)compressedIndices, compressedSize);

					bx::free(g_allocator, compressedIndices);

					if (_ramcopy)
					{
						group.m_indices = (uint32_t*)bx::alloc(g_allocator, group.m_numIndices * sizeof(uint32_t));
						const uint16_t* src = (const uint16_t*)mem->data;
						for (uint32_t i = 0; i < group.m_numIndices; ++i)
						{
							group.m_indices[i] = (uint32_t)src[i];
						}
					}

					group.m_ibh = createIndexBuffer(mem, MAX_BUFFER_NONE);
				}
				break;

				case kChunkPrimitive:
				{
					uint16_t len;
					bx::read(&reader, len, &err);

					stl::string material;
					material.resize(len);
					bx::read(&reader, const_cast<char*>(material.c_str()), len, &err);

					uint16_t num;
					bx::read(&reader, num, &err);

					for (uint32_t ii = 0; ii < num; ++ii)
					{
						bx::read(&reader, len, &err);

						stl::string name;
						name.resize(len);
						bx::read(&reader, const_cast<char*>(name.c_str()), len, &err);

						Primitive prim;
						bx::read(&reader, prim.m_startIndex, &err);
						bx::read(&reader, prim.m_numIndices, &err);
						bx::read(&reader, prim.m_startVertex, &err);
						bx::read(&reader, prim.m_numVertices, &err);
						bx::read(&reader, prim.m_sphere, &err);
						bx::read(&reader, prim.m_aabb, &err);
						bx::read(&reader, prim.m_obb, &err);

						group.m_prims.push_back(prim);
					}

					mr.m_groups.push_back(group);
					group.reset();
				}
				break;

				default:
					BX_TRACE("%08x at %d", chunk, bx::skip(&reader, 0));
					break;
				}
			}

			release(_mem);
			return handle;
		}

		MAX_API_FUNC(MeshHandle createMesh(const Memory* _vertices, const Memory* _indices, const VertexLayout& _layout))
		{
			MAX_MUTEX_SCOPE(m_resourceApiLock);

			const uint32_t meshHash = bx::hash<bx::HashMurmur2A>(_vertices->data, _vertices->size);
			const uint16_t idx = m_meshHashMap.find(meshHash);
			if (kInvalidHandle != idx)
			{
				MeshHandle handle = { idx };
				meshIncRef(handle);
				return handle;
			}

			MeshHandle handle = { m_meshHandle.alloc() };

			if (!isValid(handle))
			{
				BX_TRACE("Failed to allocate mesh handle.");
				return MAX_INVALID_HANDLE;
			}

			bool ok = m_meshHashMap.insert(meshHash, handle.idx);
			BX_ASSERT(ok, "Mesh already exists!"); BX_UNUSED(ok);

			MeshRef& mr = m_meshRef[handle.idx];
			mr.m_refCount = 1;
			mr.m_layout = _layout;

			uint16_t stride = _layout.getStride();

			Group group;
			group.m_numVertices = _vertices->size / (uint32_t)stride;
			group.m_numIndices = _indices->size / sizeof(uint32_t);

			group.m_vertices = (uint8_t*)bx::alloc(g_allocator, _vertices->size);
			bx::memCopy(group.m_vertices, _vertices->data, _vertices->size);

			group.m_indices = (uint32_t*)bx::alloc(g_allocator, _indices->size);
			bx::memCopy(group.m_indices, _indices->data, _indices->size);

			group.m_vbh = max::createVertexBuffer(max::makeRef(group.m_vertices, _vertices->size), _layout);
			group.m_ibh = max::createIndexBuffer(max::makeRef(group.m_indices, _indices->size), MAX_BUFFER_INDEX32);
			
			mr.m_groups.push_back(group);

			return handle;
		}

		MAX_API_FUNC(MeshQuery* queryMesh(MeshHandle _handle))
		{
			MAX_MUTEX_SCOPE(m_resourceApiLock);

			MAX_CHECK_HANDLE("queryMesh", m_meshHandle, _handle);

			MeshRef& mr = m_meshRef[_handle.idx];

			m_meshQuery.m_num = (uint32_t)mr.m_groups.size();
			BX_ASSERT(m_meshQuery.m_num <= MAX_CONFIG_MAX_MESH_GROUPS, "Mesh has more groups than allowed.");

			stl::vector<VertexBufferHandle> vb;
			stl::vector<IndexBufferHandle> ib;
			stl::vector<MeshQuery::Data> data;

			for (uint32_t ii = 0; ii < m_meshQuery.m_num; ++ii)
			{
				Group& group = mr.m_groups[ii];

				vb.push_back(group.m_vbh);
				ib.push_back(group.m_ibh);

				MeshQuery::Data groupData;
				groupData.m_numVertices = group.m_numVertices;
				groupData.m_numIndices = group.m_numIndices;
				groupData.m_vertices = group.m_vertices;
				groupData.m_indices = group.m_indices;
				data.push_back(groupData);
			}

			bx::memCopy(
				m_meshQuery.m_vertices,
				vb.data(),
				(sizeof(VertexBufferHandle)) * m_meshQuery.m_num
			);

			bx::memCopy(
				m_meshQuery.m_indices,
				ib.data(),
				(sizeof(IndexBufferHandle)) * m_meshQuery.m_num
			);

			bx::memCopy(
				m_meshQuery.m_data,
				data.data(),
				(sizeof(MeshQuery::Data)) * m_meshQuery.m_num
			);

			return &m_meshQuery;
		}

		MAX_API_FUNC(const max::VertexLayout getLayout(MeshHandle _handle))
		{
			MAX_MUTEX_SCOPE(m_resourceApiLock);

			MAX_CHECK_HANDLE("getLayout", m_meshHandle, _handle);

			MeshRef& mr = m_meshRef[_handle.idx];
			return mr.m_layout;
		}

		MAX_API_FUNC(void destroyMesh(MeshHandle _handle))
		{
			MAX_MUTEX_SCOPE(m_resourceApiLock);

			MAX_CHECK_HANDLE("destroyMesh", m_meshHandle, _handle);

			if (!isValid(_handle))
			{
				BX_WARN(false, "Passing invalid mesh handle to max::destroyMesh.");
				return;
			}

			meshDecRef(_handle);
		}

		void componentTakeOwnership(ComponentHandle _handle)
		{
			componentDecRef(_handle);
		}

		void componentIncRef(ComponentHandle _handle)
		{
			ComponentRef& cr = m_componentRef[_handle.idx];
			++cr.m_refCount;
		}

		void componentDecRef(ComponentHandle _handle)
		{
			ComponentRef& cr = m_componentRef[_handle.idx];
			int32_t refs = --cr.m_refCount;
			if (0 == refs)
			{
				bool ok = m_submit->free(_handle); BX_UNUSED(ok);
				BX_ASSERT(ok, "Component handle %d is already destroyed!", _handle.idx);

				bx::free(g_allocator, cr.m_data);

				cr.m_data = NULL; 
				cr.m_size = 0;
			}
		}

		MAX_API_FUNC(ComponentHandle createComponent(void* _data, uint32_t _size))
		{
			MAX_MUTEX_SCOPE(m_resourceApiLock);

			ComponentHandle handle = { m_componentHandle.alloc() };
			if (isValid(handle))
			{
				ComponentRef& cr = m_componentRef[handle.idx];
				cr.m_refCount = 1;

				cr.m_data = bx::alloc(g_allocator, _size);
				cr.m_size = _size;

				bx::memMove(cr.m_data, _data, _size);
			}

			return handle;
		}

		MAX_API_FUNC(void destroyComponent(ComponentHandle _handle))
		{
			MAX_MUTEX_SCOPE(m_resourceApiLock);

			MAX_CHECK_HANDLE("destroyComponent", m_componentHandle, _handle);

			if (!isValid(_handle))
			{
				BX_WARN(false, "Passing invalid component handle to max::destroyComponent.");
				return;
			}

			componentDecRef(_handle);
		}

		void entityTakeOwnership(EntityHandle _handle)
		{
			entityDecRef(_handle);
		}

		void entityIncRef(EntityHandle _handle)
		{
			EntityRef& er = m_entityRef[_handle.idx];
			++er.m_refCount;
		}

		void entityDecRef(EntityHandle _handle)
		{
			EntityRef& er = m_entityRef[_handle.idx];
			int32_t refs = --er.m_refCount;
			if (0 == refs)
			{
				bool ok = m_submit->free(_handle); BX_UNUSED(ok);
				BX_ASSERT(ok, "Entity handle %d is already destroyed!", _handle.idx);

				auto it = er.m_components.first();
				while (er.m_destroyComponents)
				{
					ComponentHandle handle = { it.handle };

					if (!er.m_components.next(it))
					{
						break;
					}

					if (isValid(handle))
					{
						destroyComponent(handle);
					}
				}

				er.m_components.reset();
			}
		}

		MAX_API_FUNC(EntityHandle createEntity(bool _destroyComponents))
		{
			MAX_MUTEX_SCOPE(m_resourceApiLock);

			EntityHandle handle = { m_entityHandle.alloc() };
			if (isValid(handle))
			{
				EntityRef& er = m_entityRef[handle.idx];
				er.m_refCount = 1;
				er.m_destroyComponents = _destroyComponents;
				er.m_components.reset();
			}
			
			return handle;
		}

		MAX_API_FUNC(void addComponent(EntityHandle _entity, ComponentHandle _component, uint32_t _hash))
		{
			EntityRef& er = m_entityRef[_entity.idx];
			er.m_components.insert(_hash, _component.idx);
		}

		MAX_API_FUNC(void* getComponent(EntityHandle _handle, uint32_t _hash))
		{
			EntityRef& er = m_entityRef[_handle.idx];
			ComponentHandle handle = { er.m_components.find(_hash) };
			if (isValid(handle))
			{
				ComponentRef& cr = m_componentRef[handle.idx];
				return cr.m_data;  
			}

			return NULL;
		}

		MAX_API_FUNC(EntityQuery* queryEntities(const HashQuery& _hashes))
		{
			MAX_MUTEX_SCOPE(m_resourceApiLock);

			m_entityQuery.m_num = 0;

			for (uint16_t idx = 0, end = MAX_CONFIG_MAX_ENTITIES; idx < end; ++idx)
			{
				EntityHandle handle = { idx };
				if (!isValid(handle))
				{
					continue;
				}

				EntityRef& er = m_entityRef[handle.idx];
				if (er.m_components.getNumElements() <= 0)
				{
					// @todo I dont like that I need to do this check. Because an entity can be deleted.
					continue;
				}

				bool matches = true;
				for (uint32_t ii = 0; ii < _hashes.m_num; ++ii)
				{
					uint32_t hash = _hashes.m_data[ii];
					if (er.m_components.find(hash) == bx::kInvalidHandle)
					{
						matches = false;
						break;
					}
				}

				if (matches)
				{
					m_entityQuery.m_entities[m_entityQuery.m_num] = handle;
					++m_entityQuery.m_num;
				}
			}

			return &m_entityQuery;
		}

		MAX_API_FUNC(void destroyEntity(EntityHandle _handle))
		{
			MAX_MUTEX_SCOPE(m_resourceApiLock);

			MAX_CHECK_HANDLE("destroyEntity", m_entityHandle, _handle);

			if (!isValid(_handle))
			{
				BX_WARN(false, "Passing invalid entity handle to max::destroyEntity.");
				return;
			}

			entityDecRef(_handle);
		}

		MAX_API_FUNC(BodyHandle createBody(CollisionShape::Enum _shape, const bx::Vec3& _pos, const bx::Quaternion& _quat, const bx::Vec3& _scale, LayerType::Enum _layer, MotionType::Enum _motion, Activation::Enum _activation, float _maxVelocity, uint8_t _flags))
		{
			MAX_MUTEX_SCOPE(m_resourceApiLock);

			BodyHandle handle = { m_bodyHandle.alloc() };
			if (isValid(handle))
			{
				m_physicsCtx->createBody(handle, _shape, _pos, _quat, _scale, _layer, _motion, _activation, _maxVelocity, _flags);
			}

			return handle;
		}

		MAX_API_FUNC(void setPosition(BodyHandle _handle, const bx::Vec3& _pos, Activation::Enum _activation))
		{
			MAX_MUTEX_SCOPE(m_resourceApiLock);

			MAX_CHECK_HANDLE("setPosition", m_bodyHandle, _handle);

			m_physicsCtx->setPosition(_handle, _pos, _activation);
		}

		MAX_API_FUNC(bx::Vec3 getPosition(BodyHandle _handle))
		{
			MAX_MUTEX_SCOPE(m_resourceApiLock);

			MAX_CHECK_HANDLE("getPosition", m_bodyHandle, _handle);

			return m_physicsCtx->getPosition(_handle);
		}

		MAX_API_FUNC(void setRotation(BodyHandle _handle, const bx::Quaternion& _rot, Activation::Enum _activation))
		{
			MAX_MUTEX_SCOPE(m_resourceApiLock);

			MAX_CHECK_HANDLE("setRotation", m_bodyHandle, _handle);

			m_physicsCtx->setRotation(_handle, _rot, _activation);
		}

		MAX_API_FUNC(bx::Quaternion getRotation(BodyHandle _handle))
		{
			MAX_MUTEX_SCOPE(m_resourceApiLock);

			MAX_CHECK_HANDLE("getRotation", m_bodyHandle, _handle);

			return m_physicsCtx->getRotation(_handle);
		}

		MAX_API_FUNC(void setLinearVelocity(BodyHandle _handle, const bx::Vec3& _velocity))
		{
			MAX_MUTEX_SCOPE(m_resourceApiLock);

			MAX_CHECK_HANDLE("setLinearVelocity", m_bodyHandle, _handle);

			m_physicsCtx->setLinearVelocity(_handle, _velocity);
		}

		MAX_API_FUNC(bx::Vec3 getLinearVelocity(BodyHandle _handle))
		{
			MAX_MUTEX_SCOPE(m_resourceApiLock);

			MAX_CHECK_HANDLE("getLinearVelocity", m_bodyHandle, _handle);

			return m_physicsCtx->getLinearVelocity(_handle);
		}

		MAX_API_FUNC(void setAngularVelocity(BodyHandle _handle, const bx::Vec3& _angularVelocity))
		{
			MAX_MUTEX_SCOPE(m_resourceApiLock);

			MAX_CHECK_HANDLE("setAngularVelocity", m_bodyHandle, _handle);

			m_physicsCtx->setAngularVelocity(_handle, _angularVelocity);
		}

		MAX_API_FUNC(bx::Vec3 getAngularVelocity(BodyHandle _handle))
		{
			MAX_MUTEX_SCOPE(m_resourceApiLock);

			MAX_CHECK_HANDLE("getAngularVelocity", m_bodyHandle, _handle);

			return m_physicsCtx->getAngularVelocity(_handle);
		}

		MAX_API_FUNC(void addLinearAndAngularVelocity(BodyHandle _handle, const bx::Vec3& _linearVelocity, const bx::Vec3& _angularVelocity))
		{
			MAX_MUTEX_SCOPE(m_resourceApiLock);

			MAX_CHECK_HANDLE("addLinearAndAngularVelocity", m_bodyHandle, _handle);

			m_physicsCtx->addLinearAndAngularVelocity(_handle, _linearVelocity, _angularVelocity);
		}

		MAX_API_FUNC(void addLinearImpulse(BodyHandle _handle, const bx::Vec3& _impulse))
		{
			MAX_MUTEX_SCOPE(m_resourceApiLock);

			MAX_CHECK_HANDLE("addLinearImpulse", m_bodyHandle, _handle);

			m_physicsCtx->addLinearImpulse(_handle, _impulse);
		}

		MAX_API_FUNC(void addAngularImpulse(BodyHandle _handle, const bx::Vec3& _impulse))
		{
			MAX_MUTEX_SCOPE(m_resourceApiLock);

			MAX_CHECK_HANDLE("addAngularImpulse", m_bodyHandle, _handle);

			m_physicsCtx->addAngularImpulse(_handle, _impulse);
		}

		MAX_API_FUNC(void addBuoyancyImpulse(BodyHandle _handle, const bx::Vec3& _surfacePosition, const bx::Vec3& _surfaceNormal, float _buoyancy, float _linearDrag, float _angularDrag, const bx::Vec3& _fluidVelocity, const bx::Vec3& _gravity, float _deltaTime))
		{
			MAX_MUTEX_SCOPE(m_resourceApiLock);

			MAX_CHECK_HANDLE("addBuoyancyImpulse", m_bodyHandle, _handle);

			return m_physicsCtx->addBuoyancyImpulse(_handle, _surfacePosition, _surfaceNormal, _buoyancy, _linearDrag, _angularDrag, _fluidVelocity, _gravity, _deltaTime);
		}

		MAX_API_FUNC(void addForce(BodyHandle _handle, const bx::Vec3& _force, Activation::Enum _activation))
		{
			MAX_MUTEX_SCOPE(m_resourceApiLock);

			MAX_CHECK_HANDLE("addForce", m_bodyHandle, _handle);

			m_physicsCtx->addForce(_handle, _force,  _activation);
		}

		MAX_API_FUNC(void addTorque(BodyHandle _handle, const bx::Vec3& _torque, Activation::Enum _activation))
		{
			MAX_MUTEX_SCOPE(m_resourceApiLock);

			MAX_CHECK_HANDLE("addTorque", m_bodyHandle, _handle);

			m_physicsCtx->addTorque(_handle, _torque, _activation);
		}

		MAX_API_FUNC(void addMovement(BodyHandle _handle, const bx::Vec3& _position, const bx::Quaternion& _rotation, const float _deltaTime))
		{
			MAX_MUTEX_SCOPE(m_resourceApiLock);

			MAX_CHECK_HANDLE("addMovement", m_bodyHandle, _handle);

			m_physicsCtx->addMovement(_handle, _position, _rotation, _deltaTime);
		}

		MAX_API_FUNC(void setFriction(BodyHandle _handle, float _friction))
		{
			MAX_MUTEX_SCOPE(m_resourceApiLock);

			MAX_CHECK_HANDLE("setFriction", m_bodyHandle, _handle);

			m_physicsCtx->setFriction(_handle, _friction);
		}

		MAX_API_FUNC(float getFriction(BodyHandle _handle))
		{
			MAX_MUTEX_SCOPE(m_resourceApiLock);

			MAX_CHECK_HANDLE("getFriction", m_bodyHandle, _handle);

			return m_physicsCtx->getFriction(_handle);
		}

		MAX_API_FUNC(void getGroundInfo(BodyHandle _handle, GroundInfo& _info))
		{
			MAX_MUTEX_SCOPE(m_resourceApiLock);

			MAX_CHECK_HANDLE("getGroundInfo", m_bodyHandle, _handle);

			return m_physicsCtx->getGroundInfo(_handle, _info);
		}

		MAX_API_FUNC(void destroyBody(BodyHandle _handle))
		{
			MAX_MUTEX_SCOPE(m_resourceApiLock);

			MAX_CHECK_HANDLE("destroyBody", m_bodyHandle, _handle);

			m_physicsCtx->destroyBody(_handle);

			m_freeBodyHandle[m_numFreeBodyHandles++] = _handle;
		}

		MAX_API_FUNC(const bx::Vec3 getGravity())
		{
			return m_physicsCtx->getGravity();
		}

		MAX_API_FUNC(OcclusionQueryHandle createOcclusionQuery() )
		{
			MAX_MUTEX_SCOPE(m_resourceApiLock);

			OcclusionQueryHandle handle = { m_occlusionQueryHandle.alloc() };
			if (isValid(handle) )
			{
				m_submit->m_occlusion[handle.idx] = INT32_MIN;

				CommandBuffer& cmdbuf = getCommandBuffer(CommandBuffer::InvalidateOcclusionQuery);
				cmdbuf.write(handle);
			}

			return handle;
		}

		MAX_API_FUNC(OcclusionQueryResult::Enum getResult(OcclusionQueryHandle _handle, int32_t* _result) )
		{
			MAX_MUTEX_SCOPE(m_resourceApiLock);

			MAX_CHECK_HANDLE("getResult", m_occlusionQueryHandle, _handle);

			switch (m_submit->m_occlusion[_handle.idx])
			{
			case 0:         return OcclusionQueryResult::Invisible;
			case INT32_MIN: return OcclusionQueryResult::NoResult;
			default: break;
			}

			if (NULL != _result)
			{
				*_result = m_submit->m_occlusion[_handle.idx];
			}

			return OcclusionQueryResult::Visible;
		}

		MAX_API_FUNC(void destroyOcclusionQuery(OcclusionQueryHandle _handle) )
		{
			MAX_MUTEX_SCOPE(m_resourceApiLock);

			MAX_CHECK_HANDLE("destroyOcclusionQuery", m_occlusionQueryHandle, _handle);

			m_freeOcclusionQueryHandle[m_numFreeOcclusionQueryHandles++] = _handle;
		}

		MAX_API_FUNC(void requestScreenShot(FrameBufferHandle _handle, const char* _filePath) )
		{
			MAX_MUTEX_SCOPE(m_resourceApiLock);

			MAX_CHECK_HANDLE_INVALID_OK("requestScreenShot", m_frameBufferHandle, _handle);

			if (isValid(_handle) )
			{
				const FrameBufferRef& fbr = m_frameBufferRef[_handle.idx];
				if (!fbr.m_window)
				{
					BX_TRACE("requestScreenShot can be done only for window frame buffer handles (handle: %d).", _handle.idx);
					return;
				}
			}

			if (m_submit->m_numScreenShots >= MAX_CONFIG_MAX_SCREENSHOTS)
			{
				BX_TRACE("Only %d screenshots can be requested.", MAX_CONFIG_MAX_SCREENSHOTS);
				return;
			}

			for (uint8_t ii = 0, num = m_submit->m_numScreenShots; ii < num; ++ii)
			{
				const ScreenShot& screenShot = m_submit->m_screenShot[ii];
				if (screenShot.handle.idx == _handle.idx)
				{
					BX_TRACE("Already requested screenshot on handle %d.", _handle.idx);
					return;
				}
			}

			ScreenShot& screenShot = m_submit->m_screenShot[m_submit->m_numScreenShots++];
			screenShot.handle = _handle;
			screenShot.filePath.set(_filePath);
		}

		MAX_API_FUNC(void cmdAdd(const char* _name, ConsoleFn _fn, void* _userData))
		{
			const uint32_t cmd = bx::hash<bx::HashMurmur2A>(_name, (uint32_t)bx::strLen(_name));
			BX_ASSERT(m_lookup.end() == m_lookup.find(cmd), "Command \"%s\" already exist.", _name);

			Func fn = { _fn, _userData };
			m_lookup.insert(stl::make_pair(cmd, fn));
		}

		MAX_API_FUNC(void cmdRemove(const char* _name))
		{
			const uint32_t cmd = bx::hash<bx::HashMurmur2A>(_name, (uint32_t)bx::strLen(_name));

			CmdLookup::iterator it = m_lookup.find(cmd);
			if (it != m_lookup.end())
			{
				m_lookup.erase(it);
			}
		}

		MAX_API_FUNC(void cmdExec(const char* _cmd))
		{
			for (bx::StringView next(_cmd); !next.isEmpty(); _cmd = next.getPtr())
			{
				char commandLine[1024];
				uint32_t size = sizeof(commandLine);
				int argc;
				char* argv[64];
				next = bx::tokenizeCommandLine(_cmd, commandLine, size, argc, argv, BX_COUNTOF(argv), '\n');
				if (argc > 0)
				{
					int err = -1;
					uint32_t cmd = bx::hash<bx::HashMurmur2A>(argv[0], (uint32_t)bx::strLen(argv[0]));
					CmdLookup::iterator it = m_lookup.find(cmd);
					if (it != m_lookup.end())
					{
						Func& fn = it->second;
						err = fn.m_fn(fn.m_userData, argc, argv);
					}

					switch (err)
					{
					case 0:
						break;

					case -1:
					{
						stl::string tmp(_cmd, next.getPtr() - _cmd - (next.isEmpty() ? 0 : 1));
						BX_TRACE("Command '%s' doesn't exist.", tmp.c_str());
					}
					break;

					default:
					{
						stl::string tmp(_cmd, next.getPtr() - _cmd - (next.isEmpty() ? 0 : 1));
						BX_TRACE("Failed '%s' err: %d.", tmp.c_str(), err);
					}
					break;
					}
				}
			}
		}

		MAX_API_FUNC(void setPaletteColor(uint8_t _index, const float _rgba[4]) )
		{
			MAX_MUTEX_SCOPE(m_resourceApiLock);

			BX_ASSERT(_index < MAX_CONFIG_MAX_COLOR_PALETTE, "Color palette index out of bounds %d (max: %d)."
				, _index
				, MAX_CONFIG_MAX_COLOR_PALETTE
				);
			bx::memCopy(&m_clearColor[_index][0], _rgba, 16);
			m_colorPaletteDirty = 2;
		}

		MAX_API_FUNC(void setViewName(ViewId _id, const bx::StringView& _name) )
		{
			MAX_MUTEX_SCOPE(m_resourceApiLock);

			CommandBuffer& cmdbuf = getCommandBuffer(CommandBuffer::UpdateViewName);
			cmdbuf.write(_id);
			cmdbuf.write(_name);
		}

		MAX_API_FUNC(void setViewRect(ViewId _id, uint16_t _x, uint16_t _y, uint16_t _width, uint16_t _height) )
		{
			m_view[_id].setRect(_x, _y, _width, _height);
		}

		MAX_API_FUNC(void setViewScissor(ViewId _id, uint16_t _x, uint16_t _y, uint16_t _width, uint16_t _height) )
		{
			m_view[_id].setScissor(_x, _y, _width, _height);
		}

		MAX_API_FUNC(void setViewClear(ViewId _id, uint16_t _flags, uint32_t _rgba, float _depth, uint8_t _stencil) )
		{
			BX_ASSERT(bx::isEqual(_depth, bx::clamp(_depth, 0.0f, 1.0f), 0.0001f)
				, "Clear depth value must be between 0.0 and 1.0 (_depth %f)."
				, _depth
				);

			m_view[_id].setClear(_flags, _rgba, _depth, _stencil);
		}

		MAX_API_FUNC(void setViewClear(ViewId _id, uint16_t _flags, float _depth, uint8_t _stencil, uint8_t _0, uint8_t _1, uint8_t _2, uint8_t _3, uint8_t _4, uint8_t _5, uint8_t _6, uint8_t _7) )
		{
			BX_ASSERT(bx::isEqual(_depth, bx::clamp(_depth, 0.0f, 1.0f), 0.0001f)
				, "Clear depth value must be between 0.0 and 1.0 (_depth %f)."
				, _depth
				);

			m_view[_id].setClear(_flags, _depth, _stencil, _0, _1, _2, _3, _4, _5, _6, _7);
		}

		MAX_API_FUNC(void setViewMode(ViewId _id, ViewMode::Enum _mode) )
		{
			m_view[_id].setMode(_mode);
		}

		MAX_API_FUNC(void setViewFrameBuffer(ViewId _id, FrameBufferHandle _handle) )
		{
			MAX_CHECK_HANDLE_INVALID_OK("setViewFrameBuffer", m_frameBufferHandle, _handle);
			m_view[_id].setFrameBuffer(_handle);
		}

		MAX_API_FUNC(void setViewTransform(ViewId _id, const void* _view, const void* _proj) )
		{
			m_view[_id].setTransform(_view, _proj);
		}

		MAX_API_FUNC(void resetView(ViewId _id) )
		{
			m_view[_id].reset();
		}

		MAX_API_FUNC(void setViewOrder(ViewId _id, uint16_t _num, const ViewId* _order) )
		{
			const uint32_t num = bx::min(_id + _num, MAX_CONFIG_MAX_VIEWS) - _id;
			if (NULL == _order)
			{
				for (uint32_t ii = 0; ii < num; ++ii)
				{
					ViewId id = ViewId(ii+_id);
					m_viewRemap[id] = id;
				}
			}
			else
			{
				bx::memCopy(&m_viewRemap[_id], _order, num*sizeof(ViewId) );
			}
		}

		MAX_API_FUNC(Encoder* begin(bool _forThread) );

		MAX_API_FUNC(void end(Encoder* _encoder) );

		MAX_API_FUNC(uint32_t frame(bool _capture = false) );

		uint32_t getSeqIncr(ViewId _id)
		{
			return bx::atomicFetchAndAdd<uint32_t>(&m_seq[_id], 1);
		}

		void dumpViewStats();
		void freeDynamicBuffers();
		void freeAllHandles(Frame* _frame);
		void frameNoRenderWait();
		void swap();

		// render thread
		void flip();
		RenderFrame::Enum renderFrame(int32_t _msecs = -1);
		void flushTextureUpdateBatch(CommandBuffer& _cmdbuf);
		void rendererExecCommands(CommandBuffer& _cmdbuf);

#if MAX_CONFIG_MULTITHREADED
		void apiSemPost()
		{
			if (!m_singleThreaded)
			{
				m_apiSem.post();
			}
		}

		bool apiSemWait(int32_t _msecs = -1)
		{
			if (m_singleThreaded)
			{
				return true;
			}

			MAX_PROFILER_SCOPE("max/API thread wait", 0xff2040ff);
			int64_t start = bx::getHPCounter();
			bool ok = m_apiSem.wait(_msecs);
			if (ok)
			{
				m_render->m_waitSubmit = bx::getHPCounter()-start;
				m_submit->m_perfStats.waitSubmit = m_submit->m_waitSubmit;
				return true;
			}

			return false;
		}

		void renderSemPost()
		{
			if (!m_singleThreaded)
			{
				m_renderSem.post();
			}
		}

		void renderSemWait()
		{
			if (!m_singleThreaded)
			{
				MAX_PROFILER_SCOPE("max/Render thread wait", 0xff2040ff);
				int64_t start = bx::getHPCounter();
				bool ok = m_renderSem.wait();
				BX_ASSERT(ok, "Semaphore wait failed."); BX_UNUSED(ok);
				m_submit->m_waitRender = bx::getHPCounter() - start;
				m_submit->m_perfStats.waitRender = m_submit->m_waitRender;
			}
		}

		void encoderApiWait()
		{
			uint16_t numEncoders = m_encoderHandle->getNumHandles();

			for (uint16_t ii = 1; ii < numEncoders; ++ii)
			{
				m_encoderEndSem.wait();
			}

			for (uint16_t ii = 0; ii < numEncoders; ++ii)
			{
				uint16_t idx = m_encoderHandle->getHandleAt(ii);
				m_encoderStats[ii].cpuTimeBegin = m_encoder[idx].m_cpuTimeBegin;
				m_encoderStats[ii].cpuTimeEnd   = m_encoder[idx].m_cpuTimeEnd;
			}

			m_submit->m_perfStats.numEncoders = uint8_t(numEncoders);

			m_encoderHandle->reset();
			uint16_t idx = m_encoderHandle->alloc();
			BX_ASSERT(0 == idx, "Internal encoder handle is not 0 (idx %d).", idx); BX_UNUSED(idx);
		}

		bx::Semaphore m_renderSem;
		bx::Semaphore m_apiSem;
		bx::Semaphore m_encoderEndSem;
		bx::Mutex     m_encoderApiLock;
		bx::Mutex     m_resourceApiLock;
		bx::Thread    m_thread;
#else
		void apiSemPost()
		{
		}

		bool apiSemWait(int32_t _msecs = -1)
		{
			BX_UNUSED(_msecs);
			return true;
		}

		void renderSemPost()
		{
		}

		void renderSemWait()
		{
		}

		void encoderApiWait()
		{
			m_encoderStats[0].cpuTimeBegin = m_encoder[0].m_cpuTimeBegin;
			m_encoderStats[0].cpuTimeEnd   = m_encoder[0].m_cpuTimeEnd;
			m_submit->m_perfStats.numEncoders = 1;
		}
#endif // MAX_CONFIG_MULTITHREADED

		EncoderStats* m_encoderStats;
		Encoder*      m_encoder0;
		EncoderImpl*  m_encoder;
		uint32_t      m_numEncoders;
		bx::HandleAlloc* m_encoderHandle;

		Frame  m_frame[1+(MAX_CONFIG_MULTITHREADED ? 1 : 0)];
		Frame* m_render;
		Frame* m_submit;

		uint64_t m_tempKeys[MAX_CONFIG_MAX_DRAW_CALLS];
		RenderItemCount m_tempValues[MAX_CONFIG_MAX_DRAW_CALLS];

		IndexBuffer  m_indexBuffers[MAX_CONFIG_MAX_INDEX_BUFFERS];
		VertexBuffer m_vertexBuffers[MAX_CONFIG_MAX_VERTEX_BUFFERS];

		DynamicIndexBuffer  m_dynamicIndexBuffers[MAX_CONFIG_MAX_DYNAMIC_INDEX_BUFFERS];
		DynamicVertexBuffer m_dynamicVertexBuffers[MAX_CONFIG_MAX_DYNAMIC_VERTEX_BUFFERS];

		uint16_t m_numFreeDynamicIndexBufferHandles;
		uint16_t m_numFreeDynamicVertexBufferHandles;
		uint16_t m_numFreeBodyHandles;
		uint16_t m_numFreeOcclusionQueryHandles;
		DynamicIndexBufferHandle  m_freeDynamicIndexBufferHandle[MAX_CONFIG_MAX_DYNAMIC_INDEX_BUFFERS];
		DynamicVertexBufferHandle m_freeDynamicVertexBufferHandle[MAX_CONFIG_MAX_DYNAMIC_VERTEX_BUFFERS];
		BodyHandle				  m_freeBodyHandle[MAX_CONFIG_MAX_BODIES];
		OcclusionQueryHandle      m_freeOcclusionQueryHandle[MAX_CONFIG_MAX_OCCLUSION_QUERIES];

		NonLocalAllocator m_dynIndexBufferAllocator;
		bx::HandleAllocT<MAX_CONFIG_MAX_DYNAMIC_INDEX_BUFFERS> m_dynamicIndexBufferHandle;
		NonLocalAllocator m_dynVertexBufferAllocator;
		bx::HandleAllocT<MAX_CONFIG_MAX_DYNAMIC_VERTEX_BUFFERS> m_dynamicVertexBufferHandle;

		bx::HandleAllocT<MAX_CONFIG_MAX_INDEX_BUFFERS> m_indexBufferHandle;
		bx::HandleAllocT<MAX_CONFIG_MAX_VERTEX_LAYOUTS > m_layoutHandle;

		bx::HandleAllocT<MAX_CONFIG_MAX_VERTEX_BUFFERS> m_vertexBufferHandle;
		bx::HandleAllocT<MAX_CONFIG_MAX_SHADERS> m_shaderHandle;
		bx::HandleAllocT<MAX_CONFIG_MAX_PROGRAMS> m_programHandle;
		bx::HandleAllocT<MAX_CONFIG_MAX_TEXTURES> m_textureHandle;
		bx::HandleAllocT<MAX_CONFIG_MAX_FRAME_BUFFERS> m_frameBufferHandle;
		bx::HandleAllocT<MAX_CONFIG_MAX_UNIFORMS> m_uniformHandle;
		bx::HandleAllocT<MAX_CONFIG_MAX_MESHES> m_meshHandle;
		bx::HandleAllocT<MAX_CONFIG_MAX_COMPONENTS> m_componentHandle;
		bx::HandleAllocT<MAX_CONFIG_MAX_ENTITIES> m_entityHandle;
		bx::HandleAllocT<MAX_CONFIG_MAX_BODIES> m_bodyHandle;
		bx::HandleAllocT<MAX_CONFIG_MAX_OCCLUSION_QUERIES> m_occlusionQueryHandle;

		typedef bx::HandleHashMapT<MAX_CONFIG_MAX_UNIFORMS*2> UniformHashMap;
		UniformHashMap m_uniformHashMap;
		UniformRef     m_uniformRef[MAX_CONFIG_MAX_UNIFORMS];

		typedef bx::HandleHashMapT<MAX_CONFIG_MAX_SHADERS*2> ShaderHashMap;
		ShaderHashMap m_shaderHashMap;
		ShaderRef     m_shaderRef[MAX_CONFIG_MAX_SHADERS];

		typedef bx::HandleHashMapT<MAX_CONFIG_MAX_PROGRAMS*2> ProgramHashMap;
		ProgramHashMap m_programHashMap;
		ProgramRef     m_programRef[MAX_CONFIG_MAX_PROGRAMS];

		typedef bx::HandleHashMapT<MAX_CONFIG_MAX_MESHES * 2> MeshHashMap;
		MeshHashMap m_meshHashMap;
		MeshRef		m_meshRef[MAX_CONFIG_MAX_MESHES];

		TextureRef      m_textureRef[MAX_CONFIG_MAX_TEXTURES];
		FrameBufferRef  m_frameBufferRef[MAX_CONFIG_MAX_FRAME_BUFFERS];
		EntityRef	    m_entityRef[MAX_CONFIG_MAX_ENTITIES];
		ComponentRef	m_componentRef[MAX_CONFIG_MAX_COMPONENTS];
		VertexLayoutRef m_vertexLayoutRef;

		MeshQuery m_meshQuery;
		EntityQuery m_entityQuery;

		ViewId m_viewRemap[MAX_CONFIG_MAX_VIEWS];
		uint32_t m_seq[MAX_CONFIG_MAX_VIEWS];
		View m_view[MAX_CONFIG_MAX_VIEWS];

		float m_clearColor[MAX_CONFIG_MAX_COLOR_PALETTE][4];

		uint8_t m_colorPaletteDirty;

		Init     m_init;
		int64_t  m_frameTimeLast;
		uint32_t m_frames;
		uint32_t m_debug;

		int64_t m_rtMemoryUsed;
		int64_t m_textureMemoryUsed;

		TextVideoMemBlitter m_textVideoMemBlitter;
		ClearQuad m_clearQuad;

		RendererContextI* m_renderCtx;
		PhysicsContextI* m_physicsCtx;

		typedef stl::unordered_map<stl::string, const InputBinding*> InputBindingMap;
		InputBindingMap m_inputBindingsMap;

		typedef stl::unordered_map<uint32_t, const InputMapping*> InputMappingMap;
		InputMappingMap m_inputMappingsMap;

		InputKeyboard m_keyboard;
		InputMouse m_mouse;
		Gamepad m_gamepad[MAX_CONFIG_MAX_GAMEPADS];

		struct Func
		{
			ConsoleFn m_fn;
			void* m_userData;
		};

		typedef stl::unordered_map<uint32_t, Func> CmdLookup;
		CmdLookup m_lookup;

		bool m_headless;
		bool m_rendererInitialized;
		bool m_exit;
		bool m_flipAfterRender;
		bool m_singleThreaded;
		bool m_flipped;

		typedef UpdateBatchT<256> TextureUpdateBatch;
		BX_ALIGN_DECL_CACHE_LINE(TextureUpdateBatch m_textureUpdateBatch);
	};

#undef MAX_API_FUNC

} // namespace max

#endif // MAX_P_H_HEADER_GUARD

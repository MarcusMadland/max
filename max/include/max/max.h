/*
 * Copyright 2024 Marcus Madland. All rights reserved.
 * License: https://github.com/marcusmadland/max/blob/main/LICENSE
 */

#ifndef MAX_H_HEADER_GUARD
#define MAX_H_HEADER_GUARD

#include <stdarg.h> // va_list
#include <stdint.h> // uint32_t
#include <stdlib.h> // NULL
#include <typeinfo> // typeid

#include <bx/bx.h>	   // macros
#include <bx/hash.h>   // hash
#include <bx/math.h>   // vec3
#include <bx/bounds.h> // shapes

#include "defines.h"

namespace bx { struct AllocatorI; }
namespace bimg { struct ImageContainer; }

extern "C" int _main_(int _argc, char** _argv);

///
#if 1
#define MAX_IMPLEMENT_MAIN(_app, ...)                 \
	int _main_(int _argc, char** _argv)                 \
	{                                                   \
			_app app(__VA_ARGS__);                      \
			return max::runApp(&app, _argc, _argv);   \
	}
#else
#define MAX_IMPLEMENT_MAIN(_app, ...) \
	_app s_ ## _app ## App(__VA_ARGS__)
#endif //

///
#define MAX_HANDLE(_name)                                                           \
	struct _name { uint16_t idx; };                                                  \
	inline bool isValid(_name _handle) { return max::kInvalidHandle != _handle.idx; }

#define MAX_INVALID_HANDLE { max::kInvalidHandle }

///
#define MAX_INPUT_BINDING_END { max::Key::None, max::Modifier::None, 0, NULL, NULL }
#define MAX_INPUT_MAPPING_END { UINT32_MAX, NULL }

/// MAX
namespace max
{
	/// Fatal error enum.
	///
	/// @attention C99's equivalent binding is `max_fatal_t`.
	///
	struct Fatal
	{
		enum Enum
		{
			DebugCheck,
			InvalidShader,
			UnableToInitialize,
			UnableToCreateTexture,
			DeviceLost,

			Count
		};
	};

	/// Renderer backend type enum.
	///
	struct RendererType
	{
		/// Renderer types:
		enum Enum
		{
			Noop,         //!< No rendering.
			Agc,          //!< AGC
			Direct3D11,   //!< Direct3D 11.0
			Direct3D12,   //!< Direct3D 12.0
			Gnm,          //!< GNM
			Metal,        //!< Metal
			Nvn,          //!< NVN
			OpenGLES,     //!< OpenGL ES 2.0+
			OpenGL,       //!< OpenGL 2.1+
			Vulkan,       //!< Vulkan

			Count
		};
	};

	/// Physics backend type enum.
	///
	struct PhysicsType
	{
		/// Renderer types:
		enum Enum
		{
			Noop, //!< No physics.
			Jolt, //!< Jolt

			Count
		};
	};

	/// Access mode enum.
	///
	/// @attention C99's equivalent binding is `max_access_t`.
	///
	struct Access
	{
		/// Access:
		enum Enum
		{
			Read,      //!< Read
			Write,     //!< Write
			ReadWrite, //!< Read and write

			Count
		};
	};

	/// Vertex attribute enum.
	///
	struct Attrib
	{
		/// Corresponds to vertex shader attribute.
		enum Enum
		{
			Position,  //!< a_position
			Normal,    //!< a_normal
			Tangent,   //!< a_tangent
			Bitangent, //!< a_bitangent
			Color0,    //!< a_color0
			Color1,    //!< a_color1
			Color2,    //!< a_color2
			Color3,    //!< a_color3
			Indices,   //!< a_indices
			Weight,    //!< a_weight
			TexCoord0, //!< a_texcoord0
			TexCoord1, //!< a_texcoord1
			TexCoord2, //!< a_texcoord2
			TexCoord3, //!< a_texcoord3
			TexCoord4, //!< a_texcoord4
			TexCoord5, //!< a_texcoord5
			TexCoord6, //!< a_texcoord6
			TexCoord7, //!< a_texcoord7

			Count
		};
	};

	/// Vertex attribute type enum.
	///
	struct AttribType
	{
		/// Attribute types:
		enum Enum
		{
			Uint8,  //!< Uint8
			Uint10, //!< Uint10, availability depends on: `MAX_CAPS_VERTEX_ATTRIB_UINT10`.
			Int16,  //!< Int16
			Half,   //!< Half, availability depends on: `MAX_CAPS_VERTEX_ATTRIB_HALF`.
			Float,  //!< Float

			Count
		};
	};

	/// Axis type enum.
	///
	struct Axis
	{
		enum Enum
		{
			X, //!< X Axis
			Y, //!< Y Axis
			Z, //!< Z Axis

			Count
		};
	};

	/// 
	struct MotionType
	{
		enum Enum
		{
			Static,	   //!< Non movable
			Kinematic, //!< Movable using velocities only, does not respond to forces
			Dynamic    //!< Responds to forces as a normal physics object
		};
	};

	/// 
	struct LayerType
	{
		enum Enum
		{
			NonMoving, //!< Non moving only collides with moving
			Moving,    //!< Moving collides with everything

			Count
		};
	};

	///
	struct Activation
	{
		enum Enum
		{
			Activate,	 //!< Activate the body, making it part of the simulation
			DontActivate //!< Leave activation state as it is (will not deactivate an active body)
		};
	};

	///
	struct CollisionShape
	{
		enum Enum
		{
			Sphere,
			Box,
			Capsule,

			Count
		};
	};

	/// 
	struct GroundState
	{
		enum Enum
		{
			OnGround,	   //!< Character is on the ground and can move freely.
			OnSteepGround, //!< Character is on a slope that is too steep and can't climb up any further. The caller should start applying downward velocity if sliding from the slope is desired.
			NotSupported,  //!< Character is touching an object, but is not supported by it and should fall.
			InAir,		   //!< Character is in the air and is not touching anything.
		};
	};

	/// Texture format enum.
	///
	/// Notation:
	///
	///       RGBA16S
	///       ^   ^ ^
	///       |   | +-- [ ]Unorm
	///       |   |     [F]loat
	///       |   |     [S]norm
	///       |   |     [I]nt
	///       |   |     [U]int
	///       |   +---- Number of bits per component
	///       +-------- Components
	///
	/// @attention Availability depends on Caps (see: formats).
	///
	struct TextureFormat
	{
		/// Texture formats:
		enum Enum
		{
			BC1,          //!< DXT1 R5G6B5A1
			BC2,          //!< DXT3 R5G6B5A4
			BC3,          //!< DXT5 R5G6B5A8
			BC4,          //!< LATC1/ATI1 R8
			BC5,          //!< LATC2/ATI2 RG8
			BC6H,         //!< BC6H RGB16F
			BC7,          //!< BC7 RGB 4-7 bits per color channel, 0-8 bits alpha
			ETC1,         //!< ETC1 RGB8
			ETC2,         //!< ETC2 RGB8
			ETC2A,        //!< ETC2 RGBA8
			ETC2A1,       //!< ETC2 RGB8A1
			PTC12,        //!< PVRTC1 RGB 2BPP
			PTC14,        //!< PVRTC1 RGB 4BPP
			PTC12A,       //!< PVRTC1 RGBA 2BPP
			PTC14A,       //!< PVRTC1 RGBA 4BPP
			PTC22,        //!< PVRTC2 RGBA 2BPP
			PTC24,        //!< PVRTC2 RGBA 4BPP
			ATC,          //!< ATC RGB 4BPP
			ATCE,         //!< ATCE RGBA 8 BPP explicit alpha
			ATCI,         //!< ATCI RGBA 8 BPP interpolated alpha
			ASTC4x4,      //!< ASTC 4x4 8.0 BPP
			ASTC5x4,      //!< ASTC 5x4 6.40 BPP
			ASTC5x5,      //!< ASTC 5x5 5.12 BPP
			ASTC6x5,      //!< ASTC 6x5 4.27 BPP
			ASTC6x6,      //!< ASTC 6x6 3.56 BPP
			ASTC8x5,      //!< ASTC 8x5 3.20 BPP
			ASTC8x6,      //!< ASTC 8x6 2.67 BPP
			ASTC8x8,      //!< ASTC 8x8 2.00 BPP
			ASTC10x5,     //!< ASTC 10x5 2.56 BPP
			ASTC10x6,     //!< ASTC 10x6 2.13 BPP
			ASTC10x8,     //!< ASTC 10x8 1.60 BPP
			ASTC10x10,    //!< ASTC 10x10 1.28 BPP
			ASTC12x10,    //!< ASTC 12x10 1.07 BPP
			ASTC12x12,    //!< ASTC 12x12 0.89 BPP

			Unknown,      // Compressed formats above.

			R1,
			A8,
			R8,
			R8I,
			R8U,
			R8S,
			R16,
			R16I,
			R16U,
			R16F,
			R16S,
			R32I,
			R32U,
			R32F,
			RG8,
			RG8I,
			RG8U,
			RG8S,
			RG16,
			RG16I,
			RG16U,
			RG16F,
			RG16S,
			RG32I,
			RG32U,
			RG32F,
			RGB8,
			RGB8I,
			RGB8U,
			RGB8S,
			RGB9E5F,
			BGRA8,
			RGBA8,
			RGBA8I,
			RGBA8U,
			RGBA8S,
			RGBA16,
			RGBA16I,
			RGBA16U,
			RGBA16F,
			RGBA16S,
			RGBA32I,
			RGBA32U,
			RGBA32F,
			B5G6R5,
			R5G6B5,
			BGRA4,
			RGBA4,
			BGR5A1,
			RGB5A1,
			RGB10A2,
			RG11B10F,

			UnknownDepth, // Depth formats below.

			D16,
			D24,
			D24S8,
			D32,
			D16F,
			D24F,
			D32F,
			D0S8,

			Count
		};
	};

	///
	struct Orientation
	{
		///
		enum Enum
		{
			R0,
			R90,
			R180,
			R270,
			HFlip,
			HFlipR90,
			HFlipR270,
			VFlip,
		};
	};

	/// Uniform type enum.
	///
	struct UniformType
	{
		/// Uniform types:
		enum Enum
		{
			Sampler, //!< Sampler.
			End,     //!< Reserved, do not use.

			Vec4,    //!< 4 floats vector.
			Mat3,    //!< 3x3 matrix.
			Mat4,    //!< 4x4 matrix.

			Count
		};
	};

	/// Backbuffer ratio enum.
	///
	struct BackbufferRatio
	{
		/// Backbuffer ratios:
		enum Enum
		{
			Equal,     //!< Equal to backbuffer.
			Half,      //!< One half size of backbuffer.
			Quarter,   //!< One quarter size of backbuffer.
			Eighth,    //!< One eighth size of backbuffer.
			Sixteenth, //!< One sixteenth size of backbuffer.
			Double,    //!< Double size of backbuffer.

			Count
		};
	};

	/// Occlusion query result.
	///
	struct OcclusionQueryResult
	{
		/// Occlusion query results:
		enum Enum
		{
			Invisible, //!< Query failed test.
			Visible,   //!< Query passed test.
			NoResult,  //!< Query result is not available yet.

			Count
		};
	};

	/// Primitive topology.
	///
	struct Topology
	{
		/// Primitive topology:
		enum Enum
		{
			TriList,   //!< Triangle list.
			TriStrip,  //!< Triangle strip.
			LineList,  //!< Line list.
			LineStrip, //!< Line strip.
			PointList, //!< Point list.

			Count
		};
	};

	/// Topology conversion function.
	///
	struct TopologyConvert
	{
		/// Topology conversion functions:
		enum Enum
		{
			TriListFlipWinding,  //!< Flip winding order of triangle list.
			TriStripFlipWinding, //!< Flip winding order of trinagle strip.
			TriListToLineList,   //!< Convert triangle list to line list.
			TriStripToTriList,   //!< Convert triangle strip to triangle list.
			LineStripToLineList, //!< Convert line strip to line list.

			Count
		};
	};

	/// Topology sort order.
	///
	struct TopologySort
	{
		/// Topology sort order:
		enum Enum
		{
			DirectionFrontToBackMin, //!<
			DirectionFrontToBackAvg, //!<
			DirectionFrontToBackMax, //!<
			DirectionBackToFrontMin, //!<
			DirectionBackToFrontAvg, //!<
			DirectionBackToFrontMax, //!<
			DistanceFrontToBackMin,  //!<
			DistanceFrontToBackAvg,  //!<
			DistanceFrontToBackMax,  //!<
			DistanceBackToFrontMin,  //!<
			DistanceBackToFrontAvg,  //!<
			DistanceBackToFrontMax,  //!<

			Count
		};
	};

	/// View mode sets draw call sort order.
	///
	struct ViewMode
	{
		/// View modes:
		enum Enum
		{
			Default,         //!< Default sort order.
			Sequential,      //!< Sort in the same order in which submit calls were called.
			DepthAscending,  //!< Sort draw call depth in ascending order.
			DepthDescending, //!< Sort draw call depth in descending order.

			Count
		};
	};

	/// Native window handle type.
	///
	struct NativeWindowHandleType
	{
		 enum Enum
		 {
		       Default = 0, //!< Platform default handle type (X11 on Linux).
		       Wayland,     //!< Wayland.

		       Count
		 };
	};

	///
	struct MouseButton
	{
		enum Enum
		{
			None,
			Left,
			Middle,
			Right,

			Count
		};
	};

	///
	struct MouseAxis
	{
		enum Enum
		{
			X,
			Y,

			Count
		};
	};

	///
	struct GamepadAxis
	{
		enum Enum
		{
			LeftX,
			LeftY,
			LeftZ,
			RightX,
			RightY,
			RightZ,

			Count
		};
	};

	///
	struct Modifier
	{
		enum Enum
		{
			None = 0,
			LeftAlt = 0x01,
			RightAlt = 0x02,
			LeftCtrl = 0x04,
			RightCtrl = 0x08,
			LeftShift = 0x10,
			RightShift = 0x20,
			LeftMeta = 0x40,
			RightMeta = 0x80,
		};
	};

	///
	struct Key
	{
		enum Enum
		{
			None = 0,
			Esc,
			Return,
			Tab,
			Space,
			Backspace,
			Up,
			Down,
			Left,
			Right,
			Insert,
			Delete,
			Home,
			End,
			PageUp,
			PageDown,
			Print,
			Plus,
			Minus,
			LeftBracket,
			RightBracket,
			Semicolon,
			Quote,
			Comma,
			Period,
			Slash,
			Backslash,
			Tilde,
			F1,
			F2,
			F3,
			F4,
			F5,
			F6,
			F7,
			F8,
			F9,
			F10,
			F11,
			F12,
			NumPad0,
			NumPad1,
			NumPad2,
			NumPad3,
			NumPad4,
			NumPad5,
			NumPad6,
			NumPad7,
			NumPad8,
			NumPad9,
			Key0,
			Key1,
			Key2,
			Key3,
			Key4,
			Key5,
			Key6,
			Key7,
			Key8,
			Key9,
			KeyA,
			KeyB,
			KeyC,
			KeyD,
			KeyE,
			KeyF,
			KeyG,
			KeyH,
			KeyI,
			KeyJ,
			KeyK,
			KeyL,
			KeyM,
			KeyN,
			KeyO,
			KeyP,
			KeyQ,
			KeyR,
			KeyS,
			KeyT,
			KeyU,
			KeyV,
			KeyW,
			KeyX,
			KeyY,
			KeyZ,

			GamepadA,
			GamepadB,
			GamepadX,
			GamepadY,
			GamepadThumbL,
			GamepadThumbR,
			GamepadShoulderL,
			GamepadShoulderR,
			GamepadUp,
			GamepadDown,
			GamepadLeft,
			GamepadRight,
			GamepadBack,
			GamepadStart,
			GamepadGuide,

			Count
		};
	};

	///
	struct Suspend
	{
		enum Enum
		{
			WillSuspend,
			DidSuspend,
			WillResume,
			DidResume,

			Count
		};
	};

	static const uint16_t kInvalidHandle = UINT16_MAX;

	MAX_HANDLE(DynamicIndexBufferHandle)
	MAX_HANDLE(DynamicVertexBufferHandle)
	MAX_HANDLE(FrameBufferHandle)
	MAX_HANDLE(IndexBufferHandle)
	MAX_HANDLE(IndirectBufferHandle)
	MAX_HANDLE(OcclusionQueryHandle)
	MAX_HANDLE(ProgramHandle)
	MAX_HANDLE(ShaderHandle)
	MAX_HANDLE(TextureHandle)
	MAX_HANDLE(UniformHandle)
	MAX_HANDLE(VertexBufferHandle)
	MAX_HANDLE(VertexLayoutHandle)
	MAX_HANDLE(WindowHandle)
	MAX_HANDLE(GamepadHandle)
	MAX_HANDLE(MeshHandle)
	MAX_HANDLE(ComponentHandle)
	MAX_HANDLE(EntityHandle)
	MAX_HANDLE(BodyHandle)
		//MAX_HANDLE(TrueTypeHandle)		 
		//MAX_HANDLE(FontHandle)			 
		//MAX_HANDLE(SoundHandle)		 
		//MAX_HANDLE(EmitterHandle)		 
		//MAX_HANDLE(EmitterSpriteHandle) 

	/// App interface to implement a max game or application.
	///
	struct BX_NO_VTABLE AppI
	{
		///
		AppI(const char* _name);

		///
		virtual ~AppI() = 0;

		///
		virtual void init(int32_t _argc, const char* const* _argv, uint32_t _width, uint32_t _height) = 0;

		///
		virtual int  shutdown() = 0;

		///
		virtual bool update() = 0;

		///
		const char* getName() const;

		///
		AppI* getNext();

	private:
		BX_ALIGN_DECL(16, uintptr_t) m_internal[4];
	};

	/// Callback interface to implement application specific behavior.
	/// Cached items are currently used for OpenGL and Direct3D 12 binary
	/// shaders.
	///
	/// @remarks
	///   'fatal' and 'trace' callbacks can be called from any thread. Other
	///   callbacks are called from the render thread.
	///
	struct CallbackI
	{
		virtual ~CallbackI() = 0;

		/// This callback is called on unrecoverable errors.
		/// It's not safe to continue (Excluding _code `Fatal::DebugCheck`),
		/// inform the user and terminate the application.
		///
		/// @param[in] _filePath File path where fatal message was generated.
		/// @param[in] _line Line where fatal message was generated.
		/// @param[in] _code Fatal error code.
		/// @param[in] _str More information about error.
		///
		/// @remarks
		///   Not thread safe and it can be called from any thread.
		///
		virtual void fatal(
			  const char* _filePath
			, uint16_t _line
			, Fatal::Enum _code
			, const char* _str
			) = 0;

		/// Print debug message.
		///
		/// @param[in] _filePath File path where debug message was generated.
		/// @param[in] _line Line where debug message was generated.
		/// @param[in] _format `printf` style format.
		/// @param[in] _argList Variable arguments list initialized with
		///   `va_start`.
		///
		/// @remarks
		///   Not thread safe and it can be called from any thread.
		///
		virtual void traceVargs(
			  const char* _filePath
			, uint16_t _line
			, const char* _format
			, va_list _argList
			) = 0;

		/// Profiler region begin.
		///
		/// @param[in] _name Region name, contains dynamic string.
		/// @param[in] _abgr Color of profiler region.
		/// @param[in] _filePath File path where `profilerBegin` was called.
		/// @param[in] _line Line where `profilerBegin` was called.
		///
		/// @remarks
		///   Not thread safe and it can be called from any thread.
		///
		virtual void profilerBegin(
			  const char* _name
			, uint32_t _abgr
			, const char* _filePath
			, uint16_t _line
			) = 0;

		/// Profiler region begin with string literal name.
		///
		/// @param[in] _name Region name, contains string literal.
		/// @param[in] _abgr Color of profiler region.
		/// @param[in] _filePath File path where `profilerBeginLiteral` was called.
		/// @param[in] _line Line where `profilerBeginLiteral` was called.
		///
		/// @remarks
		///   Not thread safe and it can be called from any thread.
		///
		virtual void profilerBeginLiteral(
			  const char* _name
			, uint32_t _abgr
			, const char* _filePath
			, uint16_t _line
			) = 0;

		/// Profiler region end.
		///
		/// @remarks
		///   Not thread safe and it can be called from any thread.
		///
		virtual void profilerEnd() = 0;

		/// Returns the size of a cached item. Returns 0 if no cached item was
		/// found.
		///
		/// @param[in] _id Cache id.
		/// @returns Number of bytes to read.
		///
		virtual uint32_t cacheReadSize(uint64_t _id) = 0;

		/// Read cached item.
		///
		/// @param[in] _id Cache id.
		/// @param[in] _data Buffer where to read data.
		/// @param[in] _size Size of data to read.
		///
		/// @returns True if data is read.
		///
		virtual bool cacheRead(uint64_t _id, void* _data, uint32_t _size) = 0;

		/// Write cached item.
		///
		/// @param[in] _id Cache id.
		/// @param[in] _data Data to write.
		/// @param[in] _size Size of data to write.
		///
		virtual void cacheWrite(uint64_t _id, const void* _data, uint32_t _size) = 0;

		/// Screenshot captured. Screenshot format is always 4-byte BGRA.
		///
		/// @param[in] _filePath File path.
		/// @param[in] _width Image width.
		/// @param[in] _height Image height.
		/// @param[in] _pitch Number of bytes to skip between the start of
		///   each horizontal line of the image.
		/// @param[in] _data Image data.
		/// @param[in] _size Image size.
		/// @param[in] _yflip If true, image origin is bottom left.
		///
		virtual void screenShot(
			  const char* _filePath
			, uint32_t _width
			, uint32_t _height
			, uint32_t _pitch
			, const void* _data
			, uint32_t _size
			, bool _yflip
			) = 0;

		/// Called when a video capture begins.
		///
		/// @param[in] _width Image width.
		/// @param[in] _height Image height.
		/// @param[in] _pitch Number of bytes to skip between the start of
		///   each horizontal line of the image.
		/// @param[in] _format Texture format. See: `TextureFormat::Enum`.
		/// @param[in] _yflip If true, image origin is bottom left.
		///
		virtual void captureBegin(
			  uint32_t _width
			, uint32_t _height
			, uint32_t _pitch
			, TextureFormat::Enum _format
			, bool _yflip
			) = 0;

		/// Called when a video capture ends.
		///
		virtual void captureEnd() = 0;

		/// Captured frame.
		///
		/// @param[in] _data Image data.
		/// @param[in] _size Image size.
		///
		virtual void captureFrame(const void* _data, uint32_t _size) = 0;
	};

	inline CallbackI::~CallbackI()
	{
	}

	/// Platform data.
	///
	struct PlatformData
	{
		PlatformData();

		void* ndt;                         //!< Native display type (*nix specific).
		void* nwh;                         //!< Native window handle. If `NULL`, max will create a headless
		                                   ///  context/device, provided the rendering API supports it.
		void* context;                     //!< GL context, D3D device, or Vulkan device. If `NULL`, max
		                                   ///  will create context/device.
		void* backBuffer;                  //!< GL back-buffer, or D3D render target view. If `NULL` max will
		                                   ///  create back-buffer color surface.
		void* backBufferDS;                //!< Backbuffer depth/stencil. If `NULL`, max will create a back-buffer
		                                   ///  depth/stencil surface.
		NativeWindowHandleType::Enum type; //!< Handle type. Needed for platforms having more than one option.
	};

	/// Backbuffer resolution and reset parameters.
	///
	struct Resolution
	{
		Resolution();

		TextureFormat::Enum format; //!< Backbuffer format.
		uint32_t width;             //!< Backbuffer width.
		uint32_t height;            //!< Backbuffer height.
		uint32_t reset;             //!< Reset parameters.
		uint8_t  numBackBuffers;    //!< Number of back buffers.
		uint8_t  maxFrameLatency;   //!< Maximum frame latency.
		uint8_t  debugTextScale;    //!< Scale factor for debug text.
	};

	/// Initialization parameters used by `max::init`.
	///
	struct Init
	{
		Init();

		/// Select rendering backend. When set to RendererType::Count
		/// a default rendering backend will be selected appropriate to the platform.
		/// See: `max::RendererType`
		RendererType::Enum rendererType;

		/// Select physics backend. When set to RendererType::Count
		/// a default physics backend will be selected appropriate to the platform.
		/// See: `max::PhysicsType`
		PhysicsType::Enum physicsType;

		/// Vendor PCI ID. If set to `MAX_PCI_ID_NONE`, discrete and integrated
		/// GPUs will be prioritised.
		///   - `MAX_PCI_ID_NONE` - Auto-select adapter.
		///   - `MAX_PCI_ID_SOFTWARE_RASTERIZER` - Software rasterizer.
		///   - `MAX_PCI_ID_AMD` - AMD adapter.
		///   - `MAX_PCI_ID_APPLE` - Apple adapter.
		///   - `MAX_PCI_ID_INTEL` - Intel adapter.
		///   - `MAX_PCI_ID_NVIDIA` - NVIDIA adapter.
		///   - `MAX_PCI_ID_MICROSOFT` - Microsoft adapter.
		uint16_t vendorId;

		/// Device ID. If set to 0 it will select first device, or device with
		/// matching ID.
		uint16_t deviceId;

		uint64_t capabilities; //!< Capabilities initialization mask (default: UINT64_MAX).

		bool debug;   //!< Enable device for debugging.
		bool profile; //!< Enable device for profiling.

		/// Platform data.
		PlatformData platformData;

		/// Backbuffer resolution and reset parameters. See: `max::Resolution`.
		Resolution resolution;

		/// Configurable runtime limits parameters.
		///
		struct Limits
		{
			Limits();

			uint16_t maxEncoders;       //!< Maximum number of encoder threads.
			uint32_t minResourceCbSize; //!< Minimum resource command buffer size.
			uint32_t transientVbSize;   //!< Maximum transient vertex buffer size.
			uint32_t transientIbSize;   //!< Maximum transient index buffer size.
		};

		Limits limits; //!< Configurable runtime limits.

		/// Provide application specific callback interface.
		/// See: `max::CallbackI`
		CallbackI* callback;

		/// Custom allocator. When a custom allocator is not
		/// specified, max uses the CRT allocator. Bgfx assumes
		/// custom allocator is thread safe.
		bx::AllocatorI* allocator;
	};

	/// Memory release callback.
	///
	/// param[in] _ptr Pointer to allocated data.
	/// param[in] _userData User defined data if needed.
	///
	typedef void (*ReleaseFn)(void* _ptr, void* _userData);

	/// Memory must be obtained by calling `max::alloc`, `max::copy`, or `max::makeRef`.
	///
	/// @attention It is illegal to create this structure on stack and pass it to any max API.
	///
	struct Memory
	{
		Memory() = delete;

		uint8_t* data; //!< Pointer to data.
		uint32_t size; //!< Data size.
	};

	/// Input binding callback.
	///
	/// param[in] _userData User defined data if needed.
	///
	typedef void (*InputBindingFn)(const void* _userData);

	///
	struct InputBinding
	{
		void set(Key::Enum _key, uint8_t _modifiers, uint8_t _flags, InputBindingFn _fn, const void* _userData = NULL)
		{
			m_key = _key;
			m_modifiers = _modifiers;
			m_flags = _flags;
			m_fn = _fn;
			m_userData = _userData;
		}

		void end()
		{
			m_key = Key::None;
			m_modifiers = Modifier::None;
			m_flags = 0;
			m_fn = NULL;
			m_userData = NULL;
		}

		Key::Enum m_key;
		uint8_t m_modifiers;
		uint8_t m_flags;
		InputBindingFn m_fn;
		const void* m_userData;
	};

	///
	typedef float (*InputMappingFn)(const void* _userData);

	///
	struct InputMapping
	{
		uint32_t m_action;
		InputMappingFn m_fn;
		const void* m_userData;
	};

	/// Console command callback.
	///
	/// param[in] _userData User defined data if needed.
	///
	typedef int (*ConsoleFn)(void* _userData, int _argc, char const* const* _argv);

	/// Renderer capabilities.
	///
	struct Caps
	{
		/// Renderer backend type. See: `max::RendererType`
		RendererType::Enum rendererType;

		/// Supported functionality.
		///
		/// @attention See `MAX_CAPS_*` flags at https://bkaradzic.github.io/max/max.html#available-caps
		///
		uint64_t supported;

		uint16_t vendorId;         //!< Selected GPU vendor PCI id.
		uint16_t deviceId;         //!< Selected GPU device id.
		bool     homogeneousDepth; //!< True when NDC depth is in [-1, 1] range, otherwise its [0, 1].
		bool     originBottomLeft; //!< True when NDC origin is at bottom left.
		uint8_t  numGPUs;          //!< Number of enumerated GPUs.

		/// GPU info.
		///
		struct GPU
		{
			uint16_t vendorId; //!< Vendor PCI id. See `MAX_PCI_ID_*`.
			uint16_t deviceId; //!< Device id.
		};

		GPU gpu[4]; //!< Enumerated GPUs.

		/// Renderer runtime limits.
		///
		struct Limits
		{
			uint32_t maxDrawCalls;            //!< Maximum number of draw calls.
			uint32_t maxBlits;                //!< Maximum number of blit calls.
			uint32_t maxTextureSize;          //!< Maximum texture size.
			uint32_t maxTextureLayers;        //!< Maximum texture layers.
			uint32_t maxViews;                //!< Maximum number of views.
			uint32_t maxFrameBuffers;         //!< Maximum number of frame buffer handles.
			uint32_t maxFBAttachments;        //!< Maximum number of frame buffer attachments.
			uint32_t maxPrograms;             //!< Maximum number of program handles.
			uint32_t maxShaders;              //!< Maximum number of shader handles.
			uint32_t maxTextures;             //!< Maximum number of texture handles.
			uint32_t maxTextureSamplers;      //!< Maximum number of texture samplers.
			uint32_t maxComputeBindings;      //!< Maximum number of compute bindings.
			uint32_t maxVertexLayouts;        //!< Maximum number of vertex format layouts.
			uint32_t maxVertexStreams;        //!< Maximum number of vertex streams.
			uint32_t maxIndexBuffers;         //!< Maximum number of index buffer handles.
			uint32_t maxVertexBuffers;        //!< Maximum number of vertex buffer handles.
			uint32_t maxDynamicIndexBuffers;  //!< Maximum number of dynamic index buffer handles.
			uint32_t maxDynamicVertexBuffers; //!< Maximum number of dynamic vertex buffer handles.
			uint32_t maxUniforms;             //!< Maximum number of uniform handles.
			uint32_t maxOcclusionQueries;     //!< Maximum number of occlusion query handles.
			uint32_t maxEncoders;             //!< Maximum number of encoder threads.
			uint32_t minResourceCbSize;       //!< Minimum resource command buffer size.
			uint32_t transientVbSize;         //!< Maximum transient vertex buffer size.
			uint32_t transientIbSize;         //!< Maximum transient index buffer size.
		};

		Limits limits; //!< Renderer runtime limits.

		/// Supported texture format capabilities flags:
		///   - `MAX_CAPS_FORMAT_TEXTURE_NONE` - Texture format is not supported.
		///   - `MAX_CAPS_FORMAT_TEXTURE_2D` - Texture format is supported.
		///   - `MAX_CAPS_FORMAT_TEXTURE_2D_SRGB` - Texture as sRGB format is supported.
		///   - `MAX_CAPS_FORMAT_TEXTURE_2D_EMULATED` - Texture format is emulated.
		///   - `MAX_CAPS_FORMAT_TEXTURE_3D` - Texture format is supported.
		///   - `MAX_CAPS_FORMAT_TEXTURE_3D_SRGB` - Texture as sRGB format is supported.
		///   - `MAX_CAPS_FORMAT_TEXTURE_3D_EMULATED` - Texture format is emulated.
		///   - `MAX_CAPS_FORMAT_TEXTURE_CUBE` - Texture format is supported.
		///   - `MAX_CAPS_FORMAT_TEXTURE_CUBE_SRGB` - Texture as sRGB format is supported.
		///   - `MAX_CAPS_FORMAT_TEXTURE_CUBE_EMULATED` - Texture format is emulated.
		///   - `MAX_CAPS_FORMAT_TEXTURE_VERTEX` - Texture format can be used from vertex shader.
		///   - `MAX_CAPS_FORMAT_TEXTURE_IMAGE_READ` - Texture format can be used as image
		///     and read from.
		///   - `MAX_CAPS_FORMAT_TEXTURE_IMAGE_WRITE` - Texture format can be used as image
		///     and written to.
		///   - `MAX_CAPS_FORMAT_TEXTURE_FRAMEBUFFER` - Texture format can be used as frame
		///     buffer.
		///   - `MAX_CAPS_FORMAT_TEXTURE_FRAMEBUFFER_MSAA` - Texture format can be used as MSAA
		///     frame buffer.
		///   - `MAX_CAPS_FORMAT_TEXTURE_MSAA` - Texture can be sampled as MSAA.
		///   - `MAX_CAPS_FORMAT_TEXTURE_MIP_AUTOGEN` - Texture format supports auto-generated
		///     mips.
		uint16_t formats[TextureFormat::Count];
	};

	/// Transient index buffer.
	///
	struct TransientIndexBuffer
	{
		uint8_t* data;            //!< Pointer to data.
		uint32_t size;            //!< Data size.
		uint32_t startIndex;      //!< First index.
		IndexBufferHandle handle; //!< Index buffer handle.
		bool isIndex16;           //!< Index buffer format is 16-bits if true, otherwise it is 32-bit.
	};

	/// Transient vertex buffer.
	///
	struct TransientVertexBuffer
	{
		uint8_t* data;                      //!< Pointer to data.
		uint32_t size;                      //!< Data size.
		uint32_t startVertex;               //!< First vertex.
		uint16_t stride;                    //!< Vertex stride.
		VertexBufferHandle handle;          //!< Vertex buffer handle.
		VertexLayoutHandle layoutHandle;    //!< Vertex layout handle.
	};

	/// Instance data buffer info.
	///
	struct InstanceDataBuffer
	{
		uint8_t* data;             //!< Pointer to data.
		uint32_t size;             //!< Data size.
		uint32_t offset;           //!< Offset in vertex buffer.
		uint32_t num;              //!< Number of instances.
		uint16_t stride;           //!< Vertex buffer stride.
		VertexBufferHandle handle; //!< Vertex buffer object handle.
	};

	/// Texture info.
	///
	struct TextureInfo
	{
		TextureFormat::Enum format; //!< Texture format.
		uint32_t storageSize;       //!< Total amount of bytes required to store texture.
		uint16_t width;             //!< Texture width.
		uint16_t height;            //!< Texture height.
		uint16_t depth;             //!< Texture depth.
		uint16_t numLayers;         //!< Number of layers in texture array.
		uint8_t numMips;            //!< Number of MIP maps.
		uint8_t bitsPerPixel;       //!< Format bits per pixel.
		bool    cubeMap;            //!< Texture is cubemap.
	};

	/// Uniform info.
	///
	struct UniformInfo
	{
		char name[256];         //!< Uniform name.
		UniformType::Enum type; //!< Uniform type.
		uint16_t num;           //!< Number of elements in array.
	};

	/// Physics ground info.
	///
	struct GroundInfo
	{
		GroundInfo()
			: m_state(GroundState::InAir)
			, m_position({ 0.0f, 0.0f, 0.0f })
			, m_normal({ 0.0f, 0.0f, 0.0f })
			, m_velocity({ 0.0f, 0.0f, 0.0f })
		{}

		GroundState::Enum m_state;
		bx::Vec3 m_position;
		bx::Vec3 m_normal;
		bx::Vec3 m_velocity;
	};

	/// Frame buffer texture attachment info.
	///
	struct Attachment
	{
		/// Init attachment.
		///
		/// @param[in] _handle Render target texture handle.
		/// @param[in] _access Access. See `Access::Enum`.
		/// @param[in] _layer Cubemap side or depth layer/slice to use.
		/// @param[in] _numLayers Number of texture layer/slice(s) in array to use.
		/// @param[in] _mip Mip level.
		/// @param[in] _resolve Resolve flags. See: `MAX_RESOLVE_*`
		///
		void init(
			  TextureHandle _handle
			, Access::Enum _access = Access::Write
			, uint16_t _layer = 0
			, uint16_t _numLayers = 1
			, uint16_t _mip = 0
			, uint8_t _resolve = MAX_RESOLVE_AUTO_GEN_MIPS
			);

		Access::Enum  access; //!< Attachment access. See `Access::Enum`.
		TextureHandle handle; //!< Render target texture handle.
		uint16_t mip;         //!< Mip level.
		uint16_t layer;       //!< Cubemap side or depth layer/slice to use.
		uint16_t numLayers;   //!< Number of texture layer/slice(s) in array to use.
		uint8_t  resolve;     //!< Resolve flags. See: `MAX_RESOLVE_*`
	};

	/// Transform data.
	///
	struct Transform
	{
		float* data;  //!< Pointer to first 4x4 matrix.
		uint16_t num; //!< Number of matrices.
	};

	///
	struct MouseState
	{
		MouseState()
			: m_mx(0)
			, m_my(0)
			, m_mz(0)
		{
			for (uint32_t ii = 0; ii < MouseButton::Count; ++ii)
			{
				m_buttons[ii] = MouseButton::None;
			}
		}

		int32_t m_mx;
		int32_t m_my;
		int32_t m_mz;
		uint8_t m_buttons[MouseButton::Count];
	};

	///
	struct GamepadState
	{
		GamepadState()
		{
			bx::memSet(m_axis, 0, sizeof(m_axis));
		}

		int32_t m_axis[GamepadAxis::Count];
	};

	///
	struct WindowState
	{
		WindowState()
			: m_handle(MAX_INVALID_HANDLE)
			, m_width(0)
			, m_height(0)
			, m_nwh(NULL)
			, m_dropFile("")
		{
		}

		WindowHandle m_handle;
		uint32_t     m_width;
		uint32_t     m_height;
		MouseState   m_mouse;
		void*		 m_nwh;
		const char*  m_dropFile;
	};

	/// View id.
	typedef uint16_t ViewId;

	/// View stats.
	///
	struct ViewStats
	{
		char     name[256];      //!< View name.
		ViewId   view;           //!< View id.
		int64_t  cpuTimeBegin;   //!< CPU (submit) begin time.
		int64_t  cpuTimeEnd;     //!< CPU (submit) end time.
		int64_t  gpuTimeBegin;   //!< GPU begin time.
		int64_t  gpuTimeEnd;     //!< GPU end time.
		uint32_t gpuFrameNum;    //!< Frame which generated gpuTimeBegin, gpuTimeEnd.
	};

	/// Encoder stats.
	///
	struct EncoderStats
	{
		int64_t cpuTimeBegin; //!< Encoder thread CPU submit begin time.
		int64_t cpuTimeEnd;   //!< Encoder thread CPU submit end time.
	};

	/// Renderer statistics data.
	///
	/// @remarks All time values are high-resolution timestamps, while
	///   time frequencies define timestamps-per-second for that hardware.
	struct Stats
	{
		int64_t cpuTimeFrame;               //!< CPU time between two `max::frame` calls.
		int64_t cpuTimeBegin;               //!< Render thread CPU submit begin time.
		int64_t cpuTimeEnd;                 //!< Render thread CPU submit end time.
		int64_t cpuTimerFreq;               //!< CPU timer frequency. Timestamps-per-second

		int64_t gpuTimeBegin;               //!< GPU frame begin time.
		int64_t gpuTimeEnd;                 //!< GPU frame end time.
		int64_t gpuTimerFreq;               //!< GPU timer frequency.

		int64_t waitRender;                 //!< Time spent waiting for render backend thread to finish issuing
		                                    //!  draw commands to underlying graphics API.
		int64_t waitSubmit;                 //!< Time spent waiting for submit thread to advance to next frame.

		uint32_t numDraw;                   //!< Number of draw calls submitted.
		uint32_t numCompute;                //!< Number of compute calls submitted.
		uint32_t numBlit;                   //!< Number of blit calls submitted.
		uint32_t maxGpuLatency;             //!< GPU driver latency.
		uint32_t gpuFrameNum;               //<! Frame which generated gpuTimeBegin, gpuTimeEnd.

		uint16_t numDynamicIndexBuffers;    //!< Number of used dynamic index buffers.
		uint16_t numDynamicVertexBuffers;   //!< Number of used dynamic vertex buffers.
		uint16_t numFrameBuffers;           //!< Number of used frame buffers.
		uint16_t numIndexBuffers;           //!< Number of used index buffers.
		uint16_t numOcclusionQueries;       //!< Number of used occlusion queries.
		uint16_t numPrograms;               //!< Number of used programs.
		uint16_t numShaders;                //!< Number of used shaders.
		uint16_t numTextures;               //!< Number of used textures.
		uint16_t numUniforms;               //!< Number of used uniforms.
		uint16_t numVertexBuffers;          //!< Number of used vertex buffers.
		uint16_t numVertexLayouts;          //!< Number of used vertex layouts.

		int64_t textureMemoryUsed;          //!< Estimate of texture memory used.
		int64_t rtMemoryUsed;               //!< Estimate of render target memory used.
		int32_t transientVbUsed;            //!< Amount of transient vertex buffer used.
		int32_t transientIbUsed;            //!< Amount of transient index buffer used.

		uint32_t numPrims[Topology::Count]; //!< Number of primitives rendered.

		int64_t gpuMemoryMax;               //!< Maximum available GPU memory for application.
		int64_t gpuMemoryUsed;              //!< Amount of GPU memory used by the application.

		uint16_t width;                     //!< Backbuffer width in pixels.
		uint16_t height;                    //!< Backbuffer height in pixels.
		uint16_t textWidth;                 //!< Debug text width in characters.
		uint16_t textHeight;                //!< Debug text height in characters.

		uint16_t   numViews;                //!< Number of view stats.
		ViewStats* viewStats;               //!< Array of View stats.

		uint8_t       numEncoders;          //!< Number of encoders used during frame.
		EncoderStats* encoderStats;         //!< Array of encoder stats.
	};

	/// Encoders are used for submitting draw calls from multiple threads. Only one encoder
	/// per thread should be used. Use `max::begin()` to obtain an encoder for a thread.
	///
	struct Encoder
	{
		/// Sets a debug marker. This allows you to group graphics calls together for easy
		/// browsing in graphics debugging tools.
		///
		/// @param[in] _name Marker name.
		/// @param[in] _len Marker name length (if length is INT32_MAX, it's expected that _name
		///   is zero terminated string.
		///
		void setMarker(const char* _name, int32_t _len = INT32_MAX);

		/// Set render states for draw primitive.
		///
		/// @param[in] _state State flags. Default state for primitive type is
		///   triangles. See: `MAX_STATE_DEFAULT`.
		///   - `MAX_STATE_DEPTH_TEST_*` - Depth test function.
		///   - `MAX_STATE_BLEND_*` - See remark 1 about MAX_STATE_BLEND_FUNC.
		///   - `MAX_STATE_BLEND_EQUATION_*` - See remark 2.
		///   - `MAX_STATE_CULL_*` - Backface culling mode.
		///   - `MAX_STATE_WRITE_*` - Enable R, G, B, A or Z write.
		///   - `MAX_STATE_MSAA` - Enable hardware multisample antialiasing.
		///   - `MAX_STATE_PT_[TRISTRIP/LINES/POINTS]` - Primitive type.
		///
		/// @param[in] _rgba Sets blend factor used by `MAX_STATE_BLEND_FACTOR` and
		///   `MAX_STATE_BLEND_INV_FACTOR` blend modes.
		///
		/// @remarks
		///   1. To set up more complex states use:
		///      `MAX_STATE_ALPHA_REF(_ref)`,
		///      `MAX_STATE_POINT_SIZE(_size)`,
		///      `MAX_STATE_BLEND_FUNC(_src, _dst)`,
		///      `MAX_STATE_BLEND_FUNC_SEPARATE(_srcRGB, _dstRGB, _srcA, _dstA)`,
		///      `MAX_STATE_BLEND_EQUATION(_equation)`,
		///      `MAX_STATE_BLEND_EQUATION_SEPARATE(_equationRGB, _equationA)`
		///   2. `MAX_STATE_BLEND_EQUATION_ADD` is set when no other blend
		///      equation is specified.
		///
		void setState(
			  uint64_t _state
			, uint32_t _rgba = 0
			);

		/// Set condition for rendering.
		///
		/// @param[in] _handle Occlusion query handle.
		/// @param[in] _visible Render if occlusion query is visible.
		///
		void setCondition(
			  OcclusionQueryHandle _handle
			, bool _visible
			);

		/// Set stencil test state.
		///
		/// @param[in] _fstencil Front stencil state.
		/// @param[in] _bstencil Back stencil state. If back is set to `MAX_STENCIL_NONE`
		///   _fstencil is applied to both front and back facing primitives.
		///
		void setStencil(
			  uint32_t _fstencil
			, uint32_t _bstencil = MAX_STENCIL_NONE
			);

		/// Set scissor for draw primitive. To scissor for all primitives in
		/// view see `max::setViewScissor`.
		///
		/// @param[in] _x Position x from the left side of the window.
		/// @param[in] _y Position y from the top of the window.
		/// @param[in] _width Width of scissor region.
		/// @param[in] _height Height of scissor region.
		/// @returns Scissor cache index.
		///
		uint16_t setScissor(
			  uint16_t _x
			, uint16_t _y
			, uint16_t _width
			, uint16_t _height
			);

		/// Set scissor from cache for draw primitive.
		///
		/// @param[in] _cache Index in scissor cache.
		///   Pass UINT16_MAX to have primitive use view scissor instead.
		///
		void setScissor(uint16_t _cache = UINT16_MAX);

		/// Set model matrix for draw primitive. If it is not called, model will
		/// be rendered with identity model matrix.
		///
		/// @param[in] _mtx Pointer to first matrix in array.
		/// @param[in] _num Number of matrices in array.
		/// @returns Index into matrix cache in case the same model matrix has
		///   to be used for other draw primitive call.
		///
		uint32_t setTransform(
			  const void* _mtx
			, uint16_t _num = 1
			);

		/// Reserve `_num` matrices in internal matrix cache.
		///
		/// @param[in] _transform Pointer to `Transform` structure.
		/// @param[in] _num Number of matrices.
		/// @returns Index into matrix cache.
		///
		/// @attention Pointer returned can be modified until `max::frame` is called.
		///
		uint32_t allocTransform(
			  Transform* _transform
			, uint16_t _num
			);

		/// Set model matrix from matrix cache for draw primitive.
		///
		/// @param[in] _cache Index in matrix cache.
		/// @param[in] _num Number of matrices from cache.
		///
		void setTransform(
			  uint32_t _cache
			, uint16_t _num = 1
			);

		/// Set shader uniform parameter for draw primitive.
		///
		/// @param[in] _handle Uniform.
		/// @param[in] _value Pointer to uniform data.
		/// @param[in] _num Number of elements. Passing `UINT16_MAX` will
		///   use the _num passed on uniform creation.
		///
		void setUniform(
			  UniformHandle _handle
			, const void* _value
			, uint16_t _num = 1
			);

		/// Set index buffer for draw primitive.
		///
		/// @param[in] _handle Index buffer.
		///
		void setIndexBuffer(IndexBufferHandle _handle);

		/// Set index buffer for draw primitive.
		///
		/// @param[in] _handle Index buffer.
		/// @param[in] _firstIndex First index to render.
		/// @param[in] _numIndices Number of indices to render.
		///
		void setIndexBuffer(
			  IndexBufferHandle _handle
			, uint32_t _firstIndex
			, uint32_t _numIndices
			);

		/// Set index buffer for draw primitive.
		///
		/// @param[in] _handle Dynamic index buffer.
		///
		void setIndexBuffer(DynamicIndexBufferHandle _handle);

		/// Set index buffer for draw primitive.
		///
		/// @param[in] _handle Dynamic index buffer.
		/// @param[in] _firstIndex First index to render.
		/// @param[in] _numIndices Number of indices to render.
		///
		void setIndexBuffer(
			  DynamicIndexBufferHandle _handle
			, uint32_t _firstIndex
			, uint32_t _numIndices
			);

		/// Set index buffer for draw primitive.
		///
		/// @param[in] _tib Transient index buffer.
		///
		void setIndexBuffer(const TransientIndexBuffer* _tib);

		/// Set index buffer for draw primitive.
		///
		/// @param[in] _tib Transient index buffer.
		/// @param[in] _firstIndex First index to render.
		/// @param[in] _numIndices Number of indices to render.
		///
		void setIndexBuffer(
			  const TransientIndexBuffer* _tib
			, uint32_t _firstIndex
			, uint32_t _numIndices
			);

		/// Set vertex buffer for draw primitive.
		///
		/// @param[in] _stream Vertex stream.
		/// @param[in] _handle Vertex buffer.
		///
		void setVertexBuffer(
			  uint8_t _stream
			, VertexBufferHandle _handle
			);

		/// Set vertex buffer for draw primitive.
		///
		/// @param[in] _stream Vertex stream.
		/// @param[in] _handle Vertex buffer.
		/// @param[in] _startVertex First vertex to render.
		/// @param[in] _numVertices Number of vertices to render.
		/// @param[in] _layoutHandle Vertex layout for aliasing vertex buffer. If invalid handle is
		///   used, vertex layout used for creation of vertex buffer will be used.
		///
		void setVertexBuffer(
			  uint8_t _stream
			, VertexBufferHandle _handle
			, uint32_t _startVertex
			, uint32_t _numVertices
			, VertexLayoutHandle _layoutHandle = MAX_INVALID_HANDLE
			);

		/// Set vertex buffer for draw primitive.
		///
		/// @param[in] _stream Vertex stream.
		/// @param[in] _handle Dynamic vertex buffer.
		///
		void setVertexBuffer(
			  uint8_t _stream
			, DynamicVertexBufferHandle _handle
			);

		/// Set vertex buffer for draw primitive.
		///
		/// @param[in] _stream Vertex stream.
		/// @param[in] _handle Dynamic vertex buffer.
		/// @param[in] _startVertex First vertex to render.
		/// @param[in] _numVertices Number of vertices to render.
		/// @param[in] _layoutHandle Vertex layout for aliasing vertex buffer. If invalid handle is
		///   used, vertex layout used for creation of vertex buffer will be used.
		///
		void setVertexBuffer(
			  uint8_t _stream
			, DynamicVertexBufferHandle _handle
			, uint32_t _startVertex
			, uint32_t _numVertices
			, VertexLayoutHandle _layoutHandle = MAX_INVALID_HANDLE
			);

		/// Set vertex buffer for draw primitive.
		///
		/// @param[in] _stream Vertex stream.
		/// @param[in] _tvb Transient vertex buffer.
		///
		void setVertexBuffer(
			  uint8_t _stream
			, const TransientVertexBuffer* _tvb
			);

		/// Set vertex buffer for draw primitive.
		///
		/// @param[in] _stream Vertex stream.
		/// @param[in] _tvb Transient vertex buffer.
		/// @param[in] _startVertex First vertex to render.
		/// @param[in] _numVertices Number of vertices to render.
		/// @param[in] _layoutHandle Vertex layout for aliasing vertex buffer. If invalid handle is
		///   used, vertex layout used for creation of vertex buffer will be used.
		///
		void setVertexBuffer(
			  uint8_t _stream
			, const TransientVertexBuffer* _tvb
			, uint32_t _startVertex
			, uint32_t _numVertices
			, VertexLayoutHandle _layoutHandle = MAX_INVALID_HANDLE
			);

		/// Set number of vertices for auto generated vertices use in conjunction
		/// with gl_VertexID.
		///
		/// @param[in] _numVertices Number of vertices.
		///
		/// @attention Availability depends on: `MAX_CAPS_VERTEX_ID`.
		///
		void setVertexCount(uint32_t _numVertices);

		/// Set instance data buffer for draw primitive.
		///
		/// @param[in] _idb Transient instance data buffer.
		///
		void setInstanceDataBuffer(const InstanceDataBuffer* _idb);

		/// Set instance data buffer for draw primitive.
		///
		/// @param[in] _idb Transient instance data buffer.
		/// @param[in] _start First instance data.
		/// @param[in] _num Number of data instances.
		///
		void setInstanceDataBuffer(
			  const InstanceDataBuffer* _idb
			, uint32_t _start
			, uint32_t _num
			);

		/// Set instance data buffer for draw primitive.
		///
		/// @param[in] _handle Vertex buffer.
		/// @param[in] _start First instance data.
		/// @param[in] _num Number of data instances.
		///
		void setInstanceDataBuffer(
			  VertexBufferHandle _handle
			, uint32_t _start
			, uint32_t _num
			);

		/// Set instance data buffer for draw primitive.
		///
		/// @param[in] _handle Vertex buffer.
		/// @param[in] _start First instance data.
		/// @param[in] _num Number of data instances.
		///
		void setInstanceDataBuffer(
			  DynamicVertexBufferHandle _handle
			, uint32_t _start
			, uint32_t _num
			);

		/// Set number of instances for auto generated instances use in conjunction
		/// with gl_InstanceID.
		///
		/// @param[in] _numInstances Number of instances.
		///
		/// @attention Availability depends on: `MAX_CAPS_VERTEX_ID`.
		///
		void setInstanceCount(uint32_t _numInstances);

		/// Set texture stage for draw primitive.
		///
		/// @param[in] _stage Texture unit.
		/// @param[in] _sampler Program sampler.
		/// @param[in] _handle Texture handle.
		/// @param[in] _flags Texture sampling mode. Default value UINT32_MAX uses
		///   texture sampling settings from the texture.
		///   - `MAX_SAMPLER_[U/V/W]_[MIRROR/CLAMP]` - Mirror or clamp to edge wrap
		///     mode.
		///   - `MAX_SAMPLER_[MIN/MAG/MIP]_[POINT/ANISOTROPIC]` - Point or anisotropic
		///     sampling.
		///
		void setTexture(
			  uint8_t _stage
			, UniformHandle _sampler
			, TextureHandle _handle
			, uint32_t _flags = UINT32_MAX
			);

		/// Submit an empty primitive for rendering. Uniforms and draw state
		/// will be applied but no geometry will be submitted. Useful in cases
		/// when no other draw/compute primitive is submitted to view, but it's
		/// desired to execute clear view.
		///
		/// These empty draw calls will sort before ordinary draw calls.
		///
		/// @param[in] _id View id.
		///
		void touch(ViewId _id);

		/// Submit primitive for rendering.
		///
		/// @param[in] _id View id.
		/// @param[in] _program Program.
		/// @param[in] _depth Depth for sorting.
		/// @param[in] _flags Discard or preserve states. See `MAX_DISCARD_*`.
		///
		void submit(
			  ViewId _id
			, ProgramHandle _program
			, uint32_t _depth = 0
			, uint8_t _flags  = MAX_DISCARD_ALL
			);

		/// Submit primitive with occlusion query for rendering.
		///
		/// @param[in] _id View id.
		/// @param[in] _program Program.
		/// @param[in] _occlusionQuery Occlusion query.
		/// @param[in] _depth Depth for sorting.
		/// @param[in] _flags Discard or preserve states. See `MAX_DISCARD_*`.
		///
		void submit(
			  ViewId _id
			, ProgramHandle _program
			, OcclusionQueryHandle _occlusionQuery
			, uint32_t _depth = 0
			, uint8_t _flags  = MAX_DISCARD_ALL
			);

		/// Submit primitive for rendering with index and instance data info from
		/// indirect buffer.
		///
		/// @param[in] _id View id.
		/// @param[in] _program Program.
		/// @param[in] _indirectHandle Indirect buffer.
		/// @param[in] _start First element in indirect buffer.
		/// @param[in] _num Number of draws.
		/// @param[in] _depth Depth for sorting.
		/// @param[in] _flags Discard or preserve states. See `MAX_DISCARD_*`.
		///
		/// @attention Availability depends on: `MAX_CAPS_DRAW_INDIRECT`.
		///
		void submit(
			  ViewId _id
			, ProgramHandle _program
			, IndirectBufferHandle _indirectHandle
			, uint32_t _start = 0
			, uint32_t _num = 1
			, uint32_t _depth = 0
			, uint8_t _flags = MAX_DISCARD_ALL
			);

		/// Submit primitive for rendering with index and instance data info and
		/// draw count from indirect buffers.
		///
		/// @param[in] _id View id.
		/// @param[in] _program Program.
		/// @param[in] _indirectHandle Indirect buffer.
		/// @param[in] _start First element in indirect buffer.
		/// @param[in] _numHandle Buffer for number of draws. Must be created
		///   with `MAX_BUFFER_INDEX32` and `MAX_BUFFER_DRAW_INDIRECT`.
		/// @param[in] _numIndex Element in number buffer.
		/// @param[in] _numMax Max number of draws.
		/// @param[in] _depth Depth for sorting.
		/// @param[in] _flags Discard or preserve states. See `MAX_DISCARD_*`.
		///
		/// @attention Availability depends on: `MAX_CAPS_DRAW_INDIRECT_COUNT`.
		///
		void submit(
			  ViewId _id
			, ProgramHandle _program
			, IndirectBufferHandle _indirectHandle
			, uint32_t _start
			, IndexBufferHandle _numHandle
			, uint32_t _numIndex = 0
			, uint32_t _numMax = UINT32_MAX
			, uint32_t _depth = 0
			, uint8_t _flags = MAX_DISCARD_ALL
			);

		/// Set compute index buffer.
		///
		/// @param[in] _stage Compute stage.
		/// @param[in] _handle Index buffer handle.
		/// @param[in] _access Buffer access. See `Access::Enum`.
		///
		void setBuffer(
			  uint8_t _stage
			, IndexBufferHandle _handle
			, Access::Enum _access
			);

		/// Set compute vertex buffer.
		///
		/// @param[in] _stage Compute stage.
		/// @param[in] _handle Vertex buffer handle.
		/// @param[in] _access Buffer access. See `Access::Enum`.
		///
		void setBuffer(
			  uint8_t _stage
			, VertexBufferHandle _handle
			, Access::Enum _access
			);

		/// Set compute dynamic index buffer.
		///
		/// @param[in] _stage Compute stage.
		/// @param[in] _handle Dynamic index buffer handle.
		/// @param[in] _access Buffer access. See `Access::Enum`.
		///
		void setBuffer(
			  uint8_t _stage
			, DynamicIndexBufferHandle _handle
			, Access::Enum _access
			);

		/// Set compute dynamic vertex buffer.
		///
		/// @param[in] _stage Compute stage.
		/// @param[in] _handle Dynamic vertex buffer handle.
		/// @param[in] _access Buffer access. See `Access::Enum`.
		///
		void setBuffer(
			  uint8_t _stage
			, DynamicVertexBufferHandle _handle
			, Access::Enum _access
			);

		/// Set compute indirect buffer.
		///
		/// @param[in] _stage Compute stage.
		/// @param[in] _handle Indirect buffer handle.
		/// @param[in] _access Buffer access. See `Access::Enum`.
		///
		void setBuffer(
			  uint8_t _stage
			, IndirectBufferHandle _handle
			, Access::Enum _access
			);

		/// Set compute image from texture.
		///
		/// @param[in] _stage Texture unit.
		/// @param[in] _handle Texture handle.
		/// @param[in] _mip Mip level.
		/// @param[in] _access Texture access. See `Access::Enum`.
		/// @param[in] _format Texture format. See: `TextureFormat::Enum`.
		///
		void setImage(
			  uint8_t _stage
			, TextureHandle _handle
			, uint8_t _mip
			, Access::Enum _access
			, TextureFormat::Enum _format = TextureFormat::Count
			);

		/// Dispatch compute.
		///
		/// @param[in] _id View id.
		/// @param[in] _handle Compute program.
		/// @param[in] _numX Number of groups X.
		/// @param[in] _numY Number of groups Y.
		/// @param[in] _numZ Number of groups Z.
		/// @param[in] _flags Discard or preserve states. See `MAX_DISCARD_*`.
		///
		void dispatch(
			  ViewId _id
			, ProgramHandle _handle
			, uint32_t _numX = 1
			, uint32_t _numY = 1
			, uint32_t _numZ = 1
			, uint8_t _flags = MAX_DISCARD_ALL
			);

		/// Dispatch compute indirect.
		///
		/// @param[in] _id View id.
		/// @param[in] _handle Compute program.
		/// @param[in] _indirectHandle Indirect buffer.
		/// @param[in] _start First element in indirect buffer.
		/// @param[in] _num Number of dispatches.
		/// @param[in] _flags Discard or preserve states. See `MAX_DISCARD_*`.
		///
		void dispatch(
			  ViewId _id
			, ProgramHandle _handle
			, IndirectBufferHandle _indirectHandle
			, uint32_t _start = 0
			, uint32_t _num   = 1
			, uint8_t _flags  = MAX_DISCARD_ALL
			);

		/// Discard all previously set state for draw or compute call.
		///
		/// @param[in] _flags Draw/compute states to discard.
		///
		void discard(uint8_t _flags = MAX_DISCARD_ALL);

		/// Blit texture 2D region between two 2D textures.
		///
		/// @param[in] _id View id.
		/// @param[in] _dst Destination texture handle.
		/// @param[in] _dstX Destination texture X position.
		/// @param[in] _dstY Destination texture Y position.
		/// @param[in] _src Source texture handle.
		/// @param[in] _srcX Source texture X position.
		/// @param[in] _srcY Source texture Y position.
		/// @param[in] _width Width of region.
		/// @param[in] _height Height of region.
		///
		/// @attention Destination texture must be created with `MAX_TEXTURE_BLIT_DST` flag.
		/// @attention Availability depends on: `MAX_CAPS_TEXTURE_BLIT`.
		///
		void blit(
			  ViewId _id
			, TextureHandle _dst
			, uint16_t _dstX
			, uint16_t _dstY
			, TextureHandle _src
			, uint16_t _srcX = 0
			, uint16_t _srcY = 0
			, uint16_t _width = UINT16_MAX
			, uint16_t _height = UINT16_MAX
			);

		/// Blit texture region between two textures.
		///
		/// @param[in] _id View id.
		/// @param[in] _dst Destination texture handle.
		/// @param[in] _dstMip Destination texture mip level.
		/// @param[in] _dstX Destination texture X position.
		/// @param[in] _dstY Destination texture Y position.
		/// @param[in] _dstZ If texture is 2D this argument should be 0. If destination texture is cube
		///   this argument represents destination texture cube face. For 3D texture this argument
		///   represents destination texture Z position.
		/// @param[in] _src Source texture handle.
		/// @param[in] _srcMip Source texture mip level.
		/// @param[in] _srcX Source texture X position.
		/// @param[in] _srcY Source texture Y position.
		/// @param[in] _srcZ If texture is 2D this argument should be 0. If source texture is cube
		///   this argument represents source texture cube face. For 3D texture this argument
		///   represents source texture Z position.
		/// @param[in] _width Width of region.
		/// @param[in] _height Height of region.
		/// @param[in] _depth If texture is 3D this argument represents depth of region, otherwise it's
		///   unused.
		///
		/// @attention Destination texture must be created with `MAX_TEXTURE_BLIT_DST` flag.
		/// @attention Availability depends on: `MAX_CAPS_TEXTURE_BLIT`.
		/// @attention C99's equivalent binding is `max_encoder_blit`.
		///
		void blit(
			  ViewId _id
			, TextureHandle _dst
			, uint8_t _dstMip
			, uint16_t _dstX
			, uint16_t _dstY
			, uint16_t _dstZ
			, TextureHandle _src
			, uint8_t _srcMip = 0
			, uint16_t _srcX = 0
			, uint16_t _srcY = 0
			, uint16_t _srcZ = 0
			, uint16_t _width = UINT16_MAX
			, uint16_t _height = UINT16_MAX
			, uint16_t _depth = UINT16_MAX
			);

		///
		void dbgDrawBegin(uint16_t _viewId, bool _depthTestLess = true, max::Encoder* _encoder = nullptr);

		///
		void dbgDrawEnd();

		///
		void dbgDrawPush();

		///
		void dbgDrawPop();

		///
		void dbgDrawSetDepthTestLess(bool _depthTestLess);

		///
		void dbgDrawSetState(bool _depthTest, bool _depthWrite, bool _clockwise);

		///
		void dbgDrawSetColor(uint32_t _abgr);

		///
		void dbgDrawSetLod(uint8_t _lod);

		///
		void dbgDrawSetWireframe(bool _wireframe);

		///
		void dbgDrawSetStipple(bool _stipple, float _scale = 1.0f, float _offset = 0.0f);

		///
		void dbgDrawSetSpin(float _spin);

		///
		void dbgDrawSetTransform(const void* _mtx);

		///
		void dbgDrawSetTranslate(float _x, float _y, float _z);

		///
		void dbgDrawPushTransform(const void* _mtx);

		///
		void dbgDrawPopTransform();

		///
		void dbgDrawMoveTo(float _x, float _y, float _z = 0.0f);

		///
		void dbgDrawMoveTo(const bx::Vec3& _pos);

		///
		void dbgDrawLineTo(float _x, float _y, float _z = 0.0f);

		///
		void dbgDrawLineTo(const bx::Vec3& _pos);

		///
		void dbgDrawClose();

		///
		void dbgDraw(const bx::Aabb& _aabb);

		///
		void dbgDraw(const bx::Cylinder& _cylinder);

		///
		void dbgDraw(const bx::Capsule& _capsule);

		///
		void dbgDraw(const bx::Disk& _disk);

		///
		void dbgDraw(const bx::Obb& _obb);

		///
		void dbgDraw(const bx::Sphere& _sphere);

		///
		void dbgDraw(const bx::Triangle& _triangle);

		///
		void dbgDraw(const bx::Cone& _cone);

		///
		void dbgDrawLineList(uint32_t _numVertices, const bx::Vec3* _vertices, uint32_t _numIndices = 0, const uint16_t* _indices = NULL);

		///
		void dbgDrawTriList(uint32_t _numVertices, const bx::Vec3* _vertices, uint32_t _numIndices = 0, const uint16_t* _indices = NULL);

		///
		void dbgDrawFrustum(const void* _viewProj);

		///
		void dbgDrawArc(Axis::Enum _axis, float _x, float _y, float _z, float _radius, float _degrees);

		///
		void dbgDrawCircle(const bx::Vec3& _normal, const bx::Vec3& _center, float _radius, float _weight = 0.0f);

		///
		void dbgDrawCircle(Axis::Enum _axis, float _x, float _y, float _z, float _radius, float _weight = 0.0f);

		///
		void dbgDrawQuad(const bx::Vec3& _normal, const bx::Vec3& _center, float _size);

		///
		void dbgDrawQuad(max::TextureHandle _handle, const bx::Vec3& _normal, const bx::Vec3& _center, float _size);

		///
		void dbgDrawCone(const bx::Vec3& _from, const bx::Vec3& _to, float _radius);

		///
		void dbgDrawCylinder(const bx::Vec3& _from, const bx::Vec3& _to, float _radius);

		///
		void dbgDrawCapsule(const bx::Vec3& _from, const bx::Vec3& _to, float _radius);

		///
		void dbgDrawAxis(float _x, float _y, float _z, float _len = 1.0f, Axis::Enum _highlight = Axis::Count, float _thickness = 0.0f);

		///
		void dbgDrawGrid(const bx::Vec3& _normal, const bx::Vec3& _center, uint32_t _size = 20, float _step = 1.0f);

		///
		void dbgDrawGrid(Axis::Enum _axis, const bx::Vec3& _center, uint32_t _size = 20, float _step = 1.0f);

		///
		void dbgDrawOrb(float _x, float _y, float _z, float _radius, Axis::Enum _highlight = Axis::Count);
	};

	/// Vertex layout.
	///
	/// @attention C99's equivalent binding is `max_vertex_layout_t`.
	///
	struct VertexLayout
	{
		VertexLayout();

		/// Start VertexLayout.
		///
		/// @param[in] _renderer Renderer backend type. See: `max::RendererType`
		/// @returns Returns itself.
		///
		/// @attention C99's equivalent binding is `max_vertex_layout_begin`.
		///
		VertexLayout& begin(RendererType::Enum _renderer = RendererType::Noop);

		/// End VertexLayout.
		///
		/// @attention C99's equivalent binding is `max_vertex_layout_end`.
		///
		void end();

		/// Add attribute to VertexLayout.
		///
		/// @param[in] _attrib Attribute semantics. See: `max::Attrib`
		/// @param[in] _num Number of elements 1, 2, 3 or 4.
		/// @param[in] _type Element type.
		/// @param[in] _normalized When using fixed point AttribType (f.e. Uint8)
		///   value will be normalized for vertex shader usage. When normalized
		///   is set to true, AttribType::Uint8 value in range 0-255 will be
		///   in range 0.0-1.0 in vertex shader.
		/// @param[in] _asInt Packaging rule for vertexPack, vertexUnpack, and
		///   vertexConvert for AttribType::Uint8 and AttribType::Int16.
		///   Unpacking code must be implemented inside vertex shader.
		/// @returns Returns itself.
		///
		/// @remarks
		///   Must be called between begin/end.
		///
		/// @attention C99's equivalent binding is `max_vertex_layout_add`.
		///
		VertexLayout& add(
			  Attrib::Enum _attrib
			, uint8_t _num
			, AttribType::Enum _type
			, bool _normalized = false
			, bool _asInt = false
			);

		/// Skip _num bytes in vertex stream.
		///
		/// @returns Returns itself.
		///
		/// @attention C99's equivalent binding is `max_vertex_layout_skip`.
		///
		VertexLayout& skip(uint8_t _num);

		/// Decode attribute.
		///
		/// @attention C99's equivalent binding is `max_vertex_layout_decode`.
		///
		void decode(
			  Attrib::Enum _attrib
			, uint8_t& _num
			, AttribType::Enum& _type
			, bool& _normalized
			, bool& _asInt
			) const;

		/// Returns `true` if VertexLayout contains attribute.
		///
		/// @param[in] _attrib Attribute semantics. See: `max::Attrib`
		/// @returns True if VertexLayout contains attribute.
		///
		/// @attention C99's equivalent binding is `max_vertex_layout_has`.
		///
		bool has(Attrib::Enum _attrib) const { return UINT16_MAX != m_attributes[_attrib]; }

		/// Returns relative attribute offset from the vertex.
		///
		/// @param[in] _attrib Attribute semantics. See: `max::Attrib`
		/// @returns Relative attribute offset from the vertex.
		///
		uint16_t getOffset(Attrib::Enum _attrib) const { return m_offset[_attrib]; }

		/// Returns vertex stride.
		///
		/// @returns Vertex stride.
		///
		uint16_t getStride() const { return m_stride; }

		/// Returns size of vertex buffer for number of vertices.
		///
		/// @param[in] _num Number of vertices.
		/// @returns Size of vertex buffer for number of vertices.
		///
		uint32_t getSize(uint32_t _num) const { return _num*m_stride; }

		uint32_t m_hash;                      //!< Hash.
		uint16_t m_stride;                    //!< Stride.
		uint16_t m_offset[Attrib::Count];     //!< Attribute offsets.
		uint16_t m_attributes[Attrib::Count]; //!< Used attributes.
	};

	/// Mesh query.
	///
	struct MeshQuery
	{
		void alloc(uint32_t _num);
		void free();

		struct Data
		{
			uint32_t m_numVertices;
			uint8_t* m_vertices;
			uint32_t m_numIndices;
			uint32_t* m_indices;

		};

		Data* m_data;
		VertexBufferHandle* m_vertices;
		IndexBufferHandle* m_indices;
		
		uint32_t m_num;
	};

	/// Entity query.
	///
	struct EntityQuery
	{
		void alloc(uint32_t _num);
		void free();

		uint32_t m_num;		      //!< Number of queried entities.
		EntityHandle* m_entities; //!< List of queried entity handles.
	};

	/// Hash query.
	///
	struct HashQuery
	{
		void alloc(uint32_t _num);
		void free();

		uint32_t m_num;
		uint32_t* m_data;
	};

	/// System for each entity callback.
	///
	/// param[in] _entity Current entity in loop.
	/// param[in] _userData User defined data if needed.
	///
	typedef void (*SystemFn)(EntityHandle _entity, void* _userData);

	/// Entity system.
	///
	template<typename... Components>
	struct System
	{
		System()
			: m_num(0)
		{}

		uint32_t m_num;

		void each(uint32_t _max, SystemFn _func, void* _userData = NULL)
		{
			HashQuery m_query;
			m_query.alloc(_max);

			(hashComponentType<Components>(m_query), ...);

			EntityQuery* qr = queryEntities(m_query);
			if (qr == NULL)
			{
				return;
			}

			m_num = qr->m_num;

			for (uint32_t ii = 0; ii < qr->m_num; ++ii)
			{
				_func(qr->m_entities[ii], _userData);
			}

			m_query.free();
		}

		// @todo first() ?

	private:
		template<typename T>
		void hashComponentType(HashQuery& _hashes)
		{
			uint32_t hash = bx::hash<bx::HashMurmur2A>(typeid(T).name());
			_hashes.m_data[_hashes.m_num] = hash;
			++_hashes.m_num;
		}
	};

	/// Pack vertex attribute into vertex stream format.
	///
	/// @param[in] _input Value to be packed into vertex stream.
	/// @param[in] _inputNormalized True if input value is already normalized.
	/// @param[in] _attr Attribute to pack.
	/// @param[in] _layout Vertex stream layout.
	/// @param[in] _data Destination vertex stream where data will be packed.
	/// @param[in] _index Vertex index that will be modified.
	///
	/// @attention C99's equivalent binding is `max_vertex_pack`.
	///
	void vertexPack(
		  const float _input[4]
		, bool _inputNormalized
		, Attrib::Enum _attr
		, const VertexLayout& _layout
		, void* _data
		, uint32_t _index = 0
		);

	/// Unpack vertex attribute from vertex stream format.
	///
	/// @param[out] _output Result of unpacking.
	/// @param[in]  _attr Attribute to unpack.
	/// @param[in]  _layout Vertex stream layout.
	/// @param[in]  _data Source vertex stream from where data will be unpacked.
	/// @param[in]  _index Vertex index that will be unpacked.
	///
	/// @attention C99's equivalent binding is `max_vertex_unpack`.
	///
	void vertexUnpack(
		  float _output[4]
		, Attrib::Enum _attr
		, const VertexLayout& _layout
		, const void* _data
		, uint32_t _index = 0
		);

	/// Converts vertex stream data from one vertex stream format to another.
	///
	/// @param[in] _destLayout Destination vertex stream layout.
	/// @param[in] _destData Destination vertex stream.
	/// @param[in] _srcLayout Source vertex stream layout.
	/// @param[in] _srcData Source vertex stream data.
	/// @param[in] _num Number of vertices to convert from source to destination.
	///
	/// @attention C99's equivalent binding is `max_vertex_convert`.
	///
	void vertexConvert(
		  const VertexLayout& _destLayout
		, void* _destData
		, const VertexLayout& _srcLayout
		, const void* _srcData
		, uint32_t _num = 1
		);

	/// Weld vertices.
	///
	/// @param[in] _output Welded vertices remapping table. The size of buffer
	///   must be the same as number of vertices.
	/// @param[in] _layout Vertex stream layout.
	/// @param[in] _data Vertex stream.
	/// @param[in] _num Number of vertices in vertex stream.
	/// @param[in] _index32 Set to `true` if input indices are 32-bit.
	/// @param[in] _epsilon Error tolerance for vertex position comparison.
	/// @returns Number of unique vertices after vertex welding.
	///
	/// @attention C99's equivalent binding is `max_weld_vertices`.
	///
	uint32_t weldVertices(
		  void* _output
		, const VertexLayout& _layout
		, const void* _data
		, uint32_t _num
		, bool _index32
		, float _epsilon = 0.001f
		);

	/// Convert index buffer for use with different primitive topologies.
	///
	/// @param[in] _conversion Conversion type, see `TopologyConvert::Enum`.
	/// @param[in] _dst Destination index buffer. If this argument is NULL
	///    function will return number of indices after conversion.
	/// @param[in] _dstSize Destination index buffer in bytes. It must be
	///    large enough to contain output indices. If destination size is
	///    insufficient index buffer will be truncated.
	/// @param[in] _indices Source indices.
	/// @param[in] _numIndices Number of input indices.
	/// @param[in] _index32 Set to `true` if input indices are 32-bit.
	///
	/// @returns Number of output indices after conversion.
	///
	/// @attention C99's equivalent binding is `max_topology_convert`.
	///
	uint32_t topologyConvert(
		  TopologyConvert::Enum _conversion
		, void* _dst
		, uint32_t _dstSize
		, const void* _indices
		, uint32_t _numIndices
		, bool _index32
		);

	/// Sort indices.
	///
	/// @param[in] _sort Sort order, see `TopologySort::Enum`.
	/// @param[in] _dst Destination index buffer.
	/// @param[in] _dstSize Destination index buffer in bytes. It must be
	///    large enough to contain output indices. If destination size is
	///    insufficient index buffer will be truncated.
	/// @param[in] _dir Direction (vector must be normalized).
	/// @param[in] _pos Position.
	/// @param[in] _vertices Pointer to first vertex represented as
	///    float x, y, z. Must contain at least number of vertices
	///    referencende by index buffer.
	/// @param[in] _stride Vertex stride.
	/// @param[in] _indices Source indices.
	/// @param[in] _numIndices Number of input indices.
	/// @param[in] _index32 Set to `true` if input indices are 32-bit.
	///
	/// @attention C99's equivalent binding is `max_topology_sort_tri_list`.
	///
	void topologySortTriList(
		  TopologySort::Enum _sort
		, void* _dst
		, uint32_t _dstSize
		, const float _dir[3]
		, const float _pos[3]
		, const void* _vertices
		, uint32_t _stride
		, const void* _indices
		, uint32_t _numIndices
		, bool _index32
		);

	/// Returns supported backend API renderers.
	///
	/// @param[in] _max Maximum number of elements in _enum array.
	/// @param[inout] _enum Array where supported renderers will be written.
	///
	/// @returns Number of supported renderers.
	///
	/// @attention C99's equivalent binding is `max_get_supported_renderers`.
	///
	uint8_t getSupportedRenderers(
		  uint8_t _max = 0
		, RendererType::Enum* _enum = NULL
		);

	/// Returns name of renderer.
	///
	/// @attention C99's equivalent binding is `max_get_renderer_name`.
	///
	const char* getRendererName(RendererType::Enum _type);

	/// Initialize the max library.
	///
	/// @param[in] _init Initialization parameters. See: `max::Init` for more info.
	///
	/// @returns `true` if initialization was successful.
	///
	/// @attention C99's equivalent binding is `max_init`.
	///
	bool init(const Init& _init = {});

	/// Shutdown max library.
	///
	/// @attention C99's equivalent binding is `max_shutdown`.
	///
	void shutdown();

	/// Reset graphic settings and back-buffer size.
	///
	/// @param[in] _width Back-buffer width.
	/// @param[in] _height Back-buffer height.
	/// @param[in] _flags See: `MAX_RESET_*` for more info.
	///   - `MAX_RESET_NONE` - No reset flags.
	///   - `MAX_RESET_FULLSCREEN` - Not supported yet.
	///   - `MAX_RESET_MSAA_X[2/4/8/16]` - Enable 2, 4, 8 or 16 x MSAA.
	///   - `MAX_RESET_VSYNC` - Enable V-Sync.
	///   - `MAX_RESET_MAXANISOTROPY` - Turn on/off max anisotropy.
	///   - `MAX_RESET_CAPTURE` - Begin screen capture.
	///   - `MAX_RESET_FLUSH_AFTER_RENDER` - Flush rendering after submitting to GPU.
	///   - `MAX_RESET_FLIP_AFTER_RENDER` - This flag  specifies where flip
	///     occurs. Default behavior is that flip occurs before rendering new
	///     frame. This flag only has effect when `MAX_CONFIG_MULTITHREADED=0`.
	///   - `MAX_RESET_SRGB_BACKBUFFER` - Enable sRGB back-buffer.
	/// @param[in] _format Texture format. See: `TextureFormat::Enum`.
	///
	/// @attention This call doesn’t change the window size, it just resizes
	///   the back-buffer. Your windowing code controls the window size.
	///
	/// @attention C99's equivalent binding is `max_reset`.
	///
	void reset(
		  uint32_t _width
		, uint32_t _height
		, uint32_t _flags = MAX_RESET_NONE
		, TextureFormat::Enum _format = TextureFormat::Count
		);

	/// Begin submitting draw calls from thread.
	///
	/// @param[in] _forThread Explicitly request an encoder for a worker thread.
	///
	Encoder* begin(bool _forThread = false);

	/// End submitting draw calls from thread.
	///
	void end(Encoder* _encoder);

	/// Advance to next frame. When using multithreaded renderer, this call
	/// just swaps internal buffers, kicks render thread, and returns. In
	/// singlethreaded renderer this call does frame rendering.
	///
	/// @param[in] _capture Capture frame with graphics debugger.
	///
	/// @returns Current frame number. This might be used in conjunction with
	///   double/multi buffering data outside the library and passing it to
	///   library via `max::makeRef` calls.
	///
	/// @attention C99's equivalent binding is `max_frame`.
	///
	uint32_t frame(bool _capture = false);

	/// Returns current renderer backend API type.
	///
	/// @remarks
	///   Library must be initialized.
	///
	/// @attention C99's equivalent binding is `max_get_renderer_type`.
	///
	RendererType::Enum getRendererType();

	/// Returns renderer capabilities.
	///
	/// @returns Pointer to static `max::Caps` structure.
	///
	/// @remarks
	///   Library must be initialized.
	///
	/// @attention C99's equivalent binding is `max_get_caps`.
	///
	const Caps* getCaps();

	/// Returns performance counters.
	///
	/// @attention Pointer returned is valid until `max::frame` is called.
	/// @attention C99's equivalent binding is `max_get_stats`.
	///
	const Stats* getStats();

	/// Returns max engine allocator.
	///
	bx::AllocatorI* getAllocator();

	/// Returns max engine delta time.
	///
	const float getDeltaTime();

	/// Allocate buffer to pass to max calls. Data will be freed inside max.
	///
	/// @param[in] _size Size to allocate.
	///
	/// @attention C99's equivalent binding is `max_alloc`.
	///
	const Memory* alloc(uint32_t _size);

	/// Allocate buffer and copy data into it. Data will be freed inside max.
	///
	/// @param[in] _data Pointer to data to be copied.
	/// @param[in] _size Size of data to be copied.
	///
	/// @attention C99's equivalent binding is `max_copy`.
	///
	const Memory* copy(
		  const void* _data
		, uint32_t _size
		);

	/// Make reference to data to pass to max. Unlike `max::alloc`, this call
	/// doesn't allocate memory for data. It just copies the _data pointer. You
	/// can pass `ReleaseFn` function pointer to release this memory after it's
	/// consumed, otherwise you must make sure _data is available for at least 2
	/// `max::frame` calls. `ReleaseFn` function must be able to be called
	/// from any thread.
	///
	/// @param[in] _data Pointer to data.
	/// @param[in] _size Size of data.
	/// @param[in] _releaseFn Callback function to release memory after use.
	/// @param[in] _userData User data to be passed to callback function.
	///
	/// @attention Data passed must be available for at least 2 `max::frame` calls.
	/// @attention C99's equivalent bindings are `max_make_ref`, `max_make_ref_release`.
	///
	const Memory* makeRef(
		  const void* _data
		, uint32_t _size
		, ReleaseFn _releaseFn = NULL
		, void* _userData = NULL
		);

	/// Set debug flags.
	///
	/// @param[in] _debug Available flags:
	///   - `MAX_DEBUG_IFH` - Infinitely fast hardware. When this flag is set
	///     all rendering calls will be skipped. This is useful when profiling
	///     to quickly assess potential bottlenecks between CPU and GPU.
	///   - `MAX_DEBUG_PROFILER` - Enable profiler.
	///   - `MAX_DEBUG_STATS` - Display internal statistics.
	///   - `MAX_DEBUG_TEXT` - Display debug text.
	///   - `MAX_DEBUG_WIREFRAME` - Wireframe rendering. All rendering
	///     primitives will be rendered as lines.
	///
	/// @attention C99's equivalent binding is `max_set_debug`.
	///
	void setDebug(uint32_t _debug);

	/// Clear internal debug text buffer.
	///
	/// @param[in] _attr Background color.
	/// @param[in] _small Default 8x16 or 8x8 font.
	///
	/// @attention C99's equivalent binding is `max_dbg_text_clear`.
	///
	void dbgTextClear(
		  uint8_t _attr = 0
		, bool _small = false
		);

	/// Print into internal debug text character-buffer (VGA-compatible text mode).
	///
	/// @param[in] _x, _y 2D position from top-left.
	/// @param[in] _attr Color palette. Where top 4-bits represent index of background, and bottom
	///   4-bits represent foreground color from standard VGA text palette (ANSI escape codes).
	/// @param[in] _format `printf` style format.
	///
	/// @attention C99's equivalent binding is `max_dbg_text_printf`.
	///
	void dbgTextPrintf(
		  uint16_t _x
		, uint16_t _y
		, uint8_t _attr
		, const char* _format
		, ...
		);

	/// Print into internal debug text character-buffer (VGA-compatible text mode).
	///
	/// @param[in] _x, _y 2D position from top-left.
	/// @param[in] _attr Color palette. Where top 4-bits represent index of background, and bottom
	///   4-bits represent foreground color from standard VGA text palette (ANSI escape codes).
	/// @param[in] _format `printf` style format.
	/// @param[in] _argList additional arguments for format string
	///
	/// @attention C99's equivalent binding is `max_dbg_text_vprintf`.
	///
	void dbgTextPrintfVargs(
		  uint16_t _x
		, uint16_t _y
		, uint8_t _attr
		, const char* _format
		, va_list _argList
		);

	/// Draw image into internal debug text buffer.
	///
	/// @param[in] _x, _y 2D position from top-left.
	/// @param[in] _width, _height  Image width and height.
	/// @param[in] _data  Raw image data (character/attribute raw encoding).
	/// @param[in] _pitch Image pitch in bytes.
	///
	/// @attention C99's equivalent binding is `max_dbg_text_image`.
	///
	void dbgTextImage(
		  uint16_t _x
		, uint16_t _y
		, uint16_t _width
		, uint16_t _height
		, const void* _data
		, uint16_t _pitch
		);

	///
	void dbgDrawBegin(uint16_t _viewId, bool _depthTestLess = true, max::Encoder* _encoder = nullptr);

	///
	void dbgDrawEnd();

	///
	void dbgDrawPush();

	///
	void dbgDrawPop();

	///
	void dbgDrawSetDepthTestLess(bool _depthTestLess);

	///
	void dbgDrawSetState(bool _depthTest, bool _depthWrite, bool _clockwise);

	///
	void dbgDrawSetColor(uint32_t _abgr);

	///
	void dbgDrawSetLod(uint8_t _lod);

	///
	void dbgDrawSetWireframe(bool _wireframe);

	///
	void dbgDrawSetStipple(bool _stipple, float _scale = 1.0f, float _offset = 0.0f);

	///
	void dbgDrawSetSpin(float _spin);

	///
	void dbgDrawSetTransform(const void* _mtx);

	///
	void dbgDrawSetTranslate(float _x, float _y, float _z);

	///
	void dbgDrawPushTransform(const void* _mtx);

	///
	void dbgDrawPopTransform();

	///
	void dbgDrawMoveTo(float _x, float _y, float _z = 0.0f);

	///
	void dbgDrawMoveTo(const bx::Vec3& _pos);

	///
	void dbgDrawLineTo(float _x, float _y, float _z = 0.0f);

	///
	void dbgDrawLineTo(const bx::Vec3& _pos);

	///
	void dbgDrawClose();

	///
	void dbgDraw(const bx::Aabb& _aabb);

	///
	void dbgDraw(const bx::Cylinder& _cylinder);

	///
	void dbgDraw(const bx::Capsule& _capsule);

	///
	void dbgDraw(const bx::Disk& _disk);

	///
	void dbgDraw(const bx::Obb& _obb);

	///
	void dbgDraw(const bx::Sphere& _sphere);

	///
	void dbgDraw(const bx::Triangle& _triangle);

	///
	void dbgDraw(const bx::Cone& _cone);

	///
	void dbgDrawLineList(uint32_t _numVertices, const bx::Vec3* _vertices, uint32_t _numIndices = 0, const uint16_t* _indices = NULL);

	///
	void dbgDrawTriList(uint32_t _numVertices, const bx::Vec3* _vertices, uint32_t _numIndices = 0, const uint16_t* _indices = NULL);

	///
	void dbgDrawFrustum(const void* _viewProj);

	///
	void dbgDrawArc(Axis::Enum _axis, float _x, float _y, float _z, float _radius, float _degrees);

	///
	void dbgDrawCircle(const bx::Vec3& _normal, const bx::Vec3& _center, float _radius, float _weight = 0.0f);

	///
	void dbgDrawCircle(Axis::Enum _axis, float _x, float _y, float _z, float _radius, float _weight = 0.0f);

	///
	void dbgDrawQuad(const bx::Vec3& _normal, const bx::Vec3& _center, float _size);

	///
	void dbgDrawQuad(max::TextureHandle _handle, const bx::Vec3& _normal, const bx::Vec3& _center, float _size);

	///
	void dbgDrawCone(const bx::Vec3& _from, const bx::Vec3& _to, float _radius);

	///
	void dbgDrawCylinder(const bx::Vec3& _from, const bx::Vec3& _to, float _radius);

	///
	void dbgDrawCapsule(const bx::Vec3& _from, const bx::Vec3& _to, float _radius);

	///
	void dbgDrawAxis(float _x, float _y, float _z, float _len = 1.0f, Axis::Enum _highlight = Axis::Count, float _thickness = 0.0f);

	///
	void dbgDrawGrid(const bx::Vec3& _normal, const bx::Vec3& _center, uint32_t _size = 20, float _step = 1.0f);

	///
	void dbgDrawGrid(Axis::Enum _axis, const bx::Vec3& _center, uint32_t _size = 20, float _step = 1.0f);

	///
	void dbgDrawOrb(float _x, float _y, float _z, float _radius, Axis::Enum _highlight = Axis::Count);

	///
	int runApp(AppI* _app, int _argc, const char* const* _argv);

	///
	AppI* getFirstApp();

	///
	uint32_t getNumApps();

	///
	WindowHandle createWindow(int32_t _x, int32_t _y, uint32_t _width, uint32_t _height, uint32_t _flags = MAX_WINDOW_FLAG_NONE, const char* _title = "");

	///
	void setWindowPos(WindowHandle _handle, int32_t _x, int32_t _y);

	///
	void setWindowSize(WindowHandle _handle, uint32_t _width, uint32_t _height);

	///
	void setWindowTitle(WindowHandle _handle, const char* _title);

	///
	void setWindowFlags(WindowHandle _handle, uint32_t _flags, bool _enabled);

	///
	void toggleFullscreen(WindowHandle _handle);

	///
	void setMouseLock(WindowHandle _handle, bool _lock);

	///
	void* getNativeWindowHandle(WindowHandle _handle);

	///
	void* getNativeDisplayHandle();

	///
	NativeWindowHandleType::Enum getNativeWindowHandleType();

	///
	bool processEvents(uint32_t& _width, uint32_t& _height, uint32_t& _debug, uint32_t& _reset, MouseState* _mouse = NULL);

	///
	bool processWindowEvents(WindowState& _state, uint32_t& _debug, uint32_t& _reset);

	///
	void destroyWindow(WindowHandle _handle);

	///
	void inputAddBindings(const char* _name, const InputBinding* _bindings);

	///
	void inputRemoveBindings(const char* _name);

	///
	void inputAddMappings(uint32_t _id, InputMapping* _mappings);

	///
	void inputRemoveMappings(uint32_t _id);

	///
	void inputProcess();

	///
	float inputGetAsFloat(uint32_t _id, uint32_t _action);

	///
	bool inputGetAsBool(uint32_t _id, uint32_t _action);

	///
	void inputSetKeyState(Key::Enum  _key, uint8_t _modifiers, bool _down);

	///
	bool inputGetKeyState(Key::Enum _key, uint8_t* _modifiers = NULL);

	///
	uint8_t inputGetModifiersState();

	/// Adds single UTF-8 encoded character into input buffer.
	void inputChar(uint8_t _len, const uint8_t _char[4]);

	/// Returns single UTF-8 encoded character from input buffer.
	const uint8_t* inputGetChar();

	/// Flush internal input buffer.
	void inputCharFlush();

	///
	void inputSetMouseResolution(uint16_t _width, uint16_t _height);

	///
	void inputSetMousePos(int32_t _mx, int32_t _my, int32_t _mz);

	///
	void inputSetMouseButtonState(MouseButton::Enum _button, uint8_t _state);

	///
	void inputSetMouseLock(bool _lock);

	///
	void inputGetMouse(float _mouse[3]);

	///
	bool inputIsMouseLocked();

	///
	void inputSetGamepadAxis(GamepadHandle _handle, GamepadAxis::Enum _axis, int32_t _value);

	///
	int32_t inputGetGamepadAxis(GamepadHandle _handle, GamepadAxis::Enum _axis);

	/// Create static index buffer.
	///
	/// @param[in] _mem Index buffer data.
	/// @param[in] _flags Buffer creation flags.
	///   - `MAX_BUFFER_NONE` - No flags.
	///   - `MAX_BUFFER_COMPUTE_READ` - Buffer will be read from by compute shader.
	///   - `MAX_BUFFER_COMPUTE_WRITE` - Buffer will be written into by compute shader. When buffer
	///       is created with `MAX_BUFFER_COMPUTE_WRITE` flag it cannot be updated from CPU.
	///   - `MAX_BUFFER_COMPUTE_READ_WRITE` - Buffer will be used for read/write by compute shader.
	///   - `MAX_BUFFER_ALLOW_RESIZE` - Buffer will resize on buffer update if a different amount of
	///       data is passed. If this flag is not specified, and more data is passed on update, the buffer
	///       will be trimmed to fit the existing buffer size. This flag has effect only on dynamic
	///       buffers.
	///   - `MAX_BUFFER_INDEX32` - Buffer is using 32-bit indices. This flag has effect only on
	///       index buffers.
	///
	/// @attention C99's equivalent binding is `max_create_index_buffer`.
	///
	IndexBufferHandle createIndexBuffer(
		  const Memory* _mem
		, uint16_t _flags = MAX_BUFFER_NONE
		);

	/// Set static index buffer debug name.
	///
	/// @param[in] _handle Static index buffer handle.
	/// @param[in] _name Static index buffer name.
	/// @param[in] _len Static index buffer name length (if length is INT32_MAX, it's expected
	///   that _name is zero terminated string.
	///
	/// @attention C99's equivalent binding is `max_set_index_buffer_name`.
	///
	void setName(
		  IndexBufferHandle _handle
		, const char* _name
		, int32_t _len = INT32_MAX
		);

	/// Destroy static index buffer.
	///
	/// @param[in] _handle Static index buffer handle.
	///
	/// @attention C99's equivalent binding is `max_destroy_index_buffer`.
	///
	void destroy(IndexBufferHandle _handle);

	/// Create vertex layout.
	///
	/// @attention C99's equivalent binding is `max_create_vertex_layout`.
	///
	VertexLayoutHandle createVertexLayout(const VertexLayout& _layout);

	/// Destroy vertex layout.
	///
	/// @attention C99's equivalent binding is `max_destroy_vertex_layout`.
	///
	void destroy(VertexLayoutHandle _handle);

	/// Create static vertex buffer.
	///
	/// @param[in] _mem Vertex buffer data.
	/// @param[in] _layout Vertex layout.
	/// @param[in] _flags Buffer creation flags.
	///   - `MAX_BUFFER_NONE` - No flags.
	///   - `MAX_BUFFER_COMPUTE_READ` - Buffer will be read from by compute shader.
	///   - `MAX_BUFFER_COMPUTE_WRITE` - Buffer will be written into by compute shader. When buffer
	///       is created with `MAX_BUFFER_COMPUTE_WRITE` flag it cannot be updated from CPU.
	///   - `MAX_BUFFER_COMPUTE_READ_WRITE` - Buffer will be used for read/write by compute shader.
	///   - `MAX_BUFFER_ALLOW_RESIZE` - Buffer will resize on buffer update if a different amount of
	///       data is passed. If this flag is not specified, and more data is passed on update, the buffer
	///       will be trimmed to fit the existing buffer size. This flag has effect only on dynamic
	///       buffers.
	///   - `MAX_BUFFER_INDEX32` - Buffer is using 32-bit indices. This flag has effect only on
	///       index buffers.
	/// @returns Static vertex buffer handle.
	///
	/// @attention C99's equivalent binding is `max_create_vertex_buffer`.
	///
	VertexBufferHandle createVertexBuffer(
		  const Memory* _mem
		, const VertexLayout& _layout
		, uint16_t _flags = MAX_BUFFER_NONE
		);

	/// Set static vertex buffer debug name.
	///
	/// @param[in] _handle Static vertex buffer handle.
	/// @param[in] _name Static vertex buffer name.
	/// @param[in] _len Static vertex buffer name length (if length is INT32_MAX, it's expected
	///   that _name is zero terminated string.
	///
	/// @attention C99's equivalent binding is `max_set_vertex_buffer_name`.
	///
	void setName(
		  VertexBufferHandle _handle
		, const char* _name
		, int32_t _len = INT32_MAX
		);

	/// Destroy static vertex buffer.
	///
	/// @param[in] _handle Static vertex buffer handle.
	///
	/// @attention C99's equivalent binding is `max_destroy_vertex_buffer`.
	///
	void destroy(VertexBufferHandle _handle);

	/// Create empty dynamic index buffer.
	///
	/// @param[in] _num Number of indices.
	/// @param[in] _flags Buffer creation flags.
	///   - `MAX_BUFFER_NONE` - No flags.
	///   - `MAX_BUFFER_COMPUTE_READ` - Buffer will be read from by compute shader.
	///   - `MAX_BUFFER_COMPUTE_WRITE` - Buffer will be written into by compute shader. When buffer
	///       is created with `MAX_BUFFER_COMPUTE_WRITE` flag it cannot be updated from CPU.
	///   - `MAX_BUFFER_COMPUTE_READ_WRITE` - Buffer will be used for read/write by compute shader.
	///   - `MAX_BUFFER_ALLOW_RESIZE` - Buffer will resize on buffer update if a different amount of
	///       data is passed. If this flag is not specified, and more data is passed on update, the buffer
	///       will be trimmed to fit the existing buffer size. This flag has effect only on dynamic
	///       buffers.
	///   - `MAX_BUFFER_INDEX32` - Buffer is using 32-bit indices. This flag has effect only on
	///       index buffers.
	/// @returns Dynamic index buffer handle.
	///
	/// @attention C99's equivalent binding is `max_create_dynamic_index_buffer`.
	///
	DynamicIndexBufferHandle createDynamicIndexBuffer(
		  uint32_t _num
		, uint16_t _flags = MAX_BUFFER_NONE
		);

	/// Create a dynamic index buffer and initialize it.
	///
	/// @param[in] _mem Index buffer data.
	/// @param[in] _flags Buffer creation flags.
	///   - `MAX_BUFFER_NONE` - No flags.
	///   - `MAX_BUFFER_COMPUTE_READ` - Buffer will be read from by compute shader.
	///   - `MAX_BUFFER_COMPUTE_WRITE` - Buffer will be written into by compute shader. When buffer
	///       is created with `MAX_BUFFER_COMPUTE_WRITE` flag it cannot be updated from CPU.
	///   - `MAX_BUFFER_COMPUTE_READ_WRITE` - Buffer will be used for read/write by compute shader.
	///   - `MAX_BUFFER_ALLOW_RESIZE` - Buffer will resize on buffer update if a different amount of
	///       data is passed. If this flag is not specified, and more data is passed on update, the buffer
	///       will be trimmed to fit the existing buffer size. This flag has effect only on dynamic
	///       buffers.
	///   - `MAX_BUFFER_INDEX32` - Buffer is using 32-bit indices. This flag has effect only on
	///       index buffers.
	/// @returns Dynamic index buffer handle.
	///
	/// @attention C99's equivalent binding is `max_create_dynamic_index_buffer_mem`.
	///
	DynamicIndexBufferHandle createDynamicIndexBuffer(
		  const Memory* _mem
		, uint16_t _flags = MAX_BUFFER_NONE
		);

	/// Update dynamic index buffer.
	///
	/// @param[in] _handle Dynamic index buffer handle.
	/// @param[in] _startIndex Start index.
	/// @param[in] _mem Index buffer data.
	///
	/// @attention C99's equivalent binding is `max_update_dynamic_index_buffer`.
	///
	void update(
		  DynamicIndexBufferHandle _handle
		, uint32_t _startIndex
		, const Memory* _mem
		);

	/// Destroy dynamic index buffer.
	///
	/// @param[in] _handle Dynamic index buffer handle.
	///
	/// @attention C99's equivalent binding is `max_destroy_dynamic_index_buffer`.
	///
	void destroy(DynamicIndexBufferHandle _handle);

	/// Create empty dynamic vertex buffer.
	///
	/// @param[in] _num Number of vertices.
	/// @param[in] _layout Vertex layout.
	/// @param[in] _flags Buffer creation flags.
	///   - `MAX_BUFFER_NONE` - No flags.
	///   - `MAX_BUFFER_COMPUTE_READ` - Buffer will be read from by compute shader.
	///   - `MAX_BUFFER_COMPUTE_WRITE` - Buffer will be written into by compute shader. When buffer
	///       is created with `MAX_BUFFER_COMPUTE_WRITE` flag it cannot be updated from CPU.
	///   - `MAX_BUFFER_COMPUTE_READ_WRITE` - Buffer will be used for read/write by compute shader.
	///   - `MAX_BUFFER_ALLOW_RESIZE` - Buffer will resize on buffer update if a different amount of
	///       data is passed. If this flag is not specified, and more data is passed on update, the buffer
	///       will be trimmed to fit the existing buffer size. This flag has effect only on dynamic
	///       buffers.
	///   - `MAX_BUFFER_INDEX32` - Buffer is using 32-bit indices. This flag has effect only on
	///       index buffers.
	/// @returns Dynamic vertex buffer handle.
	///
	/// @attention C99's equivalent binding is `max_create_dynamic_vertex_buffer`.
	///
	DynamicVertexBufferHandle createDynamicVertexBuffer(
		  uint32_t _num
		, const VertexLayout& _layout
		, uint16_t _flags = MAX_BUFFER_NONE
		);

	/// Create dynamic vertex buffer and initialize it.
	///
	/// @param[in] _mem Vertex buffer data.
	/// @param[in] _layout Vertex layout.
	/// @param[in] _flags Buffer creation flags.
	///   - `MAX_BUFFER_NONE` - No flags.
	///   - `MAX_BUFFER_COMPUTE_READ` - Buffer will be read from by compute shader.
	///   - `MAX_BUFFER_COMPUTE_WRITE` - Buffer will be written into by compute shader. When buffer
	///       is created with `MAX_BUFFER_COMPUTE_WRITE` flag it cannot be updated from CPU.
	///   - `MAX_BUFFER_COMPUTE_READ_WRITE` - Buffer will be used for read/write by compute shader.
	///   - `MAX_BUFFER_ALLOW_RESIZE` - Buffer will resize on buffer update if a different amount of
	///       data is passed. If this flag is not specified, and more data is passed on update, the buffer
	///       will be trimmed to fit the existing buffer size. This flag has effect only on dynamic
	///       buffers.
	///   - `MAX_BUFFER_INDEX32` - Buffer is using 32-bit indices. This flag has effect only on
	///       index buffers.
	/// @returns Dynamic vertex buffer handle.
	///
	/// @attention C99's equivalent binding is `max_create_dynamic_vertex_buffer_mem`.
	///
	DynamicVertexBufferHandle createDynamicVertexBuffer(
		  const Memory* _mem
		, const VertexLayout& _layout
		, uint16_t _flags = MAX_BUFFER_NONE
		);

	/// Update dynamic vertex buffer.
	///
	/// @param[in] _handle Dynamic vertex buffer handle.
	/// @param[in] _startVertex Start vertex.
	/// @param[in] _mem Vertex buffer data.
	///
	/// @attention C99's equivalent binding is `max_update_dynamic_vertex_buffer`.
	///
	void update(
		  DynamicVertexBufferHandle _handle
		, uint32_t _startVertex
		, const Memory* _mem
		);

	/// Destroy dynamic vertex buffer.
	///
	/// @param[in] _handle Dynamic vertex buffer handle.
	///
	/// @attention C99's equivalent binding is `max_destroy_dynamic_vertex_buffer`.
	///
	void destroy(DynamicVertexBufferHandle _handle);

	/// Returns number of requested or maximum available indices.
	///
	/// @param[in] _num Number of required indices.
	/// @param[in] _index32 Set to `true` if input indices will be 32-bit.
	///
	/// @attention C99's equivalent binding is `max_get_avail_transient_index_buffer`.
	///
	uint32_t getAvailTransientIndexBuffer(
		  uint32_t _num
		, bool _index32 = false
		);

	/// Returns number of requested or maximum available vertices.
	///
	/// @param[in] _num Number of required vertices.
	/// @param[in] _layout Vertex layout.
	///
	/// @attention C99's equivalent binding is `max_get_avail_transient_vertex_buffer`.
	///
	uint32_t getAvailTransientVertexBuffer(
		  uint32_t _num
		, const VertexLayout& _layout
		);

	/// Returns number of requested or maximum available instance buffer slots.
	///
	/// @param[in] _num Number of required instances.
	/// @param[in] _stride Stride per instance.
	///
	/// @attention C99's equivalent binding is `max_get_avail_instance_data_buffer`.
	///
	uint32_t getAvailInstanceDataBuffer(
		  uint32_t _num
		, uint16_t _stride
		);

	/// Allocate transient index buffer.
	///
	/// @param[out] _tib TransientIndexBuffer structure will be filled, and will be valid
	///   for the duration of frame, and can be reused for multiple draw calls.
	/// @param[in] _num Number of indices to allocate.
	/// @param[in] _index32 Set to `true` if input indices will be 32-bit.
	///
	/// @attention C99's equivalent binding is `max_alloc_transient_index_buffer`.
	///
	void allocTransientIndexBuffer(
		  TransientIndexBuffer* _tib
		, uint32_t _num
		, bool _index32 = false
		);

	/// Allocate transient vertex buffer.
	///
	/// @param[out] _tvb TransientVertexBuffer structure will be filled, and will be valid
	///   for the duration of frame, and can be reused for multiple draw calls.
	/// @param[in] _num Number of vertices to allocate.
	/// @param[in] _layout Vertex layout.
	///
	/// @attention C99's equivalent binding is `max_alloc_transient_vertex_buffer`.
	///
	void allocTransientVertexBuffer(
		  TransientVertexBuffer* _tvb
		, uint32_t _num
		, const VertexLayout& _layout
		);

	/// Check for required space and allocate transient vertex and index
	/// buffers. If both space requirements are satisfied function returns
	/// true.
	///
	/// @param[out] _tvb TransientVertexBuffer structure will be filled, and will be valid
	///   for the duration of frame, and can be reused for multiple draw calls.
	/// @param[in] _layout Vertex layout.
	/// @param[in] _numVertices Number of vertices to allocate.
	/// @param[out] _tib TransientIndexBuffer structure will be filled, and will be valid
	///   for the duration of frame, and can be reused for multiple draw calls.
	/// @param[in] _numIndices Number of indices to allocate.
	/// @param[in] _index32 Set to `true` if input indices will be 32-bit.
	///
	/// @attention C99's equivalent binding is `max_alloc_transient_buffers`.
	///
	bool allocTransientBuffers(
		  TransientVertexBuffer* _tvb
		, const VertexLayout& _layout
		, uint32_t _numVertices
		, TransientIndexBuffer* _tib
		, uint32_t _numIndices
		, bool _index32 = false
		);

	/// Allocate instance data buffer.
	///
	/// @param[out] _idb InstanceDataBuffer structure will be filled, and will be valid
	///   for the duration of frame, and can be reused for multiple draw calls.
	/// @param[in] _num Number of instances.
	/// @param[in] _stride Instance stride. Must be multiple of 16.
	///
	/// @attention C99's equivalent binding is `max_alloc_instance_data_buffer`.
	///
	void allocInstanceDataBuffer(
		  InstanceDataBuffer* _idb
		, uint32_t _num
		, uint16_t _stride
		);

	/// Create draw indirect buffer.
	///
	/// @param[in] _num Number of indirect calls.
	/// @returns Indirect buffer handle.
	///
	/// @attention C99's equivalent binding is `max_create_indirect_buffer`.
	///
	IndirectBufferHandle createIndirectBuffer(uint32_t _num);

	/// Destroy draw indirect buffer.
	///
	/// @param[in] _handle Indirect buffer handle.
	///
	/// @attention C99's equivalent binding is `max_destroy_indirect_buffer`.
	///
	void destroy(IndirectBufferHandle _handle);

	/// Create shader from memory buffer.
	///
	/// @returns Shader handle.
	///
	/// @remarks
	///   Shader binary is obtained by compiling shader offline with shaderc command line tool.
	///
	/// @attention C99's equivalent binding is `max_create_shader`.
	///
	ShaderHandle createShader(const Memory* _mem);

	/// Load shader from name.
	///
	/// @param[in] _name Name of the shader binary.
	/// @returns Shader handle.
	///
	/// @remarks This assumes compiled paths based on renderer type. See examples.
	/// @remarks Shader binary is obtained by compiling shader offline with shaderc command line tool.
	///
	ShaderHandle loadShader(const char* _name);

	/// Returns the number of uniforms and uniform handles used inside a shader.
	///
	/// @param[in] _handle Shader handle.
	/// @param[in] _uniforms UniformHandle array where data will be stored.
	/// @param[in] _max Maximum capacity of array.
	/// @returns Number of uniforms used by shader.
	///
	/// @remarks
	///   Only non-predefined uniforms are returned.
	///
	/// @attention C99's equivalent binding is `max_get_shader_uniforms`.
	///
	uint16_t getShaderUniforms(
		  ShaderHandle _handle
		, UniformHandle* _uniforms = NULL
		, uint16_t _max = 0
		);

	/// Set shader debug name.
	///
	/// @param[in] _handle Shader handle.
	/// @param[in] _name Shader name.
	/// @param[in] _len Shader name length (if length is INT32_MAX, it's expected
	///   that _name is zero terminated string.
	///
	/// @attention C99's equivalent binding is `max_set_shader_name`.
	///
	void setName(
		  ShaderHandle _handle
		, const char* _name
		, int32_t _len = INT32_MAX
		);

	/// Destroy shader. Once a shader program is created with _handle,
	/// it is safe to destroy that shader.
	///
	/// @param[in] _handle Shader handle.
	///
	/// @attention C99's equivalent binding is `max_destroy_shader`.
	///
	void destroy(ShaderHandle _handle);

	/// Create program with vertex and fragment shaders.
	///
	/// @param[in] _vsh Vertex shader.
	/// @param[in] _fsh Fragment shader.
	/// @param[in] _destroyShaders If true, shaders will be destroyed when
	///   program is destroyed.
	/// @returns Program handle if vertex shader output and fragment shader
	///   input are matching, otherwise returns invalid program handle.
	///
	/// @attention C99's equivalent binding is `max_create_program`.
	///
	ProgramHandle createProgram(
		  ShaderHandle _vsh
		, ShaderHandle _fsh
		, bool _destroyShaders = false
		);

	/// Create program with compute shader.
	///
	/// @param[in] _csh Compute shader.
	/// @param[in] _destroyShader If true, shader will be destroyed when
	///   program is destroyed.
	/// @returns Program handle.
	///
	/// @attention C99's equivalent binding is `max_create_compute_program`.
	///
	ProgramHandle createProgram(
		  ShaderHandle _csh
		, bool _destroyShader = false
		);

	/// Load program with vertex and fragment shaders.
	///
	/// @param[in] _vsName Vertex shader name.
	/// @param[in] _fsName Fragment shader name.
	/// @returns Program handle if vertex shader output and fragment shader
	///   input are matching, otherwise returns invalid program handle.
	///
	/// @remarks This assumes compiled paths based on renderer type. See examples.
	/// 
	ProgramHandle loadProgram(
		const char* _vsName,
		const char* _fsName
		);

	/// Destroy program.
	///
	/// @param[in] _handle Program handle.
	///
	/// @attention C99's equivalent binding is `max_destroy_program`.
	///
	void destroy(ProgramHandle _handle);

	/// Validate texture parameters.
	///
	/// @param[in] _depth Depth dimension of volume texture.
	/// @param[in] _cubeMap Indicates that texture contains cubemap.
	/// @param[in] _numLayers Number of layers in texture array.
	/// @param[in] _format Texture format. See: `TextureFormat::Enum`.
	/// @param[in] _flags Texture flags. See `MAX_TEXTURE_*`.
	/// @returns True if a texture with the same parameters can be created.
	///
	/// @attention C99's equivalent binding is `max_is_texture_valid`.
	///
	bool isTextureValid(
		  uint16_t _depth
		, bool _cubeMap
		, uint16_t _numLayers
		, TextureFormat::Enum _format
		, uint64_t _flags
		);

	/// Validate frame buffer parameters.
	///
	/// @param[in] _num Number of attachments.
	/// @param[in] _attachment Attachment texture info. See: `max::Attachment`.
	///
	/// @returns True if a frame buffer with the same parameters can be created.
	///
	bool isFrameBufferValid(
		  uint8_t _num
		, const Attachment* _attachment
		);

	/// Calculate amount of memory required for texture.
	///
	/// @param[out] _info Resulting texture info structure. See: `TextureInfo`.
	/// @param[in] _width Width.
	/// @param[in] _height Height.
	/// @param[in] _depth Depth dimension of volume texture.
	/// @param[in] _cubeMap Indicates that texture contains cubemap.
	/// @param[in] _hasMips Indicates that texture contains full mip-map chain.
	/// @param[in] _numLayers Number of layers in texture array.
	/// @param[in] _format Texture format. See: `TextureFormat::Enum`.
	///
	/// @attention C99's equivalent binding is `max_calc_texture_size`.
	///
	void calcTextureSize(
		  TextureInfo& _info
		, uint16_t _width
		, uint16_t _height
		, uint16_t _depth
		, bool _cubeMap
		, bool _hasMips
		, uint16_t _numLayers
		, TextureFormat::Enum _format
		);

	/// Create texture from memory buffer.
	///
	/// @param[in] _mem DDS, KTX or PVR texture data.
	/// @param[in] _flags Texture creation (see `MAX_TEXTURE_*`.), and sampler (see `MAX_SAMPLER_*`)
	///   flags. Default texture sampling mode is linear, and wrap mode is repeat.
	///   - `MAX_SAMPLER_[U/V/W]_[MIRROR/CLAMP]` - Mirror or clamp to edge wrap
	///     mode.
	///   - `MAX_SAMPLER_[MIN/MAG/MIP]_[POINT/ANISOTROPIC]` - Point or anisotropic
	///     sampling.
	///
	/// @param[in] _skip Skip top level mips when parsing texture.
	/// @param[out] _info When non-`NULL` is specified it returns parsed texture information.
	/// @returns Texture handle.
	///
	/// @attention C99's equivalent binding is `max_create_texture`.
	///
	TextureHandle createTexture(
		  const Memory* _mem
		, uint64_t _flags = MAX_TEXTURE_NONE|MAX_SAMPLER_NONE
		, uint8_t _skip = 0
		, TextureInfo* _info = NULL
		);

	/// Create 2D texture.
	///
	/// @param[in] _width Width.
	/// @param[in] _height Height.
	/// @param[in] _hasMips Indicates that texture contains full mip-map chain.
	/// @param[in] _numLayers Number of layers in texture array. Must be 1 if caps
	///   `MAX_CAPS_TEXTURE_2D_ARRAY` flag is not set.
	/// @param[in] _format Texture format. See: `TextureFormat::Enum`.
	/// @param[in] _flags Texture creation (see `MAX_TEXTURE_*`.), and sampler (see `MAX_SAMPLER_*`)
	///   flags. Default texture sampling mode is linear, and wrap mode is repeat.
	///   - `MAX_SAMPLER_[U/V/W]_[MIRROR/CLAMP]` - Mirror or clamp to edge wrap
	///     mode.
	///   - `MAX_SAMPLER_[MIN/MAG/MIP]_[POINT/ANISOTROPIC]` - Point or anisotropic
	///     sampling.
	///
	/// @param[in] _mem Texture data. If `_mem` is non-NULL, created texture will be immutable. If
	///   `_mem` is NULL content of the texture is uninitialized. When `_numLayers` is more than
	///   1, expected memory layout is texture and all mips together for each array element.
	///
	/// @attention C99's equivalent binding is `max_create_texture_2d`.
	///
	TextureHandle createTexture2D(
		  uint16_t _width
		, uint16_t _height
		, bool     _hasMips
		, uint16_t _numLayers
		, TextureFormat::Enum _format
		, uint64_t _flags = MAX_TEXTURE_NONE|MAX_SAMPLER_NONE
		, const Memory* _mem = NULL
		);

	/// Create texture with size based on back-buffer ratio. Texture will maintain ratio
	/// if back buffer resolution changes.
	///
	/// @param[in] _ratio Frame buffer size in respect to back-buffer size. See:
	///   `BackbufferRatio::Enum`.
	/// @param[in] _hasMips Indicates that texture contains full mip-map chain.
	/// @param[in] _numLayers Number of layers in texture array. Must be 1 if caps
	///   `MAX_CAPS_TEXTURE_2D_ARRAY` flag is not set.
	/// @param[in] _format Texture format. See: `TextureFormat::Enum`.
	/// @param[in] _flags Texture creation (see `MAX_TEXTURE_*`.), and sampler (see `MAX_SAMPLER_*`)
	///   flags. Default texture sampling mode is linear, and wrap mode is repeat.
	///   - `MAX_SAMPLER_[U/V/W]_[MIRROR/CLAMP]` - Mirror or clamp to edge wrap
	///     mode.
	///   - `MAX_SAMPLER_[MIN/MAG/MIP]_[POINT/ANISOTROPIC]` - Point or anisotropic
	///     sampling.
	///
	/// @attention C99's equivalent binding is `max_create_texture_2d_scaled`.
	///
	TextureHandle createTexture2D(
		  BackbufferRatio::Enum _ratio
		, bool _hasMips
		, uint16_t _numLayers
		, TextureFormat::Enum _format
		, uint64_t _flags = MAX_TEXTURE_NONE|MAX_SAMPLER_NONE
		);

	/// Create 3D texture.
	///
	/// @param[in] _width Width.
	/// @param[in] _height Height.
	/// @param[in] _depth Depth.
	/// @param[in] _hasMips Indicates that texture contains full mip-map chain.
	/// @param[in] _format Texture format. See: `TextureFormat::Enum`.
	/// @param[in] _flags Texture creation (see `MAX_TEXTURE_*`.), and sampler (see `MAX_SAMPLER_*`)
	///   flags. Default texture sampling mode is linear, and wrap mode is repeat.
	///   - `MAX_SAMPLER_[U/V/W]_[MIRROR/CLAMP]` - Mirror or clamp to edge wrap
	///     mode.
	///   - `MAX_SAMPLER_[MIN/MAG/MIP]_[POINT/ANISOTROPIC]` - Point or anisotropic
	///     sampling.
	///
	/// @param[in] _mem Texture data. If `_mem` is non-NULL, created texture will be immutable. If
	///   `_mem` is NULL content of the texture is uninitialized.
	///
	/// @attention C99's equivalent binding is `max_create_texture_3d`.
	///
	TextureHandle createTexture3D(
		  uint16_t _width
		, uint16_t _height
		, uint16_t _depth
		, bool _hasMips
		, TextureFormat::Enum _format
		, uint64_t _flags = MAX_TEXTURE_NONE|MAX_SAMPLER_NONE
		, const Memory* _mem = NULL
		);

	/// Create Cube texture.
	///
	/// @param[in] _size Cube side size.
	/// @param[in] _hasMips Indicates that texture contains full mip-map chain.
	/// @param[in] _numLayers Number of layers in texture array. Must be 1 if caps
	///   `MAX_CAPS_TEXTURE_CUBE_ARRAY` flag is not set.
	/// @param[in] _format Texture format. See: `TextureFormat::Enum`.
	/// @param[in] _flags Texture creation (see `MAX_TEXTURE_*`.), and sampler (see `MAX_SAMPLER_*`)
	///   flags. Default texture sampling mode is linear, and wrap mode is repeat.
	///   - `MAX_SAMPLER_[U/V/W]_[MIRROR/CLAMP]` - Mirror or clamp to edge wrap
	///     mode.
	///   - `MAX_SAMPLER_[MIN/MAG/MIP]_[POINT/ANISOTROPIC]` - Point or anisotropic
	///     sampling.
	///
	/// @param[in] _mem Texture data. If `_mem` is non-NULL, created texture will be immutable. If
	///   `_mem` is NULL content of the texture is uninitialized. When `_numLayers` is more than
	///   1, expected memory layout is texture and all mips together for each array element.
	///
	/// @attention C99's equivalent binding is `max_create_texture_cube`.
	///
	TextureHandle createTextureCube(
		  uint16_t _size
		, bool _hasMips
		, uint16_t _numLayers
		, TextureFormat::Enum _format
		, uint64_t _flags = MAX_TEXTURE_NONE|MAX_SAMPLER_NONE
		, const Memory* _mem = NULL
		);

	/// Load texture from path.
	///
	/// @param[in] _filePath Path to DDS, KTX or PVR texture.
	/// @param[in] _flags Texture creation (see `MAX_TEXTURE_*`.), and sampler (see `MAX_SAMPLER_*`)
	///   flags. Default texture sampling mode is linear, and wrap mode is repeat.
	///   - `MAX_SAMPLER_[U/V/W]_[MIRROR/CLAMP]` - Mirror or clamp to edge wrap
	///     mode.
	///   - `MAX_SAMPLER_[MIN/MAG/MIP]_[POINT/ANISOTROPIC]` - Point or anisotropic
	///     sampling.
	///
	/// @param[in] _skip Skip top level mips when parsing texture.
	/// @param[out] _info When non-`NULL` is specified it returns parsed texture information.
	/// @param[out] _orientation When non-`NULL` is specified it returns texture orientation.
	/// @returns Texture handle.
	///
	TextureHandle loadTexture(
		const char* _filePath,
		uint64_t _flags = MAX_TEXTURE_NONE | MAX_SAMPLER_NONE,
		uint8_t _skip = 0,
		TextureInfo* _info = NULL,
		Orientation::Enum* _orientation = NULL
	);

	///// @todo Add comment
	bimg::ImageContainer* loadImage(const char* _filePath, TextureFormat::Enum _dstFormat);

	/// Update 2D texture.
	///
	/// @param[in] _handle Texture handle.
	/// @param[in] _layer Layers in texture array.
	/// @param[in] _mip Mip level.
	/// @param[in] _x X offset in texture.
	/// @param[in] _y Y offset in texture.
	/// @param[in] _width Width of texture block.
	/// @param[in] _height Height of texture block.
	/// @param[in] _mem Texture update data.
	/// @param[in] _pitch Pitch of input image (bytes). When _pitch is set to
	///   UINT16_MAX, it will be calculated internally based on _width.
	///
	/// @attention It's valid to update only mutable texture. See `max::createTexture2D` for more info.
	///
	/// @attention C99's equivalent binding is `max_update_texture_2d`.
	///
	void updateTexture2D(
		  TextureHandle _handle
		, uint16_t _layer
		, uint8_t _mip
		, uint16_t _x
		, uint16_t _y
		, uint16_t _width
		, uint16_t _height
		, const Memory* _mem
		, uint16_t _pitch = UINT16_MAX
		);

	/// Update 3D texture.
	///
	/// @param[in] _handle Texture handle.
	/// @param[in] _mip Mip level.
	/// @param[in] _x X offset in texture.
	/// @param[in] _y Y offset in texture.
	/// @param[in] _z Z offset in texture.
	/// @param[in] _width Width of texture block.
	/// @param[in] _height Height of texture block.
	/// @param[in] _depth Depth of texture block.
	/// @param[in] _mem Texture update data.
	///
	/// @attention It's valid to update only mutable texture. See `max::createTexture3D` for more info.
	///
	/// @attention C99's equivalent binding is `max_update_texture_3d`.
	///
	void updateTexture3D(
		  TextureHandle _handle
		, uint8_t _mip
		, uint16_t _x
		, uint16_t _y
		, uint16_t _z
		, uint16_t _width
		, uint16_t _height
		, uint16_t _depth
		, const Memory* _mem
		);

	/// Update Cube texture.
	///
	/// @param[in] _handle Texture handle.
	/// @param[in] _layer Layers in texture array.
	/// @param[in] _side Cubemap side `MAX_CUBE_MAP_<POSITIVE or NEGATIVE>_<X, Y or Z>`,
	///   where 0 is +X, 1 is -X, 2 is +Y, 3 is -Y, 4 is +Z, and 5 is -Z.
	///
	///                  +----------+
	///                  |-z       2|
	///                  | ^  +y    |
	///                  | |        |    Unfolded cube:
	///                  | +---->+x |
	///       +----------+----------+----------+----------+
	///       |+y       1|+y       4|+y       0|+y       5|
	///       | ^  -x    | ^  +z    | ^  +x    | ^  -z    |
	///       | |        | |        | |        | |        |
	///       | +---->+z | +---->+x | +---->-z | +---->-x |
	///       +----------+----------+----------+----------+
	///                  |+z       3|
	///                  | ^  -y    |
	///                  | |        |
	///                  | +---->+x |
	///                  +----------+
	///
	/// @param[in] _mip Mip level.
	/// @param[in] _x X offset in texture.
	/// @param[in] _y Y offset in texture.
	/// @param[in] _width Width of texture block.
	/// @param[in] _height Height of texture block.
	/// @param[in] _mem Texture update data.
	/// @param[in] _pitch Pitch of input image (bytes). When _pitch is set to
	///   UINT16_MAX, it will be calculated internally based on _width.
	///
	/// @attention It's valid to update only mutable texture. See `max::createTextureCube` for more info.
	///
	/// @attention C99's equivalent binding is `max_update_texture_cube`.
	///
	void updateTextureCube(
		  TextureHandle _handle
		, uint16_t _layer
		, uint8_t _side
		, uint8_t _mip
		, uint16_t _x
		, uint16_t _y
		, uint16_t _width
		, uint16_t _height
		, const Memory* _mem
		, uint16_t _pitch = UINT16_MAX
		);

	/// Read back texture content.
	///
	/// @param[in] _handle Texture handle.
	/// @param[in] _data Destination buffer.
	/// @param[in] _mip Mip level.
	///
	/// @returns Frame number when the result will be available. See: `max::frame`.
	///
	/// @attention Texture must be created with `MAX_TEXTURE_READ_BACK` flag.
	/// @attention Availability depends on: `MAX_CAPS_TEXTURE_READ_BACK`.
	/// @attention C99's equivalent binding is `max_read_texture`.
	///
	uint32_t readTexture(
		  TextureHandle _handle
		, void* _data
		, uint8_t _mip = 0
		);

	/// Set texture debug name.
	///
	/// @param[in] _handle Texture handle.
	/// @param[in] _name Texture name.
	/// @param[in] _len Texture name length (if length is INT32_MAX, it's expected
	///   that _name is zero terminated string.
	///
	/// @attention C99's equivalent binding is `max_set_texture_name`.
	///
	void setName(
		  TextureHandle _handle
		, const char* _name
		, int32_t _len = INT32_MAX
		);

	/// Returns texture direct access pointer.
	///
	/// @param[in] _handle Texture handle.
	///
	/// @returns Pointer to texture memory. If returned pointer is `NULL` direct access
	///   is not available for this texture. If pointer is `UINTPTR_MAX` sentinel value
	///   it means texture is pending creation. Pointer returned can be cached and it
	///   will be valid until texture is destroyed.
	///
	/// @attention Availability depends on: `MAX_CAPS_TEXTURE_DIRECT_ACCESS`. This feature
	///   is available on GPUs that have unified memory architecture (UMA) support.
	///
	/// @attention C99's equivalent binding is `max_get_direct_access_ptr`.
	///
	void* getDirectAccessPtr(TextureHandle _handle);

	/// Destroy texture.
	///
	/// @param[in] _handle Texture handle.
	///
	/// @attention C99's equivalent binding is `max_destroy_texture`.
	///
	void destroy(TextureHandle _handle);

	/// Create frame buffer (simple).
	///
	/// @param[in] _width Texture width.
	/// @param[in] _height Texture height.
	/// @param[in] _format Texture format. See: `TextureFormat::Enum`.
	/// @param[in] _textureFlags Texture creation (see `MAX_TEXTURE_*`.), and sampler (see `MAX_SAMPLER_*`)
	///   flags. Default texture sampling mode is linear, and wrap mode
	///   is repeat.
	///   - `MAX_SAMPLER_[U/V/W]_[MIRROR/CLAMP]` - Mirror or clamp to edge wrap
	///     mode.
	///   - `MAX_SAMPLER_[MIN/MAG/MIP]_[POINT/ANISOTROPIC]` - Point or anisotropic
	///     sampling.
	///
	/// @returns Handle to frame buffer object.
	///
	/// @attention C99's equivalent binding is `max_create_frame_buffer`.
	///
	FrameBufferHandle createFrameBuffer(
		  uint16_t _width
		, uint16_t _height
		, TextureFormat::Enum _format
		, uint64_t _textureFlags = MAX_SAMPLER_U_CLAMP|MAX_SAMPLER_V_CLAMP
		);

	/// Create frame buffer with size based on back-buffer ratio. Frame buffer will maintain ratio
	/// if back buffer resolution changes.
	///
	/// @param[in] _ratio Frame buffer size in respect to back-buffer size. See:
	///   `BackbufferRatio::Enum`.
	/// @param[in] _format Texture format. See: `TextureFormat::Enum`.
	/// @param[in] _textureFlags Texture creation (see `MAX_TEXTURE_*`.), and sampler (see `MAX_SAMPLER_*`)
	///   flags. Default texture sampling mode is linear, and wrap mode
	///   is repeat.
	///   - `MAX_SAMPLER_[U/V/W]_[MIRROR/CLAMP]` - Mirror or clamp to edge wrap
	///     mode.
	///   - `MAX_SAMPLER_[MIN/MAG/MIP]_[POINT/ANISOTROPIC]` - Point or anisotropic
	///     sampling.
	///
	/// @returns Handle to frame buffer object.
	///
	/// @attention C99's equivalent binding is `max_create_frame_buffer_scaled`.
	///
	FrameBufferHandle createFrameBuffer(
		  BackbufferRatio::Enum _ratio
		, TextureFormat::Enum _format
		, uint64_t _textureFlags = MAX_SAMPLER_U_CLAMP|MAX_SAMPLER_V_CLAMP
		);

	/// Create MRT frame buffer from texture handles (simple).
	///
	/// @param[in] _num Number of texture attachments.
	/// @param[in] _handles Texture attachments.
	/// @param[in] _destroyTextures If true, textures will be destroyed when
	///   frame buffer is destroyed.
	///
	/// @returns Handle to frame buffer object.
	///
	/// @attention C99's equivalent binding is `max_create_frame_buffer_from_handles`.
	///
	FrameBufferHandle createFrameBuffer(
		  uint8_t _num
		, const TextureHandle* _handles
		, bool _destroyTextures = false
		);

	/// Create MRT frame buffer from texture handles with specific layer and
	/// mip level.
	///
	/// @param[in] _num Number of texture attachments.
	/// @param[in] _attachment Attachment texture info. See: `max::Attachment`.
	/// @param[in] _destroyTextures If true, textures will be destroyed when
	///   frame buffer is destroyed.
	///
	/// @returns Handle to frame buffer object.
	///
	/// @attention C99's equivalent binding is `max_create_frame_buffer_from_attachment`.
	///
	FrameBufferHandle createFrameBuffer(
		  uint8_t _num
		, const Attachment* _attachment
		, bool _destroyTextures = false
		);

	/// Create frame buffer for multiple window rendering.
	///
	/// @param[in] _nwh OS' target native window handle.
	/// @param[in] _width Window back buffer width.
	/// @param[in] _height Window back buffer height.
	/// @param[in] _format Window back buffer color format.
	/// @param[in] _depthFormat Window back buffer depth format.
	///
	/// @returns Handle to frame buffer object.
	///
	/// @remarks
	///   Frame buffer cannot be used for sampling.
	///
	/// @attention C99's equivalent binding is `max_create_frame_buffer_from_nwh`.
	///
	FrameBufferHandle createFrameBuffer(
		  void* _nwh
		, uint16_t _width
		, uint16_t _height
		, TextureFormat::Enum _format      = TextureFormat::Count
		, TextureFormat::Enum _depthFormat = TextureFormat::Count
		);

	/// Set frame buffer debug name.
	///
	/// @param[in] _handle frame buffer handle.
	/// @param[in] _name frame buffer name.
	/// @param[in] _len frame buffer name length (if length is INT32_MAX, it's expected
	///   that _name is zero terminated string.
	///
	/// @attention C99's equivalent binding is `max_set_frame_buffer_name`.
	///
	void setName(
		  FrameBufferHandle _handle
		, const char* _name
		, int32_t _len = INT32_MAX
		);

	/// Obtain texture handle of frame buffer attachment.
	///
	/// @param[in] _handle Frame buffer handle.
	/// @param[in] _attachment Frame buffer attachment index.
	///
	/// @returns Returns invalid texture handle if attachment index is not
	///   correct, or frame buffer is created with native window handle.
	///
	/// @attention C99's equivalent binding is `max_get_texture`.
	///
	TextureHandle getTexture(
		  FrameBufferHandle _handle
		, uint8_t _attachment = 0
		);

	/// Destroy frame buffer.
	///
	/// @param[in] _handle Frame buffer handle.
	///
	/// @attention C99's equivalent binding is `max_destroy_frame_buffer`.
	///
	void destroy(FrameBufferHandle _handle);

	/// Create shader uniform parameter.
	///
	/// @param[in] _name Uniform name in shader.
	/// @param[in] _type Type of uniform (See: `max::UniformType`).
	/// @param[in] _num Number of elements in array.
	///
	/// @returns Handle to uniform object.
	///
	/// @remarks
	///   1. Uniform names are unique. It's valid to call `max::createUniform`
	///      multiple times with the same uniform name. The library will always
	///      return the same handle, but the handle reference count will be
	///      incremented. This means that the same number of `max::destroyUniform`
	///      must be called to properly destroy the uniform.
	///
	///   2. Predefined uniforms (declared in `max_shader.sh`):
	///      - `u_viewRect vec4(x, y, width, height)` - view rectangle for current
	///        view, in pixels.
	///      - `u_viewTexel vec4(1.0/width, 1.0/height, undef, undef)` - inverse
	///        width and height
	///      - `u_view mat4` - view matrix
	///      - `u_invView mat4` - inverted view matrix
	///      - `u_proj mat4` - projection matrix
	///      - `u_invProj mat4` - inverted projection matrix
	///      - `u_viewProj mat4` - concatenated view projection matrix
	///      - `u_invViewProj mat4` - concatenated inverted view projection matrix
	///      - `u_model mat4[MAX_CONFIG_MAX_BONES]` - array of model matrices.
	///      - `u_modelView mat4` - concatenated model view matrix, only first
	///        model matrix from array is used.
	///      - `u_modelViewProj mat4` - concatenated model view projection matrix.
	///      - `u_alphaRef float` - alpha reference value for alpha test.
	///
	/// @attention C99's equivalent binding is `max_create_uniform`.
	///
	UniformHandle createUniform(
		  const char* _name
		, UniformType::Enum _type
		, uint16_t _num = 1
		);

	/// Retrieve uniform info.
	///
	/// @param[in] _handle Handle to uniform object.
	/// @param[out] _info Uniform info.
	///
	/// @attention C99's equivalent binding is `max_get_uniform_info`.
	///
	void getUniformInfo(
		  UniformHandle _handle
		, UniformInfo& _info
		);

	/// Destroy shader uniform parameter.
	///
	/// @param[in] _handle Handle to uniform object.
	///
	void destroy(UniformHandle _handle);

	/// Create mesh from memory buffer.
	///
	/// @returns Mesh handle.
	///
	/// @remarks
	///   Mesh binary is obtained by compiling mesh offline with geometryc command line tool.
	///
	MeshHandle createMesh(const Memory* _mem, bool _ramcopy = false);

	/// Create mesh from vertices and indices buffers.
	///
	/// @param[in] _vertices Vertex buffer data.
	/// @param[in] _indices Index buffer data.
	/// @param[in] _layout Vertex layout.
	/// @returns Mesh handle.
	///
	MeshHandle createMesh(const Memory* _vertices, const Memory* _indices, const VertexLayout& _layout);

	/// Create mesh from path.
	///
	/// @param[in] _filePath Path of the geometry binary.
	/// @param[in] _ramcopy Should copy the memory.
	/// @returns Mesh handle.
	///
	/// @remarks
	///   Mesh binary is obtained by compiling mesh offline with geometryc command line tool.
	///
	MeshHandle loadMesh(const char* _filePath, bool _ramcopy = false);

	/// 
	MeshQuery* queryMesh(MeshHandle _handle);

	/// 
	const max::VertexLayout getLayout(MeshHandle _handle);

	/// Destroy mesh.
	///
	/// @param[in] _handle Handle to mesh object.
	///
	void destroy(MeshHandle _handle);

	/// Create component from data.
	///
	/// @param[in] _data Component data.
	/// @param[in] _size Size of component data.
	/// @param[in] _hash Unique id to identify component type.
	/// @returns Component handle.
	///
	/// @remarks
	///   It's prefered that templated functions are used for automated hash control.
	///
	ComponentHandle createComponent(void* _data, uint32_t _size);

	/// Create component from data.
	///
	/// @param[in] _data Component data.
	/// @returns Component handle.
	///
	template<typename T>
	ComponentHandle createComponent(T _data = T())
	{
		uint32_t size = sizeof(T);
		return createComponent(&_data, size);
	}

	/// Destroy component.
	///
	/// @param[in] _handle Handle to component object.
	///
	void destroy(ComponentHandle _component);

	/// Create an entity.
	///
	/// @param[in] _destroyComponents If true, components will be destroyed when
	///   entity is destroyed.
	/// @returns Entity handle.
	///
	EntityHandle createEntity(bool _destroyComponents = true);

	/// Add component to entity.
	/// @todo Make it so this doesnt need hash as input. Hash of component type can be stored per component?
	///
	/// @param[in] _entity Entity that should own component.
	/// @param[in] _component Component to add.
	/// @param[in] _hash Unique id to identify component type.
	/// 
	/// @remarks
	///   It's prefered that templated functions are used for automated hash control.
	///
	void addComponent(EntityHandle _entity, ComponentHandle _component, uint32_t _hash);

	/// 
	template<typename T>
	void addComponent(EntityHandle _entity, ComponentHandle _component)
	{
		uint32_t hash = bx::hash<bx::HashMurmur2A>(typeid(T).name());
		addComponent(_entity, _component, hash);
	}

	/// 
	void* getComponent(EntityHandle _handle, uint32_t _hash);

	///
	template<typename T>
	T* getComponent(EntityHandle _entity)
	{
		uint32_t hash = bx::hash<bx::HashMurmur2A>(typeid(T).name());
		return (T*)getComponent(_entity, hash);
	}

	///
	EntityQuery* queryEntities(const HashQuery& _hashes);

	/// Destroy entity.
	///
	/// @param[in] _handle Handle to component object.
	///
	void destroy(EntityHandle _entity);

	/// 
	BodyHandle createBody(
		CollisionShape::Enum _shape,
		const bx::Vec3& _pos,
		const bx::Quaternion& _quat,
		const bx::Vec3& _scale,
		LayerType::Enum _layer,
		MotionType::Enum _motion,
		Activation::Enum _activation,
		float _maxVelocity = 500.0f,
		uint8_t _flags = MAX_BODY_ALLOW_ALL
	);

	///
	BodyHandle createBodySphere(
		const bx::Vec3& _pos,
		const bx::Quaternion& _quat,
		const float _radius,
		LayerType::Enum _layer,
		MotionType::Enum _motion,
		Activation::Enum _activation,
		float _maxVelocity = 500.0f,
		uint8_t _flags = MAX_BODY_ALLOW_ALL
		);

	///
	BodyHandle createBodyBox(
		const bx::Vec3& _pos,
		const bx::Quaternion& _quat,
		const bx::Vec3& _scale,
		LayerType::Enum _layer,
		MotionType::Enum _motion,
		Activation::Enum _activation,
		float _maxVelocity = 500.0f,
		uint8_t _flags = MAX_BODY_ALLOW_ALL
		);

	///
	BodyHandle createBodyCapsule(
		const bx::Vec3& _pos,
		const bx::Quaternion& _quat,
		const float _radius,
		const float _halfHeight,
		LayerType::Enum _layer,
		MotionType::Enum _motion,
		Activation::Enum _activation,
		float _maxVelocity = 500.0f,
		uint8_t _flags = MAX_BODY_ALLOW_ALL
		);

	///
	void setPosition(BodyHandle _handle, const bx::Vec3& _pos, Activation::Enum _activation);

	///
	bx::Vec3 getPosition(BodyHandle _handle);

	///
	void setRotation(BodyHandle _handle, const bx::Quaternion& _rot, Activation::Enum _activation);

	///
	bx::Quaternion getRotation(BodyHandle _handle);

	///
	void setLinearVelocity(BodyHandle _handle, const bx::Vec3& _velocity);

	///
	bx::Vec3 getLinearVelocity(BodyHandle _handle);

	///
	void setAngularVelocity(BodyHandle _handle, const bx::Vec3& _angularVelocity);

	///
	bx::Vec3 getAngularVelocity(BodyHandle _handle);

	///
	void addLinearAndAngularVelocity(BodyHandle _handle, const bx::Vec3& _linearVelocity, const bx::Vec3& _angularVelocity);

	///
	void addLinearImpulse(BodyHandle _handle, const bx::Vec3& _impulse);

	///
	void addAngularImpulse(BodyHandle _handle, const bx::Vec3& _impulse);

	///
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
		);

	///
	void addForce(BodyHandle _handle, const bx::Vec3& _force, Activation::Enum _activation);

	///
	void addTorque(BodyHandle _handle, const bx::Vec3& _torque, Activation::Enum _activation);

	///
	void addMovement(BodyHandle _handle, const bx::Vec3& _position, const bx::Quaternion& _rotation, const float _deltaTime);

	/// 
	void setFriction(BodyHandle _handle, float _friction);

	///
	float getFriction(BodyHandle _handle);

	/// Retrieve ground info.
	///
	/// @param[in] _handle Handle to body object.
	/// @param[out] _info Ground info.
	///
	void getGroundInfo(
		BodyHandle _handle
		, GroundInfo& _info
	);

	///
	void destroy(BodyHandle _body);

	///
	const bx::Vec3 getGravity();

	/// Create occlusion query.
	///
	/// @returns Handle to occlusion query object.
	///
	/// @attention C99's equivalent binding is `max_create_occlusion_query`.
	///
	OcclusionQueryHandle createOcclusionQuery();

	/// Retrieve occlusion query result from previous frame.
	///
	/// @param[in] _handle Handle to occlusion query object.
	/// @param[out] _result Number of pixels that passed test. This argument
	///   can be `NULL` if result of occlusion query is not needed.
	/// @returns Occlusion query result.
	///
	/// @attention C99's equivalent binding is `max_get_result`.
	///
	OcclusionQueryResult::Enum getResult(
		  OcclusionQueryHandle _handle
		, int32_t* _result = NULL
		);

	/// Destroy occlusion query.
	///
	/// @param[in] _handle Handle to occlusion query object.
	///
	/// @attention C99's equivalent binding is `max_destroy_occlusion_query`.
	///
	void destroy(OcclusionQueryHandle _handle);

	/// Set palette color value.
	///
	/// @param[in] _index Index into palette.
	/// @param[in] _rgba Packed 32-bit RGBA value.
	///
	/// @attention C99's equivalent binding is `max_set_palette_color`.
	///
	void setPaletteColor(
		  uint8_t _index
		, uint32_t _rgba
		);

	/// Set palette color value.
	///
	/// @param[in] _index Index into palette.
	/// @param[in] _r, _g, _b, _a RGBA floating point values.
	///
	/// @attention C99's equivalent binding is `max_set_palette_color`.
	///
	void setPaletteColor(
		  uint8_t _index
		, float _r
		, float _g
		, float _b
		, float _a
		);

	/// Set palette color value.
	///
	/// @param[in] _index Index into palette.
	/// @param[in] _rgba RGBA floating point value.
	///
	/// @attention C99's equivalent binding is `max_set_palette_color`.
	///
	void setPaletteColor(
		  uint8_t _index
		, const float _rgba[4]
		);

	/// Set view name.
	///
	/// @param[in] _id View id.
	/// @param[in] _name View name.
	/// @param[in] _len View name length (if length is INT32_MAX, it's expected that _name
	///   is zero terminated string.
	///
	/// @remarks
	///   This is debug only feature.
	///
	///   In graphics debugger view name will appear as:
	///
	///       "nnnce <view name>"
	///        ^  ^^ ^
	///        |  |+-- eye (L/R)
	///        |  +--- compute (C)
	///        +------ view id
	///
	/// @attention C99's equivalent binding is `max_set_view_name`.
	///
	void setViewName(
		  ViewId _id
		, const char* _name
		, int32_t _len = INT32_MAX
		);

	/// Set view rectangle. Draw primitive outside view will be clipped.
	///
	/// @param[in] _id View id.
	/// @param[in] _x Position x from the left corner of the window.
	/// @param[in] _y Position y from the top corner of the window.
	/// @param[in] _width Width of view port region.
	/// @param[in] _height Height of view port region.
	///
	/// @attention C99's equivalent binding is `max_set_view_rect`.
	///
	void setViewRect(
		  ViewId _id
		, uint16_t _x
		, uint16_t _y
		, uint16_t _width
		, uint16_t _height
		);

	/// Set view rectangle. Draw primitive outside view will be clipped.
	///
	/// @param[in] _id View id.
	/// @param[in] _x Position x from the left corner of the window.
	/// @param[in] _y Position y from the top corner of the window.
	/// @param[in] _ratio Width and height will be set in respect to back-buffer size. See:
	///   `BackbufferRatio::Enum`.
	///
	/// @attention C99's equivalent binding is `max_set_view_rect_ratio`.
	///
	void setViewRect(
		  ViewId _id
		, uint16_t _x
		, uint16_t _y
		, BackbufferRatio::Enum _ratio
		);

	/// Set view scissor. Draw primitive outside view will be clipped. When
	/// _x, _y, _width and _height are set to 0, scissor will be disabled.
	///
	/// @param[in] _id View id.
	/// @param[in] _x Position x from the left corner of the window.
	/// @param[in] _y Position y from the top corner of the window.
	/// @param[in] _width Width of scissor region.
	/// @param[in] _height Height of scissor region.
	///
	/// @attention C99's equivalent binding is `max_set_view_scissor`.
	///
	void setViewScissor(
		  ViewId _id
		, uint16_t _x = 0
		, uint16_t _y = 0
		, uint16_t _width = 0
		, uint16_t _height = 0
		);

	/// Set view clear flags.
	///
	/// @param[in] _id View id.
	/// @param[in] _flags Clear flags. Use `MAX_CLEAR_NONE` to remove any clear
	///   operation. See: `MAX_CLEAR_*`.
	/// @param[in] _rgba Color clear value.
	/// @param[in] _depth Depth clear value.
	/// @param[in] _stencil Stencil clear value.
	///
	/// @attention C99's equivalent binding is `max_set_view_clear`.
	///
	void setViewClear(
		  ViewId _id
		, uint16_t _flags
		, uint32_t _rgba = 0x000000ff
		, float _depth = 1.0f
		, uint8_t _stencil = 0
		);

	/// Set view clear flags with different clear color for each
	/// frame buffer texture. `max::setPaletteColor` must be used to set up a
	/// clear color palette.
	///
	/// @param[in] _id View id.
	/// @param[in] _flags Clear flags. Use `MAX_CLEAR_NONE` to remove any clear
	///   operation. See: `MAX_CLEAR_*`.
	/// @param[in] _depth Depth clear value.
	/// @param[in] _stencil Stencil clear value.
	/// @param[in] _0 Palette index for frame buffer attachment 0.
	/// @param[in] _1 Palette index for frame buffer attachment 1.
	/// @param[in] _2 Palette index for frame buffer attachment 2.
	/// @param[in] _3 Palette index for frame buffer attachment 3.
	/// @param[in] _4 Palette index for frame buffer attachment 4.
	/// @param[in] _5 Palette index for frame buffer attachment 5.
	/// @param[in] _6 Palette index for frame buffer attachment 6.
	/// @param[in] _7 Palette index for frame buffer attachment 7.
	///
	/// @attention C99's equivalent binding is `max_set_view_clear_mrt`.
	///
	void setViewClear(
		  ViewId _id
		, uint16_t _flags
		, float _depth
		, uint8_t _stencil
		, uint8_t _0 = UINT8_MAX
		, uint8_t _1 = UINT8_MAX
		, uint8_t _2 = UINT8_MAX
		, uint8_t _3 = UINT8_MAX
		, uint8_t _4 = UINT8_MAX
		, uint8_t _5 = UINT8_MAX
		, uint8_t _6 = UINT8_MAX
		, uint8_t _7 = UINT8_MAX
		);

	/// Set view sorting mode.
	///
	/// @param[in] _id View id.
	/// @param[in] _mode View sort mode. See `ViewMode::Enum`.
	///
	/// @remarks
	///   View mode must be set prior calling `max::submit` for the view.
	///
	/// @attention C99's equivalent binding is `max_set_view_mode`.
	///
	void setViewMode(
		  ViewId _id
		, ViewMode::Enum _mode = ViewMode::Default
		);

	/// Set view frame buffer.
	///
	/// @param[in] _id View id.
	/// @param[in] _handle Frame buffer handle. Passing `MAX_INVALID_HANDLE` as
	///   frame buffer handle will draw primitives from this view into
	///   default back buffer.
	///
	/// @remarks
	///   Not persistent after `max::reset` call.
	///
	/// @attention C99's equivalent binding is `max_set_view_frame_buffer`.
	///
	void setViewFrameBuffer(
		  ViewId _id
		, FrameBufferHandle _handle
		);

	/// Set view's view matrix and projection matrix,
	/// all draw primitives in this view will use these two matrices.
	///
	/// @param[in] _id View id.
	/// @param[in] _view View matrix.
	/// @param[in] _proj Projection matrix.
	///
	/// @attention C99's equivalent binding is `max_set_view_transform`.
	///
	void setViewTransform(
		  ViewId _id
		, const void* _view
		, const void* _proj
		);

	/// Post submit view reordering.
	///
	/// @param[in] _id First view id.
	/// @param[in] _num Number of views to remap.
	/// @param[in] _remap View remap id table. Passing `NULL` will reset view ids
	///   to default state.
	///
	/// @attention C99's equivalent binding is `max_set_view_order`.
	///
	void setViewOrder(
		  ViewId _id = 0
		, uint16_t _num = UINT16_MAX
		, const ViewId* _remap = NULL
		);

	/// Reset all view settings to default.
	///
	/// @param[in] _id View id.
	///
	/// @attention C99's equivalent binding is `max_reset_view`.
	///
	void resetView(ViewId _id);

	/// Sets a debug marker. This allows you to group graphics calls together for easy
	/// browsing in graphics debugging tools.
	///
	/// @param[in] _name Marker name.
	/// @param[in] _len Marker name length (if length is INT32_MAX, it's expected that _name
	///   is zero terminated string.
	///
	/// @attention C99's equivalent binding is `max_set_marker`.
	///
	void setMarker(const char* _name, int32_t _len = INT32_MAX);

	/// Set render states for draw primitive.
	///
	/// @param[in] _state State flags. Default state for primitive type is
	///   triangles. See: `MAX_STATE_DEFAULT`.
	///   - `MAX_STATE_DEPTH_TEST_*` - Depth test function.
	///   - `MAX_STATE_BLEND_*` - See remark 1 about MAX_STATE_BLEND_FUNC.
	///   - `MAX_STATE_BLEND_EQUATION_*` - See remark 2.
	///   - `MAX_STATE_CULL_*` - Backface culling mode.
	///   - `MAX_STATE_WRITE_*` - Enable R, G, B, A or Z write.
	///   - `MAX_STATE_MSAA` - Enable MSAA.
	///   - `MAX_STATE_PT_[TRISTRIP/LINES/POINTS]` - Primitive type.
	///
	/// @param[in] _rgba Sets blend factor used by `MAX_STATE_BLEND_FACTOR` and
	///   `MAX_STATE_BLEND_INV_FACTOR` blend modes.
	///
	/// @remarks
	///   1. To set up more complex states use:
	///      `MAX_STATE_ALPHA_REF(_ref)`,
	///      `MAX_STATE_POINT_SIZE(_size)`,
	///      `MAX_STATE_BLEND_FUNC(_src, _dst)`,
	///      `MAX_STATE_BLEND_FUNC_SEPARATE(_srcRGB, _dstRGB, _srcA, _dstA)`
	///      `MAX_STATE_BLEND_EQUATION(_equation)`
	///      `MAX_STATE_BLEND_EQUATION_SEPARATE(_equationRGB, _equationA)`
	///   2. `MAX_STATE_BLEND_EQUATION_ADD` is set when no other blend
	///      equation is specified.
	///
	/// @attention C99's equivalent binding is `max_set_state`.
	///
	void setState(
		  uint64_t _state
		, uint32_t _rgba = 0
		);

	/// Set condition for rendering.
	///
	/// @param[in] _handle Occlusion query handle.
	/// @param[in] _visible Render if occlusion query is visible.
	///
	/// @attention C99's equivalent binding is `max_set_condition`.
	///
	void setCondition(
		  OcclusionQueryHandle _handle
		, bool _visible
		);

	/// Set stencil test state.
	///
	/// @param[in] _fstencil Front stencil state.
	/// @param[in] _bstencil Back stencil state. If back is set to `MAX_STENCIL_NONE`
	///   _fstencil is applied to both front and back facing primitives.
	///
	/// @attention C99's equivalent binding is `max_set_stencil`.
	///
	void setStencil(
		  uint32_t _fstencil
		, uint32_t _bstencil = MAX_STENCIL_NONE
		);

	/// Set scissor for draw primitive. For scissor for all primitives in
	/// view see `max::setViewScissor`.
	///
	/// @param[in] _x Position x from the left corner of the window.
	/// @param[in] _y Position y from the top corner of the window.
	/// @param[in] _width Width of scissor region.
	/// @param[in] _height Height of scissor region.
	/// @returns Scissor cache index.
	///
	/// @attention C99's equivalent binding is `max_set_scissor`.
	///
	uint16_t setScissor(
		  uint16_t _x
		, uint16_t _y
		, uint16_t _width
		, uint16_t _height
		);

	/// Set scissor from cache for draw primitive.
	///
	/// @param[in] _cache Index in scissor cache. Passing UINT16_MAX unset primitive
	///   scissor and primitive will use view scissor instead.
	///
	/// @attention C99's equivalent binding is `max_set_scissor_cached`.
	///
	void setScissor(uint16_t _cache = UINT16_MAX);

	/// Set model matrix for draw primitive. If it is not called,
	/// the model will be rendered with an identity model matrix.
	///
	/// @param[in] _mtx Pointer to first matrix in array.
	/// @param[in] _num Number of matrices in array.
	/// @returns index into matrix cache in case the same model matrix has
	///   to be used for other draw primitive call.
	///
	/// @attention C99's equivalent binding is `max_set_transform`.
	///
	uint32_t setTransform(
		  const void* _mtx
		, uint16_t _num = 1
		);

	/// Reserve `_num` matrices in internal matrix cache.
	///
	/// @param[in] _transform Pointer to `Transform` structure.
	/// @param[in] _num Number of matrices.
	/// @returns index into matrix cache.
	///
	/// @attention Pointer returned can be modified until `max::frame` is called.
	/// @attention C99's equivalent binding is `max_alloc_transform`.
	///
	uint32_t allocTransform(
		  Transform* _transform
		, uint16_t _num
		);

	/// Set model matrix from matrix cache for draw primitive.
	///
	/// @param[in] _cache Index in matrix cache.
	/// @param[in] _num Number of matrices from cache.
	///
	/// @attention C99's equivalent binding is `max_set_transform_cached`.
	///
	void setTransform(
		  uint32_t _cache
		, uint16_t _num = 1
		);

	/// Set shader uniform parameter for draw primitive.
	///
	/// @param[in] _handle Uniform.
	/// @param[in] _value Pointer to uniform data.
	/// @param[in] _num Number of elements. Passing `UINT16_MAX` will
	///   use the _num passed on uniform creation.
	///
	/// @attention C99's equivalent binding is `max_set_uniform`.
	///
	void setUniform(
		  UniformHandle _handle
		, const void* _value
		, uint16_t _num = 1
		);

	/// Set index buffer for draw primitive.
	///
	/// @param[in] _handle Index buffer.
	///
	/// @attention C99's equivalent binding is `max_set_index_buffer`.
	///
	void setIndexBuffer(IndexBufferHandle _handle);

	/// Set index buffer for draw primitive.
	///
	/// @param[in] _handle Index buffer.
	/// @param[in] _firstIndex First index to render.
	/// @param[in] _numIndices Number of indices to render.
	///
	/// @attention C99's equivalent binding is `max_set_index_buffer`.
	///
	void setIndexBuffer(
		  IndexBufferHandle _handle
		, uint32_t _firstIndex
		, uint32_t _numIndices
		);

	/// Set index buffer for draw primitive.
	///
	/// @param[in] _handle Dynamic index buffer.
	///
	/// @attention C99's equivalent binding is `max_set_dynamic_index_buffer`.
	///
	void setIndexBuffer(DynamicIndexBufferHandle _handle);

	/// Set index buffer for draw primitive.
	///
	/// @param[in] _handle Dynamic index buffer.
	/// @param[in] _firstIndex First index to render.
	/// @param[in] _numIndices Number of indices to render.
	///
	/// @attention C99's equivalent binding is `max_set_dynamic_index_buffer`.
	///
	void setIndexBuffer(
		  DynamicIndexBufferHandle _handle
		, uint32_t _firstIndex
		, uint32_t _numIndices
		);

	/// Set index buffer for draw primitive.
	///
	/// @param[in] _tib Transient index buffer.
	///
	/// @attention C99's equivalent binding is `max_set_transient_index_buffer`.
	///
	void setIndexBuffer(const TransientIndexBuffer* _tib);

	/// Set index buffer for draw primitive.
	///
	/// @param[in] _tib Transient index buffer.
	/// @param[in] _firstIndex First index to render.
	/// @param[in] _numIndices Number of indices to render.
	///
	/// @attention C99's equivalent binding is `max_set_transient_index_buffer`.
	///
	void setIndexBuffer(
		  const TransientIndexBuffer* _tib
		, uint32_t _firstIndex
		, uint32_t _numIndices
		);

	/// Set vertex buffer for draw primitive.
	///
	/// @param[in] _stream Vertex stream.
	/// @param[in] _handle Vertex buffer.
	///
	/// @attention C99's equivalent binding is `max_set_vertex_buffer`.
	///
	void setVertexBuffer(
		  uint8_t _stream
		, VertexBufferHandle _handle
		);

	/// Set vertex buffer for draw primitive.
	///
	/// @param[in] _stream Vertex stream.
	/// @param[in] _handle Vertex buffer.
	/// @param[in] _startVertex First vertex to render.
	/// @param[in] _numVertices Number of vertices to render.
	/// @param[in] _layoutHandle Vertex layout for aliasing vertex buffer. If invalid handle is
	///   used, vertex layout used for creation of vertex buffer will be used.
	///
	/// @attention C99's equivalent binding is `max_set_vertex_buffer`.
	///
	void setVertexBuffer(
		  uint8_t _stream
		, VertexBufferHandle _handle
		, uint32_t _startVertex
		, uint32_t _numVertices
		, VertexLayoutHandle _layoutHandle = MAX_INVALID_HANDLE
		);

	/// Set vertex buffer for draw primitive.
	///
	/// @param[in] _stream Vertex stream.
	/// @param[in] _handle Dynamic vertex buffer.
	///
	/// @attention C99's equivalent binding is `max_set_dynamic_vertex_buffer`.
	///
	void setVertexBuffer(
		  uint8_t _stream
		, DynamicVertexBufferHandle _handle
		);

	/// Set vertex buffer for draw primitive.
	///
	/// @param[in] _stream Vertex stream.
	/// @param[in] _handle Dynamic vertex buffer.
	/// @param[in] _startVertex First vertex to render.
	/// @param[in] _numVertices Number of vertices to render.
	/// @param[in] _layoutHandle Vertex layout for aliasing vertex buffer. If invalid handle is
	///   used, vertex layout used for creation of vertex buffer will be used.
	///
	/// @attention C99's equivalent binding is `max_set_dynamic_vertex_buffer`.
	///
	void setVertexBuffer(
		  uint8_t _stream
		, DynamicVertexBufferHandle _handle
		, uint32_t _startVertex
		, uint32_t _numVertices
		, VertexLayoutHandle _layoutHandle = MAX_INVALID_HANDLE
		);

	/// Set vertex buffer for draw primitive.
	///
	/// @param[in] _stream Vertex stream.
	/// @param[in] _tvb Transient vertex buffer.
	///
	/// @attention C99's equivalent binding is `max_set_transient_vertex_buffer`.
	///
	void setVertexBuffer(
		  uint8_t _stream
		, const TransientVertexBuffer* _tvb
		);

	/// Set vertex buffer for draw primitive.
	///
	/// @param[in] _stream Vertex stream.
	/// @param[in] _tvb Transient vertex buffer.
	/// @param[in] _startVertex First vertex to render.
	/// @param[in] _numVertices Number of vertices to render.
	/// @param[in] _layoutHandle Vertex layout for aliasing vertex buffer. If invalid handle is
	///   used, vertex layout used for creation of vertex buffer will be used.
	///
	/// @attention C99's equivalent binding is `max_set_transient_vertex_buffer`.
	///
	void setVertexBuffer(
		  uint8_t _stream
		, const TransientVertexBuffer* _tvb
		, uint32_t _startVertex
		, uint32_t _numVertices
		, VertexLayoutHandle _layoutHandle = MAX_INVALID_HANDLE
		);

	/// Set number of vertices for auto generated vertices use in conjunction
	/// with gl_VertexID.
	///
	/// @param[in] _numVertices Number of vertices.
	///
	/// @attention Availability depends on: `MAX_CAPS_VERTEX_ID`.
	/// @attention C99's equivalent binding is `max_set_vertex_count`.
	///
	void setVertexCount(uint32_t _numVertices);

	/// Set instance data buffer for draw primitive.
	///
	/// @param[in] _idb Transient instance data buffer.
	///
	/// @attention C99's equivalent binding is `max_set_instance_data_buffer`.
	///
	void setInstanceDataBuffer(const InstanceDataBuffer* _idb);

	/// Set instance data buffer for draw primitive.
	///
	/// @param[in] _idb Transient instance data buffer.
	/// @param[in] _start First instance data.
	/// @param[in] _num Number of data instances.
	///
	/// @attention C99's equivalent binding is `max_set_instance_data_buffer`.
	///
	void setInstanceDataBuffer(
		  const InstanceDataBuffer* _idb
		, uint32_t _start
		, uint32_t _num
		);

	/// Set instance data buffer for draw primitive.
	///
	/// @param[in] _handle Vertex buffer.
	/// @param[in] _start First instance data.
	/// @param[in] _num Number of data instances.
	///
	/// @attention C99's equivalent binding is `max_set_instance_data_from_vertex_buffer`.
	///
	void setInstanceDataBuffer(
		  VertexBufferHandle _handle
		, uint32_t _start
		, uint32_t _num
		);

	/// Set instance data buffer for draw primitive.
	///
	/// @param[in] _handle Vertex buffer.
	/// @param[in] _start First instance data.
	/// @param[in] _num Number of data instances.
	///
	/// @attention C99's equivalent binding is `max_set_instance_data_from_dynamic_vertex_buffer`.
	///
	void setInstanceDataBuffer(
		  DynamicVertexBufferHandle _handle
		, uint32_t _start
		, uint32_t _num
		);

	/// Set number of instances for auto generated instances use in conjunction
	/// with gl_InstanceID.
	///
	/// @param[in] _numInstances Number of instances.
	///
	/// @attention Availability depends on: `MAX_CAPS_VERTEX_ID`.
	/// @attention C99's equivalent binding is `max_set_instance_count`.
	///
	void setInstanceCount(uint32_t _numInstances);

	/// Set texture stage for draw primitive.
	///
	/// @param[in] _stage Texture unit.
	/// @param[in] _sampler Program sampler.
	/// @param[in] _handle Texture handle.
	/// @param[in] _flags Texture sampling mode. Default value UINT32_MAX uses
	///   texture sampling settings from the texture.
	///   - `MAX_SAMPLER_[U/V/W]_[MIRROR/CLAMP]` - Mirror or clamp to edge wrap
	///     mode.
	///   - `MAX_SAMPLER_[MIN/MAG/MIP]_[POINT/ANISOTROPIC]` - Point or anisotropic
	///     sampling.
	///
	/// @attention C99's equivalent binding is `max_set_texture`.
	///
	void setTexture(
		  uint8_t _stage
		, UniformHandle _sampler
		, TextureHandle _handle
		, uint32_t _flags = UINT32_MAX
		);

	/// Submit an empty primitive for rendering. Uniforms and draw state
	/// will be applied but no geometry will be submitted.
	///
	/// These empty draw calls will sort before ordinary draw calls.
	///
	/// @param[in] _id View id.
	///
	/// @attention C99's equivalent binding is `max_touch`.
	///
	void touch(ViewId _id);

	/// Submit primitive for rendering.
	///
	/// @param[in] _id View id.
	/// @param[in] _program Program.
	/// @param[in] _depth Depth for sorting.
	/// @param[in] _flags Discard or preserve states. See `MAX_DISCARD_*`.
	///
	/// @attention C99's equivalent binding is `max_submit`.
	///
	void submit(
		  ViewId _id
		, ProgramHandle _program
		, uint32_t _depth = 0
		, uint8_t _flags  = MAX_DISCARD_ALL
		);

	/// Submit primitive with occlusion query for rendering.
	///
	/// @param[in] _id View id.
	/// @param[in] _program Program.
	/// @param[in] _occlusionQuery Occlusion query.
	/// @param[in] _depth Depth for sorting.
	/// @param[in] _flags Discard or preserve states. See `MAX_DISCARD_*`.
	///
	/// @attention C99's equivalent binding is `max_submit_occlusion_query`.
	///
	void submit(
		  ViewId _id
		, ProgramHandle _program
		, OcclusionQueryHandle _occlusionQuery
		, uint32_t _depth = 0
		, uint8_t _flags  = MAX_DISCARD_ALL
		);

	/// Submit primitive for rendering with index and instance data info from
	/// indirect buffer.
	///
	/// @param[in] _id View id.
	/// @param[in] _program Program.
	/// @param[in] _indirectHandle Indirect buffer.
	/// @param[in] _start First element in indirect buffer.
	/// @param[in] _num Number of draws.
	/// @param[in] _depth Depth for sorting.
	/// @param[in] _flags Discard or preserve states. See `MAX_DISCARD_*`.
	///
	/// @attention Availability depends on: `MAX_CAPS_DRAW_INDIRECT`.
	/// @attention C99's equivalent binding is `max_submit_indirect`.
	///
	void submit(
		  ViewId _id
		, ProgramHandle _program
		, IndirectBufferHandle _indirectHandle
		, uint32_t _start = 0
		, uint32_t _num   = 1
		, uint32_t _depth = 0
		, uint8_t _flags  = MAX_DISCARD_ALL
		);

	/// Submit primitive for rendering with index and instance data info and
	/// draw count from indirect buffers.
	///
	/// @param[in] _id View id.
	/// @param[in] _program Program.
	/// @param[in] _indirectHandle Indirect buffer.
	/// @param[in] _start First element in indirect buffer.
	/// @param[in] _numHandle Buffer for number of draws. Must be created
	///   with `MAX_BUFFER_INDEX32` and `MAX_BUFFER_DRAW_INDIRECT`.
	/// @param[in] _numIndex Element in number buffer.
	/// @param[in] _numMax Max number of draws.
	/// @param[in] _depth Depth for sorting.
	/// @param[in] _flags Discard or preserve states. See `MAX_DISCARD_*`.
	///
	/// @attention Availability depends on:`MAX_CAPS_DRAW_INDIRECT_COUNT`.
	/// @attention C99's equivalent binding is `max_submit_indirect_count`.
	///
	void submit(
		  ViewId _id
		, ProgramHandle _program
		, IndirectBufferHandle _indirectHandle
		, uint32_t _start
		, IndexBufferHandle _numHandle
		, uint32_t _numIndex = 0
		, uint32_t _numMax = UINT32_MAX
		, uint32_t _depth = 0
		, uint8_t _flags = MAX_DISCARD_ALL
		);

	/// Set compute index buffer.
	///
	/// @param[in] _stage Compute stage.
	/// @param[in] _handle Index buffer handle.
	/// @param[in] _access Buffer access. See `Access::Enum`.
	///
	/// @attention C99's equivalent binding is `max_set_compute_index_buffer`.
	///
	void setBuffer(
		  uint8_t _stage
		, IndexBufferHandle _handle
		, Access::Enum _access
		);

	/// Set compute vertex buffer.
	///
	/// @param[in] _stage Compute stage.
	/// @param[in] _handle Vertex buffer handle.
	/// @param[in] _access Buffer access. See `Access::Enum`.
	///
	/// @attention C99's equivalent binding is `max_set_compute_vertex_buffer`.
	///
	void setBuffer(
		  uint8_t _stage
		, VertexBufferHandle _handle
		, Access::Enum _access
		);

	/// Set compute dynamic index buffer.
	///
	/// @param[in] _stage Compute stage.
	/// @param[in] _handle Dynamic index buffer handle.
	/// @param[in] _access Buffer access. See `Access::Enum`.
	///
	/// @attention C99's equivalent binding is `max_set_compute_dynamic_index_buffer`.
	///
	void setBuffer(
		  uint8_t _stage
		, DynamicIndexBufferHandle _handle
		, Access::Enum _access
		);

	/// Set compute dynamic vertex buffer.
	///
	/// @param[in] _stage Compute stage.
	/// @param[in] _handle Dynamic vertex buffer handle.
	/// @param[in] _access Buffer access. See `Access::Enum`.
	///
	/// @attention C99's equivalent binding is `max_set_compute_dynamic_vertex_buffer`.
	///
	void setBuffer(
		  uint8_t _stage
		, DynamicVertexBufferHandle _handle
		, Access::Enum _access
		);

	/// Set compute indirect buffer.
	///
	/// @param[in] _stage Compute stage.
	/// @param[in] _handle Indirect buffer handle.
	/// @param[in] _access Buffer access. See `Access::Enum`.
	///
	/// @attention C99's equivalent binding is `max_set_compute_indirect_buffer`.
	///
	void setBuffer(
		  uint8_t _stage
		, IndirectBufferHandle _handle
		, Access::Enum _access
		);

	/// Set compute image from texture.
	///
	/// @param[in] _stage Texture unit.
	/// @param[in] _handle Texture handle.
	/// @param[in] _mip Mip level.
	/// @param[in] _access Texture access. See `Access::Enum`.
	/// @param[in] _format Texture format. See: `TextureFormat::Enum`.
	///
	/// @attention C99's equivalent binding is `max_set_image`.
	///
	void setImage(
		  uint8_t _stage
		, TextureHandle _handle
		, uint8_t _mip
		, Access::Enum _access
		, TextureFormat::Enum _format = TextureFormat::Count
		);

	/// Dispatch compute.
	///
	/// @param[in] _id View id.
	/// @param[in] _handle Compute program.
	/// @param[in] _numX Number of groups X.
	/// @param[in] _numY Number of groups Y.
	/// @param[in] _numZ Number of groups Z.
	/// @param[in] _flags Discard or preserve states. See `MAX_DISCARD_*`.
	///
	/// @attention C99's equivalent binding is `max_dispatch`.
	///
	void dispatch(
		  ViewId _id
		, ProgramHandle _handle
		, uint32_t _numX = 1
		, uint32_t _numY = 1
		, uint32_t _numZ = 1
		, uint8_t _flags = MAX_DISCARD_ALL
		);

	/// Dispatch compute indirect.
	///
	/// @param[in] _id View id.
	/// @param[in] _handle Compute program.
	/// @param[in] _indirectHandle Indirect buffer.
	/// @param[in] _start First element in indirect buffer.
	/// @param[in] _num Number of dispatches.
	/// @param[in] _flags Discard or preserve states. See `MAX_DISCARD_*`.
	///
	/// @attention C99's equivalent binding is `max_dispatch_indirect`.
	///
	void dispatch(
		  ViewId _id
		, ProgramHandle _handle
		, IndirectBufferHandle _indirectHandle
		, uint32_t _start = 0
		, uint32_t _num   = 1
		, uint8_t _flags  = MAX_DISCARD_ALL
		);

	/// Discard all previously set state for draw or compute call.
	///
	/// @param[in] _flags Draw/compute states to discard.
	///
	/// @attention C99's equivalent binding is `max_discard`.
	///
	void discard(uint8_t _flags = MAX_DISCARD_ALL);

	/// Blit 2D texture region between two 2D textures.
	///
	/// @param[in] _id View id.
	/// @param[in] _dst Destination texture handle.
	/// @param[in] _dstX Destination texture X position.
	/// @param[in] _dstY Destination texture Y position.
	/// @param[in] _src Source texture handle.
	/// @param[in] _srcX Source texture X position.
	/// @param[in] _srcY Source texture Y position.
	/// @param[in] _width Width of region.
	/// @param[in] _height Height of region.
	///
	/// @attention Destination texture must be created with `MAX_TEXTURE_BLIT_DST` flag.
	/// @attention Availability depends on: `MAX_CAPS_TEXTURE_BLIT`.
	/// @attention C99's equivalent binding is `max_blit`.
	///
	void blit(
		  ViewId _id
		, TextureHandle _dst
		, uint16_t _dstX
		, uint16_t _dstY
		, TextureHandle _src
		, uint16_t _srcX = 0
		, uint16_t _srcY = 0
		, uint16_t _width = UINT16_MAX
		, uint16_t _height = UINT16_MAX
		);

	/// Blit texture region between two textures.
	///
	/// @param[in] _id View id.
	/// @param[in] _dst Destination texture handle.
	/// @param[in] _dstMip Destination texture mip level.
	/// @param[in] _dstX Destination texture X position.
	/// @param[in] _dstY Destination texture Y position.
	/// @param[in] _dstZ If texture is 2D this argument should be 0. If destination texture is cube
	///   this argument represents destination texture cube face. For 3D texture this argument
	///   represents destination texture Z position.
	/// @param[in] _src Source texture handle.
	/// @param[in] _srcMip Source texture mip level.
	/// @param[in] _srcX Source texture X position.
	/// @param[in] _srcY Source texture Y position.
	/// @param[in] _srcZ If texture is 2D this argument should be 0. If source texture is cube
	///   this argument represents source texture cube face. For 3D texture this argument
	///   represents source texture Z position.
	/// @param[in] _width Width of region.
	/// @param[in] _height Height of region.
	/// @param[in] _depth If texture is 3D this argument represents depth of region, otherwise it's
	///   unused.
	///
	/// @attention Destination texture must be created with `MAX_TEXTURE_BLIT_DST` flag.
	/// @attention Availability depends on: `MAX_CAPS_TEXTURE_BLIT`.
	/// @attention C99's equivalent binding is `max_blit`.
	///
	void blit(
		  ViewId _id
		, TextureHandle _dst
		, uint8_t _dstMip
		, uint16_t _dstX
		, uint16_t _dstY
		, uint16_t _dstZ
		, TextureHandle _src
		, uint8_t _srcMip = 0
		, uint16_t _srcX = 0
		, uint16_t _srcY = 0
		, uint16_t _srcZ = 0
		, uint16_t _width = UINT16_MAX
		, uint16_t _height = UINT16_MAX
		, uint16_t _depth = UINT16_MAX
		);

	/// Request screen shot of window back buffer.
	///
	/// @param[in] _handle Frame buffer handle. If handle is `MAX_INVALID_HANDLE` request will be
	///   made for main window back buffer.
	/// @param[in] _filePath Will be passed to `max::CallbackI::screenShot` callback.
	///
	/// @remarks
	///   `max::CallbackI::screenShot` must be implemented.
	///
	/// @attention Frame buffer handle must be created with OS' target native window handle.
	/// @attention C99's equivalent binding is `max_request_screen_shot`.
	///
	void requestScreenShot(
		  FrameBufferHandle _handle
		, const char* _filePath
		);

	///
	void cmdAdd(const char* _name, ConsoleFn _fn, void* _userData = NULL);

	///
	void cmdRemove(const char* _name);

	///
	void cmdExec(const char* _format, ...);

} // namespace max

#ifdef MAX_BUILD_MINK

#include <mink/mink.h>

namespace mink
{
	/// Load motion from path.
	///
	/// @param[in] _filePath Path to the motion binary.
	/// @returns Motion handle.
	///
	/// @remarks Motion binary is obtained by compiling motion offline with motionlink.
	///
	MotionHandle loadMotion(const char* _filePath);

} // namespace mink

#endif // MAX_BUILD_MINK

#endif // MAX_H_HEADER_GUARD

/*
 * Copyright 2011-2024 Branimir Karadzic. All rights reserved.
 * License: https://github.com/bkaradzic/bgfx/blob/master/LICENSE
 */

#include <algorithm>

#include <bx/string.h>
#include <bgfx/bgfx.h>
#include "../../bgfx/src/vertexlayout.h"

#include <tinystl/allocator.h>
#include <tinystl/string.h>
#include <tinystl/vector.h>
#include <tinystl/unordered_map.h>
namespace stl = tinystl;

#include <meshoptimizer/src/meshoptimizer.h>

#ifdef BGFX_GEOMETRYC_FBX_COMPILER
#include <fbxsdk.h>
#endif // BGFX_GEOMETRYC_FBX_COMPILER

#define CGLTF_VALIDATE_ENABLE_ASSERTS BX_CONFIG_DEBUG
#define CGLTF_IMPLEMENTATION
#include <cgltf/cgltf.h>

#define BGFX_GEOMETRYC_VERSION_MAJOR 1
#define BGFX_GEOMETRYC_VERSION_MINOR 0

#if 0
#	define BX_TRACE(_format, ...) \
		do { \
			bx::printf(BX_FILE_LINE_LITERAL "BGFX " _format "\n", ##__VA_ARGS__); \
		} while(0)

#	define BX_WARN(_condition, _format, ...) \
		do { \
			if (!(_condition) ) \
			{ \
				BX_TRACE(BX_FILE_LINE_LITERAL "WARN " _format, ##__VA_ARGS__); \
			} \
		} while(0)

#	define BX_ASSERT(_condition, _format, ...) \
		do { \
			if (!(_condition) ) \
			{ \
				BX_TRACE(BX_FILE_LINE_LITERAL "CHECK " _format, ##__VA_ARGS__); \
				bx::debugBreak(); \
			} \
		} while(0)
#endif // 0

#include <bx/bx.h>
#include <bx/bounds.h>
#include <bx/commandline.h>
#include <bx/debug.h>
#include <bx/file.h>
#include <bx/hash.h>
#include <bx/math.h>
#include <bx/timer.h>
#include <bx/uint32_t.h>

typedef stl::vector<bx::Vec3> Vec3Array;

struct Vec4
{
	float m_data[4];
};

typedef stl::vector<Vec4> Vec4Array;

struct Unique
{
	uint32_t m_positionIndex;
	uint32_t m_normalIndex;
	uint32_t m_texcoordIndex;
	uint32_t m_indicesIndex;
	uint32_t m_weightsIndex;
};

typedef stl::vector<Unique> UniqueArray;

struct Skinning
{
	int index;
	float value;
};

struct Index3
{
	int32_t m_position;
	int32_t m_texcoord;
	int32_t m_normal;
	int32_t m_indices;
	int32_t m_weights;
	int32_t m_vbc; // Barycentric ID. Holds either 0, 1 or 2.
};

struct TriIndices
{
	Index3 m_index[3];
};

typedef stl::vector<TriIndices> TriangleArray;

struct Group
{
	uint32_t m_startTriangle;
	uint32_t m_numTriangles;
	stl::string m_name;
	stl::string m_material;
};

typedef stl::vector<Group> GroupArray;

struct Primitive
{
	uint32_t m_startVertex;
	uint32_t m_startIndex;
	uint32_t m_numVertices;
	uint32_t m_numIndices;
	stl::string m_name;
};

typedef stl::vector<Primitive> PrimitiveArray;

struct Axis
{
	enum Enum
	{
		NegativeX,
		PositiveX,
		NegativeY,
		PositiveY,
		NegativeZ,
		PositiveZ,
	};
};

static bx::Vec3 s_axisVectors[6] =
{
	bx::Vec3(-1.0f, 0.0f, 0.0f),
	bx::Vec3( 1.0f, 0.0f, 0.0f),
	bx::Vec3( 0.0f,-1.0f, 0.0f),
	bx::Vec3( 0.0f, 1.0f, 0.0f),
	bx::Vec3( 0.0f, 0.0f,-1.0f),
	bx::Vec3( 0.0f, 0.0f, 1.0f),
};

struct CoordinateSystem
{
	bx::Handedness::Enum m_handedness;
	Axis::Enum           m_up;
	Axis::Enum           m_forward;
};

struct CoordinateSystemMapping
{
	const char*      m_param;
	CoordinateSystem m_coordinateSystem;
};

static const CoordinateSystemMapping s_coordinateSystemMappings[] =
{
	{ "lh-up+y", { bx::Handedness::Left,  Axis::PositiveY, Axis::PositiveZ } },
	{ "lh-up+z", { bx::Handedness::Left,  Axis::PositiveZ, Axis::PositiveY } },
	{ "rh-up+y", { bx::Handedness::Right, Axis::PositiveY, Axis::PositiveZ } },
	{ "rh-up+z", { bx::Handedness::Right, Axis::PositiveZ, Axis::PositiveY } },
};

struct Mesh
{
	Vec3Array     m_positions;
	Vec3Array     m_normals;
	Vec3Array     m_texcoords;
	Vec4Array     m_indices;
	Vec4Array     m_weights;
	TriangleArray m_triangles;
	GroupArray    m_groups;

	CoordinateSystem m_coordinateSystem;
};

static uint32_t s_obbSteps = 17;

constexpr uint32_t kChunkVertexBuffer           = BX_MAKEFOURCC('V', 'B', ' ', 0x1);
constexpr uint32_t kChunkVertexBufferCompressed = BX_MAKEFOURCC('V', 'B', 'C', 0x0);
constexpr uint32_t kChunkIndexBuffer            = BX_MAKEFOURCC('I', 'B', ' ', 0x0);
constexpr uint32_t kChunkIndexBufferCompressed  = BX_MAKEFOURCC('I', 'B', 'C', 0x1);
constexpr uint32_t kChunkPrimitive              = BX_MAKEFOURCC('P', 'R', 'I', 0x0);

void optimizeVertexCache(uint16_t* _indices, uint32_t _numIndices, uint32_t _numVertices)
{
	uint16_t* newIndexList = new uint16_t[_numIndices];
	meshopt_optimizeVertexCache(newIndexList, _indices, _numIndices, _numVertices);
	bx::memCopy(_indices, newIndexList, _numIndices * 2);
	delete[] newIndexList;
}

uint32_t optimizeVertexFetch(
	  uint16_t* _indices
	, uint32_t _numIndices
	, uint8_t* _vertexData
	, uint32_t _numVertices
	, uint16_t _stride
	)
{
	unsigned char* newVertices = (unsigned char*)malloc(_numVertices * _stride );
	size_t vertexCount = meshopt_optimizeVertexFetch(newVertices, _indices, _numIndices, _vertexData, _numVertices, _stride);
	bx::memCopy(_vertexData, newVertices, _numVertices * _stride);
	free(newVertices);

	return uint32_t(vertexCount);
}

void writeCompressedIndices(
	  bx::WriterI* _writer
	, const uint16_t* _indices
	, uint32_t _numIndices
	, uint32_t _numVertices
	, bx::Error* _err
	)
{
	size_t maxSize = meshopt_encodeIndexBufferBound(_numIndices, _numVertices);
	unsigned char* compressedIndices = (unsigned char*)malloc(maxSize);

	size_t compressedSize = meshopt_encodeIndexBuffer(compressedIndices, maxSize, _indices, _numIndices);

	bx::printf("Indices uncompressed: %10d, compressed: %10d, ratio: %0.2f%%\n"
		, _numIndices*2
		, (uint32_t)compressedSize
		, 100.0f - float(compressedSize ) / float(_numIndices*2)*100.0f
		);

	bx::write(_writer, (uint32_t)compressedSize, _err);
	bx::write(_writer, compressedIndices, (uint32_t)compressedSize, _err);

	free(compressedIndices);
}

void writeCompressedVertices(
	  bx::WriterI* _writer
	, const uint8_t* _vertices
	, uint32_t _numVertices
	, uint16_t _stride
	, bx::Error* _err
	)
{
	size_t maxSize = meshopt_encodeVertexBufferBound(_numVertices, _stride);
	unsigned char* compressedVertices = (unsigned char*)malloc(maxSize);

	size_t compressedSize = meshopt_encodeVertexBuffer(compressedVertices, maxSize, _vertices, _numVertices, _stride);

	bx::printf("Vertices uncompressed: %10d, compressed: %10d, ratio: %0.2f%%\n"
		, _numVertices * _stride
		, (uint32_t)compressedSize
		, 100.0f - float(compressedSize) / float(_numVertices * _stride)*100.0f
		);

	bx::write(_writer, (uint32_t)compressedSize, _err);
	bx::write(_writer, compressedVertices, (uint32_t)compressedSize, _err);

	free(compressedVertices);
}

void calcTangents(void* _vertices, uint16_t _numVertices, bgfx::VertexLayout _layout, const uint16_t* _indices, uint32_t _numIndices)
{
	struct PosTexcoord
	{
		float m_x;
		float m_y;
		float m_z;
		float m_pad0;
		float m_u;
		float m_v;
		float m_pad1;
		float m_pad2;
	};

	float* tangents = new float[6*_numVertices];
	bx::memSet(tangents, 0, 6*_numVertices*sizeof(float) );

	PosTexcoord v0;
	PosTexcoord v1;
	PosTexcoord v2;

	for (uint32_t ii = 0, num = _numIndices/3; ii < num; ++ii)
	{
		const uint16_t* indices = &_indices[ii*3];
		uint32_t i0 = indices[0];
		uint32_t i1 = indices[1];
		uint32_t i2 = indices[2];

		bgfx::vertexUnpack(&v0.m_x, bgfx::Attrib::Position, _layout, _vertices, i0);
		bgfx::vertexUnpack(&v0.m_u, bgfx::Attrib::TexCoord0, _layout, _vertices, i0);

		bgfx::vertexUnpack(&v1.m_x, bgfx::Attrib::Position, _layout, _vertices, i1);
		bgfx::vertexUnpack(&v1.m_u, bgfx::Attrib::TexCoord0, _layout, _vertices, i1);

		bgfx::vertexUnpack(&v2.m_x, bgfx::Attrib::Position, _layout, _vertices, i2);
		bgfx::vertexUnpack(&v2.m_u, bgfx::Attrib::TexCoord0, _layout, _vertices, i2);

		const float bax = v1.m_x - v0.m_x;
		const float bay = v1.m_y - v0.m_y;
		const float baz = v1.m_z - v0.m_z;
		const float bau = v1.m_u - v0.m_u;
		const float bav = v1.m_v - v0.m_v;

		const float cax = v2.m_x - v0.m_x;
		const float cay = v2.m_y - v0.m_y;
		const float caz = v2.m_z - v0.m_z;
		const float cau = v2.m_u - v0.m_u;
		const float cav = v2.m_v - v0.m_v;

		const float det = (bau * cav - bav * cau);
		const float invDet = 1.0f / det;

		const float tx = (bax * cav - cax * bav) * invDet;
		const float ty = (bay * cav - cay * bav) * invDet;
		const float tz = (baz * cav - caz * bav) * invDet;

		const float bx = (cax * bau - bax * cau) * invDet;
		const float by = (cay * bau - bay * cau) * invDet;
		const float bz = (caz * bau - baz * cau) * invDet;

		for (uint32_t jj = 0; jj < 3; ++jj)
		{
			float* tanu = &tangents[indices[jj]*6];
			float* tanv = &tanu[3];
			tanu[0] += tx;
			tanu[1] += ty;
			tanu[2] += tz;

			tanv[0] += bx;
			tanv[1] += by;
			tanv[2] += bz;
		}
	}

	for (uint32_t ii = 0; ii < _numVertices; ++ii)
	{
		const bx::Vec3 tanu = bx::load<bx::Vec3>(&tangents[ii*6]);
		const bx::Vec3 tanv = bx::load<bx::Vec3>(&tangents[ii*6 + 3]);

		float nxyzw[4];
		bgfx::vertexUnpack(nxyzw, bgfx::Attrib::Normal, _layout, _vertices, ii);

		const bx::Vec3 normal  = bx::load<bx::Vec3>(nxyzw);
		const float    ndt     = bx::dot(normal, tanu);
		const bx::Vec3 nxt     = bx::cross(normal, tanu);
		const bx::Vec3 tmp     = bx::sub(tanu, bx::mul(normal, ndt) );

		float tangent[4];
		bx::store(tangent, bx::normalize(tmp) );
		tangent[3] = bx::dot(nxt, tanv) < 0.0f ? -1.0f : 1.0f;

		bgfx::vertexPack(tangent, true, bgfx::Attrib::Tangent, _layout, _vertices, ii);
	}

	delete [] tangents;
}

void write(
	  bx::WriterI* _writer
	, const void* _vertices
	, uint32_t _numVertices
	, uint32_t _stride
	, bx::Error* _err
	)
{
	bx::Sphere maxSphere;
	bx::calcMaxBoundingSphere(maxSphere, _vertices, _numVertices, _stride);

	bx::Sphere minSphere;
	bx::calcMinBoundingSphere(minSphere, _vertices, _numVertices, _stride);

	if (minSphere.radius > maxSphere.radius)
	{
		bx::write(_writer, maxSphere, _err);
	}
	else
	{
		bx::write(_writer, minSphere, _err);
	}

	bx::Aabb aabb;
	bx::toAabb(aabb, _vertices, _numVertices, _stride);
	bx::write(_writer, aabb, _err);

	bx::Obb obb;
	bx::calcObb(obb, _vertices, _numVertices, _stride, s_obbSteps);
	bx::write(_writer, obb, _err);
}

void write(
	  bx::WriterI* _writer
	, const uint8_t* _vertices
	, uint32_t _numVertices
	, const bgfx::VertexLayout& _layout
	, const uint16_t* _indices
	, uint32_t _numIndices
	, bool _compress
	, const stl::string& _material
	, const PrimitiveArray& _primitives
	, bx::Error* _err
	)
{
	using namespace bx;
	using namespace bgfx;

	uint32_t stride = _layout.getStride();

	if (_compress)
	{
		write(_writer, kChunkVertexBufferCompressed, _err);
		write(_writer, _vertices, _numVertices, stride, _err);

		write(_writer, _layout);

		write(_writer, uint16_t(_numVertices), _err);
		writeCompressedVertices(_writer, _vertices, _numVertices, uint16_t(stride), _err);
	}
	else
	{
		write(_writer, kChunkVertexBuffer, _err);
		write(_writer, _vertices, _numVertices, stride, _err);

		write(_writer, _layout, _err);

		write(_writer, uint16_t(_numVertices), _err);
		write(_writer, _vertices, _numVertices*stride, _err);
	}

	if (_compress)
	{
		write(_writer, kChunkIndexBufferCompressed, _err);
		write(_writer, _numIndices, _err);

		writeCompressedIndices(_writer, _indices, _numIndices, _numVertices, _err);
	}
	else
	{
		write(_writer, kChunkIndexBuffer, _err);
		write(_writer, _numIndices, _err);
		write(_writer, _indices, _numIndices*2, _err);
	}

	write(_writer, kChunkPrimitive, _err);

	uint16_t nameLen = uint16_t(_material.size() );
	write(_writer, nameLen, _err);

	write(_writer, _material.c_str(), nameLen, _err);
	write(_writer, uint16_t(_primitives.size() ), _err);

	for (PrimitiveArray::const_iterator primIt = _primitives.begin(); primIt != _primitives.end(); ++primIt)
	{
		const Primitive& prim = *primIt;
		nameLen = uint16_t(prim.m_name.size() );
		write(_writer, nameLen, _err);
		write(_writer, prim.m_name.c_str(), nameLen, _err);
		write(_writer, prim.m_startIndex, _err);
		write(_writer, prim.m_numIndices, _err);
		write(_writer, prim.m_startVertex, _err);
		write(_writer, prim.m_numVertices, _err);
		write(_writer, &_vertices[prim.m_startVertex*stride], prim.m_numVertices, stride, _err);
	}
}

inline uint32_t rgbaToAbgr(uint8_t _r, uint8_t _g, uint8_t _b, uint8_t _a)
{
	return (uint32_t(_r)<<0)
		 | (uint32_t(_g)<<8)
		 | (uint32_t(_b)<<16)
		 | (uint32_t(_a)<<24)
		 ;
}

struct GroupSortByMaterial
{
	bool operator()(const Group& _lhs, const Group& _rhs)
	{
		return 0 < bx::strCmp(_lhs.m_material.c_str(), _rhs.m_material.c_str() );
	}
};

void mtxCoordinateTransform(float* _result, const CoordinateSystem& _cs)
{
	bx::Vec3 up      = s_axisVectors[_cs.m_up];
	bx::Vec3 forward = s_axisVectors[_cs.m_forward];
	bx::Vec3 right   = cross(forward,up);

	if (_cs.m_handedness == bx::Handedness::Left)
	{
		right = bx::mul(right, -1.0f);
	}

	bx::mtxIdentity(_result);
	bx::store(&_result[0], right);
	bx::store(&_result[4], forward);
	bx::store(&_result[8], up);
}

float mtxDeterminant(const float* _a)
{
	const float xx = _a[ 0];
	const float xy = _a[ 1];
	const float xz = _a[ 2];
	const float xw = _a[ 3];
	const float yx = _a[ 4];
	const float yy = _a[ 5];
	const float yz = _a[ 6];
	const float yw = _a[ 7];
	const float zx = _a[ 8];
	const float zy = _a[ 9];
	const float zz = _a[10];
	const float zw = _a[11];
	const float wx = _a[12];
	const float wy = _a[13];
	const float wz = _a[14];
	const float ww = _a[15];

	float det = 0.0f;
	det += xx * (yy*(zz*ww - zw*wz) - yz*(zy*ww - zw*wy) + yw*(zy*wz - zz*wy) );
	det -= xy * (yx*(zz*ww - zw*wz) - yz*(zx*ww - zw*wx) + yw*(zx*wz - zz*wx) );
	det += xz * (yx*(zy*ww - zw*wy) - yy*(zx*ww - zw*wx) + yw*(zx*wy - zy*wx) );
	det -= xw * (yx*(zy*wz - zz*wy) - yy*(zx*wz - zz*wx) + yz*(zx*wy - zy*wx) );

	return det;
}

void parseObj(char* _data, uint32_t _size, Mesh* _mesh, bool _hasBc)
{
	// Reference(s):
	// - Wavefront .obj file
	//   https://en.wikipedia.org/wiki/Wavefront_.obj_file

	// Coordinate system is right-handed, but up/forward is not defined, but +Y Up, +Z Forward seems to be a common default
	_mesh->m_coordinateSystem.m_handedness = bx::Handedness::Right;
	_mesh->m_coordinateSystem.m_up         = Axis::PositiveY;
	_mesh->m_coordinateSystem.m_forward    = Axis::PositiveZ;

	uint32_t num = 0;

	Group group;
	group.m_startTriangle = 0;
	group.m_numTriangles = 0;

	char commandLine[2048];
	uint32_t len = sizeof(commandLine);
	int argc;
	char* argv[64];

	for (bx::StringView next(_data, _size); !next.isEmpty(); )
	{
		next = bx::tokenizeCommandLine(next, commandLine, len, argc, argv, BX_COUNTOF(argv), '\n');

		if (0 < argc)
		{
			if (0 == bx::strCmp(argv[0], "#") )
			{
				if (2 < argc
				&&  0 == bx::strCmp(argv[2], "polygons") )
				{
				}
			}
			else if (0 == bx::strCmp(argv[0], "f") )
			{
				TriIndices triangle;
				bx::memSet(&triangle, 0, sizeof(TriIndices) );

				const int numNormals   = (int)_mesh->m_normals.size();
				const int numTexcoords = (int)_mesh->m_texcoords.size();
				const int numPositions = (int)_mesh->m_positions.size();
				for (uint32_t edge = 0, numEdges = argc-1; edge < numEdges; ++edge)
				{
					Index3 index;
					index.m_texcoord = -1;
					index.m_normal = -1;
					if (_hasBc)
					{
						index.m_vbc = edge < 3 ? edge : (1+(edge+1) )&1;
					}
					else
					{
						index.m_vbc = 0;
					}

					{
						bx::StringView triplet(argv[edge + 1]);
						bx::StringView vertex(triplet);
						bx::StringView texcoord = bx::strFind(triplet, '/');

						if (!texcoord.isEmpty() )
						{
							vertex.set(vertex.getPtr(), texcoord.getPtr() );

							const bx::StringView normal = bx::strFind(bx::StringView(texcoord.getPtr() + 1, triplet.getTerm() ), '/');
							if (!normal.isEmpty() )
							{
								int32_t nn;
								bx::fromString(&nn, bx::StringView(normal.getPtr() + 1, triplet.getTerm() ) );
								index.m_normal = (nn < 0) ? nn + numNormals : nn - 1;
							}

							texcoord.set(texcoord.getPtr() + 1, normal.getPtr() );

							// Reference(s):
							// - Wavefront .obj file / Vertex normal indices without texture coordinate indices
							//   https://en.wikipedia.org/wiki/Wavefront_.obj_file#Vertex_Normal_Indices_Without_Texture_Coordinate_Indices
							if (!texcoord.isEmpty() )
							{
								int32_t tex;
								bx::fromString(&tex, texcoord);
								index.m_texcoord = (tex < 0) ? tex + numTexcoords : tex - 1;
							}
						}

						int32_t pos;
						bx::fromString(&pos, vertex);
						index.m_position = (pos < 0) ? pos + numPositions : pos - 1;
					}

					switch (edge)
					{
					case 0: case 1: case 2:
						triangle.m_index[edge] = index;
						if (2 == edge)
						{
							_mesh->m_triangles.push_back(triangle);
						}
						break;

					default:
						triangle.m_index[1] = triangle.m_index[2];
						triangle.m_index[2] = index;

						_mesh->m_triangles.push_back(triangle);
						break;
					}
				}
			}
			else if (0 == bx::strCmp(argv[0], "g") )
			{
				group.m_name = argv[1];
			}
			else if (*argv[0] == 'v')
			{
				group.m_numTriangles = (uint32_t)(_mesh->m_triangles.size() ) - group.m_startTriangle;
				if (0 < group.m_numTriangles)
				{
					_mesh->m_groups.push_back(group);
					group.m_startTriangle = (uint32_t)(_mesh->m_triangles.size() );
					group.m_numTriangles = 0;
				}

				if (0 == bx::strCmp(argv[0], "vn") )
				{
					bx::Vec3 normal(bx::InitNone);
					bx::fromString(&normal.x, argv[1]);
					bx::fromString(&normal.y, argv[2]);
					bx::fromString(&normal.z, argv[3]);

					_mesh->m_normals.push_back(normal);
				}
				else if (0 == bx::strCmp(argv[0], "vp") )
				{
					static bool once = true;
					if (once)
					{
						once = false;
						bx::printf("warning: 'parameter space vertices' are unsupported.\n");
					}
				}
				else if (0 == bx::strCmp(argv[0], "vt") )
				{
					bx::Vec3 texcoord(bx::InitNone);
					texcoord.y = 0.0f;
					texcoord.z = 0.0f;

					bx::fromString(&texcoord.x, argv[1]);

					switch (argc)
					{
					case 4:
						bx::fromString(&texcoord.z, argv[3]);
						[[fallthrough]];

					case 3:
						bx::fromString(&texcoord.y, argv[2]);
						break;

					default:
						break;
					}

					_mesh->m_texcoords.push_back(texcoord);
				}
				else
				{
					float px, py, pz, pw;
					bx::fromString(&px, argv[1]);
					bx::fromString(&py, argv[2]);
					bx::fromString(&pz, argv[3]);

					if (argc == 5 || argc == 8)
					{
						bx::fromString(&pw, argv[4]);
					}
					else
					{
						pw = 1.0f;
					}

					bx::Vec3 pos(px, py, pz);

					const float invW = bx::rcp(pw);
					pos = bx::mul(pos, invW);

					_mesh->m_positions.push_back(pos);
				}
			}
			else if (0 == bx::strCmp(argv[0], "usemtl") )
			{
				stl::string material(argv[1]);

				if (0 != bx::strCmp(material.c_str(), group.m_material.c_str() ) )
				{
					group.m_numTriangles = (uint32_t)(_mesh->m_triangles.size() ) - group.m_startTriangle;
					if (0 < group.m_numTriangles)
					{
						_mesh->m_groups.push_back(group);
						group.m_startTriangle = (uint32_t)(_mesh->m_triangles.size() );
						group.m_numTriangles = 0;
					}
				}

				group.m_material = material;
			}
		}

		++num;
	}

	group.m_numTriangles = (uint32_t)(_mesh->m_triangles.size() ) - group.m_startTriangle;

	if (0 < group.m_numTriangles)
	{
		_mesh->m_groups.push_back(group);
		group.m_startTriangle = (uint32_t)(_mesh->m_triangles.size() );
		group.m_numTriangles  = 0;
	}

	bx::printf("obj parser # %d\n", num);
}

void gltfReadFloat(const float* _accessorData, cgltf_size _accessorNumComponents, cgltf_size _index, cgltf_float* _out, cgltf_size _outElementSize)
{
	const float* input = &_accessorData[_accessorNumComponents * _index];

	for (cgltf_size ii = 0; ii < _outElementSize; ++ii)
	{
		_out[ii] = (ii < _accessorNumComponents) ? input[ii] : 0.0f;
	}
}

void processGltfNode(cgltf_node* _node, Mesh* _mesh, Group* _group, bool _hasBc)
{
	cgltf_mesh* mesh = _node->mesh;
	if (NULL != mesh)
	{
		float nodeToWorld[16];
		cgltf_node_transform_world(_node, nodeToWorld);
		float nodeToWorldNormal[16];
		bx::mtxCofactor(nodeToWorldNormal, nodeToWorld);

		for (cgltf_size primitiveIndex = 0; primitiveIndex < mesh->primitives_count; ++primitiveIndex)
		{
			cgltf_primitive* primitive = &mesh->primitives[primitiveIndex];

			cgltf_size numVertex = primitive->attributes[0].data->count;

			int32_t basePositionIndex = (int32_t)_mesh->m_positions.size();
			int32_t baseNormalIndex   = (int32_t)_mesh->m_normals.size();
			int32_t baseTexcoordIndex = (int32_t)_mesh->m_texcoords.size();

			bool hasNormal   = false;
			bool hasTexcoord = false;

			for (cgltf_size attributeIndex = 0; attributeIndex < primitive->attributes_count; ++attributeIndex)
			{
				cgltf_attribute* attribute = &primitive->attributes[attributeIndex];
				cgltf_accessor* accessor = attribute->data;
				cgltf_size accessorCount = accessor->count;

				BX_ASSERT(numVertex == accessorCount, "Invalid attribute count");

				cgltf_size floatCount = cgltf_accessor_unpack_floats(accessor, NULL, 0);
				float* accessorData = (float*)malloc(floatCount * sizeof(float) );
				cgltf_accessor_unpack_floats(accessor, accessorData, floatCount);

				cgltf_size numComponents = cgltf_num_components(accessor->type);

				if (attribute->type == cgltf_attribute_type_position && attribute->index == 0)
				{
					_mesh->m_positions.reserve(_mesh->m_positions.size() + accessorCount);

					bx::Vec3 pos(bx::InitNone);

					for (cgltf_size v = 0; v < accessorCount; ++v)
					{
						gltfReadFloat(accessorData, numComponents, v, &pos.x, 3);
						pos = mul(pos, nodeToWorld);
						_mesh->m_positions.push_back(pos);
					}
				}
				else if (attribute->type == cgltf_attribute_type_normal && attribute->index == 0)
				{
					_mesh->m_normals.reserve(_mesh->m_normals.size() + accessorCount);

					hasNormal = true;
					bx::Vec3 normal(bx::InitNone);

					for (cgltf_size v = 0; v < accessorCount; ++v)
					{
						gltfReadFloat(accessorData, numComponents, v, &normal.x, 3);
						normal = mul(normal, nodeToWorldNormal);
						_mesh->m_normals.push_back(normal);
					}
				}
				else if (attribute->type == cgltf_attribute_type_texcoord && attribute->index == 0)
				{
					_mesh->m_texcoords.reserve(_mesh->m_texcoords.size() + accessorCount);

					hasTexcoord = true;
					bx::Vec3 texcoord(bx::InitNone);

					for (cgltf_size v = 0; v < accessorCount; ++v)
					{
						gltfReadFloat(accessorData, numComponents, v, &texcoord.x, 3);
						_mesh->m_texcoords.push_back(texcoord);
					}
				}

				free(accessorData);
			}

			if (primitive->indices != NULL)
			{
				cgltf_accessor* accessor = primitive->indices;

				for (cgltf_size v = 0; v < accessor->count; v += 3)
				{
					TriIndices triangle;
					for (int i = 0; i < 3; ++i)
					{
						Index3 index;
						int32_t vertexIndex = int32_t(cgltf_accessor_read_index(accessor, v+i) );
						index.m_position = basePositionIndex + vertexIndex;
						index.m_normal   = hasNormal   ? baseNormalIndex   + vertexIndex : -1;
						index.m_texcoord = hasTexcoord ? baseTexcoordIndex + vertexIndex : -1;
						index.m_vbc      = _hasBc      ? i                               :  0;
						triangle.m_index[i] = index;
					}
					_mesh->m_triangles.push_back(triangle);
				}
			}
			else
			{
				for (cgltf_size v = 0; v < numVertex; v += 3)
				{
					TriIndices triangle;
					for (int i = 0; i < 3; ++i)
					{
						Index3 index;
						int32_t vertexIndex = int32_t(v * 3 + i);
						index.m_position = basePositionIndex + vertexIndex;
						index.m_normal   = hasNormal   ? baseNormalIndex   + vertexIndex : -1;
						index.m_texcoord = hasTexcoord ? baseTexcoordIndex + vertexIndex : -1;
						index.m_vbc      = _hasBc      ? i                               :  0;
						triangle.m_index[i] = index;
					}
					_mesh->m_triangles.push_back(triangle);
				}
			}

			_group->m_numTriangles = (uint32_t)(_mesh->m_triangles.size() ) - _group->m_startTriangle;

			if (0 < _group->m_numTriangles)
			{
				_mesh->m_groups.push_back(*_group);
				_group->m_startTriangle = (uint32_t)(_mesh->m_triangles.size() );
				_group->m_numTriangles = 0;
			}
		}
	}

	for (cgltf_size childIndex = 0; childIndex < _node->children_count; ++childIndex)
	{
		processGltfNode(_node->children[childIndex], _mesh, _group, _hasBc);
	}
}

void parseGltf(char* _data, uint32_t _size, Mesh* _mesh, bool _hasBc, const bx::StringView& _path)
{
	// Reference(s):
	// - Gltf 2.0 specification
	//  https://github.com/KhronosGroup/glTF/tree/master/specification/2.0

	_mesh->m_coordinateSystem.m_handedness = bx::Handedness::Right;
	_mesh->m_coordinateSystem.m_forward    = Axis::PositiveZ;
	_mesh->m_coordinateSystem.m_up         = Axis::PositiveY;

	Group group;
	group.m_startTriangle = 0;
	group.m_numTriangles = 0;

	cgltf_options options = { };
	cgltf_data* data = NULL;
	cgltf_result result = cgltf_parse(&options, _data, _size, &data);

	if (result == cgltf_result_success)
	{
		char* path = (char*)malloc(_path.getLength()+1);
		bx::memCopy(path, _path.getPtr(), _path.getLength() );
		path[_path.getLength()] = 0;
		result = cgltf_load_buffers(&options, data, path);
		free(path);

		if (result == cgltf_result_success)
		{
			for (cgltf_size sceneIndex = 0; sceneIndex < data->scenes_count; ++sceneIndex)
			{
				cgltf_scene* scene = &data->scenes[sceneIndex];

				for (cgltf_size nodeIndex = 0; nodeIndex < scene->nodes_count; ++nodeIndex)
				{
					cgltf_node* node = scene->nodes[nodeIndex];

					processGltfNode(node, _mesh, &group, _hasBc);
				}
			}
		}

		cgltf_free(data);
	}
}

#ifdef BGFX_GEOMETRYC_FBX_COMPILER

void processFbxNodeSkeleton(FbxNode* _node, uint32_t _index, uint32_t _parentIndex, stl::unordered_map<stl::string, int32_t>& _skeleton)
{
	if (_node->GetNodeAttribute() && _node->GetNodeAttribute()->GetAttributeType() && _node->GetNodeAttribute()->GetAttributeType() == FbxNodeAttribute::eSkeleton)
	{
		_skeleton[_node->GetNameOnly().Buffer()] = _index;
	}

	for (uint32_t ii = 0; ii < (uint32_t)_node->GetChildCount(); ii++)
	{
		processFbxNodeSkeleton(_node->GetChild(ii), (uint32_t)_skeleton.size(), _index, _skeleton);
	}
}

void processFbxNode(FbxNode* _node, Mesh* _mesh, Group* _group, bool _hasBc, stl::unordered_map<stl::string, int32_t>& _skeleton)
{
	if (_node->GetNodeAttribute() && _node->GetNodeAttribute()->GetAttributeType() == FbxNodeAttribute::eMesh)
	{
		FbxMesh* mesh = _node->GetMesh();
		if (mesh)
		{
			FbxGeometryConverter converter = FbxGeometryConverter(_node->GetFbxManager());
			mesh = (FbxMesh*)converter.Triangulate(mesh, true);

			uint32_t basePositionIndex = (uint32_t)_mesh->m_positions.size();

			uint32_t baseNormalIndex = (uint32_t)_mesh->m_normals.size();
			uint32_t baseTexcoordIndex = (uint32_t)_mesh->m_texcoords.size();
			uint32_t baseIndicesIndex = (uint32_t)_mesh->m_indices.size();
			uint32_t baseWeightsIndex = (uint32_t)_mesh->m_weights.size();

			bool hasNormal   = false;
			bool hasTexcoord = false;
			bool hasIndices  = false;
			bool hasWeights  = false;

			uint32_t triangleCount = mesh->GetPolygonCount();
			_mesh->m_triangles.reserve(triangleCount);

			stl::unordered_map<int, uint32_t> uniqueVertexMap;
			UniqueArray uniqueVertices;

			stl::vector<stl::vector<Skinning>> controlPointSkinning(mesh->GetControlPointsCount());
			FbxSkin* pSkin = (FbxSkin*)mesh->GetDeformer(0, FbxDeformer::eSkin);
			if (pSkin)
			{
				int clusterCount = pSkin->GetClusterCount();
				for (int i = 0; i < clusterCount; ++i)
				{
					FbxCluster* cluster = pSkin->GetCluster(i);
					if (!cluster->GetLink())
						continue;

					int boneIndex = _skeleton[cluster->GetLink()->GetNameOnly().Buffer()];
					int* controlPointIndices = cluster->GetControlPointIndices();
					double* controlPointWeights = cluster->GetControlPointWeights();
					int indexCount = cluster->GetControlPointIndicesCount();

					for (int j = 0; j < indexCount; ++j)
					{
						Skinning skinning;
						skinning.index = boneIndex;
						skinning.value = (float)controlPointWeights[j];
						controlPointSkinning[controlPointIndices[j]].emplace_back(skinning);
					}
				}
			}

			uint32_t vertexCounter = 0;
			for (uint32_t ii = 0; ii < triangleCount; ++ii)
			{
				TriIndices triangle;
				bx::memSet(&triangle, 0, sizeof(TriIndices));

				for (uint32_t jj = 0; jj < 3; ++jj)
				{
					int ctrlPointIndex = mesh->GetPolygonVertex(ii, jj);

					FbxVector4 position = mesh->GetControlPointAt(ctrlPointIndex);
					bx::Vec3 pos = { (float)position[0], (float)position[1], (float)position[2] };
					_mesh->m_positions.push_back(pos);

					if (mesh->GetElementNormalCount() > 0)
					{
						hasNormal = true;
						FbxVector4 meshNormal;
						mesh->GetPolygonVertexNormal(ii, jj, meshNormal);
						meshNormal.Normalize();
						bx::Vec3 normal = { (float)meshNormal[0], (float)meshNormal[1], (float)meshNormal[2] };
						_mesh->m_normals.push_back(normal);
					}

					if (mesh->GetElementUVCount() > 0)
					{
						hasTexcoord = true;
						FbxGeometryElementUV* vertexUV = mesh->GetElementUV(0);
						bx::Vec3 uv = { 0.0f, 0.0f, 0.0f };

						switch (vertexUV->GetMappingMode())
						{
						case FbxGeometryElement::eByControlPoint:
							switch (vertexUV->GetReferenceMode())
							{
							case FbxGeometryElement::eDirect:
							{
								uv.x = static_cast<float>(vertexUV->GetDirectArray().GetAt(ctrlPointIndex).mData[0]);
								uv.y = static_cast<float>(vertexUV->GetDirectArray().GetAt(ctrlPointIndex).mData[1]);
							}
							break;

							case FbxGeometryElement::eIndexToDirect:
							{
								int index = vertexUV->GetIndexArray().GetAt(ctrlPointIndex);
								uv.x = static_cast<float>(vertexUV->GetDirectArray().GetAt(index).mData[0]);
								uv.y = static_cast<float>(vertexUV->GetDirectArray().GetAt(index).mData[1]);
							}
							break;
							default:
								throw std::exception("Invalid Reference");
							}
							break;
						case FbxGeometryElement::eByPolygonVertex:
							switch (vertexUV->GetReferenceMode())
							{
							case FbxGeometryElement::eDirect:
							{
								uv.x = static_cast<float>(vertexUV->GetDirectArray().GetAt(vertexCounter).mData[0]);
								uv.y = static_cast<float>(vertexUV->GetDirectArray().GetAt(vertexCounter).mData[1]);
							}
							break;
							case FbxGeometryElement::eIndexToDirect:
							{
								int index = vertexUV->GetIndexArray().GetAt(vertexCounter);
								uv.x = static_cast<float>(vertexUV->GetDirectArray().GetAt(index).mData[0]);
								uv.y = static_cast<float>(vertexUV->GetDirectArray().GetAt(index).mData[1]);
							}
							break;
							default: throw std::exception("Invalid Reference");
							}

							break;
						}

						_mesh->m_texcoords.push_back(uv);
					}

					if (controlPointSkinning.size() > 0)
					{
						hasIndices = true;
						hasWeights = true;

						Vec4 boneIndices = { 0, 0, 0, 0 };
						Vec4 boneWeights = { 0.0f, 0.0f, 0.0f, 0.0f };

						const auto& skinningData = controlPointSkinning[ctrlPointIndex];
						int influenceCount = bx::min(4, static_cast<int>(skinningData.size()));

						for (int k = 0; k < influenceCount; ++k)
						{
							boneIndices.m_data[k] = static_cast<float>(skinningData[k].index);
							boneWeights.m_data[k] = skinningData[k].value;
						}

						_mesh->m_indices.push_back(boneIndices);
						_mesh->m_weights.push_back(boneWeights);
					}

					auto it = uniqueVertexMap.find(ctrlPointIndex);
					if (it == uniqueVertexMap.end())
					{
						Unique newVertex;
						newVertex.m_positionIndex = basePositionIndex + vertexCounter;
						newVertex.m_normalIndex = hasNormal ? baseNormalIndex + vertexCounter : -1;
						newVertex.m_texcoordIndex = hasTexcoord ? baseTexcoordIndex + vertexCounter : -1;
						newVertex.m_indicesIndex = hasIndices ? baseIndicesIndex + vertexCounter : -1;
						newVertex.m_weightsIndex = hasWeights ? baseWeightsIndex + vertexCounter : -1;
						uniqueVertices.push_back(newVertex);
						uniqueVertexMap[ctrlPointIndex] = static_cast<uint32_t>(uniqueVertices.size() - 1);
					}

					Index3 index;
					index.m_position = uniqueVertices[uniqueVertexMap[ctrlPointIndex]].m_positionIndex;
					index.m_normal = uniqueVertices[uniqueVertexMap[ctrlPointIndex]].m_normalIndex;
					index.m_texcoord = uniqueVertices[uniqueVertexMap[ctrlPointIndex]].m_texcoordIndex;
					index.m_indices = uniqueVertices[uniqueVertexMap[ctrlPointIndex]].m_indicesIndex;
					index.m_weights = uniqueVertices[uniqueVertexMap[ctrlPointIndex]].m_weightsIndex;
					index.m_vbc = _hasBc ? jj : 0;
					triangle.m_index[jj] = index;

					++vertexCounter;
				}

				_mesh->m_triangles.push_back(triangle);
			}

			_group->m_numTriangles = (uint32_t)(_mesh->m_triangles.size()) - _group->m_startTriangle;

			if (0 < _group->m_numTriangles)
			{
				_mesh->m_groups.push_back(*_group);
				_group->m_startTriangle = (uint32_t)(_mesh->m_triangles.size());
				_group->m_numTriangles = 0;
			}
		}
	}

	for (uint32_t childIndex = 0; childIndex < (uint32_t)_node->GetChildCount(); ++childIndex)
	{
		processFbxNode(_node->GetChild(childIndex), _mesh, _group, _hasBc, _skeleton);
	}
}

void parseFbx(Mesh* _mesh, bool _hasBc, const bx::StringView& _path)
{
	// Reference(s):
	// - FBX SDK Documentation
	//  http://docs.autodesk.com/FBX/2014/ENU/FBX-SDK-Documentation/

	Group group;
	group.m_startTriangle = 0;
	group.m_numTriangles = 0;

	FbxManager* manager = FbxManager::Create();
	bx::printf(
		"FBX SDK, Sdk manager version %s\n"
		, manager->GetVersion()
	);

	FbxIOSettings* ios = FbxIOSettings::Create(manager, IOSROOT);
	manager->SetIOSettings(ios);

	FbxImporter* importer = FbxImporter::Create(manager, "DefaultImporter");

	int major, minor, revision;
	importer->GetFileVersion(major, minor, revision);

	if (!importer->Initialize(_path.getPtr()))
	{
		const FbxString error = importer->GetStatus().GetErrorString();
		bx::printf("Fbx Importer initialization failed with error: %s\n", error.Buffer());
		return;
	}

	bx::printf("FBX file version %u.%u.%u\n", major, minor, revision);

	FbxScene* scene = FbxScene::Create(manager, "DefaultScene");
	if (!importer->Import(scene))
	{
		bx::printf("Error importing scene: %s\n", scene->GetName());
		return;
	}

	bx::FileReader reader;
	int32_t errcode = bx::open(&reader, _path.getPtr());

	char cmpBuffer[19] = { 0 };
	bx::read(&reader, cmpBuffer, 18, bx::ErrorAssert{});
	bx::close(&reader);

	if (bx::strCmp(cmpBuffer, "Kaydara FBX Binary") != 0)
	{
		bx::printf("Reading ASCII FBX files will be slower than binary. It is highly recommended that you convert all FBX input files to binary!\n");
	}

	FbxAxisSystem originalAxisSystem = scene->GetGlobalSettings().GetAxisSystem();
	FbxAxisSystem axisSystem = FbxAxisSystem::DirectX;
	axisSystem.DeepConvertScene(scene);

	FbxSystemUnit originalUnitSystem = scene->GetGlobalSettings().GetSystemUnit();
	FbxSystemUnit unitSystem = FbxSystemUnit::m;
	originalUnitSystem.ConvertScene(scene);

	stl::unordered_map<stl::string, int32_t> skeleton;

	FbxNode* rootNode = scene->GetRootNode();
	processFbxNodeSkeleton(rootNode, 0, UINT32_MAX, skeleton);
	processFbxNode(rootNode, _mesh, &group, _hasBc, skeleton);

	importer->Destroy();
	scene->Destroy();
	manager->Destroy();
}

#endif // BGFX_GEOMETRYC_FBX_COMPILER

void help(const char* _error = NULL)
{
	if (NULL != _error)
	{
		bx::printf("Error:\n%s\n\n", _error);
	}

	bx::printf(
		  "geometryc, bgfx geometry compiler tool, version %d.%d.%d.\n"
		  "Copyright 2011-2024 Branimir Karadzic. All rights reserved.\n"
		  "License: https://github.com/bkaradzic/bgfx/blob/master/LICENSE\n\n"
		, BGFX_GEOMETRYC_VERSION_MAJOR
		, BGFX_GEOMETRYC_VERSION_MINOR
		, BGFX_API_VERSION
		);

	bx::printf(
		  "Usage: geometryc -f <in> -o <out>\n"

		  "\n"
		  "Supported input file types:\n"
		  "    *.fbx                  Filmbox\n"
		  "    *.obj                  Wavefront\n"
		  "    *.gltf,*.glb           glTF 2.0\n"

		  "\n"
		  "Options:\n"
		  "  -h, --help               Display this help and exit.\n"
		  "  -v, --version            Output version information and exit.\n"
		  "  -f <file path>           Input's file path.\n"
		  "  -o <file path>           Output's file path.\n"
		  "  -s, --scale <num>        Scale factor.\n"
		  "      --ccw                Front face is counter-clockwise winding order.\n"
		  "      --flipv              Flip texture coordinate V.\n"
		  "      --obb <num>          Number of steps for calculating oriented bounding box.\n"
		  "           Defaults to 17.\n"
		  "           Less steps = less precise OBB.\n"
		  "           More steps = slower calculation.\n"
		  "      --packnormal <num>   Normal packing.\n"
		  "           0 - unpacked 12 bytes. (default)\n"
		  "           1 - packed 4 bytes.\n"
		  "      --packuv <num>       Texture coordinate packing.\n"
		  "           0 - unpacked 8 bytes. (default)\n"
		  "           1 - packed 4 bytes.\n"
		  "      --tangent            Calculate tangent vectors. (packing mode is the same as normal)\n"
		  "      --barycentric        Adds barycentric vertex attribute. (Packed in bgfx::Attrib::Color1)\n"
		  "  -c, --compress           Compress indices.\n"
		  "      --[l/r]h-up+[y/z]	  Coordinate system. Defaults to '--lh-up+y' — Left-Handed +Y is up.\n"

		  "\n"
		  "For additional information, see https://github.com/bkaradzic/bgfx\n"
		);
}

int main(int _argc, const char* _argv[])
{
	bx::CommandLine cmdLine(_argc, _argv);

	if (cmdLine.hasArg('v', "version") )
	{
		bx::printf(
			"geometryc, bgfx geometry compiler tool, version %d.%d.%d.\n"
			, BGFX_GEOMETRYC_VERSION_MAJOR
			, BGFX_GEOMETRYC_VERSION_MINOR
			, BGFX_API_VERSION
		);
		return bx::kExitSuccess;
	}

	if (cmdLine.hasArg('h', "help") )
	{
		help();
		return bx::kExitFailure;
	}

	const char* filePath = cmdLine.findOption('f');
	if (NULL == filePath)
	{
		help("Input file name must be specified.");
		return bx::kExitFailure;
	}

	const char* outFilePath = cmdLine.findOption('o');
	if (NULL == outFilePath)
	{
		help("Output file name must be specified.");
		return bx::kExitFailure;
	}

	float scale = 1.0f;
	const char* scaleArg = cmdLine.findOption('s', "scale");
	if (NULL != scaleArg)
	{
		if (!bx::fromString(&scale, scaleArg) )
		{
			scale = 1.0f;
		}
	}

	bool compress = cmdLine.hasArg('c', "compress");

	cmdLine.hasArg(s_obbSteps, '\0', "obb");
	s_obbSteps = bx::uint32_min(bx::uint32_max(s_obbSteps, 1), 90);

	uint32_t packNormal = 0;
	cmdLine.hasArg(packNormal, '\0', "packnormal");

	uint32_t packUv = 0;
	cmdLine.hasArg(packUv, '\0', "packuv");

	bool ccw = cmdLine.hasArg("ccw");
	bool flipV = cmdLine.hasArg("flipv");
	bool hasTangent = cmdLine.hasArg("tangent");
	bool hasBc = cmdLine.hasArg("barycentric");

	CoordinateSystem outputCoordinateSystem;
	outputCoordinateSystem.m_handedness = bx::Handedness::Left;
	outputCoordinateSystem.m_forward = Axis::PositiveZ;
	outputCoordinateSystem.m_up = Axis::PositiveY;
	for (uint32_t ii = 0; ii < BX_COUNTOF(s_coordinateSystemMappings); ++ii)
	{
		if (cmdLine.hasArg(s_coordinateSystemMappings[ii].m_param) )
		{
			outputCoordinateSystem = s_coordinateSystemMappings[ii].m_coordinateSystem;
		}
	}

	int64_t parseElapsed = -bx::getHPCounter();
	int64_t triReorderElapsed = 0;

	Mesh mesh;
	bx::StringView ext = bx::FilePath(filePath).getExt();
	if (0 == bx::strCmpI(ext, ".fbx") || 0 == bx::strCmpI(ext, ".fbx"))
	{
#ifdef BGFX_GEOMETRYC_FBX_COMPILER
		parseFbx(&mesh, hasBc, bx::FilePath(filePath));
#else
		bx::printf("FBXSDK is needed to compile fbx files.", filePath);
		return bx::kExitFailure;
#endif // BGFX_GEOMETRYC_FBX_COMPILER
	}
	else
	{
		bx::FileReader fr;
		if (!bx::open(&fr, filePath))
		{
			bx::printf("Unable to open input file '%s'.", filePath);
			return bx::kExitFailure;
		}

		uint32_t size = (uint32_t)bx::getSize(&fr);
		char* data = new char[size + 1];
		size = bx::read(&fr, data, size, bx::ErrorAssert{});
		data[size] = '\0';
		bx::close(&fr);

		if (0 == bx::strCmpI(ext, ".obj"))
		{
			parseObj(data, size, &mesh, hasBc);
		}
		else if (0 == bx::strCmpI(ext, ".gltf") || 0 == bx::strCmpI(ext, ".glb"))
		{
			parseGltf(data, size, &mesh, hasBc, bx::FilePath(filePath).getPath());
		}
		else
		{
			bx::printf("Unsupported input file format '%s'.", filePath);
			exit(bx::kExitFailure);
		}

		delete[] data;
	}

	int64_t now = bx::getHPCounter();
	parseElapsed += now;
	int64_t convertElapsed = -now;

	std::sort(mesh.m_groups.begin(), mesh.m_groups.end(), GroupSortByMaterial() );

	bool changeWinding = ccw;

	if (scale != 1.0f)
	{
		for (Vec3Array::iterator it = mesh.m_positions.begin(), itEnd = mesh.m_positions.end(); it != itEnd; ++it)
		{
			it->x *= scale;
			it->y *= scale;
			it->z *= scale;
		}
	}

	/* @todo
	{
		float meshTransform[16];
		mtxCoordinateTransform(meshTransform, mesh.m_coordinateSystem);

		float meshInvTranform[16];
		bx::mtxTranspose(meshInvTranform, meshTransform);

		float outTransform[16];
		mtxCoordinateTransform(outTransform, outputCoordinateSystem);

		float transform[16];
		bx::mtxMul(transform, meshInvTranform, outTransform);

		if (mtxDeterminant(transform) < 0.0f )
		{
			changeWinding = !changeWinding;
		}

		float identity[16];
		bx::mtxIdentity(identity);

		if (0 != bx::memCmp(identity, transform, sizeof(transform) ) )
		{
			for (Vec3Array::iterator it = mesh.m_positions.begin(), itEnd = mesh.m_positions.end(); it != itEnd; ++it)
			{
				*it = bx::mul(*it, transform);
			}

			for (Vec3Array::iterator it = mesh.m_normals.begin(), itEnd = mesh.m_normals.end(); it != itEnd; ++it)
			{
				*it = bx::mul(*it, transform);
			}
		}
	}
	*/

	bool hasColor    = false;
	bool hasNormal   = false;
	bool hasTexcoord = false;
	bool hasIndices = false;
	bool hasWeights = false;

	{
		for (TriangleArray::iterator it = mesh.m_triangles.begin(), itEnd = mesh.m_triangles.end(); it != itEnd && !hasTexcoord; ++it)
		{
			for (uint32_t i = 0; i < 3; ++i)
			{
				hasTexcoord |= -1 != it->m_index[i].m_texcoord;
			}
		}

		for (TriangleArray::iterator it = mesh.m_triangles.begin(), itEnd = mesh.m_triangles.end(); it != itEnd && !hasNormal; ++it)
		{
			for (uint32_t i = 0; i < 3; ++i)
			{
				hasNormal |= -1 != it->m_index[i].m_normal;
			}
		}

		for (TriangleArray::iterator it = mesh.m_triangles.begin(), itEnd = mesh.m_triangles.end(); it != itEnd && !hasIndices; ++it)
		{
			for (uint32_t i = 0; i < 3; ++i)
			{
				hasIndices |= -1 != it->m_index[i].m_indices;
			}
		}

		for (TriangleArray::iterator it = mesh.m_triangles.begin(), itEnd = mesh.m_triangles.end(); it != itEnd && !hasWeights; ++it)
		{
			for (uint32_t i = 0; i < 3; ++i)
			{
				hasWeights |= -1 != it->m_index[i].m_weights;
			}
		}

		if (changeWinding)
		{
			for (TriangleArray::iterator it = mesh.m_triangles.begin(), itEnd = mesh.m_triangles.end(); it != itEnd; ++it)
			{
				bx::swap(it->m_index[1], it->m_index[2]);
			}
		}
	}

	bgfx::VertexLayout layout;
	layout.begin();
	layout.add(bgfx::Attrib::Position, 3, bgfx::AttribType::Float);

	if (hasColor)
	{
		layout.add(bgfx::Attrib::Color0, 4, bgfx::AttribType::Uint8, true);
	}

	if (hasBc)
	{
		layout.add(bgfx::Attrib::Color1, 4, bgfx::AttribType::Uint8, true);
	}

	if (hasTexcoord)
	{
		switch (packUv)
		{
		default:
		case 0:
			layout.add(bgfx::Attrib::TexCoord0, 2, bgfx::AttribType::Float);
			break;

		case 1:
			layout.add(bgfx::Attrib::TexCoord0, 2, bgfx::AttribType::Half);
			break;
		}
	}

	if (hasNormal)
	{
		hasTangent &= hasTexcoord;

		switch (packNormal)
		{
		default:
		case 0:
			layout.add(bgfx::Attrib::Normal, 3, bgfx::AttribType::Float);
			if (hasTangent)
			{
				layout.add(bgfx::Attrib::Tangent, 4, bgfx::AttribType::Float);
			}
			break;

		case 1:
			layout.add(bgfx::Attrib::Normal, 4, bgfx::AttribType::Uint8, true, true);
			if (hasTangent)
			{
				layout.add(bgfx::Attrib::Tangent, 4, bgfx::AttribType::Uint8, true, true);
			}
			break;
		}
	}

	if (hasIndices)
	{
		layout.add(bgfx::Attrib::Indices, 4, bgfx::AttribType::Float);
	}

	if (hasWeights)
	{
		layout.add(bgfx::Attrib::Weight, 4, bgfx::AttribType::Float);
	}

	layout.end();

	uint32_t stride = layout.getStride();
	uint8_t* vertexData = new uint8_t[mesh.m_triangles.size() * 3 * stride];
	uint16_t* indexData = new uint16_t[mesh.m_triangles.size() * 3];
	int32_t numVertices = 0;
	int32_t numIndices = 0;

	int32_t writtenPrimitives = 0;
	int32_t writtenVertices = 0;
	int32_t writtenIndices = 0;

	uint8_t* vertices = vertexData;
	uint16_t* indices = indexData;

	const uint32_t tableSize = 65536 * 2;
	const uint32_t hashmod = tableSize - 1;
	uint32_t* table = new uint32_t[tableSize];
	bx::memSet(table, 0xff, tableSize * sizeof(uint32_t) );

	stl::string material = mesh.m_groups.empty() ? "" : mesh.m_groups.begin()->m_material;

	PrimitiveArray primitives;

	bx::FileWriter writer;
	if (!bx::open(&writer, outFilePath) )
	{
		bx::printf("Unable to open output file '%s'.", outFilePath);
		exit(bx::kExitFailure);
	}

	Primitive prim;
	prim.m_startVertex = 0;
	prim.m_startIndex  = 0;

	uint32_t positionOffset = layout.getOffset(bgfx::Attrib::Position);
	uint32_t color0Offset   = layout.getOffset(bgfx::Attrib::Color0);

	Group sentinelGroup;
	sentinelGroup.m_startTriangle = 0;
	sentinelGroup.m_numTriangles = UINT32_MAX;
	mesh.m_groups.push_back(sentinelGroup);

	bx::Error err;

	// @todo This takes a long time.
	uint32_t ii = 0;
	for (GroupArray::const_iterator groupIt = mesh.m_groups.begin(); groupIt != mesh.m_groups.end(); ++groupIt, ++ii)
	{
		const bool sentinel = groupIt->m_startTriangle == 0 && groupIt->m_numTriangles == UINT32_MAX;

		for (uint32_t tri = groupIt->m_startTriangle, end = tri + groupIt->m_numTriangles; tri < end; ++tri)
		{
			if (0 != bx::strCmp(material.c_str(), groupIt->m_material.c_str() )
			||  sentinel
			||  65533 <= numVertices)
			{
				prim.m_numVertices = numVertices - prim.m_startVertex;
				prim.m_numIndices  = numIndices  - prim.m_startIndex;

				if (0 < prim.m_numVertices)
				{
					primitives.push_back(prim);
				}

				if (hasTangent)
				{
					calcTangents(vertexData, uint16_t(numVertices), layout, indexData, numIndices);
				}

				triReorderElapsed -= bx::getHPCounter();

				for (PrimitiveArray::const_iterator primIt = primitives.begin(); primIt != primitives.end(); ++primIt)
				{
					const Primitive& prim1 = *primIt;
					optimizeVertexCache(indexData + prim1.m_startIndex, prim1.m_numIndices, numVertices);
				}

				numVertices = optimizeVertexFetch(indexData, numIndices, vertexData, numVertices, uint16_t(stride) );

				triReorderElapsed += bx::getHPCounter();

				if (0 < numVertices
				&&  0 < numIndices)
				{
					write(&writer
						, vertexData
						, numVertices
						, layout
						, indexData
						, numIndices
						, compress
						, material
						, primitives
						, &err
						);
				}
				primitives.clear();

				bx::memSet(table, 0xff, tableSize * sizeof(uint32_t) );

				++writtenPrimitives;
				writtenVertices += numVertices;
				writtenIndices += numIndices;

				vertices = vertexData;
				indices  = indexData;
				numVertices = 0;
				numIndices  = 0;
				prim.m_startVertex = 0;
				prim.m_startIndex  = 0;

				material = groupIt->m_material;

				if (sentinel)
				{
					break;
				}
			}

			TriIndices& triangle = mesh.m_triangles[tri];
			for (uint32_t edge = 0; edge < 3; ++edge)
			{
				Index3& index = triangle.m_index[edge];

				float* position = (float*)(vertices + positionOffset);
				bx::memCopy(position, &mesh.m_positions[index.m_position], 3*sizeof(float) );

				if (hasColor)
				{
					uint32_t* color0 = (uint32_t*)(vertices + color0Offset);
					*color0 = rgbaToAbgr(numVertices%255, numIndices%255, 0, 0xff);
				}

				if (hasBc)
				{
					const float bc[4] =
					{
						(index.m_vbc == 0) ? 1.0f : 0.0f,
						(index.m_vbc == 1) ? 1.0f : 0.0f,
						(index.m_vbc == 2) ? 1.0f : 0.0f,
						0.0f
					};
					bgfx::vertexPack(bc, true, bgfx::Attrib::Color1, layout, vertices);
				}

				if (hasTexcoord)
				{
					float uv[2];
					bx::memCopy(uv, &mesh.m_texcoords[index.m_texcoord == -1 ? 0 : index.m_texcoord], 2*sizeof(float) );

					if (flipV)
					{
						uv[1] = -uv[1];
					}

					bgfx::vertexPack(uv, true, bgfx::Attrib::TexCoord0, layout, vertices);
				}

				if (hasNormal)
				{
					float normal[4];
					bx::store(normal, bx::normalize(bx::load<bx::Vec3>(&mesh.m_normals[index.m_normal == -1 ? 0 : index.m_normal]) ) );
					normal[3] = 0.0f;
					bgfx::vertexPack(normal, true, bgfx::Attrib::Normal, layout, vertices);
				}

				if (hasIndices)
				{
					float indices[4];
					bx::memCopy(indices, &mesh.m_indices[index.m_indices == -1 ? 0 : index.m_indices].m_data, 4 * sizeof(float));
					bgfx::vertexPack(indices, false, bgfx::Attrib::Indices, layout, vertices);
				}

				if (hasWeights)
				{
					float weights[4];
					bx::memCopy(weights, &mesh.m_weights[index.m_weights == -1 ? 0 : index.m_weights].m_data, 4 * sizeof(float));
					bgfx::vertexPack(weights, false, bgfx::Attrib::Weight, layout, vertices);
				}

				uint32_t hash = bx::hash<bx::HashMurmur2A>(vertices, stride);
				size_t bucket = hash & hashmod;
				uint32_t vertexIndex = UINT32_MAX;

				for (size_t probe = 0; probe <= hashmod; ++probe)
				{
					uint32_t& item = table[bucket];

					if (item == ~0u)
					{
						vertices += stride;
						item = numVertices++;
						vertexIndex = item;
						break;
					}

					if (0 == bx::memCmp(vertexData + item * stride, vertices, stride) )
					{
						vertexIndex = item;
						break;
					}

					bucket = (bucket + probe + 1) & hashmod;
				}

				if ( vertexIndex == UINT32_MAX )
				{
					bx::printf("hash table insert failed");
					exit(bx::kExitFailure);
				}

				*indices++ = (uint16_t)vertexIndex;
				++numIndices;
			}
		}

		prim.m_numVertices = numVertices - prim.m_startVertex;
		if (0 < prim.m_numVertices)
		{
			prim.m_numIndices = numIndices - prim.m_startIndex;
			prim.m_name = groupIt->m_name;
			primitives.push_back(prim);
			prim.m_startVertex = numVertices;
			prim.m_startIndex  = numIndices;
		}

		BX_TRACE("%3d: s %5d, n %5d, %s\n"
			, ii
			, groupIt->m_startTriangle
			, groupIt->m_numTriangles
			, groupIt->m_material.c_str()
			);
	}

	BX_ASSERT(0 == primitives.size(), "Not all primitives are written");

	bx::printf("size: %d\n", uint32_t(bx::seek(&writer) ) );
	bx::close(&writer);

	delete [] table;
	delete [] indexData;
	delete [] vertexData;

	now = bx::getHPCounter();
	convertElapsed += now;

	bx::printf("parse %f [s]\ntri reorder %f [s]\nconvert %f [s]\ng %d, p %d, v %d, i %d\n"
		, double(parseElapsed)/bx::getHPFrequency()
		, double(triReorderElapsed)/bx::getHPFrequency()
		, double(convertElapsed)/bx::getHPFrequency()
		, uint32_t(mesh.m_groups.size()-1)
		, writtenPrimitives
		, writtenVertices
		, writtenIndices
		);

	return bx::kExitSuccess;
}

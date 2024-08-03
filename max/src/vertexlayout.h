/*
 * Copyright 2024 Marcus Madland. All rights reserved.
 * License: https://github.com/marcusmadland/max/blob/mainICENSE
 */

#ifndef MAX_VERTEXDECL_H_HEADER_GUARD
#define MAX_VERTEXDECL_H_HEADER_GUARD

#include <max/max.h>
#include <bx/readerwriter.h>

namespace max
{
	///
	void initAttribTypeSizeTable(RendererType::Enum _type);

	///
	bool isFloat(AttribType::Enum _type);

	/// Returns attribute name.
	const char* getAttribName(Attrib::Enum _attr);

	///
	const char* getAttribNameShort(Attrib::Enum _attr);

	///
	Attrib::Enum idToAttrib(uint16_t id);

	///
	uint16_t attribToId(Attrib::Enum _attr);

	///
	AttribType::Enum idToAttribType(uint16_t id);

	///
	int32_t write(bx::WriterI* _writer, const max::VertexLayout& _layout, bx::Error* _err = NULL);

	///
	int32_t read(bx::ReaderI* _reader, max::VertexLayout& _layout, bx::Error* _err = NULL);

	///
	uint32_t weldVertices(void* _output, const VertexLayout& _layout, const void* _data, uint32_t _num, bool _index32, float _epsilon, bx::AllocatorI* _allocator);

} // namespace max

#endif // MAX_VERTEXDECL_H_HEADER_GUARD

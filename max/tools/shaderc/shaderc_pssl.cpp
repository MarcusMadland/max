/*
 * Copyright 2024 Marcus Madland. All rights reserved.
 * License: https://github.com/marcusmadland/max/blob/mainICENSE
 */

#include "shaderc.h"

namespace max
{
	bool compilePSSLShader(const Options& _options, uint32_t _version, const std::string& _code, bx::WriterI* _shaderWriter, bx::WriterI* _messageWriter)
	{
		BX_UNUSED(_options, _version, _code, _shaderWriter);
		bx::ErrorAssert messageErr;
		bx::write(_messageWriter, &messageErr, "PSSL compiler is not supported.\n");
		return false;
	}

	const char* getPsslPreamble()
	{
		return "";
	}

} // namespace max

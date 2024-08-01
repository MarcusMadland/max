# bgfx.cmake - bgfx building in cmake
# Written in 2017 by Joshua Brookover <joshua.al.brookover@gmail.com>
#
# To the extent possible under law, the author(s) have dedicated all copyright
# and related and neighboring rights to this software to the public domain
# worldwide. This software is distributed without any warranty.
#
# You should have received a copy of the CC0 Public Domain Dedication along with
# this software. If not, see <http://creativecommons.org/publicdomain/zero/1.0/>.

include(3rdparty/meshoptimizer.cmake)
include(3rdparty/jolt.cmake)
include(bgfx.cmake)

if(BGFX_BUILD_TOOLS_GEOMETRY)
	include(geometryc.cmake)
endif()
if(BGFX_BUILD_TOOLS_SHADER)
	include(3rdparty/spirv-opt.cmake)
	include(3rdparty/spirv-cross.cmake)
	include(3rdparty/glslang.cmake)
	include(3rdparty/glsl-optimizer.cmake)
	include(3rdparty/fcpp.cmake)
	include(3rdparty/webgpu.cmake)
	include(shaderc.cmake)
endif()

include(shared.cmake)
include(examples.cmake)

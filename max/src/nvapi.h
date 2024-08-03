/*
 * Copyright 2024 Marcus Madland. All rights reserved.
 * License: https://github.com/marcusmadland/max/blob/mainICENSE
 */

#ifndef MAX_NVAPI_H_HEADER_GUARD
#define MAX_NVAPI_H_HEADER_GUARD

struct ID3D11Device;
struct ID3D11DeviceContext;
struct ID3D11Buffer;
struct ID3D12Device;
struct ID3D12CommandList;

namespace max
{
	struct NvPhysicalGpuHandle;
	struct NvAftermathContextHandle;

	struct NvAftermathDeviceStatus
	{
		enum Enum
		{
			Active,
			Timeout,
			OutOfMemory,
			PageFault,
			Unknown,
			NotInitialized
		};
	};

	typedef void (*PFN_NVAPI_MULTIDRAWINDIRECT)(ID3D11DeviceContext* _deviceCtx, uint32_t _numDrawIndirect, ID3D11Buffer* _ptr, uint32_t _offset, uint32_t _stride);

	///
	struct NvApi
	{
		///
		NvApi();

		///
		void init();

		///
		void shutdown();

		///
		bool isInitialized() const { return NULL != m_nvGpu; }

		///
		void getMemoryInfo(int64_t& _gpuMemoryUsed, int64_t& _gpuMemoryMax);

		///
		bool loadAftermath();

		///
		bool initAftermath(const ID3D11Device* _device, const ID3D11DeviceContext* _deviceCtx);

		///
		bool initAftermath(const ID3D12Device* _device, const ID3D12CommandList* _commandList);

		///
		NvAftermathDeviceStatus::Enum getDeviceStatus() const;

		///
		void shutdownAftermath();

		///
		void setMarker(const bx::StringView& _marker);

		///
		void* m_nvApiDll;
		NvPhysicalGpuHandle* m_nvGpu;

		void* m_nvAftermathDll;
		NvAftermathContextHandle* m_aftermathHandle;

		PFN_NVAPI_MULTIDRAWINDIRECT nvApiD3D11MultiDrawInstancedIndirect;
		PFN_NVAPI_MULTIDRAWINDIRECT nvApiD3D11MultiDrawIndexedInstancedIndirect;
	};

} // namespace max

#endif // MAX_NVAPI_H_HEADER_GUARD

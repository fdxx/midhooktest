
#include <Zydis.h>
#include <safetyhook.hpp>
#include <cstdint>
#include <cstdio>
#include <sys/mman.h>
#include <unistd.h>
#include <string.h>

class ExecMemory
{
public:
	ExecMemory(const void *data, size_t size)
	{
		m_dataSize = size;
		long pageSize = sysconf(_SC_PAGESIZE);
		m_allocSize = (m_dataSize + pageSize - 1) & ~(pageSize - 1);

		m_pAddress = mmap(nullptr, m_allocSize, (PROT_READ|PROT_WRITE|PROT_EXEC), (MAP_PRIVATE|MAP_ANONYMOUS), -1, 0);
		if (m_pAddress != MAP_FAILED)
			memcpy(m_pAddress, data, m_dataSize);
	}
	
	~ExecMemory()
	{
		if (m_pAddress != MAP_FAILED)
			munmap(m_pAddress, m_allocSize);
	}
	
	template<typename T>
	T GetFunction()
	{
		return m_pAddress != MAP_FAILED ? (T)m_pAddress : nullptr;
	}

	template<typename T>
	void Write(int offset, T data)
	{
		uintptr_t ptr = uintptr_t(m_pAddress) + offset;
		*(T*)ptr = data;
	}

	size_t m_dataSize, m_allocSize;
	void *m_pAddress;
};


SAFETYHOOK_NOINLINE int add(int x, int y)
{
	return x + y;
}

void hooked_add(SafetyHookContext& ctx, void *data)
{
	const char *str = (char*)data;
	printf("str = %s\n", str);
	ctx.eax = 1337;
}

template <typename R, typename... Args>
void* GetTargetAddr(R (*pfunc)(Args...))
{
	ZydisDecoder decoder{};
	ZydisDecoderInit(&decoder, ZYDIS_MACHINE_MODE_LEGACY_32, ZYDIS_STACK_WIDTH_32);

	uint8_t *addr = (uint8_t*)pfunc;
	while (*addr != 0xC3)
	{
		ZydisDecodedInstruction ix{};
		ZydisDecoderDecodeInstruction(&decoder, nullptr, reinterpret_cast<void*>(addr), 15, &ix);

		// Follow JMPs
		if (ix.opcode == 0xE9)
			addr += ix.length + (int32_t)ix.raw.imm[0].value.s;
		else
			addr += ix.length;
	}
	return addr;
}


int main(int argc, char *argv[])
{
	int x = strtol(argv[1], nullptr, 10);
	int y = strtol(argv[2], nullptr, 10);
	printf("add(%i, %i) = %i\n", x, y, add(x, y));

	uint8_t asmCode[] = {0x83, 0xEC, 0x14, 0xFF, 0x35, 0x17, 0x00, 0x00, 0x00, 0xFF, 0x74, 0x24, 0x1C, 0xFF, 0x15, 0x1B, 0x00, 0x00, 0x00, 0x83, 0xC4, 0x1C, 0xC3, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
	ExecMemory execMem(asmCode, sizeof(asmCode));

	execMem.Write(execMem.m_dataSize - 4, uintptr_t(hooked_add));
	execMem.Write(15, uintptr_t(execMem.m_pAddress) + execMem.m_dataSize - 4);

	const char *data = "hello!";
	execMem.Write(execMem.m_dataSize - 8, uintptr_t(data));
	execMem.Write(5, uintptr_t(execMem.m_pAddress) + execMem.m_dataSize - 8);

	void *addr = GetTargetAddr(add);
	SafetyHookMid hook = safetyhook::create_mid(addr, execMem.GetFunction<safetyhook::MidHookFn>());
	if (!hook.enabled())
	{
		printf("Failed to enable hook\n");
		return 0;
	}

	printf("hooked_add(%i, %i) = %i\n", x, y, add(x, y));
	return 0;
}

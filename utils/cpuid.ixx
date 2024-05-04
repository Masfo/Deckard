module;
#include <intrin.h>

export module deckard.cpuid;

import std;
import deckard.types;

using namespace std::chrono_literals;

namespace deckard::cpuid
{
	enum class cpu_register : u32
	{
		eax,
		ebx,
		ecx,
		edx
	};

	export class CPUID
	{
		std::array<u32, 4> regs;

	public:
		CPUID() = default;

		explicit CPUID(unsigned id) { read(id); }

		void read(unsigned id)
		{
			regs.fill(0);
#if defined(_MSC_VER)
			__cpuid((int *)regs.data(), (int)id);
#endif
		}

		u32 speed() const noexcept
		{

			u32 time_variable = 100'000'000;

			u64  cycle = 0;
			auto start = std::chrono::high_resolution_clock::now();

			auto current_cycle = __rdtsc();
			while (cycle <= time_variable)
				cycle = __rdtsc() - current_cycle;

			std::chrono::duration<f64, std::milli> total_time(std::chrono::high_resolution_clock::now() - start);

			return static_cast<u32>(std::round((1000.0 / total_time.count()) * (time_variable / 1'000'000.0)));
		}

		u32 operator[](enum class cpu_register index) { return regs[u32(index)]; }

		const u32 &EAX() const { return regs[0]; }

		const u32 &EBX() const { return regs[1]; }

		const u32 &ECX() const { return regs[2]; }

		const u32 &EDX() const { return regs[3]; }
	};

} // namespace deckard::cpuid

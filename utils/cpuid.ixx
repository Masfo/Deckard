module;
#include <intrin.h>

export module deckard.cpuid;

import std;
import deckard.types;
import deckard.helpers;

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

	struct Features
	{
		std::string  name;
		u32          id;
		cpu_register reg;
		u32          bit;
	};

	std::array<Features, 12> g_features = {{{"MMX", 1, cpu_register::edx, 23},
											{"SSE", 1, cpu_register::edx, 25},
											{"SSE2", 1, cpu_register::edx, 25},
											{"SSE3", 1, cpu_register::ecx, 0},
											{"SSE4.1", 1, cpu_register::ecx, 19},

											{"SSE4.2", 1, cpu_register::ecx, 20},
											{"AES", 1, cpu_register::ecx, 25},
											{"SHA1", 7, cpu_register::ebx, 29},
											{"AVX", 1, cpu_register::ecx, 28},
											{"AVX2", 7, cpu_register::ebx, 5},

											{"AVX512", 7, cpu_register::ebx, 16},
											{"RDRAND", 1, cpu_register::ecx, 30}}};

	constexpr bool is_bit_set(u64 value, u32 bitindex) noexcept { return ((value >> bitindex) & 1) ? true : false; }

	export class CPUID

	{
	private:
		std::array<u32, 4> regs{0};

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

		std::string features() const noexcept
		{
			std::string ret;

			for (const auto &feature : g_features)
			{
				CPUID id(feature.id);
				if (is_bit_set(id[feature.reg], feature.bit))
				{
					ret += feature.name;
					ret += " ";
				}
			}
			ret[ret.size() - 1] = 0;
			return ret;
		}

		std::string vendor() const noexcept
		{
			std::string ret;
			ret.reserve(16);

			CPUID id(0);

			ret += std::string(reinterpret_cast<const char *>(&id.EBX()), 4);
			ret += std::string(reinterpret_cast<const char *>(&id.EDX()), 4);
			ret += std::string(reinterpret_cast<const char *>(&id.ECX()), 4);

			return ret;
		}

		std::string brand() noexcept
		{


			if (CPUID id(0x8000'0000); id.EAX() >= 0x8000'0004)
			{
				std::string ret;
				ret.resize(48);
#if defined(_MSC_VER)
				__cpuidex((int *)(&ret[0] + 00), 0x8000'0002, 0);
				__cpuidex((int *)(&ret[0] + 16), 0x8000'0003, 0);
				__cpuidex((int *)(&ret[0] + 32), 0x8000'0004, 0);
#endif
				// Remove whitespace in the end
				ret.resize(ret.size() - 1);
				return ret.substr(0, ret.find_last_not_of(' ') + 1);
			}
			return {};
		}

		u32 speed_in_mhz() const noexcept
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

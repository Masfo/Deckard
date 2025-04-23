module;
#include <Windows.h>
#include <intrin.h>

export module deckard.cpuid;

import std;
import deckard.types;
import deckard.helpers;
import deckard.as;

using namespace std::chrono_literals;
using namespace std::string_view_literals;

namespace deckard::cpuid
{
	enum class Vendor : u32
	{
		AMD,
		Intel,
		Unknown,
	};


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

	struct CPU_Info
	{
		std::string vendor_string;
		std::string brand_string;
		std::string features_string;


		u32 speed_in_mhz{0};

		u32 stepping{0};

		u32 model{0};
		u32 exmodel{0};

		u32 family{0};
		u32 exfamily{0};

		u32 type{0};

		u32 cores{0};
		u32 threads{0};

		Vendor vendor{Vendor::Unknown};
	};

	enum class Feature : u32
	{
		MMX = 0,

		SSE,
		SSE2,
		SSE3,
		SSE41,
		SSE42,
		SSE4a,

		AVX,
		AVX2,
		AVX512,

		SHA,
		AES,

		RDRAND,
		RDSEED,

		MAX_FEATURE
	};

	const std::array<Features, std::to_underlying(Feature::MAX_FEATURE)> g_features = {{
	  {"MMX", 1, cpu_register::edx, 23},

	  {"SSE", 1, cpu_register::edx, 25},
	  {"SSE2", 1, cpu_register::edx, 26},
	  {"SSE3", 1, cpu_register::ecx, 0},
	  {"SSE4.1", 1, cpu_register::ecx, 19},
	  {"SSE4.2", 1, cpu_register::ecx, 20},
	  {"SSE4a", 0x8000'0001, cpu_register::ecx, 6},

	  {"AVX", 1, cpu_register::ecx, 28},
	  {"AVX2", 7, cpu_register::ebx, 5},
	  {"AVX512", 7, cpu_register::ebx, 16},

	  {"SHA", 7, cpu_register::ebx, 29},
	  {"AES", 1, cpu_register::ecx, 25},

	  {"RDRAND", 1, cpu_register::ecx, 30},
	  {"RDSEED", 7, cpu_register::ebx, 18},

	  //{"HTT", 1, cpu_register::edx, 28},
	}};

	constexpr bool is_bit_set(u64 value, u32 bitindex) { return ((value >> bitindex) & 1) ? true : false; }

	export extern "C" bool has_cpuid(); // cpuid.asm

	export auto cpuid(int id) -> std::array<u32, 4>
	{
		std::array<u32, 4> regs{0};
		__cpuid(std::bit_cast<i32*>(regs.data()), id);
		return regs;
	}

	export auto cpuidex(int id, int leaf) -> std::array<u32, 4>
	{
		std::array<u32, 4> regs{0};
		__cpuidex(std::bit_cast<i32*>(regs.data()), id, leaf);
		return regs;
	}

	export class CPUID
	{
		union FeatureInformationFamily
		{
			struct _Details
			{
				u32 stepping : 4;
				u32 model : 4;
				u32 family : 4;
				u32 type : 2;

				u32 reserved1 : 2;

				u32 exModel : 4;
				u32 exFamily : 8;

				u32 reserved2 : 4;
			};

			_Details details;

			u32 value;
		};


	private:
		std::array<u32, 4> regs{0};

	public:
		CPUID()
			: CPUID(0)
		{
		}

		explicit CPUID(unsigned id) { read(id); }

		void read(unsigned id) { regs = cpuid(id); }

		bool has(Feature f) const
		{
			const auto& feature = g_features[std::to_underlying(f)];
			CPUID       id(feature.id);

			return is_bit_set(id[feature.reg], feature.bit);
		}

		std::string feature_string() const
		{
			std::string ret;
			ret.reserve(64);

			for (const auto& feature : g_features)
			{
				CPUID id(feature.id);
				if (is_bit_set(id[feature.reg], feature.bit))
				{
					ret += feature.name;
					ret += " ";
				}
			}
			return ret.substr(0, ret.size() - 1);
		}

		Vendor vendor() const
		{
			CPUID id(0);

			// AuthenticAMD
			if ((id.EBX() == 0x6874'7541) && (id.ECX() == 0x444D'4163) && (id.EDX() == 0x6974'6E65))
				return Vendor::AMD;

			// GenuineIntel
			if ((id.EBX() == 0x756E'6547) && (id.ECX() == 0x6C65'746E) && (id.EDX() == 0x4965'6E69))
				return Vendor::Intel;

			return Vendor::Unknown;
		}

		std::string vendor_string() const
		{
			std::string ret;
			ret.reserve(16);

			CPUID id(0);

			ret += std::string(std::bit_cast<const char*>(&id.EBX()), 4);
			ret += std::string(std::bit_cast<const char*>(&id.EDX()), 4);
			ret += std::string(std::bit_cast<const char*>(&id.ECX()), 4);

			return ret;
		}

		std::string brand_string() const
		{


			if (CPUID id(0x8000'0000); id.EAX() >= 0x8000'0004)
			{
				std::string ret;
				ret.resize(48);
#if defined(_MSC_VER)
				__cpuidex(std::bit_cast<int*>(&ret[0] + 00), 0x8000'0002, 0);
				__cpuidex(std::bit_cast<int*>(&ret[0] + 16), 0x8000'0003, 0);
				__cpuidex(std::bit_cast<int*>(&ret[0] + 32), 0x8000'0004, 0);
#endif
				// Remove whitespace in the end
				return ret.substr(0, ret.find_last_not_of(" \0"sv) + 1);
			}
			return {};
		}

		u32 speed_in_mhz() const
		{
			const auto delay = 20ms;

			auto       timer_start = std::chrono::high_resolution_clock::now();
			const auto timer_end   = timer_start + delay;


			auto cycle_start = __rdtsc();
			while (timer_start <= timer_end)
				timer_start = std::chrono::high_resolution_clock::now();

			u32 hz_count = as<u32>((__rdtsc() - cycle_start) * (1000 / delay.count()));

			return hz_count / std::mega::num;
		}

		[[deprecated("On Intel does not work")]] u32 logical_cores() const
		{
			auto [mfi, _, __, ___] = cpuid(0);

			switch (vendor())
			{
				case Vendor::Intel:
				{
					if (mfi < 0xb)
					{
						if (mfi < 0)
							return 0;

						auto [_1, ebx, __1, ___1] = cpuid(1);
						u32 logical               = (ebx >> 16) & 0xFF;
						return logical;
					}

					auto [_2, ebx_intel, __2, ___2] = cpuidex(0xb, 0);
					return ebx_intel & 0xFFFF;
				}

				case Vendor::AMD:
				{
					auto [_3, ebx_amd, __3, ___3] = cpuid(1);
					return (ebx_amd >> 16) & 0xFF;
				}

				default: return 0;
			}
			std::unreachable();
		}

		[[deprecated("On Intel does not work")]] u32 threads_per_core() const
		{

			//
			auto [mfi, _1, __1, edx] = cpuid(0);
			if (mfi < 0x4)
				return 1;

			if (mfi < 0xb)
			{
				if (vendor() == Vendor::AMD)
					return 1;


				auto [_2, ebx, __2, ___2] = cpuid(1);
				if ((edx & (1 << 28)) != 0)
				{
					u32 v = (ebx >> 16) & 0xFF;
					if (v > 1)
					{
						auto [eax, _3, __3, ___3] = cpuid(4);
						u32 v2                    = (eax >> 26) + 1;
						if (v2 > 0)
						{
							return (u32)(v / v2);
						}
					}
				}

				return 1;
			}

			auto [_4, ebx, __4, ___4] = cpuidex(0xb, 0);
			if ((ebx & 0xFFFF) == 0)
			{
				if (vendor() == Vendor::AMD)
				{
					CPU_Info i                    = info();
					auto [_5, __5, ___5, edx_amd] = cpuid(1);
					if ((edx_amd & (1 << 28)) != 0 && i.family >= 23)
						return 2;
				}
				return 1;
			}

			u32 tpc = ebx & 0xFFFF;

			auto [_6, __6, ___6, edx2] = cpuid(1);

			u32 HTT = 0;
			HTT     = (((edx2 & (1 << 28)) != 0) and (mfi >= 4)) && tpc > 1;

			return tpc;
		}

		auto core_count() const -> std::pair<u32, u32>
		{
			u32 threads{0};
			u32 cores{threads};

			// u32 tpc = threads_per_core();
			//  u32 lc  = logical_cores();
			//
			//  cores   = (lc / tpc == 0) ? lc : lc / tpc;
			//  threads = lc;
			//
			//  return {cores, threads};

			if (GetProcAddress(GetModuleHandleA("kernel32"), "GetLogicalProcessorInformation") != nullptr)
			{
				PSYSTEM_LOGICAL_PROCESSOR_INFORMATION buffer{nullptr};
				PSYSTEM_LOGICAL_PROCESSOR_INFORMATION ptr{nullptr};
				DWORD                                 retlen{0};

				if (GetLogicalProcessorInformation(buffer, &retlen) == FALSE)
				{
					if (GetLastError() == ERROR_INSUFFICIENT_BUFFER)
					{
						std::free(buffer);
						buffer = static_cast<PSYSTEM_LOGICAL_PROCESSOR_INFORMATION>(std::malloc(((u64)retlen)));

						if (buffer == nullptr)
							goto backup_method;

						if (GetLogicalProcessorInformation(buffer, &retlen) == TRUE)
						{
							ptr              = buffer;
							DWORD byteOffset = 0;

							while (u64(byteOffset) + sizeof(SYSTEM_LOGICAL_PROCESSOR_INFORMATION) <= retlen)
							{
								switch (ptr->Relationship)
								{
									case RelationProcessorCore:
										cores++;
										threads += std::popcount(ptr->ProcessorMask);
										break;

									default: break;
								}
								byteOffset += sizeof(SYSTEM_LOGICAL_PROCESSOR_INFORMATION);
								ptr++;
							}

							std::free(buffer);
							buffer = nullptr;
							ptr    = nullptr;
						}
					}
				}
			}
			else
			{
			backup_method:
				SYSTEM_INFO si{};
				GetSystemInfo(&si);
				cores = threads = si.dwNumberOfProcessors;
			}

			return {cores, threads};
		}

		CPU_Info info() const
		{
			CPU_Info                 i;
			CPUID                    id(1);
			FeatureInformationFamily info;

			info.value = id.EAX();

			i.family = info.details.family;

			//
			if (i.family != 0xF)
				i.exfamily = info.details.family;
			else
				i.exfamily = info.details.exFamily + info.details.family;

			i.model = info.details.model;

			//
			if (i.family == 0x6 or i.family == 0xF)
				i.exmodel = (info.details.exModel << 4) | info.details.model;
			else
				i.exfamily = info.details.model;


			i.stepping = info.details.stepping;
			i.type     = info.details.type;

			i.speed_in_mhz    = speed_in_mhz();
			i.brand_string    = brand_string();
			i.vendor_string   = vendor_string();
			i.features_string = feature_string();

			i.vendor = vendor();

			const auto [cores, threads] = core_count();
			i.cores                     = cores;
			i.threads                   = threads;

			return i;
		}

		std::string as_string() const
		{

			CPU_Info    i = info();
			std::string ret;
			ret.reserve(256);
			ret = std::format(
			  "{} ({}MHz)\nCores: {}, Threads {}\nFamily: {:X}, Ext. Family: {:X}\nModel: {:X}, Ext. Model: {:X}\nStepping: "
			  "{:X}\nFeatures: {}",
			  i.brand_string,
			  i.speed_in_mhz,
			  i.cores,
			  i.threads,
			  i.family,
			  i.exfamily,
			  i.model,
			  i.exmodel,
			  i.stepping,
			  i.features_string);

			//
			return ret;
		}

		u32 operator[](enum class cpu_register index) const { return regs[u32(index)]; }

		const u32& EAX() const { return regs[0]; }

		const u32& EBX() const { return regs[1]; }

		const u32& ECX() const { return regs[2]; }

		const u32& EDX() const { return regs[3]; }
	};

} // namespace deckard::cpuid

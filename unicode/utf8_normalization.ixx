export module deckard.utf8:normalization;

import deckard.types;
import :view;
import :codepoints;
import :tables;
import :string;

namespace deckard::utf8
{
	// NFC + NFD

	namespace normalization
	{
		[[nodiscard]] constexpr auto plain_decompose(char32_t cp) noexcept -> std::optional<std::pair<char32_t, char32_t>>
		{
			auto it = std::ranges::lower_bound(
			  decomp_table, cp, std::ranges::less{}, &std::pair<char32_t, std::pair<char32_t, char32_t>>::first);
			if (it == decomp_table.end() or it->first != cp)
				return std::nullopt;
			return it->second;
		}

		[[nodiscard]] constexpr auto plain_compose(char32_t starter, char32_t combiner) noexcept -> std::optional<char32_t>
		{
			auto key = std::pair<char32_t, char32_t>{starter, combiner};
			auto it  = std::ranges::lower_bound(
			  compose_table, key, std::ranges::less{}, &std::pair<std::pair<char32_t, char32_t>, char32_t>::first);
			if (it == compose_table.end() or it->first != key)
				return std::nullopt;
			return it->second;
		}

		[[nodiscard]] constexpr auto decompose(char32 cp) noexcept -> std::optional<std::pair<char32, char32>>
		{
#if 1
			return plain_decompose(cp);
#else
			auto const key = static_cast<u64>(cp) << 42;
			auto       it  = std::ranges::lower_bound(decomp_span, key);
			if (it == decomp_span.end() or field_a(*it) != cp)
				return std::nullopt;
			return std::pair{field_b(*it), field_c(*it)};
#endif
		}

		[[nodiscard]] constexpr auto compose(char32 starter, char32 combiner) noexcept -> std::optional<char32>
		{
#if 1
			return plain_compose(starter, combiner);
#else

			auto const key = kComposeBit | (static_cast<u64>(starter) << 42) | (static_cast<u64>(combiner) << 21);
			auto       it  = std::ranges::lower_bound(compose_span, key);
			if (it == compose_span.end() or ((*it) >> 21) != (key >> 21))
				return std::nullopt;


			return field_c(*it);
#endif
		}

		// ############################################################################################
		// CCC ########################################################################################

		[[nodiscard]] constexpr auto get_ccc(u32 cp) noexcept -> u8
		{
			if (cp < MIN_CODEPOINT or cp > MAX_CODEPOINT)
				return 0;

			const u32 block_idx = cp >> 6;
			const u32 low_idx   = cp & 63;

			if (block_idx >= trie_directory.size())
				return 0;

			const u16    block_id = trie_directory[block_idx];
			const size_t offset   = (static_cast<size_t>(block_id) << 6) + low_idx;

			return trie_data[offset];
		}

		// Hangul
		inline constexpr char32 SBase  = 0xAC00;
		inline constexpr char32 LBase  = 0x1100;
		inline constexpr char32 VBase  = 0x1161;
		inline constexpr char32 TBase  = 0x11A7;
		inline constexpr char32 LCount = 19;
		inline constexpr char32 VCount = 21;
		inline constexpr char32 TCount = 28;
		inline constexpr char32 NCount = VCount * TCount; // 588
		inline constexpr char32 SCount = LCount * NCount; // 11172

		[[nodiscard]] constexpr bool is_hangul_syllable(char32 cp) noexcept { return cp >= SBase and cp < SBase + SCount; }

		[[nodiscard]] constexpr bool is_hangul_leading(char32 cp) noexcept { return cp >= LBase and cp < LBase + LCount; }

		[[nodiscard]] constexpr bool is_hangul_vowel(char32 cp) noexcept { return cp >= VBase and cp < VBase + VCount; }

		[[nodiscard]] constexpr bool is_hangul_trailing(char32 cp) noexcept { return cp > TBase and cp < TBase + TCount; }

		void decompose_one(char32 cp, std::vector<char32>& out)
		{
			if (is_hangul_syllable(cp))
			{
				const auto SIndex = cp - SBase;
				const auto L      = LBase + SIndex / NCount;
				const auto V      = VBase + (SIndex % NCount) / TCount;
				const auto T      = TBase + SIndex % TCount;

				out.push_back(L);
				out.push_back(V);
				if (T != TBase)
					out.push_back(T);
				return;
			}

			if (auto pair = decompose(cp))
			{
				decompose_one(pair->first, out);
				if (pair->second != 0)
					decompose_one(pair->second, out);
				return;
			}

			out.push_back(cp);
		}

		void canonical_sort(std::vector<char32>& codepoints)
		{
			auto it  = codepoints.begin();
			auto end = codepoints.end();

			while (it != end)
			{
				it             = std::find_if(it, end, [](char32 c) { return get_ccc(c) != 0; });
				auto block_end = std::find_if(it, end, [](char32 c) { return get_ccc(c) == 0; });

				if (std::distance(it, block_end) > 1)
					std::stable_sort(it, block_end, [](char32 a, char32 b) noexcept { return get_ccc(a) < get_ccc(b); });

				it = block_end;
			}
		}

		[[nodiscard]] std::vector<char32> to_nfd_codepoints(const utf8::string& s)
		{
			std::vector<char32> result;
			result.reserve(s.size());

			for (char32 cp : s)
				decompose_one(cp, result);

			canonical_sort(result);
			return result;
		}

		[[nodiscard]] std::vector<char32> compose_codepoints(std::vector<char32> nfd)
		{
			if (nfd.empty())
				return {};

			std::vector<char32> result;
			result.reserve(nfd.size());

			result.push_back(nfd[0]);
			size_t last_starter_idx = 0;
			u8     max_ccc          = 0; // Tracks the highest CCC in the current combining sequence

			for (size_t i = 1; i < nfd.size(); ++i)
			{
				char32_t&    L        = result[last_starter_idx];
				const char32 C   = nfd[i];
				const u8     ccc = get_ccc(C);
				bool         composed = false;

				if (ccc == 0)
				{
					const bool adjacent = (last_starter_idx == result.size() - 1);
					composed = false;
					if (adjacent)
					{
						char32& L = result[last_starter_idx];

						if (is_hangul_leading(L) and is_hangul_vowel(C))
						{
							L        = SBase + ((L - LBase) * NCount) + (C - VBase) * TCount;
							composed = true;
						}
						else if (is_hangul_syllable(L) and is_hangul_trailing(C) and (L - SBase) % TCount == 0)
						{
							L        = L + (C - TBase);
							composed = true;
						}
						else if (auto r = compose(L, C))
						{
							L        = *r;
							composed = true;
						}
					}

					if (not composed)
					{
						last_starter_idx = result.size();
						result.push_back(C);
						max_ccc = 0; // Reset tracking for the new starter segment
					}
				}
				else
				{
					// Character is blocked if any character seen since the last starter has a CCC >= ccc
					// Exception: If they are structurally adjacent (no characters between them), it's never blocked.
					const bool between = (last_starter_idx != result.size() - 1);
					const bool blocked = between and (max_ccc >= ccc);

					if (not blocked)
					{
						if (auto r = compose(L, C))
						{
							L        = *r;
							composed = true;
						}
					}

					if (not composed)
					{
						result.push_back(C);
						max_ccc = std::max(max_ccc, ccc);
					}
				}
			}

			return result;
		}

	} // namespace normalization

	// ############################################################################################
	// Normalize functions ########################################################################

	export [[nodiscard]] utf8::string to_nfd(const utf8::string& s)
	{
		if (s.empty())
			return {};

		auto nfd = normalization::to_nfd_codepoints(s);

		string result;
		result.reserve(s.size_in_bytes());
		for (char32 cp : nfd)
			result.append(cp);

		return result;
	}

	export [[nodiscard]] string to_nfc(const string& s)
	{
		if (s.empty())
			return {};

		auto nfd      = normalization::to_nfd_codepoints(s);
		auto composed = normalization::compose_codepoints(std::move(nfd));

		string result;
		result.reserve(s.size_in_bytes());
		for (char32 cp : composed)
			result.append(cp);

		return result;
	}

} // namespace deckard::utf8

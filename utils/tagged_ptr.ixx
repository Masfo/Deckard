export module deckard.tagged_ptr;

import std;
import deckard.types;

namespace deckard
{
	/* Usage:
	* 
		enum class node_kind : u8 { leaf, internal };

		struct node
		{
			int   value;
			node* left  = nullptr;
			node* right = nullptr;
		};

		using node_ptr = tagged_ptr<node, node_kind, 2>;

	 */

	template<typename Tag>
	concept tag_enum = std::is_scoped_enum_v<Tag>;

	static constexpr unsigned max_tag_bits = 8;


	export template<typename T, tag_enum Tag, unsigned TagBits = 2>
	class tagged_ptr
	{
		using underlying = std::underlying_type_t<Tag>;

		static_assert(TagBits > 0 and TagBits <= max_tag_bits, "tag_bits must leave room for an actual address");
		static_assert(alignof(T) >= (usize{1} << TagBits),
					  "T's alignment is too small to hold TagBits tag bits; "
					  "increase alignas(T) or reduce TagBits");

		static constexpr usize tag_mask = (usize{1} << TagBits) - 1;
		static constexpr usize ptr_mask = ~tag_mask;

		usize bits{};

		[[nodiscard]] static constexpr auto to_bits(Tag tag) noexcept -> usize
		{
			return static_cast<usize>(static_cast<underlying>(tag)) & tag_mask;
		}

	public:
		tagged_ptr() noexcept = default;

		tagged_ptr(T* ptr, Tag tag) noexcept { set(ptr, tag); }

		auto set(T* ptr, Tag tag) noexcept -> void
		{
			auto const p = std::bit_cast<usize>(ptr);
			bits         = (p & ptr_mask) | to_bits(tag);
		}

		auto set_tag(Tag tag) noexcept -> void { bits = (bits & ptr_mask) | to_bits(tag); }

		auto set_ptr(T* ptr) noexcept -> void
		{
			auto const p = std::bit_cast<usize>(ptr);
			bits         = (p & ptr_mask) | (bits & tag_mask);
		}

		[[nodiscard]] auto ptr() const noexcept -> T* { return std::bit_cast<T*>(bits & ptr_mask); }

		[[nodiscard]] auto tag() const noexcept -> Tag { return static_cast<Tag>(static_cast<underlying>(bits & tag_mask)); }

		[[nodiscard]] auto operator*() const noexcept -> T& { return *ptr(); }

		[[nodiscard]] auto operator->() const noexcept -> T* { return ptr(); }

		[[nodiscard]] explicit operator bool() const noexcept { return ptr() != nullptr; }

		friend auto operator==(tagged_ptr const&, tagged_ptr const&) noexcept -> bool = default;
	};

	enum class state : u8
	{
		clean,
		dirty,
	};
	static_assert(sizeof(tagged_ptr<int, state, 2>) == sizeof(void*), "tagged_ptr size mismatch");
} // namespace deckard

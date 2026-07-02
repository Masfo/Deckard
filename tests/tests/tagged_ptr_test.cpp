#include <catch2/catch_test_macros.hpp>

import std;
import deckard.types;
import deckard.tagged_ptr;

TEST_CASE("tagged_ptr", "[tagged_ptr]")
{
	using namespace deckard;

	enum class state : std::uint8_t
	{
		clean,
		dirty,
		pending
	};

	using state_ptr = tagged_ptr<int, state, 2>;

	static_assert(sizeof(state_ptr) == sizeof(void*), "state_ptr should be the same size as a pointer");

	SECTION("default constructor")
	{
		state_ptr p;
		CHECK(p.ptr() == nullptr);
		CHECK(not bool(p));
	}

	SECTION("constructor(ptr, tag)")
	{
		int       val = 42;
		state_ptr p{&val, state::clean};
		CHECK(p.ptr() == &val);
		CHECK(p.tag() == state::clean);
		CHECK(bool(p));
	}

	SECTION("set")
	{
		int       val = 42;
		state_ptr p;
		p.set(&val, state::dirty);
		CHECK(p.ptr() == &val);
		CHECK(*p.ptr() == 42);
		CHECK(p.tag() == state::dirty);
	}

	SECTION("set_tag preserves ptr")
	{
		int       val = 7;
		state_ptr p{&val, state::clean};
		p.set_tag(state::pending);
		CHECK(p.ptr() == &val);
		CHECK(p.tag() == state::pending);
	}

	SECTION("set_ptr preserves tag")
	{
		int       a = 1, b = 2;
		state_ptr p{&a, state::dirty};
		p.set_ptr(&b);
		CHECK(p.ptr() == &b);
		CHECK(p.tag() == state::dirty);
	}

	SECTION("operator*")
	{
		int       val = 99;
		state_ptr p{&val, state::clean};
		CHECK(*p == 99);
	}

	SECTION("operator->")
	{
		struct alignas(4) node
		{
			int value;
		};
		enum class nstate : std::uint8_t
		{
			a,
			b
		};
		using node_ptr = tagged_ptr<node, nstate, 2>;

		node     n{55};
		node_ptr p{&n, nstate::a};
		CHECK(p->value == 55);
	}

	SECTION("operator bool")
	{
		state_ptr null_p;
		CHECK(not bool(null_p));

		int       val = 0;
		state_ptr valid{&val, state::clean};
		CHECK(bool(valid));
	}

	SECTION("equality")
	{
		int       val = 42;
		state_ptr a{&val, state::clean};
		state_ptr b{&val, state::clean};
		state_ptr c{&val, state::dirty};
		state_ptr d;

		CHECK(a == b);
		CHECK(not(a == c));
		CHECK(d == state_ptr{});
	}

	SECTION("all tag values")
	{
		int       val = 0;
		state_ptr p{&val, state::clean};
		CHECK(p.tag() == state::clean);

		p.set_tag(state::dirty);
		CHECK(p.tag() == state::dirty);

		p.set_tag(state::pending);
		CHECK(p.tag() == state::pending);

		CHECK(p.ptr() == &val);
	}

	SECTION("1-bit tag")
	{
		enum class flag : u8
		{
			off,
			on
		};

		struct alignas(2) item
		{
			u16 value;
		};

		using flag_ptr = tagged_ptr<item, flag, 1>;

		item     x{10};
		flag_ptr p{&x, flag::off};
		CHECK(p.tag() == flag::off);

		p.set_tag(flag::on);
		CHECK(p.tag() == flag::on);
		CHECK(p.ptr() == &x);
	}

	SECTION("nodes")
	{
		enum class node_kind : u8 { leaf, internal };

		struct node
		{
			int   value;
			node* left  = nullptr;
			node* right = nullptr;
		};
		using node_ptr = tagged_ptr<node, node_kind, 2>;

		CHECK(sizeof(node_ptr) == sizeof(void*));

		node root{1};
		node left_child{2};
		node right_child{3};

		root.left  = &left_child;
		root.right = &right_child;

		node_ptr root_ptr{&root, node_kind::internal};
		node_ptr left_ptr{&left_child, node_kind::leaf};
		node_ptr right_ptr{&right_child, node_kind::leaf};

		CHECK(root_ptr.tag() == node_kind::internal);
		CHECK(root_ptr->value == 1);
		CHECK(root_ptr->left == left_ptr.ptr());
		CHECK(root_ptr->right == right_ptr.ptr());

		CHECK(left_ptr.tag() == node_kind::leaf);
		CHECK(left_ptr->value == 2);

		CHECK(right_ptr.tag() == node_kind::leaf);
		CHECK(right_ptr->value == 3);

		left_ptr.set_tag(node_kind::internal);
		CHECK(left_ptr.tag() == node_kind::internal);
		CHECK(left_ptr.ptr() == &left_child);
	}
}

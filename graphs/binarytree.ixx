export module deckard.graph:binarytree;
import deckard.log;

import std;
import deckard.debug;
import deckard.types;
import deckard.assert;
import deckard.function_ref;

namespace deckard::graph::binary
{

	export enum class order {
		in,
		pre,
		post,
		level,
	};

	export template<typename T>
	class tree
	{
		struct Node
		{
			std::unique_ptr<Node> left;
			std::unique_ptr<Node> right;
			T                     data;

			template<typename X = T>
			Node(X&& ele, std::unique_ptr<Node>&& lt, std::unique_ptr<Node>&& rt)
				: left{std::move(lt)}
				, right{std::move(rt)}
				, data{std::forward<X>(ele)}
			{
			}
		};

		std::unique_ptr<Node> root{nullptr};
		u32                   count;


	public:
		using visitor = function_ref<void(const T&)>;

		using nodeptr = std::unique_ptr<Node>;

		tree()
			: root{nullptr}
			, count{0}
		{
		}

		tree(const tree& other)
			: root{clone(other.root)}
			, count{other.count}
		{
		}

		tree& operator=(const tree& other)
		{
			tree tmp(other);
			std::swap(root, tmp.root);
			std::swap(count, tmp.count);

			return *this;
		}

		tree(tree&& other)
			: tree()
		{
			std::swap(root, other.root);
			std::swap(count, other.count);
		}

		tree& operator=(tree&& other)
		{
			std::swap(root, other.root);
			std::swap(count, other.count);
			return *this;
		}

		~tree() = default;

		template<typename Iter>
		tree(Iter first, Iter last)
			: tree()
		{
			using c_tp = typename std::iterator_traits<Iter>::value_type;
			static_assert(std::is_constructible<T, c_tp>::value, "Type mismatch");

			for (auto it = first; it != last; ++it)
			{
				insert(*it);
			}
		}

		tree(const std::initializer_list<T>& lst)
			: tree(std::begin(lst), std::end(lst))
		{
		}

		tree(std::initializer_list<T>&& lst)
			: tree()
		{
			for (auto&& elem : lst)
			{
				insert(std::move(elem));
			}
		}

		tree& operator=(const std::initializer_list<T>& lst)
		{
			tree tmp(lst);
			std::swap(root, tmp.root);
			std::swap(count, tmp.count);

			return *this;
		}

		bool empty() const { return count == 0; }

		const u32 size() const { return count; }


#if __cpp_lib_optional_ref
#error "return optional ref to node"
#endif

		std::optional<Node*> at(const T& value) const { return at_helper(root, value); }

		Node* get(const T& value) const { return get_helper(root, value); }

		// insert
		template<typename X = T, typename... Args>
		void insert(X&& first, Args&&... args)
		{
			if (insert_util(std::forward<X>(first), root) != nullptr)
				++count;
			insert(std::forward<Args>(args)...);
		}

		template<typename X = T>
		auto insert(X&& first) -> Node*
		{
			auto node = insert_util(std::forward<X>(first), root);
			if (node != nullptr)
				++count;
			return node;
		}

		// remove
		template<typename X = T, typename... Args>
		void remove(const X& first, Args&&... args)
		{
			if (remove_util(first, root))
				--count;
			remove(std::forward<Args>(args)...);
		}

		void remove(const T& first)
		{
			if (remove_util(first, root))
				--count;
		}

		nodeptr& get_root() { return root; }

		usize height(const nodeptr& node) const
		{
			if (not node)
				return 0;
			return 1 + std::max(height(node->left), height(node->right));
		}

		usize height() const { return height(root); }

		const T& min_element() const { return min_element(root); }

		const T& max_element() const { return max_element(root); }

		bool contains(const T& x) const { return contains(x, root); }

		bool compare(const tree& other) const { return compare_util(root, other.root); }

		bool operator==(const tree& other) const { return compare(other); }

		void visit(visitor v) const { visit_inorder(v); }

		void visit(order ord, visitor v) const
		{
			switch (ord)
			{
				default: [[fallthrough]];
				case order::in: visit_inorder(v); break;

				case order::pre: visit_preorder(v); break;
				case order::post: visit_postorder(v); break;
				case order::level: visit_levelorder(v); break;
			}
		}

		// left-root-right
		void visit_inorder(visitor v) const
		{
			if (not empty())
				visit_inorder(root, v);
		}

		// root-left-right
		void visit_preorder(visitor v) const
		{
			if (not empty())
				visit_preorder(root, v);
		}

		// left-right-root
		void visit_postorder(visitor v) const
		{
			if (not empty())
				visit_postorder(root, v);
		}

		// level-order (BFS)
		void visit_levelorder(visitor v) const
		{
			if (not empty())
				visit_levelorder(root, v);
		}

		template<typename Container = void, typename Proj = std::identity>
		[[nodiscard]] auto collect_leafs(const nodeptr& node, Proj proj = {}) const
		{
			using U      = std::remove_cvref_t<std::invoke_result_t<Proj, const T&>>;
			using Output = std::conditional_t<std::is_void_v<Container>, std::vector<U>, Container>;
			Output out{};

			if (not node)
				return out;

			auto add = [&](const T& value)
			{
				auto projected = std::invoke(proj, value);
				if constexpr (requires { out.push_back(projected); })
					out.push_back(std::move(projected));
				else
					out.insert(std::move(projected));
			};

			auto walk = [&](this auto&& self, const nodeptr& n) -> void
			{
				if (not n)
					return;
				if (not n->left and not n->right)
				{
					add(n->data);
					return;
				}
				self(n->left);
				self(n->right);
			};
			walk(node);
			return out;
		}

		template<typename Container = void, typename Proj = std::identity>
		[[nodiscard]] auto collect_leafs(Proj proj = {}) const
		{
			return collect_leafs<Container>(root, std::move(proj));
		}

		template<typename Proj = std::identity>
		std::generator<std::optional<std::remove_cvref_t<std::invoke_result_t<Proj, const T&>>>> leafs(
		  const nodeptr& node, Proj proj = {}) const
		{
			using U = std::remove_cvref_t<std::invoke_result_t<Proj, const T&>>;
			if (not node)
				co_return;
			std::deque<const Node*> pending;
			pending.emplace_back(node.get());
			while (not pending.empty())
			{
				const auto n = pending.front();
				pending.pop_front();
				if (not n->left and not n->right)
				{
					co_yield std::optional<U>{std::invoke(proj, n->data)};
					continue;
				}
				if (n->left)
					pending.emplace_back(n->left.get());
				if (n->right)
					pending.emplace_back(n->right.get());
			}
		}

		template<typename Proj = std::identity>
		std::generator<std::optional<std::remove_cvref_t<std::invoke_result_t<Proj, const T&>>>> leafs(
		  Proj proj = {}) const
		{
			return leafs(root, std::move(proj));
		}

		template<typename Container = void, typename Proj = std::identity>
		[[nodiscard]] auto collect_at_rank(const nodeptr& root, usize rank, Proj proj = {}) const
		{
			using U      = std::remove_cvref_t<std::invoke_result_t<Proj, const T&>>;
			using Output = std::conditional_t<std::is_void_v<Container>, std::vector<U>, Container>;
			Output out;
			if (rank == 0 or rank > size())
				return out;
			auto add = [&](const T& value)
			{
				auto projected = std::invoke(proj, value);
				if constexpr (requires { out.push_back(projected); })
					out.push_back(std::move(projected));
				else
					out.insert(std::move(projected));
			};

			auto walk = [&](this auto&& self, const nodeptr& n, usize r) -> void
			{
				if (not n or r == 0)
					return;
				if (r == 1)
				{
					add(n->data);
					return;
				}
				self(n->left, r - 1);
				self(n->right, r - 1);
			};

			walk(root, rank);
			return out;
		}

		template<typename Container = void, typename Proj = std::identity>
		[[nodiscard]] auto collect_at_rank(const usize rank, Proj proj = {}) const
		{
			return collect_at_rank<Container>(root, rank, std::move(proj));
		}

		template<typename Proj = std::identity>
		std::generator<std::optional<std::remove_cvref_t<std::invoke_result_t<Proj, const T&>>>> at_rank(
		  const usize rank, Proj proj = {}) const
		{
			using U = std::remove_cvref_t<std::invoke_result_t<Proj, const T&>>;
			if (rank == 0 or rank > size())
				co_return;

			std::deque<std::pair<const Node*, usize>> pending;
			pending.emplace_back(root.get(), 1);
			while (not pending.empty())
			{
				const auto [node, node_rank] = pending.front();
				pending.pop_front();
				if (node_rank == rank)
				{
					co_yield std::optional<U>{std::invoke(proj, node->data)};
					continue;
				}
				if (node->left)
					pending.emplace_back(node->left.get(), node_rank + 1);
				if (node->right)
					pending.emplace_back(node->right.get(), node_rank + 1);
			}
		}

		void clear()
		{
			root.reset(nullptr);
			count = 0;
		}

		void print() { print(root); }

		[[nodiscard]] std::string to_string() const { return to_string(root); }

		[[nodiscard]] std::string to_string(const nodeptr& node) const
		{
			std::string out;
			if (node)
				append_tree(out, node, {}, true, true);
			return out;
		}

	private:
		bool contains(const T& x, const nodeptr& node) const
		{
			auto temp = node.get();

			while (temp != nullptr)
				if (x < temp->data)
					temp = temp->left.get();
				else if (temp->data < x)
					temp = temp->right.get();
				else
					return true;

			return false;
		}

		bool compare_util(const nodeptr& left, const nodeptr& right) const
		{
			if (not left and not right)
				return true;
			else if (not left or not right)
				return false;
			else if (left->data != right->data)
				return false;

			return compare_util(left->left, right->left) and compare_util(left->right, right->right);
		}

		nodeptr clone(const nodeptr& node) const
		{
			if (not node)
				return nullptr;

			return std::make_unique<Node>(node->data, clone(node->left), clone(node->right));
		}

		template<typename X = T>
		Node* insert_util(X&& x, nodeptr& node)
		{
			Node* ret = nullptr;
			if (node == nullptr)
			{
				node = std::make_unique<Node>(std::forward<X>(x), nullptr, nullptr);
				ret  = node.get();
			}
			else if (x < node->data)
				ret = insert_util(std::forward<X>(x), node->left);
			else if (node->data < x)
				ret = insert_util(std::forward<X>(x), node->right);

			return ret;
		}

		bool remove_util(const T& x, nodeptr& node)
		{
			if (node == nullptr)
				return false;

			if (x < node->data)
				return remove_util(x, node->left);
			else if (node->data < x)
				return remove_util(x, node->right);
			else if (node->left != nullptr && node->right != nullptr)
			{
				// Two children
				const T successor = min_element(node->right);
				node->data        = successor;
				return remove_util(successor, node->right);
			}
			else
			{
				// One children
				nodeptr oldNode{std::move(node)};
				node = (oldNode->left != nullptr) ? std::move(oldNode->left) : std::move(oldNode->right);
				return true;
			}
		}

		std::optional<Node*> at_helper(const nodeptr& node, const T value) const
		{
			const auto ret = get_helper(node, value);
			if (ret == nullptr)
				return std::nullopt;
			return ret;
		}

		Node* get_helper(const nodeptr& node, const T value) const
		{
			if (!node)
				return nullptr;

			if (node->data == value)
				return node.get();
			else if (value < node->data)
				return get_helper(node->left, value);
			else
				return get_helper(node->right, value);
		}

		const T& min_element(const nodeptr& node) const
		{
			auto temp = node.get();

			if (temp != nullptr)
				while (temp->left != nullptr)
					temp = temp->left.get();

			return temp->data;
		}

		const T& max_element(const nodeptr& node) const
		{
			auto temp = node.get();

			if (temp != nullptr)
				while (temp->right != nullptr)
					temp = temp->right.get();

			return temp->data;
		}

		// left-root-right
		void visit_inorder(const nodeptr& node, const visitor v) const
		{
			if (node == nullptr)
				return;

			visit_inorder(node->left, v);
			std::invoke(v, node->data);
			visit_inorder(node->right, v);
		}

		// root-left-right
		void visit_preorder(const nodeptr& node, const visitor v) const
		{
			if (node == nullptr)
				return;
			std::invoke(v, node->data);
			visit_preorder(node->left, v);
			visit_preorder(node->right, v);
		}

		// left-right-root
		void visit_postorder(const nodeptr& node, const visitor v) const
		{
			if (node == nullptr)
				return;
			visit_postorder(node->left, v);
			visit_postorder(node->right, v);
			std::invoke(v, node->data);
		}

		// level-order (BFS)
		void visit_levelorder(const nodeptr& node, const visitor v) const
		{
			if (node == nullptr)
				return;

			std::deque<const Node*> q;

			q.push_back(root.get());
			while (not q.empty())
			{
				const auto current = q.front();
				q.pop_front();

				std::invoke(v, current->data);

				if (current->left)
					q.push_back(current->left.get());
				if (current->right)
					q.push_back(current->right.get());
			}
		}

		void print(const nodeptr& node) { log::println("{}", to_string(node)); }


		void append_tree(std::string& out, const nodeptr& n, const std::string& prefix, bool is_last, bool is_root) const
		{
			const auto branch = is_last ? "└── " : "├── ";
			auto       it     = std::back_inserter(out);

			if (!n) // placeholder for a missing child
			{
				std::format_to(it, "{}{}·\n", prefix, branch);
				return;
			}

			if (is_root)
				std::format_to(it, "{}\n", n->data);
			else
				std::format_to(it, "{}{}{}\n", prefix, branch, n->data);

			if (!n->left && !n->right)
				return;

			const auto child_prefix = is_root ? std::string{} : prefix + (is_last ? "    " : "│   ");

			append_tree(out, n->left, child_prefix, false, false);
			append_tree(out, n->right, child_prefix, true, false);
		}
	};

	// #################################

	export template<typename T, std::unsigned_integral U = u16>
	class flat_tree
	{
	private:
		struct node
		{
			T value{};
			U left{};
			U right{};
		};

		static constexpr U null_index = 0;
		std::vector<node>  nodes;

	public:
		explicit flat_tree(size_t capacity = 0)
		{
			nodes.reserve(capacity + 1);
			nodes.push_back(node{}); // index 0 is reserved for null
		}

		[[nodiscard]] auto insert(const T value) -> U
		{
			const auto index = static_cast<U>(nodes.size());

			assert::check(index < std::numeric_limits<U>::max(), "flat_tree: index overflow");

			nodes.emplace_back(value);

			if (index == 1)
				return index;

			u32 cur = 1;
			while (true)
			{
				auto& n = nodes[cur];
				if (value < n.value)
				{
					if (n.left == null_index)
					{
						n.left = index;
						break;
					}
					else
						cur = n.left;
				}
				else
				{
					if (n.right == null_index)
					{
						n.right = index;
						break;
					}
					else
						cur = n.right;
				}
			}

			return index;
		}

		[[nodiscard]] auto find(int value) const noexcept -> u32
		{
			u32 cur = size() == 0 ? null_index : 1;
			while (cur != null_index)
			{
				const auto& n = nodes[cur];
				if (value == n.value)
					return cur;
				else if (value < n.value)
					cur = n.left;
				else
					cur = n.right;
			}
			return null_index;
		}

		[[nodiscard]] auto contains(int value) const noexcept -> bool { return find(value) != null_index; }

		[[nodiscard]] auto min() const noexcept -> std::optional<int>
		{
			if (size() == 0)
				return std::nullopt;
			u32 cur = 1;
			while (nodes[cur].left != null_index)
				cur = nodes[cur].left;
			return nodes[cur].value;
		}

		[[nodiscard]] auto max() const noexcept -> std::optional<int>
		{
			if (size() == 0)
				return std::nullopt;
			u32 cur = 1;
			while (nodes[cur].right != null_index)
				cur = nodes[cur].right;
			return nodes[cur].value;
		}

		[[nodiscard]] auto height(u32 idx = 1) const noexcept -> u32
		{
			if (idx == null_index or size() == 0)
				return 0;
			const auto& n = nodes[idx];
			return 1 + std::max(height(n.left), height(n.right));
		}

		[[nodiscard]] auto get(u32 index) const -> const node&
		{
			assert::check(index < nodes.size(), "flat_tree: index out of bounds");
			return nodes[index];
		}

		[[nodiscard]] bool empty() const noexcept { return size() == 0; }

		void clear() noexcept
		{
			nodes.clear();
			nodes.emplace_back(); // null_index
		}

		[[nodiscard]] auto size() const -> size_t
		{
			return nodes.size() - 1; // exclude the null node
		}

		[[nodiscard]] auto capacity() const -> size_t
		{
			return nodes.capacity() - 1; // exclude the null node
		}
	};


} // namespace deckard::graph::binary

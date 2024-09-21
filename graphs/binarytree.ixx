export module deckard.graph:binarytree;

import std;
import deckard.debug;
import deckard.types;
import deckard.function_ref;

export namespace deckard::graph::binary
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
			insert_util(std::forward<X>(first), root);
			++count;
			insert(std::forward<Args>(args)...);
		}

		template<typename X = T>
		auto insert(X&& first) -> Node*
		{
			++count;
			return insert_util(std::forward<X>(first), root);
		}

		// remove
		template<typename X = T, typename... Args>
		void remove(const X& first, Args&&... args)
		{
			if (count == 0)
				return;

			remove_util(first, root);
			--count;
			remove(std::forward<Args>(args)...);
		}

		void remove(const T& first)
		{
			if (count == 0)
				return;

			remove_util(first, root);
			--count;
		}

		const T& min_element() const { return min_element(root); }

		const T& max_element() const { return max_element(root); }

		bool has(const T& x) const { return has(x, root); }

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

		void clear()
		{
			root.reset(nullptr);
			count = 0;
		}

	private:
		bool has(const T& x, const nodeptr& node) const
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
			if (!left && !right)
				return true;
			else if (!left || !right)
				return false;
			else if (left->data != right->data)
				return false;

			return compare_util(left->left, right->left) && compare_util(left->right, right->right);
		}

		nodeptr clone(const nodeptr& node) const
		{
			if (!node)
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

		void remove_util(const T& x, nodeptr& node)
		{
			if (node == nullptr)
				return;

			if (x < node->data)
				remove_util(x, node->left);
			else if (node->data < x)
				remove_util(x, node->right);
			else if (node->left != nullptr && node->right != nullptr)
			{
				// Two children
				node->data = find_min(node->right);
				remove_util(node->data, node->right);
			}
			else
			{
				// One children
				nodeptr oldNode{std::move(node)};
				node = (oldNode->left != nullptr) ? std::move(oldNode->left) : std::move(oldNode->right);
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

			std::queue<const Node*> q;
			q.push(root.get());
			while (not q.empty())
			{
				const auto current = q.front();
				q.pop();

				std::invoke(v, current->data);

				if (current->left)
					q.push(current->left.get());
				if (current->right)
					q.push(current->right.get());
			}
		}
	};
} // namespace deckard::graph::binary

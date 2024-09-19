export module deckard.graph:avltree;

import std;
import deckard.types;
import deckard.debug;

namespace deckard::graph
{

#if 0
	export template<typename T>
	class avl
	{
	public:
		avl()
			: root{nullptr}
			, element_count{0}
		{
		}

		avl(const avl& other)
			: root{clone(other.root)}
			, element_count{other.element_count}
		{
		}

		avl& operator=(const avl& other)
		{
			avl tmp(other);
			std::swap(root, tmp.root);
			std::swap(element_count, tmp.element_count);

			return *this;
		}

		avl(avl&& other) noexcept
			: avl()
		{
			std::swap(root, other.root);
			std::swap(element_count, other.element_count);
		}

		avl& operator=(avl&& other) noexcept
		{
			std::swap(root, other.root);
			std::swap(element_count, other.element_count);
			return *this;
		}

		~avl() noexcept = default;

		template<typename Iter>
		avl(Iter first, Iter last)
			: avl()
		{
			using c_tp = typename std::iterator_traits<Iter>::value_type;
			static_assert(std::is_constructible<T, c_tp>::value, "Type mismatch");

			for (auto it = first; it != last; ++it)
				insert(*it);
		}

		avl(const std::initializer_list<T>& lst)
			: avl(std::begin(lst), std::end(lst))
		{
		}

		avl(std::initializer_list<T>&& lst)
			: avl()
		{
			for (auto&& elem : lst)
			{
				insert(std::move(elem));
			}
		}

		avl& operator=(const std::initializer_list<T>& lst)
		{
			// copy and swap idiom
			avl temp(lst);
			std::swap(root, temp.root);
			std::swap(element_count, temp.element_count);

			return *this;
		}

	private:
		struct node
		{
			std::unique_ptr<node> left{};
			std::unique_ptr<node> right{};
			T                     data{};
			i32                   height{0};

			template<typename X = T>
			node(X&& elem)
				: left{nullptr}
				, right{nullptr}
				, data{std::forward<X>(elem)}
				, height{0}
			{
			}

			template<typename X = T>
			node(X&& elem, std::unique_ptr<node>&& lt, std::unique_ptr<node>&& rt, i32 h = 0)
				: left{std::move(lt)}
				, right{std::move(rt)}
				, data{std::forward<X>(elem)}
				, height{h}
			{
			}
		};

		using nodeptr = std::unique_ptr<node>;

		u32     element_count{0};
		nodeptr root;


	private:
		nodeptr clone(const nodeptr& node) const
		{
			if (!node)
				return nullptr;
			else
				return std::make_unique<node>(node->data, clone(node->left), clone(node->right), node->height);
		}

		i32 height(const nodeptr& node) const { return node == nullptr ? -1 : node->height; };

		const T& find_min(const nodeptr& node) const
		{
			auto temp = node.get();

			if (temp != nullptr)
			{
				while (temp->left != nullptr)
					temp = temp->left.get();
			}

			return temp->data;
		}

		const T& find_max(const nodeptr& node) const
		{
			auto temp = node.get();
			if (temp != nullptr)
			{
				while (temp->right != nullptr)
					temp = temp->right.get();
			}

			return temp->data;
		}

		void rotate_left(nodeptr& node)
		{
			auto temp  = std::move(node->left);
			temp->left = std::move(temp->right);

			temp->height = std::max(height(temp->left), height(temp->right)) + 1;
			node->height = std::max(height(node->left), height(node->right)) + 1;

			node->right = std::move(temp);
			temp        = std::move(node);
		}

		void rotate_right(nodeptr& node)
		{
			auto temp   = std::move(node->right);
			temp->right = std::move(temp->left);

			node->height = std::max(height(node->left), height(node->right)) + 1;
			temp->height = std::max(height(temp->right), height(temp->left)) + 1;

			node->left = std::move(temp);
			temp       = std::move(node);
		}

		void double_left(nodeptr& node)
		{
			rotate_right(node->left);
			rotate_left(node);
		}

		void double_right(nodeptr& node)
		{
			rotate_left(node->right);
			rotate_right(node);
		}

		void balance(nodeptr& node)
		{
			if (node == nullptr)
				return;
			if (height(node->left) - height(node->right) > 1)
			{
				if (height(node->left->left) >= height(node->left->right))
					rotate_left(node);
				else
					double_left(node);
			}
			else if (height(node->right) - height(node->left) > 1)
			{
				if (height(node->right->right) >= height(node->right->left))
					rotate_right(node);
				else
					double_right(node);
			}

			node->height = std::max(height(node->left), height(node->right)) + 1;
		}

		template<typename X = T>
		void insert_util(X&& x, std::unique_ptr<node>&& node) noexcept
		{
			//	auto nd = std::make_unique<node>(0);

			if (node == nullptr)
				node = node; // std::make_unique<node>(std::forward<X>(x));
			else if (x < node->data)
				insert_util(std::forward<X>(x), node->left);
			else if (node->data < x)
				insert_util(std::forward<X>(x), node->right);

			balance(node);
		}

		void remove_util(const T& x, nodeptr& node)
		{
			//
			if (node == nullptr)
				return;
			if (x < node->data)
				remove_util(x, node->left);
			else if (node->data < x)
				remove_util(x, node->right);
			else if (node->left != nullptr and node->right != nullptr)
			{
				// two children
				node->data = find_min(node->right);
				remove_util(node->data, node->right);
			}
			else
			{
				// one children
				nodeptr old_node{std::move(node)};
				node = (old_node->left != nullptr) ? std::move(old_node->left) : std::move(old_node->right);
			}

			balance(node);
		}

		void print(const nodeptr& node) const noexcept
		{
			if (node != nullptr)
			{
				print(node->left);
				dbg::println("{}, ", node->data);
				print(node->right);
			}
		}

	public:
		bool empty() const { return element_count == 0; }

		u32 size() const { return element_count; }

		template<typename X = T, typename... Args>
		void insert(X&& first, Args&&... args) noexcept
		{
			insert_util(std::forward<X>(first), root);
			++element_count;
			insert(std::forward<Args>(args)...);
		}

		template<typename X = T>
		void insert(X&& first) noexcept
		{
			insert_util(std::forward<X>(first), root);
			++element_count;
		}

		template<typename X = T, typename... Args>
		void remove(const X& first, Args&&... args)
		{
			remove_util(first, root);
			--element_count;
			remove(std::forward<Args>(args)...);
		}

		void remove(const T& first)
		{
			remove_util(first, root);
			--element_count;
		}

		void clear() { root.reset(nullptr); }

		void print() const
		{ //
			if (empty())
				dbg::println("{}");
			else
			{
				dbg::println("{");
				print(root);
				dbg::println("}");
			}
		}
	};
#endif


	export template<typename T>
	class avl
	{
		// node of the tree
		struct Node
		{
			std::unique_ptr<Node> left;
			std::unique_ptr<Node> right;
			T                     data;
			std::int32_t          height;

			template<typename X = T>
			Node(X&& ele, std::unique_ptr<Node>&& lt, std::unique_ptr<Node>&& rt, std::int32_t h = 0)
				: left{std::move(lt)}
				, right{std::move(rt)}
				, data{std::forward<X>(ele)}
				, height{h}
			{
			}
		};

		// top of the tree
		std::unique_ptr<Node> root;

		// Number of elements
		std::size_t sz;

	public:
		// constructors block
		avl()
			: root{nullptr}
			, sz{0}
		{
		}

		avl(const avl& other)
			: root{clone(other.root)}
			, sz{other.sz}
		{
		}

		avl& operator=(const avl& other)
		{
			// copy and swap idiom
			avl tmp(other);
			std::swap(root, tmp.root);
			std::swap(sz, tmp.sz);

			return *this;
		}

		avl(avl&& other) noexcept
			: avl()
		{
			std::swap(root, other.root);
			std::swap(sz, other.sz);
		}

		avl& operator=(avl&& other) noexcept
		{
			std::swap(root, other.root);
			std::swap(sz, other.sz);
			return *this;
		}

		~avl() noexcept = default;

		template<typename Iter>
		avl(Iter first, Iter last)
			: avl()
		{
			using c_tp = typename std::iterator_traits<Iter>::value_type;
			static_assert(std::is_constructible<T, c_tp>::value, "Type mismatch");

			for (auto it = first; it != last; ++it)
			{
				insert(*it);
			}
		}

		avl(const std::initializer_list<T>& lst)
			: avl(std::begin(lst), std::end(lst))
		{
		}

		avl(std::initializer_list<T>&& lst)
			: avl()
		{
			for (auto&& elem : lst)
			{
				insert(std::move(elem));
			}
		}

		avl& operator=(const std::initializer_list<T>& lst)
		{
			// copy and swap idiom
			avl tmp(lst);
			std::swap(root, tmp.root);
			std::swap(sz, tmp.sz);

			return *this;
		}

		// Member functions block
		inline bool empty() const noexcept { return sz == 0; }

		inline const std::size_t& size() const noexcept { return sz; }

		template<typename X = T, typename... Args>
		void insert(X&& first, Args&&... args)
		{
			insert_util(std::forward<X>(first), root);
			++sz;
			insert(std::forward<Args>(args)...);
		}

		template<typename X = T>
		void insert(X&& first)
		{
			insert_util(std::forward<X>(first), root);
			++sz;
		}

		template<typename X = T, typename... Args>
		void remove(const X& first, Args&&... args) noexcept
		{
			remove_util(first, root);
			--sz;
			remove(std::forward<Args>(args)...);
		}

		void remove(const T& first) noexcept
		{
			remove_util(first, root);
			--sz;
		}

		const T& min_element() const
		{
			if (empty())
				throw std::logic_error("Empty container");
			return findMin(root);
		}

		const T& max_element() const
		{
			if (empty())
				throw std::logic_error("Empty container");
			return findMax(root);
		}

		void clear() noexcept { root.reset(nullptr); }

		void print() const noexcept
		{
			if (empty())
				dbg::println("{}");
			else
			{
				dbg::print("{");
				print(root);
				dbg::println("}");
			}
		}

		bool search(const T& x) const noexcept { return search(x, root); }

	private:
		// recursive method to clone a tree
		std::unique_ptr<Node> clone(const std::unique_ptr<Node>& node) const
		{
			if (!node)
				return nullptr;
			else
				return std::make_unique<Node>(node->data, clone(node->left), clone(node->right), node->height);
		}

		// Returns height of a node
		inline std::int32_t height(const std::unique_ptr<Node>& node) const noexcept { return node == nullptr ? -1 : node->height; }

		// print tree inorder
		void print(const std::unique_ptr<Node>& t) const noexcept
		{
			if (t != nullptr)
			{
				print(t->left);
				dbg::print("{}, ", t->data);
				print(t->right);
			}
		}

		// binary search an element in the tree
		bool search(const T& x, const std::unique_ptr<Node>& node) const noexcept
		{
			auto t = node.get();

			while (t != nullptr)
				if (x < t->data)
					t = t->left.get();
				else if (t->data < x)
					t = t->right.get();
				else
					return true;

			return false;
		}

		// Recursive insert method
		template<typename X = T>
		void insert_util(X&& x, std::unique_ptr<Node>& t)
		{
			if (t == nullptr)
				t = std::make_unique<Node>(std::forward<X>(x), nullptr, nullptr);
			else if (x < t->data)
				insert_util(std::forward<X>(x), t->left);
			else if (t->data < x)
				insert_util(std::forward<X>(x), t->right);

			balance(t);
		}

		// Recursive delete method
		void remove_util(const T& x, std::unique_ptr<Node>& t) noexcept
		{
			if (t == nullptr)
				return; // Item not found; do nothing

			if (x < t->data)
				remove_util(x, t->left);
			else if (t->data < x)
				remove_util(x, t->right);
			else if (t->left != nullptr && t->right != nullptr)
			{ // Two children
				t->data = findMin(t->right);
				remove_util(t->data, t->right);
			}
			else
			{ // One children
				std::unique_ptr<Node> oldNode{std::move(t)};
				t = (oldNode->left != nullptr) ? std::move(oldNode->left) : std::move(oldNode->right);

				// oldNode.reset(nullptr); -> unneeded, auto delete when go out of scope
			}

			balance(t);
		}

		// Find smallest elem in a tree
		const T& findMin(const std::unique_ptr<Node>& node) const noexcept
		{
			auto t = node.get();

			if (t != nullptr)
				while (t->left != nullptr)
					t = t->left.get();

			return t->data;
		}

		// Find largest elem in a tree
		const T& findMax(const std::unique_ptr<Node>& node) const noexcept
		{
			auto t = node.get();

			if (t != nullptr)
				while (t->right != nullptr)
					t = t->right.get();

			return t->data;
		}

		// Internal method to re-balance the tree
		void balance(std::unique_ptr<Node>& t) noexcept
		{
			static const int ALLOWED_IMBALANCE = 1;

			if (t == nullptr)
				return;

			if (height(t->left) - height(t->right) > ALLOWED_IMBALANCE)
			{
				if (height(t->left->left) >= height(t->left->right))
					rotateWithLeftChild(t);
				else
					doubleWithLeftChild(t);
			}
			else if (height(t->right) - height(t->left) > ALLOWED_IMBALANCE)
			{
				if (height(t->right->right) >= height(t->right->left))
					rotateWithRightChild(t);
				else
					doubleWithRightChild(t);
			}

			t->height = std::max(height(t->left), height(t->right)) + 1;
		}

		// Rotations block

		/**
		 * Rotate binary tree node with left child.
		 * For AVL trees, this is a single rotation for case 1.
		 * Update heights, then set new root.
		 */
		void rotateWithLeftChild(std::unique_ptr<Node>& k2) noexcept
		{
			auto k1    = std::move(k2->left);
			k2->left   = std::move(k1->right);
			k2->height = std::max(height(k2->left), height(k2->right)) + 1;
			k1->height = std::max(height(k1->left), k2->height) + 1;
			k1->right  = std::move(k2);
			k2         = std::move(k1);
		}

		/**
		 * Rotate binary tree node with right child.
		 * For AVL trees, this is a single rotation for case 4.
		 * Update heights, then set new root.
		 */
		void rotateWithRightChild(std::unique_ptr<Node>& k1) noexcept
		{
			auto k2    = std::move(k1->right);
			k1->right  = std::move(k2->left);
			k1->height = std::max(height(k1->left), height(k1->right)) + 1;
			k2->height = std::max(height(k2->right), k1->height) + 1;
			k2->left   = std::move(k1);
			k1         = std::move(k2);
		}

		/**
		 * Double rotate binary tree node: first left child.
		 * with its right child; then node k3 with new left child.
		 * For AVL trees, this is a double rotation for case 2.
		 * Update heights, then set new root.
		 */
		void doubleWithLeftChild(std::unique_ptr<Node>& k3) noexcept
		{
			rotateWithRightChild(k3->left);
			rotateWithLeftChild(k3);
		}

		/**
		 * Double rotate binary tree node: first right child.
		 * with its left child; then node k1 with new right child.
		 * For AVL trees, this is a double rotation for case 3.
		 * Update heights, then set new root.
		 */
		void doubleWithRightChild(std::unique_ptr<Node>& k1) noexcept
		{
			rotateWithLeftChild(k1->right);
			rotateWithRightChild(k1);
		}

	}; // end of class avl


} // namespace deckard::graph

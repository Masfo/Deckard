export module deckard.graph:avltree;

import std;
import deckard.types;
import deckard.debug;

namespace deckard::graph
{

	export template<typename T>
	class avl
	{
		struct Node
		{
			std::unique_ptr<Node> left;
			std::unique_ptr<Node> right;
			T                     data;
			i32                   height;

			template<typename X = T>
			Node(X&& ele, std::unique_ptr<Node>&& lt, std::unique_ptr<Node>&& rt, i32 h = 0)
				: left{std::move(lt)}
				, right{std::move(rt)}
				, data{std::forward<X>(ele)}
				, height{h}
			{
			}
		};

		std::unique_ptr<Node> root;

		std::size_t count;

	public:
		using nodeptr = std::unique_ptr<Node>;

		avl()
			: root{nullptr}
			, count{0}
		{
		}

		avl(const avl& other)
			: root{clone(other.root)}
			, count{other.count}
		{
		}

		avl& operator=(const avl& other)
		{
			avl tmp(other);
			std::swap(root, tmp.root);
			std::swap(count, tmp.count);

			return *this;
		}

		avl(avl&& other) noexcept
			: avl()
		{
			std::swap(root, other.root);
			std::swap(count, other.count);
		}

		avl& operator=(avl&& other) noexcept
		{
			std::swap(root, other.root);
			std::swap(count, other.count);
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
			avl tmp(lst);
			std::swap(root, tmp.root);
			std::swap(count, tmp.count);

			return *this;
		}

		bool empty() const noexcept { return count == 0; }

		const std::size_t& size() const noexcept { return count; }

		template<typename X = T, typename... Args>
		void insert(X&& first, Args&&... args)
		{
			insert_util(std::forward<X>(first), root);
			++count;
			insert(std::forward<Args>(args)...);
		}

		template<typename X = T>
		void insert(X&& first)
		{
			insert_util(std::forward<X>(first), root);
			++count;
		}

		template<typename X = T, typename... Args>
		void remove(const X& first, Args&&... args) noexcept
		{
			remove_util(first, root);
			--count;
			remove(std::forward<Args>(args)...);
		}

		void remove(const T& first) noexcept
		{
			remove_util(first, root);
			--count;
		}

		const T& min_element() const { return find_min(root); }

		const T& max_element() const { return findMax(root); }

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
		nodeptr clone(const nodeptr& node) const
		{
			if (!node)
				return nullptr;
			else
				return std::make_unique<Node>(node->data, clone(node->left), clone(node->right), node->height);
		}

		i32 height(const nodeptr& node) const noexcept { return node == nullptr ? -1 : node->height; }

		void print(const nodeptr& t) const noexcept
		{
			if (t != nullptr)
			{
				print(t->left);
				dbg::print("{}, ", t->data);
				print(t->right);
			}
		}

		bool search(const T& x, const nodeptr& node) const noexcept
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

		template<typename X = T>
		void insert_util(X&& x, nodeptr& node)
		{
			if (node == nullptr)
				node = std::make_unique<Node>(std::forward<X>(x), nullptr, nullptr);
			else if (x < node->data)
				insert_util(std::forward<X>(x), node->left);
			else if (node->data < x)
				insert_util(std::forward<X>(x), node->right);

			balance(node);
		}

		void remove_util(const T& x, nodeptr& node) noexcept
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

			balance(node);
		}

		const T& find_min(const nodeptr& node) const noexcept
		{
			auto temp = node.get();

			if (temp != nullptr)
				while (temp->left != nullptr)
					temp = temp->left.get();

			return temp->data;
		}

		const T& findMax(const nodeptr& node) const noexcept
		{
			auto temp = node.get();

			if (temp != nullptr)
				while (temp->right != nullptr)
					temp = temp->right.get();

			return temp->data;
		}

		void balance(nodeptr& node) noexcept
		{

			if (node == nullptr)
				return;

			if (height(node->left) - height(node->right) > 1)
			{
				if (height(node->left->left) >= height(node->left->right))
					rotateWithLeftChild(node);
				else
					doubleWithLeftChild(node);
			}
			else if (height(node->right) - height(node->left) > 1)
			{
				if (height(node->right->right) >= height(node->right->left))
					rotateWithRightChild(node);
				else
					doubleWithRightChild(node);
			}

			node->height = std::max(height(node->left), height(node->right)) + 1;
		}

		void rotateWithLeftChild(nodeptr& k2) noexcept
		{
			auto k1    = std::move(k2->left);
			k2->left   = std::move(k1->right);
			k2->height = std::max(height(k2->left), height(k2->right)) + 1;
			k1->height = std::max(height(k1->left), k2->height) + 1;
			k1->right  = std::move(k2);
			k2         = std::move(k1);
		}

		void rotateWithRightChild(nodeptr& k1) noexcept
		{
			auto k2    = std::move(k1->right);
			k1->right  = std::move(k2->left);
			k1->height = std::max(height(k1->left), height(k1->right)) + 1;
			k2->height = std::max(height(k2->right), k1->height) + 1;
			k2->left   = std::move(k1);
			k1         = std::move(k2);
		}

		void doubleWithLeftChild(nodeptr& k3) noexcept
		{
			rotateWithRightChild(k3->left);
			rotateWithLeftChild(k3);
		}

		void doubleWithRightChild(nodeptr& k1) noexcept
		{
			rotateWithLeftChild(k1->right);
			rotateWithRightChild(k1);
		}

	}; // end of class avl


} // namespace deckard::graph

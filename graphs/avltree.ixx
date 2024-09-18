export module deckard.graph:avltree;

import std;
import deckard.types;
import deckard.debug;

namespace deckard::graph
{


	// template<typename T>
	// using GeneratorAlias = deckard::generator<T>;

	export template<typename T>
	struct avlnode
	{
		T                        data{};
		std::unique_ptr<avlnode> left{};

		std::unique_ptr<avlnode> right{};

		i32 height{0};

		avlnode(T value)
			: data(value)
			, left{nullptr}
			, right{nullptr}
			, height{0}
		{
		}
	};

	export template<typename T>
	class avltree
	{
	private:
		using node_t  = avlnode<T>;
		using nodeptr = std::unique_ptr<node_t>;
		nodeptr root{nullptr};

		i32 get_height(const nodeptr& node) const { return node ? node->height : 0; }

		i32 get_balance(const nodeptr& node) const { return get_height(node->right) - get_height(node->left); }

		i32 max_height(const nodeptr& node) const { return std::max(get_height(node->left), get_height(node->right)); }

		nodeptr right_rotate(nodeptr node)
		{
			//
			nodeptr temp = std::move(node->left);
			node->left   = std::move(temp->right);
			temp->right  = std::move(node);

			node->height = max_height(node) + 1;
			temp->height = max_height(temp) + 1;

			return temp;
		}

		nodeptr left_rotate(nodeptr node)
		{
			//
			nodeptr temp = std::move(node->right);
			node->right  = std::move(temp->left);
			temp->left   = std::move(node);

			node->height = max_height(node) + 1;
			temp->height = max_height(temp) + 1;

			return temp;
		}

		nodeptr insert_helper(nodeptr node, T value)
		{
			if (!node)
				return std::make_unique<node_t>(value);

			if (value < node->data)
				node->left = insert_helper(std::move(node->left), value);
			else
				node->right = insert_helper(std::move(node->right), value);

			node->height = max_height(node) + 1;
			i32 balance  = get_balance(node);

			// if (balance > 1 && value > node->right->data)
			//	return left_rotate(node);
			//
			// if (balance < -1 && value < node->left->data)
			//	return right_rotate(node);
			//
			// if (balance < -1 && value > node->left->data)
			//{
			//	node->left = right_rotate(node->left);
			//	return left_rotate(node);
			// }
			//
			// if (balance > 1 && value < node->right->data)
			//{
			//	node->right = left_rotate(node->right);
			//	return right_rotate(node);
			// }

			return node;
		}

		nodeptr delete_helper(nodeptr node, T value)
		{
			//
			if (!node)
				return nullptr;

			if (value < node->data)
				node->left = delete_helper(std::move(node->left), value);
			else if (value > node->data)
				node->right = delete_helper(std::move(node->right), value);
			else
			{
				if (!node->left)
					return std::move(node->right);
				else if (!node->right)
					return std::move(node->left);
				else
				{
					node_t* temp = minvalue(node->right.get());
					node->data   = temp->data;
					node->right  = delete_helper(std::move(node->right), temp->data);
				}
			}
			return node;
		}

		node_t* minvalue(node_t* node) const
		{
			node_t* current = node;
			while (current->left)
				current = current->left.get();
			return current;
		}

		void inorder_helper(const nodeptr& node) const
		{
			if (!node)
				return;

			inorder_helper(node->left);
			dbg::print("{} ", node->data);
			inorder_helper(node->right);
		}

		void preorder_helper(const nodeptr& node) const
		{
			if (!node)
				return;

			dbg::print("{} ", node->data);
			preorder_helper(node->left);
			preorder_helper(node->right);
		}

		void postorder_helper(const nodeptr& node) const
		{
			if (!node)
				return;

			postorder_helper(node->left);
			postorder_helper(node->right);
			dbg::print("{} ", node->data);
		}

		node_t* get_helper(const nodeptr& node, T value) const
		{
			//
			if (!node)
				return nullptr;
			if (node->data == value)
				return node.get();
			else if (value < node->data)
				return get_helper(node->left, value);
			else
				return get_helper(node->right, value);
		}

	public:
		avltree()
			: root(nullptr)
		{
		}

		void insert(const T value) { root = insert_helper(std::move(root), value); }

		void delete_node(const T value) { root = delete_helper(std::move(root), value); };

		void inorder_traversal() const
		{
			inorder_helper(root);
			dbg::println();
		}

		void preorder_traversal() const
		{
			preorder_helper(root);
			dbg::println();
		}

		void postorder_traversal() const
		{
			postorder_helper(root);
			dbg::println();
		}

		node_t* get(const T value) const { return get_helper(root, value); }
	};
} // namespace deckard::graph

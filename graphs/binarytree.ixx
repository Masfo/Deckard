export module deckard.graph;

import deckard.generator;

export namespace deckard::graph
{
	template<typename T>
	using GeneratorAlias = deckard::generator<T>;

	template<typename T>
	struct BinaryTree
	{
		T           value;
		BinaryTree *left{}, *right{};

		// In-Order
		// 1. Traverse the left subtree, i.e., call Inorder(left->subtree)
		// 2. Visit the root.
		// 3. Traverse the right subtree, i.e., call Inorder(right->subtree)
		GeneratorAlias<const T &> traverse_inorder() const
		{
			if (left)
				for (const T &x : left->traverse_inorder())
					co_yield x;

			co_yield value;
			if (right)
				for (const T &x : right->traverse_inorder())
					co_yield x;
		}

		// Preorder
		// 1. Visit the root.
		// 2. Traverse the left subtree, i.e., call Preorder(left->subtree)
		// 3. Traverse the right subtree, i.e.,call Preorder(right->subtree)
		GeneratorAlias<const T &> traverse_preorder() const
		{
			co_yield value;

			if (left)
				for (const T &x : left->traverse_preorder())
					co_yield x;

			if (right)
				for (const T &x : right->traverse_preorder())
					co_yield x;
		}

		// Postorder
		// 1.Traverse the left subtree, i.e., call Postorder(left->subtree)
		// 2. Traverse the right subtree, i.e., call Postorder(right->subtree)
		// 3. Visit the root
		GeneratorAlias<const T &> traverse_postorder() const
		{
			if (left)
				for (const T &x : left->traverse_postorder())
					co_yield x;

			if (right)
				for (const T &x : right->traverse_postorder())
					co_yield x;

			co_yield value;
		}
	};
} // namespace deckard::graph

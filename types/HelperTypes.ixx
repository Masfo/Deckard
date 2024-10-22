export module deckard.helpertypes;

import std;

export namespace deckard
{
	// Result
	using default_errortype = std::string;

	template<typename T, typename E = default_errortype>
	using result = std::expected<T, E>;

	auto ok  = []<typename T>(const T value) -> result<T> { return value; };
	auto err = [](std::string_view fmt, auto... args)
	{
		using namespace std::string_view_literals;
		if constexpr (sizeof...(args) > 0)
			return std::unexpected<default_errortype>(std::vformat(fmt, std::make_format_args(args...)));
		else
			return std::unexpected<default_errortype>(fmt);
	};


	// Option
	template<typename T>
	using option = std::optional<T>;

	auto some = []<typename T>(const T value) -> option<T> { return value; };
	auto none = []<typename T>(const T) -> option<T> { return {}; };


	// box
	template<typename T>
	using box = std::unique_ptr<T>;

	auto makebox = []<typename T>(T value) -> box<T> { return std::make_unique<T>(value); };

	// at_compile
	consteval decltype(auto) at_compile(auto&& arg) { return std::forward<decltype(arg)>(arg); }

	// Passkeys

	template<typename T>
	class PassKey
	{
		friend T;
		PassKey() = default;
	};

	template<typename T>
	class NonCopyablePassKey
	{
		friend T;
		NonCopyablePassKey()                                     = default;
		NonCopyablePassKey(const NonCopyablePassKey&)            = delete;
		NonCopyablePassKey& operator=(const NonCopyablePassKey&) = delete;
	};

} // namespace deckard

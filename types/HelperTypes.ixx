export module deckard.helpertypes;

import std;

export namespace deckard
{
	// Result
	using DefaultErrorType = std::string;

	template<typename T, typename E = DefaultErrorType>
	using Result = std::expected<T, E>;

	auto Ok  = []<typename T>(const T value) -> Result<T> { return value; };
	auto Err = [](std::string_view fmt, auto... args)
	{
		using namespace std::string_view_literals;
		if constexpr (sizeof...(args) > 0)
			return std::unexpected<DefaultErrorType>(std::format("{}"sv, std::vformat(fmt, std::make_format_args(args...))));
		else
			return std::unexpected<DefaultErrorType>(fmt);
	};


	// Option
	template<typename T>
	using Option = std::optional<T>;

	auto Some = []<typename T>(const T value) -> Option<T> { return value; };
	auto None = []<typename T>(const T) -> Option<T> { return {}; };


	// Box
	template<typename T>
	using Box = std::unique_ptr<T>;

	auto MakeBox = []<typename T>(T value) -> Box<T> { return std::make_unique<T>(value); };

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

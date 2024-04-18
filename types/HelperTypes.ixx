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


	template<typename T>
	using Option = std::optional<T>;

	auto Some = []<typename T>(const T value) -> Option<T> { return value; };
	auto None = []<typename T>(const T value) -> Option<T> { return std::nullopt_t; };


} // namespace deckard

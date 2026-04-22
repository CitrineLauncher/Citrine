#pragma once

#include <string>

namespace Citrine {

    template<typename CharT>
    class BasicStringParameter {
    public:

        constexpr BasicStringParameter(std::basic_string<CharT>&& str)

            : str(&str)
            , mySize(static_cast<std::size_t>(-1))
        {}

        constexpr BasicStringParameter(std::basic_string_view<CharT> str)

            : myData(str.data())
            , mySize(str.size())
        {}

        constexpr BasicStringParameter(auto const& str)

            : BasicStringParameter(std::basic_string_view<CharT>{ str })
        {}

        constexpr BasicStringParameter(BasicStringParameter const&) = delete;
        constexpr auto operator=(BasicStringParameter const&) = delete;

        constexpr BasicStringParameter(BasicStringParameter&&) noexcept = default;

        constexpr auto data() const noexcept -> CharT const* {

            if (IsView())
                return myData;
            return str->data();
        }

        constexpr auto size() const noexcept -> std::size_t {

            if (IsView())
                return mySize;
            return str->size();
        }

        constexpr auto begin() const noexcept -> CharT const* {

            if (IsView())
                return myData;
            return str->data();
        }

        constexpr auto end() const noexcept -> CharT const* {

            if (IsView())
                return myData + mySize;
            return str->data() + str->size();
        }

        constexpr auto empty() const noexcept -> bool {

            return size() == 0;
        }

        constexpr operator std::basic_string<CharT>() && {

            if (IsView())
                return { myData, mySize };
            return std::move(*str);
        }

        constexpr auto View() const noexcept -> std::basic_string_view<CharT> {

            if (IsView())
                return { myData, mySize };
            return *str;
        }

    private:

        constexpr auto IsView() const noexcept -> bool {

            return mySize != static_cast<std::size_t>(-1);
        }

        union {

            char const* myData;
            std::string* str;
        };
        std::size_t mySize;
    };

    using StringParameter = BasicStringParameter<char>;
    using WStringParameter = BasicStringParameter<wchar_t>;
}
#include "pch.h"
#include "IntToBoolConverter.h"
#if __has_include("IntToBoolConverter.g.cpp")
#include "IntToBoolConverter.g.cpp"
#endif

namespace winrt {

    using namespace Windows::Foundation;
    using namespace Windows::UI::Xaml::Interop;
}

namespace winrt::Citrine::implementation
{
    auto IntToBoolConverter::Convert(winrt::IInspectable const& value, winrt::TypeName const&, winrt::IInspectable const& parameter, winrt::hstring const&) -> winrt::IInspectable {

        auto val = value && [&] {

            auto v = value.as<winrt::IPropertyValue>();
            auto t = v.Type();

            switch (t) {
            case PropertyType::UInt8:   return v.GetUInt8() != 0;
            case PropertyType::Int16:   return v.GetInt16() != 0;
            case PropertyType::UInt16:  return v.GetUInt16() != 0;
            case PropertyType::Int32:   return v.GetInt32() != 0;
            case PropertyType::UInt32:  return v.GetUInt32() != 0;
            case PropertyType::Int64:   return v.GetInt64() != 0;
            case PropertyType::UInt64:  return v.GetUInt64() != 0;
            default: throw winrt::hresult_no_interface{};
            }
        }();
        auto inverted = parameter && winrt::unbox_value<winrt::hstring>(parameter) == L"Inverted";

        return winrt::box_value(val != inverted);
    }

    auto IntToBoolConverter::ConvertBack(winrt::IInspectable const& value, winrt::TypeName const& targetType, winrt::IInspectable const& parameter, winrt::hstring const&) -> winrt::IInspectable {

        auto val = value && winrt::unbox_value<bool>(value);
        auto inverted = parameter && winrt::unbox_value<winrt::hstring>(parameter) == L"Inverted";

        constexpr auto toU64 = [](std::wstring_view str) static {

            auto v = std::uint64_t{};
            if (str.size() > 6)
                return v;

            for (auto i = 0; i < str.size(); ++i) {

                auto ch = str[i];
                if (ch > 0x7F)
                    return v;

                v |= std::uint64_t{ ch } << (8 * i);
            }
            return v;
        };

        switch (toU64(targetType.Name)) {
        case toU64(L"UInt8"):   return winrt::box_value(static_cast<std::uint8_t>(val != inverted));
        case toU64(L"Int16"):   return winrt::box_value(static_cast<std::int16_t>(val != inverted));
        case toU64(L"UInt16"):  return winrt::box_value(static_cast<std::uint16_t>(val != inverted));
        case toU64(L"Int32"):   return winrt::box_value(static_cast<std::int32_t>(val != inverted));
        case toU64(L"UInt32"):  return winrt::box_value(static_cast<std::uint32_t>(val != inverted));
        case toU64(L"Int64"):   return winrt::box_value(static_cast<std::int64_t>(val != inverted));
        case toU64(L"UInt64"):  return winrt::box_value(static_cast<std::uint64_t>(val != inverted));
        default: return winrt::box_value(static_cast<std::int32_t>(val != inverted));
        }
    }
}

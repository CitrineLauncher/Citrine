#include "pch.h"
#include "NumericValueConverter.h"
#if __has_include("NumericValueConverter.g.cpp")
#include "NumericValueConverter.g.cpp"
#endif

namespace winrt {

    using namespace Windows::Foundation;
    using namespace Windows::UI::Xaml::Interop;
}

namespace winrt::Citrine::implementation
{
    auto NumericValueConverter::Convert(winrt::IInspectable const& value, winrt::TypeName const& targetType, winrt::IInspectable const&, winrt::hstring const&) -> winrt::IInspectable {

        return ConvertInternal(value, targetType);
    }

    auto NumericValueConverter::ConvertBack(winrt::IInspectable const& value, winrt::TypeName const& targetType, winrt::IInspectable const&, winrt::hstring const&) -> winrt::IInspectable {

        return ConvertInternal(value, targetType);
    }

    template<typename To, typename From>
    auto NumericValueConverter::NumericValueCast(From value) noexcept -> To {

        if constexpr (std::floating_point<From>) {

            if (value != value) {

                if constexpr (std::floating_point<To>) {

                    return std::numeric_limits<To>::quiet_NaN();
                }
                else {

                    return To{};
                }
            }
        }
        return static_cast<To>(value);
    }

    auto NumericValueConverter::ConvertInternal(winrt::IInspectable const& value, winrt::TypeName const& targetType) -> winrt::IInspectable {

        constexpr auto convertTo = []<typename T>(winrt::TypeName const& targetType, T value) static -> winrt::IInspectable {

            constexpr auto toU64 = [](std::wstring_view str) static {

                auto v = std::uint64_t{};
                if (str.size() > 6)
                    return 0xFFFFFFFFFFFFFFFF;

                for (auto i = 0; i < str.size(); ++i) {

                    auto ch = str[i];
                    if (ch > 0x7F)
                        return 0xFFFFFFFFFFFFFFFF;

                    v |= std::uint64_t{ ch } << (8 * i);
                }
                return v;
            };

            switch (toU64(targetType.Name)) {
            case toU64(L"UInt8"):   return winrt::box_value(NumericValueCast<std::uint8_t>(value));
            case toU64(L"Int16"):   return winrt::box_value(NumericValueCast<std::int16_t>(value));
            case toU64(L"UInt16"):  return winrt::box_value(NumericValueCast<std::uint16_t>(value));
            case toU64(L"Int32"):   return winrt::box_value(NumericValueCast<std::int32_t>(value));
            case toU64(L"UInt32"):  return winrt::box_value(NumericValueCast<std::uint32_t>(value));
            case toU64(L"Int64"):   return winrt::box_value(NumericValueCast<std::int64_t>(value));
            case toU64(L"UInt64"):  return winrt::box_value(NumericValueCast<std::uint64_t>(value));
            case toU64(L"Single"):  return winrt::box_value(NumericValueCast<float>(value));
            case toU64(L"Double"):  return winrt::box_value(NumericValueCast<double>(value));
            default: throw winrt::hresult_no_interface{};
            }
        };

        if (value) {

            auto v = value.as<winrt::IPropertyValue>();
            auto t = v.Type();

            switch (t) {
            case PropertyType::UInt8:   return convertTo(targetType, v.GetUInt8());
            case PropertyType::Int16:   return convertTo(targetType, v.GetInt16());
            case PropertyType::UInt16:  return convertTo(targetType, v.GetUInt16());
            case PropertyType::Int32:   return convertTo(targetType, v.GetInt32());
            case PropertyType::UInt32:  return convertTo(targetType, v.GetUInt32());
            case PropertyType::Int64:   return convertTo(targetType, v.GetInt64());
            case PropertyType::UInt64:  return convertTo(targetType, v.GetUInt64());
            case PropertyType::Single:  return convertTo(targetType, v.GetSingle());
            case PropertyType::Double:  return convertTo(targetType, v.GetDouble());
            default: throw winrt::hresult_no_interface{};
            }
        }

        return convertTo(targetType, 0);
    }
}

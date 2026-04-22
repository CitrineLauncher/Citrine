#pragma once

#include <vector>
#include <optional>
#include <algorithm>

#include <winrt/Windows.Foundation.h>
#include <winrt/Microsoft.UI.Xaml.Interop.h>

namespace Citrine {

	class BindableVector : public winrt::implements<BindableVector,
		winrt::Microsoft::UI::Xaml::Interop::IBindableVector, winrt::Microsoft::UI::Xaml::Interop::IBindableVectorView, winrt::Microsoft::UI::Xaml::Interop::IBindableIterable>
	{
    public:

        using ValueType = winrt::Windows::Foundation::IInspectable;
        using SizeType = std::uint32_t;

        BindableVector() = default;

        template<typename Container>
        BindableVector(Container&& values)

            : elements(std::forward<Container>(values))
        {}

        auto Size() const noexcept -> SizeType {

            return static_cast<SizeType>(elements.size());
        }

        auto GetAt(SizeType index) const -> ValueType {

            if (index >= elements.size())
                throw winrt::hresult_out_of_bounds{};

            return elements[index];
        }

        auto GetView() -> winrt::Microsoft::UI::Xaml::Interop::IBindableVectorView {

            return *this;
        }

        auto IndexOf(ValueType const& value, SizeType& index) const noexcept -> bool {

            index = static_cast<SizeType>(std::ranges::find(elements, value) - elements.begin());
            return index < elements.size();
        }

        auto Append(ValueType const& value) -> void {

            elements.push_back(value);
        }

        auto InsertAt(SizeType index, ValueType const& value) -> void {

            if (index > elements.size())
                throw winrt::hresult_out_of_bounds{};

            elements.insert(elements.begin() + index, value);
        }

        auto SetAt(SizeType index, ValueType const& value) -> void {

            if (index >= elements.size())
                throw winrt::hresult_out_of_bounds{};

            elements[index] = value;
        }

        auto RemoveAt(SizeType index) -> void {

            if (index >= elements.size())
                throw winrt::hresult_out_of_bounds{};

            elements.erase(elements.begin() + index);
        }

        auto RemoveAtEnd() -> void {

            if (elements.empty())
                throw winrt::hresult_out_of_bounds{};

            elements.pop_back();
        }

        auto Clear() -> void {

            elements.clear();
        }

        auto First() const -> winrt::Microsoft::UI::Xaml::Interop::IBindableIterator {

            return winrt::make<Iterator>(this);
        }

    private:

        struct Iterator : winrt::implements<Iterator, winrt::Microsoft::UI::Xaml::Interop::IBindableIterator> {

            explicit Iterator(BindableVector const* owner) noexcept

                : owner(const_cast<BindableVector*>(owner)->get_strong())
                , current(owner->elements.begin())
                , end(owner->elements.end())
            {}

            auto HasCurrent() const noexcept -> bool {

                return current != end;
            }

            auto Current() const -> ValueType {

                if (current == end)
                    throw winrt::hresult_out_of_bounds{};

                return *current;
            }

            auto MoveNext() noexcept -> bool {

                if (current != end) {

                    ++current;
                    return true;
                }

                return false;
            }

            winrt::com_ptr<BindableVector> owner;
            std::vector<ValueType>::const_iterator current;
            std::vector<ValueType>::const_iterator const end;
        };

        std::vector<ValueType> elements;
	};

    template<typename... Args>
    auto MakeBindableVector(Args&&... args) -> auto {

        return winrt::make<BindableVector>(std::forward<Args>(args)...);
    }

    class SingleItemBindableVector : public winrt::implements<SingleItemBindableVector,
        winrt::Microsoft::UI::Xaml::Interop::IBindableVector, winrt::Microsoft::UI::Xaml::Interop::IBindableVectorView, winrt::Microsoft::UI::Xaml::Interop::IBindableIterable>
    {
    public:

        using ValueType = winrt::Windows::Foundation::IInspectable;
        using SizeType = std::uint32_t;

        SingleItemBindableVector() = default;

        template<typename Container>
        SingleItemBindableVector(Container&& value)

            : element(std::forward<Container>(value))
        {}

        auto Size() const noexcept -> SizeType {

            return static_cast<SizeType>(element.has_value());
        }

        auto GetAt(SizeType index) const -> ValueType {

            if (index >= Size())
                throw winrt::hresult_out_of_bounds{};

            return *element;
        }

        auto GetView() -> winrt::Microsoft::UI::Xaml::Interop::IBindableVectorView {

            return *this;
        }

        auto IndexOf(ValueType const& value, SizeType& index) const noexcept -> bool {

            index = static_cast<SizeType>(element.has_value() && *element != value);
            return index < Size();
        }

        auto Append(ValueType const& value) -> void {

            if (element.has_value())
                throw std::bad_alloc{};

            element.emplace(value);
        }

        auto InsertAt(SizeType index, ValueType const& value) -> void {

            if (index > Size())
                throw winrt::hresult_out_of_bounds{};

            if (element.has_value())
                throw std::bad_alloc{};

            element.emplace(value);
        }

        auto SetAt(SizeType index, ValueType const& value) -> void {

            if (index >= Size())
                throw winrt::hresult_out_of_bounds{};

            *element = value;
        }

        auto RemoveAt(SizeType index) -> void {

            if (index >= Size())
                throw winrt::hresult_out_of_bounds{};

            element.reset();
        }

        auto RemoveAtEnd() -> void {

            if (!element.has_value())
                throw winrt::hresult_out_of_bounds{};

            element.reset();
        }

        auto Clear() -> void {

            element.reset();
        }

        auto First() const -> winrt::Microsoft::UI::Xaml::Interop::IBindableIterator {

            return winrt::make<Iterator>(this);
        }

    private:

        struct Iterator : winrt::implements<Iterator, winrt::Microsoft::UI::Xaml::Interop::IBindableIterator> {

            explicit Iterator(SingleItemBindableVector const* owner) noexcept

                : owner(const_cast<SingleItemBindableVector*>(owner)->get_strong())
                , current(owner->element.has_value() ? &*owner->element : nullptr)
            {}

            auto HasCurrent() const noexcept -> bool {

                return static_cast<bool>(current);
            }

            auto Current() const -> ValueType {

                if (!current)
                    throw winrt::hresult_out_of_bounds{};

                return *current;
            }

            auto MoveNext() noexcept -> bool {

                if (current) {

                    current = nullptr;
                    return true;
                }

                return false;
            }

            winrt::com_ptr<SingleItemBindableVector> owner;
            ValueType const* current;
        };

        std::optional<ValueType> element;
    };

    template<typename... Args>
    auto MakeSingleItemBindableVector(Args&&... args) -> auto {

        return winrt::make<SingleItemBindableVector>(std::forward<Args>(args)...);
    }
}

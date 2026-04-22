#pragma once

#include "Helpers/NotifyCollectionChangedBase.h"
#include "Collections/BindableVector.h"
#include "Collections/FilterableCollectionView.h"

#include "winrt/Citrine.h"

#include <vector>
#include <algorithm>
#include <ranges>

#include <winrt/Windows.Foundation.h>
#include <winrt/Windows.Foundation.Collections.h>
#include <winrt/Microsoft.UI.Xaml.Interop.h>
#include <winrt/Windows.UI.Xaml.Interop.h>

namespace Citrine {

	template<typename>
	class ObservableCollectionT;

	template<template<typename> typename D>
	class ObservableCollectionT<D<winrt::Windows::Foundation::IInspectable>> : public winrt::implements<D<winrt::Windows::Foundation::IInspectable>,
		winrt::Citrine::IObservableCollection,
		winrt::Citrine::IObservableCollectionView,
		winrt::Windows::Foundation::Collections::IVector<winrt::Windows::Foundation::IInspectable>,
		winrt::Windows::Foundation::Collections::IVectorView<winrt::Windows::Foundation::IInspectable>,
		winrt::Windows::Foundation::Collections::IIterable<winrt::Windows::Foundation::IInspectable>,
		winrt::Microsoft::UI::Xaml::Interop::IBindableVector,
		winrt::Microsoft::UI::Xaml::Interop::IBindableVectorView,
		winrt::Microsoft::UI::Xaml::Interop::IBindableIterable,
		winrt::Microsoft::UI::Xaml::Interop::INotifyCollectionChanged,
		IFilterableCollectionViewSource>
	{};

	template<template<typename> typename D, typename T>
	class ObservableCollectionT<D<T>> : public winrt::implements<D<T>,
		winrt::Citrine::IObservableCollection,
		winrt::Citrine::IObservableCollectionView,
		winrt::Windows::Foundation::Collections::IVector<winrt::Windows::Foundation::IInspectable>,
		winrt::Windows::Foundation::Collections::IVectorView<winrt::Windows::Foundation::IInspectable>,
		winrt::Windows::Foundation::Collections::IIterable<winrt::Windows::Foundation::IInspectable>,
		winrt::Windows::Foundation::Collections::IVector<T>,
		winrt::Windows::Foundation::Collections::IVectorView<T>,
		winrt::Windows::Foundation::Collections::IIterable<T>,
		winrt::Microsoft::UI::Xaml::Interop::IBindableVector,
		winrt::Microsoft::UI::Xaml::Interop::IBindableVectorView,
		winrt::Microsoft::UI::Xaml::Interop::IBindableIterable,
		winrt::Microsoft::UI::Xaml::Interop::INotifyCollectionChanged,
		IFilterableCollectionViewSource>
	{};

	template<typename T>
	class ObservableCollection : public ObservableCollectionT<ObservableCollection<T>>, public NotifyCollectionChangedBase {
	public:

		using ValueType = T;
		using SizeType = std::uint32_t;

		ObservableCollection() = default;

		ObservableCollection(std::vector<ValueType>&& values)
			
			: elements(std::move(values))
		{}

		auto Size() const noexcept -> SizeType {

			return static_cast<SizeType>(elements.size());
		}

		auto IsEmpty() const noexcept -> bool {

			return elements.empty();
		}

		auto GetAt(SizeType index) const -> ValueType {

			if (index >= elements.size())
				throw winrt::hresult_out_of_bounds{};

			return elements[index];
		}

		auto GetMany(SizeType startIndex, winrt::array_view<ValueType> values) const -> SizeType {

			if (startIndex >= elements.size())
				return 0;

			auto first = elements.begin() + startIndex;
			auto actualCount = std::min(static_cast<SizeType>(elements.size() - startIndex), values.size());
			std::ranges::copy(first, first + actualCount, values.begin());
			return actualCount;
		}

		auto GetMany(SizeType startIndex, winrt::array_view<winrt::Windows::Foundation::IInspectable> values) const -> SizeType
			requires (!std::same_as<ValueType, winrt::Windows::Foundation::IInspectable>)
		{
			if (startIndex >= elements.size())
				return 0;

			auto first = elements.begin() + startIndex;
			auto actualCount = std::min(static_cast<SizeType>(elements.size() - startIndex), values.size());
			std::ranges::copy(first, first + actualCount, values.begin());
			return actualCount;
		}

		auto GetView() const -> auto {

			struct Result {

				operator winrt::Windows::Foundation::Collections::IVectorView<winrt::Windows::Foundation::IInspectable>() const {

					return *Collection;
				}

				operator winrt::Windows::Foundation::Collections::IVectorView<ValueType>() const
					requires (!std::same_as<ValueType, winrt::Windows::Foundation::IInspectable>)
				{
					return *Collection;
				}

				operator winrt::Microsoft::UI::Xaml::Interop::IBindableVectorView() const {

					return *Collection;
				}

				ObservableCollection const* Collection;
			};
			return Result{ this };
		}

		auto GetObservableView() const -> winrt::Citrine::IObservableCollectionView {

			return *this;
		}

		auto IndexOf(ValueType const& value, SizeType& index) const noexcept -> bool {
			
			index = static_cast<SizeType>(std::ranges::find(elements, value) - elements.begin());
			return index < elements.size();
		}

		auto IndexOf(winrt::Windows::Foundation::IInspectable const& value, SizeType& index) const noexcept -> bool
			requires (!std::same_as<ValueType, winrt::Windows::Foundation::IInspectable>)
		{
			return IndexOf(value.try_as<ValueType>(), index);
		}

		auto Append(ValueType value) -> void {

			elements.emplace_back(value);

			auto newItem = winrt::make<SingleItemBindableVector>(std::move(value));
			OnCollectionChanged(NotifyCollectionChangedAction::Add, newItem, nullptr, static_cast<std::int32_t>(elements.size() - 1), -1);
		}

		auto Append(winrt::Windows::Foundation::IInspectable const& value) -> void
			requires (!std::same_as<ValueType, winrt::Windows::Foundation::IInspectable>)
		{
			Append(value.as<ValueType>());
		}

		auto InsertAt(SizeType index, ValueType value) -> void {

			if (index > elements.size())
				throw winrt::hresult_out_of_bounds{};

			elements.emplace(elements.begin() + index, value);

			auto newItem = winrt::make<SingleItemBindableVector>(std::move(value));
			OnCollectionChanged(NotifyCollectionChangedAction::Add, newItem, nullptr, static_cast<std::int32_t>(index), -1);
		}

		auto InsertAt(SizeType index, winrt::Windows::Foundation::IInspectable const& value) -> void
			requires (!std::same_as<ValueType, winrt::Windows::Foundation::IInspectable>)
		{
			InsertAt(index, value.as<ValueType>());
		}

		auto Move(SizeType oldIndex, SizeType newIndex) -> void {

			if (newIndex >= elements.size() || oldIndex >= elements.size())
				throw winrt::hresult_out_of_bounds{};

			if (newIndex == oldIndex)
				return;

			auto value = std::move(elements[oldIndex]);
			if (newIndex > oldIndex) {

				std::ranges::move(
					elements.begin() + oldIndex + 1,
					elements.begin() + newIndex + 1,
					elements.begin() + oldIndex
				);
			}
			else {

				std::ranges::move_backward(
					elements.begin() + newIndex,
					elements.begin() + oldIndex,
					elements.begin() + oldIndex + 1
				);
			}
			elements[newIndex] = value;

			auto movedItem = winrt::make<SingleItemBindableVector>(std::move(value));
			OnCollectionChanged(NotifyCollectionChangedAction::Move, movedItem, movedItem, static_cast<std::int32_t>(newIndex), static_cast<std::int32_t>(oldIndex));
		}

		auto SetAt(SizeType index, ValueType value) -> void {

			if (index >= elements.size())
				throw winrt::hresult_out_of_bounds{};

			auto oldValue = std::exchange(elements[index], value);

			auto newItem = winrt::make<SingleItemBindableVector>(std::move(value));
			auto oldItem = winrt::make<SingleItemBindableVector>(std::move(oldValue));
			OnCollectionChanged(NotifyCollectionChangedAction::Replace, newItem, oldItem, static_cast<std::int32_t>(index), static_cast<std::int32_t>(index));
		}
		
		auto SetAt(SizeType index, winrt::Windows::Foundation::IInspectable const& value) -> void
			requires (!std::same_as<ValueType, winrt::Windows::Foundation::IInspectable>)
		{
			SetAt(index, value.as<ValueType>());
		}

		auto ReplaceAll(winrt::array_view<ValueType const> values) -> void {

			elements.assign_range(values);
			OnCollectionChanged(NotifyCollectionChangedAction::Reset, nullptr, nullptr, -1, -1);
		}

		auto ReplaceAll(winrt::array_view<winrt::Windows::Foundation::IInspectable const> values) -> void
			requires (!std::same_as<ValueType, winrt::Windows::Foundation::IInspectable>)
		{
			elements.assign_range(values | std::views::transform([](auto const& value) static { return value.template as<ValueType>(); }));
			OnCollectionChanged(NotifyCollectionChangedAction::Reset, nullptr, nullptr, -1, -1);
		}

		auto RemoveAt(SizeType index) -> void {

			if (index >= elements.size())
				throw winrt::hresult_out_of_bounds{};

			auto oldValue = std::move(elements[index]);
			elements.erase(elements.begin() + index);

			auto oldItem = winrt::make<SingleItemBindableVector>(std::move(oldValue));
			OnCollectionChanged(NotifyCollectionChangedAction::Remove, nullptr, oldItem, -1, static_cast<std::int32_t>(index));
		}

		auto RemoveAtEnd() -> void {

			if (elements.empty())
				throw winrt::hresult_out_of_bounds{};

			auto oldValue = std::move(elements.back());
			elements.pop_back();

			auto oldItem = winrt::make<SingleItemBindableVector>(std::move(oldValue));
			OnCollectionChanged(NotifyCollectionChangedAction::Remove, nullptr, oldItem, -1, static_cast<std::int32_t>(elements.size()));
		}

		auto Clear() -> void {

			if (elements.empty())
				return;

			elements.clear();
			OnCollectionChanged(NotifyCollectionChangedAction::Reset, nullptr, nullptr, -1, -1);
		}

		auto First() const -> auto {

			struct Result {

				operator winrt::Windows::Foundation::Collections::IIterator<winrt::Windows::Foundation::IInspectable>() const {

					return winrt::make<Iterator>(Collection);
				}

				operator winrt::Windows::Foundation::Collections::IIterator<ValueType>() const
					requires (!std::same_as<ValueType, winrt::Windows::Foundation::IInspectable>)
				{
					return winrt::make<TypedIterator>(Collection);
				}

				operator winrt::Microsoft::UI::Xaml::Interop::IBindableIterator() const {

					return winrt::make<BindableIterator>(Collection);
				}

				ObservableCollection const* Collection;
			};
			return Result{ this };
		}

		auto ItemType() const -> winrt::Windows::UI::Xaml::Interop::TypeName {

			return winrt::xaml_typename<ValueType>();
		}

		auto CreateFilterableView(void* itemProjection) -> winrt::Citrine::IFilterableCollectionView final override {

			using FilterableViewT = FilterableCollectionView<ValueType, std::vector<ValueType>>;
			return winrt::make<FilterableViewT>(*this, elements, itemProjection);
		}

		auto Underlying() const noexcept -> std::vector<ValueType> const& {

			return elements;
		}

		auto Underlying(std::vector<ValueType>&& values) noexcept -> void {

			elements = std::move(values);
			OnCollectionChanged(NotifyCollectionChangedAction::Reset, nullptr, nullptr, -1, -1);
		}

	private:

		template<typename I>
		struct BasicIterator : winrt::implements<BasicIterator<I>, I> {

			explicit BasicIterator(ObservableCollection const* owner) noexcept

				: owner(const_cast<ObservableCollection*>(owner)->get_strong())
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

			auto GetMany(winrt::array_view<winrt::Windows::Foundation::IInspectable> values) -> SizeType
				requires std::same_as<I, winrt::Windows::Foundation::Collections::IIterator<winrt::Windows::Foundation::IInspectable>>
			{
				auto actualCount = std::min(static_cast<SizeType>(std::distance(current, end)), values.size());
				std::ranges::copy(current, current + actualCount, values.begin());
				std::advance(current, actualCount);
				return actualCount;
			}

			auto GetMany(winrt::array_view<ValueType> values) -> SizeType
				requires std::same_as<I, winrt::Windows::Foundation::Collections::IIterator<ValueType>>
			{
				auto actualCount = std::min(static_cast<SizeType>(std::distance(current, end)), values.size());
				std::ranges::copy(current, current + actualCount, values.begin());
				std::advance(current, actualCount);
				return actualCount;
			}

		private:

			winrt::com_ptr<ObservableCollection> owner;
			std::vector<ValueType>::const_iterator current;
			std::vector<ValueType>::const_iterator const end;
		};

		using Iterator = BasicIterator<winrt::Windows::Foundation::Collections::IIterator<winrt::Windows::Foundation::IInspectable>>;
		using TypedIterator = BasicIterator<winrt::Windows::Foundation::Collections::IIterator<ValueType>>;
		using BindableIterator = BasicIterator<winrt::Microsoft::UI::Xaml::Interop::IBindableIterator>;

		std::vector<ValueType> elements;
	};

	template<typename T>
	auto MakeObservableCollection() -> winrt::Citrine::IObservableCollection {

		return winrt::make<ObservableCollection>();
	}
	template<typename T>
	auto MakeObservableCollection(std::vector<T>&& values) -> winrt::Citrine::IObservableCollection {

		return winrt::make<ObservableCollection>(std::move(values));
	}
}
#pragma once

#include "Helpers/NotifyCollectionChangedBase.h"
#include "Collections/BindableVector.h"
#include "Core/Unicode/Unicode.h"

#include "winrt/Citrine.h"

#include <array>
#include <string>
#include <vector>
#include <algorithm>
#include <ranges>
#include <tuple>

#include <winrt/Windows.Foundation.h>
#include <winrt/Windows.Foundation.Collections.h>
#include <winrt/Microsoft.UI.Xaml.Interop.h>
#include <winrt/Windows.UI.Xaml.Interop.h>
#include <unknwnbase.h>

namespace Citrine {

	struct __declspec(uuid("5722acc0-8392-41c3-ac85-c6b92867985a")) IFilterableCollectionViewSource : public ::IUnknown {
	
		virtual auto CreateFilterableView(void* itemProjection) -> winrt::Citrine::IFilterableCollectionView = 0;
	};

	class FilterableCollectionViewBase {
	protected:

		static auto NormalizeString(std::wstring_view input, std::wstring& output) -> void {

			Unicode::NormalizeWhitespace(input, output);
			Unicode::FoldCaseInPlace(output);
		}

		template<typename T>
		using ItemProjection = auto(*)(T const&, std::array<std::wstring, 8>&) -> std::size_t;

		enum struct FilterChange {

			None,
			Reset,
			Extended,
			Shrunken
		};

		class FilterBase {
		public:

			auto Update(winrt::hstring const& value) -> FilterChange {

				rawString = value;
				NormalizeString(rawString, buffer);
				current.swap(buffer);

				tokens.clear();
				{
					auto it = current.data();
					auto const end = it + current.size();

					while (true) {

						auto last = std::ranges::find(it, end, L' ');
						if (last == end)
							break;

						tokens.emplace_back(it, last);
						it = last + 1;
					}

					if (it < end)
						tokens.emplace_back(it, end);
				}

				auto& previous = buffer;
				using enum FilterChange;

				if (current.size() > previous.size() && current.contains(previous))
					return Extended;

				if (previous.size() > current.size() && previous.contains(current))
					return Shrunken;

				if (current == previous)
					return None;

				return Reset;
			}

			auto IsEmpty() const noexcept -> bool {

				return tokens.empty();
			}

			auto Get() const noexcept -> winrt::hstring const& {

				return rawString;
			}

		protected:

			FilterBase() = default;

			auto MatchFields(std::wstring const* fields, std::size_t count, std::size_t startIndex = 0, std::array<bool, 8> matches = {}, std::size_t depth = 1) const noexcept -> bool {

				for (auto i = 0uz; i < count; ++i) {

					if (matches[i]) continue;
					auto field = std::wstring_view{ fields[i] };
					auto token = tokens[startIndex];

					if (auto pos = field.find(token); pos != field.npos) {

						field.remove_prefix(pos);
						auto index = startIndex + 1;

						while (index < tokens.size() && field.starts_with({ token.begin(), tokens[index].end() })) {

							field.remove_prefix(token.size() + 1);
							token = tokens[index++];
						}
						if (index == tokens.size()) return true;

						matches[i] = true;
						if (depth < count && MatchFields(fields, count, index, matches, depth + 1)) return true;
					}
				}
				return false;
			}

			winrt::hstring rawString;
			std::wstring current;
			std::wstring buffer;
			std::vector<std::wstring_view> tokens;

			static thread_local inline auto itemFields = std::array<std::wstring, 8>{};
		};

		template<typename T>
		class Filter : public FilterBase {
		public:

			Filter(void* itemProjection)
				
				: itemProjection(reinterpret_cast<ItemProjection<T>>(itemProjection))
			{}

			auto Match(T const& item) const -> bool {

				if (tokens.empty())
					return true;

				auto fieldCount = itemProjection(item, itemFields);
				return MatchFields(itemFields.data(), fieldCount);
			}

		private:

			ItemProjection<T> itemProjection;
		};

		template<typename T, std::invocable<T const&> Projection>
		friend auto MakeFilterableCollectionView(winrt::Citrine::IObservableCollectionView const& source) -> winrt::Citrine::IFilterableCollectionView;
	};

	template<typename T, typename SourceContainer>
	class FilterableCollectionView : public winrt::implements<FilterableCollectionView<T, SourceContainer>,
		winrt::Citrine::IFilterableCollectionView,
		winrt::Windows::Foundation::Collections::IVectorView<winrt::Windows::Foundation::IInspectable>,
		winrt::Windows::Foundation::Collections::IIterable<winrt::Windows::Foundation::IInspectable>,
		winrt::Microsoft::UI::Xaml::Interop::IBindableVector,
		winrt::Microsoft::UI::Xaml::Interop::IBindableVectorView,
		winrt::Microsoft::UI::Xaml::Interop::IBindableIterable,
		winrt::Microsoft::UI::Xaml::Interop::INotifyCollectionChanged>,
		FilterableCollectionViewBase,
		public NotifyCollectionChangedBase
	{
	public:

		using ValueType = T;
		using SizeType = std::uint32_t;

		FilterableCollectionView(winrt::Citrine::IObservableCollectionView const& source, SourceContainer const& sourceElements, void* itemProjection)

			: source(source)
			, sourceChangedRevoker(source.CollectionChanged(winrt::auto_revoke, { this, &FilterableCollectionView::OnSourceChanged }))
			, sourceElements(sourceElements)
			, filter(itemProjection)
			, matches(std::from_range, std::views::iota(0uz, sourceElements.size()))
		{}

		auto IsEmpty() const noexcept -> bool {

			return matches.empty();
		}

		auto Size() const noexcept -> SizeType {

			return static_cast<SizeType>(matches.size());
		}

		auto GetAt(SizeType index) const -> ValueType {

			if (index >= matches.size())
				throw winrt::hresult_out_of_bounds{};

			return sourceElements[matches[index]];
		}

		auto GetMany(SizeType startIndex, winrt::array_view<winrt::Windows::Foundation::IInspectable> values) const -> SizeType {

			if (startIndex >= matches.size())
				return 0;

			auto first = matches.begin() + startIndex;
			auto actualCount = std::min(static_cast<SizeType>(matches.size() - startIndex), values.size());
			std::ranges::transform(first, first + actualCount, values.begin(), [this](SizeType index) -> auto& { return sourceElements[index]; });
			return actualCount;
		}

		auto GetView() -> auto {

			struct Result {

				operator winrt::Windows::Foundation::Collections::IVectorView<winrt::Windows::Foundation::IInspectable>() const {

					return *Collection;
				}

				operator winrt::Microsoft::UI::Xaml::Interop::IBindableVectorView() const {

					return *Collection;
				}

				FilterableCollectionView const* Collection;
			};
			return Result{ this };
		}

		auto IndexOf(winrt::Windows::Foundation::IInspectable const& value, SizeType& index) const noexcept -> bool {

			index = static_cast<SizeType>(std::ranges::find(matches, value.try_as<ValueType>(), [this](SizeType index) -> auto& { return sourceElements[index]; }) - matches.begin());
			return index < matches.size();
		}

		auto Append(winrt::Windows::Foundation::IInspectable const&) -> void {

			throw winrt::hresult_not_implemented{};
		}

		auto InsertAt(SizeType, winrt::Windows::Foundation::IInspectable const&) -> void {

			throw winrt::hresult_not_implemented{};
		}

		auto SetAt(SizeType, winrt::Windows::Foundation::IInspectable const&) -> void {

			throw winrt::hresult_not_implemented{};
		}

		auto ReplaceAll(winrt::array_view<winrt::Windows::Foundation::IInspectable const>) -> void {

			throw winrt::hresult_not_implemented{};
		}

		auto RemoveAt(SizeType) -> void {

			throw winrt::hresult_not_implemented{};
		}

		auto RemoveAtEnd() -> void {

			throw winrt::hresult_not_implemented{};
		}

		auto Clear() -> void {

			throw winrt::hresult_not_implemented{};
		}

		auto First() const -> auto {

			struct Result {

				operator winrt::Windows::Foundation::Collections::IIterator<winrt::Windows::Foundation::IInspectable>() const {

					return winrt::make<Iterator>(Collection);
				}

				operator winrt::Microsoft::UI::Xaml::Interop::IBindableIterator() const {

					return winrt::make<BindableIterator>(Collection);
				}

				FilterableCollectionView const* Collection;
			};
			return Result{ this };
		}

		auto Source() const noexcept -> winrt::Citrine::IObservableCollectionView {

			return source;
		}

		auto Filter() const -> winrt::hstring {

			return filter.Get();
		}

		auto Filter(winrt::hstring const& value) -> void {

			auto invokeResetEvent = false;
			auto change = filter.Update(value);

			using enum FilterChange;

			switch (change) {
			case None: return;
			case Extended: {

				auto it = matches.begin();
				auto const end = matches.end();

				auto prevMatchCount = matches.size();
				auto dest = matches.begin();

				while (it != end) {

					auto index = *it;
					if (filter.Match(sourceElements[index]))
						*dest++ = index;
					++it;
				}

				matches.erase(dest, matches.end());
				invokeResetEvent = (matches.size() != prevMatchCount);
			} break;
			default: {

				if (filter.IsEmpty()) {

					if (change != Shrunken || matches.size() != sourceElements.size()) {

						matches.assign_range(std::views::iota(0uz, sourceElements.size()));
						invokeResetEvent = true;
					}
				}
				else {

					auto index = 0uz;
					auto const endPos = sourceElements.size();

					auto prevMatchCount = matches.size();
					matches.resize(sourceElements.size());
					auto dest = matches.begin();

					while (index < endPos) {

						if (filter.Match(sourceElements[index]))
							*dest++ = index;
						++index;
					}

					matches.erase(dest, matches.end());
					invokeResetEvent = (change != Shrunken || matches.size() != prevMatchCount);
				}
			} break;
			}

			if (invokeResetEvent)
				OnCollectionChanged(NotifyCollectionChangedAction::Reset, nullptr, nullptr, -1, -1);
		}

		auto Refresh() -> void {

			auto index = 0uz;
			auto const endPos = sourceElements.size();

			matches.resize(sourceElements.size());
			auto dest = matches.begin();

			while (index < endPos) {

				if (filter.Match(sourceElements[index]))
					*dest++ = index;
				++index;
			}

			matches.erase(dest, matches.end());
			OnCollectionChanged(NotifyCollectionChangedAction::Reset, nullptr, nullptr, -1, -1);
		}

		auto ItemType() const -> winrt::Windows::UI::Xaml::Interop::TypeName {

			return winrt::xaml_typename<ValueType>();
		}

	private:

		auto OnSourceChanged(winrt::Windows::Foundation::IInspectable const&, winrt::Microsoft::UI::Xaml::Interop::NotifyCollectionChangedEventArgs const& args) -> void {

			using enum winrt::Microsoft::UI::Xaml::Interop::NotifyCollectionChangedAction;

			switch (args.Action()) {
			case Add: {

				auto newIndex = args.NewStartingIndex();

				if (filter.Match(sourceElements[newIndex])) {

					auto newLocalPos = std::ranges::lower_bound(matches, newIndex) - matches.begin();
					matches.insert(matches.begin() + newLocalPos, newIndex);
					std::ranges::for_each(matches.begin() + newLocalPos + 1, matches.end(), [](SizeType& index) static { ++index; });
					OnCollectionChanged(Add, args.NewItems(), nullptr, static_cast<std::int32_t>(newLocalPos), -1);
				}
			} break;
			case Remove: {

				auto oldIndex = args.OldStartingIndex();

				auto it = std::ranges::lower_bound(matches, oldIndex);
				if (it != matches.end() && *it == oldIndex) {

					auto oldLocalPos = it - matches.begin();
					matches.erase(matches.begin() + oldLocalPos);
					std::ranges::for_each(matches.begin() + oldLocalPos, matches.end(), [](SizeType& index) static { --index; });
					OnCollectionChanged(Remove, nullptr, args.OldItems(), -1, static_cast<std::int32_t>(oldLocalPos));
				}
			} break;
			case Replace: {

				auto index = args.NewStartingIndex();

				auto it = std::ranges::lower_bound(matches, index);
				if (filter.Match(sourceElements[index])) {

					if (it != matches.end() && *it == index) {

						auto localPos = it - matches.begin();
						OnCollectionChanged(Replace, args.NewItems(), args.OldItems(), static_cast<std::int32_t>(localPos), static_cast<std::int32_t>(localPos));
					}
					else {

						auto newLocalPos = it - matches.begin();
						matches.insert(matches.begin() + newLocalPos, index);
						std::ranges::for_each(matches.begin() + newLocalPos + 1, matches.end(), [](SizeType& index) static { ++index; });
						OnCollectionChanged(Add, args.NewItems(), nullptr, static_cast<std::int32_t>(newLocalPos), -1);
					}
				}
				else if (it != matches.end() && *it == index) {

					auto oldLocalPos = it - matches.begin();
					matches.erase(matches.begin() + oldLocalPos);
					std::ranges::for_each(matches.begin() + oldLocalPos, matches.end(), [](SizeType& index) static { --index; });
					OnCollectionChanged(Remove, nullptr, args.OldItems(), -1, static_cast<std::int32_t>(oldLocalPos));
				}
			} break;
			case Move: {

				auto oldIndex = args.OldStartingIndex();
				auto newIndex = args.NewStartingIndex();

				auto it = std::ranges::lower_bound(matches, oldIndex);
				if (it != matches.end() && *it == oldIndex) {

					auto oldLocalPos = it - matches.begin();
					auto newLocalPos = std::ranges::lower_bound(matches, newIndex) - matches.begin();

					if (newLocalPos == oldLocalPos)
						break;

					if (newLocalPos > oldLocalPos) {

						std::ranges::copy(
							matches.begin() + oldLocalPos + 1,
							matches.begin() + newLocalPos + 1,
							matches.begin() + oldLocalPos
						);
						std::ranges::for_each(
							matches.begin() + oldLocalPos,
							matches.begin() + newLocalPos,
							[](SizeType& index) static { --index; }
						);
					}
					else {

						std::ranges::copy_backward(
							matches.begin() + newLocalPos,
							matches.begin() + oldLocalPos,
							matches.begin() + oldLocalPos + 1
						);
						std::ranges::for_each(
							matches.begin() + newLocalPos + 1,
							matches.begin() + oldLocalPos + 1,
							[](SizeType& index) static { ++index; }
						);
					}
					matches[newLocalPos] = newIndex;

					OnCollectionChanged(Move, args.NewItems(), args.OldItems(), static_cast<std::int32_t>(newLocalPos), static_cast<std::int32_t>(oldLocalPos));
				}
			} break;
			case Reset: {

				if (filter.IsEmpty()) {

					matches.assign_range(std::views::iota(0uz, sourceElements.size()));
				}
				else {

					auto index = 0uz;
					auto const endPos = sourceElements.size();

					matches.resize(sourceElements.size());
					auto dest = matches.begin();

					while (index < endPos) {

						if (filter.Match(sourceElements[index]))
							*dest++ = index;
						++index;
					}

					matches.erase(dest, matches.end());
				}

				OnCollectionChanged(args);
			} break;
			}
		}

		template<typename I>
		struct BasicIterator : winrt::implements<BasicIterator<I>, I> {

			explicit BasicIterator(FilterableCollectionView const* owner) noexcept

				: owner(const_cast<FilterableCollectionView*>(owner)->get_strong())
				, current(owner->matches.begin())
				, end(owner->matches.end())
			{}

			auto HasCurrent() const noexcept -> bool {

				return current != end;
			}

			auto Current() const -> ValueType {

				if (current == end)
					throw winrt::hresult_out_of_bounds{};

				return owner->sourceElements[*current];
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
				std::transform(current, current + actualCount, values.begin(), [this](SizeType index) -> auto& { return owner->sourceElements[index]; });
				std::advance(current, actualCount);
				return actualCount;
			}

		private:

			winrt::com_ptr<FilterableCollectionView> owner;
			std::vector<SizeType>::const_iterator current;
			std::vector<SizeType>::const_iterator const end;
		};

		using Iterator = BasicIterator<winrt::Windows::Foundation::Collections::IIterator<winrt::Windows::Foundation::IInspectable>>;
		using BindableIterator = BasicIterator<winrt::Microsoft::UI::Xaml::Interop::IBindableIterator>;

		winrt::Citrine::IObservableCollectionView source{ nullptr };
		winrt::Citrine::IObservableCollectionView::CollectionChanged_revoker sourceChangedRevoker;
		SourceContainer const& sourceElements;
		FilterableCollectionViewBase::Filter<ValueType> filter;
		std::vector<SizeType> matches;
	};

	template<typename T, std::invocable<T const&> Projection>
	auto MakeFilterableCollectionView(winrt::Citrine::IObservableCollectionView const& source) -> winrt::Citrine::IFilterableCollectionView {

		if (winrt::xaml_typename<T>() != source.ItemType())
			throw winrt::hresult_invalid_argument{};

		auto itemProjection = +[](T const& item, std::array<std::wstring, 8>& outFields) static -> std::size_t {

			auto properties = Projection{}(item);
			constexpr auto propertyCount = [](auto const& properties) static {

				if constexpr (requires{ properties.size(); })
					return properties.size();
				else
					return std::tuple_size_v<decltype(properties)>;

			}(properties);

			static_assert(propertyCount >= 1 && propertyCount <= outFields.size());
			[&]<std::size_t... I>(std::index_sequence<I...>) {

				((FilterableCollectionViewBase::NormalizeString([&] -> decltype(auto) {

					if constexpr (requires{ properties.get<I>(); })
						return properties.get<I>();
					else
						return std::get<I>(properties);
				}(), outFields[I])), ...);

			}(std::make_index_sequence<propertyCount>());

			return propertyCount;
		};

		static_assert(std::same_as<decltype(itemProjection), FilterableCollectionViewBase::ItemProjection<T>>);
		return source.as<IFilterableCollectionViewSource>()->CreateFilterableView(reinterpret_cast<void*>(itemProjection));
	}

	template<typename T, std::invocable<T const&> Projection>
	auto MakeFilterableCollectionView(winrt::Citrine::IObservableCollection const& source) -> winrt::Citrine::IFilterableCollectionView {

		return MakeFilterableCollectionView<T, Projection>(source.GetObservableView());
	}
}
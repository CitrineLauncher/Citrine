#pragma once

#include <type_traits>
#include <utility>
#include <iterator>
#include <memory>
#include <algorithm>
#include <stdexcept>

namespace Citrine {

	class FrozenContainerBase {
	protected:

		template<typename T>
			requires(!std::is_const_v<T> && !std::is_reference_v<T>) // Disallow const complete objects
		union ValueHolder {

#pragma warning(push)
#pragma warning(disable : 26495) // Always initialize a member variable

			constexpr ValueHolder() noexcept {}

#pragma warning(pop)

			constexpr ~ValueHolder() noexcept requires (std::is_trivially_destructible_v<T>) = default;

			constexpr ~ValueHolder() noexcept {};

			T Value;
		};

		template<typename T>
		struct StorageIterator {

			using iterator_category = std::random_access_iterator_tag;
			using value_type = T;
			using difference_type = std::ptrdiff_t;
			using pointer = value_type*;
			using reference = value_type&;

			constexpr StorageIterator() noexcept = default;
			constexpr StorageIterator(ValueHolder<T>* ptr) noexcept : Ptr(ptr) {}

			constexpr StorageIterator(StorageIterator const&) noexcept = default;
			constexpr auto operator=(StorageIterator const&) noexcept -> StorageIterator& = default;

			constexpr auto operator*() const noexcept -> reference {

				return Ptr->Value;
			}

			constexpr auto operator->() const noexcept -> pointer {

				return &Ptr->Value;
			}

			constexpr auto operator++() noexcept -> StorageIterator& {

				++Ptr;
				return *this;
			}

			constexpr auto operator++(int) noexcept -> StorageIterator {

				auto old = *this;
				++Ptr;
				return old;
			}

			constexpr auto operator--() noexcept -> StorageIterator& {

				--Ptr;
				return *this;
			}

			constexpr auto operator--(int) noexcept -> StorageIterator  {

				auto old = *this;
				--Ptr;
				return old;
			}

			constexpr auto operator+=(difference_type offset) noexcept -> StorageIterator& {

				Ptr += offset;
				return *this;
			}

			constexpr auto operator-=(difference_type offset) noexcept -> StorageIterator& {

				Ptr -= offset;
				return *this;
			}

			friend constexpr auto operator+(StorageIterator it, difference_type offset) noexcept -> StorageIterator {

				return it.Ptr + offset;
			}

			friend constexpr auto operator+(difference_type offset, StorageIterator it) noexcept -> StorageIterator {

				return offset + it.Ptr;
			}

			friend constexpr auto operator-(StorageIterator it, difference_type offset) noexcept -> StorageIterator {

				return it.Ptr - offset;
			}

			friend constexpr auto operator-(StorageIterator leftIt, StorageIterator rightIt) noexcept -> difference_type {

				return leftIt.Ptr - rightIt.Ptr;
			}

			constexpr auto operator[](difference_type offset) const noexcept -> reference {

				return Ptr[offset].Value;
			}

			constexpr auto operator<=>(StorageIterator const&) const noexcept -> std::strong_ordering = default;

			ValueHolder<T>* Ptr{};
		};

		template<typename T>
		struct StorageConstIterator {

			using iterator_category = std::random_access_iterator_tag;
			using value_type = T;
			using difference_type = std::ptrdiff_t;
			using pointer = value_type const*;
			using reference = value_type const&;

			constexpr StorageConstIterator() noexcept = default;
			constexpr StorageConstIterator(ValueHolder<T> const* ptr) noexcept : Ptr(ptr) {}
			constexpr StorageConstIterator(StorageIterator<T> it) noexcept : Ptr(it.Ptr) {}

			constexpr StorageConstIterator(StorageConstIterator const&) noexcept = default;
			constexpr auto operator=(StorageConstIterator const&) noexcept -> StorageConstIterator& = default;

			constexpr auto operator*() const noexcept -> reference {

				return Ptr->Value;
			}

			constexpr auto operator->() const noexcept -> pointer {

				return &Ptr->Value;
			}

			constexpr auto operator++() noexcept -> StorageConstIterator& {

				++Ptr;
				return *this;
			}

			constexpr auto operator++(int) noexcept -> StorageConstIterator {

				auto old = *this;
				++Ptr;
				return old;
			}

			constexpr auto operator--() noexcept -> StorageConstIterator& {

				--Ptr;
				return *this;
			}

			constexpr auto operator--(int) noexcept -> StorageConstIterator {

				auto old = *this;
				--Ptr;
				return old;
			}

			constexpr auto operator+=(difference_type offset) noexcept -> StorageConstIterator& {

				Ptr += offset;
				return *this;
			}

			constexpr auto operator-=(difference_type offset) noexcept -> StorageConstIterator& {

				Ptr -= offset;
				return *this;
			}

			friend constexpr auto operator+(StorageConstIterator it, difference_type offset) noexcept -> StorageConstIterator {

				return it.Ptr + offset;
			}

			friend constexpr auto operator+(difference_type offset, StorageConstIterator it) noexcept -> StorageConstIterator {

				return offset + it.Ptr;
			}

			friend constexpr auto operator-(StorageConstIterator it, difference_type offset) noexcept -> StorageConstIterator {

				return it.Ptr - offset;
			}

			friend constexpr auto operator-(StorageConstIterator leftIt, StorageConstIterator rightIt) noexcept -> difference_type {

				return leftIt.Ptr - rightIt.Ptr;
			}

			constexpr auto operator[](difference_type offset) const noexcept -> reference {

				return Ptr[offset].Value;
			}

			constexpr auto operator<=>(StorageConstIterator const&) const noexcept -> std::strong_ordering = default;

			ValueHolder<T> const* Ptr{};
		};

		template<typename T, std::size_t N>
		struct Storage {
		public:

			using value_type = T;
			using size_type = std::size_t;
			using difference_type = std::ptrdiff_t;
			using reference = value_type&;
			using const_reference = value_type const&;
			using pointer = value_type*;
			using const_pointer = value_type const*;
			using iterator = StorageIterator<value_type>;
			using const_iterator = StorageConstIterator<value_type>;
			using reverse_iterator = std::reverse_iterator<iterator>;
			using const_reverse_iterator = std::reverse_iterator<const_iterator>;

			constexpr Storage() noexcept = default;

			constexpr Storage(Storage const& other)
				noexcept(std::is_nothrow_copy_constructible_v<value_type>)
				requires(std::is_copy_constructible_v<value_type>)
			{
				for (auto const& element : other)
					std::construct_at(&elements[size++].Value, element);
			}

			constexpr auto operator=(Storage const&) = delete;

			constexpr auto begin() noexcept -> iterator {

				return elements;
			}

			constexpr auto begin() const noexcept -> const_iterator {

				return elements;
			}

			constexpr auto end() noexcept -> iterator {

				return elements + size;
			}

			constexpr auto end() const noexcept -> const_iterator {

				return elements + size;
			}

			constexpr auto rbegin() noexcept -> reverse_iterator {

				return reverse_iterator{ end() };
			}

			constexpr auto rbegin() const noexcept -> const_reverse_iterator {

				return const_reverse_iterator{ end() };
			}

			constexpr auto rend() noexcept -> reverse_iterator {

				return reverse_iterator{ begin() };
			}

			constexpr auto rend() const noexcept -> const_reverse_iterator {

				return const_reverse_iterator{ begin() };
			}

			constexpr auto IsEmpty() const noexcept -> bool {

				return size == 0;
			}

			constexpr auto Size() const noexcept -> std::size_t {

				return size;
			}

			constexpr auto Capacity() const noexcept -> std::size_t {

				return N;
			}

			//Does not provide strong exception guarantees
			template<typename... Args>
			constexpr auto Emplace(const_iterator position, Args&&... args)
				noexcept(std::is_nothrow_constructible_v<value_type, Args...>) -> iterator
			{
				auto const pos = const_cast<ElementType*>(position.Ptr);
				auto const endPos = end().Ptr;

				if (pos < endPos) {

					for (auto backPos = endPos; backPos > pos; --backPos) {

						std::construct_at(&backPos->Value, std::move(backPos[-1].Value));
						std::destroy_at(&backPos[-1].Value);
					}
				}

				std::construct_at(&pos->Value, std::forward<Args>(args)...);
				++size;

				return pos;
			}

			constexpr ~Storage() noexcept requires (std::is_trivially_destructible_v<T>) = default;

			constexpr ~Storage() noexcept {

				for (auto& element : *this)
					std::destroy_at(&element);
			}

		private:

			using ElementType = ValueHolder<value_type>;

			std::size_t size{};
			ElementType elements[N > 0 ? N : 1]{};
		};

		template<typename Compare>
		static constexpr auto IsTransparent = requires{

			typename Compare::is_transparent;
		};

		static consteval auto GetArraySize(auto const& arr) noexcept -> std::size_t {

			return std::size(arr);
		}
	};

	class FrozenMapBase : protected FrozenContainerBase {
	protected:

		struct GetKey {

			template<typename T1, typename T2>
			static constexpr auto operator()(std::pair<T1, T2> const& value) noexcept -> T1 const& {

				return value.first;
			}
		};

		template<typename It, typename K, typename Compare>
		static constexpr auto EqualRange(It first, It last, K const& key, Compare comp) -> std::pair<It, It> {

			return std::ranges::equal_range(first, last, key, comp, GetKey{});
		}

		template<typename Self, typename K>
		constexpr auto EqualRange(this Self& self, K const& key) -> auto {

			return EqualRange(self.begin(), self.end(), key, typename Self::key_compare{});
		}

		template<typename It, typename K, typename Compare>
		static constexpr auto LowerBound(It first, It last, K const& key, Compare comp) -> It {

			return std::ranges::lower_bound(first, last, key, comp, GetKey{});
		}

		template<typename Self, typename K>
		constexpr auto LowerBound(this Self& self, K const& key) -> auto {

			return LowerBound(self.begin(), self.end(), key, typename Self::key_compare{});
		}

		template<typename It, typename K, typename Compare>
		static constexpr auto LowerBoundEqual(It first, It last, K const& key, Compare comp) -> std::pair<It, bool> {

			auto it = LowerBound(first, last, key, comp);
			return { it, it != last && !comp(key, it->first) };
		}

		template<typename Self, typename K>
		constexpr auto LowerBoundEqual(this Self& self, K const& key) -> auto {

			return LowerBoundEqual(self.begin(), self.end(), key, typename Self::key_compare{});
		}

		template<typename It, typename K, typename Compare>
		static constexpr auto UpperBound(It first, It last, K const& key, Compare comp) -> It {

			return std::ranges::upper_bound(first, last, key, comp, GetKey{});
		}

		template<typename Self, typename K>
		constexpr auto UpperBound(this Self& self, K const& key) -> auto {

			return UpperBound(self.begin(), self.end(), key, typename Self::key_compare{});
		}

		template<typename ValueType, typename Compare>
		struct ValueComparer {

			constexpr auto operator()(ValueType const& left, ValueType const& right) const -> bool {

				return comp(left.first, right.first);
			}

			Compare comp{};
		};
	};

	template<typename Key, typename T, std::size_t N, typename Compare = std::less<Key>>
	class FrozenMap : protected FrozenMapBase {
	public:

		using key_type = Key;
		using mapped_type = T;
		using value_type = std::pair<Key const, T>;
		using size_type = std::size_t;
		using difference_type = std::ptrdiff_t;
		using key_compare = Compare;
		using value_compare = ValueComparer<value_type, key_compare>;
		using reference = value_type&;
		using const_reference = value_type const&;
		using pointer = value_type*;
		using const_pointer = value_type const*;
		using iterator = StorageIterator<value_type>;
		using const_iterator = StorageConstIterator<value_type>;
		using reverse_iterator = std::reverse_iterator<iterator>;
		using const_reverse_iterator = std::reverse_iterator<const_iterator>;

		constexpr FrozenMap() requires (N == 0) = default;

		template<typename Array>
		constexpr FrozenMap(Array&& arr) requires (GetArraySize(arr) == N) {

			for (auto& [key, value] : arr) {

				auto [it, equal] = LowerBoundEqual(key);
				if (equal) continue;
				storage.Emplace(it, std::forward_like<Array>(key), std::forward_like<Array>(value));
			}
		}

		constexpr FrozenMap(FrozenMap const&) = default;

		constexpr auto at(key_type const& key) -> mapped_type& {

			auto [it, equal] = LowerBoundEqual(key);
			return equal ? it->second : throw std::out_of_range{ "Key not found" };
		}

		constexpr auto at(key_type const& key) const -> mapped_type const& {

			auto [it, equal] = LowerBoundEqual(key);
			return equal ? it->second : throw std::out_of_range{ "Key not found" };
		}

		template<typename K> requires IsTransparent<key_compare>
		constexpr auto at(K const& key) -> mapped_type& {

			auto [it, equal] = LowerBoundEqual(key);
			return equal ? it->second : throw std::out_of_range{ "Key not found" };
		}

		template<typename K> requires IsTransparent<key_compare>
		constexpr auto at(K const& key) const -> mapped_type const& {

			auto [it, equal] = LowerBoundEqual(key);
			return equal ? it->second : throw std::out_of_range{ "Key not found" };
		}

		constexpr auto begin() noexcept -> iterator {

			return storage.begin();
		}

		constexpr auto begin() const noexcept -> const_iterator {

			return storage.begin();
		}

		constexpr auto end() noexcept -> iterator {

			return storage.end();
		}

		constexpr auto end() const noexcept -> const_iterator {

			return storage.end();
		}

		constexpr auto rbegin() noexcept -> reverse_iterator {

			return storage.rbegin();
		}

		constexpr auto rbegin() const noexcept -> const_reverse_iterator {

			return storage.rbegin();
		}

		constexpr auto rend() noexcept -> reverse_iterator {

			return storage.rend();
		}

		constexpr auto rend() const noexcept -> const_reverse_iterator {

			return storage.rend();
		}

		constexpr auto empty() const noexcept -> bool {

			return storage.IsEmpty();
		}

		constexpr auto size() const noexcept -> size_type {

			return storage.Size();
		}

		constexpr auto count(key_type const& key) const -> size_type {

			auto [it, equal] = LowerBoundEqual(key);
			return { equal };
		}

		template<typename K> requires IsTransparent<key_compare>
		constexpr auto count(K const& key) const -> size_type {

			auto [it, equal] = LowerBoundEqual(key);
			return { equal };
		}

		constexpr auto find(key_type const& key) -> iterator {

			auto [it, equal] = LowerBoundEqual(key);
			return equal ? it : end();
		}

		constexpr auto find(key_type const& key) const -> const_iterator {

			auto [it, equal] = LowerBoundEqual(key);
			return equal ? it : end();
		}

		template<typename K> requires IsTransparent<key_compare>
		constexpr auto find(K const& key) -> iterator {

			auto [it, equal] = LowerBoundEqual(key);
			return equal ? it : end();
		}

		template<typename K> requires IsTransparent<key_compare>
		constexpr auto find(K const& key) const -> const_iterator {

			auto [it, equal] = LowerBoundEqual(key);
			return equal ? it : end();
		}

		constexpr auto contains(key_type const& key) const -> bool {

			auto [it, equal] = LowerBoundEqual(key);
			return equal;
		}

		template<typename K> requires IsTransparent<key_compare>
		constexpr auto contains(K const& key) const -> bool {

			auto [it, equal] = LowerBoundEqual(key);
			return equal;
		}

		constexpr auto equal_range(key_type const& key) -> std::pair<iterator, iterator> {

			return EqualRange(key);
		}

		constexpr auto equal_range(key_type const& key) const -> std::pair<const_iterator, const_iterator> {

			return EqualRange(key);
		}

		template<typename K> requires IsTransparent<key_compare>
		constexpr auto equal_range(K const& key) -> std::pair<iterator, iterator> {

			return EqualRange(key);
		}

		template<typename K> requires IsTransparent<key_compare>
		constexpr auto equal_range(K const& key) const -> std::pair<const_iterator, const_iterator> {

			return EqualRange(key);
		}

		constexpr auto lower_bound(key_type const& key) -> iterator {

			return LowerBound(key);
		}

		constexpr auto lower_bound(key_type const& key) const -> const_iterator {

			return LowerBound(key);
		}

		template<typename K> requires IsTransparent<key_compare>
		constexpr auto lower_bound(K const& key) -> iterator {

			return LowerBound(key);
		}

		template<typename K> requires IsTransparent<key_compare>
		constexpr auto lower_bound(K const& key) const -> const_iterator {

			return LowerBound(key);
		}

		constexpr auto upper_bound(key_type const& key) -> iterator {

			return UpperBound(key);
		}

		constexpr auto upper_bound(key_type const& key) const -> const_iterator {

			return UpperBound(key);
		}

		template<typename K> requires IsTransparent<key_compare>
		constexpr auto upper_bound(K const& key) -> iterator {

			return UpperBound(key);
		}

		template<typename K> requires IsTransparent<key_compare>
		constexpr auto upper_bound(K const& key) const -> const_iterator {

			return UpperBound(key);
		}

		constexpr auto key_comp() const -> key_compare {

			return {};
		}

		constexpr auto value_comp() const -> value_compare {

			return {};
		}

	private:

		Storage<value_type, N> storage;
	};

	template<typename Key, typename T, typename Compare = std::less<Key>, std::size_t N>
	constexpr auto MakeFrozenMap(std::pair<Key, T> (&&arr)[N]) -> auto {

		return FrozenMap<Key, T, N, Compare>{ std::move(arr) };
	}

	template<typename Key, typename T, typename Compare = std::less<Key>, typename Array>
	constexpr auto MakeFrozenMap(Array&& arr) -> auto {

		return FrozenMap<Key, T, std::size(arr), Compare>{ std::forward<Array>(arr) };
	}

	template<typename Key, typename T, typename Compare = std::less<Key>>
	constexpr auto MakeFrozenMap(std::monostate) -> auto {

		return FrozenMap<Key, T, 0, Compare>{};
	}

	class FrozenSetBase : protected FrozenContainerBase {
	protected:

		template<typename It, typename K, typename Compare>
		static constexpr auto EqualRange(It first, It last, K const& key, Compare comp) -> std::pair<It, It> {

			return std::ranges::equal_range(first, last, key, comp);
		}

		template<typename Self, typename K>
		constexpr auto EqualRange(this Self const& self, K const& key) -> auto {

			return EqualRange(self.begin(), self.end(), key, typename Self::key_compare{});
		}

		template<typename It, typename K, typename Compare>
		static constexpr auto LowerBound(It first, It last, K const& key, Compare comp) -> It {

			return std::ranges::lower_bound(first, last, key, comp);
		}

		template<typename Self, typename K>
		constexpr auto LowerBound(this Self const& self, K const& key) -> auto {

			return LowerBound(self.begin(), self.end(), key, typename Self::key_compare{});
		}

		template<typename It, typename K, typename Compare>
		static constexpr auto LowerBoundEqual(It first, It last, K const& key, Compare comp) -> std::pair<It, bool> {

			auto it = LowerBound(first, last, key, comp);
			return { it, it != last && !comp(key, *it) };
		}

		template<typename Self, typename K>
		constexpr auto LowerBoundEqual(this Self const& self, K const& key) -> auto {

			return LowerBoundEqual(self.begin(), self.end(), key, typename Self::key_compare{});
		}

		template<typename It, typename K, typename Compare>
		static constexpr auto UpperBound(It first, It last, K const& key, Compare comp) -> It {

			return std::ranges::upper_bound(first, last, key, comp);
		}

		template<typename Self, typename K>
		constexpr auto UpperBound(this Self const& self, K const& key) -> auto {

			return UpperBound(self.begin(), self.end(), key, typename Self::key_compare{});
		}
	};

	template<typename Key, std::size_t N, typename Compare = std::less<Key>>
	class FrozenSet : protected FrozenSetBase {
	public:

		using key_type = Key;
		using value_type = Key;
		using size_type = std::size_t;
		using difference_type = std::ptrdiff_t;
		using key_compare = Compare;
		using value_compare = Compare;
		using reference = value_type&;
		using const_reference = value_type const&;
		using pointer = value_type*;
		using const_pointer = value_type const*;
		using iterator = StorageConstIterator<value_type>;
		using const_iterator = StorageConstIterator<value_type>;
		using reverse_iterator = std::reverse_iterator<iterator>;
		using const_reverse_iterator = std::reverse_iterator<const_iterator>;

		constexpr FrozenSet() requires (N == 0) = default;

		template<typename Array>
		constexpr FrozenSet(Array&& arr) requires (GetArraySize(arr) == N) {

			for (auto& key : arr) {

				auto [it, equal] = LowerBoundEqual(key);
				if (equal) continue;
				storage.Emplace(it, std::forward_like<Array>(key));
			}
		}

		constexpr FrozenSet(FrozenSet const&) = default;

		constexpr auto begin() const noexcept -> const_iterator {

			return storage.begin();
		}

		constexpr auto end() const noexcept -> const_iterator {

			return storage.end();
		}

		constexpr auto rbegin() const noexcept -> const_reverse_iterator {

			return storage.rbegin();
		}

		constexpr auto rend() const noexcept -> const_reverse_iterator {

			return storage.rend();
		}

		constexpr auto empty() const noexcept -> bool {

			return storage.IsEmpty();
		}

		constexpr auto size() const noexcept -> size_type {

			return storage.Size();
		}

		constexpr auto count(key_type const& key) const -> size_type {

			auto [it, equal] = LowerBoundEqual(key);
			return { equal };
		}

		template<typename K> requires IsTransparent<key_compare>
		constexpr auto count(K const& key) const -> size_type {

			auto [it, equal] = LowerBoundEqual(key);
			return { equal };
		}

		constexpr auto find(key_type const& key) const -> const_iterator {

			auto [it, equal] = LowerBoundEqual(key);
			return equal ? it : end();
		}

		template<typename K> requires IsTransparent<key_compare>
		constexpr auto find(K const& key) const -> const_iterator {

			auto [it, equal] = LowerBoundEqual(key);
			return equal ? it : end();
		}

		constexpr auto contains(key_type const& key) const -> bool {

			auto [it, equal] = LowerBoundEqual(key);
			return equal;
		}

		template<typename K> requires IsTransparent<key_compare>
		constexpr auto contains(K const& key) const -> bool {

			auto [it, equal] = LowerBoundEqual(key);
			return equal;
		}

		constexpr auto equal_range(key_type const& key) const -> std::pair<const_iterator, const_iterator> {

			return EqualRange(key);
		}

		template<typename K> requires IsTransparent<key_compare>
		constexpr auto equal_range(K const& key) const -> std::pair<const_iterator, const_iterator> {

			return EqualRange(key);
		}

		constexpr auto lower_bound(key_type const& key) const -> const_iterator {

			return LowerBound(key);
		}

		template<typename K> requires IsTransparent<key_compare>
		constexpr auto lower_bound(K const& key) const -> const_iterator {

			return LowerBound(key);
		}

		constexpr auto upper_bound(key_type const& key) const -> const_iterator {

			return UpperBound(key);
		}

		template<typename K> requires IsTransparent<key_compare>
		constexpr auto upper_bound(K const& key) const -> const_iterator {

			return UpperBound(key);
		}

		constexpr auto key_comp() const -> key_compare {

			return {};
		}

		constexpr auto value_comp() const -> value_compare {

			return {};
		}

	private:
		
		Storage<value_type, N> storage;
	};

	template<typename Key, typename Compare = std::less<Key>, std::size_t N>
	constexpr auto MakeFrozenSet(Key (&&arr)[N]) -> auto {

		return FrozenSet<Key, N, Compare>{ std::move(arr) };
	}

	template<typename Key, typename Compare = std::less<Key>, typename Array>
	constexpr auto MakeFrozenSet(Array&& arr) -> auto {

		return FrozenSet<Key, std::size(arr), Compare>{ std::forward<Array>(arr) };
	}

	template<typename Key, typename Compare = std::less<Key>>
	constexpr auto MakeFrozenSet(std::monostate) -> auto {

		return FrozenSet<Key, 0, Compare>{};
	}
}
#pragma once

#include <vector>
#include <algorithm>
#include <ranges>
#include <memory>
#include <concepts>

#include <glaze/json/read.hpp>
#include <glaze/json/write.hpp>

namespace Citrine::Minecraft::Bedrock {

	class PackageOperationCollectionBase {
	protected:

		template<typename Operation>
		class StorageNodeHandle {
		public:

			using value_type = Operation;

			constexpr StorageNodeHandle() noexcept = default;

			StorageNodeHandle(StorageNodeHandle const&) = delete;
			auto operator=(StorageNodeHandle const&) = delete;

			StorageNodeHandle(StorageNodeHandle&& other) noexcept

				: ptr(std::exchange(other.ptr, nullptr))
			{}

			auto operator=(StorageNodeHandle&& other) noexcept -> StorageNodeHandle& {

				StorageNodeHandle{ std::move(other) }.swap(*this);
				return *this;
			}

			auto empty() const noexcept -> bool {

				return ptr == nullptr;
			}

			operator bool() const noexcept {

				return ptr != nullptr;
			}

			auto value() const noexcept -> value_type& {

				return *ptr;
			}

			auto swap(StorageNodeHandle& other) noexcept -> void {

				std::swap(ptr, other.ptr);
			}

			constexpr ~StorageNodeHandle() noexcept {

				delete ptr;
			}

		private:

			StorageNodeHandle(value_type* ptr)

				: ptr(ptr)
			{}

			template<typename Operation>
			friend class Storage;

			value_type* ptr{ nullptr };
		};

		template<typename Operation>
		struct StorageElement {

			Operation* Ptr{ nullptr };
		};

		template<typename Operation>
		struct StorageConstIterator {

			using BaseIterator = std::vector<StorageElement<Operation>>::const_iterator;

			using iterator_category = std::random_access_iterator_tag;
			using value_type = Operation;
			using difference_type = std::ptrdiff_t;
			using pointer = value_type const*;
			using reference = value_type const&;

			constexpr StorageConstIterator() noexcept = default;

			StorageConstIterator(BaseIterator current) noexcept

				: current(current)
			{}

			StorageConstIterator(StorageConstIterator const&) noexcept = default;
			auto operator=(StorageConstIterator const&) noexcept -> StorageConstIterator & = default;

			auto operator*() const noexcept -> reference {

				return *current->Ptr;
			}

			auto operator->() const noexcept -> pointer {

				return current->Ptr;
			}

			auto operator++() noexcept -> StorageConstIterator& {

				++current;
				return *this;
			}

			auto operator++(int) noexcept -> StorageConstIterator {

				auto old = *this;
				++current;
				return old;
			}

			auto operator--() noexcept -> StorageConstIterator& {

				--current;
				return *this;
			}

			auto operator--(int) noexcept -> StorageConstIterator {

				auto old = *this;
				--current;
				return old;
			}

			auto operator+=(difference_type offset) noexcept -> StorageConstIterator& {

				current += offset;
				return *this;
			}

			auto operator-=(difference_type offset) noexcept -> StorageConstIterator& {

				current -= offset;
				return *this;
			}

			friend auto operator+(StorageConstIterator it, difference_type offset) noexcept -> StorageConstIterator {

				return it.current + offset;
			}

			friend auto operator+(difference_type offset, StorageConstIterator it) noexcept -> StorageConstIterator {

				return offset + it.current;
			}

			friend auto operator-(StorageConstIterator it, difference_type offset) noexcept -> StorageConstIterator {

				return it.current - offset;
			}

			friend auto operator-(StorageConstIterator leftIt, StorageConstIterator rightIt) noexcept -> difference_type {

				return leftIt.current - rightIt.current;
			}

			auto operator[](difference_type offset) const noexcept -> reference {

				return *current[offset].Ptr;
			}

			auto operator<=>(StorageConstIterator const&) const noexcept -> std::strong_ordering = default;

		private:

			auto Base() const noexcept -> BaseIterator {

				return current;
			}

			template<typename Operation>
			friend class Storage;

			BaseIterator current{};
		};

		template<typename Operation>
		class Storage {
		public:

			using ValueType = Operation;
			using SizeType = std::size_t;
			using Iterator = StorageConstIterator<Operation>;
			using ConstIterator = StorageConstIterator<Operation>;
			using ReverseIterator = std::reverse_iterator<Iterator>;
			using ConstReverseIterator = std::reverse_iterator<ConstIterator>;
			using NodeHandle = StorageNodeHandle<Operation>;

			constexpr Storage() noexcept = default;

			Storage(Storage const&) = delete;
			auto operator=(Storage const&) = delete;

			Storage(Storage&&) noexcept = default;
			auto operator=(Storage&&) noexcept -> Storage& = default;

			auto begin() const noexcept -> ConstIterator {

				return elements.begin();
			}

			auto end() const noexcept -> ConstIterator {

				return elements.end();
			}

			auto rbegin() const noexcept -> ConstReverseIterator {

				return ConstReverseIterator{ end() };
			}

			auto rend() const noexcept -> ConstReverseIterator {

				return ConstReverseIterator{ begin() };
			}

			auto IsEmpty() const noexcept -> bool {

				return elements.empty();
			}

			auto Size() const noexcept -> SizeType {

				return elements.size();
			}

			auto Reserve(SizeType newCapacity) -> void {

				elements.reserve(newCapacity);
			}

			auto Capacity() const noexcept -> SizeType {

				return elements.capacity();
			}

			auto Insert(ConstIterator pos, NodeHandle&& node) -> ConstIterator {

				auto it = elements.emplace(pos.Base(), node.ptr);
				node.ptr = nullptr;
				return it;
			}

			auto Insert(ConstIterator pos, std::unique_ptr<ValueType>&& ptr) -> ConstIterator {

				auto it = elements.emplace(pos.Base(), ptr.get());
				ptr.release();
				return it;
			}

			auto Extract(ConstIterator pos) noexcept -> NodeHandle {

				auto node = NodeHandle{ pos.Base()->Ptr };
				elements.erase(pos.Base());
				return node;
			}

			auto Remove(ConstIterator pos) noexcept -> ConstIterator {

				delete pos.Base()->Ptr;
				return elements.erase(pos.Base());
			}

			auto Remove(ConstIterator first, ConstIterator last) noexcept -> ConstIterator {

				for (auto it = first; it != last; ++it) {

					delete it.Base()->Ptr;
				}
				return elements.erase(first.Base(), last.Base());
			}

			auto Clear() noexcept -> void {

				Remove(begin(), end());
			}

			auto swap(Storage& other) noexcept -> void {

				elements.swap(other.elements);
			}

			~Storage() noexcept {

				for (auto const& element : elements) {

					delete element.Ptr;
				}
			}

		private:

			using ElementType = StorageElement<Operation>;

			std::vector<ElementType> elements;
		};

		template<typename C>
		struct Proxy {

			using value_type = C::ValueType;
			using size_type = C::SizeType;
			using iterator = C::Iterator;
			using const_iterator = C::ConstIterator;

			auto empty() const noexcept -> bool {

				return collection.IsEmpty();
			}

			auto size() const noexcept -> size_type {

				return collection.Size();
			}

			auto begin() const noexcept -> const_iterator {

				return collection.begin();
			}

			auto end() const noexcept -> const_iterator {

				return collection.end();
			}

			auto emplace(value_type&& value) -> void requires(!std::is_const_v<C>) {

				collection.Insert(std::move(value));
			}

			auto clear() -> void requires(!std::is_const_v<C>) {

				collection.Clear();
			}

			C& collection;
		};
	};

	template<typename Operation, typename OperationEqual>
	class PackageOperationCollection : PackageOperationCollectionBase {
	public:

		using KeyType = Operation;
		using ValueType = Operation;
		using SizeType = std::size_t;
		using KeyEqual = OperationEqual;
		using Iterator = StorageConstIterator<Operation>;
		using ConstIterator = StorageConstIterator<Operation>;
		using ReverseIterator = std::reverse_iterator<Iterator>;
		using ConstReverseIterator = std::reverse_iterator<ConstIterator>;
		using NodeHandle = StorageNodeHandle<Operation>;

		struct InsertResult {

			ConstIterator Position{};
			bool Inserted{};
		};

		constexpr PackageOperationCollection() noexcept = default;

		PackageOperationCollection(PackageOperationCollection const&) = delete;
		auto operator=(PackageOperationCollection const&) = delete;

		PackageOperationCollection(PackageOperationCollection&&) noexcept = default;
		auto operator=(PackageOperationCollection&&) noexcept -> PackageOperationCollection & = default;

		auto begin() const noexcept -> ConstIterator {

			return storage.begin();
		}

		auto end() const noexcept -> ConstIterator {

			return storage.end();
		}

		auto rbegin() const noexcept -> ConstReverseIterator {

			return storage.rbegin();
		}

		auto rend() const noexcept -> ConstReverseIterator {

			return storage.rend();
		}

		auto IsEmpty() const noexcept -> bool {

			return storage.IsEmpty();
		}

		auto Size() const noexcept -> SizeType {

			return storage.Size();
		}

		auto Reserve(SizeType newCapacity) -> void {

			storage.Reserve(newCapacity);
		}

		auto Capacity() const noexcept -> SizeType {

			return storage.Capacity();
		}

		auto Insert(NodeHandle&& node) -> InsertResult {

			if (!node)
				return { storage.end() };

			auto& keyValue = node.value();

			auto it = Find(keyValue);
			if (it != end())
				return { it };

			return { storage.Insert(it, std::move(node)), true };
		}

		auto Insert(ValueType const& value) -> InsertResult {

			return Emplace(value);
		}

		auto Insert(ValueType&& value) -> InsertResult {

			return Emplace(std::move(value));
		}

		template<typename... Args>
		auto Emplace(Args&&... args) -> InsertResult {

			auto keyValue = std::make_unique<ValueType>(std::forward<Args>(args)...);

			auto it = Find(*keyValue);
			if (it != end())
				return { it };

			return { storage.Insert(it, std::move(keyValue)), true };
		}

		template<typename K>
		auto Find(K const& key) const noexcept -> ConstIterator {

			constexpr auto keyEqual = KeyEqual{};

			for (auto it = storage.begin(); it != storage.end(); ++it) {

				if (keyEqual(*it, key))
					return { it };
			}
			return storage.end();
		}

		template<typename K>
		auto Contains(K const& key) const noexcept -> bool {

			return Find(key) != end();
		}

		auto Extract(ConstIterator it) noexcept -> NodeHandle {

			return storage.Extract(it);
		}

		template<typename K>
		auto Extract(K const& key) noexcept -> NodeHandle {

			auto it = Find(key);
			if (it != end())
				return storage.Extract(it);

			return {};
		}

		auto Remove(ConstIterator it) noexcept -> ConstIterator {

			return storage.Remove(it);
		}

		template<typename K>
		auto Remove(K const& key) noexcept -> SizeType {

			auto it = Find(key);
			if (it != end()) {

				storage.Remove(it);
				return 1;
			}
			return 0;
		}

		auto Clear() noexcept -> void {

			storage.Clear();
		}

		auto swap(PackageOperationCollection& other) noexcept -> void {

			storage.swap(other.storage);
		}

	private:

		friend struct ::glz::from<::glz::JSON, PackageOperationCollection>;
		friend struct ::glz::to<::glz::JSON, PackageOperationCollection>;

		auto GetProxy(this auto& self) noexcept -> auto {

			return Proxy{ self };
		}

		Storage<ValueType> storage;
	};
}

namespace glz {

	template<typename Operation, typename OperationEqual>
	struct from<JSON, ::Citrine::Minecraft::Bedrock::PackageOperationCollection<Operation, OperationEqual>>
	{
		template<auto Opts>
		static auto op(::Citrine::Minecraft::Bedrock::PackageOperationCollection<Operation, OperationEqual>& collection, auto&&... args) -> void {

			auto proxy = collection.GetProxy();
			parse<JSON>::op<Opts>(proxy, args...);
		}
	};

	template<typename Operation, typename OperationEqual>
	struct to<JSON, ::Citrine::Minecraft::Bedrock::PackageOperationCollection<Operation, OperationEqual>>
	{
		template<auto Opts>
		static auto op(::Citrine::Minecraft::Bedrock::PackageOperationCollection<Operation, OperationEqual> const& collection, auto&&... args) noexcept -> void {

			auto proxy = collection.GetProxy();
			if constexpr (requires(Operation op) { { op.IsPersistent() } -> std::same_as<bool>; }) {

				auto predicate = [](auto const& op) static { return op.IsPersistent(); };
				serialize<JSON>::op<Opts>(proxy | std::views::filter(predicate), args...);
			}
			else {

				serialize<JSON>::op<Opts>(proxy, args...);
			}
		}
	};
}
#ifndef BUCKET_STORAGE_H
#define BUCKET_STORAGE_H

#include <stdexcept>

#define SET_ITERATOR(a, b, c)                                                                                          \
	global_index = (a);                                                                                                \
	block_index = (b);                                                                                                 \
	element_index = (c);
#define IF_NOT_EQUAL(a, b, action, else_action)                                                                        \
	if ((a) != (b))                                                                                                    \
	{                                                                                                                  \
		action;                                                                                                        \
	}                                                                                                                  \
	else                                                                                                               \
	{                                                                                                                  \
		else_action;                                                                                                   \
	}
#define UPDATE_LINK(global_index, block_capacity, next_or_prev_active, head_or_tail_active, prev_or_next_global_index) \
	if (global_index != -1)                                                                                            \
	{                                                                                                                  \
		size_type block_index = static_cast< size_type >(global_index / block_capacity);                               \
		size_type element_index = static_cast< size_type >(global_index % block_capacity);                             \
		blocks[block_index]->next_or_prev_active[element_index] = prev_or_next_global_index;                           \
	}                                                                                                                  \
	else                                                                                                               \
	{                                                                                                                  \
		head_or_tail_active = prev_or_next_global_index;                                                               \
	}
#define COMPARE_ITERATORS_WITH_CONTAINER(return_1, return_2, return_3)                                                 \
	if (container != other.container)                                                                                  \
		throw std::invalid_argument("Iterators are from different containers");                                        \
	if (global_index == -1)                                                                                            \
		return return_1;                                                                                               \
	if (other.global_index == -1)                                                                                      \
		return return_2;                                                                                               \
	return return_3;
#define RETURN_HEAD_IF_EXISTS(return_1, return_2)                                                                      \
	if (head_active != -1)                                                                                             \
	{                                                                                                                  \
		size_type block_index = static_cast< size_type >(head_active / block_capacity);                                \
		size_type element_index = static_cast< size_type >(head_active % block_capacity);                              \
		return return_1;                                                                                               \
	}                                                                                                                  \
	return return_2;

// ------------------------------------
// START OF INTERFACE
//-------------------------------------
// исправления или какие добавочные комментарии написал используя //
// в нововй версии переход от элемента к элементу за O(1). Добавление за хвост за О(1) иначе за O(i+j),
// где i - кол-во блоков, j - количество элементов в блоке. Удаление за О(1)
template< typename T >
class BucketStorage
{
  public:
	class iterator;
	class Block;

	using value_type = T;
	using reference = T&;
	using const_reference = const T&;
	using size_type = std::size_t;
	using difference_type = std::ptrdiff_t;
	// не понял что требутся. итератор сам по себе константа.
	// если пользователю нужен постоянный указаетль, то он его
	// может выразить записав значние конструктора итератора в константу и вызывать конструктор по константе
	using const_iterator = iterator;

  private:
	size_type block_capacity;
	Block** blocks;
	size_type count_blocks;
	size_type blocks_capacity;
	size_type size_;
	// синоним count_blocks и это поле можно убрать в будущем, но сейчас используется и отсутсвие него в этой версии
	// приведет к ошибке
	size_type active_block_count;
	difference_type head_active;
	difference_type tail_active;

	void copy_from(const BucketStorage& other);
	template< typename... Args >
	iterator emplace(Args&&... args);
	template< typename... Args >
	iterator insert_element(size_type block_index, size_type element_index, Args&&... args);

  public:
	bool empty() const noexcept;
	size_type size() const noexcept;
	size_type capacity() const noexcept;

	iterator insert(const value_type& value);
	iterator insert(value_type&& value);
	iterator erase(const_iterator it);
	iterator get_to_distance(iterator it, const difference_type distance);
	void shrink_to_fit();
	void swap(BucketStorage& other) noexcept;

	BucketStorage();
	BucketStorage& operator=(const BucketStorage& other);
	BucketStorage& operator=(BucketStorage&& other) noexcept;
	explicit BucketStorage(size_type block_capacity_);
	BucketStorage(const BucketStorage& other);
	BucketStorage(BucketStorage&& other) noexcept;
	void clear();
	~BucketStorage();

	iterator begin() noexcept;
	const_iterator begin() const noexcept;
	const_iterator cbegin() const noexcept;
	iterator end() noexcept;
	const_iterator end() const noexcept;
	const_iterator cend() const noexcept;

	class iterator
	{
		friend class BucketStorage< T >;

	  public:
		using value_type = T;
		using difference_type = std::ptrdiff_t;
		using pointer = T*;
		using reference = T&;

		iterator();
		iterator(BucketStorage* container, size_type block_index, size_type element_index);
		reference operator*() const;
		pointer operator->() const;
		iterator& operator++();
		iterator operator++(int);
		iterator& operator--();
		iterator operator--(int);

		bool operator==(const iterator& other) const;
		bool operator!=(const iterator& other) const;
		bool operator<(const iterator& other) const;
		bool operator<=(const iterator& other) const;
		bool operator>(const iterator& other) const;
		bool operator>=(const iterator& other) const;

	  private:
		BucketStorage* container;
		size_type block_index;
		size_type element_index;
		difference_type global_index;
		void move_next();
		void move_prev();
	};

	class Block
	{
		// теперь не публично
		friend class BucketStorage< T >;
		T* elements;
		size_type* activity_flags;
		size_type count_active;
		size_type capacity;
		difference_type* next_active;
		difference_type* prev_active;

	  public:
		Block();
		Block(size_type capacity);
		Block(Block&& other) noexcept;
		Block& operator=(Block&& other) noexcept;
		void clear();
		void clean();
		void swap(Block& other) noexcept;
		~Block();
	};
};
// ------------------------------------
// START OF IMPLEMENTATION
// ------------------------------------
template< typename T >
BucketStorage< T >::Block::Block() :
	elements(static_cast< T* >(operator new[](64 * sizeof(T)))), activity_flags(new size_type[64]), count_active(0),
	capacity(64), next_active(new difference_type[64]), prev_active(new difference_type[64])
{
	// использую std::fill
	std::fill(activity_flags, activity_flags + 64, 1);
	std::fill(next_active, next_active + 64, -1);
	std::fill(prev_active, prev_active + 64, -1);
}

template< typename T >
BucketStorage< T >::Block::Block(size_type capacity_) :
	elements(static_cast< T* >(operator new[](capacity_ * sizeof(T)))), activity_flags(new size_type[capacity_]),
	count_active(0), capacity(capacity_), next_active(new difference_type[capacity_]), prev_active(new difference_type[capacity_])
{
	std::fill(activity_flags, activity_flags + capacity_, 1);
	std::fill(next_active, next_active + capacity_, -1);
	std::fill(prev_active, prev_active + capacity_, -1);
}

template< typename T >
BucketStorage< T >::Block::Block(Block&& other) noexcept :
	elements(other.elements), activity_flags(other.activity_flags), count_active(other.count_active),
	capacity(other.capacity), next_active(other.next_active), prev_active(other.prev_active)
{
	other.elements = nullptr;
	other.activity_flags = nullptr;
	other.count_active = 0;
	other.capacity = 0;
	other.next_active = nullptr;
	other.prev_active = nullptr;
}

template< typename T >
typename BucketStorage< T >::Block& BucketStorage< T >::Block::operator=(Block&& other) noexcept
{
	if (this != &other)
	{
		// испольщуй clear и swap
		clear();
		swap(other);
	}
	return *this;
}

template< typename T >
BucketStorage< T >::Block::~Block()
{
	clear();
}

template< typename T >
BucketStorage< T >& BucketStorage< T >::operator=(BucketStorage&& other) noexcept
{
	if (this != &other)
	{
		// теперь через swap
		clear();
		swap(other);
	}
	return *this;
}

template< typename T >
BucketStorage< T >& BucketStorage< T >::operator=(const BucketStorage& other)
{
	if (this != &other)
	{
		BucketStorage temp(other);
		swap(temp);
	}
	return *this;
}

template< typename T >
BucketStorage< T >::BucketStorage() :
	block_capacity(64), blocks(nullptr), count_blocks(0), blocks_capacity(0), size_(0), active_block_count(0),
	head_active(-1), tail_active(-1)
{
}

template< typename T >
BucketStorage< T >::BucketStorage(size_type block_capacity_) :
	block_capacity(block_capacity_), blocks(nullptr), count_blocks(0), blocks_capacity(0), size_(0),
	active_block_count(0), head_active(-1), tail_active(-1)
{
}

template< typename T >
BucketStorage< T >::BucketStorage(const BucketStorage& other) :
	block_capacity(other.block_capacity), blocks(nullptr), count_blocks(0), blocks_capacity(0), size_(0),
	active_block_count(0), head_active(-1), tail_active(-1)
{
	copy_from(other);
}

template< typename T >
BucketStorage< T >::BucketStorage(BucketStorage&& other) noexcept :
	block_capacity(other.block_capacity), blocks(other.blocks), count_blocks(other.count_blocks),
	blocks_capacity(other.blocks_capacity), size_(other.size_), active_block_count(other.active_block_count),
	head_active(other.head_active), tail_active(other.tail_active)
{
	// other.clear() здесь нельзя вызывать
	other.blocks = nullptr;
	other.count_blocks = 0;
	other.blocks_capacity = 0;
	other.size_ = 0;
	other.active_block_count = 0;
	other.head_active = -1;
	other.tail_active = -1;
}

template< typename T >
BucketStorage< T >::~BucketStorage()
{
	clear();
}

template< typename T >
typename BucketStorage< T >::iterator BucketStorage< T >::insert(const value_type& value)
{
	return emplace(value);
}

template< typename T >
typename BucketStorage< T >::iterator BucketStorage< T >::insert(value_type&& value)
{
	return emplace(std::move(value));
}

template< typename T >
bool BucketStorage< T >::empty() const noexcept
{
	return size_ == 0;
}

template< typename T >
typename BucketStorage< T >::size_type BucketStorage< T >::size() const noexcept
{
	return size_;
}

template< typename T >
typename BucketStorage< T >::size_type BucketStorage< T >::capacity() const noexcept
{
	return active_block_count * block_capacity;
}

template< typename T >
void BucketStorage< T >::iterator::move_prev()
{
	if (container == nullptr)
		return;

	if (global_index == -1)
	{
		IF_NOT_EQUAL(
			container->tail_active,
			-1,
			SET_ITERATOR(container->tail_active,
						 static_cast< size_type >(container->tail_active / container->block_capacity),
						 static_cast< size_type >(container->tail_active % container->block_capacity)),
			SET_ITERATOR(-1, 0, 0))
		return;
	}

	difference_type prev_global_index = container->blocks[block_index]->prev_active[element_index];
	IF_NOT_EQUAL(
		prev_global_index,
		-1,
		SET_ITERATOR(prev_global_index,
					 static_cast< size_type >(prev_global_index / container->block_capacity),
					 static_cast< size_type >(prev_global_index % container->block_capacity)),
		SET_ITERATOR(-1, 0, 0))
}

template< typename T >
void BucketStorage< T >::iterator::move_next()
{
	if (container == nullptr)
		return;

	if (global_index == -1)
	{
		IF_NOT_EQUAL(
			container->head_active,
			-1,
			SET_ITERATOR(container->head_active,
						 static_cast< size_type >(container->head_active / container->block_capacity),
						 static_cast< size_type >(container->head_active % container->block_capacity)),
			SET_ITERATOR(-1, 0, 0))
		return;
	}

	difference_type next_global_index = container->blocks[block_index]->next_active[element_index];
	IF_NOT_EQUAL(
		next_global_index,
		-1,
		SET_ITERATOR(next_global_index,
					 static_cast< size_type >(next_global_index / container->block_capacity),
					 static_cast< size_type >(next_global_index % container->block_capacity)),
		SET_ITERATOR(-1, container->count_blocks, 0))
}

template< typename T >
typename BucketStorage< T >::iterator BucketStorage< T >::get_to_distance(iterator it, const difference_type distance)
{
	iterator current = it;
	difference_type step = (distance > 0) ? 1 : -1;
	iterator finish = (distance > 0) ? end() : begin();

	for (difference_type d = 0; d != distance; d += step)
	{
		if (current == finish)
			break;
		step > 0 ? ++current : --current;
	}

	return current;
}

template< typename T >
void BucketStorage< T >::shrink_to_fit()
{
	if (size_ == 0)
	{
		clear();
		return;
	}

	size_type new_count_blocks = (size_ + block_capacity - 1) / block_capacity;

	if (new_count_blocks == count_blocks)
		return;

	BucketStorage< T > new_storage(block_capacity);
	new_storage.blocks_capacity = new_count_blocks;
	new_storage.blocks = new Block*[new_count_blocks];
	new_storage.count_blocks = new_count_blocks;

	for (size_type i = 0; i < new_count_blocks; ++i)
		new_storage.blocks[i] = new Block(block_capacity);

	for (size_type i = 0; i < count_blocks; ++i)
	{
		Block* current_block = blocks[i];
		for (size_type j = 0; j < block_capacity; ++j)
		{
			if (current_block->activity_flags[j] == 0)
				new_storage.emplace(std::move(current_block->elements[j]));
		}
	}
	// теперь просто создал новый бакет, наполнил и свапнул
	swap(new_storage);
	new_storage.clear();
}

template< typename T >
template< typename... Args >
typename BucketStorage< T >::iterator
	BucketStorage< T >::insert_element(size_type block_index, size_type element_index, Args&&... args)
{
	new (&blocks[block_index]->elements[element_index]) T(std::forward< Args >(args)...);
	blocks[block_index]->activity_flags[element_index] = 0;
	++blocks[block_index]->count_active;
	++size_;

	difference_type global_index = static_cast< difference_type >(block_index * block_capacity + element_index);

	if (head_active == -1)
	{
		head_active = global_index;
		tail_active = global_index;
	}
	else
	{
		size_type tail_block_index = static_cast< size_type >(tail_active / block_capacity);
		size_type tail_element_index = static_cast< size_type >(tail_active % block_capacity);
		blocks[tail_block_index]->next_active[tail_element_index] = global_index;
		blocks[block_index]->prev_active[element_index] = tail_active;
		tail_active = global_index;
	}

	if (blocks[block_index]->count_active == 1)
		++active_block_count;

	return iterator(this, block_index, element_index);
}

template< typename T >
template< typename... Args >
typename BucketStorage< T >::iterator BucketStorage< T >::emplace(Args&&... args)
{
	if (tail_active != -1)
	{
		size_type tail_block_index = static_cast< size_type >(tail_active / block_capacity);
		size_type tail_element_index = static_cast< size_type >(tail_active % block_capacity);
		// за хвостом есть пустой элемент
		if (tail_element_index + 1 < blocks[tail_block_index]->capacity &&
			blocks[tail_block_index]->activity_flags[tail_element_index + 1] != 0)
		{
			return insert_element(tail_block_index, tail_element_index + 1, std::forward< Args >(args)...);
		}
		// есть пустой элемент в блоке за хвостом
		else if (tail_block_index + 1 < count_blocks &&
				 blocks[tail_block_index + 1]->count_active < blocks[tail_block_index + 1]->capacity)
		{
			size_type next_block_index = tail_block_index + 1;
			for (size_type j = 0; j < blocks[next_block_index]->capacity; j++)
			{
				if (blocks[next_block_index]->activity_flags[j] != 0)
				{
					return insert_element(next_block_index, j, std::forward< Args >(args)...);
				}
			}
		}
	}
	// есть пустой элемент в теле
	for (size_type i = 0; i < count_blocks; i++)
	{
		if (blocks[i]->count_active < blocks[i]->capacity)
		{
			for (size_type j = 0; j < blocks[i]->capacity; j++)
			{
				if (blocks[i]->activity_flags[j] != 0)
				{
					return insert_element(i, j, std::forward< Args >(args)...);
				}
			}
		}
	}
	// место закончилось - выделить новое
	size_type new_blocks_capacity = blocks_capacity == 0 ? 1 : blocks_capacity * 2;
	Block** new_blocks = new Block*[new_blocks_capacity];
	std::copy(blocks, blocks + count_blocks, new_blocks);
	std::fill(new_blocks + count_blocks, new_blocks + new_blocks_capacity, nullptr);
	delete[] blocks;
	blocks = new_blocks;
	blocks_capacity = new_blocks_capacity;

	blocks[count_blocks] = new Block(block_capacity);
	++count_blocks;
	return insert_element(count_blocks - 1, 0, std::forward< Args >(args)...);
}

template< typename T >
typename BucketStorage< T >::iterator BucketStorage< T >::erase(const_iterator it)
{
	if (it == end())
		return end();

	size_type block_index = it.block_index;
	size_type element_index = it.element_index;

	if (block_index >= count_blocks || element_index >= block_capacity)
		throw std::out_of_range("Iterator out of range");

	Block* block = blocks[block_index];

	if (block->activity_flags[element_index] != 0)
		return it;

	block->elements[element_index].~T();
	block->activity_flags[element_index] = 1;

	difference_type prev_global_index = block->prev_active[element_index];
	difference_type next_global_index = block->next_active[element_index];

	UPDATE_LINK(prev_global_index, block_capacity, next_active, head_active, next_global_index);
	UPDATE_LINK(next_global_index, block_capacity, prev_active, tail_active, prev_global_index);

	block->next_active[element_index] = -1;
	block->prev_active[element_index] = -1;

	--block->count_active;
	--size_;
	if (block->count_active == 0)
	{
		block->clean();
		--active_block_count;
	}

	if (next_global_index != -1)
	{
		size_type next_block = static_cast< size_type >(next_global_index / block_capacity);
		size_type next_element = static_cast< size_type >(next_global_index % block_capacity);
		return iterator(this, next_block, next_element);
	}
	return end();
}

template< typename T >
void BucketStorage< T >::copy_from(const BucketStorage& other)
{
	block_capacity = other.block_capacity;
	blocks_capacity = other.count_blocks;
	count_blocks = other.count_blocks;
	size_ = other.size_;
	head_active = other.head_active;
	tail_active = other.tail_active;
	active_block_count = other.active_block_count;

	blocks = new Block*[blocks_capacity];
	for (size_type i = 0; i < count_blocks; i++)
	{
		// здесь несколькок вещей можно пепереписать через std::copy, но цикл в любом случае останется. Как упростить я
		// не понял
		blocks[i] = new Block(block_capacity);
		blocks[i]->count_active = other.blocks[i]->count_active;
		for (size_type j = 0; j < block_capacity; j++)
		{
			blocks[i]->activity_flags[j] = other.blocks[i]->activity_flags[j];
			if (blocks[i]->activity_flags[j] == 0)
				new (&blocks[i]->elements[j]) T(other.blocks[i]->elements[j]);
			blocks[i]->prev_active[j] = other.blocks[i]->prev_active[j];
			blocks[i]->next_active[j] = other.blocks[i]->next_active[j];
		}
	}
}
template< typename T >
void BucketStorage< T >::clear()
{
	for (size_type i = 0; i < count_blocks; ++i)
		delete blocks[i];
	delete[] blocks;
	blocks = nullptr;
	count_blocks = 0;
	blocks_capacity = 0;
	size_ = 0;
	active_block_count = 0;
	head_active = -1;
	tail_active = -1;
}

template< typename T >
void BucketStorage< T >::Block::clear()
{
	for (size_type i = 0; i < capacity; i++)
	{
		if (activity_flags[i] == 0)
			elements[i].~T();
	}
	delete[] activity_flags;
	operator delete[](elements);
	delete[] next_active;
	delete[] prev_active;

	elements = nullptr;
	activity_flags = nullptr;
	next_active = nullptr;
	prev_active = nullptr;
	count_active = 0;
	capacity = 0;
}

template< typename T >
void BucketStorage< T >::Block::clean()
{
	for (size_type i = 0; i < capacity; ++i)
	{
		if (activity_flags[i] == 0)
			elements[i].~T();
	}
	std::fill(activity_flags, activity_flags + capacity, 1);
	std::fill(next_active, next_active + capacity, -1);
	std::fill(prev_active, prev_active + capacity, -1);
	count_active = 0;
}

template< typename T >
BucketStorage< T >::iterator::iterator() : container(nullptr), block_index(0), element_index(0), global_index(-1)
{
}

template< typename T >
BucketStorage< T >::iterator::iterator(BucketStorage* container_, size_type block_index_, size_type element_index_) :
	container(container_), block_index(block_index_), element_index(element_index_)
{
	if (container != nullptr && block_index < container->count_blocks)
		global_index = static_cast< difference_type >(block_index * container->block_capacity + element_index);
	else
		global_index = -1;
}

template< typename T >
void BucketStorage< T >::swap(BucketStorage& other) noexcept
{
	// теперь использую using std::swap чтобы копилятор искал реализацию в начале
	// в моей реализаци и если не находил использовал обычный std::swap
	using std::swap;
	swap(block_capacity, other.block_capacity);
	swap(blocks, other.blocks);
	swap(count_blocks, other.count_blocks);
	swap(blocks_capacity, other.blocks_capacity);
	swap(size_, other.size_);
	swap(head_active, other.head_active);
	swap(tail_active, other.tail_active);
	swap(active_block_count, other.active_block_count);
}

template< typename T >
void BucketStorage< T >::Block::swap(Block& other) noexcept
{
	using std::swap;
	swap(elements, other.elements);
	swap(activity_flags, other.activity_flags);
	swap(count_active, other.count_active);
	swap(capacity, other.capacity);
	swap(next_active, other.next_active);
	swap(prev_active, other.prev_active);
}

template< typename T >
typename BucketStorage< T >::iterator BucketStorage< T >::begin() noexcept
{
	RETURN_HEAD_IF_EXISTS(iterator(this, block_index, element_index), end())
}

template< typename T >
typename BucketStorage< T >::const_iterator BucketStorage< T >::cbegin() const noexcept
{
	RETURN_HEAD_IF_EXISTS(const_iterator(const_cast< BucketStorage* >(this), block_index, element_index), cend())
}

template< typename T >
typename BucketStorage< T >::iterator BucketStorage< T >::end() noexcept
{
	return iterator(this, count_blocks, 0);
}

template< typename T >
typename BucketStorage< T >::const_iterator BucketStorage< T >::cend() const noexcept
{
	return const_iterator(const_cast< BucketStorage* >(this), count_blocks, 0);
}

template< typename T >
typename BucketStorage< T >::iterator::reference BucketStorage< T >::iterator::operator*() const
{
	return container->blocks[block_index]->elements[element_index];
}

template< typename T >
typename BucketStorage< T >::iterator::pointer BucketStorage< T >::iterator::operator->() const
{
	return &(operator*());
}

template< typename T >
typename BucketStorage< T >::iterator& BucketStorage< T >::iterator::operator++()
{
	move_next();
	return *this;
}

template< typename T >
typename BucketStorage< T >::iterator BucketStorage< T >::iterator::operator++(int)
{
	iterator tmp = *this;
	move_next();
	return tmp;
}

template< typename T >
typename BucketStorage< T >::iterator& BucketStorage< T >::iterator::operator--()
{
	move_prev();
	return *this;
}

template< typename T >
typename BucketStorage< T >::iterator BucketStorage< T >::iterator::operator--(int)
{
	iterator tmp = *this;
	move_prev();
	return tmp;
}

template< typename T >
bool BucketStorage< T >::iterator::operator==(const iterator& other) const
{
	if (container != other.container)
		throw std::invalid_argument("Iterators are from different containers");
	return global_index == other.global_index;
}

template< typename T >
bool BucketStorage< T >::iterator::operator!=(const iterator& other) const
{
	return !(*this == other);
}

template< typename T >
bool BucketStorage< T >::iterator::operator<(const iterator& other) const
{
	COMPARE_ITERATORS_WITH_CONTAINER(false, true, global_index < other.global_index)
}

template< typename T >
bool BucketStorage< T >::iterator::operator<=(const iterator& other) const
{
	COMPARE_ITERATORS_WITH_CONTAINER(other.global_index == -1, true, global_index <= other.global_index)
}

template< typename T >
bool BucketStorage< T >::iterator::operator>(const iterator& other) const
{
	return other < *this;
}

template< typename T >
bool BucketStorage< T >::iterator::operator>=(const iterator& other) const
{
	return other <= *this;
}

#endif

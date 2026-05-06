#include <not_implemented.h>
#include "../include/allocator_sorted_list.h"

using FitMode = allocator_with_fit_mode::fit_mode;
using MemResPtr = std::pmr::memory_resource *;
using BlockInfo = allocator_test_utils::block_info;

allocator_sorted_list::allocator_sorted_list(
        size_t space_size,
        std::pmr::memory_resource *parent_allocator,
        allocator_with_fit_mode::fit_mode allocate_fit_mode)
    : _trusted_memory(nullptr)
{
    std::pmr::memory_resource *alloc = parent_allocator == nullptr ? std::pmr::get_default_resource() : parent_allocator;
    
    size_t total_size = allocator_metadata_size + space_size;
    _trusted_memory = alloc->allocate(total_size, alignof(std::max_align_t));
    
    char *mem = static_cast<char *>(_trusted_memory);
    
    std::pmr::memory_resource **meta_parent = reinterpret_cast<std::pmr::memory_resource **>(mem);
    *meta_parent = alloc;
    
    allocator_with_fit_mode::fit_mode *meta_fit_mode = reinterpret_cast<allocator_with_fit_mode::fit_mode *>(mem + sizeof(std::pmr::memory_resource *));
    *meta_fit_mode = allocate_fit_mode;
    
    size_t *meta_total_size = reinterpret_cast<size_t *>(mem + sizeof(std::pmr::memory_resource *) + sizeof(allocator_with_fit_mode::fit_mode));
    *meta_total_size = space_size;
    
    void **meta_mutex = reinterpret_cast<void **>(mem + sizeof(std::pmr::memory_resource *) + sizeof(allocator_with_fit_mode::fit_mode) + sizeof(size_t));
    new (meta_mutex) std::mutex();
    
    void **meta_list_head = reinterpret_cast<void **>(mem + sizeof(std::pmr::memory_resource *) + sizeof(allocator_with_fit_mode::fit_mode) + sizeof(size_t) + sizeof(std::mutex));
    *meta_list_head = nullptr;
    
    char *data_start = mem + allocator_metadata_size;
    void **first_block_next = reinterpret_cast<void **>(data_start);
    *first_block_next = nullptr;
    
    size_t *first_block_size = reinterpret_cast<size_t *>(data_start + sizeof(void *));
    *first_block_size = space_size - block_metadata_size;
    
    *meta_list_head = data_start;
}

allocator_sorted_list::~allocator_sorted_list()
{
    if (_trusted_memory == nullptr) return;
    
    char *mem = static_cast<char *>(_trusted_memory);
    std::pmr::memory_resource *parent = *reinterpret_cast<std::pmr::memory_resource **>(mem);
    
    void *mutex_ptr = mem + sizeof(std::pmr::memory_resource *) + sizeof(allocator_with_fit_mode::fit_mode) + sizeof(size_t);
    std::mutex *m = reinterpret_cast<std::mutex *>(mutex_ptr);
    m->~mutex();
    
    parent->deallocate(_trusted_memory, 0, alignof(std::max_align_t));
    _trusted_memory = nullptr;
}

allocator_sorted_list::allocator_sorted_list(const allocator_sorted_list &other)
{
    throw not_implemented("allocator_sorted_list::allocator_sorted_list(const allocator_sorted_list &other)", "your code should be here...");
}

allocator_sorted_list &allocator_sorted_list::operator=(const allocator_sorted_list &other)
{
    return *this;
}

allocator_sorted_list::allocator_sorted_list(allocator_sorted_list &&other) noexcept
    : _trusted_memory(other._trusted_memory)
{
    other._trusted_memory = nullptr;
}

allocator_sorted_list &allocator_sorted_list::operator=(allocator_sorted_list &&other) noexcept
{
    if (this != &other) {
        if (_trusted_memory != nullptr) {
            this->~allocator_sorted_list();
        }
        _trusted_memory = other._trusted_memory;
        other._trusted_memory = nullptr;
    }
    return *this;
}

[[nodiscard]] void *allocator_sorted_list::do_allocate_sm(size_t size)
{
    std::lock_guard<std::mutex> lock(*get_mutex());
    
    size_t needed = size + block_metadata_size;
    
    void *best_block = nullptr;
    void *prev_best = nullptr;
    size_t best_size = 0;
    void *prev = nullptr;
    
    allocator_with_fit_mode::fit_mode mode = *get_fit_mode();
    
    for (auto it = free_begin(); it != free_end(); ++it) {
        void *block = *it;
        size_t block_size = get_block_size(block);
        
        if (block_size >= size) {
            if (mode == allocator_with_fit_mode::fit_mode::first_fit) {
                best_block = block;
                prev_best = prev;
                best_size = block_size;
                break;
            }
            
            if (best_block == nullptr || 
                (mode == allocator_with_fit_mode::fit_mode::the_best_fit && block_size < best_size) ||
                (mode == allocator_with_fit_mode::fit_mode::the_worst_fit && block_size > best_size)) {
                best_block = block;
                prev_best = prev;
                best_size = block_size;
            }
        }
        prev = block;
    }
    
    if (best_block == nullptr) {
        throw std::bad_alloc();
    }
    
    remove_free_block(prev_best, best_block);
    
    if (best_size >= size + block_metadata_size + 1) {
        void *remaining = static_cast<char *>(best_block) + block_metadata_size + size;
        
        size_t *remaining_size = reinterpret_cast<size_t *>(static_cast<char *>(remaining) + sizeof(void *));
        *remaining_size = best_size - size - block_metadata_size;
        
        void **remaining_next = reinterpret_cast<void **>(remaining);
        *remaining_next = nullptr;
        
        insert_free_block(remaining);
    }
    
    set_block_size(best_block, size);
    mark_block_occupied(best_block);
    
    return static_cast<char *>(best_block) + block_metadata_size;
}

void allocator_sorted_list::do_deallocate_sm(void *at)
{
    if (at == nullptr) return;
    
    void *block = static_cast<char *>(at) - block_metadata_size;
    
    std::lock_guard<std::mutex> lock(*get_mutex());
    
    if (!is_from_trusted_memory(block)) {
        throw std::invalid_argument("Block does not belong to this allocator");
    }
    
    size_t block_size = get_block_size(block);
    mark_block_free(block);
    
    insert_sorted_free_block(block, block_size);
    
    merge_adjacent_free_blocks();
}

bool allocator_sorted_list::do_is_equal(const std::pmr::memory_resource &other) const noexcept
{
    return dynamic_cast<const allocator_sorted_list *>(&other) != nullptr;
}

inline void allocator_sorted_list::set_fit_mode(allocator_with_fit_mode::fit_mode mode)
{
    std::lock_guard<std::mutex> lock(*get_mutex());
    *get_fit_mode() = mode;
}

std::vector<allocator_test_utils::block_info> allocator_sorted_list::get_blocks_info() const noexcept
{
    std::lock_guard<std::mutex> lock(*const_cast<std::mutex *>(get_mutex()));
    return get_blocks_info_inner();
}

std::vector<allocator_test_utils::block_info> allocator_sorted_list::get_blocks_info_inner() const
{
    std::vector<block_info> result;
    
    if (_trusted_memory == nullptr) return result;
    
    char *mem = static_cast<char *>(_trusted_memory);
    char *data_start = mem + allocator_metadata_size;
    char *data_end = data_start + *get_total_size();
    
    for (sorted_iterator it = begin(); it != sorted_iterator(data_end, data_end); ++it) {
        block_info info;
        info.block_size = get_block_size(*it);
        info.is_block_occupied = is_block_occupied(*it);
        result.push_back(info);
    }
    
    return result;
}

std::mutex *allocator_sorted_list::get_mutex() const
{
    char *mem = static_cast<char *>(_trusted_memory);
    void *mutex_ptr = mem + sizeof(std::pmr::memory_resource *) + sizeof(allocator_with_fit_mode::fit_mode) + sizeof(size_t);
    return reinterpret_cast<std::mutex *>(mutex_ptr);
}

allocator_with_fit_mode::fit_mode *allocator_sorted_list::get_fit_mode() const
{
    char *mem = static_cast<char *>(_trusted_memory);
    return reinterpret_cast<allocator_with_fit_mode::fit_mode *>(mem + sizeof(std::pmr::memory_resource *));
}

size_t *allocator_sorted_list::get_total_size() const
{
    char *mem = static_cast<char *>(_trusted_memory);
    return reinterpret_cast<size_t *>(mem + sizeof(std::pmr::memory_resource *) + sizeof(allocator_with_fit_mode::fit_mode));
}

void **allocator_sorted_list::get_list_head() const
{
    char *mem = static_cast<char *>(_trusted_memory);
    return reinterpret_cast<void **>(mem + sizeof(std::pmr::memory_resource *) + sizeof(allocator_with_fit_mode::fit_mode) + sizeof(size_t) + sizeof(std::mutex));
}

bool allocator_sorted_list::is_from_trusted_memory(void *ptr) const
{
    if (_trusted_memory == nullptr) return false;
    
    char *mem = static_cast<char *>(_trusted_memory);
    char *data_start = mem + allocator_metadata_size;
    char *end = data_start + *get_total_size();
    
    char *p = static_cast<char *>(ptr);
    return p >= data_start && p < end;
}

size_t allocator_sorted_list::get_block_size(void *block) const
{
    return *reinterpret_cast<size_t *>(static_cast<char *>(block) + sizeof(void *));
}

void allocator_sorted_list::set_block_size(void *block, size_t size)
{
    *reinterpret_cast<size_t *>(static_cast<char *>(block) + sizeof(void *)) = size;
}

bool allocator_sorted_list::is_block_occupied(void *block) const
{
    return *reinterpret_cast<char *>(static_cast<char *>(block) + sizeof(void *) + sizeof(size_t)) != 0;
}

void allocator_sorted_list::mark_block_occupied(void *block)
{
    *reinterpret_cast<char *>(static_cast<char *>(block) + sizeof(void *) + sizeof(size_t)) = 1;
}

void allocator_sorted_list::mark_block_free(void *block)
{
    *reinterpret_cast<char *>(static_cast<char *>(block) + sizeof(void *) + sizeof(size_t)) = 0;
}

void **allocator_sorted_list::get_block_next(void *block) const
{
    return reinterpret_cast<void **>(block);
}

void allocator_sorted_list::insert_free_block(void *block)
{
    void **head = get_list_head();
    void **block_next = get_block_next(block);
    *block_next = *head;
    *head = block;
}

void allocator_sorted_list::remove_free_block(void *prev, void *block)
{
    void **head = get_list_head();
    
    if (prev == nullptr) {
        *head = *get_block_next(block);
    } else {
        void **prev_next = get_block_next(prev);
        *prev_next = *get_block_next(block);
    }
    
    *get_block_next(block) = nullptr;
}

void allocator_sorted_list::insert_sorted_free_block(void *block, size_t size)
{
    set_block_size(block, size);
    
    void **head = get_list_head();
    void *current = *head;
    void *prev = nullptr;
    
    while (current != nullptr && current < block) {
        prev = current;
        current = *get_block_next(current);
    }
    
    if (prev == nullptr) {
        *get_block_next(block) = *head;
        *head = block;
    } else {
        *get_block_next(block) = *get_block_next(prev);
        *get_block_next(prev) = block;
    }
}

void allocator_sorted_list::merge_adjacent_free_blocks()
{
    void **head = get_list_head();
    if (*head == nullptr) return;
    
    void *current = *head;
    while (current != nullptr) {
        void *next = *get_block_next(current);
        if (next == nullptr) break;
        
        char *current_end = static_cast<char *>(current) + block_metadata_size + get_block_size(current);
        char *next_start = static_cast<char *>(next);
        
        if (current_end == next_start) {
            size_t new_size = get_block_size(current) + block_metadata_size + get_block_size(next);
            set_block_size(current, new_size);
            *get_block_next(current) = *get_block_next(next);
        } else {
            current = next;
        }
    }
}

allocator_sorted_list::sorted_free_iterator allocator_sorted_list::free_begin() const noexcept
{
    return sorted_free_iterator(*get_list_head());
}

allocator_sorted_list::sorted_free_iterator allocator_sorted_list::free_end() const noexcept
{
    return sorted_free_iterator(nullptr);
}

allocator_sorted_list::sorted_iterator allocator_sorted_list::begin() const noexcept
{
    char *mem = static_cast<char *>(_trusted_memory);
    char *data_start = mem + allocator_metadata_size;
    char *end = data_start + *get_total_size();
    return sorted_iterator(data_start, end);
}

allocator_sorted_list::sorted_iterator allocator_sorted_list::end() const noexcept
{
    char *mem = static_cast<char *>(_trusted_memory);
    char *data_start = mem + allocator_metadata_size;
    char *end = data_start + *get_total_size();
    return sorted_iterator(end, end);
}

bool allocator_sorted_list::sorted_free_iterator::operator==(const allocator_sorted_list::sorted_free_iterator &other) const noexcept
{
    return _free_ptr == other._free_ptr;
}

bool allocator_sorted_list::sorted_free_iterator::operator!=(const allocator_sorted_list::sorted_free_iterator &other) const noexcept
{
    return _free_ptr != other._free_ptr;
}

allocator_sorted_list::sorted_free_iterator &allocator_sorted_list::sorted_free_iterator::operator++() & noexcept
{
    if (_free_ptr != nullptr) {
        _free_ptr = *reinterpret_cast<void **>(_free_ptr);
    }
    return *this;
}

allocator_sorted_list::sorted_free_iterator allocator_sorted_list::sorted_free_iterator::operator++(int n)
{
    sorted_free_iterator temp = *this;
    if (_free_ptr != nullptr) {
        ++(*this);
    }
    return temp;
}

size_t allocator_sorted_list::sorted_free_iterator::size() const noexcept
{
    if (_free_ptr == nullptr) return 0;
    return *reinterpret_cast<size_t *>(static_cast<char *>(_free_ptr) + sizeof(void *));
}

void *allocator_sorted_list::sorted_free_iterator::operator*() const noexcept
{
    return _free_ptr;
}

allocator_sorted_list::sorted_free_iterator::sorted_free_iterator()
    : _free_ptr(nullptr)
{
}

allocator_sorted_list::sorted_free_iterator::sorted_free_iterator(void *trusted)
    : _free_ptr(trusted)
{
}

bool allocator_sorted_list::sorted_iterator::operator==(const allocator_sorted_list::sorted_iterator &other) const noexcept
{
    return _current_ptr == other._current_ptr;
}

bool allocator_sorted_list::sorted_iterator::operator!=(const allocator_sorted_list::sorted_iterator &other) const noexcept
{
    return _current_ptr != other._current_ptr;
}

allocator_sorted_list::sorted_iterator &allocator_sorted_list::sorted_iterator::operator++() & noexcept
{
    if (_current_ptr == nullptr || _current_ptr >= _end_ptr) {
        _current_ptr = _end_ptr;
        return *this;
    }
    
    bool occupied = *reinterpret_cast<char *>(static_cast<char *>(_current_ptr) + sizeof(void *) + sizeof(size_t));
    size_t block_sz = *reinterpret_cast<size_t *>(static_cast<char *>(_current_ptr) + sizeof(void *));
    
    if (occupied) {
        _current_ptr = static_cast<char *>(_current_ptr) + block_metadata_size + block_sz;
    } else {
        _current_ptr = *reinterpret_cast<void **>(_current_ptr);
    }
    
    if (_current_ptr > _end_ptr) _current_ptr = _end_ptr;
    
    return *this;
}

allocator_sorted_list::sorted_iterator allocator_sorted_list::sorted_iterator::operator++(int n)
{
    sorted_iterator temp = *this;
    ++(*this);
    return temp;
}

size_t allocator_sorted_list::sorted_iterator::size() const noexcept
{
    return *reinterpret_cast<size_t *>(static_cast<char *>(_current_ptr) + sizeof(void *));
}

void *allocator_sorted_list::sorted_iterator::operator*() const noexcept
{
    return _current_ptr;
}

bool allocator_sorted_list::sorted_iterator::occupied() const noexcept
{
    return *reinterpret_cast<char *>(static_cast<char *>(_current_ptr) + sizeof(void *) + sizeof(size_t)) != 0;
}

allocator_sorted_list::sorted_iterator::sorted_iterator()
    : _current_ptr(nullptr), _end_ptr(nullptr)
{
}

allocator_sorted_list::sorted_iterator::sorted_iterator(void *current, void *end)
    : _current_ptr(current), _end_ptr(end)
{
}
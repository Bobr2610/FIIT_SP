#include "../include/allocator_boundary_tags.h"

using FitMode = allocator_with_fit_mode::fit_mode;
using MemResPtr = std::pmr::memory_resource *;
using BlockInfo = allocator_test_utils::block_info;

allocator_boundary_tags::allocator_boundary_tags(
        size_t space_size,
        std::pmr::memory_resource *parent_allocator,
        allocator_with_fit_mode::fit_mode allocate_fit_mode)
    : _trusted_memory(nullptr)
{
    MemResPtr alloc = parent_allocator == nullptr ? std::pmr::get_default_resource() : parent_allocator;
    
    size_t total_size = allocator_metadata_size + space_size;
    _trusted_memory = alloc->allocate(total_size, alignof(std::max_align_t));
    
    char *mem = static_cast<char *>(_trusted_memory);
    
    MemResPtr *meta_parent = reinterpret_cast<MemResPtr *>(mem);
    *meta_parent = alloc;
    
    FitMode *meta_fit_mode = reinterpret_cast<FitMode *>(mem + sizeof(MemResPtr));
    *meta_fit_mode = allocate_fit_mode;
    
    size_t *meta_total_size = reinterpret_cast<size_t *>(mem + sizeof(MemResPtr) + sizeof(FitMode));
    *meta_total_size = space_size;
    
    void **meta_mutex = reinterpret_cast<void **>(mem + sizeof(MemResPtr) + sizeof(FitMode) + sizeof(size_t));
    new (meta_mutex) std::mutex();
    
    char *data_start = mem + allocator_metadata_size;

    set_block_size(data_start, space_size);
    set_block_next(data_start, nullptr);
    set_block_prev(data_start, nullptr);
    set_block_next_foot(data_start, nullptr);
}

allocator_boundary_tags::~allocator_boundary_tags()
{
    if (_trusted_memory == nullptr) return;
    
    char *mem = static_cast<char *>(_trusted_memory);
    MemResPtr parent = *reinterpret_cast<MemResPtr *>(mem);
    
    void *mutex_ptr = mem + sizeof(MemResPtr) + sizeof(FitMode) + sizeof(size_t);
    std::mutex *m = reinterpret_cast<std::mutex *>(mutex_ptr);
    m->~mutex();
    
    parent->deallocate(_trusted_memory, 0, alignof(std::max_align_t));
    _trusted_memory = nullptr;
}

allocator_boundary_tags::allocator_boundary_tags(const allocator_boundary_tags &other)
{
    throw std::runtime_error("Copy constructor not implemented");
}

allocator_boundary_tags &allocator_boundary_tags::operator=(const allocator_boundary_tags &other)
{
    return *this;
}

allocator_boundary_tags::allocator_boundary_tags(allocator_boundary_tags &&other) noexcept
    : _trusted_memory(other._trusted_memory)
{
    other._trusted_memory = nullptr;
}

allocator_boundary_tags &allocator_boundary_tags::operator=(allocator_boundary_tags &&other) noexcept
{
    if (this != &other) {
        if (_trusted_memory != nullptr) {
            this->~allocator_boundary_tags();
        }
        _trusted_memory = other._trusted_memory;
        other._trusted_memory = nullptr;
    }
    return *this;
}

std::mutex *allocator_boundary_tags::get_mutex() const
{
    char *mem = static_cast<char *>(_trusted_memory);
    void *mutex_ptr = mem + sizeof(MemResPtr) + sizeof(FitMode) + sizeof(size_t);
    return reinterpret_cast<std::mutex *>(mutex_ptr);
}

FitMode *allocator_boundary_tags::get_fit_mode() const
{
    char *mem = static_cast<char *>(_trusted_memory);
    return reinterpret_cast<FitMode *>(mem + sizeof(MemResPtr));
}

size_t *allocator_boundary_tags::get_total_size() const
{
    char *mem = static_cast<char *>(_trusted_memory);
    return reinterpret_cast<size_t *>(mem + sizeof(MemResPtr) + sizeof(FitMode));
}

char *allocator_boundary_tags::get_data_start() const
{
    char *mem = static_cast<char *>(_trusted_memory);
    return mem + allocator_metadata_size;
}

void *allocator_boundary_tags::get_first_block() const
{
    return get_data_start();
}

char *allocator_boundary_tags::get_data_end() const
{
    return get_data_start() + *get_total_size();
}

bool allocator_boundary_tags::is_from_trusted_memory(void *ptr) const
{
    if (_trusted_memory == nullptr) return false;
    
    char *p = static_cast<char *>(ptr);
    return p >= get_data_start() && p < get_data_end();
}

size_t allocator_boundary_tags::get_block_size(void *block) const
{
    return *reinterpret_cast<size_t *>(block);
}

void allocator_boundary_tags::set_block_size(void *block, size_t size)
{
    *reinterpret_cast<size_t *>(block) = size;
}

void *allocator_boundary_tags::get_block_next(void *block) const
{
    return *reinterpret_cast<void **>(static_cast<char *>(block) + sizeof(size_t));
}

void allocator_boundary_tags::set_block_next(void *block, void *next)
{
    *reinterpret_cast<void **>(static_cast<char *>(block) + sizeof(size_t)) = next;
}

void *allocator_boundary_tags::get_block_prev(void *block) const
{
    return *reinterpret_cast<void **>(static_cast<char *>(block) + sizeof(size_t) + sizeof(void *));
}

void *allocator_boundary_tags::get_block_next_foot(void *block) const
{
    return *reinterpret_cast<void **>(static_cast<char *>(block) + sizeof(size_t) + sizeof(void *) * 2);
}

void allocator_boundary_tags::set_block_prev(void *block, void *prev)
{
    *reinterpret_cast<void **>(static_cast<char *>(block) + sizeof(size_t) + sizeof(void *)) = prev;
}

void allocator_boundary_tags::set_block_next_foot(void *block, void *next_foot)
{
    *reinterpret_cast<void **>(static_cast<char *>(block) + sizeof(size_t) + sizeof(void *) * 2) = next_foot;
}

bool allocator_boundary_tags::is_block_occupied(void *block) const
{
    return get_block_next_foot(block) != nullptr;
}

void allocator_boundary_tags::mark_block_occupied(void *block)
{
    set_block_next_foot(block, block);
}

void allocator_boundary_tags::mark_block_free(void *block)
{
    set_block_next_foot(block, nullptr);
}

void *allocator_boundary_tags::get_user_data_start(void *block) const
{
    return static_cast<char *>(block) + occupied_block_metadata_size;
}

void *allocator_boundary_tags::get_block_from_user_data(void *user_ptr) const
{
    return static_cast<char *>(user_ptr) - occupied_block_metadata_size;
}

void allocator_boundary_tags::split_block(void *block, size_t user_size)
{
    size_t current_size = get_block_size(block);
    size_t needed = occupied_block_metadata_size + user_size;
    
    if (current_size < needed + occupied_block_metadata_size + 1) {
        return;
    }
    
    char *remaining = static_cast<char *>(block) + needed;
    size_t remaining_size = current_size - needed;

    void *old_next = get_block_next(block);

    set_block_size(block, needed);
    set_block_next(block, remaining);

    set_block_size(remaining, remaining_size);
    set_block_next(remaining, old_next);
    set_block_prev(remaining, block);
    set_block_next_foot(remaining, nullptr);

    void *next = old_next;
    if (next != nullptr) {
        set_block_prev(next, remaining);
    }
}

void allocator_boundary_tags::merge_with_next(void *block)
{
    void *next = get_block_next(block);
    if (next == nullptr) return;
    
    char *block_end = static_cast<char *>(block) + get_block_size(block);
    char *next_start = static_cast<char *>(next);
    
    if (block_end != next_start) return;
    
    if (is_block_occupied(next)) return;
    
    size_t new_size = get_block_size(block) + get_block_size(next);
    set_block_size(block, new_size);
    
    void *next_next = get_block_next(next);
    set_block_next(block, next_next);
    
    if (next_next != nullptr) {
        set_block_prev(next_next, block);
    }
}

[[nodiscard]] void *allocator_boundary_tags::do_allocate_sm(size_t size)
{
    std::lock_guard<std::mutex> lock(*get_mutex());
    
    void *best_block = nullptr;
    size_t best_size = 0;
    void *prev = nullptr;
    void *prev_best = nullptr;
    
    FitMode mode = *get_fit_mode();
    
    char *data_start = get_data_start();
    char *data_end = get_data_end();
    char *current = data_start;
    void *prev_block = nullptr;
    
    while (current < data_end) {
        void *block = current;
        
        if (!is_block_occupied(block)) {
            size_t block_size = get_block_size(block);
            
            if (block_size >= occupied_block_metadata_size + size) {
                if (best_block == nullptr || 
                    (mode == FitMode::first_fit) ||
                    (mode == FitMode::the_best_fit && block_size < best_size) ||
                    (mode == FitMode::the_worst_fit && block_size > best_size)) {
                    best_block = block;
                    best_size = block_size;
                    prev_best = prev_block;
                    
                    if (mode == FitMode::first_fit) {
                        break;
                    }
                }
            }
        }
        
        prev_block = block;
        size_t block_sz = get_block_size(block);
        current = current + block_sz;
        
        if (current > data_end) break;
    }
    
    if (best_block == nullptr) {
        throw std::bad_alloc();
    }
    
    split_block(best_block, size);
    mark_block_occupied(best_block);
    
    return get_user_data_start(best_block);
}

void allocator_boundary_tags::do_deallocate_sm(void *at)
{
    if (at == nullptr) return;
    
    void *block = get_block_from_user_data(at);
    
    std::lock_guard<std::mutex> lock(*get_mutex());
    
    if (!is_from_trusted_memory(block)) {
        throw std::invalid_argument("Block does not belong to this allocator");
    }
    
    mark_block_free(block);
    
    merge_with_next(block);
    
    void *prev = get_block_prev(block);
    if (prev != nullptr && !is_block_occupied(prev)) {
        char *prev_end = static_cast<char *>(prev) + get_block_size(prev);
        if (prev_end == block) {
            block = prev;
            merge_with_next(block);
        }
    }
}

bool allocator_boundary_tags::do_is_equal(const std::pmr::memory_resource &other) const noexcept
{
    return dynamic_cast<const allocator_boundary_tags *>(&other) != nullptr;
}

inline void allocator_boundary_tags::set_fit_mode(allocator_with_fit_mode::fit_mode mode)
{
    std::lock_guard<std::mutex> lock(*get_mutex());
    *get_fit_mode() = mode;
}

std::vector<allocator_test_utils::block_info> allocator_boundary_tags::get_blocks_info() const
{
    std::lock_guard<std::mutex> lock(*const_cast<std::mutex *>(get_mutex()));
    return get_blocks_info_inner();
}

std::vector<allocator_test_utils::block_info> allocator_boundary_tags::get_blocks_info_inner() const
{
    std::vector<BlockInfo> result;
    
    if (_trusted_memory == nullptr) return result;
    
    char *data_start = get_data_start();
    char *data_end = get_data_end();
    char *current = data_start;
    
    while (current < data_end) {
        void *block = current;
        BlockInfo info;
        info.block_size = get_block_size(block);
        info.is_block_occupied = is_block_occupied(block);
        result.push_back(info);
        
        size_t block_sz = get_block_size(block);
        current = current + block_sz;
        
        if (current > data_end) break;
    }
    
    return result;
}

allocator_boundary_tags::boundary_iterator allocator_boundary_tags::begin() const noexcept
{
    return boundary_iterator(_trusted_memory);
}

allocator_boundary_tags::boundary_iterator allocator_boundary_tags::end() const noexcept
{
    return boundary_iterator(nullptr);
}

bool allocator_boundary_tags::boundary_iterator::operator==(const allocator_boundary_tags::boundary_iterator &other) const noexcept
{
    return _occupied_ptr == other._occupied_ptr;
}

bool allocator_boundary_tags::boundary_iterator::operator!=(const allocator_boundary_tags::boundary_iterator &other) const noexcept
{
    return _occupied_ptr != other._occupied_ptr;
}

allocator_boundary_tags::boundary_iterator &allocator_boundary_tags::boundary_iterator::operator++() & noexcept
{
    if (_occupied_ptr == nullptr) return *this;
    
    char *mem = static_cast<char *>(_trusted_memory);
    char *data_start = mem + allocator_metadata_size;
    size_t *total_size = reinterpret_cast<size_t *>(mem + sizeof(MemResPtr) + sizeof(FitMode));
    char *data_end = data_start + *total_size;
    
    void *current = _occupied_ptr;
    size_t block_size = *reinterpret_cast<size_t *>(current);
    char *next = static_cast<char *>(current) + block_size;
    
    if (next >= data_end) {
        _occupied_ptr = nullptr;
    } else {
        _occupied_ptr = next;
    }
    
    return *this;
}

allocator_boundary_tags::boundary_iterator &allocator_boundary_tags::boundary_iterator::operator--() & noexcept
{
    if (_occupied_ptr == nullptr) {
        if (_trusted_memory == nullptr) return *this;
        
        char *mem = static_cast<char *>(_trusted_memory);
        char *data_start = mem + allocator_metadata_size;
        size_t *total_size = reinterpret_cast<size_t *>(mem + sizeof(MemResPtr) + sizeof(FitMode));
        char *data_end = data_start + *total_size;
        
        _occupied_ptr = data_end;
        return *this;
    }
    
    char *current = static_cast<char *>(_occupied_ptr);
    char *mem = static_cast<char *>(_trusted_memory);
    char *data_start = mem + allocator_metadata_size;
    
    if (current <= data_start) {
        _occupied_ptr = nullptr;
        return *this;
    }
    
    _occupied_ptr = nullptr;
    char *search = data_start;
    void *prev_block = nullptr;
    
    while (search < current) {
        prev_block = search;
        size_t block_sz = *reinterpret_cast<size_t *>(search);
        search = search + block_sz;
        
        if (search > current) break;
    }
    
    _occupied_ptr = prev_block;
    
    return *this;
}

allocator_boundary_tags::boundary_iterator allocator_boundary_tags::boundary_iterator::operator++(int n)
{
    boundary_iterator temp = *this;
    ++(*this);
    return temp;
}

allocator_boundary_tags::boundary_iterator allocator_boundary_tags::boundary_iterator::operator--(int n)
{
    boundary_iterator temp = *this;
    --(*this);
    return temp;
}

size_t allocator_boundary_tags::boundary_iterator::size() const noexcept
{
    if (_occupied_ptr == nullptr) return 0;
    return *reinterpret_cast<size_t *>(_occupied_ptr);
}

bool allocator_boundary_tags::boundary_iterator::occupied() const noexcept
{
    if (_occupied_ptr == nullptr) return false;
    void *flag_ptr = *reinterpret_cast<void **>(static_cast<char *>(_occupied_ptr) + sizeof(size_t) + sizeof(void *) * 2);
    return flag_ptr != nullptr;
}

void *allocator_boundary_tags::boundary_iterator::operator*() const noexcept
{
    return _occupied_ptr;
}

void *allocator_boundary_tags::boundary_iterator::get_ptr() const noexcept
{
    return _occupied_ptr;
}

allocator_boundary_tags::boundary_iterator::boundary_iterator()
    : _occupied_ptr(nullptr), _trusted_memory(nullptr)
{
}

allocator_boundary_tags::boundary_iterator::boundary_iterator(void *trusted)
    : _occupied_ptr(nullptr), _trusted_memory(nullptr)
{
    if (trusted != nullptr) {
        char *mem = static_cast<char *>(trusted);
        char *data_start = mem + allocator_metadata_size;
        size_t *total_size = reinterpret_cast<size_t *>(mem + sizeof(MemResPtr) + sizeof(FitMode));
        
        if (*total_size > 0) {
            _occupied_ptr = data_start;
            _trusted_memory = trusted;
        }
    }
}
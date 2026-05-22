#include <cstring>
#include <new>
#include <typeinfo>
#include "../include/allocator_buddies_system.h"

std::uint8_t *byte_ptr(void *ptr) noexcept {
    return reinterpret_cast<std::uint8_t *>(ptr);
}

const std::uint8_t *byte_ptr(const void *ptr) noexcept {
    return reinterpret_cast<const std::uint8_t *>(ptr);
}

struct _block_header {
    bool occupied : 1;
    unsigned char size : 7;
};

_block_header *header(void *block) noexcept {
    return reinterpret_cast<_block_header *>(block);
}

unsigned char get_k(void *block) noexcept {
    return static_cast<unsigned char>(header(block)->size);
}

bool get_occupied(void *block) noexcept {
    return header(block)->occupied;
}

void set_k(void *block, unsigned char k) noexcept {
    header(block)->size = k;
}

void set_occupied(void *block, bool occ) noexcept {
    header(block)->occupied = occ;
}

size_t block_sz(void *block) noexcept {
    return 1ull << get_k(block);
}

void *&free_list_head(void *trusted) noexcept {
    return *reinterpret_cast<void **>(byte_ptr(trusted) + sizeof(std::pmr::memory_resource *) + sizeof(allocator_with_fit_mode::fit_mode) + sizeof(unsigned char) + sizeof(std::mutex));
}

void *&iter_current(void *trusted) noexcept {
    return *reinterpret_cast<void **>(byte_ptr(trusted) + sizeof(std::pmr::memory_resource *) + sizeof(allocator_with_fit_mode::fit_mode) + sizeof(unsigned char) + sizeof(std::mutex) + sizeof(void *));
}

std::uint8_t *arena_begin(void *trusted) noexcept {
    return byte_ptr(trusted) + sizeof(std::pmr::memory_resource *) + sizeof(allocator_with_fit_mode::fit_mode) + sizeof(unsigned char) + sizeof(std::mutex) + sizeof(void *) + sizeof(void *);
}

void *&parent_allocator_ref(void *trusted) noexcept {
    return *reinterpret_cast<void **>(byte_ptr(trusted) + 0);
}

allocator_with_fit_mode::fit_mode &fit_mode_ref(void *trusted) noexcept {
    return *reinterpret_cast<allocator_with_fit_mode::fit_mode *>(byte_ptr(trusted) + sizeof(std::pmr::memory_resource *));
}

unsigned char &max_k_ref(void *trusted) noexcept {
    return *reinterpret_cast<unsigned char *>(byte_ptr(trusted) + sizeof(std::pmr::memory_resource *) + sizeof(allocator_with_fit_mode::fit_mode));
}

std::mutex &mutex_ref(void *trusted) noexcept {
    return *reinterpret_cast<std::mutex *>(byte_ptr(trusted) + sizeof(std::pmr::memory_resource *) + sizeof(allocator_with_fit_mode::fit_mode) + sizeof(unsigned char));
}

size_t arena_sz(void *trusted) noexcept {
    return 1ull << max_k_ref(trusted);
}

std::uint8_t *arena_end(void *trusted) noexcept {
    return arena_begin(trusted) + arena_sz(trusted);
}

bool in_arena(void *trusted, void *block) noexcept {
    return byte_ptr(block) >= arena_begin(trusted) && byte_ptr(block) < arena_end(trusted);
}

void *buddy_address(void *trusted, void *block, unsigned char k) noexcept {
    std::uint8_t *arena_start = arena_begin(trusted);
    size_t offset = byte_ptr(block) - arena_start;
    size_t buddy_offs = offset ^ (1ull << k);
    return arena_start + buddy_offs;
}

void *&next_free_ref(void *block) noexcept {
    return *reinterpret_cast<void **>(byte_ptr(block) + 1);
}

static size_t _min_k() {
    struct _tmp { bool occ : 1; unsigned char sz : 7; };
    return __detail::nearest_greater_k_of_2(sizeof(_tmp) + sizeof(void *));
}

size_t ceil_k(size_t val) noexcept {
    size_t k = _min_k();
    while ((1ull << k) < val) {
        ++k;
    }
    return k;
}

void add_free(void *trusted, void *block) noexcept {
    next_free_ref(block) = free_list_head(trusted);
    free_list_head(trusted) = block;
}

void remove_free(void *trusted, void *block) noexcept {
    void *prev = nullptr;
    void *cur = free_list_head(trusted);

    while (cur) {
        if (cur == block) {
            if (prev) {
                next_free_ref(prev) = next_free_ref(cur);
            } else {
                free_list_head(trusted) = next_free_ref(cur);
            }
            return;
        }
        prev = cur;
        cur = next_free_ref(cur);
    }
}

void merge(void *trusted, void *block) {
    unsigned char k = get_k(block);
    unsigned char max_k = max_k_ref(trusted);

    while (k < max_k) {
        void *buddy = buddy_address(trusted, block, k);

        if (!in_arena(trusted, buddy)) {
            break;
        }

        if (get_occupied(buddy) || get_k(buddy) != k) {
            break;
        }

        remove_free(trusted, buddy);

        void *left = byte_ptr(block) < byte_ptr(buddy) ? block : buddy;

        set_k(left, k + 1);

        block = left;
        k = k + 1;
    }
}

void destroy_storage(void *&memory) noexcept {
    if (!memory) {
        return;
    }

    mutex_ref(memory).~mutex();

    auto *parent = reinterpret_cast<std::pmr::memory_resource *>(parent_allocator_ref(memory));
    size_t total = sizeof(std::pmr::memory_resource *) + sizeof(allocator_with_fit_mode::fit_mode) + sizeof(unsigned char) + sizeof(std::mutex) + sizeof(void *) + sizeof(void *) + arena_sz(memory);
    parent->deallocate(memory, total);

    memory = nullptr;
}

void *create_storage(size_t space_size,
                     std::pmr::memory_resource *parent,
                     allocator_with_fit_mode::fit_mode fit) {
    if (!parent) {
        parent = std::pmr::get_default_resource();
    }

    size_t min_block = 1ull << _min_k();

    if (space_size < min_block) {
        throw std::logic_error("space too small");
    }

    size_t k = ceil_k(space_size);
    size_t arena_s = 1ull << k;
    size_t meta_sz = sizeof(std::pmr::memory_resource *) + sizeof(allocator_with_fit_mode::fit_mode) + sizeof(unsigned char) + sizeof(std::mutex) + sizeof(void *) + sizeof(void *);
    size_t total = meta_sz + arena_s;

    auto *memory = parent->allocate(total);

    if (!memory) {
        return nullptr;
    }

    std::memset(memory, 0, total);

    parent_allocator_ref(memory) = parent;
    fit_mode_ref(memory) = fit;
    max_k_ref(memory) = static_cast<unsigned char>(k);
    new(&mutex_ref(memory)) std::mutex();

    free_list_head(memory) = nullptr;
    iter_current(memory) = nullptr;

    std::uint8_t *first = arena_begin(memory);
    set_k(first, static_cast<unsigned char>(k));
    set_occupied(first, false);
    next_free_ref(first) = nullptr;
    free_list_head(memory) = first;

    return memory;
}

allocator_buddies_system::allocator_buddies_system(size_t space_size,
                                                   std::pmr::memory_resource *parent_allocator,
                                                   allocator_with_fit_mode::fit_mode allocate_fit_mode)
    : _trusted_memory(create_storage(space_size, parent_allocator, allocate_fit_mode)) {}

allocator_buddies_system::allocator_buddies_system(const allocator_buddies_system &other)
    : _trusted_memory(nullptr) {
    if (!other._trusted_memory) {
        return;
    }

    auto *other_mem = const_cast<void *>(other._trusted_memory);

    std::lock_guard lock(mutex_ref(other_mem));

    _trusted_memory = create_storage(arena_sz(other_mem), reinterpret_cast<std::pmr::memory_resource *>(parent_allocator_ref(other_mem)), fit_mode_ref(other_mem));

    size_t ar_sz = arena_sz(other_mem);
    std::memcpy(arena_begin(_trusted_memory), arena_begin(other_mem), ar_sz);

    std::ptrdiff_t offset = arena_begin(_trusted_memory) - arena_begin(other_mem);

    void *other_head = free_list_head(other_mem);
    if (other_head) {
        free_list_head(_trusted_memory) = byte_ptr(other_head) + offset;
    }

    void *cur = free_list_head(_trusted_memory);
    while (cur) {
        void *orig_next = next_free_ref(byte_ptr(cur) - offset);
        next_free_ref(cur) = orig_next ? byte_ptr(orig_next) + offset : nullptr;
        cur = next_free_ref(cur);
    }
}

allocator_buddies_system::allocator_buddies_system(allocator_buddies_system &&other) noexcept
    : _trusted_memory(other._trusted_memory) {
    other._trusted_memory = nullptr;
}

allocator_buddies_system &allocator_buddies_system::operator=(const allocator_buddies_system &other) {
    if (this == &other) {
        return *this;
    }

    allocator_buddies_system copy(other);

    *this = std::move(copy);

    return *this;
}

allocator_buddies_system &allocator_buddies_system::operator=(allocator_buddies_system &&other) noexcept {
    if (this == &other) {
        return *this;
    }

    destroy_storage(_trusted_memory);

    _trusted_memory = other._trusted_memory;
    other._trusted_memory = nullptr;

    return *this;
}

allocator_buddies_system::~allocator_buddies_system() {
    destroy_storage(_trusted_memory);
}

[[nodiscard]] void *allocator_buddies_system::do_allocate_sm(size_t size) {
    std::lock_guard lock(mutex_ref(_trusted_memory));

    size_t needed = size + occupied_block_metadata_size;
    size_t need_k = ceil_k(needed);

    if (need_k > max_k_ref(_trusted_memory)) {
        throw std::bad_alloc();
    }

    void *best = nullptr;
    void *best_prev = nullptr;
    void *prev = nullptr;
    void *cur = free_list_head(_trusted_memory);

    while (cur) {
        unsigned char cur_k = get_k(cur);

        if (cur_k < need_k) {
            prev = cur;
            cur = next_free_ref(cur);
            continue;
        }

        bool is_better = false;

        is_better |= best == nullptr;
        is_better |= fit_mode_ref(_trusted_memory) == fit_mode::first_fit;
        is_better |= best && (fit_mode_ref(_trusted_memory) == fit_mode::the_best_fit && cur_k < get_k(best));
        is_better |= best && (fit_mode_ref(_trusted_memory) == fit_mode::the_worst_fit && cur_k > get_k(best));

        if (!is_better) {
            prev = cur;
            cur = next_free_ref(cur);
            continue;
        }

        best = cur;
        best_prev = prev;

        if (fit_mode_ref(_trusted_memory) == fit_mode::first_fit) {
            break;
        }

        prev = cur;
        cur = next_free_ref(cur);
    }

    if (!best) {
        throw std::bad_alloc();
    }

    unsigned char cur_k = get_k(best);

    while (cur_k > need_k) {
        void *left = best;
        void *right = byte_ptr(left) + (1ull << (cur_k - 1));

        remove_free(_trusted_memory, best);

        set_k(left, cur_k - 1);
        set_occupied(left, false);
        set_k(right, cur_k - 1);
        set_occupied(right, false);

        add_free(_trusted_memory, right);
        add_free(_trusted_memory, left);

        best = left;
        cur_k = cur_k - 1;
    }

    remove_free(_trusted_memory, best);

    set_occupied(best, true);

    return byte_ptr(best) + occupied_block_metadata_size;
}

void allocator_buddies_system::do_deallocate_sm(void *at) {
    if (!at) {
        return;
    }

    std::lock_guard lock(mutex_ref(_trusted_memory));

    void *block = byte_ptr(at) - occupied_block_metadata_size;

    if (!in_arena(_trusted_memory, block)) {
        return;
    }

    set_occupied(block, false);

    merge(_trusted_memory, block);

    add_free(_trusted_memory, block);
}

bool allocator_buddies_system::do_is_equal(const std::pmr::memory_resource &other) const noexcept {
    if (this == &other) {
        return true;
    }
    auto *casted = dynamic_cast<const allocator_buddies_system *>(&other);
    return casted && _trusted_memory == casted->_trusted_memory;
}

inline void allocator_buddies_system::set_fit_mode(allocator_with_fit_mode::fit_mode mode) {
    std::lock_guard lock(mutex_ref(_trusted_memory));

    fit_mode_ref(_trusted_memory) = mode;
}

std::vector<allocator_test_utils::block_info> allocator_buddies_system::get_blocks_info() const noexcept {
    std::lock_guard lock(mutex_ref(_trusted_memory));

    return get_blocks_info_inner();
}

std::vector<allocator_test_utils::block_info> allocator_buddies_system::get_blocks_info_inner() const {
    std::vector<allocator_test_utils::block_info> result;

    for (auto iterator = begin(); iterator != end(); ++iterator) {
        result.push_back({iterator.size(), iterator.occupied()});
    }

    return result;
}

allocator_buddies_system::buddy_iterator allocator_buddies_system::begin() const noexcept {
    return buddy_iterator(_trusted_memory);
}

allocator_buddies_system::buddy_iterator allocator_buddies_system::end() const noexcept {
    return {};
}

bool allocator_buddies_system::buddy_iterator::operator==(const buddy_iterator &other) const noexcept {
    return operator*() == other.operator*();
}

bool allocator_buddies_system::buddy_iterator::operator!=(const buddy_iterator &other) const noexcept {
    return !(*this == other);
}

allocator_buddies_system::buddy_iterator &allocator_buddies_system::buddy_iterator::operator++() & noexcept {
    if (!_block) {
        return *this;
    }

    void *&it_cur = iter_current(_block);
    void *cur = it_cur;

    if (!cur) {
        return *this;
    }

    size_t sz = block_sz(cur);

    cur = byte_ptr(cur) + sz;

    if (byte_ptr(cur) >= arena_end(_block)) {
        cur = nullptr;
    }

    it_cur = cur;

    return *this;
}

allocator_buddies_system::buddy_iterator allocator_buddies_system::buddy_iterator::operator++(int n) {
    auto copy = *this;

    ++(*this);

    return copy;
}

size_t allocator_buddies_system::buddy_iterator::size() const noexcept {
    if (!_block) {
        return 0;
    }

    void *cur = operator*();

    if (!cur) {
        return 0;
    }

    return block_sz(cur);
}

bool allocator_buddies_system::buddy_iterator::occupied() const noexcept {
    if (!_block) {
        return false;
    }

    void *cur = operator*();

    if (!cur) {
        return false;
    }

    return get_occupied(cur);
}

void *allocator_buddies_system::buddy_iterator::operator*() const noexcept {
    if (!_block) {
        return nullptr;
    }

    return iter_current(_block);
}

allocator_buddies_system::buddy_iterator::buddy_iterator(void *start)
    : _block(start) {
    if (start) {
        iter_current(start) = arena_begin(start);
    }
}

allocator_buddies_system::buddy_iterator::buddy_iterator()
    : _block(nullptr) {}

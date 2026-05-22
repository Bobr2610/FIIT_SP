#include <cstring>
#include <new>
#include <typeinfo>
#include "../include/allocator_red_black_tree.h"

std::uint8_t *byte_ptr(void *ptr) noexcept {
    return reinterpret_cast<std::uint8_t *>(ptr);
}

const std::uint8_t *byte_ptr(const void *ptr) noexcept {
    return reinterpret_cast<const std::uint8_t *>(ptr);
}

struct _block_data {
    bool occupied : 4;
    unsigned char color : 4;
};

_block_data *get_block_data(void *block) noexcept {
    return reinterpret_cast<_block_data *>(block);
}

constexpr unsigned char COLOR_RED = 0;
constexpr unsigned char COLOR_BLACK = 1;

bool is_red(void *block) noexcept {
    return block && get_block_data(block)->color == COLOR_RED;
}

bool is_black(void *block) noexcept {
    return !block || get_block_data(block)->color == COLOR_BLACK;
}

void set_red(void *block) noexcept {
    if (block) {
        get_block_data(block)->color = COLOR_RED;
    }
}

void set_black(void *block) noexcept {
    if (block) {
        get_block_data(block)->color = COLOR_BLACK;
    }
}

bool is_occupied(void *block) noexcept {
    return get_block_data(block)->occupied;
}

void set_occupied_flag(void *block, bool occ) noexcept {
    get_block_data(block)->occupied = occ;
}

void *&prev_block(void *block) noexcept {
    return *reinterpret_cast<void **>(byte_ptr(block) + 1);
}

void *&next_block(void *block) noexcept {
    return *reinterpret_cast<void **>(byte_ptr(block) + 1 + sizeof(void *));
}

void *&tree_parent(void *block) noexcept {
    return *reinterpret_cast<void **>(byte_ptr(block) + 1 + 2 * sizeof(void *));
}

void *&tree_left(void *block) noexcept {
    return *reinterpret_cast<void **>(byte_ptr(block) + 1 + 3 * sizeof(void *));
}

void *&tree_right(void *block) noexcept {
    return *reinterpret_cast<void **>(byte_ptr(block) + 1 + 4 * sizeof(void *));
}

size_t total_block_size(void *block, void *trusted) noexcept {
    void *nxt = next_block(block);
    if (nxt) {
        return byte_ptr(nxt) - byte_ptr(block);
    }

    auto *end = byte_ptr(trusted) + sizeof(std::pmr::memory_resource *) + sizeof(allocator_with_fit_mode::fit_mode) + sizeof(size_t) + sizeof(std::mutex) + sizeof(void *) + *reinterpret_cast<size_t *>(byte_ptr(trusted) + sizeof(std::pmr::memory_resource *) + sizeof(allocator_with_fit_mode::fit_mode));
    return end - byte_ptr(block);
}

void *&get_root(void *trusted) noexcept {
    return *reinterpret_cast<void **>(byte_ptr(trusted) + sizeof(std::pmr::memory_resource *) + sizeof(allocator_with_fit_mode::fit_mode) + sizeof(size_t) + sizeof(std::mutex));
}

std::pmr::memory_resource *&get_parent_alloc(void *trusted) noexcept {
    return *reinterpret_cast<std::pmr::memory_resource **>(byte_ptr(trusted) + 0);
}

allocator_with_fit_mode::fit_mode &get_fit_mode(void *trusted) noexcept {
    return *reinterpret_cast<allocator_with_fit_mode::fit_mode *>(byte_ptr(trusted) + sizeof(std::pmr::memory_resource *));
}

size_t &get_space_size(void *trusted) noexcept {
    return *reinterpret_cast<size_t *>(byte_ptr(trusted) + sizeof(std::pmr::memory_resource *) + sizeof(allocator_with_fit_mode::fit_mode));
}

std::mutex &get_mutex(void *trusted) noexcept {
    return *reinterpret_cast<std::mutex *>(byte_ptr(trusted) + sizeof(std::pmr::memory_resource *) + sizeof(allocator_with_fit_mode::fit_mode) + sizeof(size_t));
}

void *arena_begin(void *trusted) noexcept {
    return byte_ptr(trusted) + sizeof(std::pmr::memory_resource *) + sizeof(allocator_with_fit_mode::fit_mode) + sizeof(size_t) + sizeof(std::mutex) + sizeof(void *);
}

void *arena_end(void *trusted) noexcept {
    return byte_ptr(trusted) + sizeof(std::pmr::memory_resource *) + sizeof(allocator_with_fit_mode::fit_mode) + sizeof(size_t) + sizeof(std::mutex) + sizeof(void *) + get_space_size(trusted);
}

void destroy_storage(void *&memory) noexcept {
    if (!memory) {
        return;
    }

    get_mutex(memory).~mutex();

    auto *parent = get_parent_alloc(memory);
    size_t total = sizeof(std::pmr::memory_resource *) + sizeof(allocator_with_fit_mode::fit_mode) + sizeof(size_t) + sizeof(std::mutex) + sizeof(void *) + get_space_size(memory);
    parent->deallocate(memory, total);

    memory = nullptr;
}

void rb_insert(void *trusted, void *block);

void *create_storage(size_t space_size,
                     std::pmr::memory_resource *parent,
                     allocator_with_fit_mode::fit_mode fit) {
    if (!parent) {
        parent = std::pmr::get_default_resource();
    }

    size_t meta = sizeof(std::pmr::memory_resource *) + sizeof(allocator_with_fit_mode::fit_mode) + sizeof(size_t) + sizeof(std::mutex) + sizeof(void *);
    size_t total = meta + space_size;

    auto *memory = parent->allocate(total);

    if (!memory) {
        return nullptr;
    }

    std::memset(memory, 0, total);

    get_parent_alloc(memory) = parent;
    get_fit_mode(memory) = fit;
    get_space_size(memory) = space_size;
    new(&get_mutex(memory)) std::mutex();
    get_root(memory) = nullptr;

    void *first = arena_begin(memory);
    get_block_data(first)->occupied = false;
    get_block_data(first)->color = COLOR_BLACK;
    prev_block(first) = nullptr;
    next_block(first) = nullptr;

    rb_insert(memory, first);

    return memory;
}

void rotate_left(void *trusted, void *x) {
    void *y = tree_right(x);
    tree_right(x) = tree_left(y);
    if (tree_left(y)) {
        tree_parent(tree_left(y)) = x;
    }
    tree_parent(y) = tree_parent(x);
    if (!tree_parent(x)) {
        get_root(trusted) = y;
    } else if (x == tree_left(tree_parent(x))) {
        tree_left(tree_parent(x)) = y;
    } else {
        tree_right(tree_parent(x)) = y;
    }
    tree_left(y) = x;
    tree_parent(x) = y;
}

void rotate_right(void *trusted, void *x) {
    void *y = tree_left(x);
    tree_left(x) = tree_right(y);
    if (tree_right(y)) {
        tree_parent(tree_right(y)) = x;
    }
    tree_parent(y) = tree_parent(x);
    if (!tree_parent(x)) {
        get_root(trusted) = y;
    } else if (x == tree_right(tree_parent(x))) {
        tree_right(tree_parent(x)) = y;
    } else {
        tree_left(tree_parent(x)) = y;
    }
    tree_right(y) = x;
    tree_parent(x) = y;
}

void rb_insert_fixup(void *trusted, void *z) {
    while (is_red(tree_parent(z))) {
        if (tree_parent(z) == tree_left(tree_parent(tree_parent(z)))) {
            void *y = tree_right(tree_parent(tree_parent(z)));
            if (is_red(y)) {
                set_black(tree_parent(z));
                set_black(y);
                set_red(tree_parent(tree_parent(z)));
                z = tree_parent(tree_parent(z));
            } else {
                if (z == tree_right(tree_parent(z))) {
                    z = tree_parent(z);
                    rotate_left(trusted, z);
                }
                set_black(tree_parent(z));
                set_red(tree_parent(tree_parent(z)));
                rotate_right(trusted, tree_parent(tree_parent(z)));
            }
        } else {
            void *y = tree_left(tree_parent(tree_parent(z)));
            if (is_red(y)) {
                set_black(tree_parent(z));
                set_black(y);
                set_red(tree_parent(tree_parent(z)));
                z = tree_parent(tree_parent(z));
            } else {
                if (z == tree_left(tree_parent(z))) {
                    z = tree_parent(z);
                    rotate_right(trusted, z);
                }
                set_black(tree_parent(z));
                set_red(tree_parent(tree_parent(z)));
                rotate_left(trusted, tree_parent(tree_parent(z)));
            }
        }
    }
    set_black(get_root(trusted));
}

void rb_insert(void *trusted, void *block) {
    void *y = nullptr;
    void *x = get_root(trusted);
    size_t block_sz = total_block_size(block, trusted);

    while (x) {
        y = x;
        size_t x_sz = total_block_size(x, trusted);
        if (block_sz < x_sz || (block_sz == x_sz && byte_ptr(block) < byte_ptr(x))) {
            x = tree_left(x);
        } else {
            x = tree_right(x);
        }
    }

    tree_parent(block) = y;
    if (!y) {
        get_root(trusted) = block;
    } else {
        size_t y_sz = total_block_size(y, trusted);
        if (block_sz < y_sz || (block_sz == y_sz && byte_ptr(block) < byte_ptr(y))) {
            tree_left(y) = block;
        } else {
            tree_right(y) = block;
        }
    }

    tree_left(block) = nullptr;
    tree_right(block) = nullptr;
    set_red(block);

    rb_insert_fixup(trusted, block);
}

void rb_transplant(void *trusted, void *u, void *v) {
    if (!tree_parent(u)) {
        get_root(trusted) = v;
    } else if (u == tree_left(tree_parent(u))) {
        tree_left(tree_parent(u)) = v;
    } else {
        tree_right(tree_parent(u)) = v;
    }
    if (v) {
        tree_parent(v) = tree_parent(u);
    }
}

void *rb_minimum(void *node) noexcept {
    while (node && tree_left(node)) {
        node = tree_left(node);
    }
    return node;
}

void rb_delete_fixup(void *trusted, void *x) {
    while (x != get_root(trusted) && is_black(x)) {
        if (x == tree_left(tree_parent(x))) {
            void *w = tree_right(tree_parent(x));
            if (is_red(w)) {
                set_black(w);
                set_red(tree_parent(x));
                rotate_left(trusted, tree_parent(x));
                w = tree_right(tree_parent(x));
            }
            if (is_black(tree_left(w)) && is_black(tree_right(w))) {
                set_red(w);
                x = tree_parent(x);
            } else {
                if (is_black(tree_right(w))) {
                    set_black(tree_left(w));
                    set_red(w);
                    rotate_right(trusted, w);
                    w = tree_right(tree_parent(x));
                }
                get_block_data(w)->color = get_block_data(tree_parent(x))->color;
                set_black(tree_parent(x));
                set_black(tree_right(w));
                rotate_left(trusted, tree_parent(x));
                x = get_root(trusted);
            }
        } else {
            void *w = tree_left(tree_parent(x));
            if (is_red(w)) {
                set_black(w);
                set_red(tree_parent(x));
                rotate_right(trusted, tree_parent(x));
                w = tree_left(tree_parent(x));
            }
            if (is_black(tree_right(w)) && is_black(tree_left(w))) {
                set_red(w);
                x = tree_parent(x);
            } else {
                if (is_black(tree_left(w))) {
                    set_black(tree_right(w));
                    set_red(w);
                    rotate_left(trusted, w);
                    w = tree_left(tree_parent(x));
                }
                get_block_data(w)->color = get_block_data(tree_parent(x))->color;
                set_black(tree_parent(x));
                set_black(tree_left(w));
                rotate_right(trusted, tree_parent(x));
                x = get_root(trusted);
            }
        }
    }
    set_black(x);
}

void rb_remove(void *trusted, void *block) {
    void *y = block;
    unsigned char y_original_color = get_block_data(y)->color;
    void *x;

    if (!tree_left(block)) {
        x = tree_right(block);
        rb_transplant(trusted, block, tree_right(block));
    } else if (!tree_right(block)) {
        x = tree_left(block);
        rb_transplant(trusted, block, tree_left(block));
    } else {
        y = rb_minimum(tree_right(block));
        y_original_color = get_block_data(y)->color;
        x = tree_right(y);

        if (tree_parent(y) == block) {
            if (x) {
                tree_parent(x) = y;
            }
        } else {
            rb_transplant(trusted, y, tree_right(y));
            tree_right(y) = tree_right(block);
            tree_parent(tree_right(y)) = y;
        }

        rb_transplant(trusted, block, y);
        tree_left(y) = tree_left(block);
        tree_parent(tree_left(y)) = y;
        get_block_data(y)->color = get_block_data(block)->color;
    }

    if (y_original_color == COLOR_BLACK && x) {
        rb_delete_fixup(trusted, x);
    }
}

void insert_after(void *block, void *new_block) noexcept {
    void *nxt = next_block(block);

    next_block(block) = new_block;
    prev_block(new_block) = block;
    next_block(new_block) = nxt;

    if (nxt) {
        prev_block(nxt) = new_block;
    }
}

allocator_red_black_tree::allocator_red_black_tree(size_t space_size,
                                                   std::pmr::memory_resource *parent_allocator,
                                                   allocator_with_fit_mode::fit_mode allocate_fit_mode)
    : _trusted_memory(create_storage(space_size, parent_allocator, allocate_fit_mode)) {}

allocator_red_black_tree::allocator_red_black_tree(const allocator_red_black_tree &other)
    : _trusted_memory(nullptr) {
    if (!other._trusted_memory) {
        return;
    }

    auto *other_mem = const_cast<void *>(other._trusted_memory);

    std::lock_guard lock(get_mutex(other_mem));

    _trusted_memory = create_storage(get_space_size(other_mem), get_parent_alloc(other_mem), get_fit_mode(other_mem));

    size_t sp_size = get_space_size(other_mem);
    void *src_arena = arena_begin(other_mem);
    void *dst_arena = arena_begin(_trusted_memory);

    std::memcpy(dst_arena, src_arena, sp_size);

    std::ptrdiff_t offset = byte_ptr(dst_arena) - byte_ptr(src_arena);

    for (auto iterator = begin(); iterator != end(); ++iterator) {
        void *block = *iterator;

        if (prev_block(block)) {
            prev_block(block) = byte_ptr(prev_block(block)) + offset;
        }
        if (next_block(block)) {
            next_block(block) = byte_ptr(next_block(block)) + offset;
        }

        if (!is_occupied(block)) {
            if (tree_parent(block)) {
                tree_parent(block) = byte_ptr(tree_parent(block)) + offset;
            }
            if (tree_left(block)) {
                tree_left(block) = byte_ptr(tree_left(block)) + offset;
            }
            if (tree_right(block)) {
                tree_right(block) = byte_ptr(tree_right(block)) + offset;
            }
        }
    }

    if (get_root(other_mem)) {
        get_root(_trusted_memory) = byte_ptr(get_root(other_mem)) + offset;
    }
}

allocator_red_black_tree::allocator_red_black_tree(allocator_red_black_tree &&other) noexcept
    : _trusted_memory(other._trusted_memory) {
    other._trusted_memory = nullptr;
}

allocator_red_black_tree &allocator_red_black_tree::operator=(const allocator_red_black_tree &other) {
    if (this == &other) {
        return *this;
    }

    allocator_red_black_tree copy(other);

    *this = std::move(copy);

    return *this;
}

allocator_red_black_tree &allocator_red_black_tree::operator=(allocator_red_black_tree &&other) noexcept {
    if (this == &other) {
        return *this;
    }

    destroy_storage(_trusted_memory);

    _trusted_memory = other._trusted_memory;
    other._trusted_memory = nullptr;

    return *this;
}

allocator_red_black_tree::~allocator_red_black_tree() {
    destroy_storage(_trusted_memory);
}

[[nodiscard]] void *allocator_red_black_tree::do_allocate_sm(size_t size) {
    std::lock_guard lock(get_mutex(_trusted_memory));

    size_t needed_total = size + occupied_block_metadata_size;

    void *selected = nullptr;

    void *cur = get_root(_trusted_memory);
    while (cur) {
        size_t cur_sz = total_block_size(cur, _trusted_memory);

        if (cur_sz >= needed_total) {
            selected = cur;
            cur = tree_left(cur);
        } else {
            cur = tree_right(cur);
        }
    }

    if (!selected) {
        throw std::bad_alloc();
    }

    if (get_fit_mode(_trusted_memory) == fit_mode::the_best_fit) {
        cur = tree_right(selected);
        while (cur) {
            size_t cur_sz = total_block_size(cur, _trusted_memory);
            if (cur_sz >= needed_total) {
                selected = cur;
                cur = tree_left(cur);
            } else {
                cur = tree_right(cur);
            }
        }
    } else if (get_fit_mode(_trusted_memory) == fit_mode::the_worst_fit) {
        void *largest = selected;
        cur = get_root(_trusted_memory);
        while (cur) {
            size_t cur_sz = total_block_size(cur, _trusted_memory);
            if (cur_sz >= needed_total) {
                if (cur_sz > total_block_size(largest, _trusted_memory)) {
                    largest = cur;
                }
            }
            cur = tree_right(cur);
        }
        if (total_block_size(largest, _trusted_memory) > total_block_size(selected, _trusted_memory)) {
            selected = largest;
        }
    }

    rb_remove(_trusted_memory, selected);

    size_t selected_sz = total_block_size(selected, _trusted_memory);

    if (selected_sz >= needed_total + occupied_block_metadata_size + 1) {
        void *remaining = byte_ptr(selected) + needed_total;
        get_block_data(remaining)->occupied = false;
        get_block_data(remaining)->color = COLOR_BLACK;

        void *nxt = next_block(selected);
        prev_block(remaining) = selected;
        next_block(remaining) = nxt;
        next_block(selected) = remaining;
        if (nxt) {
            prev_block(nxt) = remaining;
        }

        rb_insert(_trusted_memory, remaining);
    }

    set_occupied_flag(selected, true);

    return byte_ptr(selected) + occupied_block_metadata_size;
}

void allocator_red_black_tree::do_deallocate_sm(void *at) {
    if (!at) {
        return;
    }

    std::lock_guard lock(get_mutex(_trusted_memory));

    void *block = byte_ptr(at) - occupied_block_metadata_size;

    if (byte_ptr(block) < byte_ptr(arena_begin(_trusted_memory)) || byte_ptr(block) >= byte_ptr(arena_end(_trusted_memory))) {
        return;
    }

    set_occupied_flag(block, false);

    void *prv = prev_block(block);
    void *nxt = next_block(block);

    if (prv && !is_occupied(prv)) {
        rb_remove(_trusted_memory, prv);

        next_block(prv) = nxt;
        if (nxt) {
            prev_block(nxt) = prv;
        }

        block = prv;
    }

    if (nxt && !is_occupied(nxt)) {
        rb_remove(_trusted_memory, nxt);

        next_block(block) = next_block(nxt);
        if (next_block(nxt)) {
            prev_block(next_block(nxt)) = block;
        }
    }

    rb_insert(_trusted_memory, block);
}

bool allocator_red_black_tree::do_is_equal(const std::pmr::memory_resource &other) const noexcept {
    if (this == &other) {
        return true;
    }
    auto *casted = dynamic_cast<const allocator_red_black_tree *>(&other);
    return casted && _trusted_memory == casted->_trusted_memory;
}

inline void allocator_red_black_tree::set_fit_mode(allocator_with_fit_mode::fit_mode mode) {
    std::lock_guard lock(get_mutex(_trusted_memory));

    get_fit_mode(_trusted_memory) = mode;
}

std::vector<allocator_test_utils::block_info> allocator_red_black_tree::get_blocks_info() const {
    std::lock_guard lock(get_mutex(_trusted_memory));

    return get_blocks_info_inner();
}

std::vector<allocator_test_utils::block_info> allocator_red_black_tree::get_blocks_info_inner() const {
    std::vector<allocator_test_utils::block_info> result;

    for (auto iterator = begin(); iterator != end(); ++iterator) {
        result.push_back({iterator.size(), iterator.occupied()});
    }

    return result;
}

allocator_red_black_tree::rb_iterator allocator_red_black_tree::begin() const noexcept {
    return rb_iterator(_trusted_memory);
}

allocator_red_black_tree::rb_iterator allocator_red_black_tree::end() const noexcept {
    return {};
}

bool allocator_red_black_tree::rb_iterator::operator==(const rb_iterator &other) const noexcept {
    return _block_ptr == other._block_ptr;
}

bool allocator_red_black_tree::rb_iterator::operator!=(const rb_iterator &other) const noexcept {
    return !(*this == other);
}

allocator_red_black_tree::rb_iterator &allocator_red_black_tree::rb_iterator::operator++() & noexcept {
    if (!_block_ptr) {
        return *this;
    }

    _block_ptr = next_block(_block_ptr);

    return *this;
}

allocator_red_black_tree::rb_iterator allocator_red_black_tree::rb_iterator::operator++(int n) {
    auto copy = *this;

    ++(*this);

    return copy;
}

size_t allocator_red_black_tree::rb_iterator::size() const noexcept {
    if (!_block_ptr || !_trusted) {
        return 0;
    }

    return total_block_size(_block_ptr, _trusted);
}

void *allocator_red_black_tree::rb_iterator::operator*() const noexcept {
    return _block_ptr;
}

allocator_red_black_tree::rb_iterator::rb_iterator()
    : _block_ptr(nullptr), _trusted(nullptr) {}

allocator_red_black_tree::rb_iterator::rb_iterator(void *trusted)
    : _block_ptr(nullptr), _trusted(trusted) {
    if (trusted) {
        _block_ptr = arena_begin(trusted);
    }
}

bool allocator_red_black_tree::rb_iterator::occupied() const noexcept {
    if (!_block_ptr) {
        return false;
    }

    return is_occupied(_block_ptr);
}

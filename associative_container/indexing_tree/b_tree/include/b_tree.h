#ifndef SYS_PROG_B_TREE_H
#define SYS_PROG_B_TREE_H

#include <iterator>
#include <utility>
#include <boost/container/static_vector.hpp>
#include <vector>
#include <stack>
#include <pp_allocator.h>
#include <associative_container.h>
#include <not_implemented.h>
#include <initializer_list>

template <typename tkey, typename tvalue, comparator<tkey> compare = std::less<tkey>, std::size_t t = 5>
class B_tree final : private compare // EBCO
{
public:

    using tree_data_type = std::pair<tkey, tvalue>;
    using tree_data_type_const = std::pair<const tkey, tvalue>;
    using value_type = tree_data_type_const;

    class exception : public std::logic_error
    {
    public:
        explicit exception(const std::string& message) : std::logic_error(message) {}
    };

    class out_of_range : public exception
    {
    public:
        explicit out_of_range(const std::string& message) : exception(message) {}
    };

private:

    static constexpr const size_t minimum_keys_in_node = t - 1;
    static constexpr const size_t maximum_keys_in_node = 2 * t - 1;

    // region comparators declaration

    inline bool compare_keys(const tkey& lhs, const tkey& rhs) const;
    inline bool compare_pairs(const tree_data_type& lhs, const tree_data_type& rhs) const;

    // endregion comparators declaration


    struct btree_node
    {
        boost::container::static_vector<tree_data_type, maximum_keys_in_node + 1> _keys;
        boost::container::static_vector<btree_node*, maximum_keys_in_node + 2> _pointers;
        btree_node() noexcept;
    };

    pp_allocator<value_type> _allocator;
    btree_node* _root;
    size_t _size;

    pp_allocator<value_type> get_allocator() const noexcept;

    bool key_equal(const tkey& lhs, const tkey& rhs) const;
    bool is_leaf(const btree_node* node) const noexcept;
    size_t find_key_index(const btree_node* node, const tkey& key) const;

    btree_node* clone_subtree(const btree_node* node);
    void destroy_subtree(btree_node* node) noexcept;

    void split_child(btree_node* parent, size_t index);
    void insert_nonfull(btree_node* node, tree_data_type&& data);

    tree_data_type get_predecessor(btree_node* node) const;
    tree_data_type get_successor(btree_node* node) const;
    void merge_children(btree_node* parent, size_t index);
    void borrow_from_prev(btree_node* parent, size_t index);
    void borrow_from_next(btree_node* parent, size_t index);
    bool remove_key(btree_node* node, const tkey& key);

public:

    // region constructors declaration

    explicit B_tree(const compare& cmp = compare(), pp_allocator<value_type> = pp_allocator<value_type>());

    explicit B_tree(pp_allocator<value_type> alloc, const compare& comp = compare());

    template<input_iterator_for_pair<tkey, tvalue> iterator>
    explicit B_tree(iterator begin, iterator end, const compare& cmp = compare(), pp_allocator<value_type> = pp_allocator<value_type>());

    B_tree(std::initializer_list<std::pair<tkey, tvalue>> data, const compare& cmp = compare(), pp_allocator<value_type> = pp_allocator<value_type>());

    // endregion constructors declaration

    // region five declaration

    B_tree(const B_tree& other);

    B_tree(B_tree&& other) noexcept;

    B_tree& operator=(const B_tree& other);

    B_tree& operator=(B_tree&& other) noexcept;

    ~B_tree() noexcept;

    // endregion five declaration

    // region iterators declaration

    class btree_iterator;
    class btree_reverse_iterator;
    class btree_const_iterator;
    class btree_const_reverse_iterator;

    class btree_iterator final
    {
        std::stack<std::pair<btree_node**, size_t>> _path;
        size_t _index;

    public:
        using value_type = tree_data_type_const;
        using reference = value_type&;
        using pointer = value_type*;
        using iterator_category = std::bidirectional_iterator_tag;
        using difference_type = ptrdiff_t;
        using self = btree_iterator;

        friend class B_tree;
        friend class btree_reverse_iterator;
        friend class btree_const_iterator;
        friend class btree_const_reverse_iterator;

        reference operator*() const noexcept;
        pointer operator->() const noexcept;

        self& operator++();
        self operator++(int);

        self& operator--();
        self operator--(int);

        bool operator==(const self& other) const noexcept;
        bool operator!=(const self& other) const noexcept;

        size_t depth() const noexcept;
        size_t current_node_keys_count() const noexcept;
        bool is_terminate_node() const noexcept;
        size_t index() const noexcept;

        explicit btree_iterator(const std::stack<std::pair<btree_node**, size_t>>& path = std::stack<std::pair<btree_node**, size_t>>(), size_t index = 0);

    };

    class btree_const_iterator final
    {
        std::stack<std::pair<btree_node* const*, size_t>> _path;
        size_t _index;

    public:

        using value_type = tree_data_type_const;
        using reference = const value_type&;
        using pointer = const value_type*;
        using iterator_category = std::bidirectional_iterator_tag;
        using difference_type = ptrdiff_t;
        using self = btree_const_iterator;

        friend class B_tree;
        friend class btree_reverse_iterator;
        friend class btree_iterator;
        friend class btree_const_reverse_iterator;

        btree_const_iterator(const btree_iterator& it) noexcept;

        reference operator*() const noexcept;
        pointer operator->() const noexcept;

        self& operator++();
        self operator++(int);

        self& operator--();
        self operator--(int);

        bool operator==(const self& other) const noexcept;
        bool operator!=(const self& other) const noexcept;

        size_t depth() const noexcept;
        size_t current_node_keys_count() const noexcept;
        bool is_terminate_node() const noexcept;
        size_t index() const noexcept;

        explicit btree_const_iterator(const std::stack<std::pair<btree_node* const*, size_t>>& path = std::stack<std::pair<btree_node* const*, size_t>>(), size_t index = 0);
    };

    class btree_reverse_iterator final
    {
        std::stack<std::pair<btree_node**, size_t>> _path;
        size_t _index;

    public:

        using value_type = tree_data_type_const;
        using reference = value_type&;
        using pointer = value_type*;
        using iterator_category = std::bidirectional_iterator_tag;
        using difference_type = ptrdiff_t;
        using self = btree_reverse_iterator;

        friend class B_tree;
        friend class btree_iterator;
        friend class btree_const_iterator;
        friend class btree_const_reverse_iterator;

        btree_reverse_iterator(const btree_iterator& it) noexcept;
        operator btree_iterator() const noexcept;

        reference operator*() const noexcept;
        pointer operator->() const noexcept;

        self& operator++();
        self operator++(int);

        self& operator--();
        self operator--(int);

        bool operator==(const self& other) const noexcept;
        bool operator!=(const self& other) const noexcept;

        size_t depth() const noexcept;
        size_t current_node_keys_count() const noexcept;
        bool is_terminate_node() const noexcept;
        size_t index() const noexcept;

        explicit btree_reverse_iterator(const std::stack<std::pair<btree_node**, size_t>>& path = std::stack<std::pair<btree_node**, size_t>>(), size_t index = 0);
    };

    class btree_const_reverse_iterator final
    {
        std::stack<std::pair<btree_node* const*, size_t>> _path;
        size_t _index;

    public:

        using value_type = tree_data_type_const;
        using reference = const value_type&;
        using pointer = const value_type*;
        using iterator_category = std::bidirectional_iterator_tag;
        using difference_type = ptrdiff_t;
        using self = btree_const_reverse_iterator;

        friend class B_tree;
        friend class btree_reverse_iterator;
        friend class btree_const_iterator;
        friend class btree_iterator;

        btree_const_reverse_iterator(const btree_reverse_iterator& it) noexcept;
        operator btree_const_iterator() const noexcept;

        reference operator*() const noexcept;
        pointer operator->() const noexcept;

        self& operator++();
        self operator++(int);

        self& operator--();
        self operator--(int);

        bool operator==(const self& other) const noexcept;
        bool operator!=(const self& other) const noexcept;

        size_t depth() const noexcept;
        size_t current_node_keys_count() const noexcept;
        bool is_terminate_node() const noexcept;
        size_t index() const noexcept;

        explicit btree_const_reverse_iterator(const std::stack<std::pair<btree_node* const*, size_t>>& path = std::stack<std::pair<btree_node* const*, size_t>>(), size_t index = 0);
    };

    friend class btree_iterator;
    friend class btree_const_iterator;
    friend class btree_reverse_iterator;
    friend class btree_const_reverse_iterator;

    // endregion iterators declaration

    // region element access declaration

    /*
     * Returns a reference to the mapped value of the element with specified key. If no such element exists, an exception of type std::out_of_range is thrown.
     */
    tvalue& at(const tkey&);
    const tvalue& at(const tkey&) const;

    /*
     * If key not exists, makes default initialization of value
     */
    tvalue& operator[](const tkey& key);
    tvalue& operator[](tkey&& key);

    // endregion element access declaration
    // region iterator begins declaration

    btree_iterator begin();
    btree_iterator end();

    btree_const_iterator begin() const;
    btree_const_iterator end() const;

    btree_const_iterator cbegin() const;
    btree_const_iterator cend() const;

    btree_reverse_iterator rbegin();
    btree_reverse_iterator rend();

    btree_const_reverse_iterator rbegin() const;
    btree_const_reverse_iterator rend() const;

    btree_const_reverse_iterator crbegin() const;
    btree_const_reverse_iterator crend() const;

    // endregion iterator begins declaration

    // region lookup declaration

    size_t size() const noexcept;
    bool empty() const noexcept;

    /*
     * Returns end() if not exist
     */

    btree_iterator find(const tkey& key);
    btree_const_iterator find(const tkey& key) const;

    btree_iterator lower_bound(const tkey& key);
    btree_const_iterator lower_bound(const tkey& key) const;

    btree_iterator upper_bound(const tkey& key);
    btree_const_iterator upper_bound(const tkey& key) const;

    bool contains(const tkey& key) const;

    // endregion lookup declaration

    // region modifiers declaration

    void clear() noexcept;

    /*
     * Does nothing if key exists, delegates to emplace.
     * Second return value is true, when inserted
     */
    std::pair<btree_iterator, bool> insert(const tree_data_type& data);
    std::pair<btree_iterator, bool> insert(tree_data_type&& data);

    template <typename ...Args>
    std::pair<btree_iterator, bool> emplace(Args&&... args);

    /*
     * Updates value if key exists, delegates to emplace.
     */
    btree_iterator insert_or_assign(const tree_data_type& data);
    btree_iterator insert_or_assign(tree_data_type&& data);

    template <typename ...Args>
    btree_iterator emplace_or_assign(Args&&... args);

    /*
     * Return iterator to node next ro removed or end() if key not exists
     */
    btree_iterator erase(btree_iterator pos);
    btree_iterator erase(btree_const_iterator pos);

    btree_iterator erase(btree_iterator beg, btree_iterator en);
    btree_iterator erase(btree_const_iterator beg, btree_const_iterator en);


    btree_iterator erase(const tkey& key);

    // endregion modifiers declaration
};

template<std::input_iterator iterator, comparator<typename std::iterator_traits<iterator>::value_type::first_type> compare = std::less<typename std::iterator_traits<iterator>::value_type::first_type>,
        std::size_t t = 5, typename U>
B_tree(iterator begin, iterator end, const compare &cmp = compare(), pp_allocator<U> = pp_allocator<U>()) -> B_tree<typename std::iterator_traits<iterator>::value_type::first_type, typename std::iterator_traits<iterator>::value_type::second_type, compare, t>;

template<typename tkey, typename tvalue, comparator<tkey> compare = std::less<tkey>, std::size_t t = 5, typename U>
B_tree(std::initializer_list<std::pair<tkey, tvalue>> data, const compare &cmp = compare(), pp_allocator<U> = pp_allocator<U>()) -> B_tree<tkey, tvalue, compare, t>;

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
bool B_tree<tkey, tvalue, compare, t>::compare_pairs(const B_tree::tree_data_type &lhs,
                                                     const B_tree::tree_data_type &rhs) const
{
    return compare_keys(lhs.first, rhs.first);
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
bool B_tree<tkey, tvalue, compare, t>::compare_keys(const tkey &lhs, const tkey &rhs) const
{
    return compare::operator()(lhs, rhs);
}


template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
B_tree<tkey, tvalue, compare, t>::btree_node::btree_node() noexcept
{
    _keys.clear();
    _pointers.clear();
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
pp_allocator<typename B_tree<tkey, tvalue, compare, t>::value_type> B_tree<tkey, tvalue, compare, t>::get_allocator() const noexcept
{
    return _allocator;
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
bool B_tree<tkey, tvalue, compare, t>::key_equal(const tkey& lhs, const tkey& rhs) const
{
    return !compare_keys(lhs, rhs) && !compare_keys(rhs, lhs);
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
bool B_tree<tkey, tvalue, compare, t>::is_leaf(const btree_node* node) const noexcept
{
    return node == nullptr || node->_pointers.empty();
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
size_t B_tree<tkey, tvalue, compare, t>::find_key_index(const btree_node* node, const tkey& key) const
{
    size_t idx = 0;
    while (idx < node->_keys.size() && compare_keys(node->_keys[idx].first, key)) {
        ++idx;
    }
    return idx;
}

// region constructors implementation

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
B_tree<tkey, tvalue, compare, t>::B_tree(
        const compare& cmp,
        pp_allocator<value_type> alloc)
    : compare(cmp), _allocator(alloc), _root(nullptr), _size(0)
{
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
B_tree<tkey, tvalue, compare, t>::B_tree(
        pp_allocator<value_type> alloc,\
        const compare& comp)
    : compare(comp), _allocator(alloc), _root(nullptr), _size(0)
{
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
template<input_iterator_for_pair<tkey, tvalue> iterator>
B_tree<tkey, tvalue, compare, t>::B_tree(
        iterator begin,
        iterator end,
        const compare& cmp,
        pp_allocator<value_type> alloc)
    : compare(cmp), _allocator(alloc), _root(nullptr), _size(0)
{
    for (auto it = begin; it != end; ++it) {
        emplace(it->first, it->second);
    }
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
B_tree<tkey, tvalue, compare, t>::B_tree(
        std::initializer_list<std::pair<tkey, tvalue>> data,
        const compare& cmp,
        pp_allocator<value_type> alloc)
    : compare(cmp), _allocator(alloc), _root(nullptr), _size(0)
{
    for (const auto& item : data) {
        emplace(item.first, item.second);
    }
}

// endregion constructors implementation

// region five implementation

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
B_tree<tkey, tvalue, compare, t>::~B_tree() noexcept
{
    clear();
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
B_tree<tkey, tvalue, compare, t>::B_tree(const B_tree& other)
    : compare(static_cast<const compare&>(other)), _allocator(other._allocator), _root(nullptr), _size(other._size)
{
    _root = clone_subtree(other._root);
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
B_tree<tkey, tvalue, compare, t>& B_tree<tkey, tvalue, compare, t>::operator=(const B_tree& other)
{
    if (this == &other) {
        return *this;
    }

    clear();
    static_cast<compare&>(*this) = static_cast<const compare&>(other);
    _allocator = other._allocator;
    _size = other._size;
    _root = clone_subtree(other._root);
    return *this;
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
B_tree<tkey, tvalue, compare, t>::B_tree(B_tree&& other) noexcept
    : compare(static_cast<compare&&>(other)), _allocator(std::move(other._allocator)), _root(other._root), _size(other._size)
{
    other._root = nullptr;
    other._size = 0;
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
B_tree<tkey, tvalue, compare, t>& B_tree<tkey, tvalue, compare, t>::operator=(B_tree&& other) noexcept
{
    if (this == &other) {
        return *this;
    }

    clear();
    static_cast<compare&>(*this) = static_cast<compare&&>(other);
    _allocator = std::move(other._allocator);
    _root = other._root;
    _size = other._size;

    other._root = nullptr;
    other._size = 0;
    return *this;
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
typename B_tree<tkey, tvalue, compare, t>::btree_node*
B_tree<tkey, tvalue, compare, t>::clone_subtree(const btree_node* node)
{
    if (node == nullptr) {
        return nullptr;
    }

    btree_node* copy = _allocator.template new_object<btree_node>();
    copy->_keys = node->_keys;
    copy->_pointers.clear();

    for (size_t i = 0; i < node->_pointers.size(); ++i) {
        copy->_pointers.push_back(clone_subtree(node->_pointers[i]));
    }

    return copy;
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
void B_tree<tkey, tvalue, compare, t>::destroy_subtree(btree_node* node) noexcept
{
    if (node == nullptr) {
        return;
    }

    for (auto* child : node->_pointers) {
        destroy_subtree(child);
    }

    _allocator.template delete_object(node);
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
void B_tree<tkey, tvalue, compare, t>::split_child(btree_node* parent, size_t index)
{
    btree_node* full = parent->_pointers[index];
    btree_node* right = _allocator.template new_object<btree_node>();

    const size_t middle = t - 1;

    for (size_t i = middle + 1; i < full->_keys.size(); ++i) {
        right->_keys.push_back(full->_keys[i]);
    }

    if (!is_leaf(full)) {
        for (size_t i = middle + 1; i < full->_pointers.size(); ++i) {
            right->_pointers.push_back(full->_pointers[i]);
        }
    }

    tree_data_type middle_key = full->_keys[middle];

    full->_keys.resize(middle);
    if (!is_leaf(full)) {
        full->_pointers.resize(middle + 1);
    }

    parent->_keys.insert(parent->_keys.begin() + static_cast<long long>(index), middle_key);
    parent->_pointers.insert(parent->_pointers.begin() + static_cast<long long>(index + 1), right);
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
void B_tree<tkey, tvalue, compare, t>::insert_nonfull(btree_node* node, tree_data_type&& data)
{
    size_t i = node->_keys.size();

    if (is_leaf(node)) {
        while (i > 0 && compare_keys(data.first, node->_keys[i - 1].first)) {
            --i;
        }
        node->_keys.insert(node->_keys.begin() + static_cast<long long>(i), std::move(data));
        return;
    }

    while (i > 0 && compare_keys(data.first, node->_keys[i - 1].first)) {
        --i;
    }

    insert_nonfull(node->_pointers[i], std::move(data));

    if (node->_pointers[i]->_keys.size() > maximum_keys_in_node) {
        split_child(node, i);
        if (compare_keys(node->_keys[i].first, data.first)) {
            ++i;
        }
    }
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
typename B_tree<tkey, tvalue, compare, t>::tree_data_type
B_tree<tkey, tvalue, compare, t>::get_predecessor(btree_node* node) const
{
    btree_node* current = node;
    while (!is_leaf(current)) {
        current = current->_pointers.back();
    }
    return current->_keys.back();
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
typename B_tree<tkey, tvalue, compare, t>::tree_data_type
B_tree<tkey, tvalue, compare, t>::get_successor(btree_node* node) const
{
    btree_node* current = node;
    while (!is_leaf(current)) {
        current = current->_pointers.front();
    }
    return current->_keys.front();
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
void B_tree<tkey, tvalue, compare, t>::merge_children(btree_node* parent, size_t index)
{
    btree_node* left = parent->_pointers[index];
    btree_node* right = parent->_pointers[index + 1];

    left->_keys.push_back(parent->_keys[index]);
    for (auto& key : right->_keys) {
        left->_keys.push_back(std::move(key));
    }

    if (!is_leaf(right)) {
        for (auto* child : right->_pointers) {
            left->_pointers.push_back(child);
        }
    }

    parent->_keys.erase(parent->_keys.begin() + static_cast<long long>(index));
    parent->_pointers.erase(parent->_pointers.begin() + static_cast<long long>(index + 1));

    _allocator.template delete_object(right);
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
void B_tree<tkey, tvalue, compare, t>::borrow_from_prev(btree_node* parent, size_t index)
{
    btree_node* child = parent->_pointers[index];
    btree_node* left = parent->_pointers[index - 1];

    child->_keys.insert(child->_keys.begin(), parent->_keys[index - 1]);
    parent->_keys[index - 1] = left->_keys.back();
    left->_keys.pop_back();

    if (!is_leaf(left)) {
        child->_pointers.insert(child->_pointers.begin(), left->_pointers.back());
        left->_pointers.pop_back();
    }
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
void B_tree<tkey, tvalue, compare, t>::borrow_from_next(btree_node* parent, size_t index)
{
    btree_node* child = parent->_pointers[index];
    btree_node* right = parent->_pointers[index + 1];

    child->_keys.push_back(parent->_keys[index]);
    parent->_keys[index] = right->_keys.front();
    right->_keys.erase(right->_keys.begin());

    if (!is_leaf(right)) {
        child->_pointers.push_back(right->_pointers.front());
        right->_pointers.erase(right->_pointers.begin());
    }
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
bool B_tree<tkey, tvalue, compare, t>::remove_key(btree_node* node, const tkey& key)
{
    if (node == nullptr) {
        return false;
    }

    size_t idx = find_key_index(node, key);

    if (idx < node->_keys.size() && key_equal(node->_keys[idx].first, key)) {
        if (is_leaf(node)) {
            node->_keys.erase(node->_keys.begin() + static_cast<long long>(idx));
            return true;
        }

        if (node->_pointers[idx]->_keys.size() >= t) {
            tree_data_type pred = get_predecessor(node->_pointers[idx]);
            node->_keys[idx] = pred;
            return remove_key(node->_pointers[idx], pred.first);
        }

        if (node->_pointers[idx + 1]->_keys.size() >= t) {
            tree_data_type succ = get_successor(node->_pointers[idx + 1]);
            node->_keys[idx] = succ;
            return remove_key(node->_pointers[idx + 1], succ.first);
        }

        merge_children(node, idx);
        return remove_key(node->_pointers[idx], key);
    }

    if (is_leaf(node)) {
        return false;
    }

    bool last = (idx == node->_keys.size());
    btree_node* child = node->_pointers[idx];

    if (child->_keys.size() < t) {
        if (idx > 0 && node->_pointers[idx - 1]->_keys.size() >= t) {
            borrow_from_prev(node, idx);
        } else if (idx < node->_pointers.size() - 1 && node->_pointers[idx + 1]->_keys.size() >= t) {
            borrow_from_next(node, idx);
        } else {
            if (idx < node->_pointers.size() - 1) {
                merge_children(node, idx);
            } else {
                merge_children(node, idx - 1);
                child = node->_pointers[idx - 1];
            }
        }
    }

    if (last && idx > node->_keys.size()) {
        return remove_key(node->_pointers[idx - 1], key);
    }

    return remove_key(node->_pointers[idx], key);
}

// endregion five implementation

// region iterators implementation

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
B_tree<tkey, tvalue, compare, t>::btree_iterator::btree_iterator(
        const std::stack<std::pair<btree_node**, size_t>>& path, size_t index)
    : _path(path), _index(index)
{
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
typename B_tree<tkey, tvalue, compare, t>::btree_iterator::reference
B_tree<tkey, tvalue, compare, t>::btree_iterator::operator*() const noexcept
{
    btree_node* node = *_path.top().first;
    return *reinterpret_cast<value_type*>(&node->_keys[_index]);
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
typename B_tree<tkey, tvalue, compare, t>::btree_iterator::pointer
B_tree<tkey, tvalue, compare, t>::btree_iterator::operator->() const noexcept
{
    btree_node* node = *_path.top().first;
    return reinterpret_cast<value_type*>(&node->_keys[_index]);
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
typename B_tree<tkey, tvalue, compare, t>::btree_iterator&
B_tree<tkey, tvalue, compare, t>::btree_iterator::operator++()
{
    if (_path.empty()) {
        return *this;
    }

    btree_node* node = *_path.top().first;

    if (!node->_pointers.empty()) {
        size_t next_child_index = _index + 1;
        if (next_child_index < node->_pointers.size()) {
            btree_node* child = node->_pointers[next_child_index];
            _path.push({&node->_pointers[next_child_index], next_child_index});
            while (!child->_pointers.empty()) {
                _path.push({&child->_pointers[0], 0});
                child = child->_pointers[0];
            }
            _index = 0;
            return *this;
        }
    }

    if (_index + 1 < node->_keys.size()) {
        ++_index;
        return *this;
    }

    while (_path.size() > 1) {
        auto current = _path.top();
        _path.pop();
        auto parent = _path.top();
        btree_node* parent_node = *parent.first;
        size_t child_index = current.second;
        if (child_index < parent_node->_keys.size()) {
            _index = child_index;
            return *this;
        }
    }

    _path = std::stack<std::pair<btree_node**, size_t>>();
    _index = 0;
    return *this;
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
typename B_tree<tkey, tvalue, compare, t>::btree_iterator
B_tree<tkey, tvalue, compare, t>::btree_iterator::operator++(int)
{
    self temp = *this;
    ++(*this);
    return temp;
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
typename B_tree<tkey, tvalue, compare, t>::btree_iterator&
B_tree<tkey, tvalue, compare, t>::btree_iterator::operator--()
{
    if (_path.empty()) {
        return *this;
    }

    btree_node* node = *_path.top().first;

    if (!node->_pointers.empty()) {
        btree_node* child = node->_pointers[_index];
        _path.push({&node->_pointers[_index], _index});
        while (!child->_pointers.empty()) {
            size_t idx = child->_pointers.size() - 1;
            _path.push({&child->_pointers[idx], idx});
            child = child->_pointers[idx];
        }
        _index = child->_keys.size() - 1;
        return *this;
    }

    if (_index > 0) {
        --_index;
        return *this;
    }

    while (_path.size() > 1) {
        auto current = _path.top();
        _path.pop();
        auto parent = _path.top();
        btree_node* parent_node = *parent.first;
        size_t child_index = current.second;
        if (child_index > 0) {
            _index = child_index - 1;
            return *this;
        }
    }

    _path = std::stack<std::pair<btree_node**, size_t>>();
    _index = 0;
    return *this;
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
typename B_tree<tkey, tvalue, compare, t>::btree_iterator
B_tree<tkey, tvalue, compare, t>::btree_iterator::operator--(int)
{
    self temp = *this;
    --(*this);
    return temp;
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
bool B_tree<tkey, tvalue, compare, t>::btree_iterator::operator==(const self& other) const noexcept
{
    if (_path.empty() && other._path.empty()) {
        return true;
    }
    if (_path.empty() || other._path.empty()) {
        return false;
    }
    return _path.top().first == other._path.top().first && _index == other._index;
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
bool B_tree<tkey, tvalue, compare, t>::btree_iterator::operator!=(const self& other) const noexcept
{
    return !(*this == other);
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
size_t B_tree<tkey, tvalue, compare, t>::btree_iterator::depth() const noexcept
{
    return _path.empty() ? 0 : _path.size() - 1;
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
size_t B_tree<tkey, tvalue, compare, t>::btree_iterator::current_node_keys_count() const noexcept
{
    if (_path.empty()) {
        return 0;
    }
    btree_node* node = *_path.top().first;
    return node->_keys.size();
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
bool B_tree<tkey, tvalue, compare, t>::btree_iterator::is_terminate_node() const noexcept
{
    return _path.empty();
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
size_t B_tree<tkey, tvalue, compare, t>::btree_iterator::index() const noexcept
{
    return _index;
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
B_tree<tkey, tvalue, compare, t>::btree_const_iterator::btree_const_iterator(
        const std::stack<std::pair<btree_node* const*, size_t>>& path, size_t index)
    : _path(path), _index(index)
{
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
B_tree<tkey, tvalue, compare, t>::btree_const_iterator::btree_const_iterator(
        const btree_iterator& it) noexcept
    : _index(it._index)
{
    std::vector<std::pair<btree_node**, size_t>> items;
    auto temp = it._path;
    while (!temp.empty()) {
        items.push_back(temp.top());
        temp.pop();
    }
    for (auto it_item = items.rbegin(); it_item != items.rend(); ++it_item) {
        _path.push({const_cast<btree_node* const*>(it_item->first), it_item->second});
    }
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
typename B_tree<tkey, tvalue, compare, t>::btree_const_iterator::reference
B_tree<tkey, tvalue, compare, t>::btree_const_iterator::operator*() const noexcept
{
    const btree_node* node = *_path.top().first;
    return *reinterpret_cast<const value_type*>(&node->_keys[_index]);
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
typename B_tree<tkey, tvalue, compare, t>::btree_const_iterator::pointer
B_tree<tkey, tvalue, compare, t>::btree_const_iterator::operator->() const noexcept
{
    const btree_node* node = *_path.top().first;
    return reinterpret_cast<const value_type*>(&node->_keys[_index]);
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
typename B_tree<tkey, tvalue, compare, t>::btree_const_iterator&
B_tree<tkey, tvalue, compare, t>::btree_const_iterator::operator++()
{
    if (_path.empty()) {
        return *this;
    }

    const btree_node* node = *_path.top().first;

    if (!node->_pointers.empty()) {
        size_t next_child_index = _index + 1;
        if (next_child_index < node->_pointers.size()) {
            const btree_node* child = node->_pointers[next_child_index];
            _path.push({&node->_pointers[next_child_index], next_child_index});
            while (!child->_pointers.empty()) {
                _path.push({&child->_pointers[0], 0});
                child = child->_pointers[0];
            }
            _index = 0;
            return *this;
        }
    }

    if (_index + 1 < node->_keys.size()) {
        ++_index;
        return *this;
    }

    while (_path.size() > 1) {
        auto current = _path.top();
        _path.pop();
        auto parent = _path.top();
        const btree_node* parent_node = *parent.first;
        size_t child_index = current.second;
        if (child_index < parent_node->_keys.size()) {
            _index = child_index;
            return *this;
        }
    }

    _path = std::stack<std::pair<btree_node* const*, size_t>>();
    _index = 0;
    return *this;
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
typename B_tree<tkey, tvalue, compare, t>::btree_const_iterator
B_tree<tkey, tvalue, compare, t>::btree_const_iterator::operator++(int)
{
    self temp = *this;
    ++(*this);
    return temp;
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
typename B_tree<tkey, tvalue, compare, t>::btree_const_iterator&
B_tree<tkey, tvalue, compare, t>::btree_const_iterator::operator--()
{
    if (_path.empty()) {
        return *this;
    }

    btree_node* node = const_cast<btree_node*>(*_path.top().first);

    if (!node->_pointers.empty()) {
        btree_node* child = node->_pointers[_index];
        _path.push({&node->_pointers[_index], _index});
        while (!child->_pointers.empty()) {
            size_t idx = child->_pointers.size() - 1;
            _path.push({&child->_pointers[idx], idx});
            child = child->_pointers[idx];
        }
        _index = child->_keys.size() - 1;
        return *this;
    }

    if (_index > 0) {
        --_index;
        return *this;
    }

    while (_path.size() > 1) {
        auto current = _path.top();
        _path.pop();
        auto parent = _path.top();
        btree_node* parent_node = const_cast<btree_node*>(*parent.first);
        size_t child_index = current.second;
        if (child_index > 0) {
            _index = child_index - 1;
            return *this;
        }
    }

    _path = std::stack<std::pair<btree_node* const*, size_t>>();
    _index = 0;
    return *this;
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
typename B_tree<tkey, tvalue, compare, t>::btree_const_iterator
B_tree<tkey, tvalue, compare, t>::btree_const_iterator::operator--(int)
{
    self temp = *this;
    --(*this);
    return temp;
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
bool B_tree<tkey, tvalue, compare, t>::btree_const_iterator::operator==(const self& other) const noexcept
{
    if (_path.empty() && other._path.empty()) {
        return true;
    }
    if (_path.empty() || other._path.empty()) {
        return false;
    }
    return _path.top().first == other._path.top().first && _index == other._index;
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
bool B_tree<tkey, tvalue, compare, t>::btree_const_iterator::operator!=(const self& other) const noexcept
{
    return !(*this == other);
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
size_t B_tree<tkey, tvalue, compare, t>::btree_const_iterator::depth() const noexcept
{
    return _path.empty() ? 0 : _path.size() - 1;
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
size_t B_tree<tkey, tvalue, compare, t>::btree_const_iterator::current_node_keys_count() const noexcept
{
    if (_path.empty()) {
        return 0;
    }
    const btree_node* node = *_path.top().first;
    return node->_keys.size();
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
bool B_tree<tkey, tvalue, compare, t>::btree_const_iterator::is_terminate_node() const noexcept
{
    return _path.empty();
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
size_t B_tree<tkey, tvalue, compare, t>::btree_const_iterator::index() const noexcept
{
    return _index;
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
B_tree<tkey, tvalue, compare, t>::btree_reverse_iterator::btree_reverse_iterator(
        const std::stack<std::pair<btree_node**, size_t>>& path, size_t index)
    : _path(path), _index(index)
{
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
B_tree<tkey, tvalue, compare, t>::btree_reverse_iterator::btree_reverse_iterator(
        const btree_iterator& it) noexcept
    : _path(it._path), _index(it._index)
{
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
B_tree<tkey, tvalue, compare, t>::btree_reverse_iterator::operator B_tree<tkey, tvalue, compare, t>::btree_iterator() const noexcept
{
    return btree_iterator(_path, _index);
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
typename B_tree<tkey, tvalue, compare, t>::btree_reverse_iterator::reference
B_tree<tkey, tvalue, compare, t>::btree_reverse_iterator::operator*() const noexcept
{
    btree_node* node = *_path.top().first;
    return *reinterpret_cast<value_type*>(&node->_keys[_index]);
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
typename B_tree<tkey, tvalue, compare, t>::btree_reverse_iterator::pointer
B_tree<tkey, tvalue, compare, t>::btree_reverse_iterator::operator->() const noexcept
{
    btree_node* node = *_path.top().first;
    return reinterpret_cast<value_type*>(&node->_keys[_index]);
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
typename B_tree<tkey, tvalue, compare, t>::btree_reverse_iterator&
B_tree<tkey, tvalue, compare, t>::btree_reverse_iterator::operator++()
{
    if (_path.empty()) {
        return *this;
    }
    btree_iterator temp(_path, _index);
    --temp;
    _path = temp._path;
    _index = temp._index;
    return *this;
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
typename B_tree<tkey, tvalue, compare, t>::btree_reverse_iterator
B_tree<tkey, tvalue, compare, t>::btree_reverse_iterator::operator++(int)
{
    self temp = *this;
    ++(*this);
    return temp;
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
typename B_tree<tkey, tvalue, compare, t>::btree_reverse_iterator&
B_tree<tkey, tvalue, compare, t>::btree_reverse_iterator::operator--()
{
    if (_path.empty()) {
        return *this;
    }
    btree_iterator temp(_path, _index);
    ++temp;
    _path = temp._path;
    _index = temp._index;
    return *this;
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
typename B_tree<tkey, tvalue, compare, t>::btree_reverse_iterator
B_tree<tkey, tvalue, compare, t>::btree_reverse_iterator::operator--(int)
{
    self temp = *this;
    --(*this);
    return temp;
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
bool B_tree<tkey, tvalue, compare, t>::btree_reverse_iterator::operator==(const self& other) const noexcept
{
    if (_path.empty() && other._path.empty()) {
        return true;
    }
    if (_path.empty() || other._path.empty()) {
        return false;
    }
    return _path.top().first == other._path.top().first && _index == other._index;
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
bool B_tree<tkey, tvalue, compare, t>::btree_reverse_iterator::operator!=(const self& other) const noexcept
{
    return !(*this == other);
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
size_t B_tree<tkey, tvalue, compare, t>::btree_reverse_iterator::depth() const noexcept
{
    return _path.empty() ? 0 : _path.size() - 1;
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
size_t B_tree<tkey, tvalue, compare, t>::btree_reverse_iterator::current_node_keys_count() const noexcept
{
    if (_path.empty()) {
        return 0;
    }
    btree_node* node = *_path.top().first;
    return node->_keys.size();
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
bool B_tree<tkey, tvalue, compare, t>::btree_reverse_iterator::is_terminate_node() const noexcept
{
    return _path.empty();
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
size_t B_tree<tkey, tvalue, compare, t>::btree_reverse_iterator::index() const noexcept
{
    return _index;
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
B_tree<tkey, tvalue, compare, t>::btree_const_reverse_iterator::btree_const_reverse_iterator(
        const std::stack<std::pair<btree_node* const*, size_t>>& path, size_t index)
    : _path(path), _index(index)
{
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
B_tree<tkey, tvalue, compare, t>::btree_const_reverse_iterator::btree_const_reverse_iterator(
        const btree_reverse_iterator& it) noexcept
    : _index(it._index)
{
    std::vector<std::pair<btree_node**, size_t>> items;
    auto temp = it._path;
    while (!temp.empty()) {
        items.push_back(temp.top());
        temp.pop();
    }
    for (auto it_item = items.rbegin(); it_item != items.rend(); ++it_item) {
        _path.push({const_cast<btree_node* const*>(it_item->first), it_item->second});
    }
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
B_tree<tkey, tvalue, compare, t>::btree_const_reverse_iterator::operator B_tree<tkey, tvalue, compare, t>::btree_const_iterator() const noexcept
{
    return btree_const_iterator(_path, _index);
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
typename B_tree<tkey, tvalue, compare, t>::btree_const_reverse_iterator::reference
B_tree<tkey, tvalue, compare, t>::btree_const_reverse_iterator::operator*() const noexcept
{
    const btree_node* node = *_path.top().first;
    return *reinterpret_cast<const value_type*>(&node->_keys[_index]);
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
typename B_tree<tkey, tvalue, compare, t>::btree_const_reverse_iterator::pointer
B_tree<tkey, tvalue, compare, t>::btree_const_reverse_iterator::operator->() const noexcept
{
    const btree_node* node = *_path.top().first;
    return reinterpret_cast<const value_type*>(&node->_keys[_index]);
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
typename B_tree<tkey, tvalue, compare, t>::btree_const_reverse_iterator&
B_tree<tkey, tvalue, compare, t>::btree_const_reverse_iterator::operator++()
{
    if (_path.empty()) {
        return *this;
    }
    btree_const_iterator temp(_path, _index);
    --temp;
    _path = temp._path;
    _index = temp._index;
    return *this;
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
typename B_tree<tkey, tvalue, compare, t>::btree_const_reverse_iterator
B_tree<tkey, tvalue, compare, t>::btree_const_reverse_iterator::operator++(int)
{
    self temp = *this;
    ++(*this);
    return temp;
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
typename B_tree<tkey, tvalue, compare, t>::btree_const_reverse_iterator&
B_tree<tkey, tvalue, compare, t>::btree_const_reverse_iterator::operator--()
{
    if (_path.empty()) {
        return *this;
    }
    btree_const_iterator temp(_path, _index);
    ++temp;
    _path = temp._path;
    _index = temp._index;
    return *this;
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
typename B_tree<tkey, tvalue, compare, t>::btree_const_reverse_iterator
B_tree<tkey, tvalue, compare, t>::btree_const_reverse_iterator::operator--(int)
{
    self temp = *this;
    --(*this);
    return temp;
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
bool B_tree<tkey, tvalue, compare, t>::btree_const_reverse_iterator::operator==(const self& other) const noexcept
{
    if (_path.empty() && other._path.empty()) {
        return true;
    }
    if (_path.empty() || other._path.empty()) {
        return false;
    }
    return _path.top().first == other._path.top().first && _index == other._index;
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
bool B_tree<tkey, tvalue, compare, t>::btree_const_reverse_iterator::operator!=(const self& other) const noexcept
{
    return !(*this == other);
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
size_t B_tree<tkey, tvalue, compare, t>::btree_const_reverse_iterator::depth() const noexcept
{
    return _path.empty() ? 0 : _path.size() - 1;
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
size_t B_tree<tkey, tvalue, compare, t>::btree_const_reverse_iterator::current_node_keys_count() const noexcept
{
    if (_path.empty()) {
        return 0;
    }
    const btree_node* node = *_path.top().first;
    return node->_keys.size();
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
bool B_tree<tkey, tvalue, compare, t>::btree_const_reverse_iterator::is_terminate_node() const noexcept
{
    return _path.empty();
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
size_t B_tree<tkey, tvalue, compare, t>::btree_const_reverse_iterator::index() const noexcept
{
    return _index;
}

// endregion iterators implementation

// region element access implementation

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
tvalue& B_tree<tkey, tvalue, compare, t>::at(const tkey& key)
{
    auto it = find(key);
    if (it == end()) {
        throw out_of_range("key not found");
    }
    return it->second;
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
const tvalue& B_tree<tkey, tvalue, compare, t>::at(const tkey& key) const
{
    auto it = find(key);
    if (it == end()) {
        throw out_of_range("key not found");
    }
    return it->second;
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
tvalue& B_tree<tkey, tvalue, compare, t>::operator[](const tkey& key)
{
    auto it = find(key);
    if (it != end()) {
        return it->second;
    }

    auto result = emplace(key, tvalue{});
    return result.first->second;
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
tvalue& B_tree<tkey, tvalue, compare, t>::operator[](tkey&& key)
{
    auto it = find(key);
    if (it != end()) {
        return it->second;
    }

    auto result = emplace(std::move(key), tvalue{});
    return result.first->second;
}

// endregion element access implementation

// region iterator begins implementation

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
typename B_tree<tkey, tvalue, compare, t>::btree_iterator B_tree<tkey, tvalue, compare, t>::begin()
{
    if (_root == nullptr) {
        return end();
    }

    std::stack<std::pair<btree_node**, size_t>> path;
    btree_node* current = _root;
    path.push({&_root, 0});
    while (!current->_pointers.empty()) {
        path.push({&current->_pointers[0], 0});
        current = current->_pointers[0];
    }
    return btree_iterator(path, 0);
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
typename B_tree<tkey, tvalue, compare, t>::btree_iterator B_tree<tkey, tvalue, compare, t>::end()
{
    return btree_iterator();
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
typename B_tree<tkey, tvalue, compare, t>::btree_const_iterator B_tree<tkey, tvalue, compare, t>::begin() const
{
    return cbegin();
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
typename B_tree<tkey, tvalue, compare, t>::btree_const_iterator B_tree<tkey, tvalue, compare, t>::end() const
{
    return cend();
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
typename B_tree<tkey, tvalue, compare, t>::btree_const_iterator B_tree<tkey, tvalue, compare, t>::cbegin() const
{
    if (_root == nullptr) {
        return cend();
    }

    std::stack<std::pair<btree_node* const*, size_t>> path;
    btree_node* current = _root;
    path.push({&_root, 0});
    while (!current->_pointers.empty()) {
        path.push({&current->_pointers[0], 0});
        current = current->_pointers[0];
    }
    return btree_const_iterator(path, 0);
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
typename B_tree<tkey, tvalue, compare, t>::btree_const_iterator B_tree<tkey, tvalue, compare, t>::cend() const
{
    return btree_const_iterator();
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
typename B_tree<tkey, tvalue, compare, t>::btree_reverse_iterator B_tree<tkey, tvalue, compare, t>::rbegin()
{
    if (_root == nullptr) {
        return rend();
    }

    std::stack<std::pair<btree_node**, size_t>> path;
    btree_node* current = _root;
    path.push({&_root, 0});
    while (!current->_pointers.empty()) {
        size_t idx = current->_pointers.size() - 1;
        path.push({&current->_pointers[idx], idx});
        current = current->_pointers[idx];
    }
    return btree_reverse_iterator(path, current->_keys.size() - 1);
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
typename B_tree<tkey, tvalue, compare, t>::btree_reverse_iterator B_tree<tkey, tvalue, compare, t>::rend()
{
    return btree_reverse_iterator();
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
typename B_tree<tkey, tvalue, compare, t>::btree_const_reverse_iterator B_tree<tkey, tvalue, compare, t>::rbegin() const
{
    return crbegin();
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
typename B_tree<tkey, tvalue, compare, t>::btree_const_reverse_iterator B_tree<tkey, tvalue, compare, t>::rend() const
{
    return crend();
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
typename B_tree<tkey, tvalue, compare, t>::btree_const_reverse_iterator B_tree<tkey, tvalue, compare, t>::crbegin() const
{
    if (_root == nullptr) {
        return crend();
    }

    std::stack<std::pair<btree_node* const*, size_t>> path;
    btree_node* current = _root;
    path.push({&_root, 0});
    while (!current->_pointers.empty()) {
        size_t idx = current->_pointers.size() - 1;
        path.push({&current->_pointers[idx], idx});
        current = current->_pointers[idx];
    }
    return btree_const_reverse_iterator(path, current->_keys.size() - 1);
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
typename B_tree<tkey, tvalue, compare, t>::btree_const_reverse_iterator B_tree<tkey, tvalue, compare, t>::crend() const
{
    return btree_const_reverse_iterator();
}

// endregion iterator begins implementation

// region lookup implementation

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
size_t B_tree<tkey, tvalue, compare, t>::size() const noexcept
{
    return _size;
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
bool B_tree<tkey, tvalue, compare, t>::empty() const noexcept
{
    return _size == 0;
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
typename B_tree<tkey, tvalue, compare, t>::btree_iterator B_tree<tkey, tvalue, compare, t>::find(const tkey& key)
{
    for (auto it = begin(); it != end(); ++it) {
        if (key_equal(it->first, key)) {
            return it;
        }
    }
    return end();
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
typename B_tree<tkey, tvalue, compare, t>::btree_const_iterator B_tree<tkey, tvalue, compare, t>::find(const tkey& key) const
{
    for (auto it = cbegin(); it != cend(); ++it) {
        if (key_equal(it->first, key)) {
            return it;
        }
    }
    return cend();
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
typename B_tree<tkey, tvalue, compare, t>::btree_iterator B_tree<tkey, tvalue, compare, t>::lower_bound(const tkey& key)
{
    for (auto it = begin(); it != end(); ++it) {
        if (!compare_keys(it->first, key)) {
            return it;
        }
    }
    return end();
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
typename B_tree<tkey, tvalue, compare, t>::btree_const_iterator B_tree<tkey, tvalue, compare, t>::lower_bound(const tkey& key) const
{
    for (auto it = cbegin(); it != cend(); ++it) {
        if (!compare_keys(it->first, key)) {
            return it;
        }
    }
    return cend();
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
typename B_tree<tkey, tvalue, compare, t>::btree_iterator B_tree<tkey, tvalue, compare, t>::upper_bound(const tkey& key)
{
    auto it = lower_bound(key);
    if (it != end() && key_equal(it->first, key)) {
        ++it;
    }
    return it;
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
typename B_tree<tkey, tvalue, compare, t>::btree_const_iterator B_tree<tkey, tvalue, compare, t>::upper_bound(const tkey& key) const
{
    auto it = lower_bound(key);
    if (it != cend() && key_equal(it->first, key)) {
        ++it;
    }
    return it;
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
bool B_tree<tkey, tvalue, compare, t>::contains(const tkey& key) const
{
    return find(key) != end();
}

// endregion lookup implementation

// region modifiers implementation

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
void B_tree<tkey, tvalue, compare, t>::clear() noexcept
{
    destroy_subtree(_root);
    _root = nullptr;
    _size = 0;
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
std::pair<typename B_tree<tkey, tvalue, compare, t>::btree_iterator, bool>
B_tree<tkey, tvalue, compare, t>::insert(const tree_data_type& data)
{
    auto it = find(data.first);
    if (it != end()) {
        return {it, false};
    }
    return emplace(data.first, data.second);
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
std::pair<typename B_tree<tkey, tvalue, compare, t>::btree_iterator, bool>
B_tree<tkey, tvalue, compare, t>::insert(tree_data_type&& data)
{
    auto it = find(data.first);
    if (it != end()) {
        return {it, false};
    }
    return emplace(std::move(data.first), std::move(data.second));
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
template<typename... Args>
std::pair<typename B_tree<tkey, tvalue, compare, t>::btree_iterator, bool>
B_tree<tkey, tvalue, compare, t>::emplace(Args&&... args)
{
    tree_data_type data(std::forward<Args>(args)...);
    tkey key = data.first;

    auto it = find(data.first);
    if (it != end()) {
        return {it, false};
    }

    if (_root == nullptr) {
        _root = _allocator.template new_object<btree_node>();
        _root->_keys.push_back(std::move(data));
        ++_size;
        return {begin(), true};
    }

    insert_nonfull(_root, std::move(data));

    if (_root->_keys.size() > maximum_keys_in_node) {
        btree_node* new_root = _allocator.template new_object<btree_node>();
        new_root->_pointers.push_back(_root);
        split_child(new_root, 0);
        _root = new_root;
    }
    ++_size;
    return {find(key), true};
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
typename B_tree<tkey, tvalue, compare, t>::btree_iterator
B_tree<tkey, tvalue, compare, t>::insert_or_assign(const tree_data_type& data)
{
    auto it = find(data.first);
    if (it != end()) {
        it->second = data.second;
        return it;
    }
    return emplace(data.first, data.second).first;
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
typename B_tree<tkey, tvalue, compare, t>::btree_iterator
B_tree<tkey, tvalue, compare, t>::insert_or_assign(tree_data_type&& data)
{
    auto it = find(data.first);
    if (it != end()) {
        it->second = std::move(data.second);
        return it;
    }
    return emplace(std::move(data.first), std::move(data.second)).first;
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
template<typename... Args>
typename B_tree<tkey, tvalue, compare, t>::btree_iterator
B_tree<tkey, tvalue, compare, t>::emplace_or_assign(Args&&... args)
{
    tree_data_type data(std::forward<Args>(args)...);
    return insert_or_assign(std::move(data));
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
typename B_tree<tkey, tvalue, compare, t>::btree_iterator
B_tree<tkey, tvalue, compare, t>::erase(btree_iterator pos)
{
    if (pos == end()) {
        return pos;
    }
    tkey key = pos->first;
    erase(key);
    return lower_bound(key);
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
typename B_tree<tkey, tvalue, compare, t>::btree_iterator
B_tree<tkey, tvalue, compare, t>::erase(btree_const_iterator pos)
{
    if (pos == cend()) {
        return end();
    }
    tkey key = pos->first;
    erase(key);
    return lower_bound(key);
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
typename B_tree<tkey, tvalue, compare, t>::btree_iterator
B_tree<tkey, tvalue, compare, t>::erase(btree_iterator beg, btree_iterator en)
{
    while (beg != en) {
        beg = erase(beg);
    }
    return beg;
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
typename B_tree<tkey, tvalue, compare, t>::btree_iterator
B_tree<tkey, tvalue, compare, t>::erase(btree_const_iterator beg, btree_const_iterator en)
{
    auto to_mutable = [](const btree_const_iterator& it) {
        std::vector<std::pair<btree_node* const*, size_t>> items;
        auto tmp = it._path;
        while (!tmp.empty()) {
            items.push_back(tmp.top());
            tmp.pop();
        }
        std::stack<std::pair<btree_node**, size_t>> path;
        for (auto it_item = items.rbegin(); it_item != items.rend(); ++it_item) {
            path.push({const_cast<btree_node**>(it_item->first), it_item->second});
        }
        return btree_iterator(path, it._index);
    };

    auto it = to_mutable(beg);
    auto end_it = to_mutable(en);
    while (it != end_it) {
        it = erase(it);
    }
    return it;
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
typename B_tree<tkey, tvalue, compare, t>::btree_iterator
B_tree<tkey, tvalue, compare, t>::erase(const tkey& key)
{
    if (_root == nullptr) {
        return end();
    }

    bool removed = remove_key(_root, key);
    if (removed) {
        --_size;
    }

    if (_root != nullptr && _root->_keys.empty()) {
        if (_root->_pointers.empty()) {
            _allocator.template delete_object(_root);
            _root = nullptr;
        } else {
            btree_node* old_root = _root;
            _root = _root->_pointers[0];
            _allocator.template delete_object(old_root);
        }
    }

    return lower_bound(key);
}

// endregion modifiers implementation

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
bool compare_pairs(const typename B_tree<tkey, tvalue, compare, t>::tree_data_type &lhs,
                   const typename B_tree<tkey, tvalue, compare, t>::tree_data_type &rhs)
{
    return compare{}(lhs.first, rhs.first);
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
bool compare_keys(const tkey &lhs, const tkey &rhs)
{
    return compare{}(lhs, rhs);
}


#endif
#include <iterator>
#include <utility>
#include <vector>
#include <boost/container/static_vector.hpp>
#include <concepts>
#include <stack>
#include <pp_allocator.h>
#include <associative_container.h>
#include <not_implemented.h>
#include <initializer_list>

#ifndef SYS_PROG_B_PLUS_TREE_H
#define SYS_PROG_B_PLUS_TREE_H

template <typename tkey, typename tvalue, comparator<tkey> compare = std::less<tkey>, std::size_t t = 5>
class BP_tree final : private compare //EBCO
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

    inline bool compare_keys(const tkey& lhs, const tkey& rhs) const;
    inline bool compare_pairs(const tree_data_type& lhs, const tree_data_type& rhs) const;
    inline bool key_equal(const tkey& lhs, const tkey& rhs) const;

    struct bptree_node_base
    {
        bool _is_terminate;

        bptree_node_base() noexcept;
        virtual ~bptree_node_base() =default;
    };

    struct bptree_node_term : public bptree_node_base
    {
        bptree_node_term* _next;

        boost::container::static_vector<tree_data_type, maximum_keys_in_node + 1> _data;
        bptree_node_term() noexcept;
    };

    struct bptree_node_middle : public bptree_node_base
    {
        boost::container::static_vector<tkey, maximum_keys_in_node + 1> _keys;
        boost::container::static_vector<bptree_node_base*, maximum_keys_in_node + 2> _pointers;
        bptree_node_middle() noexcept;
    };

    pp_allocator<value_type> _allocator;
    bptree_node_base* _root;
    size_t _size;

    pp_allocator<value_type> get_allocator() const noexcept;

    bool is_term(const bptree_node_base* node) const noexcept;
    bptree_node_term* leftmost_leaf() const noexcept;

    bptree_node_middle* clone_subtree(bptree_node_middle* node);
    bptree_node_term* clone_leaf(bptree_node_term* leaf);
    void destroy_subtree(bptree_node_base* node) noexcept;

    bptree_node_term* find_leaf(const tkey& key) const;

    void insert_into_leaf(bptree_node_term* leaf, tree_data_type&& data);
    void insert_into_parent(bptree_node_base* left, bptree_node_base* right, const tkey& key,
                            const std::stack<std::pair<bptree_node_middle*, size_t>>& path);

    void remove_from_leaf(bptree_node_term* leaf, const tkey& key,
                          const std::stack<std::pair<bptree_node_middle*, size_t>>& path);

public:

    explicit BP_tree(const compare& cmp = compare(), pp_allocator<value_type> = pp_allocator<value_type>());

    explicit BP_tree(pp_allocator<value_type> alloc, const compare& comp = compare());

    template<input_iterator_for_pair<tkey, tvalue> iterator>
    explicit BP_tree(iterator begin, iterator end, const compare& cmp = compare(), pp_allocator<value_type> = pp_allocator<value_type>());

    BP_tree(std::initializer_list<std::pair<tkey, tvalue>> data, const compare& cmp = compare(), pp_allocator<value_type> = pp_allocator<value_type>());

    BP_tree(const BP_tree& other);

    BP_tree(BP_tree&& other) noexcept;

    BP_tree& operator=(const BP_tree& other);

    BP_tree& operator=(BP_tree&& other) noexcept;

    ~BP_tree() noexcept;

    class bptree_iterator;
    class bptree_const_iterator;

    class bptree_iterator final
    {
        bptree_node_term* _node;
        size_t _index;

    public:
        using value_type = tree_data_type_const;
        using reference = value_type&;
        using pointer = value_type*;
        using iterator_category = std::forward_iterator_tag;
        using difference_type = ptrdiff_t;
        using self = bptree_iterator;

        friend class BP_tree;
        friend class bptree_const_iterator;

        reference operator*() const noexcept;
        pointer operator->() const noexcept;

        self& operator++();
        self operator++(int);

        bool operator==(const self& other) const noexcept;
        bool operator!=(const self& other) const noexcept;

        size_t current_node_keys_count() const noexcept;
        size_t index() const noexcept;

        explicit bptree_iterator(bptree_node_term* node = nullptr, size_t index = 0);

    };

    class bptree_const_iterator final
    {
        const bptree_node_term* _node;
        size_t _index;

    public:

        using value_type = tree_data_type_const;
        using reference = const value_type&;
        using pointer = const value_type*;
        using iterator_category = std::forward_iterator_tag;
        using difference_type = ptrdiff_t;
        using self = bptree_const_iterator;

        friend class BP_tree;
        friend class bptree_iterator;

        bptree_const_iterator(const bptree_iterator& it) noexcept;

        reference operator*() const noexcept;
        pointer operator->() const noexcept;

        self& operator++();
        self operator++(int);

        bool operator==(const self& other) const noexcept;
        bool operator!=(const self& other) const noexcept;

        size_t current_node_keys_count() const noexcept;
        size_t index() const noexcept;

        explicit bptree_const_iterator(const bptree_node_term* node = nullptr, size_t index = 0);
    };

    friend class bptree_iterator;
    friend class bptree_const_iterator;

    tvalue& at(const tkey&);
    const tvalue& at(const tkey&) const;

    tvalue& operator[](const tkey& key);
    tvalue& operator[](tkey&& key);

    bptree_iterator begin();
    bptree_iterator end();

    bptree_const_iterator begin() const;
    bptree_const_iterator end() const;

    bptree_const_iterator cbegin() const;
    bptree_const_iterator cend() const;

    size_t size() const noexcept;
    bool empty() const noexcept;

    bptree_iterator find(const tkey& key);
    bptree_const_iterator find(const tkey& key) const;

    bptree_iterator lower_bound(const tkey& key);
    bptree_const_iterator lower_bound(const tkey& key) const;

    bptree_iterator upper_bound(const tkey& key);
    bptree_const_iterator upper_bound(const tkey& key) const;

    bool contains(const tkey& key) const;

    void clear() noexcept;

    std::pair<bptree_iterator, bool> insert(const tree_data_type& data);
    std::pair<bptree_iterator, bool> insert(tree_data_type&& data);

    template <typename ...Args>
    std::pair<bptree_iterator, bool> emplace(Args&&... args);

    bptree_iterator insert_or_assign(const tree_data_type& data);
    bptree_iterator insert_or_assign(tree_data_type&& data);

    template <typename ...Args>
    bptree_iterator emplace_or_assign(Args&&... args);

    bptree_iterator erase(bptree_iterator pos);
    bptree_iterator erase(bptree_const_iterator pos);

    bptree_iterator erase(bptree_iterator beg, bptree_iterator en);
    bptree_iterator erase(bptree_const_iterator beg, bptree_const_iterator en);

    bptree_iterator erase(const tkey& key);
};

template<std::input_iterator iterator, comparator<typename std::iterator_traits<iterator>::value_type::first_type> compare = std::less<typename std::iterator_traits<iterator>::value_type::first_type>,
        std::size_t t = 5, typename U>
BP_tree(iterator begin, iterator end, const compare &cmp = compare(), pp_allocator<U> = pp_allocator<U>()) -> BP_tree<typename std::iterator_traits<iterator>::value_type::first_type, typename std::iterator_traits<iterator>::value_type::second_type, compare, t>;

template<typename tkey, typename tvalue, comparator<tkey> compare = std::less<tkey>, std::size_t t = 5, typename U>
BP_tree(std::initializer_list<std::pair<tkey, tvalue>> data, const compare &cmp = compare(), pp_allocator<U> = pp_allocator<U>()) -> BP_tree<tkey, tvalue, compare, t>;

// comparators

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
bool BP_tree<tkey, tvalue, compare, t>::compare_pairs(const BP_tree::tree_data_type &lhs,
                                                     const BP_tree::tree_data_type &rhs) const
{
    return compare_keys(lhs.first, rhs.first);
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
bool BP_tree<tkey, tvalue, compare, t>::compare_keys(const tkey &lhs, const tkey &rhs) const
{
    return compare::operator()(lhs, rhs);
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
bool BP_tree<tkey, tvalue, compare, t>::key_equal(const tkey& lhs, const tkey& rhs) const
{
    return !compare_keys(lhs, rhs) && !compare_keys(rhs, lhs);
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
bool BP_tree<tkey, tvalue, compare, t>::is_term(const bptree_node_base* node) const noexcept
{
    return node != nullptr && node->_is_terminate;
}

// node constructors

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
BP_tree<tkey, tvalue, compare, t>::bptree_node_base::bptree_node_base() noexcept
    : _is_terminate(true)
{
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
BP_tree<tkey, tvalue, compare, t>::bptree_node_term::bptree_node_term() noexcept
    : bptree_node_base(), _next(nullptr)
{
    this->_is_terminate = true;
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
BP_tree<tkey, tvalue, compare, t>::bptree_node_middle::bptree_node_middle() noexcept
    : bptree_node_base()
{
    this->_is_terminate = false;
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
pp_allocator<typename BP_tree<tkey, tvalue, compare, t>::value_type> BP_tree<tkey, tvalue, compare, t>::
get_allocator() const noexcept
{
    return _allocator;
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
typename BP_tree<tkey, tvalue, compare, t>::bptree_node_term * BP_tree<tkey, tvalue, compare, t>::leftmost_leaf() const noexcept
{
    if (!_root) {
        return nullptr;
    }
    bptree_node_base* current = _root;
    while (!is_term(current)) {
        current = static_cast<bptree_node_middle*>(current)->_pointers.front();
    }
    return static_cast<bptree_node_term*>(current);
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
typename BP_tree<tkey, tvalue, compare, t>::bptree_node_term *
BP_tree<tkey, tvalue, compare, t>::find_leaf(const tkey& key) const
{
    if (!_root) {
        return nullptr;
    }

    bptree_node_base* current = _root;
    while (!is_term(current)) {
        auto* mid = static_cast<bptree_node_middle*>(current);
        size_t idx = 0;
        while (idx < mid->_keys.size() && !compare_keys(key, mid->_keys[idx])) {
            ++idx;
        }
        current = mid->_pointers[idx];
    }
    return static_cast<bptree_node_term*>(current);
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
typename BP_tree<tkey, tvalue, compare, t>::bptree_node_middle *
BP_tree<tkey, tvalue, compare, t>::clone_subtree(bptree_node_middle* node)
{
    if (!node) {
        return nullptr;
    }

    auto* copy = _allocator.template new_object<bptree_node_middle>();
    copy->_is_terminate = false;
    copy->_keys = node->_keys;

    for (auto* child : node->_pointers) {
        if (is_term(child)) {
            copy->_pointers.push_back(clone_leaf(static_cast<bptree_node_term*>(child)));
        } else {
            copy->_pointers.push_back(clone_subtree(static_cast<bptree_node_middle*>(child)));
        }
    }

    return copy;
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
typename BP_tree<tkey, tvalue, compare, t>::bptree_node_term *
BP_tree<tkey, tvalue, compare, t>::clone_leaf(bptree_node_term* leaf)
{
    auto* copy = _allocator.template new_object<bptree_node_term>();
    copy->_is_terminate = true;
    copy->_data = leaf->_data;
    copy->_next = leaf->_next;
    return copy;
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
void BP_tree<tkey, tvalue, compare, t>::destroy_subtree(bptree_node_base* node) noexcept
{
    if (!node) {
        return;
    }

    if (is_term(node)) {
        _allocator.delete_object(static_cast<bptree_node_term*>(node));
        return;
    }

    auto* mid = static_cast<bptree_node_middle*>(node);
    for (auto* child : mid->_pointers) {
        destroy_subtree(child);
    }
    _allocator.delete_object(mid);
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
void BP_tree<tkey, tvalue, compare, t>::insert_into_leaf(bptree_node_term* leaf, tree_data_type&& data)
{
    size_t i = 0;
    while (i < leaf->_data.size() && compare_keys(leaf->_data[i].first, data.first)) {
        ++i;
    }

    leaf->_data.insert(leaf->_data.begin() + static_cast<long long>(i), std::move(data));
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
void BP_tree<tkey, tvalue, compare, t>::insert_into_parent(
    bptree_node_base* left,
    bptree_node_base* right,
    const tkey& key,
    const std::stack<std::pair<bptree_node_middle*, size_t>>& path)
{
    if (path.empty()) {
        auto* new_root = _allocator.template new_object<bptree_node_middle>();
        new_root->_keys.push_back(key);
        new_root->_pointers.push_back(left);
        new_root->_pointers.push_back(right);
        _root = new_root;
        return;
    }

    auto top = path.top();
    bptree_node_middle* parent = top.first;
    size_t idx = top.second;

    parent->_keys.insert(parent->_keys.begin() + static_cast<long long>(idx), key);
    parent->_pointers[idx] = left;
    parent->_pointers.insert(parent->_pointers.begin() + static_cast<long long>(idx + 1), right);

    if (parent->_keys.size() > maximum_keys_in_node) {
        size_t middle = t - 1;
        tkey up_key = parent->_keys[middle];

        auto* new_parent = _allocator.template new_object<bptree_node_middle>();

        for (size_t i = middle + 1; i < parent->_keys.size(); ++i) {
            new_parent->_keys.push_back(parent->_keys[i]);
        }

        for (size_t i = middle + 1; i < parent->_pointers.size(); ++i) {
            new_parent->_pointers.push_back(parent->_pointers[i]);
        }

        parent->_keys.resize(middle);
        parent->_pointers.resize(middle + 1);

        auto path_copy = path;
        path_copy.pop();
        insert_into_parent(parent, new_parent, up_key, path_copy);
    }
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
void BP_tree<tkey, tvalue, compare, t>::remove_from_leaf(bptree_node_term* leaf, const tkey& key,
    const std::stack<std::pair<bptree_node_middle*, size_t>>& path)
{
    size_t idx = 0;
    while (idx < leaf->_data.size() && !key_equal(leaf->_data[idx].first, key)) {
        ++idx;
    }
    if (idx >= leaf->_data.size()) {
        return;
    }

    leaf->_data.erase(leaf->_data.begin() + static_cast<long long>(idx));

    if (!path.empty() && leaf->_data.size() < minimum_keys_in_node) {
        auto top = path.top();
        bptree_node_middle* parent = top.first;
        size_t child_idx = top.second;

        bptree_node_term* left_sib = nullptr;
        bptree_node_term* right_sib = nullptr;

        if (child_idx > 0) {
            left_sib = static_cast<bptree_node_term*>(parent->_pointers[child_idx - 1]);
        }
        if (child_idx + 1 < parent->_pointers.size()) {
            right_sib = static_cast<bptree_node_term*>(parent->_pointers[child_idx + 1]);
        }

        if (right_sib && right_sib->_data.size() > minimum_keys_in_node) {
            leaf->_data.push_back(right_sib->_data.front());
            right_sib->_data.erase(right_sib->_data.begin());
            parent->_keys[child_idx] = right_sib->_data.front().first;
        } else if (left_sib && left_sib->_data.size() > minimum_keys_in_node) {
            leaf->_data.insert(leaf->_data.begin(), left_sib->_data.back());
            left_sib->_data.pop_back();
            parent->_keys[child_idx - 1] = leaf->_data.front().first;
        } else {
            if (right_sib) {
                for (auto& item : right_sib->_data) {
                    leaf->_data.push_back(std::move(item));
                }
                leaf->_next = right_sib->_next;

                parent->_keys.erase(parent->_keys.begin() + static_cast<long long>(child_idx));
                parent->_pointers.erase(parent->_pointers.begin() + static_cast<long long>(child_idx + 1));

                _allocator.delete_object(right_sib);
            } else if (left_sib) {
                for (auto& item : leaf->_data) {
                    left_sib->_data.push_back(std::move(item));
                }
                left_sib->_next = leaf->_next;

                parent->_keys.erase(parent->_keys.begin() + static_cast<long long>(child_idx - 1));
                parent->_pointers.erase(parent->_pointers.begin() + static_cast<long long>(child_idx));

                _allocator.delete_object(leaf);
            }

            if (parent == _root && parent->_keys.empty()) {
                _allocator.delete_object(parent);
                _root = parent->_pointers.front();
            } else if (!path.empty() && parent->_keys.size() < minimum_keys_in_node && parent != _root) {
                // internal node underflow - propagate up
                auto path_copy = path;
                path_copy.pop();
                if (!path_copy.empty()) {
                    auto grand_top = path_copy.top();
                    bptree_node_middle* grand = grand_top.first;
                    size_t p_idx = grand_top.second;

                    bptree_node_middle* left_uncle = nullptr;
                    bptree_node_middle* right_uncle = nullptr;
                    if (p_idx > 0) {
                        left_uncle = static_cast<bptree_node_middle*>(grand->_pointers[p_idx - 1]);
                    }
                    if (p_idx + 1 < grand->_pointers.size()) {
                        right_uncle = static_cast<bptree_node_middle*>(grand->_pointers[p_idx + 1]);
                    }

                    if (left_uncle && left_uncle->_keys.size() > minimum_keys_in_node) {
                        parent->_keys.insert(parent->_keys.begin(), grand->_keys[p_idx - 1]);
                        parent->_pointers.insert(parent->_pointers.begin(), left_uncle->_pointers.back());
                        grand->_keys[p_idx - 1] = left_uncle->_keys.back();
                        left_uncle->_keys.pop_back();
                        left_uncle->_pointers.pop_back();
                    } else if (right_uncle && right_uncle->_keys.size() > minimum_keys_in_node) {
                        parent->_keys.push_back(grand->_keys[p_idx]);
                        parent->_pointers.push_back(right_uncle->_pointers.front());
                        grand->_keys[p_idx] = right_uncle->_keys.front();
                        right_uncle->_keys.erase(right_uncle->_keys.begin());
                        right_uncle->_pointers.erase(right_uncle->_pointers.begin());
                    } else {
                        if (left_uncle) {
                            left_uncle->_keys.push_back(grand->_keys[p_idx - 1]);
                            for (auto& k : parent->_keys) {
                                left_uncle->_keys.push_back(std::move(k));
                            }
                            for (auto* ptr : parent->_pointers) {
                                left_uncle->_pointers.push_back(ptr);
                            }

                            grand->_keys.erase(grand->_keys.begin() + static_cast<long long>(p_idx - 1));
                            grand->_pointers.erase(grand->_pointers.begin() + static_cast<long long>(p_idx));

                            _allocator.delete_object(parent);
                        } else if (right_uncle) {
                            parent->_keys.push_back(grand->_keys[p_idx]);
                            for (auto& k : right_uncle->_keys) {
                                parent->_keys.push_back(std::move(k));
                            }
                            for (auto* ptr : right_uncle->_pointers) {
                                parent->_pointers.push_back(ptr);
                            }

                            grand->_keys.erase(grand->_keys.begin() + static_cast<long long>(p_idx));
                            grand->_pointers.erase(grand->_pointers.begin() + static_cast<long long>(p_idx + 1));

                            _allocator.delete_object(right_uncle);
                        }

                        if (grand == _root && grand->_keys.empty()) {
                            _allocator.delete_object(grand);
                            _root = grand->_pointers.front();
                        }
                    }
                }
            }
        }
    }
}

// constructors

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
BP_tree<tkey, tvalue, compare, t>::BP_tree(
    const compare& cmp,
    pp_allocator<value_type> alloc)
    : compare(cmp), _allocator(alloc), _root(nullptr), _size(0)
{
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
BP_tree<tkey, tvalue, compare, t>::BP_tree(
    pp_allocator<value_type> alloc,
    const compare& comp)
    : compare(comp), _allocator(alloc), _root(nullptr), _size(0)
{
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
template<input_iterator_for_pair<tkey, tvalue> iterator>
BP_tree<tkey, tvalue, compare, t>::BP_tree(
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
BP_tree<tkey, tvalue, compare, t>::BP_tree(
    std::initializer_list<std::pair<tkey, tvalue>> data,
    const compare& cmp,
    pp_allocator<value_type> alloc)
    : compare(cmp), _allocator(alloc), _root(nullptr), _size(0)
{
    for (const auto& item : data) {
        emplace(item.first, item.second);
    }
}

// five

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
BP_tree<tkey, tvalue, compare, t>::~BP_tree() noexcept
{
    clear();
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
BP_tree<tkey, tvalue, compare, t>::BP_tree(const BP_tree& other)
    : compare(static_cast<const compare&>(other)), _allocator(other._allocator), _root(nullptr), _size(other._size)
{
    if (!other._root) {
        return;
    }

    if (is_term(other._root)) {
        _root = clone_leaf(static_cast<bptree_node_term*>(other._root));
    } else {
        _root = clone_subtree(static_cast<bptree_node_middle*>(other._root));
    }

    auto* leaf = static_cast<bptree_node_term*>(other.leftmost_leaf());
    auto* my_leaf = leftmost_leaf();
    while (leaf) {
        my_leaf->_next = leaf->_next;
        leaf = leaf->_next;
        my_leaf = my_leaf->_next;
    }
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
BP_tree<tkey, tvalue, compare, t>& BP_tree<tkey, tvalue, compare, t>::operator=(const BP_tree& other)
{
    if (this == &other) {
        return *this;
    }

    clear();
    static_cast<compare&>(*this) = static_cast<const compare&>(other);
    _allocator = other._allocator;
    _size = other._size;

    if (!other._root) {
        _root = nullptr;
        return *this;
    }

    if (is_term(other._root)) {
        _root = clone_leaf(static_cast<bptree_node_term*>(other._root));
    } else {
        _root = clone_subtree(static_cast<bptree_node_middle*>(other._root));
    }

    return *this;
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
BP_tree<tkey, tvalue, compare, t>::BP_tree(BP_tree&& other) noexcept
    : compare(static_cast<compare&&>(other)), _allocator(std::move(other._allocator)), _root(other._root), _size(other._size)
{
    other._root = nullptr;
    other._size = 0;
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
BP_tree<tkey, tvalue, compare, t>& BP_tree<tkey, tvalue, compare, t>::operator=(BP_tree&& other) noexcept
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

// iterators

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
BP_tree<tkey, tvalue, compare, t>::bptree_iterator::bptree_iterator(bptree_node_term *node, size_t index)
    : _node(node), _index(index)
{
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
typename BP_tree<tkey, tvalue, compare, t>::bptree_iterator::reference
BP_tree<tkey, tvalue, compare, t>::bptree_iterator::operator*() const noexcept
{
    return *reinterpret_cast<value_type*>(&_node->_data[_index]);
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
typename BP_tree<tkey, tvalue, compare, t>::bptree_iterator::pointer
BP_tree<tkey, tvalue, compare, t>::bptree_iterator::operator->() const noexcept
{
    return reinterpret_cast<value_type*>(&_node->_data[_index]);
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
typename BP_tree<tkey, tvalue, compare, t>::bptree_iterator::self &
BP_tree<tkey, tvalue, compare, t>::bptree_iterator::operator++()
{
    if (!_node) {
        return *this;
    }

    if (_index + 1 < _node->_data.size()) {
        ++_index;
        return *this;
    }

    _node = _node->_next;
    _index = 0;
    return *this;
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
typename BP_tree<tkey, tvalue, compare, t>::bptree_iterator::self
BP_tree<tkey, tvalue, compare, t>::bptree_iterator::operator++(int)
{
    self temp = *this;
    ++(*this);
    return temp;
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
bool BP_tree<tkey, tvalue, compare, t>::bptree_iterator::operator==(const self &other) const noexcept
{
    return _node == other._node && _index == other._index;
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
bool BP_tree<tkey, tvalue, compare, t>::bptree_iterator::operator!=(const self &other) const noexcept
{
    return !(*this == other);
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
size_t BP_tree<tkey, tvalue, compare, t>::bptree_iterator::current_node_keys_count() const noexcept
{
    return _node ? _node->_data.size() : 0;
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
size_t BP_tree<tkey, tvalue, compare, t>::bptree_iterator::index() const noexcept
{
    return _index;
}

// const iterators

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
BP_tree<tkey, tvalue, compare, t>::bptree_const_iterator::bptree_const_iterator(const bptree_iterator &it) noexcept
    : _node(it._node), _index(it._index)
{
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
BP_tree<tkey, tvalue, compare, t>::bptree_const_iterator::bptree_const_iterator(const bptree_node_term *node, size_t index)
    : _node(node), _index(index)
{
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
typename BP_tree<tkey, tvalue, compare, t>::bptree_const_iterator::reference
BP_tree<tkey, tvalue, compare, t>::bptree_const_iterator::operator*() const noexcept
{
    return *reinterpret_cast<const value_type*>(&_node->_data[_index]);
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
typename BP_tree<tkey, tvalue, compare, t>::bptree_const_iterator::pointer
BP_tree<tkey, tvalue, compare, t>::bptree_const_iterator::operator->() const noexcept
{
    return reinterpret_cast<const value_type*>(&_node->_data[_index]);
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
typename BP_tree<tkey, tvalue, compare, t>::bptree_const_iterator::self &
BP_tree<tkey, tvalue, compare, t>::bptree_const_iterator::operator++()
{
    if (!_node) {
        return *this;
    }

    if (_index + 1 < _node->_data.size()) {
        ++_index;
        return *this;
    }

    _node = _node->_next;
    _index = 0;
    return *this;
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
typename BP_tree<tkey, tvalue, compare, t>::bptree_const_iterator::self
BP_tree<tkey, tvalue, compare, t>::bptree_const_iterator::operator++(int)
{
    self temp = *this;
    ++(*this);
    return temp;
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
bool BP_tree<tkey, tvalue, compare, t>::bptree_const_iterator::operator==(const self &other) const noexcept
{
    return _node == other._node && _index == other._index;
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
bool BP_tree<tkey, tvalue, compare, t>::bptree_const_iterator::operator!=(const self &other) const noexcept
{
    return !(*this == other);
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
size_t BP_tree<tkey, tvalue, compare, t>::bptree_const_iterator::current_node_keys_count() const noexcept
{
    return _node ? _node->_data.size() : 0;
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
size_t BP_tree<tkey, tvalue, compare, t>::bptree_const_iterator::index() const noexcept
{
    return _index;
}

// begin/end

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
typename BP_tree<tkey, tvalue, compare, t>::bptree_iterator BP_tree<tkey, tvalue, compare, t>::begin()
{
    auto* leaf = leftmost_leaf();
    if (!leaf) {
        return bptree_iterator();
    }
    if (leaf->_data.empty()) {
        return end();
    }
    return bptree_iterator(leaf, 0);
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
typename BP_tree<tkey, tvalue, compare, t>::bptree_iterator BP_tree<tkey, tvalue, compare, t>::end()
{
    return bptree_iterator();
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
typename BP_tree<tkey, tvalue, compare, t>::bptree_const_iterator BP_tree<tkey, tvalue, compare, t>::begin() const
{
    return cbegin();
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
typename BP_tree<tkey, tvalue, compare, t>::bptree_const_iterator BP_tree<tkey, tvalue, compare, t>::end() const
{
    return cend();
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
typename BP_tree<tkey, tvalue, compare, t>::bptree_const_iterator BP_tree<tkey, tvalue, compare, t>::cbegin() const
{
    auto* leaf = leftmost_leaf();
    if (!leaf) {
        return bptree_const_iterator();
    }
    if (leaf->_data.empty()) {
        return cend();
    }
    return bptree_const_iterator(leaf, 0);
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
typename BP_tree<tkey, tvalue, compare, t>::bptree_const_iterator BP_tree<tkey, tvalue, compare, t>::cend() const
{
    return bptree_const_iterator();
}

// size/empty

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
size_t BP_tree<tkey, tvalue, compare, t>::size() const noexcept
{
    return _size;
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
bool BP_tree<tkey, tvalue, compare, t>::empty() const noexcept
{
    return _size == 0;
}

// element access

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
tvalue& BP_tree<tkey, tvalue, compare, t>::at(const tkey& key)
{
    auto* leaf = find_leaf(key);
    if (!leaf) {
        throw out_of_range("key not found");
    }
    for (size_t i = 0; i < leaf->_data.size(); ++i) {
        if (key_equal(leaf->_data[i].first, key)) {
            return leaf->_data[i].second;
        }
    }
    throw out_of_range("key not found");
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
const tvalue& BP_tree<tkey, tvalue, compare, t>::at(const tkey& key) const
{
    auto* leaf = find_leaf(key);
    if (!leaf) {
        throw out_of_range("key not found");
    }
    for (size_t i = 0; i < leaf->_data.size(); ++i) {
        if (key_equal(leaf->_data[i].first, key)) {
            return leaf->_data[i].second;
        }
    }
    throw out_of_range("key not found");
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
tvalue& BP_tree<tkey, tvalue, compare, t>::operator[](const tkey& key)
{
    auto it = find(key);
    if (it != end()) {
        return it->second;
    }
    auto result = emplace(key, tvalue{});
    return result.first->second;
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
tvalue& BP_tree<tkey, tvalue, compare, t>::operator[](tkey&& key)
{
    auto it = find(key);
    if (it != end()) {
        return it->second;
    }
    auto result = emplace(std::move(key), tvalue{});
    return result.first->second;
}

// lookup

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
typename BP_tree<tkey, tvalue, compare, t>::bptree_iterator BP_tree<tkey, tvalue, compare, t>::find(const tkey& key)
{
    auto* leaf = find_leaf(key);
    if (!leaf) {
        return end();
    }
    for (size_t i = 0; i < leaf->_data.size(); ++i) {
        if (key_equal(leaf->_data[i].first, key)) {
            return bptree_iterator(leaf, i);
        }
    }
    return end();
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
typename BP_tree<tkey, tvalue, compare, t>::bptree_const_iterator BP_tree<tkey, tvalue, compare, t>::find(const tkey& key) const
{
    auto* leaf = find_leaf(key);
    if (!leaf) {
        return cend();
    }
    for (size_t i = 0; i < leaf->_data.size(); ++i) {
        if (key_equal(leaf->_data[i].first, key)) {
            return bptree_const_iterator(leaf, i);
        }
    }
    return cend();
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
typename BP_tree<tkey, tvalue, compare, t>::bptree_iterator BP_tree<tkey, tvalue, compare, t>::lower_bound(const tkey& key)
{
    for (auto it = begin(); it != end(); ++it) {
        if (!compare_keys(it->first, key)) {
            return it;
        }
    }
    return end();
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
typename BP_tree<tkey, tvalue, compare, t>::bptree_const_iterator BP_tree<tkey, tvalue, compare, t>::lower_bound(const tkey& key) const
{
    for (auto it = cbegin(); it != cend(); ++it) {
        if (!compare_keys(it->first, key)) {
            return it;
        }
    }
    return cend();
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
typename BP_tree<tkey, tvalue, compare, t>::bptree_iterator BP_tree<tkey, tvalue, compare, t>::upper_bound(const tkey& key)
{
    auto it = lower_bound(key);
    if (it != end() && key_equal(it->first, key)) {
        ++it;
    }
    return it;
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
typename BP_tree<tkey, tvalue, compare, t>::bptree_const_iterator BP_tree<tkey, tvalue, compare, t>::upper_bound(const tkey& key) const
{
    auto it = lower_bound(key);
    if (it != cend() && key_equal(it->first, key)) {
        ++it;
    }
    return it;
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
bool BP_tree<tkey, tvalue, compare, t>::contains(const tkey& key) const
{
    return find(key) != end();
}

// modifiers

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
void BP_tree<tkey, tvalue, compare, t>::clear() noexcept
{
    destroy_subtree(_root);
    _root = nullptr;
    _size = 0;
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
std::pair<typename BP_tree<tkey, tvalue, compare, t>::bptree_iterator, bool>
BP_tree<tkey, tvalue, compare, t>::insert(const tree_data_type& data)
{
    auto it = find(data.first);
    if (it != end()) {
        return {it, false};
    }
    return emplace(data.first, data.second);
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
std::pair<typename BP_tree<tkey, tvalue, compare, t>::bptree_iterator, bool>
BP_tree<tkey, tvalue, compare, t>::insert(tree_data_type&& data)
{
    auto it = find(data.first);
    if (it != end()) {
        return {it, false};
    }
    return emplace(std::move(data.first), std::move(data.second));
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
template<typename... Args>
std::pair<typename BP_tree<tkey, tvalue, compare, t>::bptree_iterator, bool>
BP_tree<tkey, tvalue, compare, t>::emplace(Args&&... args)
{
    tree_data_type data(std::forward<Args>(args)...);
    tkey key = data.first;

    auto it = find(key);
    if (it != end()) {
        return {it, false};
    }

    if (!_root) {
        _root = _allocator.template new_object<bptree_node_term>();
        static_cast<bptree_node_term*>(_root)->_data.push_back(std::move(data));
        ++_size;
        return {bptree_iterator(static_cast<bptree_node_term*>(_root), 0), true};
    }

    auto* leaf = find_leaf(key);
    insert_into_leaf(leaf, std::move(data));

    if (leaf->_data.size() > maximum_keys_in_node) {
        bool do_3way = leaf->_data.size() > t + 1;

        if (do_3way) {
            bool root_is_leaf = is_term(_root);

            if (root_is_leaf) {
                auto* L1 = leaf;
                auto* L2 = _allocator.template new_object<bptree_node_term>();
                auto* L3 = _allocator.template new_object<bptree_node_term>();

                L2->_data.push_back(L1->_data[t]);
                for (size_t i = t + 1; i < L1->_data.size(); ++i) {
                    L3->_data.push_back(L1->_data[i]);
                }
                L1->_data.resize(t);

                L1->_next = L2;
                L2->_next = L3;
                L3->_next = nullptr;

                tkey k2 = L2->_data.front().first;
                tkey k3 = L3->_data.front().first;

                auto* new_root = _allocator.template new_object<bptree_node_middle>();
                new_root->_keys.push_back(k2);
                new_root->_keys.push_back(k3);
                new_root->_pointers.push_back(L1);
                new_root->_pointers.push_back(L2);
                new_root->_pointers.push_back(L3);
                _root = new_root;
            } else {
                auto* L1 = leaf;
                auto* L2 = _allocator.template new_object<bptree_node_term>();
                auto* L3 = _allocator.template new_object<bptree_node_term>();

                L2->_data.push_back(L1->_data[t]);
                for (size_t i = t + 1; i < L1->_data.size(); ++i) {
                    L3->_data.push_back(L1->_data[i]);
                }
                L1->_data.resize(t);

                L1->_next = L2;
                L2->_next = L3;
                L3->_next = nullptr;

                tkey k2 = L2->_data.front().first;
                tkey k3 = L3->_data.front().first;

                {
                    std::stack<std::pair<bptree_node_middle*, size_t>> path;
                    bptree_node_base* current = _root;
                    while (!is_term(current)) {
                        auto* mid = static_cast<bptree_node_middle*>(current);
                        size_t idx = 0;
                        while (idx < mid->_keys.size() && !compare_keys(k2, mid->_keys[idx])) {
                            ++idx;
                        }
                        path.push({mid, idx});
                        current = mid->_pointers[idx];
                    }
                    insert_into_parent(L1, L2, k2, path);
                }
                {
                    std::stack<std::pair<bptree_node_middle*, size_t>> path;
                    bptree_node_base* current = _root;
                    while (!is_term(current)) {
                        auto* mid = static_cast<bptree_node_middle*>(current);
                        size_t idx = 0;
                        while (idx < mid->_keys.size() && !compare_keys(k3, mid->_keys[idx])) {
                            ++idx;
                        }
                        path.push({mid, idx});
                        current = mid->_pointers[idx];
                    }
                    insert_into_parent(L2, L3, k3, path);
                }
            }
        } else {
            size_t middle = t;
            auto* new_leaf = _allocator.template new_object<bptree_node_term>();

            for (size_t i = middle; i < leaf->_data.size(); ++i) {
                new_leaf->_data.push_back(leaf->_data[i]);
            }
            leaf->_data.resize(middle);

            new_leaf->_next = leaf->_next;
            leaf->_next = new_leaf;

            tkey up_key = new_leaf->_data.front().first;

            std::stack<std::pair<bptree_node_middle*, size_t>> path;
            bptree_node_base* current = _root;
            while (!is_term(current)) {
                auto* mid = static_cast<bptree_node_middle*>(current);
                size_t idx = 0;
                while (idx < mid->_keys.size() && !compare_keys(up_key, mid->_keys[idx])) {
                    ++idx;
                }
                path.push({mid, idx});
                current = mid->_pointers[idx];
            }

            insert_into_parent(leaf, new_leaf, up_key, path);
        }
    }

    ++_size;
    return {find(key), true};
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
typename BP_tree<tkey, tvalue, compare, t>::bptree_iterator
BP_tree<tkey, tvalue, compare, t>::insert_or_assign(const tree_data_type& data)
{
    auto it = find(data.first);
    if (it != end()) {
        it->second = data.second;
        return it;
    }
    return emplace(data.first, data.second).first;
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
typename BP_tree<tkey, tvalue, compare, t>::bptree_iterator
BP_tree<tkey, tvalue, compare, t>::insert_or_assign(tree_data_type&& data)
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
typename BP_tree<tkey, tvalue, compare, t>::bptree_iterator
BP_tree<tkey, tvalue, compare, t>::emplace_or_assign(Args&&... args)
{
    tree_data_type data(std::forward<Args>(args)...);
    return insert_or_assign(std::move(data));
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
typename BP_tree<tkey, tvalue, compare, t>::bptree_iterator
BP_tree<tkey, tvalue, compare, t>::erase(bptree_iterator pos)
{
    if (pos == end()) {
        return end();
    }
    tkey key = pos->first;
    auto it = erase(key);
    return it;
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
typename BP_tree<tkey, tvalue, compare, t>::bptree_iterator
BP_tree<tkey, tvalue, compare, t>::erase(bptree_const_iterator pos)
{
    if (pos == cend()) {
        return end();
    }
    tkey key = pos->first;
    auto it = erase(key);
    return it;
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
typename BP_tree<tkey, tvalue, compare, t>::bptree_iterator
BP_tree<tkey, tvalue, compare, t>::erase(bptree_iterator beg, bptree_iterator en)
{
    while (beg != en) {
        beg = erase(beg);
    }
    return beg;
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
typename BP_tree<tkey, tvalue, compare, t>::bptree_iterator
BP_tree<tkey, tvalue, compare, t>::erase(bptree_const_iterator beg, bptree_const_iterator en)
{
    auto it = find(beg->first);
    auto end_it = en == cend() ? end() : find(en->first);
    while (it != end_it) {
        it = erase(it);
    }
    return it;
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
typename BP_tree<tkey, tvalue, compare, t>::bptree_iterator
BP_tree<tkey, tvalue, compare, t>::erase(const tkey& key)
{
    if (!_root) {
        return end();
    }

    auto* leaf = find_leaf(key);
    if (!leaf) {
        return end();
    }

    bool found = false;
    for (size_t i = 0; i < leaf->_data.size(); ++i) {
        if (key_equal(leaf->_data[i].first, key)) {
            found = true;
            break;
        }
    }
    if (!found) {
        return lower_bound(key);
    }

    std::stack<std::pair<bptree_node_middle*, size_t>> path;
    if (!is_term(_root)) {
        bptree_node_base* current = _root;
        while (!is_term(current)) {
            auto* mid = static_cast<bptree_node_middle*>(current);
            size_t idx = 0;
            while (idx < mid->_keys.size() && !compare_keys(key, mid->_keys[idx])) {
                ++idx;
            }
            path.push({mid, idx});
            current = mid->_pointers[idx];
        }
    }

    remove_from_leaf(leaf, key, path);

    --_size;

    if (_root && is_term(_root)) {
        auto* rleaf = static_cast<bptree_node_term*>(_root);
        if (rleaf->_data.empty()) {
            _allocator.delete_object(rleaf);
            _root = nullptr;
        }
    }

    if (_root && !is_term(_root)) {
        auto* rmid = static_cast<bptree_node_middle*>(_root);
        if (rmid->_pointers.size() == 1) {
            bptree_node_base* old = _root;
            _root = rmid->_pointers.front();
            _allocator.delete_object(old);
        }
    }

    return lower_bound(key);
}

#endif

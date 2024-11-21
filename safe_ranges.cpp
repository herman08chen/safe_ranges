#include <atomic>
#include <ranges>
#include "safe_iterator.cpp"

namespace safe_ranges {
    template<std::ranges::range R>
    class view : public std::ranges::view_interface<view<R>>{
    public:
        typedef std::ranges::range_value_t<R> value_type;
        typedef std::size_t size_type;
        typedef value_type& reference;
        typedef const value_type& const_reference;
        typedef iterator_t<R> iterator;
        typedef iterator_t<const R> const_iterator;
        typedef std::reverse_iterator<iterator> reverse_iterator;
        typedef std::reverse_iterator<const_iterator> const_reverse_iterator;
        typedef sentinel_t<R> sentinel;
        typedef sentinel_t<const R> const_sentinel;


        explicit view(const R& Range, control_block* Control) : Range{&Range}, Control {Control} { Control->ref_count++; }
        view(const view& other) : view{other.Range, other.Control} {}

        ~view() {
            Control->release();
        }

        const_iterator begin() const { return iterator_t{std::ranges::cbegin(*Range), *Range, Control}; }
        const_iterator cbegin() const { return begin(); }
        const_sentinel end() const { return const_sentinel{std::ranges::cend(*Range), *Range}; }
        const_sentinel cend() const { return end(); }

        auto rbegin() const { return std::ranges::views::reverse(*this).cbegin(); }
        auto crbegin() const { return rbegin(); }
        auto rend() const { return std::ranges::views::reverse(*this).cend();; }
        auto crend() const { return rend(); }

        template<class U = void>
        requires requires (R Range) { std::ranges::empty(Range); }
        [[nodiscard]] bool empty() const { return std::ranges::empty(Range); }

        template<class U = void>
        requires requires (R Range) { std::ranges::empty(Range); }
        auto& front() const {
            if(empty()) [[unlikely]] throw std::out_of_range("out of range");
            return *begin();
        }
        template<class U = void>
        requires requires (R Range) { std::ranges::empty(Range); }
        auto& back() const {
            if(empty()) [[unlikely]] throw std::out_of_range("out of range");
            return *rbegin();
        }

        template<class U = void>
        requires requires (R Range) { std::ranges::empty(Range); }
        explicit operator bool() const { return !empty(); }

        template<class U = void>
        requires std::ranges::sized_range<R>
        [[nodiscard]] size_type size() const { return std::ranges::size(*Range); }

        template<class U = void>
        requires std::ranges::random_access_range<R>
        const_reference operator[](size_type pos) {
            if(pos >= size()) [[unlikely]] throw std::out_of_range("out of range");
            return (*Range)[pos];
        }

    private:
        const R* Range;
        control_block* Control;
    };

    template<std::ranges::range R>
    class range {
    public:
        typedef std::ranges::range_value_t<R> value_type;
        typedef std::size_t size_type;
        typedef value_type& reference;
        typedef const value_type& const_reference;
        typedef iterator_t<std::ranges::iterator_t<R>> iterator;
        typedef iterator_t<std::ranges::const_iterator_t<R>> const_iterator;
        typedef std::reverse_iterator<iterator> reverse_iterator;
        typedef std::reverse_iterator<const_iterator> const_reverse_iterator;
        typedef sentinel_t<R> sentinel;
        typedef sentinel_t<const R> const_sentinel;

        explicit range(const R& rng) : Range(rng), Control{new control_block{0, 1}} {}
        explicit range(R&& rng) : Range(std::move(rng)), Control{new control_block{0, 1}} {}
        ~range() {
            Control->gen_count++; //no views/iterators are on this generation
            Control->release();
        }
        auto& operator=(auto&& other) {
            Control->gen_count++;
            Range = std::forward<decltype(other)>(other);
            return *this;
        }


        auto begin() { return iterator_t{std::ranges::begin(Range), Range, Control}; }
        auto begin() const { return const_iterator{std::ranges::cbegin(Range), Range, Control}; }
        auto cbegin() const { return begin(); }
        auto end() { return sentinel_t{std::ranges::end(Range), Range}; }
        auto end() const { return const_sentinel{std::ranges::cend(Range), Range}; }
        auto cend() const { return end(); }

        auto rbegin() { return std::ranges::views::reverse(*this).begin(); }
        auto rbegin() const { return std::ranges::views::reverse(*this).cbegin(); }
        auto crbegin() const { return rbegin(); }
        auto rend() { return std::ranges::views::reverse(*this).end();; }
        auto rend() const { return std::ranges::views::reverse(*this).cend();; }
        auto crend() const { return rend(); }

        template<class U = void>
        requires std::ranges::sized_range<R>
        [[nodiscard]] size_type size() const { return std::ranges::size(Range); }

        template<class U = void>
        requires std::ranges::random_access_range<R>
        const_reference operator[](size_type pos) const {
            if(pos >= size()) [[unlikely]] throw std::out_of_range("out of range");
            return Range[pos];
        }
        template<class U = void>
        requires std::ranges::random_access_range<R>
        reference operator[](const size_type pos) {
            return operator[](pos);
        }

        auto view() const { return safe_ranges::view{Range}; }
        auto& get_unchecked() { return Range; }
        auto& get() {
            ++Control->gen_count;
            return Range;
        }

        //Common Member Functions
        template<class U = void>
        requires requires (R Range) { std::ranges::empty(Range); }
        [[nodiscard]] bool empty() const { return std::ranges::empty(Range); }

        template<class U = void>
        requires requires (R Range) { std::ranges::empty(Range); }
        auto& front() const {
            if(empty()) [[unlikely]] throw std::out_of_range("out of range");
            return *begin();
        }
        template<class U = void>
        requires requires (R Range) { std::ranges::empty(Range); }
        auto& back() const {
            if(empty()) [[unlikely]] throw std::out_of_range("out of range");
            return *rbegin();
        }
        template<class U = void>
        requires std::ranges::contiguous_range<R>
        auto data() const { return std::ranges::data(Range); }

    private:
        R Range;
        control_block* Control;
    };
};
#include <atomic>
#include <ranges>

namespace safe_ranges {
    struct control_block {
        std::atomic<std::size_t> gen_count;
        std::atomic<std::size_t> ref_count;
        void release() {
            ref_count--;
            if (ref_count == 0) delete this;
        }
    };
    template<class It, class R>
    class iter {
    public:
        typedef It iterator_type;
        explicit iter(iterator_type it, R& Range, control_block* Control) : it{it}, Range{Range}, Control{Control}, Gen{Control->gen_count} {
            Control->ref_count++;
        }
        iter(const iter& other) : iter {other.it, other.Range, other.Control} {}

        ~iter() {
            Control->release();
        }

        operator iterator_type() { return it; }

        iter& operator++() {
            if(it == std::ranges::end(Range)) [[unlikely]] throw std::out_of_range("incremented past end");
            ++it;
            return *this;
        }
        iter operator++(int) {
            iter old = *this;
            operator++();
            return old;
        }
        iter& operator--() {
            if(it == std::ranges::begin(Range)) [[unlikely]] throw std::out_of_range("decremented begin");
            --it;
            return *this;
        }
        iter operator--(int) {
            iter old = *this;
            operator--();
            return old;
        }

        template<class U = void>
        requires std::ranges::random_access_range<R>
        friend iter operator+(iter lhs, const int& i) {
            const auto current_index = std::distance(std::ranges::begin(lhs.Range), lhs.it);
            if( current_index + i >= std::ranges::size(lhs.Range) | current_index + i < 0) [[unlikely]] throw std::out_of_range("out of range");
            return iter{lhs.it + i, lhs.Range};
        }
        template<class U = void>
        requires std::ranges::random_access_range<R>
        friend iter operator-(iter lhs, const int& i) {
            return operator+(lhs, -i);
        }
        template<class U = void>
        requires std::ranges::random_access_range<R>
        iter& operator+=(const int& i) {
            *this = *this + i;
            return *this;
        }
        template<class U = void>
        requires std::ranges::random_access_range<R>
        iter& operator-=(const int& i) {
            return operator+=(-i);
        }
        bool operator==(const iter& rhs) const { return it == rhs.it; };
        bool operator!=(const iter& rhs) const { return it != rhs.it; };

        auto& operator*() {
            if(Gen != Control->gen_count) [[unlikely]] throw std::out_of_range("iterator invalidated");
            if(it == std::ranges::end(Range)) [[unlikely]] throw std::out_of_range("dereferenced past end");
            return *it;
        }
        auto& operator[](std::size_t pos) {
            return *(*this + pos);
        }

    private:
        iterator_type it;
        R& Range;
        control_block* Control;
        std::size_t Gen;
    };

    template<std::ranges::range R>
    class view {
    public:
        typedef std::ranges::range_value_t<R> value_type;
        typedef std::size_t size_type;
        typedef value_type& reference;
        typedef const value_type& const_reference;
        typedef iter<std::ranges::iterator_t<R>, R> iterator;
        typedef iter<std::ranges::const_iterator_t<R>, R> const_iterator;
        typedef std::reverse_iterator<iterator> reverse_iterator;
        typedef std::reverse_iterator<const_iterator> const_reverse_iterator;

        explicit view(const R& Range, control_block* Control) : Range{Range}, Control {Control} { Control->ref_count++; }
        view(const view& other) : view{other.Range, other.Control} {}

        ~view() {
            Control->release();
        }

        const_iterator begin() const { return iter{std::ranges::cbegin(Range), const_cast<R&>(Range), Control}; }
        const_iterator cbegin() const { return begin(); }
        const_iterator end() const { return iter{std::ranges::cend(Range), const_cast<R&>(Range), Control}; }
        const_iterator cend() const { return end(); }
        const_reverse_iterator rbegin() const { return reverse_iterator{std::ranges::crbegin(Range), const_cast<R&>(Range), Control}; }
        const_reverse_iterator crbegin() const { return rbegin(); }
        const_reverse_iterator rend() const { return reverse_iterator{std::ranges::crend(Range), const_cast<R&>(Range), Control}; }
        const_reverse_iterator crend() const { return rend(); }

        template<class U = void>
        requires std::ranges::sized_range<R>
        [[nodiscard]] size_type size() const { return std::ranges::size(Range); }

        template<class U = void>
        requires std::ranges::random_access_range<R>
        const_reference operator[](size_type pos) {
            if(pos >= size()) [[unlikely]] throw std::out_of_range("out of range");
            return Range[pos];
        }

    private:
        const R& Range;
        control_block* Control;
    };

    template<std::ranges::range R>
    class range {
    public:
        typedef std::ranges::range_value_t<R> value_type;
        typedef std::size_t size_type;
        typedef value_type& reference;
        typedef const value_type& const_reference;
        typedef iter<std::ranges::iterator_t<R>, R> iterator;
        typedef iter<std::ranges::const_iterator_t<R>, R> const_iterator;
        typedef std::reverse_iterator<iterator> reverse_iterator;
        typedef std::reverse_iterator<const_iterator> const_reverse_iterator;


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


        iterator begin() { return iter{std::ranges::begin(Range), Range, Control}; }
        const_iterator begin() const { return iter{std::ranges::cbegin(Range), Range, Control}; }
        const_iterator cbegin() const { return begin(); }
        iterator end() { return iter{std::ranges::end(Range), Range, Control}; }
        const_iterator end() const { return iter{std::ranges::cend(Range), Range, Control}; }
        const_iterator cend() const { return end(); }
        reverse_iterator rbegin() { return reverse_iterator{std::ranges::rbegin(Range), Range, Control}; }
        const_reverse_iterator rbegin() const { return reverse_iterator{std::ranges::crbegin(Range), Range, Control}; }
        const_reverse_iterator crbegin() const { return rbegin(); }
        reverse_iterator rend() { return reverse_iterator{std::ranges::rend(Range), Range, Control}; }
        const_reverse_iterator rend() const { return reverse_iterator{std::ranges::crend(Range), Range, Control}; }
        const_reverse_iterator crend() const { return rend(); }

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
        auto& get() { return Range; }
        auto& get_iter_inv() {
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
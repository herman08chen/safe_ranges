#include <atomic>
struct control_block {
    std::atomic<std::size_t> gen_count;
    std::atomic<std::size_t> ref_count;
    void release() {
        ref_count--;
        if (ref_count == 0) delete this;
    }
};
#ifndef MEM_MGR_H
#define MEM_MGR_H

#include <cstddef>
#include <string>
#include <vector>

class Mem_mgr {
public:
    static constexpr std::size_t kInvalidAddress = static_cast<std::size_t>(-1);

    struct SegmentSnapshot {
        std::size_t start = 0;
        std::size_t size = 0;
        bool free = true;
        std::string label;
    };

    explicit Mem_mgr(std::size_t total_bytes = 4096);

    void reset();

    std::size_t allocate(std::size_t bytes, const std::string& label = "");
    bool free_block(std::size_t address);

    bool write(std::size_t address, const void* data, std::size_t bytes);
    bool read(std::size_t address, void* destination, std::size_t bytes) const;

    bool owns(std::size_t address) const;
    std::size_t capacity() const;
    std::size_t used_bytes() const;
    std::size_t free_bytes() const;
    std::size_t allocation_count() const;

    std::vector<SegmentSnapshot> snapshot_segments() const;
    std::string describe_layout() const;
    void dump(int level = 0) const;

private:
    struct Segment {
        std::size_t start = 0;
        std::size_t size = 0;
        bool free = true;
        std::string label;
    };

    std::vector<unsigned char> heap_bytes;
    std::vector<Segment> segments;
    std::size_t used_byte_count = 0;

    std::size_t find_segment_index(std::size_t address) const;
    void coalesce_free_segments();
    bool range_within_allocated_segment(std::size_t address, std::size_t bytes) const;
};

using MemoryManager = Mem_mgr;

#endif // MEM_MGR_H

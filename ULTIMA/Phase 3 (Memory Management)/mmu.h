/* =========================================================================
 * mmu.h — Phase 3 memory-management interface
 * =========================================================================
 * Team Thunder #001
 *
 * Team Members: Stewart Pawley, Zander Hayes, Nicholas Kobs
 *
 * Workflow Ownership:
 *   Stewart Pawley - MMU interface contract, protection rules,
 *                    semaphore integration contract, final merge
 *   Zander Hayes   - allocator data model, segment/free-space structure
 *   Nicholas Kobs  - read/write API contract, dump interface,
 *                    failure-case evidence requirements
 * =========================================================================
 */

#ifndef ULTIMA_PHASE3_MMU_H
#define ULTIMA_PHASE3_MMU_H

#include <deque>
#include <list>
#include <mutex>
#include <string>
#include <unordered_map>
#include <vector>

class Scheduler;
class Semaphore;

class mmu {
public:
    struct SegmentSnapshot {
        int segment_id = 0;
        bool free = true;
        int handle = -1;
        int owner_task_id = -1;
        int start = 0;
        int end = -1;
        int size = 0;
        int requested_size = 0;
        int current_location = -1;
    };

    struct HandleSnapshot {
        int handle = -1;
        int owner_task_id = -1;
        int base = 0;
        int limit = 0;
        int requested_size = 0;
        int reserved_size = 0;
        int current_location = 0;
        bool active = false;
    };

    explicit mmu(
        int size = 1024,
        char default_initial_value = '.',
        int page_size = 64,
        Scheduler* scheduler = nullptr,
        Semaphore* core_semaphore = nullptr
    );
    ~mmu() = default;

    int init(Scheduler* scheduler, Semaphore* core_semaphore);
    void reset();

    int Mem_Alloc(int size);
    int Mem_Free(int memory_handle);

    int Mem_Read(int memory_handle, char* ch);
    int Mem_Write(int memory_handle, char ch);
    int Mem_Read(int memory_handle, int offset_from_beg, int text_size, char* text);
    int Mem_Write(int memory_handle, int offset_from_beg, int text_size, const char* text);

    int Mem_Left() const;
    int Mem_Largest() const;
    int Mem_Smallest() const;
    int Mem_Coalesce();
    int Mem_Dump(int starting_from = 0, int num_bytes = -1) const;

    int capacity() const;
    int page_size() const;
    int waiting_task_count() const;
    int get_handle_owner(int memory_handle) const;
    int get_handle_base(int memory_handle) const;
    int get_handle_limit(int memory_handle) const;
    int get_handle_current_offset(int memory_handle) const;

    std::vector<SegmentSnapshot> snapshot_segments() const;
    std::vector<HandleSnapshot> snapshot_handles() const;

    std::string layout_table_string() const;
    std::string memory_char_dump_string(
        int starting_from = 0,
        int num_bytes = -1,
        int bytes_per_line = 64
    ) const;
    std::string memory_hex_dump_string(
        int starting_from = 0,
        int num_bytes = -1,
        int bytes_per_line = 16
    ) const;
    std::string pretty_dump_string(int starting_from = 0, int num_bytes = -1) const;
    std::string describe_handle(int memory_handle) const;

    std::string get_last_event() const;
    std::string get_last_pre_coalesce_dump() const;
    std::string get_last_post_coalesce_dump() const;

private:
    struct Segment {
        int segment_id = 0;
        int start = 0;
        int size = 0;
        bool free = true;
        int handle = -1;
        int owner_task_id = -1;
        int requested_size = 0;
        int current_location = -1;
    };

    struct AllocationRecord {
        int handle = -1;
        int owner_task_id = -1;
        int base = 0;
        int reserved_size = 0;
        int requested_size = 0;
        int current_location = 0;
        bool active = false;
    };

    std::vector<unsigned char> core_memory_;
    std::list<Segment> segments_;
    std::unordered_map<int, AllocationRecord> allocations_;
    std::deque<int> waiting_tasks_;
    Scheduler* scheduler_;
    Semaphore* core_semaphore_;
    char default_initial_value_;
    int page_size_;
    int next_segment_id_;
    int next_generation_;
    mutable std::mutex guard_;
    std::string last_event_;
    std::string last_pre_coalesce_dump_;
    std::string last_post_coalesce_dump_;

    int current_task_id() const;
    int make_handle(int owner_task_id);
    int reserve_size_for_request(int size) const;
    int clamp_dump_start(int starting_from) const;
    int clamp_dump_count(int starting_from, int num_bytes) const;
    bool validate_access(const AllocationRecord& allocation, int offset, int length) const;
    void fill_memory(int start, int size, unsigned char value);
    void note_event(const std::string& message);
    void wake_waiting_tasks();
    void enter_core_region();
    void leave_core_region();

    std::list<Segment>::iterator find_segment_by_handle(int memory_handle);
    std::list<Segment>::const_iterator find_segment_by_handle(int memory_handle) const;
    std::unordered_map<int, AllocationRecord>::iterator find_allocation(int memory_handle);
    std::unordered_map<int, AllocationRecord>::const_iterator find_allocation(int memory_handle) const;

    int coalesce_locked();
    int free_bytes_locked() const;
    int largest_free_locked() const;
    int smallest_free_locked() const;
    std::vector<SegmentSnapshot> snapshot_segments_locked() const;
    std::vector<HandleSnapshot> snapshot_handles_locked() const;
    std::string layout_table_string_locked() const;
    std::string memory_char_dump_string_locked(int starting_from, int num_bytes, int bytes_per_line) const;
    std::string memory_hex_dump_string_locked(int starting_from, int num_bytes, int bytes_per_line) const;
    std::string pretty_dump_string_locked(int starting_from, int num_bytes) const;
};

using MMU = mmu;

#endif // ULTIMA_PHASE3_MMU_H

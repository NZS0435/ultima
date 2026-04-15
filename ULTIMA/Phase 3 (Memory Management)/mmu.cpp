/* =========================================================================
 * mmu.cpp — Phase 3 memory-management implementation
 * =========================================================================
 * Team Thunder #001
 *
 * Team Members: Stewart Pawley, Zander Hayes, Nicholas Kobs
 *
 * Workflow Ownership:
 *   Stewart Pawley - protection checks, memory semaphore integration,
 *                    blocked-task / wake-up behavior, final merge
 *   Zander Hayes   - allocator engine, first-fit splitting,
 *                    coalescing, free-space accounting
 *   Nicholas Kobs  - single-byte and ranged I/O paths,
 *                    char/HEX dumps, failure-case evidence hooks
 * =========================================================================
 */

#include "mmu.h"

#include "../Phase 2 (IPC)/Sched.h"
#include "../Phase 1 (Scheduler)/Sema.h"

#include <algorithm>
#include <cctype>
#include <cstring>
#include <iomanip>
#include <iostream>
#include <iterator>
#include <sstream>

namespace {

int round_up_to_multiple(int value, int block_size) {
    if (value <= 0) {
        return 0;
    }

    const int remainder = value % block_size;
    return (remainder == 0) ? value : (value + (block_size - remainder));
}

char printable_or_placeholder(unsigned char value) {
    return std::isprint(static_cast<int>(value)) ? static_cast<char>(value) : '.';
}

std::string task_label(int task_id) {
    return (task_id >= 0) ? ("T-" + std::to_string(task_id)) : "SYS";
}

} // namespace

mmu::mmu(int size, char default_initial_value, int page_size, Scheduler* scheduler, Semaphore* core_semaphore) :
    core_memory_(static_cast<std::size_t>(std::max(size, 0)), static_cast<unsigned char>(default_initial_value)),
    scheduler_(scheduler),
    core_semaphore_(core_semaphore),
    default_initial_value_(default_initial_value),
    page_size_(std::max(page_size, 1)),
    next_segment_id_(1),
    next_generation_(1),
    last_event_("MMU initialized."),
    last_pre_coalesce_dump_(),
    last_post_coalesce_dump_() {
    reset();
}

int mmu::init(Scheduler* scheduler, Semaphore* core_semaphore) {
    std::lock_guard<std::mutex> guard(guard_);
    scheduler_ = scheduler;
    core_semaphore_ = core_semaphore;
    note_event("MMU runtime links refreshed.");
    return 1;
}

void mmu::reset() {
    std::lock_guard<std::mutex> guard(guard_);

    std::fill(
        core_memory_.begin(),
        core_memory_.end(),
        static_cast<unsigned char>(default_initial_value_)
    );

    allocations_.clear();
    waiting_tasks_.clear();
    segments_.clear();
    next_segment_id_ = 1;
    next_generation_ = 1;
    last_pre_coalesce_dump_.clear();
    last_post_coalesce_dump_.clear();

    if (!core_memory_.empty()) {
        segments_.push_back(Segment {
            next_segment_id_++,
            0,
            static_cast<int>(core_memory_.size()),
            true,
            -1,
            -1,
            0,
            -1
        });
    }

    note_event(
        "MMU reset to one free segment of "
        + std::to_string(core_memory_.size())
        + " bytes with block size "
        + std::to_string(page_size_)
        + "."
    );
}

int mmu::Mem_Alloc(int size) {
    std::lock_guard<std::mutex> guard(guard_);
    enter_core_region();

    if (size <= 0) {
        note_event("Mem_Alloc rejected non-positive request.");
        leave_core_region();
        return -1;
    }

    const int owner_task = current_task_id();
    const int reserved_size = reserve_size_for_request(size);

    for (auto segment_it = segments_.begin(); segment_it != segments_.end(); ++segment_it) {
        if (!segment_it->free || segment_it->size < reserved_size) {
            continue;
        }

        const int handle = make_handle(owner_task);
        const int original_size = segment_it->size;
        const int base = segment_it->start;
        const int remainder = original_size - reserved_size;

        if (remainder > 0) {
            segments_.insert(
                std::next(segment_it),
                Segment {
                    next_segment_id_++,
                    base + reserved_size,
                    remainder,
                    true,
                    -1,
                    -1,
                    0,
                    -1
                }
            );
        }

        segment_it->size = reserved_size;
        segment_it->free = false;
        segment_it->handle = handle;
        segment_it->owner_task_id = owner_task;
        segment_it->requested_size = size;
        segment_it->current_location = 0;

        allocations_[handle] = AllocationRecord {
            handle,
            owner_task,
            base,
            reserved_size,
            size,
            0,
            true
        };

        waiting_tasks_.erase(
            std::remove(waiting_tasks_.begin(), waiting_tasks_.end(), owner_task),
            waiting_tasks_.end()
        );

        fill_memory(base, reserved_size, static_cast<unsigned char>(default_initial_value_));

        note_event(
            "Mem_Alloc granted " + std::to_string(size)
            + " requested bytes (" + std::to_string(reserved_size)
            + " reserved) to " + task_label(owner_task)
            + " at base " + std::to_string(base)
            + " with handle 0x"
            + [&handle]() {
                std::ostringstream out;
                out << std::uppercase << std::hex << handle;
                return out.str();
            }()
            + "."
        );

        leave_core_region();
        return handle;
    }

    if (owner_task >= 0 && scheduler_ != nullptr) {
        if (std::find(waiting_tasks_.begin(), waiting_tasks_.end(), owner_task) == waiting_tasks_.end()) {
            waiting_tasks_.push_back(owner_task);
        }
        scheduler_->block_task(owner_task);
    }

    note_event(
        "Mem_Alloc could not satisfy " + std::to_string(size)
        + " requested bytes (" + std::to_string(reserved_size)
        + " reserved) for " + task_label(owner_task)
        + "; largest free segment is " + std::to_string(largest_free_locked()) + " bytes."
    );

    leave_core_region();
    return -1;
}

int mmu::Mem_Free(int memory_handle) {
    std::lock_guard<std::mutex> guard(guard_);
    enter_core_region();

    auto allocation_it = find_allocation(memory_handle);
    if (allocation_it == allocations_.end()) {
        note_event("Mem_Free failed because handle was not found.");
        leave_core_region();
        return -1;
    }

    const AllocationRecord allocation = allocation_it->second;
    const int actor_task = current_task_id();
    if (actor_task >= 0 && actor_task != allocation.owner_task_id) {
        note_event(
            "Segmentation fault prevented Mem_Free on handle 0x"
            + [&memory_handle]() {
                std::ostringstream out;
                out << std::uppercase << std::hex << memory_handle;
                return out.str();
            }()
            + " by " + task_label(actor_task)
            + " because it belongs to " + task_label(allocation.owner_task_id) + "."
        );
        leave_core_region();
        return -1;
    }

    auto segment_it = find_segment_by_handle(memory_handle);
    if (segment_it == segments_.end()) {
        note_event("Mem_Free failed because the segment ledger was inconsistent.");
        leave_core_region();
        return -1;
    }

    fill_memory(
        allocation.base,
        allocation.reserved_size,
        static_cast<unsigned char>('#')
    );

    segment_it->free = true;
    segment_it->handle = -1;
    segment_it->owner_task_id = -1;
    segment_it->requested_size = 0;
    segment_it->current_location = -1;
    allocations_.erase(allocation_it);

    last_pre_coalesce_dump_ = pretty_dump_string_locked(0, static_cast<int>(core_memory_.size()));
    const int merges = coalesce_locked();
    last_post_coalesce_dump_ = pretty_dump_string_locked(0, static_cast<int>(core_memory_.size()));
    wake_waiting_tasks();

    note_event(
        "Mem_Free released handle 0x"
        + [&memory_handle]() {
            std::ostringstream out;
            out << std::uppercase << std::hex << memory_handle;
            return out.str();
        }()
        + " from " + task_label(allocation.owner_task_id)
        + "; Mem_Coalesce performed " + std::to_string(merges) + " merge(s)."
    );

    leave_core_region();
    return 1;
}

int mmu::Mem_Read(int memory_handle, char* ch) {
    std::lock_guard<std::mutex> guard(guard_);
    if (ch == nullptr) {
        note_event("Mem_Read failed because the destination pointer was null.");
        return -1;
    }

    auto allocation_it = find_allocation(memory_handle);
    if (allocation_it == allocations_.end()) {
        note_event("Mem_Read failed because handle was not found.");
        return -1;
    }

    AllocationRecord& allocation = allocation_it->second;
    if (!validate_access(allocation, allocation.current_location, 1)) {
        note_event(
            "Segmentation fault prevented single-byte read on handle 0x"
            + [&memory_handle]() {
                std::ostringstream out;
                out << std::uppercase << std::hex << memory_handle;
                return out.str();
            }()
            + "."
        );
        return -1;
    }

    *ch = static_cast<char>(core_memory_[static_cast<std::size_t>(allocation.base + allocation.current_location)]);
    ++allocation.current_location;

    auto segment_it = find_segment_by_handle(memory_handle);
    if (segment_it != segments_.end()) {
        segment_it->current_location = allocation.current_location;
    }

    note_event(
        "Mem_Read returned one byte from handle 0x"
        + [&memory_handle]() {
            std::ostringstream out;
            out << std::uppercase << std::hex << memory_handle;
            return out.str();
        }()
        + " at current location " + std::to_string(allocation.current_location - 1) + "."
    );
    return 1;
}

int mmu::Mem_Write(int memory_handle, char ch) {
    std::lock_guard<std::mutex> guard(guard_);

    auto allocation_it = find_allocation(memory_handle);
    if (allocation_it == allocations_.end()) {
        note_event("Mem_Write failed because handle was not found.");
        return -1;
    }

    AllocationRecord& allocation = allocation_it->second;
    if (!validate_access(allocation, allocation.current_location, 1)) {
        note_event(
            "Segmentation fault prevented single-byte write on handle 0x"
            + [&memory_handle]() {
                std::ostringstream out;
                out << std::uppercase << std::hex << memory_handle;
                return out.str();
            }()
            + "."
        );
        return -1;
    }

    core_memory_[static_cast<std::size_t>(allocation.base + allocation.current_location)] =
        static_cast<unsigned char>(ch);
    ++allocation.current_location;

    auto segment_it = find_segment_by_handle(memory_handle);
    if (segment_it != segments_.end()) {
        segment_it->current_location = allocation.current_location;
    }

    note_event(
        "Mem_Write stored one byte into handle 0x"
        + [&memory_handle]() {
            std::ostringstream out;
            out << std::uppercase << std::hex << memory_handle;
            return out.str();
        }()
        + " at current location " + std::to_string(allocation.current_location - 1) + "."
    );
    return 1;
}

int mmu::Mem_Read(int memory_handle, int offset_from_beg, int text_size, char* text) {
    std::lock_guard<std::mutex> guard(guard_);

    if (text == nullptr && text_size > 0) {
        note_event("Mem_Read range failed because the destination pointer was null.");
        return -1;
    }

    auto allocation_it = find_allocation(memory_handle);
    if (allocation_it == allocations_.end()) {
        note_event("Mem_Read range failed because handle was not found.");
        return -1;
    }

    const AllocationRecord& allocation = allocation_it->second;
    if (!validate_access(allocation, offset_from_beg, text_size)) {
        note_event(
            "Segmentation fault prevented ranged read on handle 0x"
            + [&memory_handle]() {
                std::ostringstream out;
                out << std::uppercase << std::hex << memory_handle;
                return out.str();
            }()
            + "."
        );
        return -1;
    }

    std::memcpy(
        text,
        core_memory_.data() + allocation.base + offset_from_beg,
        static_cast<std::size_t>(text_size)
    );

    note_event(
        "Mem_Read copied " + std::to_string(text_size)
        + " byte(s) from handle 0x"
        + [&memory_handle]() {
            std::ostringstream out;
            out << std::uppercase << std::hex << memory_handle;
            return out.str();
        }()
        + " starting at offset " + std::to_string(offset_from_beg) + "."
    );
    return text_size;
}

int mmu::Mem_Write(int memory_handle, int offset_from_beg, int text_size, const char* text) {
    std::lock_guard<std::mutex> guard(guard_);

    if (text == nullptr && text_size > 0) {
        note_event("Mem_Write range failed because the source pointer was null.");
        return -1;
    }

    auto allocation_it = find_allocation(memory_handle);
    if (allocation_it == allocations_.end()) {
        note_event("Mem_Write range failed because handle was not found.");
        return -1;
    }

    const AllocationRecord& allocation = allocation_it->second;
    if (!validate_access(allocation, offset_from_beg, text_size)) {
        note_event(
            "Segmentation fault prevented ranged write on handle 0x"
            + [&memory_handle]() {
                std::ostringstream out;
                out << std::uppercase << std::hex << memory_handle;
                return out.str();
            }()
            + "."
        );
        return -1;
    }

    std::memcpy(
        core_memory_.data() + allocation.base + offset_from_beg,
        text,
        static_cast<std::size_t>(text_size)
    );

    note_event(
        "Mem_Write copied " + std::to_string(text_size)
        + " byte(s) into handle 0x"
        + [&memory_handle]() {
            std::ostringstream out;
            out << std::uppercase << std::hex << memory_handle;
            return out.str();
        }()
        + " starting at offset " + std::to_string(offset_from_beg) + "."
    );
    return text_size;
}

int mmu::Mem_Left() const {
    std::lock_guard<std::mutex> guard(guard_);
    return free_bytes_locked();
}

int mmu::Mem_Largest() const {
    std::lock_guard<std::mutex> guard(guard_);
    return largest_free_locked();
}

int mmu::Mem_Smallest() const {
    std::lock_guard<std::mutex> guard(guard_);
    return smallest_free_locked();
}

int mmu::Mem_Coalesce() {
    std::lock_guard<std::mutex> guard(guard_);
    const int merges = coalesce_locked();
    note_event("Mem_Coalesce completed " + std::to_string(merges) + " merge(s).");
    return merges;
}

int mmu::Mem_Dump(int starting_from, int num_bytes) const {
    std::lock_guard<std::mutex> guard(guard_);
    const int dumped_bytes = clamp_dump_count(starting_from, num_bytes);
    std::cout << pretty_dump_string_locked(starting_from, num_bytes);
    return dumped_bytes;
}

int mmu::capacity() const {
    std::lock_guard<std::mutex> guard(guard_);
    return static_cast<int>(core_memory_.size());
}

int mmu::page_size() const {
    std::lock_guard<std::mutex> guard(guard_);
    return page_size_;
}

int mmu::waiting_task_count() const {
    std::lock_guard<std::mutex> guard(guard_);
    return static_cast<int>(waiting_tasks_.size());
}

int mmu::get_handle_owner(int memory_handle) const {
    std::lock_guard<std::mutex> guard(guard_);
    auto it = find_allocation(memory_handle);
    return (it != allocations_.end()) ? it->second.owner_task_id : -1;
}

int mmu::get_handle_base(int memory_handle) const {
    std::lock_guard<std::mutex> guard(guard_);
    auto it = find_allocation(memory_handle);
    return (it != allocations_.end()) ? it->second.base : -1;
}

int mmu::get_handle_limit(int memory_handle) const {
    std::lock_guard<std::mutex> guard(guard_);
    auto it = find_allocation(memory_handle);
    return (it != allocations_.end()) ? it->second.reserved_size : -1;
}

int mmu::get_handle_current_offset(int memory_handle) const {
    std::lock_guard<std::mutex> guard(guard_);
    auto it = find_allocation(memory_handle);
    return (it != allocations_.end()) ? it->second.current_location : -1;
}

std::vector<mmu::SegmentSnapshot> mmu::snapshot_segments() const {
    std::lock_guard<std::mutex> guard(guard_);
    return snapshot_segments_locked();
}

std::vector<mmu::HandleSnapshot> mmu::snapshot_handles() const {
    std::lock_guard<std::mutex> guard(guard_);
    return snapshot_handles_locked();
}

std::string mmu::layout_table_string() const {
    std::lock_guard<std::mutex> guard(guard_);
    return layout_table_string_locked();
}

std::string mmu::memory_char_dump_string(int starting_from, int num_bytes, int bytes_per_line) const {
    std::lock_guard<std::mutex> guard(guard_);
    return memory_char_dump_string_locked(starting_from, num_bytes, bytes_per_line);
}

std::string mmu::memory_hex_dump_string(int starting_from, int num_bytes, int bytes_per_line) const {
    std::lock_guard<std::mutex> guard(guard_);
    return memory_hex_dump_string_locked(starting_from, num_bytes, bytes_per_line);
}

std::string mmu::pretty_dump_string(int starting_from, int num_bytes) const {
    std::lock_guard<std::mutex> guard(guard_);
    return pretty_dump_string_locked(starting_from, num_bytes);
}

std::string mmu::describe_handle(int memory_handle) const {
    std::lock_guard<std::mutex> guard(guard_);
    auto it = find_allocation(memory_handle);
    if (it == allocations_.end()) {
        return "Handle not found.\n";
    }

    const AllocationRecord& allocation = it->second;
    std::ostringstream out;
    out << "Handle 0x" << std::uppercase << std::hex << memory_handle << std::dec << '\n';
    out << "Owner: " << task_label(allocation.owner_task_id) << '\n';
    out << "Base: " << allocation.base << '\n';
    out << "Limit: " << allocation.reserved_size << '\n';
    out << "Requested: " << allocation.requested_size << '\n';
    out << "Current Location: " << allocation.current_location << '\n';
    return out.str();
}

std::string mmu::get_last_event() const {
    std::lock_guard<std::mutex> guard(guard_);
    return last_event_;
}

std::string mmu::get_last_pre_coalesce_dump() const {
    std::lock_guard<std::mutex> guard(guard_);
    return last_pre_coalesce_dump_;
}

std::string mmu::get_last_post_coalesce_dump() const {
    std::lock_guard<std::mutex> guard(guard_);
    return last_post_coalesce_dump_;
}

int mmu::current_task_id() const {
    return (scheduler_ != nullptr) ? scheduler_->get_current_task_id() : -1;
}

int mmu::make_handle(int owner_task_id) {
    int handle = 0;
    do {
        const int owner_bits = (owner_task_id >= 0) ? (owner_task_id & 0xFF) : 0;
        const int generation_bits = (next_generation_++ & 0x00FFFFFF) << 8;
        handle = generation_bits | owner_bits;
    } while (allocations_.find(handle) != allocations_.end());

    return handle;
}

int mmu::reserve_size_for_request(int size) const {
    return round_up_to_multiple(size, page_size_);
}

int mmu::clamp_dump_start(int starting_from) const {
    if (core_memory_.empty()) {
        return 0;
    }

    return std::clamp(starting_from, 0, static_cast<int>(core_memory_.size()) - 1);
}

int mmu::clamp_dump_count(int starting_from, int num_bytes) const {
    if (core_memory_.empty()) {
        return 0;
    }

    const int start = clamp_dump_start(starting_from);
    const int max_count = static_cast<int>(core_memory_.size()) - start;
    if (num_bytes < 0) {
        return max_count;
    }
    return std::clamp(num_bytes, 0, max_count);
}

bool mmu::validate_access(const AllocationRecord& allocation, int offset, int length) const {
    if (!allocation.active || offset < 0 || length < 0) {
        return false;
    }

    const int actor_task = current_task_id();
    if (actor_task >= 0 && actor_task != allocation.owner_task_id) {
        return false;
    }

    return offset <= allocation.reserved_size
        && length <= allocation.reserved_size
        && offset + length <= allocation.reserved_size;
}

void mmu::fill_memory(int start, int size, unsigned char value) {
    if (size <= 0 || core_memory_.empty()) {
        return;
    }

    const int safe_start = std::clamp(start, 0, static_cast<int>(core_memory_.size()));
    const int safe_end = std::clamp(start + size, 0, static_cast<int>(core_memory_.size()));
    std::fill(
        core_memory_.begin() + safe_start,
        core_memory_.begin() + safe_end,
        value
    );
}

void mmu::note_event(const std::string& message) {
    last_event_ = message;
}

void mmu::wake_waiting_tasks() {
    if (scheduler_ == nullptr) {
        waiting_tasks_.clear();
        return;
    }

    while (!waiting_tasks_.empty()) {
        const int task_id = waiting_tasks_.front();
        waiting_tasks_.pop_front();
        scheduler_->unblock_task(task_id);
    }
}

void mmu::enter_core_region() {
    if (core_semaphore_ != nullptr) {
        core_semaphore_->down();
    }
}

void mmu::leave_core_region() {
    if (core_semaphore_ != nullptr) {
        core_semaphore_->up();
    }
}

std::list<mmu::Segment>::iterator mmu::find_segment_by_handle(int memory_handle) {
    return std::find_if(
        segments_.begin(),
        segments_.end(),
        [memory_handle](const Segment& segment) {
            return !segment.free && segment.handle == memory_handle;
        }
    );
}

std::list<mmu::Segment>::const_iterator mmu::find_segment_by_handle(int memory_handle) const {
    return std::find_if(
        segments_.cbegin(),
        segments_.cend(),
        [memory_handle](const Segment& segment) {
            return !segment.free && segment.handle == memory_handle;
        }
    );
}

std::unordered_map<int, mmu::AllocationRecord>::iterator mmu::find_allocation(int memory_handle) {
    return allocations_.find(memory_handle);
}

std::unordered_map<int, mmu::AllocationRecord>::const_iterator mmu::find_allocation(int memory_handle) const {
    return allocations_.find(memory_handle);
}

int mmu::coalesce_locked() {
    int merges = 0;

    for (auto segment_it = segments_.begin(); segment_it != segments_.end();) {
        auto next_it = std::next(segment_it);
        if (next_it == segments_.end()) {
            break;
        }

        if (segment_it->free && next_it->free) {
            segment_it->size += next_it->size;
            segments_.erase(next_it);
            ++merges;
            continue;
        }

        ++segment_it;
    }

    for (const Segment& segment : segments_) {
        if (segment.free) {
            fill_memory(
                segment.start,
                segment.size,
                static_cast<unsigned char>(default_initial_value_)
            );
        }
    }

    return merges;
}

int mmu::free_bytes_locked() const {
    int free_bytes = 0;
    for (const Segment& segment : segments_) {
        if (segment.free) {
            free_bytes += segment.size;
        }
    }
    return free_bytes;
}

int mmu::largest_free_locked() const {
    int largest = 0;
    for (const Segment& segment : segments_) {
        if (segment.free) {
            largest = std::max(largest, segment.size);
        }
    }
    return largest;
}

int mmu::smallest_free_locked() const {
    int smallest = 0;
    for (const Segment& segment : segments_) {
        if (!segment.free) {
            continue;
        }
        smallest = (smallest == 0) ? segment.size : std::min(smallest, segment.size);
    }
    return smallest;
}

std::vector<mmu::SegmentSnapshot> mmu::snapshot_segments_locked() const {
    std::vector<SegmentSnapshot> snapshots;
    snapshots.reserve(segments_.size());

    for (const Segment& segment : segments_) {
        snapshots.push_back(SegmentSnapshot {
            segment.segment_id,
            segment.free,
            segment.handle,
            segment.owner_task_id,
            segment.start,
            segment.start + segment.size - 1,
            segment.size,
            segment.requested_size,
            segment.current_location
        });
    }

    return snapshots;
}

std::vector<mmu::HandleSnapshot> mmu::snapshot_handles_locked() const {
    std::vector<HandleSnapshot> snapshots;
    snapshots.reserve(allocations_.size());

    for (const auto& entry : allocations_) {
        const AllocationRecord& allocation = entry.second;
        snapshots.push_back(HandleSnapshot {
            allocation.handle,
            allocation.owner_task_id,
            allocation.base,
            allocation.reserved_size,
            allocation.requested_size,
            allocation.reserved_size,
            allocation.current_location,
            allocation.active
        });
    }

    std::sort(
        snapshots.begin(),
        snapshots.end(),
        [](const HandleSnapshot& left, const HandleSnapshot& right) {
            return left.base < right.base;
        }
    );

    return snapshots;
}

std::string mmu::layout_table_string_locked() const {
    std::ostringstream out;
    out << "Memory Usage Ledger\n";
    out << "===================\n";
    out << "Core Bytes: " << core_memory_.size()
        << " | Block Size: " << page_size_
        << " | Free: " << free_bytes_locked()
        << " | Largest: " << largest_free_locked()
        << " | Smallest: " << smallest_free_locked()
        << " | Waiting: " << waiting_tasks_.size() << "\n\n";

    out << std::left
        << std::setw(10) << "Status"
        << std::setw(12) << "Handle"
        << std::setw(10) << "Start"
        << std::setw(10) << "End"
        << std::setw(10) << "Bytes"
        << std::setw(12) << "Requested"
        << std::setw(12) << "Current"
        << "Task-ID\n";
    out << std::string(78, '-') << "\n";

    for (const Segment& segment : segments_) {
        out << std::left
            << std::setw(10) << (segment.free ? "Free" : "Used");

        if (segment.free) {
            out << std::setw(12) << "MMU"
                << std::setw(10) << segment.start
                << std::setw(10) << (segment.start + segment.size - 1)
                << std::setw(10) << segment.size
                << std::setw(12) << "NA"
                << std::setw(12) << "NA"
                << "MMU\n";
            continue;
        }

        std::ostringstream handle_stream;
        handle_stream << "0x" << std::uppercase << std::hex << segment.handle;
        out << std::setw(12) << handle_stream.str()
            << std::dec
            << std::setw(10) << segment.start
            << std::setw(10) << (segment.start + segment.size - 1)
            << std::setw(10) << segment.size
            << std::setw(12) << segment.requested_size
            << std::setw(12) << segment.current_location
            << segment.owner_task_id
            << '\n';
    }

    out << '\n';
    return out.str();
}

std::string mmu::memory_char_dump_string_locked(int starting_from, int num_bytes, int bytes_per_line) const {
    std::ostringstream out;
    out << "Memory Core Dump (Character View)\n";
    out << "=================================\n";

    if (core_memory_.empty()) {
        out << "[empty]\n\n";
        return out.str();
    }

    const int start = clamp_dump_start(starting_from);
    const int count = clamp_dump_count(starting_from, num_bytes);
    const int line_width = std::max(bytes_per_line, 1);

    for (int offset = 0; offset < count; offset += line_width) {
        const int line_start = start + offset;
        const int line_count = std::min(line_width, count - offset);
        out << std::setw(4) << line_start << " : ";
        for (int index = 0; index < line_count; ++index) {
            out << printable_or_placeholder(core_memory_[static_cast<std::size_t>(line_start + index)]);
        }
        out << '\n';
    }

    out << '\n';
    return out.str();
}

std::string mmu::memory_hex_dump_string_locked(int starting_from, int num_bytes, int bytes_per_line) const {
    std::ostringstream out;
    out << "Memory Core Dump (HEX / ASCII)\n";
    out << "==============================\n";

    if (core_memory_.empty()) {
        out << "[empty]\n\n";
        return out.str();
    }

    const int start = clamp_dump_start(starting_from);
    const int count = clamp_dump_count(starting_from, num_bytes);
    const int line_width = std::max(bytes_per_line, 1);

    for (int offset = 0; offset < count; offset += line_width) {
        const int line_start = start + offset;
        const int line_count = std::min(line_width, count - offset);

        out << std::setw(4) << line_start << "  ";
        for (int index = 0; index < line_width; ++index) {
            if (index < line_count) {
                out << std::uppercase << std::hex
                    << std::setw(2) << std::setfill('0')
                    << static_cast<int>(core_memory_[static_cast<std::size_t>(line_start + index)])
                    << std::setfill(' ') << ' ';
            } else {
                out << "   ";
            }
        }

        out << " |";
        for (int index = 0; index < line_count; ++index) {
            out << printable_or_placeholder(core_memory_[static_cast<std::size_t>(line_start + index)]);
        }
        out << "|\n" << std::dec;
    }

    out << '\n';
    return out.str();
}

std::string mmu::pretty_dump_string_locked(int starting_from, int num_bytes) const {
    std::ostringstream out;
    out << layout_table_string_locked();
    out << memory_char_dump_string_locked(starting_from, num_bytes, page_size_);
    out << memory_hex_dump_string_locked(starting_from, num_bytes, 16);
    return out.str();
}

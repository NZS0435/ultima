#include "Mem_mgr.h"

#include <algorithm>
#include <cstring>
#include <iostream>
#include <sstream>

namespace {

std::string normalize_label(const std::string& label) {
    return label.empty() ? "unnamed" : label;
}

} // namespace

Mem_mgr::Mem_mgr(std::size_t total_bytes) : heap_bytes(total_bytes, 0), used_byte_count(0) {
    reset();
}

void Mem_mgr::reset() {
    std::fill(heap_bytes.begin(), heap_bytes.end(), static_cast<unsigned char>(0));
    segments.clear();
    used_byte_count = 0;

    if (!heap_bytes.empty()) {
        segments.push_back({0, heap_bytes.size(), true, ""});
    }
}

std::size_t Mem_mgr::allocate(std::size_t bytes, const std::string& label) {
    if (bytes == 0) {
        return kInvalidAddress;
    }

    for (std::size_t index = 0; index < segments.size(); ++index) {
        Segment& segment = segments[index];
        if (!segment.free || segment.size < bytes) {
            continue;
        }

        const std::size_t start = segment.start;
        const std::size_t remaining = segment.size - bytes;

        segment.size = bytes;
        segment.free = false;
        segment.label = normalize_label(label);

        if (remaining > 0) {
            segments.insert(
                segments.begin() + static_cast<std::ptrdiff_t>(index + 1),
                Segment{start + bytes, remaining, true, ""}
            );
        }

        used_byte_count += bytes;
        return start;
    }

    return kInvalidAddress;
}

bool Mem_mgr::free_block(std::size_t address) {
    const std::size_t index = find_segment_index(address);
    if (index == kInvalidAddress) {
        return false;
    }

    Segment& segment = segments[index];
    if (segment.free || segment.start != address) {
        return false;
    }

    std::fill(
        heap_bytes.begin() + static_cast<std::ptrdiff_t>(segment.start),
        heap_bytes.begin() + static_cast<std::ptrdiff_t>(segment.start + segment.size),
        static_cast<unsigned char>(0)
    );

    used_byte_count -= segment.size;
    segment.free = true;
    segment.label.clear();

    coalesce_free_segments();
    return true;
}

bool Mem_mgr::write(std::size_t address, const void* data, std::size_t bytes) {
    if (data == nullptr) {
        return bytes == 0;
    }

    if (!range_within_allocated_segment(address, bytes)) {
        return false;
    }

    std::memcpy(
        heap_bytes.data() + static_cast<std::ptrdiff_t>(address),
        data,
        bytes
    );
    return true;
}

bool Mem_mgr::read(std::size_t address, void* destination, std::size_t bytes) const {
    if (destination == nullptr) {
        return bytes == 0;
    }

    if (!range_within_allocated_segment(address, bytes)) {
        return false;
    }

    std::memcpy(
        destination,
        heap_bytes.data() + static_cast<std::ptrdiff_t>(address),
        bytes
    );
    return true;
}

bool Mem_mgr::owns(std::size_t address) const {
    return address < heap_bytes.size();
}

std::size_t Mem_mgr::capacity() const {
    return heap_bytes.size();
}

std::size_t Mem_mgr::used_bytes() const {
    return used_byte_count;
}

std::size_t Mem_mgr::free_bytes() const {
    return capacity() - used_byte_count;
}

std::size_t Mem_mgr::allocation_count() const {
    std::size_t count = 0;
    for (const Segment& segment : segments) {
        if (!segment.free) {
            ++count;
        }
    }
    return count;
}

std::vector<Mem_mgr::SegmentSnapshot> Mem_mgr::snapshot_segments() const {
    std::vector<SegmentSnapshot> snapshot;
    snapshot.reserve(segments.size());

    for (const Segment& segment : segments) {
        snapshot.push_back({segment.start, segment.size, segment.free, segment.label});
    }

    return snapshot;
}

std::string Mem_mgr::describe_layout() const {
    std::ostringstream out;

    out << "Memory Manager Dump\n";
    out << "===================\n";
    out << "Capacity: " << capacity() << " bytes\n";
    out << "Used: " << used_bytes() << " bytes\n";
    out << "Free: " << free_bytes() << " bytes\n";
    out << "Allocations: " << allocation_count() << "\n";
    out << "Segments:\n";

    if (segments.empty()) {
        out << "  [empty]\n";
        return out.str();
    }

    for (const Segment& segment : segments) {
        out << "  [" << segment.start << ".." << (segment.start + segment.size - 1) << "] "
            << (segment.free ? "FREE " : "USED ")
            << "size=" << segment.size;

        if (!segment.free) {
            out << " label=" << segment.label;
        }

        out << "\n";
    }

    return out.str();
}

void Mem_mgr::dump(int level) const {
    std::cout << describe_layout();
    if (level > 0) {
        std::cout << "Internal segment count: " << segments.size() << "\n";
    }
}

std::size_t Mem_mgr::find_segment_index(std::size_t address) const {
    for (std::size_t index = 0; index < segments.size(); ++index) {
        const Segment& segment = segments[index];
        if (address >= segment.start && address < (segment.start + segment.size)) {
            return index;
        }
    }

    return kInvalidAddress;
}

void Mem_mgr::coalesce_free_segments() {
    if (segments.empty()) {
        return;
    }

    std::vector<Segment> merged;
    merged.reserve(segments.size());

    for (const Segment& segment : segments) {
        if (!merged.empty() && merged.back().free && segment.free) {
            merged.back().size += segment.size;
            continue;
        }

        merged.push_back(segment);
    }

    segments = std::move(merged);
}

bool Mem_mgr::range_within_allocated_segment(std::size_t address, std::size_t bytes) const {
    if (bytes == 0) {
        return owns(address) || (address == 0 && heap_bytes.empty());
    }

    const std::size_t index = find_segment_index(address);
    if (index == kInvalidAddress) {
        return false;
    }

    const Segment& segment = segments[index];
    if (segment.free) {
        return false;
    }

    return address + bytes <= segment.start + segment.size;
}

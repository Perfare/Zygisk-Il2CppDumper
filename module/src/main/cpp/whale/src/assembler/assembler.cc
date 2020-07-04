#include "assembler/assembler.h"

namespace whale {


AssemblerBuffer::AssemblerBuffer() {
    static const size_t kInitialBufferCapacity = 4 * KB;
    contents_ = reinterpret_cast<uint8_t *>(malloc(kInitialBufferCapacity));
    cursor_ = contents_;
    limit_ = ComputeLimit(contents_, kInitialBufferCapacity);
    fixup_ = nullptr;
    slow_path_ = nullptr;
    has_ensured_capacity_ = false;
    fixups_processed_ = false;
    // Verify internal state.
    CHECK_EQ(Capacity(), kInitialBufferCapacity);
    CHECK_EQ(Size(), 0U);
}


AssemblerBuffer::~AssemblerBuffer() {
    free(contents_);
}


void AssemblerBuffer::ProcessFixups(const MemoryRegion &region) {
    AssemblerFixup *fixup = fixup_;
    while (fixup != nullptr) {
        fixup->Process(region, fixup->position());
        fixup = fixup->previous();
    }
}


void AssemblerBuffer::FinalizeInstructions(const MemoryRegion &instructions) {
    // Copy the instructions from the buffer.
    MemoryRegion from(reinterpret_cast<void *>(contents()), Size());
    instructions.CopyFrom(0, from);
    // Process fixups in the instructions.
    ProcessFixups(instructions);
    fixups_processed_ = true;
}


void AssemblerBuffer::ExtendCapacity(size_t min_capacity) {
    size_t old_size = Size();
    size_t old_capacity = Capacity();
    DCHECK_GT(min_capacity, old_capacity);
    size_t new_capacity = std::min(old_capacity * 2, old_capacity + 1 * MB);
    new_capacity = std::max(new_capacity, min_capacity);

    // Allocate the new data area and copy contents of the old one to it.
    contents_ = reinterpret_cast<uint8_t *>(realloc(contents_, new_capacity));

    // Update the cursor and recompute the limit.
    cursor_ = contents_ + old_size;
    limit_ = ComputeLimit(contents_, new_capacity);

    // Verify internal state.
    CHECK_EQ(Capacity(), new_capacity);
    CHECK_EQ(Size(), old_size);
}

}  // namespace whale

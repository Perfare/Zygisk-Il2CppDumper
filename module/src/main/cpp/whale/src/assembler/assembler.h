#ifndef WHALE_ASSEMBLER_ASSEMBLER_H_
#define WHALE_ASSEMBLER_ASSEMBLER_H_

#include "assembler/label.h"
#include "assembler/memory_region.h"

namespace whale {

class Assembler;

class AssemblerBuffer;

class Label;


// Assembler fixups are positions in generated code that require processing
// after the code has been copied to executable memory. This includes building
// relocation information.
class AssemblerFixup {
 public:
    virtual void Process(const MemoryRegion &region, int position) = 0;

    virtual ~AssemblerFixup() = default;

 private:
    AssemblerFixup *previous_;
    int position_;

    AssemblerFixup *previous() const { return previous_; }

    void set_previous(AssemblerFixup *previous_in) { previous_ = previous_in; }

    int position() const { return position_; }

    void set_position(int position_in) { position_ = position_in; }

    friend class AssemblerBuffer;
};

class SlowPath {
 public:
    SlowPath() : next_(nullptr) {}

    virtual ~SlowPath() {}

    Label *Continuation() { return &continuation_; }

    Label *Entry() { return &entry_; }

    // Generate code for slow path
    virtual void Emit(Assembler *sp_asm) = 0;

 protected:
    // Entry branched to by fast path
    Label entry_;
    // Optional continuation that is branched to at the end of the slow path
    Label continuation_;
    // Next in linked list of slow paths
    SlowPath *next_;

 private:
    friend class AssemblerBuffer;
    DISALLOW_COPY_AND_ASSIGN(SlowPath);
};

class AssemblerBuffer {
 public:
    explicit AssemblerBuffer();

    ~AssemblerBuffer();

    // Basic support for emitting, loading, and storing.
    template<typename T>
    void Emit(T value) {
        CHECK(HasEnsuredCapacity());
        *reinterpret_cast<T *>(cursor_) = value;
        cursor_ += sizeof(T);
    }

    template<typename T>
    T Load(size_t position) {
        CHECK_LE(position, Size() - static_cast<int>(sizeof(T)));
        return *reinterpret_cast<T *>(contents_ + position);
    }

    template<typename T>
    void Store(size_t position, T value) {
        CHECK_LE(position, Size() - static_cast<int>(sizeof(T)));
        *reinterpret_cast<T *>(contents_ + position) = value;
    }

    void Resize(size_t new_size) {
        if (new_size > Capacity()) {
            ExtendCapacity(new_size);
        }
        cursor_ = contents_ + new_size;
    }

    void Move(size_t newposition, size_t oldposition, size_t size) {
        // Move a chunk of the buffer from oldposition to newposition.
        DCHECK_LE(oldposition + size, Size());
        DCHECK_LE(newposition + size, Size());
        memmove(contents_ + newposition, contents_ + oldposition, size);
    }

    // Emit a fixup at the current location.
    void EmitFixup(AssemblerFixup *fixup) {
        fixup->set_previous(fixup_);
        fixup->set_position(Size());
        fixup_ = fixup;
    }

    void EnqueueSlowPath(SlowPath *slowpath) {
        if (slow_path_ == nullptr) {
            slow_path_ = slowpath;
        } else {
            SlowPath *cur = slow_path_;
            for (; cur->next_ != nullptr; cur = cur->next_) {}
            cur->next_ = slowpath;
        }
    }

    void EmitSlowPaths(Assembler *sp_asm) {
        SlowPath *cur = slow_path_;
        SlowPath *next = nullptr;
        slow_path_ = nullptr;
        for (; cur != nullptr; cur = next) {
            cur->Emit(sp_asm);
            next = cur->next_;
            delete cur;
        }
    }

    // Get the size of the emitted code.
    size_t Size() const {
        CHECK_GE(cursor_, contents_);
        return cursor_ - contents_;
    }

    uint8_t *contents() const { return contents_; }

    // Copy the assembled instructions into the specified memory block
    // and apply all fixups.
    void FinalizeInstructions(const MemoryRegion &region);

    // To emit an instruction to the assembler buffer, the EnsureCapacity helper
    // must be used to guarantee that the underlying data area is big enough to
    // hold the emitted instruction. Usage:
    //
    //     AssemblerBuffer buffer;
    //     AssemblerBuffer::EnsureCapacity ensured(&buffer);
    //     ... emit bytes for single instruction ...

    class EnsureCapacity {
     public:
        explicit EnsureCapacity(AssemblerBuffer *buffer) {
            if (buffer->cursor() > buffer->limit()) {
                buffer->ExtendCapacity(buffer->Size() + kMinimumGap);
            }
            // In debug mode, we save the assembler buffer along with the gap
            // size before we start emitting to the buffer. This allows us to
            // check that any single generated instruction doesn't overflow the
            // limit implied by the minimum gap size.
            buffer_ = buffer;
            gap_ = ComputeGap();
            // Make sure that extending the capacity leaves a big enough gap
            // for any kind of instruction.
            CHECK_GE(gap_, kMinimumGap);
            // Mark the buffer as having ensured the capacity.
            CHECK(!buffer->HasEnsuredCapacity());  // Cannot nest.
            buffer->has_ensured_capacity_ = true;
        }

        ~EnsureCapacity() {
            // Unmark the buffer, so we cannot emit after this.
            buffer_->has_ensured_capacity_ = false;
            // Make sure the generated instruction doesn't take up more
            // space than the minimum gap.
            size_t delta = gap_ - ComputeGap();
            CHECK_LE(delta, kMinimumGap);
        }

     private:
        AssemblerBuffer *buffer_;
        size_t gap_;

        size_t ComputeGap() { return buffer_->Capacity() - buffer_->Size(); }
    };

    bool has_ensured_capacity_;

    bool HasEnsuredCapacity() const { return has_ensured_capacity_; }

    // Returns the position in the instruction stream.
    size_t GetPosition() { return cursor_ - contents_; }

    size_t Capacity() const {
        CHECK_GE(limit_, contents_);
        return (limit_ - contents_) + kMinimumGap;
    }

    // Unconditionally increase the capacity.
    // The provided `min_capacity` must be higher than current `Capacity()`.
    void ExtendCapacity(size_t min_capacity);

 private:
    // The limit is set to kMinimumGap bytes before the end of the data area.
    // This leaves enough space for the longest possible instruction and allows
    // for a single, fast space check per instruction.
    static const int kMinimumGap = 32;

    uint8_t *contents_;
    uint8_t *cursor_;
    uint8_t *limit_;
    AssemblerFixup *fixup_;
    bool fixups_processed_;

    // Head of linked list of slow paths
    SlowPath *slow_path_;

    uint8_t *cursor() const { return cursor_; }

    uint8_t *limit() const { return limit_; }

    // Process the fixup chain starting at the given fixup. The offset is
    // non-zero for fixups in the body if the preamble is non-empty.
    void ProcessFixups(const MemoryRegion &region);

    // Compute the limit based on the data area and the capacity. See
    // description of kMinimumGap for the reasoning behind the value.
    static uint8_t *ComputeLimit(uint8_t *data, size_t capacity) {
        return data + capacity - kMinimumGap;
    }

    friend class AssemblerFixup;
};

class Assembler {
 public:
    // Finalize the code; emit slow paths, fixup branches, add literal pool, etc.
    virtual void FinalizeCode() { buffer_.EmitSlowPaths(this); }

    // Size of generated code
    virtual size_t CodeSize() const { return buffer_.Size(); }

    virtual const uint8_t *CodeBufferBaseAddress() const { return buffer_.contents(); }

    // CodePosition() is a non-const method similar to CodeSize(), which is used to
    // record positions within the code buffer for the purpose of signal handling
    // (stack overflow checks and implicit null checks may trigger signals and the
    // signal handlers expect them right before the recorded positions).
    // On most architectures CodePosition() should be equivalent to CodeSize(), but
    // the MIPS assembler needs to be aware of this recording, so it doesn't put
    // the instructions that can trigger signals into branch delay slots. Handling
    // signals from instructions in delay slots is a bit problematic and should be
    // avoided.
    virtual size_t CodePosition() { return CodeSize(); }

    // Copy instructions out of assembly buffer into the given region of memory
    virtual void FinalizeInstructions(const MemoryRegion &region) {
        buffer_.FinalizeInstructions(region);
    }

    virtual void Comment(const char *format, ...) {}

    virtual void Bind(Label *label) = 0;

    virtual void Jump(Label *label) = 0;

    virtual ~Assembler() = default;

    AssemblerBuffer *GetBuffer() {
        return &buffer_;
    }

 protected:
    explicit Assembler() : buffer_() {}

    AssemblerBuffer buffer_;
};

}  // namespace whale

#endif  // WHALE_ASSEMBLER_ASSEMBLER_H_

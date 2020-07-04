#ifndef WHALE_ASSEMBLER_X86_64_ASSEMBLER_X86_64_H_
#define WHALE_ASSEMBLER_X86_64_ASSEMBLER_X86_64_H_

#include <cstdint>
#include <vector>
#include <assembler/assembler.h>
#include <assembler/label.h>
#include <base/array_ref.h>
#include "assembler/value_object.h"
#include "base/bit_utils.h"
#include "assembler/x86_64/registers_x86_64.h"
#include "assembler/x86_64/constants_x86_64.h"
#include "base/logging.h"
#include "assembler/assembler.h"
#include "base/offsets.h"
#include "managed_register_x86_64.h"

// If true, references within the heap are poisoned (negated).
#ifdef USE_HEAP_POISONING
static constexpr bool kPoisonHeapReferences = true;
#else
static constexpr bool kPoisonHeapReferences = false;
#endif

namespace whale {
namespace x86_64 {


// Encodes an immediate value for operands.
//
// Note: Immediates can be 64b on x86-64 for certain instructions, but are often restricted
// to 32b.
//
// Note: As we support cross-compilation, the value type must be int64_t. Please be aware of
// conversion rules in expressions regarding negation, especially size_t on 32b.
class Immediate : public ValueObject {
 public:
    explicit Immediate(int64_t value_in) : value_(value_in) {}

    int64_t value() const { return value_; }

    bool is_int8() const { return IsInt<8>(value_); }

    bool is_uint8() const { return IsUint<8>(value_); }

    bool is_int16() const { return IsInt<16>(value_); }

    bool is_uint16() const { return IsUint<16>(value_); }

    bool is_int32() const { return IsInt<32>(value_); }

 private:
    const int64_t value_;
};


class Operand : public ValueObject {
 public:
    uint8_t mod() const {
        return (encoding_at(0) >> 6) & 3;
    }

    Register rm() const {
        return static_cast<Register>(encoding_at(0) & 7);
    }

    ScaleFactor scale() const {
        return static_cast<ScaleFactor>((encoding_at(1) >> 6) & 3);
    }

    Register index() const {
        return static_cast<Register>((encoding_at(1) >> 3) & 7);
    }

    Register base() const {
        return static_cast<Register>(encoding_at(1) & 7);
    }

    CpuRegister cpu_rm() const {
        int ext = (rex_ & 1) != 0 ? x86_64::R8 : x86_64::RAX;
        return static_cast<CpuRegister>(rm() + ext);
    }

    CpuRegister cpu_index() const {
        int ext = (rex_ & 2) != 0 ? x86_64::R8 : x86_64::RAX;
        return static_cast<CpuRegister>(index() + ext);
    }

    CpuRegister cpu_base() const {
        int ext = (rex_ & 1) != 0 ? x86_64::R8 : x86_64::RAX;
        return static_cast<CpuRegister>(base() + ext);
    }

    uint8_t rex() const {
        return rex_;
    }

    int8_t disp8() const {
        CHECK_GE(length_, 2);
        return static_cast<int8_t>(encoding_[length_ - 1]);
    }

    int32_t disp32() const {
        CHECK_GE(length_, 5);
        int32_t value;
        memcpy(&value, &encoding_[length_ - 4], sizeof(value));
        return value;
    }

    bool IsRegister(CpuRegister reg) const {
        return ((encoding_[0] & 0xF8) == 0xC0)  // Addressing mode is register only.
               && ((encoding_[0] & 0x07) == reg.LowBits())  // Register codes match.
               && (reg.NeedsRex() == ((rex_ & 1) != 0));  // REX.000B bits match.
    }

    AssemblerFixup *GetFixup() const {
        return fixup_;
    }

 protected:
    // Operand can be sub classed (e.g: Address).
    Operand() : rex_(0), length_(0), fixup_(nullptr) {}

    void SetModRM(uint8_t mod_in, CpuRegister rm_in) {
        CHECK_EQ(mod_in & ~3, 0);
        if (rm_in.NeedsRex()) {
            rex_ |= 0x41;  // REX.000B
        }
        encoding_[0] = (mod_in << 6) | rm_in.LowBits();
        length_ = 1;
    }

    void SetSIB(ScaleFactor scale_in, CpuRegister index_in, CpuRegister base_in) {
        CHECK_EQ(length_, 1);
        CHECK_EQ(scale_in & ~3, 0);
        if (base_in.NeedsRex()) {
            rex_ |= 0x41;  // REX.000B
        }
        if (index_in.NeedsRex()) {
            rex_ |= 0x42;  // REX.00X0
        }
        encoding_[1] = (scale_in << 6) | (static_cast<uint8_t>(index_in.LowBits()) << 3) |
                       static_cast<uint8_t>(base_in.LowBits());
        length_ = 2;
    }

    void SetDisp8(int8_t disp) {
        CHECK(length_ == 1 || length_ == 2);
        encoding_[length_++] = static_cast<uint8_t>(disp);
    }

    void SetDisp32(int32_t disp) {
        CHECK(length_ == 1 || length_ == 2);
        int disp_size = sizeof(disp);
        memmove(&encoding_[length_], &disp, disp_size);
        length_ += disp_size;
    }

    void SetFixup(AssemblerFixup *fixup) {
        fixup_ = fixup;
    }

 private:
    uint8_t rex_;
    uint8_t length_;
    uint8_t encoding_[6];
    AssemblerFixup *fixup_;

    explicit Operand(CpuRegister reg) : rex_(0), length_(0), fixup_(nullptr) { SetModRM(3, reg); }

    // Get the operand encoding byte at the given index.
    uint8_t encoding_at(int index_in) const {
        CHECK_GE(index_in, 0);
        CHECK_LT(index_in, length_);
        return encoding_[index_in];
    }

    friend class X86_64Assembler;
};


class Address : public Operand {
 public:
    Address(CpuRegister base_in, int32_t disp) {
        Init(base_in, disp);
    }

    Address(CpuRegister base_in, Offset disp) {
        Init(base_in, disp.Int32Value());
    }

    Address(CpuRegister base_in, FrameOffset disp) {
        CHECK_EQ(base_in.AsRegister(), RSP);
        Init(CpuRegister(RSP), disp.Int32Value());
    }

    Address(CpuRegister base_in, MemberOffset disp) {
        Init(base_in, disp.Int32Value());
    }

    void Init(CpuRegister base_in, int32_t disp) {
        if (disp == 0 && base_in.LowBits() != RBP) {
            SetModRM(0, base_in);
            if (base_in.LowBits() == RSP) {
                SetSIB(TIMES_1, CpuRegister(RSP), base_in);
            }
        } else if (disp >= -128 && disp <= 127) {
            SetModRM(1, base_in);
            if (base_in.LowBits() == RSP) {
                SetSIB(TIMES_1, CpuRegister(RSP), base_in);
            }
            SetDisp8(disp);
        } else {
            SetModRM(2, base_in);
            if (base_in.LowBits() == RSP) {
                SetSIB(TIMES_1, CpuRegister(RSP), base_in);
            }
            SetDisp32(disp);
        }
    }


    Address(CpuRegister index_in, ScaleFactor scale_in, int32_t disp) {
        CHECK_NE(index_in.AsRegister(), RSP);  // Illegal addressing mode.
        SetModRM(0, CpuRegister(RSP));
        SetSIB(scale_in, index_in, CpuRegister(RBP));
        SetDisp32(disp);
    }

    Address(CpuRegister base_in, CpuRegister index_in, ScaleFactor scale_in, int32_t disp) {
        CHECK_NE(index_in.AsRegister(), RSP);  // Illegal addressing mode.
        if (disp == 0 && base_in.LowBits() != RBP) {
            SetModRM(0, CpuRegister(RSP));
            SetSIB(scale_in, index_in, base_in);
        } else if (disp >= -128 && disp <= 127) {
            SetModRM(1, CpuRegister(RSP));
            SetSIB(scale_in, index_in, base_in);
            SetDisp8(disp);
        } else {
            SetModRM(2, CpuRegister(RSP));
            SetSIB(scale_in, index_in, base_in);
            SetDisp32(disp);
        }
    }

    // If no_rip is true then the Absolute address isn't RIP relative.
    static Address Absolute(uintptr_t addr, bool no_rip = false) {
        Address result;
        if (no_rip) {
            result.SetModRM(0, CpuRegister(RSP));
            result.SetSIB(TIMES_1, CpuRegister(RSP), CpuRegister(RBP));
            result.SetDisp32(addr);
        } else {
            // RIP addressing is done using RBP as the base register.
            // The value in RBP isn't used.  Instead the offset is added to RIP.
            result.SetModRM(0, CpuRegister(RBP));
            result.SetDisp32(addr);
        }
        return result;
    }

    // An RIP relative address that will be fixed up later.
    static Address RIP(AssemblerFixup *fixup) {
        Address result;
        // RIP addressing is done using RBP as the base register.
        // The value in RBP isn't used.  Instead the offset is added to RIP.
        result.SetModRM(0, CpuRegister(RBP));
        result.SetDisp32(0);
        result.SetFixup(fixup);
        return result;
    }

    // If no_rip is true then the Absolute address isn't RIP relative.
    static Address Absolute(ThreadOffset64 addr, bool no_rip = false) {
        return Absolute(addr.Int32Value(), no_rip);
    }

 private:
    Address() {}
};

std::ostream &operator<<(std::ostream &os, const Address &addr);

/**
 * Class to handle constant area values.
 */
class ConstantArea {
 public:
    explicit ConstantArea()
            : buffer_() {}

    // Add a double to the constant area, returning the offset into
    // the constant area where the literal resides.
    size_t AddDouble(double v);

    // Add a float to the constant area, returning the offset into
    // the constant area where the literal resides.
    size_t AddFloat(float v);

    // Add an int32_t to the constant area, returning the offset into
    // the constant area where the literal resides.
    size_t AddInt32(int32_t v);

    // Add an int32_t to the end of the constant area, returning the offset into
    // the constant area where the literal resides.
    size_t AppendInt32(int32_t v);

    // Add an int64_t to the constant area, returning the offset into
    // the constant area where the literal resides.
    size_t AddInt64(int64_t v);

    size_t GetSize() const {
        return buffer_.size() * elem_size_;
    }

    ArrayRef<const int32_t> GetBuffer() const {
        return ArrayRef<const int32_t>(buffer_);
    }

 private:
    static constexpr size_t elem_size_ = sizeof(int32_t);
    std::vector<int32_t> buffer_;
};


// This is equivalent to the Label class, used in a slightly different context. We
// inherit the functionality of the Label class, but prevent unintended
// derived-to-base conversions by making the base class private.
class NearLabel : private Label {
 public:
    NearLabel() : Label() {}

    // Expose the Label routines that we need.
    using Label::Position;
    using Label::LinkPosition;
    using Label::IsBound;
    using Label::IsUnused;
    using Label::IsLinked;

 private:
    using Label::BindTo;
    using Label::LinkTo;

    friend class x86_64::X86_64Assembler;

    DISALLOW_COPY_AND_ASSIGN(NearLabel);
};


class X86_64Assembler final : public Assembler {
 public:
    explicit X86_64Assembler()
            : Assembler(), constant_area_() {}

    virtual ~X86_64Assembler() {}

    /*
     * Emit Machine Instructions.
     */
    void call(CpuRegister reg);

    void call(const Address &address);

    void call(Label *label);

    void pushq(CpuRegister reg);

    void pushq(const Address &address);

    void pushq(const Immediate &imm);

    void popq(CpuRegister reg);

    void popq(const Address &address);

    void movq(CpuRegister dst, const Immediate &src);

    void movl(CpuRegister dst, const Immediate &src);

    void movq(CpuRegister dst, CpuRegister src);

    void movl(CpuRegister dst, CpuRegister src);

    void movntl(const Address &dst, CpuRegister src);

    void movntq(const Address &dst, CpuRegister src);

    void movq(CpuRegister dst, const Address &src);

    void movl(CpuRegister dst, const Address &src);

    void movq(const Address &dst, CpuRegister src);

    void movq(const Address &dst, const Immediate &imm);

    void movl(const Address &dst, CpuRegister src);

    void movl(const Address &dst, const Immediate &imm);

    void cmov(Condition c, CpuRegister dst, CpuRegister src);  // This is the 64b version.
    void cmov(Condition c, CpuRegister dst, CpuRegister src, bool is64bit);

    void cmov(Condition c, CpuRegister dst, const Address &src, bool is64bit);

    void movzxb(CpuRegister dst, CpuRegister src);

    void movzxb(CpuRegister dst, const Address &src);

    void movsxb(CpuRegister dst, CpuRegister src);

    void movsxb(CpuRegister dst, const Address &src);

    void movb(CpuRegister dst, const Address &src);

    void movb(const Address &dst, CpuRegister src);

    void movb(const Address &dst, const Immediate &imm);

    void movzxw(CpuRegister dst, CpuRegister src);

    void movzxw(CpuRegister dst, const Address &src);

    void movsxw(CpuRegister dst, CpuRegister src);

    void movsxw(CpuRegister dst, const Address &src);

    void movw(CpuRegister dst, const Address &src);

    void movw(const Address &dst, CpuRegister src);

    void movw(const Address &dst, const Immediate &imm);

    void leaq(CpuRegister dst, const Address &src);

    void leal(CpuRegister dst, const Address &src);

    void movaps(XmmRegister dst, XmmRegister src);     // move
    void movaps(XmmRegister dst, const Address &src);  // load aligned
    void movups(XmmRegister dst, const Address &src);  // load unaligned
    void movaps(const Address &dst, XmmRegister src);  // store aligned
    void movups(const Address &dst, XmmRegister src);  // store unaligned

    void movss(XmmRegister dst, const Address &src);

    void movss(const Address &dst, XmmRegister src);

    void movss(XmmRegister dst, XmmRegister src);

    void movsxd(CpuRegister dst, CpuRegister src);

    void movsxd(CpuRegister dst, const Address &src);

    void movd(XmmRegister dst, CpuRegister src);  // Note: this is the r64 version, formally movq.
    void movd(CpuRegister dst, XmmRegister src);  // Note: this is the r64 version, formally movq.
    void movd(XmmRegister dst, CpuRegister src, bool is64bit);

    void movd(CpuRegister dst, XmmRegister src, bool is64bit);

    void addss(XmmRegister dst, XmmRegister src);

    void addss(XmmRegister dst, const Address &src);

    void subss(XmmRegister dst, XmmRegister src);

    void subss(XmmRegister dst, const Address &src);

    void mulss(XmmRegister dst, XmmRegister src);

    void mulss(XmmRegister dst, const Address &src);

    void divss(XmmRegister dst, XmmRegister src);

    void divss(XmmRegister dst, const Address &src);

    void addps(XmmRegister dst, XmmRegister src);  // no addr variant (for now)
    void subps(XmmRegister dst, XmmRegister src);

    void mulps(XmmRegister dst, XmmRegister src);

    void divps(XmmRegister dst, XmmRegister src);

    void movapd(XmmRegister dst, XmmRegister src);     // move
    void movapd(XmmRegister dst, const Address &src);  // load aligned
    void movupd(XmmRegister dst, const Address &src);  // load unaligned
    void movapd(const Address &dst, XmmRegister src);  // store aligned
    void movupd(const Address &dst, XmmRegister src);  // store unaligned

    void movsd(XmmRegister dst, const Address &src);

    void movsd(const Address &dst, XmmRegister src);

    void movsd(XmmRegister dst, XmmRegister src);

    void addsd(XmmRegister dst, XmmRegister src);

    void addsd(XmmRegister dst, const Address &src);

    void subsd(XmmRegister dst, XmmRegister src);

    void subsd(XmmRegister dst, const Address &src);

    void mulsd(XmmRegister dst, XmmRegister src);

    void mulsd(XmmRegister dst, const Address &src);

    void divsd(XmmRegister dst, XmmRegister src);

    void divsd(XmmRegister dst, const Address &src);

    void addpd(XmmRegister dst, XmmRegister src);  // no addr variant (for now)
    void subpd(XmmRegister dst, XmmRegister src);

    void mulpd(XmmRegister dst, XmmRegister src);

    void divpd(XmmRegister dst, XmmRegister src);

    void movdqa(XmmRegister dst, XmmRegister src);     // move
    void movdqa(XmmRegister dst, const Address &src);  // load aligned
    void movdqu(XmmRegister dst, const Address &src);  // load unaligned
    void movdqa(const Address &dst, XmmRegister src);  // store aligned
    void movdqu(const Address &dst, XmmRegister src);  // store unaligned

    void paddb(XmmRegister dst, XmmRegister src);  // no addr variant (for now)
    void psubb(XmmRegister dst, XmmRegister src);

    void paddw(XmmRegister dst, XmmRegister src);

    void psubw(XmmRegister dst, XmmRegister src);

    void pmullw(XmmRegister dst, XmmRegister src);

    void paddd(XmmRegister dst, XmmRegister src);

    void psubd(XmmRegister dst, XmmRegister src);

    void pmulld(XmmRegister dst, XmmRegister src);

    void paddq(XmmRegister dst, XmmRegister src);

    void psubq(XmmRegister dst, XmmRegister src);

    void paddusb(XmmRegister dst, XmmRegister src);

    void paddsb(XmmRegister dst, XmmRegister src);

    void paddusw(XmmRegister dst, XmmRegister src);

    void paddsw(XmmRegister dst, XmmRegister src);

    void psubusb(XmmRegister dst, XmmRegister src);

    void psubsb(XmmRegister dst, XmmRegister src);

    void psubusw(XmmRegister dst, XmmRegister src);

    void psubsw(XmmRegister dst, XmmRegister src);

    void cvtsi2ss(XmmRegister dst, CpuRegister src);  // Note: this is the r/m32 version.
    void cvtsi2ss(XmmRegister dst, CpuRegister src, bool is64bit);

    void cvtsi2ss(XmmRegister dst, const Address &src, bool is64bit);

    void cvtsi2sd(XmmRegister dst, CpuRegister src);  // Note: this is the r/m32 version.
    void cvtsi2sd(XmmRegister dst, CpuRegister src, bool is64bit);

    void cvtsi2sd(XmmRegister dst, const Address &src, bool is64bit);

    void cvtss2si(CpuRegister dst, XmmRegister src);  // Note: this is the r32 version.
    void cvtss2sd(XmmRegister dst, XmmRegister src);

    void cvtss2sd(XmmRegister dst, const Address &src);

    void cvtsd2si(CpuRegister dst, XmmRegister src);  // Note: this is the r32 version.
    void cvtsd2ss(XmmRegister dst, XmmRegister src);

    void cvtsd2ss(XmmRegister dst, const Address &src);

    void cvttss2si(CpuRegister dst, XmmRegister src);  // Note: this is the r32 version.
    void cvttss2si(CpuRegister dst, XmmRegister src, bool is64bit);

    void cvttsd2si(CpuRegister dst, XmmRegister src);  // Note: this is the r32 version.
    void cvttsd2si(CpuRegister dst, XmmRegister src, bool is64bit);

    void cvtdq2ps(XmmRegister dst, XmmRegister src);

    void cvtdq2pd(XmmRegister dst, XmmRegister src);

    void comiss(XmmRegister a, XmmRegister b);

    void comiss(XmmRegister a, const Address &b);

    void comisd(XmmRegister a, XmmRegister b);

    void comisd(XmmRegister a, const Address &b);

    void ucomiss(XmmRegister a, XmmRegister b);

    void ucomiss(XmmRegister a, const Address &b);

    void ucomisd(XmmRegister a, XmmRegister b);

    void ucomisd(XmmRegister a, const Address &b);

    void roundsd(XmmRegister dst, XmmRegister src, const Immediate &imm);

    void roundss(XmmRegister dst, XmmRegister src, const Immediate &imm);

    void sqrtsd(XmmRegister dst, XmmRegister src);

    void sqrtss(XmmRegister dst, XmmRegister src);

    void xorpd(XmmRegister dst, const Address &src);

    void xorpd(XmmRegister dst, XmmRegister src);

    void xorps(XmmRegister dst, const Address &src);

    void xorps(XmmRegister dst, XmmRegister src);

    void pxor(XmmRegister dst, XmmRegister src);  // no addr variant (for now)

    void andpd(XmmRegister dst, const Address &src);

    void andpd(XmmRegister dst, XmmRegister src);

    void andps(XmmRegister dst, XmmRegister src);  // no addr variant (for now)
    void pand(XmmRegister dst, XmmRegister src);

    void andn(CpuRegister dst, CpuRegister src1, CpuRegister src2);

    void andnpd(XmmRegister dst, XmmRegister src);  // no addr variant (for now)
    void andnps(XmmRegister dst, XmmRegister src);

    void pandn(XmmRegister dst, XmmRegister src);

    void orpd(XmmRegister dst, XmmRegister src);  // no addr variant (for now)
    void orps(XmmRegister dst, XmmRegister src);

    void por(XmmRegister dst, XmmRegister src);

    void pavgb(XmmRegister dst, XmmRegister src);  // no addr variant (for now)
    void pavgw(XmmRegister dst, XmmRegister src);

    void psadbw(XmmRegister dst, XmmRegister src);

    void pmaddwd(XmmRegister dst, XmmRegister src);

    void phaddw(XmmRegister dst, XmmRegister src);

    void phaddd(XmmRegister dst, XmmRegister src);

    void haddps(XmmRegister dst, XmmRegister src);

    void haddpd(XmmRegister dst, XmmRegister src);

    void phsubw(XmmRegister dst, XmmRegister src);

    void phsubd(XmmRegister dst, XmmRegister src);

    void hsubps(XmmRegister dst, XmmRegister src);

    void hsubpd(XmmRegister dst, XmmRegister src);

    void pminsb(XmmRegister dst, XmmRegister src);  // no addr variant (for now)
    void pmaxsb(XmmRegister dst, XmmRegister src);

    void pminsw(XmmRegister dst, XmmRegister src);

    void pmaxsw(XmmRegister dst, XmmRegister src);

    void pminsd(XmmRegister dst, XmmRegister src);

    void pmaxsd(XmmRegister dst, XmmRegister src);

    void pminub(XmmRegister dst, XmmRegister src);  // no addr variant (for now)
    void pmaxub(XmmRegister dst, XmmRegister src);

    void pminuw(XmmRegister dst, XmmRegister src);

    void pmaxuw(XmmRegister dst, XmmRegister src);

    void pminud(XmmRegister dst, XmmRegister src);

    void pmaxud(XmmRegister dst, XmmRegister src);

    void minps(XmmRegister dst, XmmRegister src);  // no addr variant (for now)
    void maxps(XmmRegister dst, XmmRegister src);

    void minpd(XmmRegister dst, XmmRegister src);

    void maxpd(XmmRegister dst, XmmRegister src);

    void pcmpeqb(XmmRegister dst, XmmRegister src);

    void pcmpeqw(XmmRegister dst, XmmRegister src);

    void pcmpeqd(XmmRegister dst, XmmRegister src);

    void pcmpeqq(XmmRegister dst, XmmRegister src);

    void pcmpgtb(XmmRegister dst, XmmRegister src);

    void pcmpgtw(XmmRegister dst, XmmRegister src);

    void pcmpgtd(XmmRegister dst, XmmRegister src);

    void pcmpgtq(XmmRegister dst, XmmRegister src);  // SSE4.2

    void shufpd(XmmRegister dst, XmmRegister src, const Immediate &imm);

    void shufps(XmmRegister dst, XmmRegister src, const Immediate &imm);

    void pshufd(XmmRegister dst, XmmRegister src, const Immediate &imm);

    void punpcklbw(XmmRegister dst, XmmRegister src);

    void punpcklwd(XmmRegister dst, XmmRegister src);

    void punpckldq(XmmRegister dst, XmmRegister src);

    void punpcklqdq(XmmRegister dst, XmmRegister src);

    void punpckhbw(XmmRegister dst, XmmRegister src);

    void punpckhwd(XmmRegister dst, XmmRegister src);

    void punpckhdq(XmmRegister dst, XmmRegister src);

    void punpckhqdq(XmmRegister dst, XmmRegister src);

    void psllw(XmmRegister reg, const Immediate &shift_count);

    void pslld(XmmRegister reg, const Immediate &shift_count);

    void psllq(XmmRegister reg, const Immediate &shift_count);

    void psraw(XmmRegister reg, const Immediate &shift_count);

    void psrad(XmmRegister reg, const Immediate &shift_count);
    // no psraq

    void psrlw(XmmRegister reg, const Immediate &shift_count);

    void psrld(XmmRegister reg, const Immediate &shift_count);

    void psrlq(XmmRegister reg, const Immediate &shift_count);

    void psrldq(XmmRegister reg, const Immediate &shift_count);

    void flds(const Address &src);

    void fstps(const Address &dst);

    void fsts(const Address &dst);

    void fldl(const Address &src);

    void fstpl(const Address &dst);

    void fstl(const Address &dst);

    void fstsw();

    void fucompp();

    void fnstcw(const Address &dst);

    void fldcw(const Address &src);

    void fistpl(const Address &dst);

    void fistps(const Address &dst);

    void fildl(const Address &src);

    void filds(const Address &src);

    void fincstp();

    void ffree(const Immediate &index);

    void fsin();

    void fcos();

    void fptan();

    void fprem();

    void xchgl(CpuRegister dst, CpuRegister src);

    void xchgq(CpuRegister dst, CpuRegister src);

    void xchgl(CpuRegister reg, const Address &address);

    void cmpb(const Address &address, const Immediate &imm);

    void cmpw(const Address &address, const Immediate &imm);

    void cmpl(CpuRegister reg, const Immediate &imm);

    void cmpl(CpuRegister reg0, CpuRegister reg1);

    void cmpl(CpuRegister reg, const Address &address);

    void cmpl(const Address &address, CpuRegister reg);

    void cmpl(const Address &address, const Immediate &imm);

    void cmpq(CpuRegister reg0, CpuRegister reg1);

    void cmpq(CpuRegister reg0, const Immediate &imm);

    void cmpq(CpuRegister reg0, const Address &address);

    void cmpq(const Address &address, const Immediate &imm);

    void testl(CpuRegister reg1, CpuRegister reg2);

    void testl(CpuRegister reg, const Address &address);

    void testl(CpuRegister reg, const Immediate &imm);

    void testq(CpuRegister reg1, CpuRegister reg2);

    void testq(CpuRegister reg, const Address &address);

    void testb(const Address &address, const Immediate &imm);

    void testl(const Address &address, const Immediate &imm);

    void andl(CpuRegister dst, const Immediate &imm);

    void andl(CpuRegister dst, CpuRegister src);

    void andl(CpuRegister reg, const Address &address);

    void andq(CpuRegister dst, const Immediate &imm);

    void andq(CpuRegister dst, CpuRegister src);

    void andq(CpuRegister reg, const Address &address);

    void orl(CpuRegister dst, const Immediate &imm);

    void orl(CpuRegister dst, CpuRegister src);

    void orl(CpuRegister reg, const Address &address);

    void orq(CpuRegister dst, CpuRegister src);

    void orq(CpuRegister dst, const Immediate &imm);

    void orq(CpuRegister reg, const Address &address);

    void xorl(CpuRegister dst, CpuRegister src);

    void xorl(CpuRegister dst, const Immediate &imm);

    void xorl(CpuRegister reg, const Address &address);

    void xorq(CpuRegister dst, const Immediate &imm);

    void xorq(CpuRegister dst, CpuRegister src);

    void xorq(CpuRegister reg, const Address &address);

    void addl(CpuRegister dst, CpuRegister src);

    void addl(CpuRegister reg, const Immediate &imm);

    void addl(CpuRegister reg, const Address &address);

    void addl(const Address &address, CpuRegister reg);

    void addl(const Address &address, const Immediate &imm);

    void addw(const Address &address, const Immediate &imm);

    void addq(CpuRegister reg, const Immediate &imm);

    void addq(CpuRegister dst, CpuRegister src);

    void addq(CpuRegister dst, const Address &address);

    void subl(CpuRegister dst, CpuRegister src);

    void subl(CpuRegister reg, const Immediate &imm);

    void subl(CpuRegister reg, const Address &address);

    void subq(CpuRegister reg, const Immediate &imm);

    void subq(CpuRegister dst, CpuRegister src);

    void subq(CpuRegister dst, const Address &address);

    void cdq();

    void cqo();

    void idivl(CpuRegister reg);

    void idivq(CpuRegister reg);

    void imull(CpuRegister dst, CpuRegister src);

    void imull(CpuRegister reg, const Immediate &imm);

    void imull(CpuRegister dst, CpuRegister src, const Immediate &imm);

    void imull(CpuRegister reg, const Address &address);

    void imulq(CpuRegister src);

    void imulq(CpuRegister dst, CpuRegister src);

    void imulq(CpuRegister reg, const Immediate &imm);

    void imulq(CpuRegister reg, const Address &address);

    void imulq(CpuRegister dst, CpuRegister reg, const Immediate &imm);

    void imull(CpuRegister reg);

    void imull(const Address &address);

    void mull(CpuRegister reg);

    void mull(const Address &address);

    void shll(CpuRegister reg, const Immediate &imm);

    void shll(CpuRegister operand, CpuRegister shifter);

    void shrl(CpuRegister reg, const Immediate &imm);

    void shrl(CpuRegister operand, CpuRegister shifter);

    void sarl(CpuRegister reg, const Immediate &imm);

    void sarl(CpuRegister operand, CpuRegister shifter);

    void shlq(CpuRegister reg, const Immediate &imm);

    void shlq(CpuRegister operand, CpuRegister shifter);

    void shrq(CpuRegister reg, const Immediate &imm);

    void shrq(CpuRegister operand, CpuRegister shifter);

    void sarq(CpuRegister reg, const Immediate &imm);

    void sarq(CpuRegister operand, CpuRegister shifter);

    void negl(CpuRegister reg);

    void negq(CpuRegister reg);

    void notl(CpuRegister reg);

    void notq(CpuRegister reg);

    void enter(const Immediate &imm);

    void leave();

    void ret();

    void ret(const Immediate &imm);

    void nop();

    void int3();

    void hlt();

    void j(Condition condition, Label *label);

    void j(Condition condition, NearLabel *label);

    void jrcxz(NearLabel *label);

    void jmp(CpuRegister reg);

    void jmp(const Address &address);

    void jmp(Label *label);

    void jmp(NearLabel *label);

    X86_64Assembler *lock();

    void cmpxchgl(const Address &address, CpuRegister reg);

    void cmpxchgq(const Address &address, CpuRegister reg);

    void mfence();

    X86_64Assembler *gs();

    void setcc(Condition condition, CpuRegister dst);

    void bswapl(CpuRegister dst);

    void bswapq(CpuRegister dst);

    void bsfl(CpuRegister dst, CpuRegister src);

    void bsfl(CpuRegister dst, const Address &src);

    void bsfq(CpuRegister dst, CpuRegister src);

    void bsfq(CpuRegister dst, const Address &src);

    void blsi(CpuRegister dst, CpuRegister src);  // no addr variant (for now)
    void blsmsk(CpuRegister dst, CpuRegister src);  // no addr variant (for now)
    void blsr(CpuRegister dst, CpuRegister src);  // no addr variant (for now)

    void bsrl(CpuRegister dst, CpuRegister src);

    void bsrl(CpuRegister dst, const Address &src);

    void bsrq(CpuRegister dst, CpuRegister src);

    void bsrq(CpuRegister dst, const Address &src);

    void popcntl(CpuRegister dst, CpuRegister src);

    void popcntl(CpuRegister dst, const Address &src);

    void popcntq(CpuRegister dst, CpuRegister src);

    void popcntq(CpuRegister dst, const Address &src);

    void rorl(CpuRegister reg, const Immediate &imm);

    void rorl(CpuRegister operand, CpuRegister shifter);

    void roll(CpuRegister reg, const Immediate &imm);

    void roll(CpuRegister operand, CpuRegister shifter);

    void rorq(CpuRegister reg, const Immediate &imm);

    void rorq(CpuRegister operand, CpuRegister shifter);

    void rolq(CpuRegister reg, const Immediate &imm);

    void rolq(CpuRegister operand, CpuRegister shifter);

    void repne_scasb();

    void repne_scasw();

    void repe_cmpsw();

    void repe_cmpsl();

    void repe_cmpsq();

    void rep_movsw();

    //
    // Macros for High-level operations.
    //

    void AddImmediate(CpuRegister reg, const Immediate &imm);

    void LoadDoubleConstant(XmmRegister dst, double value);

    void LockCmpxchgl(const Address &address, CpuRegister reg) {
        lock()->cmpxchgl(address, reg);
    }

    void LockCmpxchgq(const Address &address, CpuRegister reg) {
        lock()->cmpxchgq(address, reg);
    }

    //
    // Misc. functionality
    //
    int PreferredLoopAlignment() { return 16; }

    void Align(int alignment, int offset);

    void Bind(Label *label) override;

    void Jump(Label *label) override {
        jmp(label);
    }

    void Bind(NearLabel *label);

    // Add a double to the constant area, returning the offset into
    // the constant area where the literal resides.
    size_t AddDouble(double v) { return constant_area_.AddDouble(v); }

    // Add a float to the constant area, returning the offset into
    // the constant area where the literal resides.
    size_t AddFloat(float v) { return constant_area_.AddFloat(v); }

    // Add an int32_t to the constant area, returning the offset into
    // the constant area where the literal resides.
    size_t AddInt32(int32_t v) {
        return constant_area_.AddInt32(v);
    }

    // Add an int32_t to the end of the constant area, returning the offset into
    // the constant area where the literal resides.
    size_t AppendInt32(int32_t v) {
        return constant_area_.AppendInt32(v);
    }

    // Add an int64_t to the constant area, returning the offset into
    // the constant area where the literal resides.
    size_t AddInt64(int64_t v) { return constant_area_.AddInt64(v); }

    // Add the contents of the constant area to the assembler buffer.
    void AddConstantArea();

    // Is the constant area empty? Return true if there are no literals in the constant area.
    bool IsConstantAreaEmpty() const { return constant_area_.GetSize() == 0; }

    // Return the current size of the constant area.
    size_t ConstantAreaSize() const { return constant_area_.GetSize(); }

    //
    // Heap poisoning.
    //

    // Poison a heap reference contained in `reg`.
    void PoisonHeapReference(CpuRegister reg) { negl(reg); }

    // Unpoison a heap reference contained in `reg`.
    void UnpoisonHeapReference(CpuRegister reg) { negl(reg); }

    // Poison a heap reference contained in `reg` if heap poisoning is enabled.
    void MaybePoisonHeapReference(CpuRegister reg) {
        if (kPoisonHeapReferences) {
            PoisonHeapReference(reg);
        }
    }

    // Unpoison a heap reference contained in `reg` if heap poisoning is enabled.
    void MaybeUnpoisonHeapReference(CpuRegister reg) {
        if (kPoisonHeapReferences) {
            UnpoisonHeapReference(reg);
        }
    }

 private:
    void EmitUint8(uint8_t value);

    void EmitInt32(int32_t value);

    void EmitInt64(int64_t value);

    void EmitRegisterOperand(uint8_t rm, uint8_t reg);

    void EmitXmmRegisterOperand(uint8_t rm, XmmRegister reg);

    void EmitFixup(AssemblerFixup *fixup);

    void EmitOperandSizeOverride();

    void EmitOperand(uint8_t rm, const Operand &operand);

    void EmitImmediate(const Immediate &imm, bool is_16_op = false);

    void EmitComplex(
            uint8_t rm, const Operand &operand, const Immediate &immediate, bool is_16_op = false);

    void EmitLabel(Label *label, int instruction_size);

    void EmitLabelLink(Label *label);

    void EmitLabelLink(NearLabel *label);

    void EmitGenericShift(bool wide, int rm, CpuRegister reg, const Immediate &imm);

    void EmitGenericShift(bool wide, int rm, CpuRegister operand, CpuRegister shifter);

    // If any input is not false, output the necessary rex prefix.
    void EmitOptionalRex(bool force, bool w, bool r, bool x, bool b);

    // Emit a rex prefix byte if necessary for reg. ie if reg is a register in the range R8 to R15.
    void EmitOptionalRex32(CpuRegister reg);

    void EmitOptionalRex32(CpuRegister dst, CpuRegister src);

    void EmitOptionalRex32(XmmRegister dst, XmmRegister src);

    void EmitOptionalRex32(CpuRegister dst, XmmRegister src);

    void EmitOptionalRex32(XmmRegister dst, CpuRegister src);

    void EmitOptionalRex32(const Operand &operand);

    void EmitOptionalRex32(CpuRegister dst, const Operand &operand);

    void EmitOptionalRex32(XmmRegister dst, const Operand &operand);

    // Emit a REX.W prefix plus necessary register bit encodings.
    void EmitRex64();

    void EmitRex64(CpuRegister reg);

    void EmitRex64(const Operand &operand);

    void EmitRex64(CpuRegister dst, CpuRegister src);

    void EmitRex64(CpuRegister dst, const Operand &operand);

    void EmitRex64(XmmRegister dst, const Operand &operand);

    void EmitRex64(XmmRegister dst, CpuRegister src);

    void EmitRex64(CpuRegister dst, XmmRegister src);

    // Emit a REX prefix to normalize byte registers plus necessary register bit encodings.
    void EmitOptionalByteRegNormalizingRex32(CpuRegister dst, CpuRegister src);

    void EmitOptionalByteRegNormalizingRex32(CpuRegister dst, const Operand &operand);

    // Emit a 3 byte VEX Prefix
    uint8_t EmitVexByteZero(bool is_two_byte);

    uint8_t EmitVexByte1(bool r, bool x, bool b, int mmmmm);

    uint8_t EmitVexByte2(bool w, int l, X86_64ManagedRegister operand, int pp);

    ConstantArea constant_area_;

    DISALLOW_COPY_AND_ASSIGN(X86_64Assembler);
};

inline void X86_64Assembler::EmitUint8(uint8_t value) {
    buffer_.Emit<uint8_t>(value);
}

inline void X86_64Assembler::EmitInt32(int32_t value) {
    buffer_.Emit<int32_t>(value);
}

inline void X86_64Assembler::EmitInt64(int64_t value) {
    // Write this 64-bit value as two 32-bit words for alignment reasons
    // (this is essentially when running on ARM, which does not allow
    // 64-bit unaligned accesses).  We assume little-endianness here.
    EmitInt32(Low32Bits(value));
    EmitInt32(High32Bits(value));
}

inline void X86_64Assembler::EmitRegisterOperand(uint8_t rm, uint8_t reg) {
    CHECK_GE(rm, 0);
    CHECK_LT(rm, 8);
    buffer_.Emit<uint8_t>((0xC0 | (reg & 7)) + (rm << 3));
}

inline void X86_64Assembler::EmitXmmRegisterOperand(uint8_t rm, XmmRegister reg) {
    EmitRegisterOperand(rm, static_cast<uint8_t>(reg.AsFloatRegister()));
}

inline void X86_64Assembler::EmitFixup(AssemblerFixup *fixup) {
    buffer_.EmitFixup(fixup);
}

inline void X86_64Assembler::EmitOperandSizeOverride() {
    EmitUint8(0x66);
}


}  // namespace x86_64
}  // namespace whale

#endif  // WHALE_ASSEMBLER_X86_64_ASSEMBLER_X86_64_H_

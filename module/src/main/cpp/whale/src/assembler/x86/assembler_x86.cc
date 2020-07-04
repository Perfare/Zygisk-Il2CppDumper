#include "assembler/x86/assembler_x86.h"

namespace whale {
namespace x86 {


uint8_t X86Assembler::EmitVexByteZero(bool is_two_byte) {
    uint8_t vex_zero = 0xC0;
    if (!is_two_byte) {
        vex_zero |= 0xC4;
    } else {
        vex_zero |= 0xC5;
    }
    return vex_zero;
}

uint8_t X86Assembler::EmitVexByte1(bool r, bool x, bool b, int mmmmm ) {
    // VEX Byte 1
    uint8_t vex_prefix = 0;
    if (!r) {
        vex_prefix |= 0x80;  // VEX.R
    }
    if (!x) {
        vex_prefix |= 0x40;  // VEX.X
    }
    if (!b) {
        vex_prefix |= 0x20;  // VEX.B
    }

    // VEX.mmmmm
    switch (mmmmm) {
        case 1:
            // implied 0F leading opcode byte
            vex_prefix |= 0x01;
            break;
        case 2:
            // implied leading 0F 38 opcode byte
            vex_prefix |= 0x02;
            break;
        case 3:
            // implied leading OF 3A opcode byte
            vex_prefix |= 0x03;
            break;
        default:
            LOG(FATAL) << "unknown opcode bytes";
    }
    return vex_prefix;
}

uint8_t X86Assembler::EmitVexByte2(bool w, int l, X86ManagedRegister operand, int pp) {
    uint8_t vex_prefix = 0;
    // VEX Byte 2
    if (w) {
        vex_prefix |= 0x80;
    }
    // VEX.vvvv
    if (operand.IsXmmRegister()) {
        XmmRegister vvvv = operand.AsXmmRegister();
        int inverted_reg = 15-static_cast<int>(vvvv);
        uint8_t reg = static_cast<uint8_t>(inverted_reg);
        vex_prefix |= ((reg & 0x0F) << 3);
    } else if (operand.IsCpuRegister()) {
        Register vvvv = operand.AsCpuRegister();
        int inverted_reg = 15 - static_cast<int>(vvvv);
        uint8_t reg = static_cast<uint8_t>(inverted_reg);
        vex_prefix |= ((reg & 0x0F) << 3);
    }

    // VEX.L
    if (l == 256) {
        vex_prefix |= 0x04;
    }

    // VEX.pp
    switch (pp) {
        case 0:
            // SIMD Pefix - None
            vex_prefix |= 0x00;
            break;
        case 1:
            // SIMD Prefix - 66
            vex_prefix |= 0x01;
            break;
        case 2:
            // SIMD Prefix - F3
            vex_prefix |= 0x02;
            break;
        case 3:
            // SIMD Prefix - F2
            vex_prefix |= 0x03;
            break;
        default:
            LOG(FATAL) << "unknown SIMD Prefix";
    }

    return vex_prefix;
}

void X86Assembler::call(Register reg) {
    AssemblerBuffer::EnsureCapacity ensured(&buffer_);
    EmitUint8(0xFF);
    EmitRegisterOperand(2, reg);
}


void X86Assembler::call(const Address& address) {
    AssemblerBuffer::EnsureCapacity ensured(&buffer_);
    EmitUint8(0xFF);
    EmitOperand(2, address);
}


void X86Assembler::call(Label* label) {
    AssemblerBuffer::EnsureCapacity ensured(&buffer_);
    EmitUint8(0xE8);
    static const int kSize = 5;
    // Offset by one because we already have emitted the opcode.
    EmitLabel(label, kSize - 1);
}


void X86Assembler::call(const ExternalLabel& label) {
    AssemblerBuffer::EnsureCapacity ensured(&buffer_);
    intptr_t call_start = buffer_.GetPosition();
    EmitUint8(0xE8);
    EmitInt32(label.address());
    static const intptr_t kCallExternalLabelSize = 5;
    DCHECK_EQ((buffer_.GetPosition() - call_start), kCallExternalLabelSize);
}


void X86Assembler::pushl(Register reg) {
    AssemblerBuffer::EnsureCapacity ensured(&buffer_);
    EmitUint8(0x50 + reg);
}


void X86Assembler::pushl(const Address& address) {
    AssemblerBuffer::EnsureCapacity ensured(&buffer_);
    EmitUint8(0xFF);
    EmitOperand(6, address);
}


void X86Assembler::pushl(const Immediate& imm) {
    AssemblerBuffer::EnsureCapacity ensured(&buffer_);
    if (imm.is_int8()) {
        EmitUint8(0x6A);
        EmitUint8(imm.value() & 0xFF);
    } else {
        EmitUint8(0x68);
        EmitImmediate(imm);
    }
}


void X86Assembler::popl(Register reg) {
    AssemblerBuffer::EnsureCapacity ensured(&buffer_);
    EmitUint8(0x58 + reg);
}


void X86Assembler::popl(const Address& address) {
    AssemblerBuffer::EnsureCapacity ensured(&buffer_);
    EmitUint8(0x8F);
    EmitOperand(0, address);
}


void X86Assembler::movl(Register dst, const Immediate& imm) {
    AssemblerBuffer::EnsureCapacity ensured(&buffer_);
    EmitUint8(0xB8 + dst);
    EmitImmediate(imm);
}


void X86Assembler::movl(Register dst, Register src) {
    AssemblerBuffer::EnsureCapacity ensured(&buffer_);
    EmitUint8(0x89);
    EmitRegisterOperand(src, dst);
}


void X86Assembler::movl(Register dst, const Address& src) {
    AssemblerBuffer::EnsureCapacity ensured(&buffer_);
    EmitUint8(0x8B);
    EmitOperand(dst, src);
}


void X86Assembler::movl(const Address& dst, Register src) {
    AssemblerBuffer::EnsureCapacity ensured(&buffer_);
    EmitUint8(0x89);
    EmitOperand(src, dst);
}


void X86Assembler::movl(const Address& dst, const Immediate& imm) {
    AssemblerBuffer::EnsureCapacity ensured(&buffer_);
    EmitUint8(0xC7);
    EmitOperand(0, dst);
    EmitImmediate(imm);
}

void X86Assembler::movl(const Address& dst, Label* lbl) {
    AssemblerBuffer::EnsureCapacity ensured(&buffer_);
    EmitUint8(0xC7);
    EmitOperand(0, dst);
    EmitLabel(lbl, dst.length_ + 5);
}

void X86Assembler::movntl(const Address& dst, Register src) {
    AssemblerBuffer::EnsureCapacity ensured(&buffer_);
    EmitUint8(0x0F);
    EmitUint8(0xC3);
    EmitOperand(src, dst);
}

void X86Assembler::blsi(Register dst, Register src) {
    AssemblerBuffer::EnsureCapacity ensured(&buffer_);
    uint8_t byte_zero = EmitVexByteZero(/*is_two_byte=*/ false);
    uint8_t byte_one = EmitVexByte1(/*r=*/ false,
            /*x=*/ false,
            /*b=*/ false,
            /*mmmmm=*/ 2);
    uint8_t byte_two = EmitVexByte2(/*w=*/ false,
            /*l=*/ 128,
                                           X86ManagedRegister::FromCpuRegister(dst),
            /*pp=*/ 0);
    EmitUint8(byte_zero);
    EmitUint8(byte_one);
    EmitUint8(byte_two);
    EmitUint8(0xF3);
    EmitRegisterOperand(3, src);
}

void X86Assembler::blsmsk(Register dst, Register src) {
    AssemblerBuffer::EnsureCapacity ensured(&buffer_);
    uint8_t byte_zero = EmitVexByteZero(/*is_two_byte=*/ false);
    uint8_t byte_one = EmitVexByte1(/*r=*/ false,
            /*x=*/ false,
            /*b=*/ false,
            /*mmmmm=*/ 2);
    uint8_t byte_two = EmitVexByte2(/*w=*/ false,
            /*l=*/ 128,
                                           X86ManagedRegister::FromCpuRegister(dst),
            /*pp=*/ 0);
    EmitUint8(byte_zero);
    EmitUint8(byte_one);
    EmitUint8(byte_two);
    EmitUint8(0xF3);
    EmitRegisterOperand(2, src);
}

void X86Assembler::blsr(Register dst, Register src) {
    AssemblerBuffer::EnsureCapacity ensured(&buffer_);
    uint8_t byte_zero = EmitVexByteZero(/*is_two_byte=*/ false);
    uint8_t byte_one = EmitVexByte1(/*r=*/ false,
            /*x=*/ false,
            /*b=*/ false,
            /*mmmmm=*/ 2);
    uint8_t byte_two = EmitVexByte2(/*w=*/ false,
            /*l=*/ 128,
                                           X86ManagedRegister::FromCpuRegister(dst),
            /*pp=*/ 0);
    EmitUint8(byte_zero);
    EmitUint8(byte_one);
    EmitUint8(byte_two);
    EmitUint8(0xF3);
    EmitRegisterOperand(1, src);
}

void X86Assembler::bswapl(Register dst) {
    AssemblerBuffer::EnsureCapacity ensured(&buffer_);
    EmitUint8(0x0F);
    EmitUint8(0xC8 + dst);
}

void X86Assembler::bsfl(Register dst, Register src) {
    AssemblerBuffer::EnsureCapacity ensured(&buffer_);
    EmitUint8(0x0F);
    EmitUint8(0xBC);
    EmitRegisterOperand(dst, src);
}

void X86Assembler::bsfl(Register dst, const Address& src) {
    AssemblerBuffer::EnsureCapacity ensured(&buffer_);
    EmitUint8(0x0F);
    EmitUint8(0xBC);
    EmitOperand(dst, src);
}

void X86Assembler::bsrl(Register dst, Register src) {
    AssemblerBuffer::EnsureCapacity ensured(&buffer_);
    EmitUint8(0x0F);
    EmitUint8(0xBD);
    EmitRegisterOperand(dst, src);
}

void X86Assembler::bsrl(Register dst, const Address& src) {
    AssemblerBuffer::EnsureCapacity ensured(&buffer_);
    EmitUint8(0x0F);
    EmitUint8(0xBD);
    EmitOperand(dst, src);
}

void X86Assembler::popcntl(Register dst, Register src) {
    AssemblerBuffer::EnsureCapacity ensured(&buffer_);
    EmitUint8(0xF3);
    EmitUint8(0x0F);
    EmitUint8(0xB8);
    EmitRegisterOperand(dst, src);
}

void X86Assembler::popcntl(Register dst, const Address& src) {
    AssemblerBuffer::EnsureCapacity ensured(&buffer_);
    EmitUint8(0xF3);
    EmitUint8(0x0F);
    EmitUint8(0xB8);
    EmitOperand(dst, src);
}

void X86Assembler::movzxb(Register dst, ByteRegister src) {
    AssemblerBuffer::EnsureCapacity ensured(&buffer_);
    EmitUint8(0x0F);
    EmitUint8(0xB6);
    EmitRegisterOperand(dst, src);
}


void X86Assembler::movzxb(Register dst, const Address& src) {
    AssemblerBuffer::EnsureCapacity ensured(&buffer_);
    EmitUint8(0x0F);
    EmitUint8(0xB6);
    EmitOperand(dst, src);
}


void X86Assembler::movsxb(Register dst, ByteRegister src) {
    AssemblerBuffer::EnsureCapacity ensured(&buffer_);
    EmitUint8(0x0F);
    EmitUint8(0xBE);
    EmitRegisterOperand(dst, src);
}


void X86Assembler::movsxb(Register dst, const Address& src) {
    AssemblerBuffer::EnsureCapacity ensured(&buffer_);
    EmitUint8(0x0F);
    EmitUint8(0xBE);
    EmitOperand(dst, src);
}


void X86Assembler::movb(Register /*dst*/, const Address& /*src*/) {
    LOG(FATAL) << "Use movzxb or movsxb instead.";
}


void X86Assembler::movb(const Address& dst, ByteRegister src) {
    AssemblerBuffer::EnsureCapacity ensured(&buffer_);
    EmitUint8(0x88);
    EmitOperand(src, dst);
}


void X86Assembler::movb(const Address& dst, const Immediate& imm) {
    AssemblerBuffer::EnsureCapacity ensured(&buffer_);
    EmitUint8(0xC6);
    EmitOperand(EAX, dst);
    CHECK(imm.is_int8());
    EmitUint8(imm.value() & 0xFF);
}


void X86Assembler::movzxw(Register dst, Register src) {
    AssemblerBuffer::EnsureCapacity ensured(&buffer_);
    EmitUint8(0x0F);
    EmitUint8(0xB7);
    EmitRegisterOperand(dst, src);
}


void X86Assembler::movzxw(Register dst, const Address& src) {
    AssemblerBuffer::EnsureCapacity ensured(&buffer_);
    EmitUint8(0x0F);
    EmitUint8(0xB7);
    EmitOperand(dst, src);
}


void X86Assembler::movsxw(Register dst, Register src) {
    AssemblerBuffer::EnsureCapacity ensured(&buffer_);
    EmitUint8(0x0F);
    EmitUint8(0xBF);
    EmitRegisterOperand(dst, src);
}


void X86Assembler::movsxw(Register dst, const Address& src) {
    AssemblerBuffer::EnsureCapacity ensured(&buffer_);
    EmitUint8(0x0F);
    EmitUint8(0xBF);
    EmitOperand(dst, src);
}


void X86Assembler::movw(Register /*dst*/, const Address& /*src*/) {
    LOG(FATAL) << "Use movzxw or movsxw instead.";
}


void X86Assembler::movw(const Address& dst, Register src) {
    AssemblerBuffer::EnsureCapacity ensured(&buffer_);
    EmitOperandSizeOverride();
    EmitUint8(0x89);
    EmitOperand(src, dst);
}


void X86Assembler::movw(const Address& dst, const Immediate& imm) {
    AssemblerBuffer::EnsureCapacity ensured(&buffer_);
    EmitOperandSizeOverride();
    EmitUint8(0xC7);
    EmitOperand(0, dst);
    CHECK(imm.is_uint16() || imm.is_int16());
    EmitUint8(imm.value() & 0xFF);
    EmitUint8(imm.value() >> 8);
}


void X86Assembler::leal(Register dst, const Address& src) {
    AssemblerBuffer::EnsureCapacity ensured(&buffer_);
    EmitUint8(0x8D);
    EmitOperand(dst, src);
}


void X86Assembler::cmovl(Condition condition, Register dst, Register src) {
    AssemblerBuffer::EnsureCapacity ensured(&buffer_);
    EmitUint8(0x0F);
    EmitUint8(0x40 + condition);
    EmitRegisterOperand(dst, src);
}


void X86Assembler::cmovl(Condition condition, Register dst, const Address& src) {
    AssemblerBuffer::EnsureCapacity ensured(&buffer_);
    EmitUint8(0x0F);
    EmitUint8(0x40 + condition);
    EmitOperand(dst, src);
}


void X86Assembler::setb(Condition condition, Register dst) {
    AssemblerBuffer::EnsureCapacity ensured(&buffer_);
    EmitUint8(0x0F);
    EmitUint8(0x90 + condition);
    EmitOperand(0, Operand(dst));
}


void X86Assembler::movaps(XmmRegister dst, XmmRegister src) {
    AssemblerBuffer::EnsureCapacity ensured(&buffer_);
    EmitUint8(0x0F);
    EmitUint8(0x28);
    EmitXmmRegisterOperand(dst, src);
}


void X86Assembler::movaps(XmmRegister dst, const Address& src) {
    AssemblerBuffer::EnsureCapacity ensured(&buffer_);
    EmitUint8(0x0F);
    EmitUint8(0x28);
    EmitOperand(dst, src);
}


void X86Assembler::movups(XmmRegister dst, const Address& src) {
    AssemblerBuffer::EnsureCapacity ensured(&buffer_);
    EmitUint8(0x0F);
    EmitUint8(0x10);
    EmitOperand(dst, src);
}


void X86Assembler::movaps(const Address& dst, XmmRegister src) {
    AssemblerBuffer::EnsureCapacity ensured(&buffer_);
    EmitUint8(0x0F);
    EmitUint8(0x29);
    EmitOperand(src, dst);
}


void X86Assembler::movups(const Address& dst, XmmRegister src) {
    AssemblerBuffer::EnsureCapacity ensured(&buffer_);
    EmitUint8(0x0F);
    EmitUint8(0x11);
    EmitOperand(src, dst);
}


void X86Assembler::movss(XmmRegister dst, const Address& src) {
    AssemblerBuffer::EnsureCapacity ensured(&buffer_);
    EmitUint8(0xF3);
    EmitUint8(0x0F);
    EmitUint8(0x10);
    EmitOperand(dst, src);
}


void X86Assembler::movss(const Address& dst, XmmRegister src) {
    AssemblerBuffer::EnsureCapacity ensured(&buffer_);
    EmitUint8(0xF3);
    EmitUint8(0x0F);
    EmitUint8(0x11);
    EmitOperand(src, dst);
}


void X86Assembler::movss(XmmRegister dst, XmmRegister src) {
    AssemblerBuffer::EnsureCapacity ensured(&buffer_);
    EmitUint8(0xF3);
    EmitUint8(0x0F);
    EmitUint8(0x11);
    EmitXmmRegisterOperand(src, dst);
}


void X86Assembler::movd(XmmRegister dst, Register src) {
    AssemblerBuffer::EnsureCapacity ensured(&buffer_);
    EmitUint8(0x66);
    EmitUint8(0x0F);
    EmitUint8(0x6E);
    EmitOperand(dst, Operand(src));
}


void X86Assembler::movd(Register dst, XmmRegister src) {
    AssemblerBuffer::EnsureCapacity ensured(&buffer_);
    EmitUint8(0x66);
    EmitUint8(0x0F);
    EmitUint8(0x7E);
    EmitOperand(src, Operand(dst));
}


void X86Assembler::addss(XmmRegister dst, XmmRegister src) {
    AssemblerBuffer::EnsureCapacity ensured(&buffer_);
    EmitUint8(0xF3);
    EmitUint8(0x0F);
    EmitUint8(0x58);
    EmitXmmRegisterOperand(dst, src);
}


void X86Assembler::addss(XmmRegister dst, const Address& src) {
    AssemblerBuffer::EnsureCapacity ensured(&buffer_);
    EmitUint8(0xF3);
    EmitUint8(0x0F);
    EmitUint8(0x58);
    EmitOperand(dst, src);
}


void X86Assembler::subss(XmmRegister dst, XmmRegister src) {
    AssemblerBuffer::EnsureCapacity ensured(&buffer_);
    EmitUint8(0xF3);
    EmitUint8(0x0F);
    EmitUint8(0x5C);
    EmitXmmRegisterOperand(dst, src);
}


void X86Assembler::subss(XmmRegister dst, const Address& src) {
    AssemblerBuffer::EnsureCapacity ensured(&buffer_);
    EmitUint8(0xF3);
    EmitUint8(0x0F);
    EmitUint8(0x5C);
    EmitOperand(dst, src);
}


void X86Assembler::mulss(XmmRegister dst, XmmRegister src) {
    AssemblerBuffer::EnsureCapacity ensured(&buffer_);
    EmitUint8(0xF3);
    EmitUint8(0x0F);
    EmitUint8(0x59);
    EmitXmmRegisterOperand(dst, src);
}


void X86Assembler::mulss(XmmRegister dst, const Address& src) {
    AssemblerBuffer::EnsureCapacity ensured(&buffer_);
    EmitUint8(0xF3);
    EmitUint8(0x0F);
    EmitUint8(0x59);
    EmitOperand(dst, src);
}


void X86Assembler::divss(XmmRegister dst, XmmRegister src) {
    AssemblerBuffer::EnsureCapacity ensured(&buffer_);
    EmitUint8(0xF3);
    EmitUint8(0x0F);
    EmitUint8(0x5E);
    EmitXmmRegisterOperand(dst, src);
}


void X86Assembler::divss(XmmRegister dst, const Address& src) {
    AssemblerBuffer::EnsureCapacity ensured(&buffer_);
    EmitUint8(0xF3);
    EmitUint8(0x0F);
    EmitUint8(0x5E);
    EmitOperand(dst, src);
}


void X86Assembler::addps(XmmRegister dst, XmmRegister src) {
    AssemblerBuffer::EnsureCapacity ensured(&buffer_);
    EmitUint8(0x0F);
    EmitUint8(0x58);
    EmitXmmRegisterOperand(dst, src);
}


void X86Assembler::subps(XmmRegister dst, XmmRegister src) {
    AssemblerBuffer::EnsureCapacity ensured(&buffer_);
    EmitUint8(0x0F);
    EmitUint8(0x5C);
    EmitXmmRegisterOperand(dst, src);
}


void X86Assembler::mulps(XmmRegister dst, XmmRegister src) {
    AssemblerBuffer::EnsureCapacity ensured(&buffer_);
    EmitUint8(0x0F);
    EmitUint8(0x59);
    EmitXmmRegisterOperand(dst, src);
}


void X86Assembler::divps(XmmRegister dst, XmmRegister src) {
    AssemblerBuffer::EnsureCapacity ensured(&buffer_);
    EmitUint8(0x0F);
    EmitUint8(0x5E);
    EmitXmmRegisterOperand(dst, src);
}


void X86Assembler::movapd(XmmRegister dst, XmmRegister src) {
    AssemblerBuffer::EnsureCapacity ensured(&buffer_);
    EmitUint8(0x66);
    EmitUint8(0x0F);
    EmitUint8(0x28);
    EmitXmmRegisterOperand(dst, src);
}


void X86Assembler::movapd(XmmRegister dst, const Address& src) {
    AssemblerBuffer::EnsureCapacity ensured(&buffer_);
    EmitUint8(0x66);
    EmitUint8(0x0F);
    EmitUint8(0x28);
    EmitOperand(dst, src);
}


void X86Assembler::movupd(XmmRegister dst, const Address& src) {
    AssemblerBuffer::EnsureCapacity ensured(&buffer_);
    EmitUint8(0x66);
    EmitUint8(0x0F);
    EmitUint8(0x10);
    EmitOperand(dst, src);
}


void X86Assembler::movapd(const Address& dst, XmmRegister src) {
    AssemblerBuffer::EnsureCapacity ensured(&buffer_);
    EmitUint8(0x66);
    EmitUint8(0x0F);
    EmitUint8(0x29);
    EmitOperand(src, dst);
}


void X86Assembler::movupd(const Address& dst, XmmRegister src) {
    AssemblerBuffer::EnsureCapacity ensured(&buffer_);
    EmitUint8(0x66);
    EmitUint8(0x0F);
    EmitUint8(0x11);
    EmitOperand(src, dst);
}


void X86Assembler::flds(const Address& src) {
    AssemblerBuffer::EnsureCapacity ensured(&buffer_);
    EmitUint8(0xD9);
    EmitOperand(0, src);
}


void X86Assembler::fsts(const Address& dst) {
    AssemblerBuffer::EnsureCapacity ensured(&buffer_);
    EmitUint8(0xD9);
    EmitOperand(2, dst);
}


void X86Assembler::fstps(const Address& dst) {
    AssemblerBuffer::EnsureCapacity ensured(&buffer_);
    EmitUint8(0xD9);
    EmitOperand(3, dst);
}


void X86Assembler::movsd(XmmRegister dst, const Address& src) {
    AssemblerBuffer::EnsureCapacity ensured(&buffer_);
    EmitUint8(0xF2);
    EmitUint8(0x0F);
    EmitUint8(0x10);
    EmitOperand(dst, src);
}


void X86Assembler::movsd(const Address& dst, XmmRegister src) {
    AssemblerBuffer::EnsureCapacity ensured(&buffer_);
    EmitUint8(0xF2);
    EmitUint8(0x0F);
    EmitUint8(0x11);
    EmitOperand(src, dst);
}


void X86Assembler::movsd(XmmRegister dst, XmmRegister src) {
    AssemblerBuffer::EnsureCapacity ensured(&buffer_);
    EmitUint8(0xF2);
    EmitUint8(0x0F);
    EmitUint8(0x11);
    EmitXmmRegisterOperand(src, dst);
}


void X86Assembler::movhpd(XmmRegister dst, const Address& src) {
    AssemblerBuffer::EnsureCapacity ensured(&buffer_);
    EmitUint8(0x66);
    EmitUint8(0x0F);
    EmitUint8(0x16);
    EmitOperand(dst, src);
}


void X86Assembler::movhpd(const Address& dst, XmmRegister src) {
    AssemblerBuffer::EnsureCapacity ensured(&buffer_);
    EmitUint8(0x66);
    EmitUint8(0x0F);
    EmitUint8(0x17);
    EmitOperand(src, dst);
}


void X86Assembler::addsd(XmmRegister dst, XmmRegister src) {
    AssemblerBuffer::EnsureCapacity ensured(&buffer_);
    EmitUint8(0xF2);
    EmitUint8(0x0F);
    EmitUint8(0x58);
    EmitXmmRegisterOperand(dst, src);
}


void X86Assembler::addsd(XmmRegister dst, const Address& src) {
    AssemblerBuffer::EnsureCapacity ensured(&buffer_);
    EmitUint8(0xF2);
    EmitUint8(0x0F);
    EmitUint8(0x58);
    EmitOperand(dst, src);
}


void X86Assembler::subsd(XmmRegister dst, XmmRegister src) {
    AssemblerBuffer::EnsureCapacity ensured(&buffer_);
    EmitUint8(0xF2);
    EmitUint8(0x0F);
    EmitUint8(0x5C);
    EmitXmmRegisterOperand(dst, src);
}


void X86Assembler::subsd(XmmRegister dst, const Address& src) {
    AssemblerBuffer::EnsureCapacity ensured(&buffer_);
    EmitUint8(0xF2);
    EmitUint8(0x0F);
    EmitUint8(0x5C);
    EmitOperand(dst, src);
}


void X86Assembler::mulsd(XmmRegister dst, XmmRegister src) {
    AssemblerBuffer::EnsureCapacity ensured(&buffer_);
    EmitUint8(0xF2);
    EmitUint8(0x0F);
    EmitUint8(0x59);
    EmitXmmRegisterOperand(dst, src);
}


void X86Assembler::mulsd(XmmRegister dst, const Address& src) {
    AssemblerBuffer::EnsureCapacity ensured(&buffer_);
    EmitUint8(0xF2);
    EmitUint8(0x0F);
    EmitUint8(0x59);
    EmitOperand(dst, src);
}


void X86Assembler::divsd(XmmRegister dst, XmmRegister src) {
    AssemblerBuffer::EnsureCapacity ensured(&buffer_);
    EmitUint8(0xF2);
    EmitUint8(0x0F);
    EmitUint8(0x5E);
    EmitXmmRegisterOperand(dst, src);
}


void X86Assembler::divsd(XmmRegister dst, const Address& src) {
    AssemblerBuffer::EnsureCapacity ensured(&buffer_);
    EmitUint8(0xF2);
    EmitUint8(0x0F);
    EmitUint8(0x5E);
    EmitOperand(dst, src);
}


void X86Assembler::addpd(XmmRegister dst, XmmRegister src) {
    AssemblerBuffer::EnsureCapacity ensured(&buffer_);
    EmitUint8(0x66);
    EmitUint8(0x0F);
    EmitUint8(0x58);
    EmitXmmRegisterOperand(dst, src);
}


void X86Assembler::subpd(XmmRegister dst, XmmRegister src) {
    AssemblerBuffer::EnsureCapacity ensured(&buffer_);
    EmitUint8(0x66);
    EmitUint8(0x0F);
    EmitUint8(0x5C);
    EmitXmmRegisterOperand(dst, src);
}


void X86Assembler::mulpd(XmmRegister dst, XmmRegister src) {
    AssemblerBuffer::EnsureCapacity ensured(&buffer_);
    EmitUint8(0x66);
    EmitUint8(0x0F);
    EmitUint8(0x59);
    EmitXmmRegisterOperand(dst, src);
}


void X86Assembler::divpd(XmmRegister dst, XmmRegister src) {
    AssemblerBuffer::EnsureCapacity ensured(&buffer_);
    EmitUint8(0x66);
    EmitUint8(0x0F);
    EmitUint8(0x5E);
    EmitXmmRegisterOperand(dst, src);
}


void X86Assembler::movdqa(XmmRegister dst, XmmRegister src) {
    AssemblerBuffer::EnsureCapacity ensured(&buffer_);
    EmitUint8(0x66);
    EmitUint8(0x0F);
    EmitUint8(0x6F);
    EmitXmmRegisterOperand(dst, src);
}


void X86Assembler::movdqa(XmmRegister dst, const Address& src) {
    AssemblerBuffer::EnsureCapacity ensured(&buffer_);
    EmitUint8(0x66);
    EmitUint8(0x0F);
    EmitUint8(0x6F);
    EmitOperand(dst, src);
}


void X86Assembler::movdqu(XmmRegister dst, const Address& src) {
    AssemblerBuffer::EnsureCapacity ensured(&buffer_);
    EmitUint8(0xF3);
    EmitUint8(0x0F);
    EmitUint8(0x6F);
    EmitOperand(dst, src);
}


void X86Assembler::movdqa(const Address& dst, XmmRegister src) {
    AssemblerBuffer::EnsureCapacity ensured(&buffer_);
    EmitUint8(0x66);
    EmitUint8(0x0F);
    EmitUint8(0x7F);
    EmitOperand(src, dst);
}


void X86Assembler::movdqu(const Address& dst, XmmRegister src) {
    AssemblerBuffer::EnsureCapacity ensured(&buffer_);
    EmitUint8(0xF3);
    EmitUint8(0x0F);
    EmitUint8(0x7F);
    EmitOperand(src, dst);
}


void X86Assembler::paddb(XmmRegister dst, XmmRegister src) {
    AssemblerBuffer::EnsureCapacity ensured(&buffer_);
    EmitUint8(0x66);
    EmitUint8(0x0F);
    EmitUint8(0xFC);
    EmitXmmRegisterOperand(dst, src);
}


void X86Assembler::psubb(XmmRegister dst, XmmRegister src) {
    AssemblerBuffer::EnsureCapacity ensured(&buffer_);
    EmitUint8(0x66);
    EmitUint8(0x0F);
    EmitUint8(0xF8);
    EmitXmmRegisterOperand(dst, src);
}


void X86Assembler::paddw(XmmRegister dst, XmmRegister src) {
    AssemblerBuffer::EnsureCapacity ensured(&buffer_);
    EmitUint8(0x66);
    EmitUint8(0x0F);
    EmitUint8(0xFD);
    EmitXmmRegisterOperand(dst, src);
}


void X86Assembler::psubw(XmmRegister dst, XmmRegister src) {
    AssemblerBuffer::EnsureCapacity ensured(&buffer_);
    EmitUint8(0x66);
    EmitUint8(0x0F);
    EmitUint8(0xF9);
    EmitXmmRegisterOperand(dst, src);
}


void X86Assembler::pmullw(XmmRegister dst, XmmRegister src) {
    AssemblerBuffer::EnsureCapacity ensured(&buffer_);
    EmitUint8(0x66);
    EmitUint8(0x0F);
    EmitUint8(0xD5);
    EmitXmmRegisterOperand(dst, src);
}


void X86Assembler::paddd(XmmRegister dst, XmmRegister src) {
    AssemblerBuffer::EnsureCapacity ensured(&buffer_);
    EmitUint8(0x66);
    EmitUint8(0x0F);
    EmitUint8(0xFE);
    EmitXmmRegisterOperand(dst, src);
}


void X86Assembler::psubd(XmmRegister dst, XmmRegister src) {
    AssemblerBuffer::EnsureCapacity ensured(&buffer_);
    EmitUint8(0x66);
    EmitUint8(0x0F);
    EmitUint8(0xFA);
    EmitXmmRegisterOperand(dst, src);
}


void X86Assembler::pmulld(XmmRegister dst, XmmRegister src) {
    AssemblerBuffer::EnsureCapacity ensured(&buffer_);
    EmitUint8(0x66);
    EmitUint8(0x0F);
    EmitUint8(0x38);
    EmitUint8(0x40);
    EmitXmmRegisterOperand(dst, src);
}


void X86Assembler::paddq(XmmRegister dst, XmmRegister src) {
    AssemblerBuffer::EnsureCapacity ensured(&buffer_);
    EmitUint8(0x66);
    EmitUint8(0x0F);
    EmitUint8(0xD4);
    EmitXmmRegisterOperand(dst, src);
}


void X86Assembler::psubq(XmmRegister dst, XmmRegister src) {
    AssemblerBuffer::EnsureCapacity ensured(&buffer_);
    EmitUint8(0x66);
    EmitUint8(0x0F);
    EmitUint8(0xFB);
    EmitXmmRegisterOperand(dst, src);
}


void X86Assembler::paddusb(XmmRegister dst, XmmRegister src) {
    AssemblerBuffer::EnsureCapacity ensured(&buffer_);
    EmitUint8(0x66);
    EmitUint8(0x0F);
    EmitUint8(0xDC);
    EmitXmmRegisterOperand(dst, src);
}


void X86Assembler::paddsb(XmmRegister dst, XmmRegister src) {
    AssemblerBuffer::EnsureCapacity ensured(&buffer_);
    EmitUint8(0x66);
    EmitUint8(0x0F);
    EmitUint8(0xEC);
    EmitXmmRegisterOperand(dst, src);
}


void X86Assembler::paddusw(XmmRegister dst, XmmRegister src) {
    AssemblerBuffer::EnsureCapacity ensured(&buffer_);
    EmitUint8(0x66);
    EmitUint8(0x0F);
    EmitUint8(0xDD);
    EmitXmmRegisterOperand(dst, src);
}


void X86Assembler::paddsw(XmmRegister dst, XmmRegister src) {
    AssemblerBuffer::EnsureCapacity ensured(&buffer_);
    EmitUint8(0x66);
    EmitUint8(0x0F);
    EmitUint8(0xED);
    EmitXmmRegisterOperand(dst, src);
}


void X86Assembler::psubusb(XmmRegister dst, XmmRegister src) {
    AssemblerBuffer::EnsureCapacity ensured(&buffer_);
    EmitUint8(0x66);
    EmitUint8(0x0F);
    EmitUint8(0xD8);
    EmitXmmRegisterOperand(dst, src);
}


void X86Assembler::psubsb(XmmRegister dst, XmmRegister src) {
    AssemblerBuffer::EnsureCapacity ensured(&buffer_);
    EmitUint8(0x66);
    EmitUint8(0x0F);
    EmitUint8(0xE8);
    EmitXmmRegisterOperand(dst, src);
}


void X86Assembler::psubusw(XmmRegister dst, XmmRegister src) {
    AssemblerBuffer::EnsureCapacity ensured(&buffer_);
    EmitUint8(0x66);
    EmitUint8(0x0F);
    EmitUint8(0xD9);
    EmitXmmRegisterOperand(dst, src);
}


void X86Assembler::psubsw(XmmRegister dst, XmmRegister src) {
    AssemblerBuffer::EnsureCapacity ensured(&buffer_);
    EmitUint8(0x66);
    EmitUint8(0x0F);
    EmitUint8(0xE9);
    EmitXmmRegisterOperand(dst, src);
}


void X86Assembler::cvtsi2ss(XmmRegister dst, Register src) {
    AssemblerBuffer::EnsureCapacity ensured(&buffer_);
    EmitUint8(0xF3);
    EmitUint8(0x0F);
    EmitUint8(0x2A);
    EmitOperand(dst, Operand(src));
}


void X86Assembler::cvtsi2sd(XmmRegister dst, Register src) {
    AssemblerBuffer::EnsureCapacity ensured(&buffer_);
    EmitUint8(0xF2);
    EmitUint8(0x0F);
    EmitUint8(0x2A);
    EmitOperand(dst, Operand(src));
}


void X86Assembler::cvtss2si(Register dst, XmmRegister src) {
    AssemblerBuffer::EnsureCapacity ensured(&buffer_);
    EmitUint8(0xF3);
    EmitUint8(0x0F);
    EmitUint8(0x2D);
    EmitXmmRegisterOperand(dst, src);
}


void X86Assembler::cvtss2sd(XmmRegister dst, XmmRegister src) {
    AssemblerBuffer::EnsureCapacity ensured(&buffer_);
    EmitUint8(0xF3);
    EmitUint8(0x0F);
    EmitUint8(0x5A);
    EmitXmmRegisterOperand(dst, src);
}


void X86Assembler::cvtsd2si(Register dst, XmmRegister src) {
    AssemblerBuffer::EnsureCapacity ensured(&buffer_);
    EmitUint8(0xF2);
    EmitUint8(0x0F);
    EmitUint8(0x2D);
    EmitXmmRegisterOperand(dst, src);
}


void X86Assembler::cvttss2si(Register dst, XmmRegister src) {
    AssemblerBuffer::EnsureCapacity ensured(&buffer_);
    EmitUint8(0xF3);
    EmitUint8(0x0F);
    EmitUint8(0x2C);
    EmitXmmRegisterOperand(dst, src);
}


void X86Assembler::cvttsd2si(Register dst, XmmRegister src) {
    AssemblerBuffer::EnsureCapacity ensured(&buffer_);
    EmitUint8(0xF2);
    EmitUint8(0x0F);
    EmitUint8(0x2C);
    EmitXmmRegisterOperand(dst, src);
}


void X86Assembler::cvtsd2ss(XmmRegister dst, XmmRegister src) {
    AssemblerBuffer::EnsureCapacity ensured(&buffer_);
    EmitUint8(0xF2);
    EmitUint8(0x0F);
    EmitUint8(0x5A);
    EmitXmmRegisterOperand(dst, src);
}


void X86Assembler::cvtdq2ps(XmmRegister dst, XmmRegister src) {
    AssemblerBuffer::EnsureCapacity ensured(&buffer_);
    EmitUint8(0x0F);
    EmitUint8(0x5B);
    EmitXmmRegisterOperand(dst, src);
}


void X86Assembler::cvtdq2pd(XmmRegister dst, XmmRegister src) {
    AssemblerBuffer::EnsureCapacity ensured(&buffer_);
    EmitUint8(0xF3);
    EmitUint8(0x0F);
    EmitUint8(0xE6);
    EmitXmmRegisterOperand(dst, src);
}


void X86Assembler::comiss(XmmRegister a, XmmRegister b) {
    AssemblerBuffer::EnsureCapacity ensured(&buffer_);
    EmitUint8(0x0F);
    EmitUint8(0x2F);
    EmitXmmRegisterOperand(a, b);
}


void X86Assembler::comiss(XmmRegister a, const Address& b) {
    AssemblerBuffer::EnsureCapacity ensured(&buffer_);
    EmitUint8(0x0F);
    EmitUint8(0x2F);
    EmitOperand(a, b);
}


void X86Assembler::comisd(XmmRegister a, XmmRegister b) {
    AssemblerBuffer::EnsureCapacity ensured(&buffer_);
    EmitUint8(0x66);
    EmitUint8(0x0F);
    EmitUint8(0x2F);
    EmitXmmRegisterOperand(a, b);
}


void X86Assembler::comisd(XmmRegister a, const Address& b) {
    AssemblerBuffer::EnsureCapacity ensured(&buffer_);
    EmitUint8(0x66);
    EmitUint8(0x0F);
    EmitUint8(0x2F);
    EmitOperand(a, b);
}


void X86Assembler::ucomiss(XmmRegister a, XmmRegister b) {
    AssemblerBuffer::EnsureCapacity ensured(&buffer_);
    EmitUint8(0x0F);
    EmitUint8(0x2E);
    EmitXmmRegisterOperand(a, b);
}


void X86Assembler::ucomiss(XmmRegister a, const Address& b) {
    AssemblerBuffer::EnsureCapacity ensured(&buffer_);
    EmitUint8(0x0F);
    EmitUint8(0x2E);
    EmitOperand(a, b);
}


void X86Assembler::ucomisd(XmmRegister a, XmmRegister b) {
    AssemblerBuffer::EnsureCapacity ensured(&buffer_);
    EmitUint8(0x66);
    EmitUint8(0x0F);
    EmitUint8(0x2E);
    EmitXmmRegisterOperand(a, b);
}


void X86Assembler::ucomisd(XmmRegister a, const Address& b) {
    AssemblerBuffer::EnsureCapacity ensured(&buffer_);
    EmitUint8(0x66);
    EmitUint8(0x0F);
    EmitUint8(0x2E);
    EmitOperand(a, b);
}


void X86Assembler::roundsd(XmmRegister dst, XmmRegister src, const Immediate& imm) {
    AssemblerBuffer::EnsureCapacity ensured(&buffer_);
    EmitUint8(0x66);
    EmitUint8(0x0F);
    EmitUint8(0x3A);
    EmitUint8(0x0B);
    EmitXmmRegisterOperand(dst, src);
    EmitUint8(imm.value());
}


void X86Assembler::roundss(XmmRegister dst, XmmRegister src, const Immediate& imm) {
    AssemblerBuffer::EnsureCapacity ensured(&buffer_);
    EmitUint8(0x66);
    EmitUint8(0x0F);
    EmitUint8(0x3A);
    EmitUint8(0x0A);
    EmitXmmRegisterOperand(dst, src);
    EmitUint8(imm.value());
}


void X86Assembler::sqrtsd(XmmRegister dst, XmmRegister src) {
    AssemblerBuffer::EnsureCapacity ensured(&buffer_);
    EmitUint8(0xF2);
    EmitUint8(0x0F);
    EmitUint8(0x51);
    EmitXmmRegisterOperand(dst, src);
}


void X86Assembler::sqrtss(XmmRegister dst, XmmRegister src) {
    AssemblerBuffer::EnsureCapacity ensured(&buffer_);
    EmitUint8(0xF3);
    EmitUint8(0x0F);
    EmitUint8(0x51);
    EmitXmmRegisterOperand(dst, src);
}


void X86Assembler::xorpd(XmmRegister dst, const Address& src) {
    AssemblerBuffer::EnsureCapacity ensured(&buffer_);
    EmitUint8(0x66);
    EmitUint8(0x0F);
    EmitUint8(0x57);
    EmitOperand(dst, src);
}


void X86Assembler::xorpd(XmmRegister dst, XmmRegister src) {
    AssemblerBuffer::EnsureCapacity ensured(&buffer_);
    EmitUint8(0x66);
    EmitUint8(0x0F);
    EmitUint8(0x57);
    EmitXmmRegisterOperand(dst, src);
}


void X86Assembler::xorps(XmmRegister dst, const Address& src) {
    AssemblerBuffer::EnsureCapacity ensured(&buffer_);
    EmitUint8(0x0F);
    EmitUint8(0x57);
    EmitOperand(dst, src);
}


void X86Assembler::xorps(XmmRegister dst, XmmRegister src) {
    AssemblerBuffer::EnsureCapacity ensured(&buffer_);
    EmitUint8(0x0F);
    EmitUint8(0x57);
    EmitXmmRegisterOperand(dst, src);
}


void X86Assembler::pxor(XmmRegister dst, XmmRegister src) {
    AssemblerBuffer::EnsureCapacity ensured(&buffer_);
    EmitUint8(0x66);
    EmitUint8(0x0F);
    EmitUint8(0xEF);
    EmitXmmRegisterOperand(dst, src);
}


void X86Assembler::andpd(XmmRegister dst, XmmRegister src) {
    AssemblerBuffer::EnsureCapacity ensured(&buffer_);
    EmitUint8(0x66);
    EmitUint8(0x0F);
    EmitUint8(0x54);
    EmitXmmRegisterOperand(dst, src);
}


void X86Assembler::andpd(XmmRegister dst, const Address& src) {
    AssemblerBuffer::EnsureCapacity ensured(&buffer_);
    EmitUint8(0x66);
    EmitUint8(0x0F);
    EmitUint8(0x54);
    EmitOperand(dst, src);
}


void X86Assembler::andps(XmmRegister dst, XmmRegister src) {
    AssemblerBuffer::EnsureCapacity ensured(&buffer_);
    EmitUint8(0x0F);
    EmitUint8(0x54);
    EmitXmmRegisterOperand(dst, src);
}


void X86Assembler::andps(XmmRegister dst, const Address& src) {
    AssemblerBuffer::EnsureCapacity ensured(&buffer_);
    EmitUint8(0x0F);
    EmitUint8(0x54);
    EmitOperand(dst, src);
}


void X86Assembler::pand(XmmRegister dst, XmmRegister src) {
    AssemblerBuffer::EnsureCapacity ensured(&buffer_);
    EmitUint8(0x66);
    EmitUint8(0x0F);
    EmitUint8(0xDB);
    EmitXmmRegisterOperand(dst, src);
}

void X86Assembler::andn(Register dst, Register src1, Register src2) {
    AssemblerBuffer::EnsureCapacity ensured(&buffer_);
    uint8_t byte_zero = EmitVexByteZero(/*is_two_byte=*/ false);
    uint8_t byte_one = EmitVexByte1(/*r=*/ false,
            /*x=*/ false,
            /*b=*/ false,
            /*mmmmm=*/ 2);
    uint8_t byte_two = EmitVexByte2(/*w=*/ false,
            /*l=*/ 128,
                                           X86ManagedRegister::FromCpuRegister(src1),
            /*pp=*/ 0);
    EmitUint8(byte_zero);
    EmitUint8(byte_one);
    EmitUint8(byte_two);
    // Opcode field
    EmitUint8(0xF2);
    EmitRegisterOperand(dst, src2);
}


void X86Assembler::andnpd(XmmRegister dst, XmmRegister src) {
    AssemblerBuffer::EnsureCapacity ensured(&buffer_);
    EmitUint8(0x66);
    EmitUint8(0x0F);
    EmitUint8(0x55);
    EmitXmmRegisterOperand(dst, src);
}


void X86Assembler::andnps(XmmRegister dst, XmmRegister src) {
    AssemblerBuffer::EnsureCapacity ensured(&buffer_);
    EmitUint8(0x0F);
    EmitUint8(0x55);
    EmitXmmRegisterOperand(dst, src);
}


void X86Assembler::pandn(XmmRegister dst, XmmRegister src) {
    AssemblerBuffer::EnsureCapacity ensured(&buffer_);
    EmitUint8(0x66);
    EmitUint8(0x0F);
    EmitUint8(0xDF);
    EmitXmmRegisterOperand(dst, src);
}


void X86Assembler::orpd(XmmRegister dst, XmmRegister src) {
    AssemblerBuffer::EnsureCapacity ensured(&buffer_);
    EmitUint8(0x66);
    EmitUint8(0x0F);
    EmitUint8(0x56);
    EmitXmmRegisterOperand(dst, src);
}


void X86Assembler::orps(XmmRegister dst, XmmRegister src) {
    AssemblerBuffer::EnsureCapacity ensured(&buffer_);
    EmitUint8(0x0F);
    EmitUint8(0x56);
    EmitXmmRegisterOperand(dst, src);
}


void X86Assembler::por(XmmRegister dst, XmmRegister src) {
    AssemblerBuffer::EnsureCapacity ensured(&buffer_);
    EmitUint8(0x66);
    EmitUint8(0x0F);
    EmitUint8(0xEB);
    EmitXmmRegisterOperand(dst, src);
}


void X86Assembler::pavgb(XmmRegister dst, XmmRegister src) {
    AssemblerBuffer::EnsureCapacity ensured(&buffer_);
    EmitUint8(0x66);
    EmitUint8(0x0F);
    EmitUint8(0xE0);
    EmitXmmRegisterOperand(dst, src);
}


void X86Assembler::pavgw(XmmRegister dst, XmmRegister src) {
    AssemblerBuffer::EnsureCapacity ensured(&buffer_);
    EmitUint8(0x66);
    EmitUint8(0x0F);
    EmitUint8(0xE3);
    EmitXmmRegisterOperand(dst, src);
}


void X86Assembler::psadbw(XmmRegister dst, XmmRegister src) {
    AssemblerBuffer::EnsureCapacity ensured(&buffer_);
    EmitUint8(0x66);
    EmitUint8(0x0F);
    EmitUint8(0xF6);
    EmitXmmRegisterOperand(dst, src);
}


void X86Assembler::pmaddwd(XmmRegister dst, XmmRegister src) {
    AssemblerBuffer::EnsureCapacity ensured(&buffer_);
    EmitUint8(0x66);
    EmitUint8(0x0F);
    EmitUint8(0xF5);
    EmitXmmRegisterOperand(dst, src);
}


void X86Assembler::phaddw(XmmRegister dst, XmmRegister src) {
    AssemblerBuffer::EnsureCapacity ensured(&buffer_);
    EmitUint8(0x66);
    EmitUint8(0x0F);
    EmitUint8(0x38);
    EmitUint8(0x01);
    EmitXmmRegisterOperand(dst, src);
}


void X86Assembler::phaddd(XmmRegister dst, XmmRegister src) {
    AssemblerBuffer::EnsureCapacity ensured(&buffer_);
    EmitUint8(0x66);
    EmitUint8(0x0F);
    EmitUint8(0x38);
    EmitUint8(0x02);
    EmitXmmRegisterOperand(dst, src);
}


void X86Assembler::haddps(XmmRegister dst, XmmRegister src) {
    AssemblerBuffer::EnsureCapacity ensured(&buffer_);
    EmitUint8(0xF2);
    EmitUint8(0x0F);
    EmitUint8(0x7C);
    EmitXmmRegisterOperand(dst, src);
}


void X86Assembler::haddpd(XmmRegister dst, XmmRegister src) {
    AssemblerBuffer::EnsureCapacity ensured(&buffer_);
    EmitUint8(0x66);
    EmitUint8(0x0F);
    EmitUint8(0x7C);
    EmitXmmRegisterOperand(dst, src);
}


void X86Assembler::phsubw(XmmRegister dst, XmmRegister src) {
    AssemblerBuffer::EnsureCapacity ensured(&buffer_);
    EmitUint8(0x66);
    EmitUint8(0x0F);
    EmitUint8(0x38);
    EmitUint8(0x05);
    EmitXmmRegisterOperand(dst, src);
}


void X86Assembler::phsubd(XmmRegister dst, XmmRegister src) {
    AssemblerBuffer::EnsureCapacity ensured(&buffer_);
    EmitUint8(0x66);
    EmitUint8(0x0F);
    EmitUint8(0x38);
    EmitUint8(0x06);
    EmitXmmRegisterOperand(dst, src);
}


void X86Assembler::hsubps(XmmRegister dst, XmmRegister src) {
    AssemblerBuffer::EnsureCapacity ensured(&buffer_);
    EmitUint8(0xF2);
    EmitUint8(0x0F);
    EmitUint8(0x7D);
    EmitXmmRegisterOperand(dst, src);
}


void X86Assembler::hsubpd(XmmRegister dst, XmmRegister src) {
    AssemblerBuffer::EnsureCapacity ensured(&buffer_);
    EmitUint8(0x66);
    EmitUint8(0x0F);
    EmitUint8(0x7D);
    EmitXmmRegisterOperand(dst, src);
}


void X86Assembler::pminsb(XmmRegister dst, XmmRegister src) {
    AssemblerBuffer::EnsureCapacity ensured(&buffer_);
    EmitUint8(0x66);
    EmitUint8(0x0F);
    EmitUint8(0x38);
    EmitUint8(0x38);
    EmitXmmRegisterOperand(dst, src);
}

void X86Assembler::pmaxsb(XmmRegister dst, XmmRegister src) {
    AssemblerBuffer::EnsureCapacity ensured(&buffer_);
    EmitUint8(0x66);
    EmitUint8(0x0F);
    EmitUint8(0x38);
    EmitUint8(0x3C);
    EmitXmmRegisterOperand(dst, src);
}

void X86Assembler::pminsw(XmmRegister dst, XmmRegister src) {
    AssemblerBuffer::EnsureCapacity ensured(&buffer_);
    EmitUint8(0x66);
    EmitUint8(0x0F);
    EmitUint8(0xEA);
    EmitXmmRegisterOperand(dst, src);
}

void X86Assembler::pmaxsw(XmmRegister dst, XmmRegister src) {
    AssemblerBuffer::EnsureCapacity ensured(&buffer_);
    EmitUint8(0x66);
    EmitUint8(0x0F);
    EmitUint8(0xEE);
    EmitXmmRegisterOperand(dst, src);
}

void X86Assembler::pminsd(XmmRegister dst, XmmRegister src) {
    AssemblerBuffer::EnsureCapacity ensured(&buffer_);
    EmitUint8(0x66);
    EmitUint8(0x0F);
    EmitUint8(0x38);
    EmitUint8(0x39);
    EmitXmmRegisterOperand(dst, src);
}

void X86Assembler::pmaxsd(XmmRegister dst, XmmRegister src) {
    AssemblerBuffer::EnsureCapacity ensured(&buffer_);
    EmitUint8(0x66);
    EmitUint8(0x0F);
    EmitUint8(0x38);
    EmitUint8(0x3D);
    EmitXmmRegisterOperand(dst, src);
}

void X86Assembler::pminub(XmmRegister dst, XmmRegister src) {
    AssemblerBuffer::EnsureCapacity ensured(&buffer_);
    EmitUint8(0x66);
    EmitUint8(0x0F);
    EmitUint8(0xDA);
    EmitXmmRegisterOperand(dst, src);
}

void X86Assembler::pmaxub(XmmRegister dst, XmmRegister src) {
    AssemblerBuffer::EnsureCapacity ensured(&buffer_);
    EmitUint8(0x66);
    EmitUint8(0x0F);
    EmitUint8(0xDE);
    EmitXmmRegisterOperand(dst, src);
}

void X86Assembler::pminuw(XmmRegister dst, XmmRegister src) {
    AssemblerBuffer::EnsureCapacity ensured(&buffer_);
    EmitUint8(0x66);
    EmitUint8(0x0F);
    EmitUint8(0x38);
    EmitUint8(0x3A);
    EmitXmmRegisterOperand(dst, src);
}

void X86Assembler::pmaxuw(XmmRegister dst, XmmRegister src) {
    AssemblerBuffer::EnsureCapacity ensured(&buffer_);
    EmitUint8(0x66);
    EmitUint8(0x0F);
    EmitUint8(0x38);
    EmitUint8(0x3E);
    EmitXmmRegisterOperand(dst, src);
}

void X86Assembler::pminud(XmmRegister dst, XmmRegister src) {
    AssemblerBuffer::EnsureCapacity ensured(&buffer_);
    EmitUint8(0x66);
    EmitUint8(0x0F);
    EmitUint8(0x38);
    EmitUint8(0x3B);
    EmitXmmRegisterOperand(dst, src);
}

void X86Assembler::pmaxud(XmmRegister dst, XmmRegister src) {
    AssemblerBuffer::EnsureCapacity ensured(&buffer_);
    EmitUint8(0x66);
    EmitUint8(0x0F);
    EmitUint8(0x38);
    EmitUint8(0x3F);
    EmitXmmRegisterOperand(dst, src);
}

void X86Assembler::minps(XmmRegister dst, XmmRegister src) {
    AssemblerBuffer::EnsureCapacity ensured(&buffer_);
    EmitUint8(0x0F);
    EmitUint8(0x5D);
    EmitXmmRegisterOperand(dst, src);
}

void X86Assembler::maxps(XmmRegister dst, XmmRegister src) {
    AssemblerBuffer::EnsureCapacity ensured(&buffer_);
    EmitUint8(0x0F);
    EmitUint8(0x5F);
    EmitXmmRegisterOperand(dst, src);
}

void X86Assembler::minpd(XmmRegister dst, XmmRegister src) {
    AssemblerBuffer::EnsureCapacity ensured(&buffer_);
    EmitUint8(0x66);
    EmitUint8(0x0F);
    EmitUint8(0x5D);
    EmitXmmRegisterOperand(dst, src);
}

void X86Assembler::maxpd(XmmRegister dst, XmmRegister src) {
    AssemblerBuffer::EnsureCapacity ensured(&buffer_);
    EmitUint8(0x66);
    EmitUint8(0x0F);
    EmitUint8(0x5F);
    EmitXmmRegisterOperand(dst, src);
}

void X86Assembler::pcmpeqb(XmmRegister dst, XmmRegister src) {
    AssemblerBuffer::EnsureCapacity ensured(&buffer_);
    EmitUint8(0x66);
    EmitUint8(0x0F);
    EmitUint8(0x74);
    EmitXmmRegisterOperand(dst, src);
}


void X86Assembler::pcmpeqw(XmmRegister dst, XmmRegister src) {
    AssemblerBuffer::EnsureCapacity ensured(&buffer_);
    EmitUint8(0x66);
    EmitUint8(0x0F);
    EmitUint8(0x75);
    EmitXmmRegisterOperand(dst, src);
}


void X86Assembler::pcmpeqd(XmmRegister dst, XmmRegister src) {
    AssemblerBuffer::EnsureCapacity ensured(&buffer_);
    EmitUint8(0x66);
    EmitUint8(0x0F);
    EmitUint8(0x76);
    EmitXmmRegisterOperand(dst, src);
}


void X86Assembler::pcmpeqq(XmmRegister dst, XmmRegister src) {
    AssemblerBuffer::EnsureCapacity ensured(&buffer_);
    EmitUint8(0x66);
    EmitUint8(0x0F);
    EmitUint8(0x38);
    EmitUint8(0x29);
    EmitXmmRegisterOperand(dst, src);
}


void X86Assembler::pcmpgtb(XmmRegister dst, XmmRegister src) {
    AssemblerBuffer::EnsureCapacity ensured(&buffer_);
    EmitUint8(0x66);
    EmitUint8(0x0F);
    EmitUint8(0x64);
    EmitXmmRegisterOperand(dst, src);
}


void X86Assembler::pcmpgtw(XmmRegister dst, XmmRegister src) {
    AssemblerBuffer::EnsureCapacity ensured(&buffer_);
    EmitUint8(0x66);
    EmitUint8(0x0F);
    EmitUint8(0x65);
    EmitXmmRegisterOperand(dst, src);
}


void X86Assembler::pcmpgtd(XmmRegister dst, XmmRegister src) {
    AssemblerBuffer::EnsureCapacity ensured(&buffer_);
    EmitUint8(0x66);
    EmitUint8(0x0F);
    EmitUint8(0x66);
    EmitXmmRegisterOperand(dst, src);
}


void X86Assembler::pcmpgtq(XmmRegister dst, XmmRegister src) {
    AssemblerBuffer::EnsureCapacity ensured(&buffer_);
    EmitUint8(0x66);
    EmitUint8(0x0F);
    EmitUint8(0x38);
    EmitUint8(0x37);
    EmitXmmRegisterOperand(dst, src);
}


void X86Assembler::shufpd(XmmRegister dst, XmmRegister src, const Immediate& imm) {
    AssemblerBuffer::EnsureCapacity ensured(&buffer_);
    EmitUint8(0x66);
    EmitUint8(0x0F);
    EmitUint8(0xC6);
    EmitXmmRegisterOperand(dst, src);
    EmitUint8(imm.value());
}


void X86Assembler::shufps(XmmRegister dst, XmmRegister src, const Immediate& imm) {
    AssemblerBuffer::EnsureCapacity ensured(&buffer_);
    EmitUint8(0x0F);
    EmitUint8(0xC6);
    EmitXmmRegisterOperand(dst, src);
    EmitUint8(imm.value());
}


void X86Assembler::pshufd(XmmRegister dst, XmmRegister src, const Immediate& imm) {
    AssemblerBuffer::EnsureCapacity ensured(&buffer_);
    EmitUint8(0x66);
    EmitUint8(0x0F);
    EmitUint8(0x70);
    EmitXmmRegisterOperand(dst, src);
    EmitUint8(imm.value());
}


void X86Assembler::punpcklbw(XmmRegister dst, XmmRegister src) {
    AssemblerBuffer::EnsureCapacity ensured(&buffer_);
    EmitUint8(0x66);
    EmitUint8(0x0F);
    EmitUint8(0x60);
    EmitXmmRegisterOperand(dst, src);
}


void X86Assembler::punpcklwd(XmmRegister dst, XmmRegister src) {
    AssemblerBuffer::EnsureCapacity ensured(&buffer_);
    EmitUint8(0x66);
    EmitUint8(0x0F);
    EmitUint8(0x61);
    EmitXmmRegisterOperand(dst, src);
}


void X86Assembler::punpckldq(XmmRegister dst, XmmRegister src) {
    AssemblerBuffer::EnsureCapacity ensured(&buffer_);
    EmitUint8(0x66);
    EmitUint8(0x0F);
    EmitUint8(0x62);
    EmitXmmRegisterOperand(dst, src);
}


void X86Assembler::punpcklqdq(XmmRegister dst, XmmRegister src) {
    AssemblerBuffer::EnsureCapacity ensured(&buffer_);
    EmitUint8(0x66);
    EmitUint8(0x0F);
    EmitUint8(0x6C);
    EmitXmmRegisterOperand(dst, src);
}


void X86Assembler::punpckhbw(XmmRegister dst, XmmRegister src) {
    AssemblerBuffer::EnsureCapacity ensured(&buffer_);
    EmitUint8(0x66);
    EmitUint8(0x0F);
    EmitUint8(0x68);
    EmitXmmRegisterOperand(dst, src);
}


void X86Assembler::punpckhwd(XmmRegister dst, XmmRegister src) {
    AssemblerBuffer::EnsureCapacity ensured(&buffer_);
    EmitUint8(0x66);
    EmitUint8(0x0F);
    EmitUint8(0x69);
    EmitXmmRegisterOperand(dst, src);
}


void X86Assembler::punpckhdq(XmmRegister dst, XmmRegister src) {
    AssemblerBuffer::EnsureCapacity ensured(&buffer_);
    EmitUint8(0x66);
    EmitUint8(0x0F);
    EmitUint8(0x6A);
    EmitXmmRegisterOperand(dst, src);
}


void X86Assembler::punpckhqdq(XmmRegister dst, XmmRegister src) {
    AssemblerBuffer::EnsureCapacity ensured(&buffer_);
    EmitUint8(0x66);
    EmitUint8(0x0F);
    EmitUint8(0x6D);
    EmitXmmRegisterOperand(dst, src);
}


void X86Assembler::psllw(XmmRegister reg, const Immediate& shift_count) {
    DCHECK(shift_count.is_uint8());
    AssemblerBuffer::EnsureCapacity ensured(&buffer_);
    EmitUint8(0x66);
    EmitUint8(0x0F);
    EmitUint8(0x71);
    EmitXmmRegisterOperand(6, reg);
    EmitUint8(shift_count.value());
}


void X86Assembler::pslld(XmmRegister reg, const Immediate& shift_count) {
    DCHECK(shift_count.is_uint8());
    AssemblerBuffer::EnsureCapacity ensured(&buffer_);
    EmitUint8(0x66);
    EmitUint8(0x0F);
    EmitUint8(0x72);
    EmitXmmRegisterOperand(6, reg);
    EmitUint8(shift_count.value());
}


void X86Assembler::psllq(XmmRegister reg, const Immediate& shift_count) {
    DCHECK(shift_count.is_uint8());
    AssemblerBuffer::EnsureCapacity ensured(&buffer_);
    EmitUint8(0x66);
    EmitUint8(0x0F);
    EmitUint8(0x73);
    EmitXmmRegisterOperand(6, reg);
    EmitUint8(shift_count.value());
}


void X86Assembler::psraw(XmmRegister reg, const Immediate& shift_count) {
    DCHECK(shift_count.is_uint8());
    AssemblerBuffer::EnsureCapacity ensured(&buffer_);
    EmitUint8(0x66);
    EmitUint8(0x0F);
    EmitUint8(0x71);
    EmitXmmRegisterOperand(4, reg);
    EmitUint8(shift_count.value());
}


void X86Assembler::psrad(XmmRegister reg, const Immediate& shift_count) {
    DCHECK(shift_count.is_uint8());
    AssemblerBuffer::EnsureCapacity ensured(&buffer_);
    EmitUint8(0x66);
    EmitUint8(0x0F);
    EmitUint8(0x72);
    EmitXmmRegisterOperand(4, reg);
    EmitUint8(shift_count.value());
}


void X86Assembler::psrlw(XmmRegister reg, const Immediate& shift_count) {
    DCHECK(shift_count.is_uint8());
    AssemblerBuffer::EnsureCapacity ensured(&buffer_);
    EmitUint8(0x66);
    EmitUint8(0x0F);
    EmitUint8(0x71);
    EmitXmmRegisterOperand(2, reg);
    EmitUint8(shift_count.value());
}


void X86Assembler::psrld(XmmRegister reg, const Immediate& shift_count) {
    DCHECK(shift_count.is_uint8());
    AssemblerBuffer::EnsureCapacity ensured(&buffer_);
    EmitUint8(0x66);
    EmitUint8(0x0F);
    EmitUint8(0x72);
    EmitXmmRegisterOperand(2, reg);
    EmitUint8(shift_count.value());
}


void X86Assembler::psrlq(XmmRegister reg, const Immediate& shift_count) {
    DCHECK(shift_count.is_uint8());
    AssemblerBuffer::EnsureCapacity ensured(&buffer_);
    EmitUint8(0x66);
    EmitUint8(0x0F);
    EmitUint8(0x73);
    EmitXmmRegisterOperand(2, reg);
    EmitUint8(shift_count.value());
}


void X86Assembler::psrldq(XmmRegister reg, const Immediate& shift_count) {
    DCHECK(shift_count.is_uint8());
    AssemblerBuffer::EnsureCapacity ensured(&buffer_);
    EmitUint8(0x66);
    EmitUint8(0x0F);
    EmitUint8(0x73);
    EmitXmmRegisterOperand(3, reg);
    EmitUint8(shift_count.value());
}


void X86Assembler::fldl(const Address& src) {
    AssemblerBuffer::EnsureCapacity ensured(&buffer_);
    EmitUint8(0xDD);
    EmitOperand(0, src);
}


void X86Assembler::fstl(const Address& dst) {
    AssemblerBuffer::EnsureCapacity ensured(&buffer_);
    EmitUint8(0xDD);
    EmitOperand(2, dst);
}


void X86Assembler::fstpl(const Address& dst) {
    AssemblerBuffer::EnsureCapacity ensured(&buffer_);
    EmitUint8(0xDD);
    EmitOperand(3, dst);
}


void X86Assembler::fstsw() {
    AssemblerBuffer::EnsureCapacity ensured(&buffer_);
    EmitUint8(0x9B);
    EmitUint8(0xDF);
    EmitUint8(0xE0);
}


void X86Assembler::fnstcw(const Address& dst) {
    AssemblerBuffer::EnsureCapacity ensured(&buffer_);
    EmitUint8(0xD9);
    EmitOperand(7, dst);
}


void X86Assembler::fldcw(const Address& src) {
    AssemblerBuffer::EnsureCapacity ensured(&buffer_);
    EmitUint8(0xD9);
    EmitOperand(5, src);
}


void X86Assembler::fistpl(const Address& dst) {
    AssemblerBuffer::EnsureCapacity ensured(&buffer_);
    EmitUint8(0xDF);
    EmitOperand(7, dst);
}


void X86Assembler::fistps(const Address& dst) {
    AssemblerBuffer::EnsureCapacity ensured(&buffer_);
    EmitUint8(0xDB);
    EmitOperand(3, dst);
}


void X86Assembler::fildl(const Address& src) {
    AssemblerBuffer::EnsureCapacity ensured(&buffer_);
    EmitUint8(0xDF);
    EmitOperand(5, src);
}


void X86Assembler::filds(const Address& src) {
    AssemblerBuffer::EnsureCapacity ensured(&buffer_);
    EmitUint8(0xDB);
    EmitOperand(0, src);
}


void X86Assembler::fincstp() {
    AssemblerBuffer::EnsureCapacity ensured(&buffer_);
    EmitUint8(0xD9);
    EmitUint8(0xF7);
}


void X86Assembler::ffree(const Immediate& index) {
    CHECK_LT(index.value(), 7);
    AssemblerBuffer::EnsureCapacity ensured(&buffer_);
    EmitUint8(0xDD);
    EmitUint8(0xC0 + index.value());
}


void X86Assembler::fsin() {
    AssemblerBuffer::EnsureCapacity ensured(&buffer_);
    EmitUint8(0xD9);
    EmitUint8(0xFE);
}


void X86Assembler::fcos() {
    AssemblerBuffer::EnsureCapacity ensured(&buffer_);
    EmitUint8(0xD9);
    EmitUint8(0xFF);
}


void X86Assembler::fptan() {
    AssemblerBuffer::EnsureCapacity ensured(&buffer_);
    EmitUint8(0xD9);
    EmitUint8(0xF2);
}


void X86Assembler::fucompp() {
    AssemblerBuffer::EnsureCapacity ensured(&buffer_);
    EmitUint8(0xDA);
    EmitUint8(0xE9);
}


void X86Assembler::fprem() {
    AssemblerBuffer::EnsureCapacity ensured(&buffer_);
    EmitUint8(0xD9);
    EmitUint8(0xF8);
}


void X86Assembler::xchgl(Register dst, Register src) {
    AssemblerBuffer::EnsureCapacity ensured(&buffer_);
    EmitUint8(0x87);
    EmitRegisterOperand(dst, src);
}


void X86Assembler::xchgl(Register reg, const Address& address) {
    AssemblerBuffer::EnsureCapacity ensured(&buffer_);
    EmitUint8(0x87);
    EmitOperand(reg, address);
}


void X86Assembler::cmpb(const Address& address, const Immediate& imm) {
    AssemblerBuffer::EnsureCapacity ensured(&buffer_);
    EmitUint8(0x80);
    EmitOperand(7, address);
    EmitUint8(imm.value() & 0xFF);
}


void X86Assembler::cmpw(const Address& address, const Immediate& imm) {
    AssemblerBuffer::EnsureCapacity ensured(&buffer_);
    EmitUint8(0x66);
    EmitComplex(7, address, imm, /* is_16_op= */ true);
}


void X86Assembler::cmpl(Register reg, const Immediate& imm) {
    AssemblerBuffer::EnsureCapacity ensured(&buffer_);
    EmitComplex(7, Operand(reg), imm);
}


void X86Assembler::cmpl(Register reg0, Register reg1) {
    AssemblerBuffer::EnsureCapacity ensured(&buffer_);
    EmitUint8(0x3B);
    EmitOperand(reg0, Operand(reg1));
}


void X86Assembler::cmpl(Register reg, const Address& address) {
    AssemblerBuffer::EnsureCapacity ensured(&buffer_);
    EmitUint8(0x3B);
    EmitOperand(reg, address);
}


void X86Assembler::addl(Register dst, Register src) {
    AssemblerBuffer::EnsureCapacity ensured(&buffer_);
    EmitUint8(0x03);
    EmitRegisterOperand(dst, src);
}


void X86Assembler::addl(Register reg, const Address& address) {
    AssemblerBuffer::EnsureCapacity ensured(&buffer_);
    EmitUint8(0x03);
    EmitOperand(reg, address);
}


void X86Assembler::cmpl(const Address& address, Register reg) {
    AssemblerBuffer::EnsureCapacity ensured(&buffer_);
    EmitUint8(0x39);
    EmitOperand(reg, address);
}


void X86Assembler::cmpl(const Address& address, const Immediate& imm) {
    AssemblerBuffer::EnsureCapacity ensured(&buffer_);
    EmitComplex(7, address, imm);
}


void X86Assembler::testl(Register reg1, Register reg2) {
    AssemblerBuffer::EnsureCapacity ensured(&buffer_);
    EmitUint8(0x85);
    EmitRegisterOperand(reg1, reg2);
}


void X86Assembler::testl(Register reg, const Address& address) {
    AssemblerBuffer::EnsureCapacity ensured(&buffer_);
    EmitUint8(0x85);
    EmitOperand(reg, address);
}


void X86Assembler::testl(Register reg, const Immediate& immediate) {
    AssemblerBuffer::EnsureCapacity ensured(&buffer_);
    // For registers that have a byte variant (EAX, EBX, ECX, and EDX)
    // we only test the byte register to keep the encoding short.
    if (immediate.is_uint8() && reg < 4) {
        // Use zero-extended 8-bit immediate.
        if (reg == EAX) {
            EmitUint8(0xA8);
        } else {
            EmitUint8(0xF6);
            EmitUint8(0xC0 + reg);
        }
        EmitUint8(immediate.value() & 0xFF);
    } else if (reg == EAX) {
        // Use short form if the destination is EAX.
        EmitUint8(0xA9);
        EmitImmediate(immediate);
    } else {
        EmitUint8(0xF7);
        EmitOperand(0, Operand(reg));
        EmitImmediate(immediate);
    }
}


void X86Assembler::testb(const Address& dst, const Immediate& imm) {
    AssemblerBuffer::EnsureCapacity ensured(&buffer_);
    EmitUint8(0xF6);
    EmitOperand(EAX, dst);
    CHECK(imm.is_int8());
    EmitUint8(imm.value() & 0xFF);
}


void X86Assembler::testl(const Address& dst, const Immediate& imm) {
    AssemblerBuffer::EnsureCapacity ensured(&buffer_);
    EmitUint8(0xF7);
    EmitOperand(0, dst);
    EmitImmediate(imm);
}


void X86Assembler::andl(Register dst, Register src) {
    AssemblerBuffer::EnsureCapacity ensured(&buffer_);
    EmitUint8(0x23);
    EmitOperand(dst, Operand(src));
}


void X86Assembler::andl(Register reg, const Address& address) {
    AssemblerBuffer::EnsureCapacity ensured(&buffer_);
    EmitUint8(0x23);
    EmitOperand(reg, address);
}


void X86Assembler::andl(Register dst, const Immediate& imm) {
    AssemblerBuffer::EnsureCapacity ensured(&buffer_);
    EmitComplex(4, Operand(dst), imm);
}


void X86Assembler::orl(Register dst, Register src) {
    AssemblerBuffer::EnsureCapacity ensured(&buffer_);
    EmitUint8(0x0B);
    EmitOperand(dst, Operand(src));
}


void X86Assembler::orl(Register reg, const Address& address) {
    AssemblerBuffer::EnsureCapacity ensured(&buffer_);
    EmitUint8(0x0B);
    EmitOperand(reg, address);
}


void X86Assembler::orl(Register dst, const Immediate& imm) {
    AssemblerBuffer::EnsureCapacity ensured(&buffer_);
    EmitComplex(1, Operand(dst), imm);
}


void X86Assembler::xorl(Register dst, Register src) {
    AssemblerBuffer::EnsureCapacity ensured(&buffer_);
    EmitUint8(0x33);
    EmitOperand(dst, Operand(src));
}


void X86Assembler::xorl(Register reg, const Address& address) {
    AssemblerBuffer::EnsureCapacity ensured(&buffer_);
    EmitUint8(0x33);
    EmitOperand(reg, address);
}


void X86Assembler::xorl(Register dst, const Immediate& imm) {
    AssemblerBuffer::EnsureCapacity ensured(&buffer_);
    EmitComplex(6, Operand(dst), imm);
}


void X86Assembler::addl(Register reg, const Immediate& imm) {
    AssemblerBuffer::EnsureCapacity ensured(&buffer_);
    EmitComplex(0, Operand(reg), imm);
}


void X86Assembler::addl(const Address& address, Register reg) {
    AssemblerBuffer::EnsureCapacity ensured(&buffer_);
    EmitUint8(0x01);
    EmitOperand(reg, address);
}


void X86Assembler::addl(const Address& address, const Immediate& imm) {
    AssemblerBuffer::EnsureCapacity ensured(&buffer_);
    EmitComplex(0, address, imm);
}


void X86Assembler::addw(const Address& address, const Immediate& imm) {
    AssemblerBuffer::EnsureCapacity ensured(&buffer_);
    CHECK(imm.is_uint16() || imm.is_int16()) << imm.value();
    EmitUint8(0x66);
    EmitComplex(0, address, imm, /* is_16_op= */ true);
}


void X86Assembler::adcl(Register reg, const Immediate& imm) {
    AssemblerBuffer::EnsureCapacity ensured(&buffer_);
    EmitComplex(2, Operand(reg), imm);
}


void X86Assembler::adcl(Register dst, Register src) {
    AssemblerBuffer::EnsureCapacity ensured(&buffer_);
    EmitUint8(0x13);
    EmitOperand(dst, Operand(src));
}


void X86Assembler::adcl(Register dst, const Address& address) {
    AssemblerBuffer::EnsureCapacity ensured(&buffer_);
    EmitUint8(0x13);
    EmitOperand(dst, address);
}


void X86Assembler::subl(Register dst, Register src) {
    AssemblerBuffer::EnsureCapacity ensured(&buffer_);
    EmitUint8(0x2B);
    EmitOperand(dst, Operand(src));
}


void X86Assembler::subl(Register reg, const Immediate& imm) {
    AssemblerBuffer::EnsureCapacity ensured(&buffer_);
    EmitComplex(5, Operand(reg), imm);
}


void X86Assembler::subl(Register reg, const Address& address) {
    AssemblerBuffer::EnsureCapacity ensured(&buffer_);
    EmitUint8(0x2B);
    EmitOperand(reg, address);
}


void X86Assembler::subl(const Address& address, Register reg) {
    AssemblerBuffer::EnsureCapacity ensured(&buffer_);
    EmitUint8(0x29);
    EmitOperand(reg, address);
}


void X86Assembler::cdq() {
    AssemblerBuffer::EnsureCapacity ensured(&buffer_);
    EmitUint8(0x99);
}


void X86Assembler::idivl(Register reg) {
    AssemblerBuffer::EnsureCapacity ensured(&buffer_);
    EmitUint8(0xF7);
    EmitUint8(0xF8 | reg);
}


void X86Assembler::imull(Register dst, Register src) {
    AssemblerBuffer::EnsureCapacity ensured(&buffer_);
    EmitUint8(0x0F);
    EmitUint8(0xAF);
    EmitOperand(dst, Operand(src));
}


void X86Assembler::imull(Register dst, Register src, const Immediate& imm) {
    AssemblerBuffer::EnsureCapacity ensured(&buffer_);
    // See whether imm can be represented as a sign-extended 8bit value.
    int32_t v32 = static_cast<int32_t>(imm.value());
    if (IsInt<8>(v32)) {
        // Sign-extension works.
        EmitUint8(0x6B);
        EmitOperand(dst, Operand(src));
        EmitUint8(static_cast<uint8_t>(v32 & 0xFF));
    } else {
        // Not representable, use full immediate.
        EmitUint8(0x69);
        EmitOperand(dst, Operand(src));
        EmitImmediate(imm);
    }
}


void X86Assembler::imull(Register reg, const Immediate& imm) {
    imull(reg, reg, imm);
}


void X86Assembler::imull(Register reg, const Address& address) {
    AssemblerBuffer::EnsureCapacity ensured(&buffer_);
    EmitUint8(0x0F);
    EmitUint8(0xAF);
    EmitOperand(reg, address);
}


void X86Assembler::imull(Register reg) {
    AssemblerBuffer::EnsureCapacity ensured(&buffer_);
    EmitUint8(0xF7);
    EmitOperand(5, Operand(reg));
}


void X86Assembler::imull(const Address& address) {
    AssemblerBuffer::EnsureCapacity ensured(&buffer_);
    EmitUint8(0xF7);
    EmitOperand(5, address);
}


void X86Assembler::mull(Register reg) {
    AssemblerBuffer::EnsureCapacity ensured(&buffer_);
    EmitUint8(0xF7);
    EmitOperand(4, Operand(reg));
}


void X86Assembler::mull(const Address& address) {
    AssemblerBuffer::EnsureCapacity ensured(&buffer_);
    EmitUint8(0xF7);
    EmitOperand(4, address);
}


void X86Assembler::sbbl(Register dst, Register src) {
    AssemblerBuffer::EnsureCapacity ensured(&buffer_);
    EmitUint8(0x1B);
    EmitOperand(dst, Operand(src));
}


void X86Assembler::sbbl(Register reg, const Immediate& imm) {
    AssemblerBuffer::EnsureCapacity ensured(&buffer_);
    EmitComplex(3, Operand(reg), imm);
}


void X86Assembler::sbbl(Register dst, const Address& address) {
    AssemblerBuffer::EnsureCapacity ensured(&buffer_);
    EmitUint8(0x1B);
    EmitOperand(dst, address);
}


void X86Assembler::sbbl(const Address& address, Register src) {
    AssemblerBuffer::EnsureCapacity ensured(&buffer_);
    EmitUint8(0x19);
    EmitOperand(src, address);
}


void X86Assembler::incl(Register reg) {
    AssemblerBuffer::EnsureCapacity ensured(&buffer_);
    EmitUint8(0x40 + reg);
}


void X86Assembler::incl(const Address& address) {
    AssemblerBuffer::EnsureCapacity ensured(&buffer_);
    EmitUint8(0xFF);
    EmitOperand(0, address);
}


void X86Assembler::decl(Register reg) {
    AssemblerBuffer::EnsureCapacity ensured(&buffer_);
    EmitUint8(0x48 + reg);
}


void X86Assembler::decl(const Address& address) {
    AssemblerBuffer::EnsureCapacity ensured(&buffer_);
    EmitUint8(0xFF);
    EmitOperand(1, address);
}


void X86Assembler::shll(Register reg, const Immediate& imm) {
    EmitGenericShift(4, Operand(reg), imm);
}


void X86Assembler::shll(Register operand, Register shifter) {
    EmitGenericShift(4, Operand(operand), shifter);
}


void X86Assembler::shll(const Address& address, const Immediate& imm) {
    EmitGenericShift(4, address, imm);
}


void X86Assembler::shll(const Address& address, Register shifter) {
    EmitGenericShift(4, address, shifter);
}


void X86Assembler::shrl(Register reg, const Immediate& imm) {
    EmitGenericShift(5, Operand(reg), imm);
}


void X86Assembler::shrl(Register operand, Register shifter) {
    EmitGenericShift(5, Operand(operand), shifter);
}


void X86Assembler::shrl(const Address& address, const Immediate& imm) {
    EmitGenericShift(5, address, imm);
}


void X86Assembler::shrl(const Address& address, Register shifter) {
    EmitGenericShift(5, address, shifter);
}


void X86Assembler::sarl(Register reg, const Immediate& imm) {
    EmitGenericShift(7, Operand(reg), imm);
}


void X86Assembler::sarl(Register operand, Register shifter) {
    EmitGenericShift(7, Operand(operand), shifter);
}


void X86Assembler::sarl(const Address& address, const Immediate& imm) {
    EmitGenericShift(7, address, imm);
}


void X86Assembler::sarl(const Address& address, Register shifter) {
    EmitGenericShift(7, address, shifter);
}


void X86Assembler::shld(Register dst, Register src, Register shifter) {
    DCHECK_EQ(ECX, shifter);
    AssemblerBuffer::EnsureCapacity ensured(&buffer_);
    EmitUint8(0x0F);
    EmitUint8(0xA5);
    EmitRegisterOperand(src, dst);
}


void X86Assembler::shld(Register dst, Register src, const Immediate& imm) {
    AssemblerBuffer::EnsureCapacity ensured(&buffer_);
    EmitUint8(0x0F);
    EmitUint8(0xA4);
    EmitRegisterOperand(src, dst);
    EmitUint8(imm.value() & 0xFF);
}


void X86Assembler::shrd(Register dst, Register src, Register shifter) {
    DCHECK_EQ(ECX, shifter);
    AssemblerBuffer::EnsureCapacity ensured(&buffer_);
    EmitUint8(0x0F);
    EmitUint8(0xAD);
    EmitRegisterOperand(src, dst);
}


void X86Assembler::shrd(Register dst, Register src, const Immediate& imm) {
    AssemblerBuffer::EnsureCapacity ensured(&buffer_);
    EmitUint8(0x0F);
    EmitUint8(0xAC);
    EmitRegisterOperand(src, dst);
    EmitUint8(imm.value() & 0xFF);
}


void X86Assembler::roll(Register reg, const Immediate& imm) {
    EmitGenericShift(0, Operand(reg), imm);
}


void X86Assembler::roll(Register operand, Register shifter) {
    EmitGenericShift(0, Operand(operand), shifter);
}


void X86Assembler::rorl(Register reg, const Immediate& imm) {
    EmitGenericShift(1, Operand(reg), imm);
}


void X86Assembler::rorl(Register operand, Register shifter) {
    EmitGenericShift(1, Operand(operand), shifter);
}


void X86Assembler::negl(Register reg) {
    AssemblerBuffer::EnsureCapacity ensured(&buffer_);
    EmitUint8(0xF7);
    EmitOperand(3, Operand(reg));
}


void X86Assembler::notl(Register reg) {
    AssemblerBuffer::EnsureCapacity ensured(&buffer_);
    EmitUint8(0xF7);
    EmitUint8(0xD0 | reg);
}


void X86Assembler::enter(const Immediate& imm) {
    AssemblerBuffer::EnsureCapacity ensured(&buffer_);
    EmitUint8(0xC8);
    CHECK(imm.is_uint16());
    EmitUint8(imm.value() & 0xFF);
    EmitUint8((imm.value() >> 8) & 0xFF);
    EmitUint8(0x00);
}


void X86Assembler::leave() {
    AssemblerBuffer::EnsureCapacity ensured(&buffer_);
    EmitUint8(0xC9);
}


void X86Assembler::ret() {
    AssemblerBuffer::EnsureCapacity ensured(&buffer_);
    EmitUint8(0xC3);
}


void X86Assembler::ret(const Immediate& imm) {
    AssemblerBuffer::EnsureCapacity ensured(&buffer_);
    EmitUint8(0xC2);
    CHECK(imm.is_uint16());
    EmitUint8(imm.value() & 0xFF);
    EmitUint8((imm.value() >> 8) & 0xFF);
}



void X86Assembler::nop() {
    AssemblerBuffer::EnsureCapacity ensured(&buffer_);
    EmitUint8(0x90);
}


void X86Assembler::int3() {
    AssemblerBuffer::EnsureCapacity ensured(&buffer_);
    EmitUint8(0xCC);
}


void X86Assembler::hlt() {
    AssemblerBuffer::EnsureCapacity ensured(&buffer_);
    EmitUint8(0xF4);
}


void X86Assembler::j(Condition condition, Label* label) {
    AssemblerBuffer::EnsureCapacity ensured(&buffer_);
    if (label->IsBound()) {
        static const int kShortSize = 2;
        static const int kLongSize = 6;
        int offset = label->Position() - buffer_.Size();
        CHECK_LE(offset, 0);
        if (IsInt<8>(offset - kShortSize)) {
            EmitUint8(0x70 + condition);
            EmitUint8((offset - kShortSize) & 0xFF);
        } else {
            EmitUint8(0x0F);
            EmitUint8(0x80 + condition);
            EmitInt32(offset - kLongSize);
        }
    } else {
        EmitUint8(0x0F);
        EmitUint8(0x80 + condition);
        EmitLabelLink(label);
    }
}


void X86Assembler::j(Condition condition, NearLabel* label) {
    AssemblerBuffer::EnsureCapacity ensured(&buffer_);
    if (label->IsBound()) {
        static const int kShortSize = 2;
        int offset = label->Position() - buffer_.Size();
        CHECK_LE(offset, 0);
        CHECK(IsInt<8>(offset - kShortSize));
        EmitUint8(0x70 + condition);
        EmitUint8((offset - kShortSize) & 0xFF);
    } else {
        EmitUint8(0x70 + condition);
        EmitLabelLink(label);
    }
}


void X86Assembler::jecxz(NearLabel* label) {
    AssemblerBuffer::EnsureCapacity ensured(&buffer_);
    if (label->IsBound()) {
        static const int kShortSize = 2;
        int offset = label->Position() - buffer_.Size();
        CHECK_LE(offset, 0);
        CHECK(IsInt<8>(offset - kShortSize));
        EmitUint8(0xE3);
        EmitUint8((offset - kShortSize) & 0xFF);
    } else {
        EmitUint8(0xE3);
        EmitLabelLink(label);
    }
}


void X86Assembler::jmp(Register reg) {
    AssemblerBuffer::EnsureCapacity ensured(&buffer_);
    EmitUint8(0xFF);
    EmitRegisterOperand(4, reg);
}

void X86Assembler::jmp(const Address& address) {
    AssemblerBuffer::EnsureCapacity ensured(&buffer_);
    EmitUint8(0xFF);
    EmitOperand(4, address);
}

void X86Assembler::jmp(Label* label) {
    AssemblerBuffer::EnsureCapacity ensured(&buffer_);
    if (label->IsBound()) {
        static const int kShortSize = 2;
        static const int kLongSize = 5;
        int offset = label->Position() - buffer_.Size();
        CHECK_LE(offset, 0);
        if (IsInt<8>(offset - kShortSize)) {
            EmitUint8(0xEB);
            EmitUint8((offset - kShortSize) & 0xFF);
        } else {
            EmitUint8(0xE9);
            EmitInt32(offset - kLongSize);
        }
    } else {
        EmitUint8(0xE9);
        EmitLabelLink(label);
    }
}


void X86Assembler::jmp(NearLabel* label) {
    AssemblerBuffer::EnsureCapacity ensured(&buffer_);
    if (label->IsBound()) {
        static const int kShortSize = 2;
        int offset = label->Position() - buffer_.Size();
        CHECK_LE(offset, 0);
        CHECK(IsInt<8>(offset - kShortSize));
        EmitUint8(0xEB);
        EmitUint8((offset - kShortSize) & 0xFF);
    } else {
        EmitUint8(0xEB);
        EmitLabelLink(label);
    }
}


void X86Assembler::repne_scasb() {
    AssemblerBuffer::EnsureCapacity ensured(&buffer_);
    EmitUint8(0xF2);
    EmitUint8(0xAE);
}


void X86Assembler::repne_scasw() {
    AssemblerBuffer::EnsureCapacity ensured(&buffer_);
    EmitUint8(0x66);
    EmitUint8(0xF2);
    EmitUint8(0xAF);
}


void X86Assembler::repe_cmpsb() {
    AssemblerBuffer::EnsureCapacity ensured(&buffer_);
    EmitUint8(0xF2);
    EmitUint8(0xA6);
}


void X86Assembler::repe_cmpsw() {
    AssemblerBuffer::EnsureCapacity ensured(&buffer_);
    EmitUint8(0x66);
    EmitUint8(0xF3);
    EmitUint8(0xA7);
}


void X86Assembler::repe_cmpsl() {
    AssemblerBuffer::EnsureCapacity ensured(&buffer_);
    EmitUint8(0xF3);
    EmitUint8(0xA7);
}


void X86Assembler::rep_movsb() {
    AssemblerBuffer::EnsureCapacity ensured(&buffer_);
    EmitUint8(0xF3);
    EmitUint8(0xA4);
}


void X86Assembler::rep_movsw() {
    AssemblerBuffer::EnsureCapacity ensured(&buffer_);
    EmitUint8(0x66);
    EmitUint8(0xF3);
    EmitUint8(0xA5);
}


X86Assembler* X86Assembler::lock() {
    AssemblerBuffer::EnsureCapacity ensured(&buffer_);
    EmitUint8(0xF0);
    return this;
}


void X86Assembler::cmpxchgl(const Address& address, Register reg) {
    AssemblerBuffer::EnsureCapacity ensured(&buffer_);
    EmitUint8(0x0F);
    EmitUint8(0xB1);
    EmitOperand(reg, address);
}


void X86Assembler::cmpxchg8b(const Address& address) {
    AssemblerBuffer::EnsureCapacity ensured(&buffer_);
    EmitUint8(0x0F);
    EmitUint8(0xC7);
    EmitOperand(1, address);
}


void X86Assembler::mfence() {
    AssemblerBuffer::EnsureCapacity ensured(&buffer_);
    EmitUint8(0x0F);
    EmitUint8(0xAE);
    EmitUint8(0xF0);
}

X86Assembler* X86Assembler::fs() {
    // TODO: fs is a prefix and not an instruction
    AssemblerBuffer::EnsureCapacity ensured(&buffer_);
    EmitUint8(0x64);
    return this;
}

X86Assembler* X86Assembler::gs() {
    // TODO: fs is a prefix and not an instruction
    AssemblerBuffer::EnsureCapacity ensured(&buffer_);
    EmitUint8(0x65);
    return this;
}

void X86Assembler::AddImmediate(Register reg, const Immediate& imm) {
    int value = imm.value();
    if (value > 0) {
        if (value == 1) {
            incl(reg);
        } else if (value != 0) {
            addl(reg, imm);
        }
    } else if (value < 0) {
        value = -value;
        if (value == 1) {
            decl(reg);
        } else if (value != 0) {
            subl(reg, Immediate(value));
        }
    }
}


void X86Assembler::LoadLongConstant(XmmRegister dst, int64_t value) {
    // TODO: Need to have a code constants table.
    pushl(Immediate(High32Bits(value)));
    pushl(Immediate(Low32Bits(value)));
    movsd(dst, Address(ESP, 0));
    addl(ESP, Immediate(2 * sizeof(int32_t)));
}


void X86Assembler::LoadDoubleConstant(XmmRegister dst, double value) {
    // TODO: Need to have a code constants table.
    int64_t constant = bit_cast<int64_t, double>(value);
    LoadLongConstant(dst, constant);
}


void X86Assembler::Align(int alignment, int offset) {
    CHECK(IsPowerOfTwo(alignment));
    // Emit nop instruction until the real position is aligned.
    while (((offset + buffer_.GetPosition()) & (alignment-1)) != 0) {
        nop();
    }
}


void X86Assembler::Bind(Label* label) {
    int bound = buffer_.Size();
    CHECK(!label->IsBound());  // Labels can only be bound once.
    while (label->IsLinked()) {
        int position = label->LinkPosition();
        int next = buffer_.Load<int32_t>(position);
        buffer_.Store<int32_t>(position, bound - (position + 4));
        label->position_ = next;
    }
    label->BindTo(bound);
}


void X86Assembler::Bind(NearLabel* label) {
    int bound = buffer_.Size();
    CHECK(!label->IsBound());  // Labels can only be bound once.
    while (label->IsLinked()) {
        int position = label->LinkPosition();
        uint8_t delta = buffer_.Load<uint8_t>(position);
        int offset = bound - (position + 1);
        CHECK(IsInt<8>(offset));
        buffer_.Store<int8_t>(position, offset);
        label->position_ = delta != 0u ? label->position_ - delta : 0;
    }
    label->BindTo(bound);
}


void X86Assembler::EmitOperand(int reg_or_opcode, const Operand& operand) {
    CHECK_GE(reg_or_opcode, 0);
    CHECK_LT(reg_or_opcode, 8);
    const int length = operand.length_;
    CHECK_GT(length, 0);
    // Emit the ModRM byte updated with the given reg value.
    CHECK_EQ(operand.encoding_[0] & 0x38, 0);
    EmitUint8(operand.encoding_[0] + (reg_or_opcode << 3));
    // Emit the rest of the encoded operand.
    for (int i = 1; i < length; i++) {
        EmitUint8(operand.encoding_[i]);
    }
    AssemblerFixup* fixup = operand.GetFixup();
    if (fixup != nullptr) {
        EmitFixup(fixup);
    }
}


void X86Assembler::EmitImmediate(const Immediate& imm, bool is_16_op) {
    if (is_16_op) {
        EmitUint8(imm.value() & 0xFF);
        EmitUint8(imm.value() >> 8);
    } else {
        EmitInt32(imm.value());
    }
}


void X86Assembler::EmitComplex(int reg_or_opcode,
                               const Operand& operand,
                               const Immediate& immediate,
                               bool is_16_op) {
    CHECK_GE(reg_or_opcode, 0);
    CHECK_LT(reg_or_opcode, 8);
    if (immediate.is_int8()) {
        // Use sign-extended 8-bit immediate.
        EmitUint8(0x83);
        EmitOperand(reg_or_opcode, operand);
        EmitUint8(immediate.value() & 0xFF);
    } else if (operand.IsRegister(EAX)) {
        // Use short form if the destination is eax.
        EmitUint8(0x05 + (reg_or_opcode << 3));
        EmitImmediate(immediate, is_16_op);
    } else {
        EmitUint8(0x81);
        EmitOperand(reg_or_opcode, operand);
        EmitImmediate(immediate, is_16_op);
    }
}


void X86Assembler::EmitLabel(Label* label, int instruction_size) {
    if (label->IsBound()) {
        int offset = label->Position() - buffer_.Size();
        CHECK_LE(offset, 0);
        EmitInt32(offset - instruction_size);
    } else {
        EmitLabelLink(label);
    }
}


void X86Assembler::EmitLabelLink(Label* label) {
    CHECK(!label->IsBound());
    int position = buffer_.Size();
    EmitInt32(label->position_);
    label->LinkTo(position);
}


void X86Assembler::EmitLabelLink(NearLabel* label) {
    CHECK(!label->IsBound());
    int position = buffer_.Size();
    if (label->IsLinked()) {
        // Save the delta in the byte that we have to play with.
        uint32_t delta = position - label->LinkPosition();
        CHECK(IsUint<8>(delta));
        EmitUint8(delta & 0xFF);
    } else {
        EmitUint8(0);
    }
    label->LinkTo(position);
}


void X86Assembler::EmitGenericShift(int reg_or_opcode,
                                    const Operand& operand,
                                    const Immediate& imm) {
    AssemblerBuffer::EnsureCapacity ensured(&buffer_);
    CHECK(imm.is_int8());
    if (imm.value() == 1) {
        EmitUint8(0xD1);
        EmitOperand(reg_or_opcode, operand);
    } else {
        EmitUint8(0xC1);
        EmitOperand(reg_or_opcode, operand);
        EmitUint8(imm.value() & 0xFF);
    }
}


void X86Assembler::EmitGenericShift(int reg_or_opcode,
                                    const Operand& operand,
                                    Register shifter) {
    AssemblerBuffer::EnsureCapacity ensured(&buffer_);
    CHECK_EQ(shifter, ECX);
    EmitUint8(0xD3);
    EmitOperand(reg_or_opcode, operand);
}

void X86Assembler::AddConstantArea() {
    ArrayRef<const int32_t> area = constant_area_.GetBuffer();
    // Generate the data for the literal area.
    for (size_t i = 0, e = area.size(); i < e; i++) {
        AssemblerBuffer::EnsureCapacity ensured(&buffer_);
        EmitInt32(area[i]);
    }
}

size_t ConstantArea::AppendInt32(int32_t v) {
    size_t result = buffer_.size() * elem_size_;
    buffer_.push_back(v);
    return result;
}

size_t ConstantArea::AddInt32(int32_t v) {
    for (size_t i = 0, e = buffer_.size(); i < e; i++) {
        if (v == buffer_[i]) {
            return i * elem_size_;
        }
    }

    // Didn't match anything.
    return AppendInt32(v);
}

size_t ConstantArea::AddInt64(int64_t v) {
    int32_t v_low = Low32Bits(v);
    int32_t v_high = High32Bits(v);
    if (buffer_.size() > 1) {
        // Ensure we don't pass the end of the buffer.
        for (size_t i = 0, e = buffer_.size() - 1; i < e; i++) {
            if (v_low == buffer_[i] && v_high == buffer_[i + 1]) {
                return i * elem_size_;
            }
        }
    }

    // Didn't match anything.
    size_t result = buffer_.size() * elem_size_;
    buffer_.push_back(v_low);
    buffer_.push_back(v_high);
    return result;
}

size_t ConstantArea::AddDouble(double v) {
    // Treat the value as a 64-bit integer value.
    return AddInt64(bit_cast<int64_t, double>(v));
}

size_t ConstantArea::AddFloat(float v) {
    // Treat the value as a 32-bit integer value.
    return AddInt32(bit_cast<int32_t, float>(v));
}

}  // namespace x86
}  // namespace whale

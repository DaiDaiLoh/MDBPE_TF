#include "x64.hh"

#include <babel-serializer/detail/log.hh>

#include <clean-core/flags.hh>
#include <clean-core/format.hh>

#include <rich-log/log.hh>

// explanations:
// - https://www-user.tu-chemnitz.de/~heha/hsn/chm/x86.chm/x64.htm
// - https://www.systutorials.com/beginners-guide-x86-64-instruction-encoding/
// - https://pyokagan.name/blog/2019-09-20-x86encoding/

// references:
// - https://wiki.osdev.org/X86-64_Instruction_Encoding
//   (contains register mapping at the beginning)
//   (contains parts of the instruction)
// - http://ref.x86asm.net/geek64.html
//   (field description in http://ref.x86asm.net/#column_flds )
// - https://www.intel.com/content/www/us/en/developer/articles/technical/intel-sdm.html (waaay too long)
// - https://www.felixcloutier.com/x86/index.html

#define ENABLE_VERBOSE_DECODE_LOG 1

#if ENABLE_VERBOSE_DECODE_LOG
#define VERBOSE_DECODE_LOG(...) LOG(__VA_ARGS__)
#else
#define VERBOSE_DECODE_LOG(...) CC_FORCE_SEMICOLON
#endif

namespace babel::x64
{
namespace
{
constexpr reg64 reg64_from_op(uint32_t op, bool extended) { return reg64((op & 0b111) + (extended ? 8 : 0)); }
constexpr reg32 reg32_from_op(uint32_t op) { return reg32((op & 0b111)); }

//
// REX
//

// see https://wiki.osdev.org/X86-64_Instruction_Encoding#Encoding
constexpr bool is_rex(std::byte b)
{
    auto v = uint8_t(b);
    return (v & 0b11110000) == 0b01000000;
}
// precondition: is rex
// aka is REX.B set
constexpr bool is_rex_b(uint8_t v) { return v & 0b0001; }
// precondition: is rex
// aka is REX.X set
constexpr bool is_rex_x(uint8_t v) { return v & 0b0010; }
// precondition: is rex
// aka is REX.R set
constexpr bool is_rex_r(uint8_t v) { return v & 0b0100; }
// precondition: is rex
// aka is REX.W set
constexpr bool is_rex_w(uint8_t v) { return v & 0b1000; }

//
// ModR/M
//

enum class modrm_mode : uint8_t
{
    register_indirect = 0b00,
    memory_disp8 = 0b01,
    memory_disp32_64 = 0b10,
    register_direct = 0b11,
};
modrm_mode modrm_mode_of(std::byte b) { return modrm_mode(uint8_t(b) >> 6); }
uint8_t modrm_reg_of(std::byte b) { return (uint8_t(b) >> 3) & 0b111; }
uint8_t modrm_rm_of(std::byte b) { return uint8_t(b) & 0b111; }

//
// ModR/M
//

uint8_t sib_scale_of(std::byte b) { return 1 << (uint8_t(b) >> 6); }
uint8_t sib_index_of(std::byte b, uint8_t rex) { return ((uint8_t(b) >> 3) & 0b111) + (is_rex_x(rex) ? 8 : 0); }
uint8_t sib_base_of(std::byte b, uint8_t rex) { return (uint8_t(b) & 0b111) + (is_rex_b(rex) ? 8 : 0); }

//
// Formatting
//

[[maybe_unused]] void add_opreg_to_string(cc::string& s, babel::x64::instruction const& in)
{
    if (is_rex_w(in.rex))
    {
        LOG_WARN("TODO: which rex byte is used here?");
        CC_UNREACHABLE("TODO");
        // s += x64::to_string(reg64_from_op(opcode, is_rex_b(rex)));
    }
    else
        s += x64::to_string(reg32_from_op(in.opcode));
}

[[maybe_unused]] void add_opreg64_to_string(cc::string& s, babel::x64::instruction const& in)
{
    if (is_rex_w(in.rex))
    {
        LOG_WARN("TODO: which rex byte is used here?");
        CC_UNREACHABLE("TODO");
        // s += x64::to_string(reg64_from_op(opcode, is_rex_b(rex)));
    }
    else
        s += x64::to_string(reg64_from_op(in.opcode, false));
}

[[maybe_unused]] void add_modr_to_string(cc::string& s, babel::x64::instruction const& in)
{
    auto const modrm = in.data[in.offset_modrm];
    // auto const mode = modrm_mode_of(modrm);
    // CC_ASSERT(mode == modrm_mode::register_direct && "TODO"); -- is always register, right?

    auto regi = modrm_reg_of(modrm);

    // 64bit
    if (is_rex_w(in.rex))
    {
        if (is_rex_r(in.rex))
            regi += 8;

        s += x64::to_string(reg64(regi));
    }
    else // 32bit
    {
        s += x64::to_string(reg32(regi));
    }
}

[[maybe_unused]] void add_disp8_to_string(cc::string& s, std::byte d)
{
    auto v = int8_t(d);
    if (v >= 0)
    {
        s += " + 0x";
        s += cc::to_string(d);
    }
    else
    {
        s += " - 0x";
        s += cc::to_string(std::byte(-v));
    }
}

[[maybe_unused]] void add_modm_to_string(cc::string& s, babel::x64::instruction const& in)
{
    auto const modrm = in.data[in.offset_modrm];
    auto const mode = modrm_mode_of(modrm);

    if (mode != modrm_mode::register_direct)
        s += '[';

    auto regi = modrm_rm_of(modrm);
    if (is_rex_b(in.rex))
        regi += 8;

    auto skip_plus = false;

    // SIB
    auto has_special_sip_disp32 = false;
    if (mode != modrm_mode::register_direct && regi == 0b100)
    {
        CC_ASSERT(in.offset_modrm > 0 && "no mod/rm sib");
        auto const sib = in.data[in.offset_modrm + 1];
        auto const scale = sib_scale_of(sib);
        auto const index = sib_index_of(sib, in.rex);
        auto const base = sib_base_of(sib, in.rex);

        auto const no_base_reg = mode == modrm_mode::register_indirect && (base & 0b111) == 0b101;
        auto const no_index_reg = mode != modrm_mode::register_indirect && index == 0b0100;

        if (no_base_reg)
            has_special_sip_disp32 = true;

        if (no_index_reg && no_base_reg)
        {
            // only disp
            skip_plus = true;
        }
        else if (no_index_reg)
        {
            s += x64::to_string(reg64(base));
        }
        else if (no_base_reg)
        {
            s += x64::to_string(reg64(index));
            s += " * ";
            s += cc::to_string(scale);
        }
        else
        {
            s += x64::to_string(reg64(base));
            s += " + ";
            s += x64::to_string(reg64(index));
            s += " * ";
            s += cc::to_string(scale);
        }
    }
    else if (mode == modrm_mode::register_indirect && regi == 0b101)
    {
        CC_UNREACHABLE("TODO: impl RIP+disp");
    }
    // r/m register
    else
    {
        if (mode != modrm_mode::register_direct || is_rex_w(in.rex))
            s += x64::to_string(reg64(regi));
        else
            s += x64::to_string(reg32(regi));
    }

    // displacement
    if (mode == modrm_mode::memory_disp8)
    {
        CC_ASSERT(in.offset_displacement > 0 && "instruction has no disp set");
        add_disp8_to_string(s, in.data[in.offset_displacement]);
    }
    else if (mode == modrm_mode::memory_disp32_64 || has_special_sip_disp32)
    {
        CC_ASSERT(in.offset_displacement > 0 && "instruction has no disp set");
        if (!skip_plus)
            s += " + ";
        s += "0x";
        for (auto i = 3; i >= 0; --i)
            s += cc::to_string(in.data[in.offset_displacement + i]);
    }

    if (mode != modrm_mode::register_direct)
        s += ']';
}

[[maybe_unused]] void add_imm8_to_string(cc::string& s, babel::x64::instruction const& in)
{
    CC_ASSERT(in.offset_immediate > 0 && "no immediate available");
    s += "0x";
    s += cc::to_string(in.data[in.offset_immediate]);
}

[[maybe_unused]] void add_imm16_to_string(cc::string& s, babel::x64::instruction const& in)
{
    CC_ASSERT(in.offset_immediate > 0 && "no immediate available");
    s += "0x";
    for (auto i = 1; i >= 0; --i)
        s += cc::to_string(in.data[in.offset_immediate + i]);
}

[[maybe_unused]] void add_imm32_to_string(cc::string& s, babel::x64::instruction const& in)
{
    CC_ASSERT(in.offset_immediate > 0 && "no immediate available");
    s += "0x";
    for (auto i = 3; i >= 0; --i)
        s += cc::to_string(in.data[in.offset_immediate + i]);
}

} // namespace
} // namespace babel::x64

babel::x64::instruction babel::x64::decode_one(std::byte const* data, std::byte const* end)
{
    if (data >= end)
        return {};

    VERBOSE_DECODE_LOG("decoding %s ...", cc::span<std::byte const>(data, cc::min(data + 16, end)));

    instruction instr;
    mnemonic mnem = mnemonic::_invalid;
    std::byte rex = std::byte(0);
    instr.data = data;
    auto op = *data++;

    // parse prefixes
    auto pref_0F = false; // two-byte opcodes
    auto pref_66 = false; // size prefix | op sel
    [[maybe_unused]] auto pref_67 = false; // addr size prefix
    auto pref_F2 = false; // rep
    auto pref_F3 = false; // rep
    auto pref_F0 = false; // lock

    auto check_for_prefixes = true;
    while (check_for_prefixes)
    {
        switch (uint8_t(op))
        {
        case 0x0F:
            pref_0F = true;
            VERBOSE_DECODE_LOG("  got prefix 0F");
            break;
        case 0x66:
            pref_66 = true;
            VERBOSE_DECODE_LOG("  got prefix 66");
            break;
        case 0x67:
            pref_67 = true;
            VERBOSE_DECODE_LOG("  got prefix 67");
            break;
        case 0xF2:
            pref_F2 = true;
            VERBOSE_DECODE_LOG("  got prefix F2");
            break;
        case 0xF3:
            pref_F3 = true;
            VERBOSE_DECODE_LOG("  got prefix F3");
            break;
        case 0xF0:
            pref_F0 = true;
            VERBOSE_DECODE_LOG("  got prefix F0");
            break;

        case 0x9B:
            LOG_WARN("TODO: support x87 instruction for byte 0x%s (in %s)", op, cc::span<std::byte const>(instr.data, cc::min(instr.data + 16, end)));
            return {};

        default:
            if (is_rex(op))
            {
                rex = op;
                VERBOSE_DECODE_LOG("  got prefix %s (REX)", op);
            }
            else
            {
                // no prefix anymore
                check_for_prefixes = false;

                // no next data
                continue;
            }
        }

        // next
        if (data >= end)
            return {};

        op = *data++;
    }
    instr.offset_op = data - instr.data;

    VERBOSE_DECODE_LOG("  primary op is %s", op);

    // first round of decode
    // two tables can be directly addressed
    //   one-byte ops from 0x000..0x0FF
    //   two-byte ops from 0x100..0x1FF
    auto has_modrm = false;
    auto need_phase2 = false;
    uint16_t decode_entry;
    instr.opcode = uint16_t(op) + (pref_0F ? 0x0F00 : 0);
    {
        auto dec_idx = int(op);
        if (pref_0F)
            dec_idx += 0x100;

        VERBOSE_DECODE_LOG("  looking up idx %s", dec_idx);

        decode_entry = detail::decode_table[dec_idx];
        mnem = detail::entry_mnemonic(decode_entry);
        has_modrm = detail::entry_has_modrm(decode_entry);
        need_phase2 = detail::entry_is_phase2(decode_entry);

        if (need_phase2)
        {
            VERBOSE_DECODE_LOG("    - need phase2");
            VERBOSE_DECODE_LOG("    - is extended op: %s", has_modrm);
            VERBOSE_DECODE_LOG("    - need to add secop: %s", detail::entry_phase2_add_secondary(decode_entry));
            VERBOSE_DECODE_LOG("    - phase2 offset: %s", detail::entry_phase2_get_offset(mnem));
        }
        else
        {
            VERBOSE_DECODE_LOG("    - mnemonic %s", mnem);
            VERBOSE_DECODE_LOG("    - has_modrm: %s", has_modrm);
            VERBOSE_DECODE_LOG("    - imm_size: %s", detail::entry_get_immsize(decode_entry));
        }
    }

    // unknown op?
    if (mnem == mnemonic::_invalid)
    {
        LOG_WARN("unknown instruction for byte 0x%s (in %s)", op, cc::span<std::byte const>(instr.data, cc::min(instr.data + 16, end)));
        return {};
    }

    // phase 2 lookup (for ops that have different mnemonic/args with prefixes)
    // NOTE: subidx computation must match in generator
    //   - see MakeTableEntryPhase2
    //   - see computed_subidx
    auto subidx = 0;
    if (need_phase2)
    {
        subidx = detail::entry_phase2_get_offset(mnem);
        /*
            if (is_extended)
                computed_subidx += int.Parse(extended_op!);
            if (has_secondary_op)
                computed_subidx += int.Parse(secondary_op!, System.Globalization.NumberStyles.HexNumber) ^ 0xE1;
            if (prefix == "F3")
                computed_subidx += 2;
            if (prefix == "F2")
                computed_subidx += 1;
            if (prefix == "66")
                computed_subidx += 8;
         */
        if (detail::entry_phase2_add_secondary(decode_entry))
        {
            if (data >= end)
                return {};

            subidx += uint8_t(*data) ^ 0xE1;
        }
        if (pref_F3)
            subidx += 2;
        if (pref_F2)
            subidx += 1;
        if (pref_66)
            subidx += 8;

        // if no ModR/M was signaled, we can immediately look up phase2
        if (!has_modrm)
        {
            VERBOSE_DECODE_LOG("  immediate phase2: looking up idx %s", subidx);

            decode_entry = detail::decode_table[subidx];
            CC_ASSERT(!detail::entry_is_phase2(decode_entry));
            mnem = detail::entry_mnemonic(decode_entry);
            CC_ASSERT(!detail::entry_has_modrm(decode_entry));
            need_phase2 = false;

            VERBOSE_DECODE_LOG("    - mnemonic %s", mnem);
            VERBOSE_DECODE_LOG("    - has_modrm: %s", has_modrm);
            VERBOSE_DECODE_LOG("    - imm_size: %s", detail::entry_get_immsize(decode_entry));
        }
    }

    // subresolve:
    // - some op codes like 0x80 (add/or/...) have 8 "subcodes" in their ModR/M byte
    // - those are marked as subresolve in the 1st phase lookup

    // ModR/M
    if (has_modrm)
    {
        if (data >= end)
            return {};

        instr.offset_modrm = data - instr.data;
        auto modrm = *data++;
        auto mod = modrm_mode_of(modrm);

        // delayed phase2 lookup
        if (need_phase2)
        {
            // add ext_op idx
            subidx += modrm_reg_of(modrm);

            decode_entry = detail::decode_table[subidx];
            CC_ASSERT(!detail::entry_is_phase2(decode_entry));
            mnem = detail::entry_mnemonic(decode_entry);
            CC_ASSERT(!detail::entry_has_modrm(decode_entry));
        }
        if (mnem == mnemonic::_invalid)
        {
            LOG_WARN("unknown instruction for byte 0x%s (in %s)", op, cc::span<std::byte const>(instr.data, cc::min(instr.data + 16, end)));
            return {};
        }

        // see https://wiki.osdev.org/X86-64_Instruction_Encoding#32.2F64-bit_addressing

        // SIB
        auto has_special_sip_disp32 = false;
        if (mod != modrm_mode::register_direct && modrm_rm_of(modrm) == 0b100)
        {
            if (data >= end)
                return {};

            auto const sib = *data++;

            if (mod == modrm_mode::register_indirect && sib_base_of(sib, 0) == 0b101)
                has_special_sip_disp32 = true;
        }

        // disp8
        if (mod == modrm_mode::memory_disp8)
        {
            if (data >= end)
                return {};

            instr.offset_displacement = data - instr.data;
            instr.size_displacement = 0;
            data++;
        }
        // disp32
        else if (mod == modrm_mode::memory_disp32_64 || has_special_sip_disp32)
        {
            if (data + 3 >= end)
                return {};

            // TODO: is there 64 bit displacement?
            instr.offset_displacement = data - instr.data;
            instr.size_displacement = 2;
            data += 4;
        }

        // special
        CC_ASSERT(!(mod == modrm_mode::register_indirect && modrm_rm_of(modrm) == 0b101) && "TODO: implement RIP/EIP+disp32");
    }

    // immediate args
    auto imm_size = detail::entry_get_immsize(decode_entry);
    CC_ASSERT(0 <= imm_size && imm_size <= 4);
    if (imm_size > 0)
    {
        auto imm_size_log2 = imm_size - 1;
        auto real_size = 1 << imm_size_log2;
        if (data + real_size > end)
            return {};
        instr.offset_immediate = data - instr.data;
        instr.size_immediate = imm_size_log2;
        data += real_size;
    }

    // finalize
    instr.is_lock = uint8_t(pref_F0);
    instr.mnemonic_packed = uint16_t(mnem);
    instr.rex = uint8_t(rex) & 0b1111;
    instr.size = data - instr.data;
    return instr;
}

cc::string babel::x64::instruction::to_string() const
{
    if (!is_valid())
        return "<invalid instruction>";

    cc::string s;
    if (is_lock)
        s += "lock ";
    s += x64::to_string(mnemonic());

    /*switch (arg_format())
    {
        // 0 args
    case arg_format::none:
        break;

        // 1 arg
    case arg_format::opreg:
        s += ' '; // TODO: pad?
        add_opreg_to_string(s, *this);
        break;
    case arg_format::opreg64:
        s += ' '; // TODO: pad?
        add_opreg64_to_string(s, *this);
        break;
    case arg_format::imm8:
        s += ' '; // TODO: pad?
        add_imm8_to_string(s, *this);
        break;
    case arg_format::imm16:
        s += ' '; // TODO: pad?
        add_imm16_to_string(s, *this);
        break;
    case arg_format::imm32:
        s += ' '; // TODO: pad?
        add_imm32_to_string(s, *this);
        break;
    case arg_format::imm32_64:
        // TODO: 64?
        s += ' '; // TODO: pad?
        add_imm32_to_string(s, *this);
        break;
    case arg_format::modm:
        s += ' '; // TODO: pad?
        add_modm_to_string(s, *this);
        break;

        // 2 args
    case arg_format::modm_modr:
        s += ' '; // TODO: pad?
        add_modm_to_string(s, *this);
        s += ", ";
        add_modr_to_string(s, *this);
        break;
    case arg_format::modr_modm:
        s += ' '; // TODO: pad?
        add_modr_to_string(s, *this);
        s += ", ";
        add_modm_to_string(s, *this);
        break;
    case arg_format::opreg_imm:
        CC_UNREACHABLE("TODO");
        break;
    case arg_format::modm_imm8:
        s += ' '; // TODO: pad?
        add_modm_to_string(s, *this);
        s += ", ";
        add_imm8_to_string(s, *this);
        break;
    case arg_format::modm_imm32:
        s += ' '; // TODO: pad?
        add_modm_to_string(s, *this);
        s += ", ";
        add_imm32_to_string(s, *this);
        break;
    case arg_format::modm_imm32_64:
        // TODO: 64?
        s += ' '; // TODO: pad?
        add_modm_to_string(s, *this);
        s += ", ";
        add_imm32_to_string(s, *this);
        break;

        // 3 args
    case arg_format::modm_modr_imm8:
        s += ' '; // TODO: pad?
        add_modm_to_string(s, *this);
        s += ", ";
        add_modr_to_string(s, *this);
        s += ", ";
        add_imm8_to_string(s, *this);
        break;
    case arg_format::modm_modr_imm32:
        s += ' '; // TODO: pad?
        add_modm_to_string(s, *this);
        s += ", ";
        add_modr_to_string(s, *this);
        s += ", ";
        add_imm32_to_string(s, *this);
        break;
    case arg_format::modr_modm_imm8:
        s += ' '; // TODO: pad?
        add_modr_to_string(s, *this);
        s += ", ";
        add_modm_to_string(s, *this);
        s += ", ";
        add_imm8_to_string(s, *this);
        break;
    }*/

    return s;
}

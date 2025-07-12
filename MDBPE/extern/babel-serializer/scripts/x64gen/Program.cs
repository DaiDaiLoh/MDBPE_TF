
using System.Net.Http;
using System.Xml.Linq;

var xml_local_path = "x64asm-ref.xml";
var xml_url = "http://ref.x86asm.net/x86reference.xml";
var gen_file_cc = "../../src/babel-serializer/assembly/x64.gen.cc";
var gen_file_hh = "../../src/babel-serializer/assembly/x64.gen.hh";

//
// fetch reference
//
if (!File.Exists(xml_local_path))
{
    System.Console.WriteLine("Local x64 reference does not exist, downloading it ...");
    using (var client = new HttpClient())
    {
        var response = await client.GetAsync(xml_url);

        if (!response.IsSuccessStatusCode)
        {
            System.Console.WriteLine("Error while downloading");
            return;
        }

        using (var content = response.Content)
        {
            var data = await content.ReadAsStringAsync();
            File.WriteAllText(xml_local_path, data);
            System.Console.WriteLine($"Written ref to {xml_url}");
        }
    }
}
if (!File.Exists(xml_local_path))
{
    System.Console.WriteLine("Still no ref data...");
    return;
}
var refdoc = XDocument.Load(xml_local_path);

//
// parse reference
//
var mnemonics_set = new HashSet<string>();
string NormalizeMnemonic(string s)
{
    s = s.Trim().ToLower();
    // some rewrites
    switch (s)
    {
        case "shl": s = "sal"; break;
        case "jnae": s = "jb"; break;
        case "jc": s = "jb"; break;
        case "jnb": s = "jae"; break;
        case "jnc": s = "jae"; break;
        case "jz": s = "je"; break;
        case "jnz": s = "jne"; break;
        case "jna": s = "jbe"; break;
        case "jnbe": s = "ja"; break;
        case "jp": s = "jpe"; break;
        case "jnp": s = "jpo"; break;
        case "jnge": s = "jl"; break;
        case "jnl": s = "jge"; break;
        case "jng": s = "jle"; break;
        case "jnle": s = "jg"; break;
    }
    return s;
}
string SafeMnemonic(string s)
{
    switch (s)
    {
        case "and": return "and_";
        case "or": return "or_";
        case "xor": return "xor_";
        case "int": return "int_";
        case "not": return "not_";
        default:
            return s;
    }
}
string AddrOfOp(XElement? e)
{
    // "immediate 1" -> aka no real arg
    if (e?.Attribute("address") != null && e?.Value == "1")
        return "";

    return e?.Element("a")?.Value ?? e?.Attribute("address")?.Value ?? e?.Value ?? "";
}
string TypeOfOp(XElement? e) => e?.Element("t")?.Value ?? e?.Attribute("type")?.Value ?? "";
var entries = new List<OpEntry>();
var entry_by_code = new Dictionary<int, OpEntry>();
var has_errors = false;
var todos = new List<string>();
void AddEntry(OpEntry e)
{
    if (entry_by_code.ContainsKey(e.full_opcode))
    {
        has_errors = true;
        var ee = entry_by_code[e.full_opcode];
        System.Console.WriteLine($"ERROR: duplicate full op for {e.full_opcode:X} {e.primary_opcode:X} {e.mnemonic} {e.category} | collides with {ee.primary_opcode:X} {ee.mnemonic} {ee.category}");
        return;
    }

    entries.Add(e);
    entry_by_code.Add(e.full_opcode, e);
}
void ProcessEntry(XElement entry, XElement syntax, string mnemonic)
{
    if (entry.Parent!.Name != "pri_opcd")
        throw new InvalidOperationException("expected pri_opcd as parent");

    // any mnemonic seens is recorded
    mnemonics_set.Add(mnemonic);

    var pri_opcd = entry.Parent.Attribute("value")!.Value;
    var pri_opcode = int.Parse(pri_opcd, System.Globalization.NumberStyles.HexNumber);
    var prefix = entry.Element("pref")?.Value;

    // TODO: print hints
    // SC/AL/... are fixed implicit args and not encoded
    bool isEmptyArg(string s) => s == "" || s == "SC" || s == "CL" || s == "ST" || s == "ST1" || s == "ST2" || s == "ST3" || s == "ST4" || s == "ST5" || s == "ST6" || s == "F" || s == "DX" || s == "DI" || s == "EDI" || s == "AX" || s == "EAX" || s == "XMM0" || s == "EDX" || s == "BX" || s == "SP" || s == "ESP" || s == "CX" || s == "ECX" || s == "EBX" || s == "BP" || s == "EBP" || s == "AL" || s == "eAX" || s == "rAX" || s == "rCX" || s == "rBP" || s == "X" || s == "Y";
    bool isOpReg(string s) => s == "Z";
    // O is some kind of offset, treated as immediate for decoding
    bool isImmediate(string s) => s == "J" || s == "I" || s == "A" || s == "O";
    bool isModR(string s) => s == "G" || s == "V" || s == "P" || s == "C" || s == "T" || s == "D" || s == "S";
    bool isModM(string s) => s == "E" || s == "M" || s == "EST" || s == "ES" || s == "W" || s == "H" || s == "Q" || s == "M" || s == "U" || s == "N" || s == "R";
    string ImmTypeOf(string s, string t)
    {
        switch (t)
        {
            case "b":
            case "bs": // sign extended
            case "bss": // sign extended to stack ptr size
                return "8";

            // "The most important part is ds code, which means doubleword, sign-extended to 64 bits for 64-bit operand size."
            // TODO: is 16 bit with 0x66 size prefix
            case "vds":
                return "32";
            case "vs":
                return "32";

            // promoted by REX.W
            case "vqp":
                return "32_64";

            case "w":
                return "16";
        }
        System.Console.WriteLine($"unknown immediate type '{t}' for {mnemonic} {pri_opcd} {syntax}");
        return "";
    }

    // FIXME: 
    var syn_parts = syntax.Elements().ToList();
    if (syn_parts.Count == 0)
        throw new InvalidOperationException("empty syntax");
    if (syn_parts[0].Name != "mnem")
        throw new InvalidOperationException("mnem must be first");

    syn_parts = syn_parts.Skip(1).Where(p => !isEmptyArg(AddrOfOp(p)) && (p.Attribute("displayed")?.Value ?? "") != "no").ToList();

    var op1_e = syn_parts.Count >= 1 ? syn_parts[0] : null;
    var op2_e = syn_parts.Count >= 2 ? syn_parts[1] : null;
    var op3_e = syn_parts.Count >= 3 ? syn_parts[2] : null;
    var op1 = AddrOfOp(op1_e);
    var op2 = AddrOfOp(op2_e);
    var op3 = AddrOfOp(op3_e);
    var op1_t = TypeOfOp(op1_e);
    var op2_t = TypeOfOp(op2_e);
    var op3_t = TypeOfOp(op3_e);

    if (syn_parts.Count >= 4)
    {
        System.Console.WriteLine($"ERROR: USE ARG for {pri_opcd} {syn_parts[3]}");
    }

    var op_cat = entry.Parent!.Parent!.Name;
    var is_one_byte = op_cat == "one-byte";
    var is_two_byte = op_cat == "two-byte"; // 0x0F
    var cat = is_one_byte ? "one" :
              is_two_byte ? "two" :
              throw new InvalidOperationException();
    var has_op1 = !isEmptyArg(op1);
    var has_op2 = !isEmptyArg(op2);
    var has_op3 = !isEmptyArg(op3);

    if (has_op3 && !isImmediate(op3))
    {
        System.Console.WriteLine($"ERROR: TODO 3rd ARG assumed imm for {pri_opcd} {syn_parts[2]}");
    }

    var extended_op = entry.Element("opcd_ext")?.Value;
    var is_extended = !string.IsNullOrEmpty(extended_op);
    var secondary_op = entry.Element("sec_opcd")?.Value;
    var has_secondary_op = !string.IsNullOrEmpty(secondary_op);

    //
    // corner cases
    //

    // int3 is weirdly defined in the doc
    if (is_one_byte && pri_opcd == "CC")
    {
        has_op2 = false;
        has_op1 = false;
    }
    // disambiguate 0x90 to xchg -> so it generates 91..97
    if (is_one_byte && pri_opcd == "90" && prefix == null && mnemonic != "xchg")
        return;
    // disambiguate 0xf3 0x90 to pause
    if (is_one_byte && pri_opcd == "90" && prefix == "F3" && mnemonic == "nop")
        return;
    // TODO: weird x87fpu stuff
    if (is_one_byte && pri_opcd == "DB")
        return;
    if (is_one_byte && pri_opcd == "D9")
        return;
    // weird fpu WAIT
    if (prefix == "9B")
        return;
    // no idea how to disambiguate these
    if (is_one_byte && pri_opcd == "6D")
        return;
    if (is_one_byte && pri_opcd == "6F")
        return;
    // looks ambiguous but might be mem vs reg adressing
    // TODO: this seems to be the HL/LH version is for reg-to-reg and the other for to-mem
    if (is_two_byte && pri_opcd == "12" && mnemonic == "movlps")
        return;
    if (is_two_byte && pri_opcd == "16" && mnemonic == "movhps")
        return;

    // no disambiguation needed for both PALIGNR versions
    if (is_two_byte && pri_opcd == "3A" && secondary_op == "0F" && prefix == "66")
        return;

    // don't count as extended if we only have 1 subcode
    if (is_one_byte && (pri_opcd == "8F" || pri_opcd == "C6" || pri_opcd == "C7"))
    {
        is_extended = false;
        extended_op = null;
    }
    if (is_two_byte && (pri_opcode >= 0x90 && pri_opcode <= 0x9F))
    {
        is_extended = false;
        extended_op = null;
    }

    // invalid ops
    if (!is_extended)
    {
        var is_non_64bit = entry.Attribute("mode")?.Value != "e";
        var has_64bit_siblings = entry.Parent!.Elements("entry").Any(e => e.Attribute("mode")?.Value == "e");

        // ignore non-64bit ops if we are not extended and have 64bit siblings
        if (is_non_64bit && has_64bit_siblings)
            return;
    }

    //
    // actual entries
    //
    // TODO: above 256, these kinda get sparse...
    var full_opcode = pri_opcode;
    if (is_two_byte)
        full_opcode += 0x0F_00;
    if (is_extended)
        full_opcode += int.Parse(extended_op!) << 8;
    // this is only for prefixes that change the mnemonic and semantic
    switch (prefix)
    {
        case "66":
            full_opcode += 0x10_00;
            break;
        case "F2":
            full_opcode += 0x11_00;
            break;
        case "F3":
            full_opcode += 0x12_00;
            break;
    }
    if (has_secondary_op)
    {
        // NOTE: this can clash in theory but we check for that so if it compiles it's fine
        var sop = int.Parse(secondary_op!, System.Globalization.NumberStyles.HexNumber);
        full_opcode = (sop << 16) + pri_opcode;
    }
    // if (full_opcode > ushort.MaxValue)
    //     throw new InvalidOperationException("cannot encode full op");

    var computed_subidx = 0;
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

    var has_rm = false;
    string? imm_fmt = null;
    if (is_extended)
        has_rm = true;
    if (isModR(op1) || isModM(op1))
        has_rm = true;
    foreach (var se in syn_parts)
        if (isImmediate(AddrOfOp(se)))
            imm_fmt = ImmTypeOf(AddrOfOp(se), TypeOfOp(se));

    OpEntry MakeOp(int offset = 0) => new OpEntry
    {
        category = cat!,
        mnemonic = mnemonic,
        primary_opcode = pri_opcode + offset,
        full_opcode = full_opcode + offset,
        computed_subidx = computed_subidx,
        has_extended_op = is_extended,
        has_secondary_op = has_secondary_op,
        has_modrm = has_rm,
        imm_fmt = imm_fmt,
        trail_bin_format = (has_rm ? "rm" : "") + (imm_fmt ?? "")
    };

    // TODO: format hints
    if (isOpReg(op1))
    {
        for (var i = 0; i < 8; ++i)
            AddEntry(MakeOp(i));
    }
    else
    {
        AddEntry(MakeOp());
    }
}
foreach (var op in refdoc.Descendants("pri_opcd"))
{
    var has_e = op.Descendants("entry").Any(e => e.Attribute("mode")?.Value == "e");
    var has_ext_op = op.Descendants("entry").Any(e => e.Element("opcd_ext") != null);

    foreach (var entry in op.Descendants("entry"))
    {
        var amode = entry.Attribute("mode");
        if (amode != null && amode.Value != "e")
            continue; // not 64bit mode
        if (has_e && amode == null)
            continue; // if any entry has e, we only use e ones
        if (has_ext_op && entry.Element("opcd_ext") == null)
            continue; // if any entry has extended ops, we only consider those

        if (entry.Attribute("ring")?.Value == "0")
            continue; // ring 0

        if (entry.Attribute("mod")?.Value == "mem")
            continue; // weird x87 stuff
        if (entry.Attribute("mod")?.Value == "nomem")
            continue; // weird x87 stuff
        if (entry.Attribute("mem_format") != null)
            continue; // weird x87 stuff
        if (entry.Attribute("fpop") != null)
            continue; // weird x87 stuff
        if (entry.Attribute("fpush") != null)
            continue; // weird x87 stuff

        if (entry.Element("grp1")?.Value == "x87fpu")
            continue; // just ignore x87 completely for now

        var is_extended = !string.IsNullOrEmpty(entry.Element("opcd_ext")?.Value);

        foreach (var syntax in entry.Elements("syntax"))
        {
            var mnem = syntax.Element("mnem")?.Value;
            if (string.IsNullOrEmpty(mnem))
                continue;

            var s = NormalizeMnemonic(mnem);

            // prefixes
            if (s == "rex" || s.StartsWith("rex."))
                continue; // rex
            if (s == "alter" || s == "fs" || s == "gs")
                continue; // segment / branch prefix
            if (s == "lock")
                continue; // lock
            if (s == "rep")
                continue; // reps

            ProcessEntry(entry, syntax, s);

            // only process the first valid syntax node per entry
            break;
        }

        // THIS IS WRONG
        // if (!is_extended)
        //     break; // for non-extended, only take first entry
    }
}
var mnemonics = mnemonics_set.OrderBy(s => s).ToList();
System.Console.WriteLine($"found {mnemonics.Count} mnemonics");
if (mnemonics.Count > 4 * 256 - 2)
{
    System.Console.WriteLine("ERROR: too many mnemonics! 10 bit assumption violated!");
    return;
}

// DEBUG
if (has_errors)
    return;
// foreach (var e in entries)
//     System.Console.WriteLine($"set_op(0x{e.full_opcode.ToString("X").PadLeft(2, '0')}, mnemonic::{SafeMnemonic(e.mnemonic)}, arg_format::{e.arg_format});");
foreach (var t in todos)
    System.Console.WriteLine("TODO: " + t);
System.Console.WriteLine($"Max full opcode: {entry_by_code.Keys.Max()}");

//
// generate decode table
//
var phase1_to_ops = new Dictionary<int, List<OpEntry>>();
foreach (var e in entries)
{
    if (!phase1_to_ops.ContainsKey(e.phase1_idx))
        phase1_to_ops.Add(e.phase1_idx, new List<OpEntry>());

    // "false" clashes of opcodes that can unambiguously decoded together
    if (phase1_to_ops[e.phase1_idx].Any(ee => e.decode_sig == ee.decode_sig))
        continue;

    phase1_to_ops[e.phase1_idx].Add(e);
}
var clash_cnt = 0;
var phase2_blocks = new List<Dictionary<int, OpEntry>>();
for (var i = 0; i < 512; ++i)
{
    if (!phase1_to_ops.ContainsKey(i))
    {
        System.Console.WriteLine($"[{i.ToString("X").PadLeft(3, '0')}] HOLE");
        continue;
    }

    var es = phase1_to_ops[i];
    if (es.Count > 1)
    {
        System.Console.WriteLine($"[{i.ToString("X").PadLeft(3, '0')}] clash {string.Join(", ", es.Select(e => $"{e.mnemonic}/{e.trail_bin_format}").OrderBy(s => s))}");
        ++clash_cnt;

        var op_by_sub_idx = new Dictionary<int, OpEntry>();
        foreach (var e in es)
        {
            var sub_idx = e.computed_subidx;
            if (op_by_sub_idx.ContainsKey(sub_idx))
            {
                var ee = op_by_sub_idx[sub_idx];
                System.Console.WriteLine($"ERROR: subidx {sub_idx} clash {ee.mnemonic}/{ee.trail_bin_format}/{ee.primary_opcode:X} vs {e.mnemonic}/{e.trail_bin_format}/{e.primary_opcode:X}");
                return;
            }

            op_by_sub_idx.Add(sub_idx, e);
        }

        var any_ext = es.Any(e => e.has_extended_op);
        var any_not_ext = es.Any(e => !e.has_extended_op);
        if (any_ext && any_not_ext)
        {
            System.Console.WriteLine("ERROR: clash with inconsistent is-ext!");
            return;
        }

        var any_sec = es.Any(e => e.has_secondary_op);
        var any_not_sec = es.Any(e => !e.has_secondary_op);
        if (any_sec && any_not_sec)
        {
            System.Console.WriteLine("ERROR: clash with inconsistent is-sec!");
            return;
        }

        var minidx = op_by_sub_idx.Keys.Min();
        var maxidx = op_by_sub_idx.Keys.Max();
        System.Console.WriteLine($"  .. resolved by {maxidx - minidx + 1} slots, {minidx}..{maxidx} | used {op_by_sub_idx.Count}");
        phase2_blocks.Add(op_by_sub_idx);
    }
}
System.Console.WriteLine($"Clashes: {clash_cnt}");

// phase 2
// don't reuse slots from phase1 so we detect unsupported ops!
var phase2_slots = new List<OpEntry?>();
for (var i = 0; i < 300; ++i)
    phase2_slots.Add(null);
var phase2_slots_used = 0;
foreach (var block in phase2_blocks.OrderByDescending(b => b.Keys.Max() - b.Keys.Min()))
{
    var minidx = block.Keys.Min();
    var maxidx = block.Keys.Max();
    System.Console.WriteLine($"trying to alloc a range of {maxidx - minidx + 1} (with used {block.Count} slots)");

    // just brute force alloc
    var cnt = maxidx - minidx + 1;
    var best_idx = -1;
    for (var bi = 0; bi < phase2_slots.Count - cnt; ++bi)
    {
        var ok = true;
        foreach (var kvp in block)
            if (phase2_slots[bi + kvp.Key - minidx] != null)
            {
                ok = false;
                break; // slot taken
            }

        if (ok)
        {
            best_idx = bi;
            break; // first is fine
        }
    }
    if (best_idx == -1)
    {
        System.Console.WriteLine("ERROR: could not alloc");
        return;
    }

    foreach (var kvp in block)
    {
        kvp.Value.phase2_offset = best_idx - minidx;
        phase2_slots[kvp.Key + best_idx - minidx] = kvp.Value;
        phase2_slots_used++;
    }

    System.Console.WriteLine($"used {phase2_slots_used} slots in total");
}
System.Console.WriteLine("phase2:");
while (phase2_slots.Last() == null)
    phase2_slots.RemoveAt(phase2_slots.Count - 1);
for (var i = 0; i < phase2_slots.Count; ++i)
{
    var e = phase2_slots[i];
    System.Console.WriteLine($"[{i:D3}] {(e == null ? "-" : $"{e.mnemonic}/{e.trail_bin_format}/{e.primary_opcode:X}")}");
    // if (e != null && !e.trail_bin_format.StartsWith("rm"))
    // {
    //     System.Console.WriteLine("ERROR: no modrm byte");
    //     return;
    // }
}

//
// generate file
//

ushort MakeTableEntry(string mnemonic, bool has_rm, string? imm_fmt)
{
    int mnem = 0; // invalid
    if (mnemonic == "_invalid")
        mnem = 0;
    else mnem = 1 + mnemonics!.IndexOf(mnemonic);

    int imm_type = 0;
    switch (imm_fmt)
    {
        case "":
        case null: imm_type = 0; break;
        case "8": imm_type = 1; break;
        case "16": imm_type = 2; break;
        case "32": imm_type = 3; break;
        case "32_64": imm_type = 4; break;
        default:
            System.Console.WriteLine("ERROR: unknown fmt " + imm_fmt);
            throw new InvalidOperationException();
    }

    return (ushort)(mnem + ((has_rm ? 1 : 0) << 10) + (imm_type << 11));
}
ushort MakeTableEntryPhase2(bool is_extended, bool has_secondary, int offset)
{
    int code = 0;
    offset += 136;
    if (offset < 0 || offset >= 1024)
        throw new InvalidOperationException("offset out of range");
    code += offset;
    if (is_extended)
        code += 1 << 10; // treat as ModR/M
    if (has_secondary)
        code += 1 << 11;

    code += 1 << 15; // mark as phase2
    return (ushort)(code);
}

var table = new List<ushort>();
// phase 1
for (var i = 0; i < 256 * 2; ++i)
{
    ushort tentry = 0;
    if (!phase1_to_ops.ContainsKey(i))
    {
        tentry = MakeTableEntry("_invalid", false, null);
    }
    else
    {
        var es = phase1_to_ops[i];
        var e = es[0];

        if (es.Count > 1)
        {
            if (es.Any(ee => ee.phase2_offset != e.phase2_offset))
                throw new InvalidOperationException("phase2 offset must be unique");
            if (es.Any(ee => ee.has_secondary_op != e.has_secondary_op))
                throw new InvalidOperationException("has_secondary_op must be unique");

            // ensure that phase2_ext implies modrm
            tentry = MakeTableEntryPhase2(e.has_extended_op, e.has_secondary_op, e.phase2_offset);

            foreach (var ce in es)
            {
                // TODO: check that idx in phase2 part is correct
                var idx2 
            }
        }
        else
        {
            tentry = MakeTableEntry(e.mnemonic, e.has_modrm, e.imm_fmt);
        }
    }
    table.Add(tentry);
}
// phase 2
for (var i = 0; i < phase2_slots.Count; ++i)
{
    ushort tentry = 0;
    var e = phase2_slots[i];
    if (e == null)
        tentry = MakeTableEntry("_invalid", false, null);
    else
        tentry = MakeTableEntry(e.mnemonic, e.has_modrm, e.imm_fmt);
    table.Add(tentry);
}

var table_str = "";
for (var i = 0; i < table.Count; ++i)
{
    if (i % 16 == 0)
        table_str += "    ";
    table_str += table[i];
    table_str += ", ";
    if (i % 16 == 15)
        table_str += " //\n";
}

var hh = @$"#pragma once
// CAUTION: this file is auto-generated. DO NOT MODIFY!

#include <cstdint>

namespace babel::x64
{{

enum class mnemonic : uint16_t 
{{
    _invalid,

    {string.Join(",\n    ", mnemonics.Select(s => SafeMnemonic(s)))}
}};
char const* to_string(mnemonic m);

namespace detail
{{
extern uint16_t const decode_table[{table.Count}];

static constexpr mnemonic entry_mnemonic(uint16_t entry) {{ return mnemonic(entry & 0b11'1111'1111); }}
static constexpr bool entry_has_modrm(uint16_t entry) {{ return (entry & (1 << 10)) != 0; }}
static constexpr bool entry_phase2_add_secondary(uint16_t entry) {{ return (entry & (1 << 11)) != 0; }}
static constexpr bool entry_is_phase2(uint16_t entry) {{ return (entry & (1 << 15)) != 0; }}
static constexpr uint8_t entry_get_immsize(uint16_t entry) {{ return entry >> 11; }}
static constexpr int entry_phase2_get_offset(mnemonic m) {{ return int(m) - 136; }}
}}

}} // babel::x64

";

var cc = @$"// CAUTION: this file is auto-generated. DO NOT MODIFY!
#include ""x64.hh""
#include ""x64.gen.hh""

static char const* s_mnemonic_names[] = {{
    ""<invalid-mnemonic>"",
    {string.Join("\n    ", mnemonics.Select(s => $@"""{s}"","))}
}};

char const* babel::x64::to_string(mnemonic m)
{{
    if (m > mnemonic::{mnemonics.Last()})
        return ""<unknown-mnemonic>"";
    return s_mnemonic_names[int(m)];
}}

uint16_t const babel::x64::detail::decode_table[{table.Count}] =
{{
{table_str}
}};

";

// mnemonic to-string

File.WriteAllText(gen_file_cc, cc);
System.Console.WriteLine("written generated source to " + gen_file_cc);
File.WriteAllText(gen_file_hh, hh);
System.Console.WriteLine("written generated source to " + gen_file_hh);

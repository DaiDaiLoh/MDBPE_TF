class OpEntry
{
    // 1 byte, 2 byte
    public string category = "";
    public int primary_opcode = 0;
    // full is a unique int that is way too large
    public int full_opcode = 0;
    public int phase1_idx => category == "one" ? primary_opcode : primary_opcode + 256;
    public int decode_idx = -1;
    public string mnemonic = "";

    public int computed_subidx = 0;
    public int phase2_offset = int.MaxValue;

    public bool has_modrm = false;
    public string? imm_fmt = null;
    public bool has_extended_op = false;
    public bool has_secondary_op = false;

    // code to identify how trailing bytes must be parsed
    public string trail_bin_format = "";

    // this must be recoverable/known during decode
    public string decode_sig => $"{mnemonic}/{trail_bin_format}";
}
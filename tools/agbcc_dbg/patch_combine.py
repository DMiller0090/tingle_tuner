s = open('combine.c', encoding='latin-1').read()
helper = '''
extern char *current_function_name;
static int dbg_watch (int r)
{
  char *e = getenv ("DBG_REGS");
  char buf[16];
  if (!e) return 0;
  if (getenv ("DBG_FN") && strcmp (getenv ("DBG_FN"), current_function_name)) return 0;
  sprintf (buf, ",%d,", r);
  {
    char tmp[256];
    sprintf (tmp, ",%s,", e);
    return strstr (tmp, buf) != 0;
  }
}
'''
anchor = 'num_sign_bit_copies (x, mode)\n     rtx x;'
i = s.index(anchor)
# insert helper before the function's comment block start: find previous "\n/*" 
j = s.rfind('\n/*', 0, i)
s = s[:j] + helper + s[j:]
old = '''	  && INSN_CUID (reg_last_set[REGNO (x)]) < subst_low_cuid)
	return reg_last_set_sign_bit_copies[REGNO (x)];

      tem =  get_last_value (x);
      if (tem != 0)
	return num_sign_bit_copies (tem, mode);
'''
new = '''	  && INSN_CUID (reg_last_set[REGNO (x)]) < subst_low_cuid)
	{
	  if (dbg_watch (REGNO (x)))
	    fprintf (stderr, "nsbc reg %d path1 -> %d (insn %d)\\n", REGNO (x), reg_last_set_sign_bit_copies[REGNO (x)], INSN_UID (subst_insn));
	  return reg_last_set_sign_bit_copies[REGNO (x)];
	}
      if (dbg_watch (REGNO (x)))
	fprintf (stderr, "nsbc reg %d path1 fail: val=%p mode=%d/%d nsets=%d lbl=%d/%d cuid %d<%d\\n", REGNO (x), (void *) reg_last_set_value[REGNO (x)], reg_last_set_mode[REGNO (x)], mode, REG_N_SETS (REGNO (x)), reg_last_set_label[REGNO (x)], label_tick, reg_last_set[REGNO (x)] ? INSN_CUID (reg_last_set[REGNO (x)]) : -1, subst_low_cuid);

      tem =  get_last_value (x);
      if (tem != 0)
	return num_sign_bit_copies (tem, mode);
      if (dbg_watch (REGNO (x)))
	fprintf (stderr, "nsbc reg %d glv fail, global %d valid %d\\n", REGNO (x), reg_sign_bit_copies[REGNO (x)], nonzero_sign_valid);
'''
assert s.count(old) == 1
s = s.replace(old, new)
old2 = '''  reg_last_set_value[regno] = value;

  if (value)
    {'''
new2 = '''  reg_last_set_value[regno] = value;
  if (dbg_watch (regno))
    {
      fprintf (stderr, "record reg %d insn %d invalid %d value:", regno, insn ? INSN_UID (insn) : -1, reg_last_set_invalid[regno]);
      if (value) print_inline_rtx (stderr, value, 2);
      fprintf (stderr, "\\n");
    }

  if (value)
    {'''
assert s.count(old2) == 1
s = s.replace(old2, new2)
open('combine_dbg.c', 'w', encoding='latin-1', newline='\n').write(s)
s = open('combine_dbg.c', encoding='latin-1').read()
s = s.replace('static rtx\ntry_combine (i3, i2, i1)\n     register rtx i3, i2, i1;\n{',
 'static rtx try_combine_real (rtx, rtx, rtx);\nstatic rtx\ntry_combine (i3, i2, i1)\n     register rtx i3, i2, i1;\n{\n  rtx r = try_combine_real (i3, i2, i1);\n  if (getenv ("DBG_TC") && !strcmp (getenv ("DBG_FN"), current_function_name))\n    fprintf (stderr, "try_combine i3=%d i2=%d i1=%d -> %s" "\n", INSN_UID (i3), INSN_UID (i2), i1 ? INSN_UID (i1) : 0, r ? "OK" : "fail");\n  return r;\n}\nstatic rtx\ntry_combine_real (i3, i2, i1)\n     register rtx i3, i2, i1;\n{', 1)
s = s.replace('static int\ncan_combine_p (insn, i3, pred, succ, pdest, psrc)\n     rtx insn;\n     rtx i3;\n     rtx pred ATTRIBUTE_UNUSED;\n     rtx succ;\n     rtx *pdest, *psrc;\n{',
 'static int can_combine_p_real (rtx, rtx, rtx, rtx, rtx *, rtx *);\nstatic int\ncan_combine_p (insn, i3, pred, succ, pdest, psrc)\n     rtx insn;\n     rtx i3;\n     rtx pred ATTRIBUTE_UNUSED;\n     rtx succ;\n     rtx *pdest, *psrc;\n{\n  int r = can_combine_p_real (insn, i3, pred, succ, pdest, psrc);\n  if (getenv ("DBG_TC") && !strcmp (getenv ("DBG_FN"), current_function_name))\n    fprintf (stderr, "  can_combine_p insn=%d i3=%d -> %d" "\n", INSN_UID (insn), INSN_UID (i3), r);\n  return r;\n}\nstatic int\ncan_combine_p_real (insn, i3, pred, succ, pdest, psrc)\n     rtx insn;\n     rtx i3;\n     rtx pred ATTRIBUTE_UNUSED;\n     rtx succ;\n     rtx *pdest, *psrc;\n{', 1)
assert 'try_combine_real (i3' in s and 'can_combine_p_real (insn' in s
open('combine_dbg.c', 'w', encoding='latin-1', newline='\n').write(s)

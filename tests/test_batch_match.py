from __future__ import annotations

import json
import sys
import tempfile
import unittest
from pathlib import Path
from unittest import mock

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / "tools"))

import batch_match  # noqa: E402
from retail_common import RetailError  # noqa: E402


# These parser inputs use an invented address space and synthetic instructions.
ASM = """\
.include "macro.inc"

.section .text, "ax"

glabel func_81201000
    /* 0 81201000 0800E003 */  jr         $ra
    /* 4 81201004 00000000 */   nop

glabel func_81201008
    /* 8 81201008 1900AC24 */ addiu $t4, $a1, 25
    /* C 8120100C 0800E003 */  jr         $ra
    /* 10 81201010 00000000 */   nop
"""

FALLTHROUGH_LABEL = """\
.section .text, "ax"

glabel func_81201100
    /* 0 81201100 B52A093C */  lui        $t1, 0x2AB5
    /* 4 81201104 42002A95 */  lhu        $t2, 0x42($t1)

glabel func_81201108
    /* 8 81201108 0800E003 */  jr         $ra
    /* C 8120110C 00000000 */   nop
"""

DIRECT_JUMP_THUNK = """\
.section .text, "ax"

glabel func_81201200
    /* 0 81201200 20074808 */  j          func_81201C80
    /* 4 81201204 00000000 */   nop
"""


class EnumerateTests(unittest.TestCase):
    def test_completed_endlabel_excludes_alignment_padding(self) -> None:
        text = '''glabel first
/* 0 81200000 0800E003 */ jr $ra
/* 4 81200004 00000000 */ nop
endlabel first
/* 8 81200008 00000000 */ nop
glabel second
/* C 8120000C 1900AC24 */ addiu $t4, $a1, 25
/* 10 81200010 0800E003 */ jr $ra
/* 14 81200014 00000000 */ nop
'''
        funcs = batch_match.enumerate_functions(text)
        self.assertEqual([(f.name, f.size) for f in funcs],
                         [("first", 8), ("second", 12)])

    def test_endlabel_before_delay_slot_does_not_truncate(self) -> None:
        text = DIRECT_JUMP_THUNK.replace(
            "    /* 4", "endlabel func_81201200\n    /* 4")
        funcs = batch_match.enumerate_functions(text)
        self.assertEqual([(f.name, f.size) for f in funcs], [("func_81201200", 8)])

    def test_label_in_jump_delay_slot_does_not_end_range(self) -> None:
        text = DIRECT_JUMP_THUNK.replace(
            "    /* 4", "glabel delay_slot\n    /* 4")
        funcs = batch_match.enumerate_functions(text)
        self.assertEqual([(f.name, f.size) for f in funcs], [("func_81201200", 8)])

    def test_missing_jump_delay_slot_is_not_a_complete_function(self) -> None:
        text = DIRECT_JUMP_THUNK.split("    /* 4")[0]
        self.assertEqual(batch_match.enumerate_functions(text), [])

    def test_noncontiguous_instruction_cannot_supply_jump_delay_slot(self) -> None:
        text = DIRECT_JUMP_THUNK.replace("4 81201204", "8 81201208")
        self.assertEqual(batch_match.enumerate_functions(text), [])

    def test_forward_target_beyond_input_is_not_a_complete_function(self) -> None:
        text = '''glabel entry
/* 0 81200000 0400C010 */ beqz $a2, missing_target
/* 4 81200004 00000000 */ nop
/* 8 81200008 0800E003 */ jr $ra
/* C 8120000C 00000000 */ nop
'''
        self.assertEqual(batch_match.enumerate_functions(text), [])

    def test_uncrossed_jr_ra_splits_even_without_a_second_glabel(self) -> None:
        """Frameless GTE helpers are often one splat label with many jr $ra tails."""

        text = """\
glabel func_81202000
/* 0 81202000 00000000 */ nop
/* 4 81202004 0800E003 */ jr $ra
/* 8 81202008 00000000 */ nop
/* C 8120200C 00000000 */ nop
/* 10 81202010 3000A394 */ lhu $v1, 0x30($a1)
/* 14 81202014 0800E003 */ jr $ra
/* 18 81202018 00000000 */ nop
"""
        funcs = batch_match.enumerate_functions(text)
        self.assertEqual(
            [(f.name, f.vram, f.size) for f in funcs],
            [
                ("func_81202000", 0x81202000, 12),
                ("func_81202010", 0x81202010, 12),
            ],
        )

    def test_early_return_with_forward_branch_is_not_split(self) -> None:
        text = """\
glabel entry
/* 0 81200000 0300C014 */ bnez $a2, body
/* 4 81200004 00000000 */ nop
/* 8 81200008 0800E003 */ jr $ra
/* C 8120000C 00000000 */ nop
glabel body
/* 10 81200010 37100D24 */ addiu $t5, $zero, 0x1037
/* 14 81200014 0800E003 */ jr $ra
/* 18 81200018 00000000 */ nop
"""
        funcs = batch_match.enumerate_functions(text)
        self.assertEqual([(f.name, f.size) for f in funcs], [("entry", 28)])

    def test_forward_branch_keeps_alternate_path_and_shared_return(self) -> None:
        text = '''glabel entry
/* 0 81200000 0400C010 */ beqz $a2, shared_return
/* 4 81200004 00000000 */ nop
/* 8 81200008 05004808 */ j shared_return
glabel jump_delay
/* C 8120000C 00000000 */ nop
glabel alternate
/* 10 81200010 00000000 */ nop
alabel shared_return
/* 14 81200014 0800E003 */ jr $ra
/* 18 81200018 00000000 */ nop
glabel next_function
/* 1C 8120001C 0800E003 */ jr $ra
/* 20 81200020 00000000 */ nop
'''
        funcs = batch_match.enumerate_functions(text)
        self.assertEqual([(f.name, f.size) for f in funcs],
                         [("entry", 28), ("next_function", 8)])

    def test_functions_are_found_with_their_addresses(self) -> None:
        funcs = batch_match.enumerate_functions(ASM)
        self.assertEqual([f.name for f in funcs], ["func_81201000", "func_81201008"])
        self.assertEqual(funcs[0].vram, 0x81201000)

    def test_size_is_measured_from_the_instruction_words(self) -> None:
        funcs = batch_match.enumerate_functions(ASM)
        self.assertEqual(funcs[0].size, 8)
        self.assertEqual(funcs[1].size, 12)

    def test_a_file_with_no_functions_yields_nothing(self) -> None:
        self.assertEqual(batch_match.enumerate_functions(".section .text\n"), [])

    def test_a_fallthrough_split_is_coalesced_at_its_real_entry(self) -> None:
        """A fall-through label belongs to the preceding callable entry."""

        funcs = batch_match.enumerate_functions(FALLTHROUGH_LABEL)
        self.assertEqual([f.name for f in funcs], ["func_81201100"])
        self.assertEqual(funcs[0].size, 16)

    def test_m2c_input_rewrites_inner_glabels_as_alabels(self) -> None:
        function = batch_match.enumerate_functions(FALLTHROUGH_LABEL)[0]
        rewritten = batch_match.m2c_assembly_text(FALLTHROUGH_LABEL, function)
        self.assertIn("glabel func_81201100", rewritten)
        self.assertIn("alabel func_81201108", rewritten)

    def test_direct_jump_thunk_is_not_a_c_candidate(self) -> None:
        thunk = batch_match.enumerate_functions(DIRECT_JUMP_THUNK)[0]
        self.assertTrue(thunk.is_direct_jump_thunk)
        self.assertFalse(batch_match.should_attempt(Path("missing.c"), thunk))

    def test_internal_label_rewrite_preserves_instruction_lines(self) -> None:
        function = batch_match.enumerate_functions(FALLTHROUGH_LABEL)[0]
        for text in (FALLTHROUGH_LABEL, FALLTHROUGH_LABEL.replace("\n", "\r\n")):
            with self.subTest(crlf="\r\n" in text):
                rewritten = batch_match.m2c_assembly_text(text, function)
                self.assertEqual(rewritten, text.replace("glabel func_81201108", "alabel func_81201108"))


class SafetyTests(unittest.TestCase):
    """The harness must preserve existing source files."""

    def test_an_existing_source_is_never_overwritten(self) -> None:
        with tempfile.TemporaryDirectory() as scratch:
            target = Path(scratch) / "8012ab0.c"
            target.write_text("/* hand written and verified */\n")
            with self.assertRaises(RetailError):
                batch_match.promote(Path(scratch) / "candidate.c", target)
            self.assertEqual(target.read_text(), "/* hand written and verified */\n")

    def test_promotion_writes_a_new_path(self) -> None:
        with tempfile.TemporaryDirectory() as scratch:
            produced = Path(scratch) / "candidate.c"
            produced.write_text("void f(void) {}\n")
            target = Path(scratch) / "new" / "8012ab0.c"
            batch_match.promote(produced, target)
            self.assertEqual(target.read_text(), "void f(void) {}\n")

    def test_an_existing_source_is_skipped_before_any_work(self) -> None:
        with tempfile.TemporaryDirectory() as scratch:
            target = Path(scratch) / "8012ab0.c"
            target.write_text("x")
            self.assertFalse(batch_match.should_attempt(target))
            self.assertTrue(batch_match.should_attempt(Path(scratch) / "absent.c"))


class HeaderTests(unittest.TestCase):
    """Only a successful oracle comparison may mark a candidate verified."""

    STUB = "import sys; sys.stdout.write('void f(void) {}\\n')\n"

    def _run_m2c(self, scratch: Path) -> Path:
        stub = scratch / "stub_m2c.py"
        stub.write_text(self.STUB)
        out = scratch / "func.c"
        self.assertTrue(
            batch_match.run_m2c(stub, scratch / "x.s", "func", out)
        )
        return out

    def test_candidate_header_claims_no_verification(self) -> None:
        with tempfile.TemporaryDirectory() as scratch:
            out = self._run_m2c(Path(scratch))
            text = out.read_text()
            self.assertNotIn("verified byte-exact", text)
            self.assertIn("NOT verified", text)

    def test_stamp_verified_replaces_the_draft_header(self) -> None:
        with tempfile.TemporaryDirectory() as scratch:
            out = self._run_m2c(Path(scratch))
            batch_match.stamp_verified(out, "x.s")
            text = out.read_text()
            self.assertIn("verified byte-exact", text)
            self.assertNotIn("not verified", text)
            self.assertIn("void f(void) {}", text)

    def test_stamp_verified_prepends_when_no_draft_header(self) -> None:
        with tempfile.TemporaryDirectory() as scratch:
            bare = Path(scratch) / "bare.c"
            bare.write_text("void f(void) {}\n")
            batch_match.stamp_verified(bare, "x.s")
            text = bare.read_text()
            self.assertIn("verified byte-exact", text)
            self.assertIn("void f(void) {}", text)


class SourcePathTests(unittest.TestCase):
    def test_a_main_function_maps_to_its_address(self) -> None:
        path = batch_match.source_path(Path("/repo"), "main", 0x81201000)
        self.assertEqual(path, Path("/repo/src/main/81201000.c"))

    def test_an_overlay_function_maps_under_its_region(self) -> None:
        path = batch_match.source_path(Path("/repo"), "main_0012", 0x81300040)
        self.assertEqual(path, Path("/repo/src/overlays/main_0012/81300040.c"))


class SanitizeTests(unittest.TestCase):
    """m2c's `?` placeholder for an unseen symbol is not valid C.

    m2c cannot always tell an uncalled data symbol from a called function, so
    the sanitizer decides from the body: a declared name that is never
    followed by `(` anywhere in the function is data, not a callee.
    """

    def test_a_called_name_keeps_its_function_declaration(self) -> None:
        out = batch_match._sanitize(
            "? func_flush();                                  /* extern */\n\n"
            "void func_record(void) {\n    func_flush();\n}\n"
        )
        self.assertIn("void func_flush();", out)
        self.assertIn("func_flush();\n}", out)

    def test_an_uncalled_name_becomes_extern_data(self) -> None:
        """The failure this guards: an unknown symbol only ever assigned to,
        never called, must not be declared as a function -- assigning to a
        function name is not a valid lvalue."""

        out = batch_match._sanitize(
            "? D_sample_tick();                                     /* static */\n\n"
            "void func_store_tick(s32 ticks) {\n    D_sample_tick = ticks + 7;\n}\n"
        )
        self.assertIn("extern s32 D_sample_tick;", out)
        self.assertNotIn("D_sample_tick()", out)

    def test_unknown_pointer_parameter_becomes_void_star(self) -> None:
        out = batch_match._sanitize(
            "? func_accept(? *);                               /* static */\n\n"
            "void func_visit(void) {\n    func_accept(0);\n}\n"
        )
        self.assertIn("void func_accept(void *);", out)

    def test_unknown_local_variable_type_becomes_s32(self) -> None:
        out = batch_match._sanitize(
            "void func_visit(void) {\n    ? sample_value;\n\n    func_accept(&sample_value);\n}\n"
        )
        self.assertIn("    s32 sample_value;", out)

    def test_unknown_argument_list_becomes_empty_parens(self) -> None:
        out = batch_match._sanitize(
            "? func_signal(?);                                 /* static */\n\n"
            "void func_record(void) {\n    func_signal(0);\n}\n"
        )
        self.assertIn("func_signal()", out)
        self.assertNotIn("(?)", out)

    def test_the_call_site_itself_is_left_untouched(self) -> None:
        out = batch_match._sanitize("void func_record(void) {\n    func_signal(0);\n}\n")
        self.assertIn("func_signal(0)", out)

    def test_unknown_function_pointer_local_becomes_s32(self) -> None:
        """An untyped local callback needs a return type."""

        out = batch_match._sanitize(
            "void func_invoke(void) {\n    ? (*notify)();\n    notify();\n}\n"
        )
        self.assertIn("s32 (*notify)();", out)
        self.assertNotIn("? (*notify)()", out)

    def test_unknown_extern_data_declaration_becomes_s32(self) -> None:
        """`extern ? D_*;` is the same register-width-unknown data case as
        `? name;` locals, just spelled as an extern declaration."""

        out = batch_match._sanitize("extern ? D_sample;\nvoid f(void) {\n}\n")
        self.assertIn("extern s32 D_sample;", out)
        self.assertNotIn("extern ? ", out)

    def test_anonymous_function_pointer_parameter_becomes_s32(self) -> None:
        """Preserve the unnamed callback parameter."""

        out = batch_match._sanitize(
            "void func_take_callback(? (*)(), s32);\nvoid f(void) {\n}\n"
        )
        self.assertIn("void func_take_callback(s32 (*)(), s32);", out)
        self.assertNotIn("? (*)()", out)

    def test_named_unknown_parameter_becomes_s32(self) -> None:
        """Supply a register-sized type for each named parameter."""

        out = batch_match._sanitize(
            "void func_measure(? arg1, s32 arg2, ? arg3) {\n}\n"
        )
        self.assertIn("void func_measure(s32 arg1, s32 arg2, s32 arg3) {", out)
        self.assertNotIn("? arg", out)

    def test_function_definition_with_unknown_return_becomes_s32(self) -> None:
        """A definition with an unknown pointer return can use a register-sized result."""

        out = batch_match._sanitize("? (*func_make(void))() {\n}\n")
        self.assertIn("s32 func_make(void) {", out)

    def test_callback_prototype_keeps_its_shape_with_s32_return(self) -> None:
        """Preserve the callback returned by a factory prototype."""

        out = batch_match._sanitize(
            "? (*func_pick(s32))(void *);\nvoid f(void) {\n}\n"
        )
        self.assertIn("s32 (*func_pick(s32))(void *);", out)
        self.assertNotIn("? (*func_pick", out)

    def test_null_macro_becomes_zero(self) -> None:
        """The freestanding compiler provides no hosted NULL definition."""

        out = batch_match._sanitize(
            "s32 f(s32 *p) {\n    if (p == NULL) {\n        return NULL;\n    }\n    return 1;\n}\n"
        )
        self.assertIn("if (p == 0) {", out)
        self.assertIn("return 0;", out)
        self.assertNotIn("NULL", out)

    def test_deref_only_extern_data_becomes_pointer(self) -> None:
        """A symbol used only as a destination pointer needs a pointer declaration."""

        out = batch_match._sanitize(
            "extern s32 D_sample_output;\nvoid f(void) {\n    *D_sample_output = 23;\n}\n"
        )
        self.assertIn("extern s32 *D_sample_output;", out)

    def test_mixed_deref_and_assign_stays_s32(self) -> None:
        """`*D` plus a plain `D = v` is contradictory — no declaration
        satisfies both without knowing the layout. Leave it failing
        loudly for hand work rather than guess."""

        out = batch_match._sanitize(
            "extern s32 D;\nvoid f(void) {\n    D = 1;\n    *D = 2;\n}\n"
        )
        self.assertIn("extern s32 D;", out)
        self.assertNotIn("s32 *D;", out)

    def test_called_through_extern_becomes_function_pointer(self) -> None:
        """`(*D)(x)` calls through D, so D is a callback, not data.
        `s32 *D` would not compile there; `s32 (*D)()` does, with the
        same jalr sequence either way."""

        out = batch_match._sanitize(
            "extern s32 D;\nvoid f(void) {\n    (*D)(1);\n}\n"
        )
        self.assertIn("extern s32 (*D)();", out)

    def test_called_but_undeclared_func_gets_void_prototype(self) -> None:
        """A call without a declaration gets an unchecked C89 prototype."""

        out = batch_match._sanitize("void f(void) {\n    func_push(1);\n}\n")
        self.assertIn("void func_push();", out)

    def test_value_used_but_undeclared_data_gets_extern(self) -> None:
        out = batch_match._sanitize("void f(void) {\n    g(D_sample_value);\n}\n")
        self.assertIn("extern s32 D_sample_value;", out)

    def test_no_duplicate_decl_for_callback_prototype(self) -> None:
        """`s32 (*f(..))(..);` already declares f: the missing-symbol
        pass must recognise the `(*f` form as a declaration, or it
        would append a conflicting `void f();` and break a file that
        currently compiles."""

        src = "s32 (*func_pick(s32))(void *);\nvoid f(void) {\n}\n"
        out = batch_match._sanitize(src)
        self.assertEqual(out.count("func_pick"), 1)

    def test_no_decl_for_the_defined_function_itself(self) -> None:
        """A recursive call must not conjure a prototype that conflicts
        with the definition's return type."""

        src = "s32 target(s32 x) {\n    return target(x - 1);\n}\n"
        out = batch_match._sanitize(src)
        self.assertNotIn("void target();", out)
        self.assertNotIn("extern", out)

    def test_m2c_warning_lines_are_dropped(self) -> None:
        """A diagnostic printed to stdout cannot be compiled as C."""

        out = batch_match._sanitize(
            'Warning: fixture has an incomplete block.\n'
            "void func_emit(void) {\n}\n"
        )
        self.assertNotIn("Warning:", out)
        self.assertIn("void func_emit(void) {", out)

    def test_bare_unknown_parameter_becomes_s32(self) -> None:
        """A lone `?` parameter is an unknown word-sized argument, not the
        C89 unchecked-args `(?)` form, which keeps its own empty-parens
        mapping."""

        out = batch_match._sanitize(
            "void func_scale(?, ?);\nvoid f(void) {\n}\n"
        )
        self.assertIn("void func_scale(s32, s32);", out)
        self.assertNotIn("(?,", out)


GTE_SNIPPET = """\
glabel func_81202000
/* 0 81202000 2600AB84 */ lh $t3, 0x26($a1)
/* 4 81202004 0030CB48 */ ctc2 $t3, $6
/* 8 81202008 0800E003 */ jr $ra
/* C 8120200C 00000000 */ nop
"""


class GteOverlayTests(unittest.TestCase):
    def test_instruction_words_use_little_endian_encoding(self) -> None:
        function = batch_match.FunctionRange("func_81202000", 0x81202000, 16)
        words = batch_match.instruction_words(GTE_SNIPPET, function)
        self.assertEqual(
            [(addr, word, op) for addr, word, op in words],
            [
                (0x81202000, 0x84AB0026, "lh"),
                (0x81202004, 0x48CB3000, "ctc2"),
                (0x81202008, 0x03E00008, "jr"),
                (0x8120200C, 0x00000000, "nop"),
            ],
        )

    def test_gte_opcode_is_detected(self) -> None:
        function = batch_match.FunctionRange("func_81202000", 0x81202000, 16)
        self.assertTrue(batch_match.function_contains_gte(GTE_SNIPPET, function))
        self.assertFalse(batch_match.function_contains_gte(ASM, batch_match.FunctionRange("func_81201000", 0x81201000, 8)))

    def test_overlay_source_emits_gte_mnemonics_not_word_dumps(self) -> None:
        function = batch_match.FunctionRange("func_81202000", 0x81202000, 16)
        lines = batch_match.instruction_lines(GTE_SNIPPET, function)
        source = batch_match.assembly_overlay_source(function, lines)
        self.assertIn("ctc2 $t3,$6", source)
        self.assertIn("lh $t3,0x26($a1)", source)
        self.assertNotIn(".word 0x", source)
        self.assertIn("func_81202000:", source)
        self.assertEqual(batch_match.progress.classify_recovery(source), "assembly")

    def test_overlay_uses_word_only_for_dmpsx_fake_ops(self) -> None:
        listing = """\
glabel func_81202100
/* 0 81202100 00288C48 */ mtc2 $t4, $5
/* 4 81202104 12244B4A */ mvmva 1, 1, 2, 1, 1
/* 8 81202108 0800E003 */ jr $ra
/* C 8120210C 00000000 */ nop
"""
        function = batch_match.FunctionRange("func_81202100", 0x81202100, 16)
        source = batch_match.assembly_overlay_source(
            function, batch_match.instruction_lines(listing, function), listing=listing
        )
        self.assertIn("mtc2 $t4,$5", source)
        self.assertIn(".word 0x4A4B2412", source)
        self.assertIn("jr $ra", source)
        self.assertEqual(source.count(".word 0x"), 1)
        self.assertEqual(batch_match.progress.classify_recovery(source), "assembly")

    def test_overlay_compacts_commas_and_evaluates_splat_immediates(self) -> None:
        listing = """\
glabel func_81202200
/* 0 81202200 2B50C500 */ sltu $t2, $a2, $a1
/* 4 81202204 695A093C */ lui $t1, (0x5A690000 >> 16)
/* 8 81202208 00288C48 */ mtc2 $t4, $5
/* C 8120220C 0800E003 */ jr $ra
/* 10 81202210 00000000 */ nop
"""
        function = batch_match.FunctionRange("func_81202200", 0x81202200, 20)
        source = batch_match.assembly_overlay_source(
            function, batch_match.instruction_lines(listing, function), listing=listing
        )
        self.assertIn("sltu $t2,$a2,$a1", source)
        self.assertIn("lui $t1,0x5A69", source)
        self.assertNotIn("(0x5A690000 >> 16)", source)
        self.assertIn("mtc2 $t4,$5", source)
        self.assertEqual(batch_match.progress.classify_recovery(source), "assembly")

    def test_overlay_encodes_break_as_its_instruction_word(self) -> None:
        listing = """\
glabel func_81202300
/* 0 81202300 00288C48 */ mtc2 $t4, $5
/* 4 81202304 0D000B00 */ break 11
/* 8 81202308 0800E003 */ jr $ra
/* C 8120230C 00000000 */ nop
"""
        function = batch_match.FunctionRange("func_81202300", 0x81202300, 16)
        source = batch_match.assembly_overlay_source(
            function, batch_match.instruction_lines(listing, function), listing=listing
        )
        self.assertIn("mtc2 $t4,$5", source)
        self.assertIn(".word 0x000B000D", source)
        self.assertNotIn("break", source)
        self.assertEqual(batch_match.progress.classify_recovery(source), "assembly")

    def test_non_gte_overlay_is_refused(self) -> None:
        function = batch_match.FunctionRange("func_81201000", 0x81201000, 8)
        words = batch_match.instruction_words(ASM, function)
        with self.assertRaises(batch_match.RetailError):
            batch_match.assembly_overlay_source(function, words)

    def test_gte_opcodes_do_not_include_cpu_priv_ops(self) -> None:
        self.assertNotIn("mfc0", batch_match.GTE_OPCODES)
        self.assertNotIn("mtc0", batch_match.GTE_OPCODES)
        self.assertNotIn("syscall", batch_match.GTE_OPCODES)
        self.assertIn("ctc2", batch_match.GTE_OPCODES)


class PromotedEntryTests(unittest.TestCase):
    def test_promoted_c_is_classified_as_a_complete_c_function(self) -> None:
        function = batch_match.FunctionRange("func_81201000", 0x81201000, 12)
        entry = batch_match.promoted_entry(
            function,
            "main",
            "src/main/81201000.c",
            "int func_81201000(int a, int b) { return a != b; }\n",
            "-O2",
        )
        self.assertEqual(entry["recovery"], "c")
        self.assertEqual(entry["extent"], "function")
        self.assertEqual(entry["optimization"], "-O2")

    def test_promoted_asm_overlay_is_classified_as_assembly(self) -> None:
        function = batch_match.FunctionRange("func_81202900", 0x81202900, 112)
        entry = batch_match.promoted_entry(
            function,
            "main",
            "src/main/81202900.c",
            '__asm__("jr $ra\\nnop\\n");\n',
            None,
        )
        self.assertEqual(entry["recovery"], "assembly")
        self.assertEqual(entry["extent"], "function")
        self.assertNotIn("optimization", entry)


class RegisterTests(unittest.TestCase):
    """Each successful promotion must reach the registry immediately."""

    def test_register_appends_and_persists_immediately(self) -> None:
        with tempfile.TemporaryDirectory() as scratch:
            registry_path = Path(scratch) / "matches.json"
            registry = {"matches": []}
            entry = {"name": "func_81201000", "vram": 0x81201000, "size": 12,
                      "region": "main", "source": "src/main/81201000.c"}
            batch_match.register(registry, registry_path, entry)
            self.assertTrue(registry_path.is_file())
            reloaded = json.loads(registry_path.read_text())
            self.assertEqual(reloaded["matches"], [entry])

    def test_register_does_not_duplicate_an_existing_entry(self) -> None:
        with tempfile.TemporaryDirectory() as scratch:
            registry_path = Path(scratch) / "matches.json"
            entry = {"name": "func_81201000", "vram": 0x81201000, "size": 12,
                      "region": "main", "source": "src/main/81201000.c"}
            registry = {"matches": [dict(entry)]}
            batch_match.register(registry, registry_path, entry)
            self.assertEqual(len(registry["matches"]), 1)

    def test_register_keeps_matches_sorted_by_region_then_vram(self) -> None:
        with tempfile.TemporaryDirectory() as scratch:
            registry_path = Path(scratch) / "matches.json"
            registry = {"matches": [
                {"name": "func_81201008", "vram": 0x81201008, "size": 4, "region": "main", "source": "a"},
            ]}
            batch_match.register(
                registry, registry_path,
                {"name": "func_81201000", "vram": 0x81201000, "size": 4, "region": "main", "source": "b"},
            )
            self.assertEqual([m["vram"] for m in registry["matches"]], [0x81201000, 0x81201008])


class OutcomeTests(unittest.TestCase):
    """Only an exact match is promoted; everything else leaves no trace."""

    def test_a_mismatch_is_not_promoted(self) -> None:
        self.assertFalse(batch_match.Outcome("func_x", "mismatch").promoted)

    def test_a_match_is_promoted(self) -> None:
        self.assertTrue(batch_match.Outcome("func_x", "match").promoted)

    def test_a_build_failure_is_not_promoted(self) -> None:
        self.assertFalse(batch_match.Outcome("func_x", "build-failed").promoted)


class OptimizationFallbackTests(unittest.TestCase):
    def test_attempt_promotes_only_after_the_o0_retry_matches(self) -> None:
        """A failed default build may be recoverable under the retail -O0 lane."""

        function = batch_match.FunctionRange("func_81200000", 0x81200000, 8)
        with tempfile.TemporaryDirectory() as temporary:
            repo = Path(temporary)
            asm = repo / "main.s"
            asm.write_text("glabel func_81200000\n/* 0 81200000 00000000 */ nop\n")
            m2c = repo / "m2c.py"
            m2c.write_text("")
            scratch = repo / "scratch"
            scratch.mkdir()

            def fake_m2c(_m2c, _asm, _name, output, **_kwargs):
                output.write_text("void func_81200000(void) {}\n")
                return True

            build_optimizations: list[str] = []
            match_results = iter([1, 0])

            def fake_quiet(fn, argv):
                if fn is batch_match.build_candidate.main:
                    build_optimizations.append(next(item for item in argv if item.startswith("--optimization=")))
                    Path(argv[argv.index("--output") + 1]).write_bytes(b"\0" * 8)
                    return 0
                self.assertIs(fn, batch_match.match_function.main)
                return next(match_results)

            with mock.patch.object(batch_match, "run_m2c", side_effect=fake_m2c), mock.patch.object(
                batch_match, "_quiet", side_effect=fake_quiet
            ):
                outcome = batch_match.attempt(
                    function, "main", {"kind": "executable"}, repo, asm, m2c, scratch
                )

            self.assertTrue(outcome.promoted)
            self.assertEqual(outcome.optimization, "-O0")
            self.assertEqual(build_optimizations, ["--optimization=-O2", "--optimization=-O0"])
            self.assertTrue((repo / "src/main/81200000.c").is_file())


class GteAttemptTests(unittest.TestCase):
    def test_gte_function_promotes_an_overlay_without_m2c(self) -> None:
        function = batch_match.FunctionRange("func_81202000", 0x81202000, 16)
        with tempfile.TemporaryDirectory() as temporary:
            repo = Path(temporary)
            asm = repo / "main.s"
            asm.write_text(GTE_SNIPPET)
            m2c = repo / "m2c.py"
            m2c.write_text("")
            scratch = repo / "scratch"
            scratch.mkdir()

            def fake_quiet(fn, argv):
                if fn is batch_match.build_candidate.main:
                    Path(argv[argv.index("--output") + 1]).write_bytes(b"\0" * 16)
                    return 0
                self.assertIs(fn, batch_match.match_function.main)
                return 0

            with mock.patch.object(batch_match, "run_m2c") as run_m2c, mock.patch.object(
                batch_match, "_quiet", side_effect=fake_quiet
            ):
                outcome = batch_match.attempt(
                    function, "main", {"kind": "executable"}, repo, asm, m2c, scratch
                )

            run_m2c.assert_not_called()
            self.assertTrue(outcome.promoted)
            self.assertIsNone(outcome.optimization)
            text = (repo / "src/main/81202000.c").read_text()
            self.assertIn("ctc2 $t3,$6", text)
            self.assertNotIn(".word 0x", text)
            self.assertNotIn("Decompiled by m2c", text)
            self.assertEqual(batch_match.progress.classify_recovery(text), "assembly")


class VerifyExistingTests(unittest.TestCase):
    """Existing src/ files can be registered on MATCH but must never be rewritten."""

    def test_match_does_not_modify_the_existing_source(self) -> None:
        function = batch_match.FunctionRange("func_81200000", 0x81200000, 8)
        with tempfile.TemporaryDirectory() as temporary:
            repo = Path(temporary)
            source = repo / "src/main/81200000.c"
            source.parent.mkdir(parents=True)
            original = "void func_81200000(void) {}\n"
            source.write_text(original)
            scratch = repo / "scratch"
            scratch.mkdir()

            def fake_quiet(fn, argv):
                if fn is batch_match.build_candidate.main:
                    Path(argv[argv.index("--output") + 1]).write_bytes(b"\0" * 8)
                    return 0
                self.assertIs(fn, batch_match.match_function.main)
                return 0

            with mock.patch.object(batch_match, "_quiet", side_effect=fake_quiet):
                outcome = batch_match.verify_existing(
                    function, "main", {"kind": "executable"}, source, scratch
                )

            self.assertTrue(outcome.promoted)
            self.assertEqual(outcome.optimization, "-O2")
            self.assertEqual(source.read_text(), original)

    def test_mismatch_is_not_promoted_and_leaves_the_file(self) -> None:
        function = batch_match.FunctionRange("func_81200000", 0x81200000, 8)
        with tempfile.TemporaryDirectory() as temporary:
            source = Path(temporary) / "src.c"
            source.write_text("void func_81200000(void) {}\n")
            scratch = Path(temporary) / "scratch"
            scratch.mkdir()

            def fake_quiet(fn, argv):
                if fn is batch_match.build_candidate.main:
                    Path(argv[argv.index("--output") + 1]).write_bytes(b"\0" * 8)
                    return 0
                return 1

            with mock.patch.object(batch_match, "_quiet", side_effect=fake_quiet):
                outcome = batch_match.verify_existing(
                    function, "main", {"kind": "executable"}, source, scratch
                )

            self.assertFalse(outcome.promoted)
            self.assertEqual(outcome.status, "mismatch")
            self.assertTrue(source.is_file())

    def test_missing_source_is_skipped(self) -> None:
        function = batch_match.FunctionRange("func_81200000", 0x81200000, 8)
        with tempfile.TemporaryDirectory() as temporary:
            outcome = batch_match.verify_existing(
                function,
                "main",
                {"kind": "executable"},
                Path(temporary) / "missing.c",
                Path(temporary),
            )
        self.assertEqual(outcome.status, "skipped-missing")
        self.assertFalse(outcome.promoted)


class CandidateBankTests(unittest.TestCase):
    def test_sanitizer_preserves_typed_context_externs(self) -> None:
        source = (
            'typedef struct State { int field; } State;\n'
            'extern State D_state;\n'
            'extern struct State D_table[2];\n'
            'void func_test(void) { D_state.field = D_table[1].field; }\n'
        )
        self.assertEqual(batch_match._sanitize(source), source)

    def test_banked_candidate_lookup_requires_an_existing_draft(self) -> None:
        function = batch_match.FunctionRange("func_81200000", 0x81200000, 8)
        with tempfile.TemporaryDirectory() as temporary:
            bank = Path(temporary)
            self.assertIsNone(batch_match.banked_candidate_path(None, function))
            self.assertIsNone(batch_match.banked_candidate_path(bank, function))
            candidate = bank / "func_81200000.c"
            candidate.write_text("void func_81200000(void) {}\n")
            self.assertEqual(batch_match.banked_candidate_path(bank, function), candidate)

    def test_banked_draft_is_oracle_checked_without_rerunning_m2c(self) -> None:
        """The candidate cache is only an input optimization, never authority."""

        function = batch_match.FunctionRange("func_81200000", 0x81200000, 8)
        with tempfile.TemporaryDirectory() as temporary:
            repo = Path(temporary)
            asm = repo / "main.s"
            asm.write_text("glabel func_81200000\n/* 0 81200000 00000000 */ nop\n")
            m2c = repo / "m2c.py"
            m2c.write_text("")
            scratch = repo / "scratch"
            scratch.mkdir()
            bank = repo / "bank"
            bank.mkdir()
            banked = bank / "func_81200000.c"
            original = "void func_81200000(void) {}\n"
            banked.write_text(original)

            def fake_quiet(fn, argv):
                if fn is batch_match.build_candidate.main:
                    Path(argv[argv.index("--output") + 1]).write_bytes(b"\0" * 8)
                    return 0
                self.assertIs(fn, batch_match.match_function.main)
                return 0

            with mock.patch.object(batch_match, "run_m2c") as run_m2c, mock.patch.object(
                batch_match, "_quiet", side_effect=fake_quiet
            ):
                outcome = batch_match.attempt(
                    function, "main", {"kind": "executable"}, repo, asm, m2c, scratch, bank
                )

            run_m2c.assert_not_called()
            self.assertTrue(outcome.promoted)
            self.assertEqual(banked.read_text(), original)
            self.assertIn("verified byte-exact", (repo / "src/main/81200000.c").read_text())


if __name__ == "__main__":
    unittest.main()

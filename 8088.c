/*
  Copyright (C) 2000-2016  Hideki EIRAKU

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/

typedef unsigned int t16;
typedef unsigned int t20;
typedef unsigned int u32;

#include <stdio.h>

void printip8088(void);
#define E_WARN(mes) (fprintf (stderr, "8088: WARNING: %s\n", mes), printip8088 ())

struct instdbg_data
{
  t16 regs[8];
  t16 flags;
  t16 flagstmp;
};

#ifdef DEBUG
#define INSTDBG_PREPARE_REG() \
  struct instdbg_data instdbg_before, instdbg_after; \
  instdbg_before.regs[0] = reg2 (0); \
  instdbg_before.regs[1] = reg2 (1); \
  instdbg_before.regs[2] = reg2 (2); \
  instdbg_before.regs[3] = reg2 (3); \
  instdbg_before.regs[4] = reg2 (4); \
  instdbg_before.regs[5] = reg2 (5); \
  instdbg_before.regs[6] = reg2 (6); \
  instdbg_before.regs[7] = reg2 (7); \
  instdbg_before.flags = f_get (); \
  instdbg_before.flagstmp = flagstmp

#define INSTDBG_PREPARE_OP(op1, op2, op3) \
  struct instdbg_data instdbg_before, instdbg_after; \
  instdbg_before.regs[0] = (op1); \
  instdbg_before.regs[1] = (op2); \
  instdbg_before.regs[2] = (op3); \
  instdbg_before.regs[3] = reg2 (3); \
  instdbg_before.regs[4] = reg2 (4); \
  instdbg_before.regs[5] = reg2 (5); \
  instdbg_before.regs[6] = reg2 (6); \
  instdbg_before.regs[7] = reg2 (7); \
  instdbg_before.flags = f_get (); \
  instdbg_before.flagstmp = flagstmp

#define INSTDBG_PREPARE_OPC(op1, op2, op3) \
  struct instdbg_data instdbg_before, instdbg_after; \
  instdbg_before.regs[0] = (op1); \
  instdbg_before.regs[1] = (op2); \
  instdbg_before.regs[2] = (op3); \
  instdbg_before.regs[3] = reg2 (3); \
  instdbg_before.regs[4] = reg2 (4); \
  instdbg_before.regs[5] = reg2 (5); \
  instdbg_before.regs[6] = reg2 (6); \
  instdbg_before.regs[7] = reg2 (7); \
  instdbg_before.flags = f_get (); \
  instdbg_before.flagstmp = 0

#define INSTDBG_COMPARE(inst, name, REG1, REG2) \
  if ((instdbg_after.REG1) != (REG2)) \
    fprintf (stderr, "INSTDBG: mismatch inst=%s reg=%s ax=%04x->%04x" \
	     " before=%04x result=%04x should be %04x\n", (inst), (name), \
	     instdbg_before.regs[0], instdbg_after.regs[0], \
	     (instdbg_before.REG1), (REG2), (instdbg_after.REG1))

#define INSTDBG_TEST_REG(inst) \
  INSTDBG_TEST_OP (inst, reg2 (0), reg2 (1), reg2 (2))

#define INSTDBG_TEST_OP(inst, op1, op2, op3) \
  INSTDBG_TEST_OPF(inst, op1, op2, op3, ~0)

#define INSTDBG_TEST_OPF(inst, op1, op2, op3, FLG) \
  asm volatile ("pusha; push %%edx;" \
		"pushf; and %%eax,(%%esp); or %%ebx,(%%esp);" \
		"mov 0(%%ecx),%%eax; mov 8(%%ecx),%%edx;" \
		"mov 12(%%ecx),%%ebx; mov 20(%%ecx),%%ebp;" \
		"mov 24(%%ecx),%%esi; mov 28(%%ecx),%%edi;" \
		"mov 4(%%ecx),%%ecx; popf; " inst ";" \
		"xchg %%edx,(%%esp); mov %%eax,0(%%edx);" \
		"mov %%ecx, 4(%%edx); popl 8(%%edx); mov %%ebx,12(%%edx);" \
		"mov %%ebp,20(%%edx); mov %%esi,24(%%edx);" \
		"mov %%edi,28(%%edx); pushf; popl 32(%%edx); popa" \
		: : "a" (~(2048+16+1+128+64+4)), \
		"b" (instdbg_before.flags & (2048+16+1+128+64+4)), \
		"c" (&instdbg_before), "d" (&instdbg_after) \
		: "cc", "memory"); \
  instdbg_after.flags &= (2048+16+1+128+64+4); \
  instdbg_after.flags |= instdbg_before.flags & ~(2048+16+1+128+64+4); \
  instdbg_after.flags &= (FLG); \
  instdbg_after.flags |= f_get () & ~(FLG); \
  instdbg_after.flags |= instdbg_before.flagstmp; \
  INSTDBG_COMPARE (inst, "AX", regs[0], (op1)); \
  INSTDBG_COMPARE (inst, "CX", regs[1], (op2)); \
  INSTDBG_COMPARE (inst, "DX", regs[2], (op3)); \
  INSTDBG_COMPARE (inst, "BX", regs[3], reg2 (3)); \
  /*INSTDBG_COMPARE (inst, "SP", regs[4], reg2 (4));*/ \
  INSTDBG_COMPARE (inst, "BP", regs[5], reg2 (5)); \
  INSTDBG_COMPARE (inst, "SI", regs[6], reg2 (6)); \
  INSTDBG_COMPARE (inst, "DI", regs[7], reg2 (7)); \
  INSTDBG_COMPARE (inst, "FLAGS", flags, f_get ())
#else
#define INSTDBG_PREPARE_REG()
#define INSTDBG_PREPARE_OP(op1, op2, op3)
#define INSTDBG_PREPARE_OPC(op1, op2, op3)
#define INSTDBG_COMPARE(inst, name, REG1, REG2)
#define INSTDBG_TEST_REG(inst)
#define INSTDBG_TEST_OP(inst, op1, op2, op3)
#define INSTDBG_TEST_OPF(inst, op1, op2, op3, FLG)
#endif

#define getes2() sregs[0]
#define getcs2() sregs[1]
#define getss2() sregs[2]
#define getds2() sregs[3]
#define letes2(a) sregs[0]=(a)
#define letcs2(a) (sregs[1]=(a), E_WARN ("letcs2!"))
#define letss2(a) sregs[2]=(a)
#define letds2(a) sregs[3]=(a)
#define getax2() reg2 (0)
#define getcx2() reg2 (1)
#define getdx2() reg2 (2)
#define getbx2() reg2 (3)
#define getsp2() reg2 (4)
#define getbp2() reg2 (5)
#define getsi2() reg2 (6)
#define getdi2() reg2 (7)
#define getal1() reg1 (0)
#define getcl1() reg1 (1)
#define getdl1() reg1 (2)
#define getbl1() reg1 (3)
#define getah1() reg1 (4)
#define getch1() reg1 (5)
#define getdh1() reg1 (6)
#define getbh1() reg1 (7)
#define getip2() ip
#define getflags2() f_get ()
#define letax2(a) letreg2 (0, (a))
#define letcx2(a) letreg2 (1, (a))
#define letdx2(a) letreg2 (2, (a))
#define letbx2(a) letreg2 (3, (a))
#define letsp2(a) letreg2 (4, (a))
#define letbp2(a) letreg2 (5, (a))
#define letsi2(a) letreg2 (6, (a))
#define letdi2(a) letreg2 (7, (a))
#define letal1(a) letreg1 (0, (a))
#define letcl1(a) letreg1 (1, (a))
#define letdl1(a) letreg1 (2, (a))
#define letbl1(a) letreg1 (3, (a))
#define letah1(a) letreg1 (4, (a))
#define letch1(a) letreg1 (5, (a))
#define letdh1(a) letreg1 (6, (a))
#define letbh1(a) letreg1 (7, (a))
#define letflags2(a) f_clear (), f_or (a), f_mask (65535)

#define br break

#define E_ADD(a,b,c) let##a##c (alu_add##c (get##a##c (), get##b##c ()))
#define E_OR(a,b,c)  let##a##c (alu_or##c  (get##a##c (), get##b##c ()))
#define E_ADC(a,b,c) let##a##c (alu_adc##c (get##a##c (), get##b##c ()))
#define E_SBB(a,b,c) let##a##c (alu_sbb##c (get##a##c (), get##b##c ()))
#define E_AND(a,b,c) let##a##c (alu_and##c (get##a##c (), get##b##c ()))
#define E_SUB(a,b,c) let##a##c (alu_sub##c (get##a##c (), get##b##c ()))
#define E_XOR(a,b,c) let##a##c (alu_xor##c (get##a##c (), get##b##c ()))
#define E_CMP(a,b,c) (void)    (alu_sub##c (get##a##c (), get##b##c ()))
#define E_TEST(a,b,c) (void)   (alu_and##c (get##a##c (), get##b##c ()))
#define E_PUSH(a) (letreg2 (4, reg2 (4) + 65534), letmem2 (sregs[2], reg2 (4), get##a##2 ()))
#define E_POP(a) (letreg2 (4, reg2 (4) + 2), let##a##2 (getmem2 (sregs[2], reg2 (4) + 65534)))
#define E_POP2() (letreg2 (4, reg2 (4) + 2), getmem2 (sregs[2], reg2 (4) + 65534))
#define E_DAA() e_daa ()
#define E_DAS() e_das ()
#define E_AAA() e_aaa ()
#define E_AAS() e_aas ()
#define E_INC(a,c) let##a##c (alu_inc##c (get##a##c ()))
#define E_DEC(a,c) let##a##c (alu_dec##c (get##a##c ()))
#define E_JMPIF(a) (tmp = getimm12 (), ((a) ? jmpn (tmp) : (void)0))
#define E_JMPIFc(a) (tmp = getimm12 (), ((a) ? jmpn (tmp), ca(16-12) : ca (4)))
#define E_TABLE1(a,b,c) switch (modrm & 070) { \
case 000: E_ADD (a, b, c); br; \
case 010: E_OR (a, b, c); br; \
case 020: E_ADC (a, b, c); br; \
case 030: E_SBB (a, b, c); br; \
case 040: E_AND (a, b, c); br; \
case 050: E_SUB (a, b, c); br; \
case 060: E_XOR (a, b, c); br; \
case 070: E_CMP (a, b, c); br; \
}
#define E_XCHG(a,b,c) (tmp=get##a##c(),let##a##c(get##b##c()),let##b##c(tmp))
#define E_MOV(a,b,c) let##a##c (get##b##c ())
#define getsregfrommodrm2() sregs[(modrm >> 3) & 3]
#define getsregfrommodrmc2() (ca (1), getsregfrommodrm2 ())
#define letsregfrommodrm2(a) sregs[(modrm >> 3) & 3] = a
#define letsregfrommodrmc2(a) (ca (1), letsregfrommodrm2 (a))
#define E_LEA(a, c) (eamem == 1) ? let##a##c (eaofs) : E_WARN("LEA REG,REG")
#define E_CBW() letreg2 (0, reg12 (0))
#define E_CWD() letreg2 (2, (reg2 (0) & 32768) ? 65535 : 0)
#define E_SAHF() (f_clear (), f_or (reg1 (4)), f_mask (255))
#define E_LAHF() letreg1 (4, f_get ())
#define E_MOVSB() e_movsb ()
#define E_MOVSW() e_movsw ()
#define E_CMPSB() e_cmpsb ()
#define E_CMPSW() e_cmpsw ()
#define E_STOSB() e_stosb ()
#define E_STOSW() e_stosw ()
#define E_LODSB() e_lodsb ()
#define E_LODSW() e_lodsw ()
#define E_SCASB() e_scasb ()
#define E_SCASW() e_scasw ()
#define E_LES() (eamem ? letr2 (getrm2 ()), eaofs += 2, sregs[0] = getrm2 () : E_WARN ("LES REG"))
#define E_LDS() (eamem ? letr2 (getrm2 ()), eaofs += 2, sregs[3] = getrm2 () : E_WARN ("LDS REG"))
#define E_INT(a) e_int (a)
#define E_TABLE2(a,b,c) switch (modrm & 070) { \
case 000: let##a##c (alu_rol##c (get##a##c (), (b))); br; \
case 010: let##a##c (alu_ror##c (get##a##c (), (b))); br; \
case 020: let##a##c (alu_rcl##c (get##a##c (), (b))); br; \
case 030: let##a##c (alu_rcr##c (get##a##c (), (b))); br; \
case 040: let##a##c (alu_shl##c (get##a##c (), (b))); br; \
case 050: let##a##c (alu_shr##c (get##a##c (), (b))); br; \
case 060: let##a##c (alu_shl##c (get##a##c (), (b))); br; \
case 070: let##a##c (alu_sar##c (get##a##c (), (b))); br; \
}
#define E_AAM(a) (a?letreg1(4,reg1(0)/a),letreg1(0,reg1(0)%a),tmp2=reg1(0),f_or(((tmp2>=128)?128:0)|(tmp2?0:64)|ptable[tmp2]),f_mask(196):E_INT(0))
#define E_AAD(a) (letreg1(0,reg1(4)*(a)+reg1(0)),letreg1(4,0),tmp2=reg1(0),f_or(((tmp2>=128)?128:0)|(tmp2?0:64)|ptable[tmp2]),f_mask(196))
#define E_XLAT() (easeg=sregs[segover==4?3:segover],eaofs=(reg2(3)+reg1(0))&65535,eamem=1,letreg1(0,getrm1()))
#define E_ESC() (E_WARN ("ESC"),getmodrm ())
#define E_LOOPNE(a) (letreg2(1,reg2(1)+65535),(reg2(1)&&!(f_get()&64))?jmpn(a):(void)0)
#define E_LOOPNEc(a) (letreg2(1,reg2(1)+65535),(reg2(1)&&!(f_get()&64))?jmpn(a),ca(19-12):ca(5))
#define E_LOOPE(a) (letreg2(1,reg2(1)+65535),(reg2(1)&&(f_get()&64))?jmpn(a):(void)0)
#define E_LOOPEc(a) (letreg2(1,reg2(1)+65535),(reg2(1)&&(f_get()&64))?jmpn(a),ca(18-12):ca(6))
#define E_LOOP(a) (letreg2(1,reg2(1)+65535),reg2(1)?jmpn(a):(void)0)
#define E_LOOPc(a) (letreg2(1,reg2(1)+65535),reg2(1)?jmpn(a),ca(17-12):ca(5))
#define E_JCXZ(a) ((!reg2(1))?jmpn(a):(void)0)
#define E_JCXZc(a) ((!reg2(1))?jmpn(a),ca(18-12):ca(6))
#define E_LOCK() E_WARN ("LOCK")
#define E_REPE() e_repe ()
#define E_REPNE() e_repne ()
#define E_REPEc() e_repec ()
#define E_REPNEc() e_repnec ()
#define E_HLT() E_WARN ("HLT")
#define E_CMC() (f_clear (), f_or ((f_get () & 1) ? 0 : 1), f_mask (1))
#define E_NOT(a,c) let##a##c (get##a##c () ^ (c == 1 ? 255 : 65535))
#define E_NEG(a,c) let##a##c (alu_neg##c (get##a##c ()))
#define E_MUL(c) e_mul##c ()
#define E_IMUL(c) e_imul##c ()
#define E_DIV(c) e_div##c ()
#define E_IDIV(c) e_idiv##c ()
#define E_TABLE3(c) switch (modrm & 070) { \
case 000: E_TEST (rm, imm, c); br; \
case 010: E_WARN ("TABLE3 010"); br; \
case 020: E_NOT (rm, c); br; \
case 030: E_NEG (rm, c); br; \
case 040: E_MUL (c); br; \
case 050: E_IMUL (c); br; \
case 060: E_DIV (c); br; \
case 070: E_IDIV (c); br; \
}
#define E_TABLE3c(c) switch (modrm & 070) { \
case 000: E_TEST (rmc, immc, c); ca (2); br; \
case 010: E_WARN ("TABLE3 010"); br; \
case 020: E_NOT (rmc, c); ca (1); br; \
case 030: E_NEG (rmc, c); ca (1); br; \
case 040: E_MUL (c); ca (eamem ? (c == 1 ? 76 /* ~83 */ : 124 /* ~139 */) : (c == 1 ? 70 /* ~77 */ : 118 /* ~133 */)); br; \
case 050: E_IMUL (c); ca (eamem ? (c == 1 ? 86 /* ~104 */ : 134 /* ~160 */) : (c == 1 ? 80 /* ~98 */ : 128 /* ~154 */)); br; \
case 060: E_DIV (c); ca (eamem ? (c == 1 ? 86 /* ~96 */ : 150 /* ~168 */) : (c == 1 ? 80 /* ~90 */ : 144 /* ~162 */)); br; \
case 070: E_IDIV (c); ca (eamem ? (c == 1 ? 107 /* ~118 */ : 171 /* ~190 */) : (c == 1 ? 101 /* ~112 */ : 165 /* ~184 */)); br; \
}
#define E_CLC() f_mask (1)
#define E_STC() f_or (1), f_mask (1)
#define E_CLI() f_mask (512)
#define E_STI() f_or (512), f_mask (512)
#define E_CLD() f_mask (1024)
#define E_STD() f_or (1024), f_mask (1024)
#define E_TABLE4() switch (modrm & 070) { \
case 000: E_INC (rm, 1); br; \
case 010: E_DEC (rm, 1); br; \
default: E_WARN ("TABLE4 ???"); \
}
#define E_TABLE4c() switch (modrm & 070) { \
case 000: E_INC (rm, 1); ca (eamem ? 15 : 3); br; \
case 010: E_DEC (rm, 1); ca (eamem ? 15 : 3); br; \
default: E_WARN ("TABLE4 ???"); \
}
#define E_TABLE5() switch (modrm & 070) { \
case 000: E_INC (rm, 2); br; \
case 010: E_DEC (rm, 2); br; \
case 020: E_PUSH (ip); jmpna (getrm2 ()); br; /* CALLN */ \
case 030: if(!eamem)E_WARN("CALL FAR REG");E_PUSH(cs);E_PUSH(ip);tmp=getrm2();eaofs+=2;jmpf(getrm2(),tmp);br;/*CALLF*/ \
case 040: jmpna (getrm2 ()); br; /* JMP rm16 */ \
case 050: if(!eamem)E_WARN("JMP FAR REG");tmp=getrm2();eaofs+=2;jmpf(getrm2(),tmp);br;/*JMPF*/ \
case 060: E_PUSH (rm2); br; \
case 070: E_POP (rm2); br; \
}
#define E_TABLE5c() switch (modrm & 070) { \
case 000: E_INC (rm, 2); ca (eamem ? 15 : 3); br; \
case 010: E_DEC (rm, 2); ca (eamem ? 15 : 3); br; \
case 020: E_PUSH (ip); tmp = getrm2 (); jmpna (tmp); ca (eamem?21:16); br; /* CALLN */ \
case 030: if(!eamem)E_WARN("CALL FAR REG");E_PUSH(cs);E_PUSH(ip);tmp=getrm2();eaofs+=2;tmp2=getrm2();jmpf(tmp2,tmp);ca(37);br;/*CALLF*/ \
case 040: tmp = getrm2 (); jmpna (tmp); ca (eamem ? 18 : 11); br; /* JMP rm16 */ \
case 050: if(!eamem)E_WARN("JMP FAR REG");tmp=getrm2();eaofs+=2;tmp2=getrm2();jmpf(tmp2,tmp);ca(24);br;/*JMPF*/ \
case 060: E_PUSH (rm); ca (eamem ? 16 : 10); br; \
case 070: E_POP (rm); ca (eamem ? 17 : 10); br; \
}
#define E_WAIT() E_WARN ("WAIT")

static t16 ptable[] = {
  4,0,0,4,0,4,4,0,0,4,4,0,4,0,0,4,0,4,4,0,4,0,0,4,4,0,0,4,0,4,4,0,
  0,4,4,0,4,0,0,4,4,0,0,4,0,4,4,0,4,0,0,4,0,4,4,0,0,4,4,0,4,0,0,4,
  0,4,4,0,4,0,0,4,4,0,0,4,0,4,4,0,4,0,0,4,0,4,4,0,0,4,4,0,4,0,0,4,
  4,0,0,4,0,4,4,0,0,4,4,0,4,0,0,4,0,4,4,0,4,0,0,4,4,0,0,4,0,4,4,0,
  0,4,4,0,4,0,0,4,4,0,0,4,0,4,4,0,4,0,0,4,0,4,4,0,0,4,4,0,4,0,0,4,
  4,0,0,4,0,4,4,0,0,4,4,0,4,0,0,4,0,4,4,0,4,0,0,4,4,0,0,4,0,4,4,0,
  4,0,0,4,0,4,4,0,0,4,4,0,4,0,0,4,0,4,4,0,4,0,0,4,4,0,0,4,0,4,4,0,
  0,4,4,0,4,0,0,4,4,0,0,4,0,4,4,0,4,0,0,4,0,4,4,0,0,4,4,0,4,0,0,4
};

#define PREFETCHQUEUELEN 4

static t16 flagstmp;
static t16 regs[8], sregs[4], ip, flags;
static t16 modrm, eamem, eaofs, easeg;
static int segover;
static int cycles, cycles_biu;
static int biu_c, biu_f, biu_pf;
static t16 pf_q[PREFETCHQUEUELEN], pf_ri, pf_wi, pf_wip, pf_len;
static t16 biu_data;
static t20 biu_address;
static int f_intr, f_nmi;

extern t16 memory_read (t20, int *);
extern void memory_write (t20, t16, int *);
extern t16 ioport_read (t20, int *);
extern void ioport_write (t20, t16, int *);
extern void interrupt_nmi (void);
extern t16 interrupt_iac (void);

static t16 reg2 (t16);
static t16 getrm1 (void);
static void letreg2 (t16, t16);
static void letrm1 (t16);
static t16 getrm2 (void);
static void letrm2 (t16);
static t16 alu_sub1 (t16, t16);
static t16 alu_sub2 (t16, t16);
static t16 reg1 (t16);
static void letreg1 (t16, t16);
static void jmpf (t16, t16);
static t16 get1 (void);
static t16 get2 (void);
static t16 get12 (void);
static t16 getmem1 (t16, t16);
static t16 getmem2 (t16, t16);
static void letmem1 (t16, t16, t16);
static void letmem2 (t16, t16, t16);
static void e_int (t16);

static void
biu_ct (void)
{
  int i;

  for (; cycles_biu ; cycles_biu--)
    {
      if (!biu_c)
	{
	  biu_pf = 0;
	  if (biu_f)
	    {
	      switch (biu_f)
		{
		case 1:
		  biu_data = memory_read (biu_address, &i);
		  break;
		case 2:
		  memory_write (biu_address, biu_data, &i);
		  break;
		case 3:
		  biu_data = ioport_read (biu_address, &i);
		  break;
		case 4:
		  ioport_write (biu_address, biu_data, &i);
		  break;
		}
	      biu_c += i;
	      biu_f += 10;
	    }
	  else
	    {
	      if (pf_len != PREFETCHQUEUELEN)
		{
		  biu_pf = 1;
		  pf_q[pf_wi] = memory_read ((sregs[1] & 65535) * 16 +
					     (pf_wip & 65535), &i);
		  biu_c += i;
		}
	      else
		biu_c += 1;
	    }
	}
      biu_c--;
      if (!biu_c)
	{
	  if (biu_pf)
	    {
	      pf_wi++;
	      if (pf_wi == PREFETCHQUEUELEN)
		pf_wi = 0;
	      pf_len++;
	      pf_wip++;
	    }
	  if (biu_f > 10)
	    biu_f = 0;
	}
    }
}

static void
cyclesadd (int n)
{
  cycles += n;
  cycles_biu += n;
}

#define ca cyclesadd

static t16
biu_fetch (void)
{
  t16 a;

  biu_ct ();
  while (pf_len < PREFETCHQUEUELEN)//(!pf_len)
    {
      cyclesadd (biu_c ? biu_c : 1);
      biu_ct ();
    }
  pf_len--;
  a = pf_q[pf_ri];
  pf_ri++;
  if (pf_ri == PREFETCHQUEUELEN)
    pf_ri = 0;
  return a;
}

static t16
biu_memr (t20 addr)
{
  biu_ct ();
  biu_f = 1;
  biu_address = addr & 1048575;
  while (biu_f)
    {
      cyclesadd (biu_c ? biu_c : 1);
      biu_ct ();
    }
  return biu_data;
}

static void
biu_memw (t20 addr, t16 value)
{
  biu_ct ();
  biu_f = 2;
  biu_address = addr & 1048575;
  biu_data = value;
  while (biu_f)
    {
      cyclesadd (biu_c ? biu_c : 1);
      biu_ct ();
    }
}

static t16
biu_in (t20 addr)
{
  biu_ct ();
  biu_f = 3;
  biu_address = addr & 65535;
  while (biu_f)
    {
      cyclesadd (biu_c ? biu_c : 1);
      biu_ct ();
    }
  return biu_data;
}

static void
biu_out (t20 addr, t16 value)
{
  biu_ct ();
  biu_f = 4;
  biu_address = addr & 65535;
  biu_data = value;
  while (biu_f)
    {
      cyclesadd (biu_c ? biu_c : 1);
      biu_ct ();
    }
}

static void
biu_clear (void)
{
  biu_ct ();
  biu_pf = 0;
  pf_ri = pf_wi = pf_len = 0;
  pf_wip = ip;
}

static void
f_clear (void)
{
  flagstmp = 0;
}

static void
f_or (t16 v)
{
  flagstmp |= v;
}

static void
f_mask (t16 m)
{
  flags &= m ^ 0xffff;
  flags |= flagstmp & m;
}

static t16
f_get (void)
{
  return flags;
}

static void
e_movsb (void)			/* a4 */
{
  t16 tmp;

  eamem = 1;
  easeg = sregs[(segover == 4) ? 3 : segover];
  eaofs = reg2 (6);
  tmp = getrm1 ();
  letreg2 (6, reg2 (6) + ((f_get () & 1024) ? 65535 : 1));
  eamem = 1;
  easeg = sregs[0];
  eaofs = reg2 (7);
  letrm1 (tmp);
  letreg2 (7, reg2 (7) + ((f_get () & 1024) ? 65535 : 1));
}

static void
e_movsw (void)			/* a5 */
{
  t16 tmp;
  
  eamem = 1;
  easeg = sregs[(segover == 4) ? 3 : segover];
  eaofs = reg2 (6);
  tmp = getrm2 ();
  letreg2 (6, reg2 (6) + ((f_get () & 1024) ? 65534 : 2));
  eamem = 1;
  easeg = sregs[0];
  eaofs = reg2 (7);
  letrm2 (tmp);
  letreg2 (7, reg2 (7) + ((f_get () & 1024) ? 65534 : 2));
}

static void
e_cmpsb (void)			/* a6 */
{
  t16 tmp, tmp2;
  
  eamem = 1;
  easeg = sregs[(segover == 4) ? 3 : segover];
  eaofs = reg2 (6);
  tmp = getrm1 ();
  letreg2 (6, reg2 (6) + ((f_get () & 1024) ? 65535 : 1));
  eamem = 1;
  easeg = sregs[0];
  eaofs = reg2 (7);
  tmp2 = getrm1 ();
  letreg2 (7, reg2 (7) + ((f_get () & 1024) ? 65535 : 1));
  alu_sub1 (tmp, tmp2);
}

static void
e_cmpsw (void)			/* a7 */
{
  t16 tmp, tmp2;

  eamem = 1;
  easeg = sregs[(segover == 4) ? 3 : segover];
  eaofs = reg2 (6);
  tmp = getrm2 ();
  letreg2 (6, reg2 (6) + ((f_get () & 1024) ? 65534 : 2));
  eamem = 1;
  easeg = sregs[0];
  eaofs = reg2 (7);
  tmp2 = getrm2 ();
  letreg2 (7, reg2 (7) + ((f_get () & 1024) ? 65534 : 2));
  alu_sub2 (tmp, tmp2);
}

static void
e_stosb (void)			/* aa */
{
  eamem = 1;
  easeg = sregs[0];
  eaofs = reg2 (7);
  letrm1 (reg1 (0));
  letreg2 (7, reg2 (7) + ((f_get () & 1024) ? 65535 : 1));
}

static void
e_stosw (void)			/* ab */
{
  eamem = 1;
  easeg = sregs[0];
  eaofs = reg2 (7);
  letrm2 (reg2 (0));
  letreg2 (7, reg2 (7) + ((f_get () & 1024) ? 65534 : 2));
}

static void
e_lodsb (void)			/* ac */
{
  eamem = 1;
  easeg = sregs[(segover == 4) ? 3 : segover];
  eaofs = reg2 (6);
  letreg1 (0, getrm1 ());
  letreg2 (6, reg2 (6) + ((f_get () & 1024) ? 65535 : 1));
}

static void
e_lodsw (void)			/* ad */
{
  eamem = 1;
  easeg = sregs[(segover == 4) ? 3 : segover];
  eaofs = reg2 (6);
  letreg2 (0, getrm2 ());
  letreg2 (6, reg2 (6) + ((f_get () & 1024) ? 65534 : 2));
}

static void
e_scasb (void)			/* ae */
{
  t16 tmp2;
  
  eamem = 1;
  easeg = sregs[0];
  eaofs = reg2 (7);
  tmp2 = getrm1 ();
  letreg2 (7, reg2 (7) + ((f_get () & 1024) ? 65535 : 1));
  alu_sub1 (reg1 (0), tmp2);
}

static void
e_scasw (void)			/* af */
{
  t16 tmp2;
  
  eamem = 1;
  easeg = sregs[0];
  eaofs = reg2 (7);
  tmp2 = getrm2 ();
  letreg2 (7, reg2 (7) + ((f_get () & 1024) ? 65534 : 2));
  alu_sub2 (reg2 (0), tmp2);
}

void
reset8088 (void)
{
  jmpf (0xffff, 0);
  f_clear ();
  f_mask (0xffff);
  segover = 4;
  cycles_biu = 0;
  biu_c = 0;
  biu_f = 0;
  biu_clear ();
  f_intr = 0;
  f_nmi = 0;
}

static void
getmodrm (void)
{
  modrm = get1 ();
  eamem = 0;
  if ((modrm & 0xc0) != 0xc0)
    {
      int clocks = 0;

      eamem = 1;
      switch (modrm & 7)
	{
	case 0:
	  eaofs = regs[3] + regs[6];
	  easeg = 3;
	  clocks += 7;
	  break;
	case 1:
	  eaofs = regs[3] + regs[7];
	  easeg = 3;
	  clocks += 7;
	  break;
	case 2:
	  eaofs = regs[5] + regs[6];
	  easeg = 2;
	  clocks += 7;
	  break;
	case 3:
	  eaofs = regs[5] + regs[7];
	  easeg = 2;
	  clocks += 7;
	  break;
	case 4:
	  eaofs = regs[6];
	  easeg = 3;
	  clocks += 5;
	  break;
	case 5:
	  eaofs = regs[7];
	  easeg = 3;
	  clocks += 5;
	  break;
	case 6:
	  eaofs = regs[5];
	  easeg = 2;
	  clocks += 5;
	  break;
	case 7:
	  eaofs = regs[3];
	  easeg = 3;
	  clocks += 5;
	  break;
	}
      if ((modrm & 0xc7) == 6)
	{
	  eaofs = 0;
	  easeg = 3;
	  clocks -= 3;
	}
      if ((modrm & 0xc0) == 0x80 || (modrm & 0xc7) == 6)
	eaofs += get2 (), clocks += 4;
      if ((modrm & 0xc0) == 0x40)
	eaofs += get12 (), clocks += 4;
      eaofs &= 0xffff;
      if (segover != 4)
	easeg = segover;
      easeg = sregs[easeg];
      cyclesadd (clocks);
    }
  else
    eaofs = modrm & 7;
}

static t16
reg1 (t16 n)
{
  if (n >= 4)
    return (regs[n - 4] / 256) & 255;
  return regs[n] & 255;
}

static void
letreg1 (t16 n, t16 v)
{
  if (n >= 4)
    {
      regs[n - 4] &= 255;
      regs[n - 4] |= (v & 255) * 256;
    }
  else
    {
      regs[n] &= 0xff00;
      regs[n] |= v & 255;
    }
}

static t16
reg2 (t16 n)
{
  return regs[n];
}

static void
letreg2 (t16 n, t16 v)
{
  regs[n] = v & 65535;
}

static t16
reg12 (t16 n)
{
  t16 tmp;

  tmp = reg1 (n);
  if (tmp >= 128)
    tmp |= 0xff00;
  return tmp;
}

#if 0				/* Unused */
static t16
sreg2 (t16 n)
{
  return sregs[n];
}
#endif

static t16
getrm1 (void)
{
  if (!eamem)
    return reg1 (eaofs);
  return getmem1 (easeg, eaofs);
}

static t16
getrmc1 (void)
{
  t16 tmp;

  if (!eamem)
    return ca (1), getrm1 ();
  ca (4);
  tmp = getrm1 ();
  ca (3);
  return tmp;
}

static void
letrm1 (t16 v)
{
  if (!eamem)
    letreg1 (eaofs, v);
  else
    letmem1 (easeg, eaofs, v);
}

static void
letrmc1 (t16 v)
{
  if (!eamem)
    ca (1), letrm1 (v);
  else
    ca (4), letrm1 (v), ca (4);
}

static t16
getrm2 (void)
{
  if (!eamem)
    return reg2 (eaofs);
  return getmem2 (easeg, eaofs);
}

static t16
getrmc2 (void)
{
  t16 tmp;

  if (!eamem)
    return ca (1), getrm2 ();
  ca (4);
  tmp = getrm2 ();
  ca (3);
  return tmp;
}

static void
letrm2 (t16 v)
{
  if (!eamem)
    letreg2 (eaofs, v);
  else
    letmem2 (easeg, eaofs, v);      
}

static void
letrmc2 (t16 v)
{
  if (!eamem)
    ca (1), letrm2 (v);
  else
    ca (4), letrm2 (v), ca (4);
}

static t16
getrm12 (void)
{
  t16 tmp;

  tmp = getrm1 ();
  if (tmp >= 128)
    tmp |= 0xff00;
  return tmp;
}

#if 0
static t16
getrmc12 (void)
{
  t16 tmp;

  if (!eamem)
    return ca (1), getrm12 ();
  ca (4);
  tmp = getrm12 ();
  ca (3);
  return tmp;
}
#endif

static t16
getr1 (void)
{
  return reg1 ((modrm / 8) & 7);
}

static t16
getrc1 (void)
{
  return ca (1), getr1 ();
}

static void
letr1 (t16 v)
{
  letreg1 ((modrm / 8) & 7, v);
}

static void
letrc1 (t16 v)
{
  ca (1), letr1 (v);
}

static  t16
getr2 (void)
{
  return reg2 ((modrm / 8) & 7);
}

static t16
getrc2 (void)
{
  return ca (1), getr2 ();
}

static void
letr2 (t16 v)
{
  letreg2 ((modrm / 8) & 7, v);
}

static void
letrc2 (t16 v)
{
  ca (1), letr2 (v);
}

#if 0				/* Unused */
static t16
getr12 (void)
{
  t16 tmp;

  tmp = getr1 ();
  if (tmp >= 128)
    tmp |= 0xff00;
  return tmp;
}

static t16
getrc12 (void)
{
  return ca (1), getr12 ();
}
#endif

static t16
getmem1 (t16 seg, t16 ofs)
{
  return biu_memr ((seg & 65535) * 16 + (ofs & 65535)) & 255;
}

static void
letmem1 (t16 seg, t16 ofs, t16 v)
{
  biu_memw ((seg & 65535) * 16 + (ofs & 65535), v & 255);
}

static t16
getmem2 (t16 seg, t16 ofs)
{
  return getmem1 (seg, ofs) + getmem1 (seg, ofs + 1) * 256;
}

static void
letmem2 (t16 seg, t16 ofs, t16 v)
{
  letmem1 (seg, ofs, v & 255);
  letmem1 (seg, ofs + 1, (v / 256) & 255);
}

static void
jmpf (t16 seg, t16 ofs)
{
  sregs[1] = seg & 65535;
  ip = ofs & 65535;
  biu_clear ();
}

static t16
do_add (t16 op1, t16 op2, t16 carry, t16 mask, t16 sign)
{
  t16 r;
  u32 tmp;

  op1 &= mask;
  op2 &= mask;
  if ((op1 & 0xf) + (op2 & 0xf) + carry > 0xf)
    f_or (16);
  if (op1 + op2 + carry > mask)
    f_or (1);
  /* unsigned calculation */
  /* (32768+x)+(32768+y)+z=65536+x+y+z */
  /* min=65536+(-32768)=32768=sign */
  /* max=65536+32767=32768+65535=sign+mask */
  tmp = (op1 ^ sign) + (op2 ^ sign) + carry;
  if (tmp < sign || tmp > mask + sign)
    f_or (2048);

  r = tmp & mask;
  if (r & sign)
    f_or (128);
  if (r == 0)
    f_or (64);
  f_or (ptable[r & 255]);
  f_mask (2048+16+1+128+64+4);
  return r;
}

static t16
do_sub (t16 op1, t16 op2, t16 carry, t16 mask, t16 sign)
{
  t16 r;
  u32 tmp;

  op1 &= mask;
  op2 &= mask;
  if (0x10 + (op1 & 0xf) - (op2 & 0xf) - carry < 0x10)
    f_or (16);
  if (0x10000 + op1 - op2 - carry < 0x10000)
    f_or (1);
  /* unsigned calculation */
  /* 65536+(32768+x)-(32768+y)-z=65536+x-y-z */
  /* min=65536+(-32768)=32768=sign */
  /* max=65536+32767=32768+65535=sign+mask */
  tmp = mask + 1 + (op1 ^ sign) - (op2 ^ sign) - carry;
  if (tmp < sign || tmp > mask + sign)
    f_or (2048);

  r = tmp & mask;
  if (r & sign)
    f_or (128);
  if (r == 0)
    f_or (64);
  f_or (ptable[r & 255]);
  f_mask (2048+16+1+128+64+4);
  return (t16)r;
}

static t16
alu_add1 (t16 op1, t16 op2)
{
  t16 r;

  INSTDBG_PREPARE_OP (op1 & 255, op2 & 255, 0);
  r = do_add (op1, op2, 0, 0xff, 0x80);
  INSTDBG_TEST_OP ("add %%cl,%%al", r, op2, 0);
  return r;
}

static t16
alu_add2 (t16 op1, t16 op2)
{
  t16 r;

  INSTDBG_PREPARE_OP (op1, op2, 0);
  r = do_add (op1, op2, 0, 0xffff, 0x8000);
  INSTDBG_TEST_OP ("add %%cx,%%ax", r, op2, 0);
  return r;
}

static t16
alu_sub1 (t16 op1, t16 op2)
{
  t16 r;

  INSTDBG_PREPARE_OP (op1 & 255, op2 & 255, 0);
  r = do_sub (op1, op2, 0, 0xff, 0x80);
  INSTDBG_TEST_OP ("sub %%cl,%%al", r, op2, 0);
  return r;
}

static t16
alu_sub2 (t16 op1, t16 op2)
{
  t16 r;

  INSTDBG_PREPARE_OP (op1, op2, 0);
  r = do_sub (op1, op2, 0, 0xffff, 0x8000);
  INSTDBG_TEST_OP ("sub %%cx,%%ax", r, op2, 0);
  return r;
}

static t16
alu_or1 (t16 op1, t16 op2)
{
  t16 r;

  INSTDBG_PREPARE_OP (op1 & 255, op2 & 255, 0);
  op1 &= 255;
  op2 &= 255;
  r = (op1 | op2) & 255;
  if (r >= 128)
    f_or (128);
  if (r == 0)
    f_or (64);
  f_or (ptable[r & 255]);
  f_mask (2048+16+1+128+64+4);
  INSTDBG_TEST_OP ("or %%cl,%%al", r, op2, 0);
  return r;
}

static t16
alu_or2 (t16 op1, t16 op2)
{
  t16 r;

  INSTDBG_PREPARE_OP (op1, op2, 0);
  op1 &= 65535;
  op2 &= 65535;
  r = (op1 | op2) & 65535;
  if (r >= 32768)
    f_or (128);
  if (r == 0)
    f_or (64);
  f_or (ptable[r & 255]);
  f_mask (2048+16+1+128+64+4);
  INSTDBG_TEST_OP ("or %%cx,%%ax", r, op2, 0);
  return r;
}

static t16
alu_adc1 (t16 op1, t16 op2)
{
  t16 r;

  INSTDBG_PREPARE_OP (op1 & 255, op2 & 255, 0);
  r = do_add (op1, op2, f_get () & 1, 0xff, 0x80);
  INSTDBG_TEST_OP ("adc %%cl,%%al", r, op2, 0);
  return r;
}

static t16
alu_adc2 (t16 op1, t16 op2)
{
  t16 r;

  INSTDBG_PREPARE_OP (op1, op2, 0);
  r = do_add (op1, op2, f_get () & 1, 0xffff, 0x8000);
  INSTDBG_TEST_OP ("adc %%cx,%%ax", r, op2, 0);
  return r;
}

static t16
alu_sbb1 (t16 op1, t16 op2)
{
  t16 r;

  INSTDBG_PREPARE_OP (op1 & 255, op2 & 255, 0);
  r = do_sub (op1, op2, f_get () & 1, 0xff, 0x80);
  INSTDBG_TEST_OP ("sbb %%cl,%%al", r, op2, 0);
  return r;
}

static t16
alu_sbb2 (t16 op1, t16 op2)
{
  t16 r;

  INSTDBG_PREPARE_OP (op1, op2, 0);
  r = do_sub (op1, op2, f_get () & 1, 0xffff, 0x8000);
  INSTDBG_TEST_OP ("sbb %%cx,%%ax", r, op2, 0);
  return r;
}

static t16
alu_and1 (t16 op1, t16 op2)
{
  t16 r;

  INSTDBG_PREPARE_OP (op1 & 255, op2 & 255, 0);
  op1 &= 255;
  op2 &= 255;
  r = (op1 & op2) & 255;
  if (r >= 128)
    f_or (128);
  if (r == 0)
    f_or (64);
  f_or (ptable[r & 255]);
  f_mask (2048+16+1+128+64+4);
  INSTDBG_TEST_OP ("and %%cl,%%al", r, op2, 0);
  return r;
}

static t16
alu_and2 (t16 op1, t16 op2)
{
  t16 r;

  INSTDBG_PREPARE_OP (op1, op2, 0);
  op1 &= 65535;
  op2 &= 65535;
  r = (op1 & op2) & 65535;
  if (r >= 32768)
    f_or (128);
  if (r == 0)
    f_or (64);
  f_or (ptable[r & 255]);
  f_mask (2048+16+1+128+64+4);
  INSTDBG_TEST_OP ("and %%cx,%%ax", r, op2, 0);
  return r;
}

static t16
alu_xor1 (t16 op1, t16 op2)
{
  t16 r;

  INSTDBG_PREPARE_OP (op1 & 255, op2 & 255, 0);
  op1 &= 255;
  op2 &= 255;
  r = (op1 ^ op2) & 255;
  if (r >= 128)
    f_or (128);
  if (r == 0)
    f_or (64);
  f_or (ptable[r & 255]);
  f_mask (2048+16+1+128+64+4);
  INSTDBG_TEST_OP ("xor %%cl,%%al", r, op2, 0);
  return r;
}

static t16
alu_xor2 (t16 op1, t16 op2)
{
  t16 r;

  INSTDBG_PREPARE_OP (op1, op2, 0);
  op1 &= 65535;
  op2 &= 65535;
  r = (op1 ^ op2) & 65535;
  if (r >= 32768)
    f_or (128);
  if (r == 0)
    f_or (64);
  f_or (ptable[r & 255]);
  f_mask (2048+16+1+128+64+4);
  INSTDBG_TEST_OP ("xor %%cx,%%ax", r, op2, 0);
  return r;
}

static t16
alu_inc1 (t16 op1)
{
  t16 r;

  INSTDBG_PREPARE_OP (op1 & 255, 0, 0);
  op1 &= 255;
  r = (op1 + 1) & 255;
  if ((op1 & 128) == (1 & 128) && (op1 & 128) != (r & 128))
    f_or (2048);
  if ((op1 & 15) > (r & 15))
    f_or (16);
  if (r >= 128)
    f_or (128);
  if (r == 0)
    f_or (64);
  f_or (ptable[r & 255]);
  f_mask (2048+16+128+64+4);
  INSTDBG_TEST_OP ("inc %%al", r, 0, 0);
  return r;
}

static t16
alu_inc2 (t16 op1)
{
  t16 r;

  INSTDBG_PREPARE_OP (op1, 0, 0);
  op1 &= 65535;
  r = (op1 + 1) & 65535;
  if ((op1 & 32768) == (1 & 32768) && (op1 & 32768) != (r & 32768))
    f_or (2048);
  if ((op1 & 15) > (r & 15))
    f_or (16);
  if (r >= 32768)
    f_or (128);
  if (r == 0)
    f_or (64);
  f_or (ptable[r & 255]);
  f_mask (2048+16+128+64+4);
  INSTDBG_TEST_OP ("inc %%ax", r, 0, 0);
  return r;
}

static t16
alu_dec1 (t16 op1)
{
  t16 r;

  INSTDBG_PREPARE_OP (op1 & 255, 0, 0);
  op1 &= 255;
  r = (256 + op1 - 1) & 255;
  if ((op1 & 128) != (1 & 128) && (op1 & 128) != (r & 128))
    f_or (2048);
  if ((op1 & 15) < (r & 15))
    f_or (16);
  if (r >= 128)
    f_or (128);
  if (r == 0)
    f_or (64);
  f_or (ptable[r & 255]);
  f_mask (2048+16+128+64+4);
  INSTDBG_TEST_OP ("dec %%al", r, 0, 0);
  return r;
}

static t16
alu_dec2 (t16 op1)
{
  t16 r;

  INSTDBG_PREPARE_OP (op1, 0, 0);
  op1 &= 65535;
  r = (65536 + op1 - 1) & 65535;
  if ((op1 & 32768) != (1 & 32768) && (op1 & 32768) != (r & 32768))
    f_or (2048);
  if ((op1 & 15) < (r & 15))
    f_or (16);
  if (r >= 32768)
    f_or (128);
  if (r == 0)
    f_or (64);
  f_or (ptable[r & 255]);
  f_mask (2048+16+128+64+4);
  INSTDBG_TEST_OP ("dec %%ax", r, 0, 0);
  return r;
}

static void
jmpna (t16 ofs)
{
  jmpf (sregs[1], ofs);
}

static void
jmpn (t16 d)
{
  jmpna (ip + d);
}

static t16
alu_rol1 (t16 op1, t16 cl)
{
  t16 tmp, tmp2, c = cl;

  INSTDBG_PREPARE_OP (op1 & 255, cl > 31 ? 31 : cl, 0);
  tmp = op1 & 255;
  if (!cl)
    return tmp;
  while (cl)
    {
      if (tmp & 128)
	tmp2 = 1;
      else
	tmp2 = 0;
      tmp = ((tmp * 2) + tmp2) & 255;
      cl--;
    }
  f_or (tmp & 1);
  f_mask (1);
  if (c == 1)
    {
      if ((op1 & 128) != (tmp & 128))
	f_or (2048);
      f_mask (2048);
    }
  INSTDBG_TEST_OP ("rol %%cl,%%al", tmp, instdbg_before.regs[1], 0);
  return tmp;
}

static t16
alu_ror1 (t16 op1, t16 cl)
{
  t16 tmp, tmp2, c = cl;

  INSTDBG_PREPARE_OP (op1 & 255, cl > 31 ? 31 : cl, 0);
  tmp = op1 & 255;
  if (!cl)
    return tmp;
  while (cl)
    {
      if (tmp & 1)
	tmp2 = 128;
      else
	tmp2 = 0;
      tmp = ((tmp / 2) + tmp2) & 255;
      cl--;
    }
  if (tmp & 128)
    f_or (1);
  f_mask (1);
  if (c == 1)
    {
      if ((op1 & 128) != (tmp & 128))
	f_or (2048);
      f_mask (2048);
    }
  INSTDBG_TEST_OP ("ror %%cl,%%al", tmp, instdbg_before.regs[1], 0);
  return tmp;
}

static t16
alu_rcl1 (t16 op1, t16 cl)
{
  t16 tmp, tmp2, c = cl;

  INSTDBG_PREPARE_OPC (op1 & 255, cl > 31 ? 31 : cl, 0);
  tmp = op1 & 255;
  if (!cl)
    return tmp;
  while (cl)
    {
      f_clear ();
      if (tmp & 128)
	tmp2 = 1;
      else
	tmp2 = 0;
      tmp = ((tmp * 2) + (f_get () & 1)) & 255;
      f_or (tmp2);
      f_mask (1);
      cl--;
    }
  if (c == 1)
    {
      if (((tmp & 128) ? 1 : 0) != (f_get () & 1))
	f_or (2048);
      f_mask (2048);
    }
  INSTDBG_TEST_OP ("rcl %%cl,%%al", tmp, instdbg_before.regs[1], 0);
  return tmp;
}

static t16
alu_rcr1 (t16 op1, t16 cl)
{
  t16 tmp, tmp2;

  INSTDBG_PREPARE_OPC (op1 & 255, cl > 31 ? 31 : cl, 0);
  tmp = op1 & 255;
  if (!cl)
    return tmp;
  if (cl == 1)
    {
      if (((op1 & 128) ? 1 : 0) != (f_get () & 1))
	f_or (2048);
      f_mask (2048);
    }
  while (cl)
    {
      f_clear ();
      if (tmp & 1)
	tmp2 = 1;
      else
	tmp2 = 0;
      tmp = ((tmp / 2) + ((f_get () & 1) ? 128 : 0)) & 255;
      f_or (tmp2);
      f_mask (1);
      cl--;
    }
  INSTDBG_TEST_OP ("rcr %%cl,%%al", tmp, instdbg_before.regs[1], 0);
  return tmp;
}

static t16
alu_shl1 (t16 op1, t16 cl)
{
  t16 tmp, c = cl;

  INSTDBG_PREPARE_OPC (op1 & 255, cl > 31 ? 31 : cl, 0);
  tmp = op1 & 255;
  if (!cl)
    return tmp;
  while (cl)
    {
      f_clear ();
      if (tmp & 128)
	f_or (1);
      f_mask (1);
      tmp = (tmp * 2) & 255;
      cl--;
    }
  if (c == 1)
    {
      if (((tmp & 128) ? 1 : 0) != (f_get () & 1))
	f_or (2048);
      f_mask (2048);
    }
  if (tmp >= 128)
    f_or (128);
  if (tmp == 0)
    f_or (64);
  f_or (ptable[tmp]);
  f_mask (128 + 64 + 4);
  INSTDBG_TEST_OP ("shl %%cl,%%al", tmp, instdbg_before.regs[1], 0);
  return tmp;
}

static t16
alu_shr1 (t16 op1, t16 cl)
{
  t16 tmp, c = cl;

  INSTDBG_PREPARE_OPC (op1 & 255, cl > 31 ? 31 : cl, 0);
  tmp = op1 & 255;
  if (!cl)
    return tmp;
  while (cl)
    {
      f_clear ();
      if (tmp & 1)
	f_or (1);
      f_mask (1);
      tmp = (tmp / 2) & 255;
      cl--;
    }
  if (c == 1)
    {
      if (op1 & 128)
	f_or (2048);
      f_mask (2048);
    }
  if (tmp >= 128)
    f_or (128);
  if (tmp == 0)
    f_or (64);
  f_or (ptable[tmp]);
  f_mask (128 + 64 + 4);
  INSTDBG_TEST_OP ("shr %%cl,%%al", tmp, instdbg_before.regs[1], 0);
  return tmp;
}

static t16
alu_sar1 (t16 op1, t16 cl)
{
  t16 tmp, c = cl;

  INSTDBG_PREPARE_OPC (op1 & 255, cl > 31 ? 31 : cl, 0);
  tmp = op1 & 255;
  if (!cl)
    return tmp;
  while (cl)
    {
      f_clear ();
      if (tmp & 1)
	f_or (1);
      f_mask (1);
      tmp = ((tmp / 2) + ((tmp & 128) ? 128 : 0)) & 255;
      cl--;
    }
  if (c == 1)
    f_mask (2048);
  if (tmp >= 128)
    f_or (128);
  if (tmp == 0)
    f_or (64);
  f_or (ptable[tmp]);
  f_mask (128 + 64 + 4);
  INSTDBG_TEST_OP ("sar %%cl,%%al", tmp, instdbg_before.regs[1], 0);
  return tmp;
}

static t16
alu_rol2 (t16 op1, t16 cl)
{
  t16 tmp, tmp2, c = cl;

  INSTDBG_PREPARE_OP (op1, cl > 31 ? 31 : cl, 0);
  tmp = op1 & 65535;
  if (!cl)
    return tmp;
  while (cl)
    {
      if (tmp & 32768)
	tmp2 = 1;
      else
	tmp2 = 0;
      tmp = ((tmp * 2) + tmp2) & 65535;
      cl--;
    }
  f_or (tmp & 1);
  f_mask (1);
  if (c == 1)
    {
      if ((op1 & 32768) != (tmp & 32768))
	f_or (2048);
      f_mask (2048);
    }
  INSTDBG_TEST_OP ("rol %%cl,%%ax", tmp, instdbg_before.regs[1], 0);
  return tmp;
}

static t16
alu_ror2 (t16 op1, t16 cl)
{
  t16 tmp, tmp2, c = cl;

  INSTDBG_PREPARE_OP (op1, cl > 31 ? 31 : cl, 0);
  tmp = op1 & 65535;
  if (!cl)
    return tmp;
  while (cl)
    {
      if (tmp & 1)
	tmp2 = 32768;
      else
	tmp2 = 0;
      tmp = ((tmp / 2) + tmp2) & 65535;
      cl--;
    }
  if (tmp & 32768)
    f_or (1);
  f_mask (1);
  if (c == 1)
    {
      if ((op1 & 32768) != (tmp & 32768))
	f_or (2048);
      f_mask (2048);
    }
  INSTDBG_TEST_OP ("ror %%cl,%%ax", tmp, instdbg_before.regs[1], 0);
  return tmp;
}

static t16
alu_rcl2 (t16 op1, t16 cl)
{
  t16 tmp, tmp2, c = cl;

  INSTDBG_PREPARE_OPC (op1, cl > 31 ? 31 : cl, 0);
  tmp = op1 & 65535;
  if (!cl)
    return tmp;
  while (cl)
    {
      f_clear ();
      if (tmp & 32768)
	tmp2 = 1;
      else
	tmp2 = 0;
      tmp = ((tmp * 2) + (f_get () & 1)) & 65535;
      f_or (tmp2);
      f_mask (1);
      cl--;
    }
  if (c == 1)
    {
      if (((tmp & 32768) ? 1 : 0) != (f_get () & 1))
	f_or (2048);
      f_mask (2048);
    }
  INSTDBG_TEST_OP ("rcl %%cl,%%ax", tmp, instdbg_before.regs[1], 0);
  return tmp;
}

static t16
alu_rcr2 (t16 op1, t16 cl)
{
  t16 tmp, tmp2;

  INSTDBG_PREPARE_OPC (op1, cl > 31 ? 31 : cl, 0);
  tmp = op1 & 65535;
  if (!cl)
    return tmp;
  if (cl == 1)
    {
      if (((op1 & 32768) ? 1 : 0) != (f_get () & 1))
	f_or (2048);
      f_mask (2048);
    }
  while (cl)
    {
      f_clear ();
      if (tmp & 1)
	tmp2 = 1;
      else
	tmp2 = 0;
      tmp = ((tmp / 2) + ((f_get () & 1) ? 32768 : 0)) & 65535;
      f_or (tmp2);
      f_mask (1);
      cl--;
    }
  INSTDBG_TEST_OP ("rcr %%cl,%%ax", tmp, instdbg_before.regs[1], 0);
  return tmp;
}

static t16
alu_shl2 (t16 op1, t16 cl)
{
  t16 tmp, c = cl;

  INSTDBG_PREPARE_OPC (op1, cl > 31 ? 31 : cl, 0);
  tmp = op1 & 65535;
  if (!cl)
    return tmp;
  while (cl)
    {
      f_clear ();
      if (tmp & 32768)
	f_or (1);
      f_mask (1);
      tmp = (tmp * 2) & 65535;
      cl--;
    }
  if (c == 1)
    {
      if (((tmp & 32768) ? 1 : 0) != (f_get () & 1))
	f_or (2048);
      f_mask (2048);
    }
  if (tmp >= 32768)
    f_or (128);
  if (tmp == 0)
    f_or (64);
  f_or (ptable[tmp & 255]);
  f_mask (128 + 64 + 4);
  INSTDBG_TEST_OP ("shl %%cl,%%ax", tmp, instdbg_before.regs[1], 0);
  return tmp;
}

static t16
alu_shr2 (t16 op1, t16 cl)
{
  t16 tmp, c = cl;

  INSTDBG_PREPARE_OPC (op1, cl > 31 ? 31 : cl, 0);
  tmp = op1 & 65535;
  if (!cl)
    return tmp;
  while (cl)
    {
      f_clear ();
      if (tmp & 1)
	f_or (1);
      f_mask (1);
      tmp = (tmp / 2) & 65535;
      cl--;
    }
  if (c == 1)
    {
      if (op1 & 32768)
	f_or (2048);
      f_mask (2048);
    }
  if (tmp >= 32768)
    f_or (128);
  if (tmp == 0)
    f_or (64);
  f_or (ptable[tmp & 255]);
  f_mask (128 + 64 + 4);
  INSTDBG_TEST_OP ("shr %%cl,%%ax", tmp, instdbg_before.regs[1], 0);
  return tmp;
}

static t16
alu_sar2 (t16 op1, t16 cl)
{
  t16 tmp, c = cl;

  INSTDBG_PREPARE_OPC (op1, cl > 31 ? 31 : cl, 0);
  tmp = op1 & 65535;
  if (!cl)
    return tmp;
  while (cl)
    {
      f_clear ();
      if (tmp & 1)
	f_or (1);
      f_mask (1);
      tmp = ((tmp / 2) + ((tmp & 32768) ? 32768 : 0)) & 65535;
      cl--;
    }
  if (c == 1)
    f_mask (2048);
  if (tmp >= 32768)
    f_or (128);
  if (tmp == 0)
    f_or (64);
  f_or (ptable[tmp & 255]);
  f_mask (128 + 64 + 4);
  INSTDBG_TEST_OP ("sar %%cl,%%ax", tmp, instdbg_before.regs[1], 0);
  return tmp;
}

static t16
in (t16 n)
{
  return biu_in (n & 65535);
}

static void
out (t16 n, t16 v)
{
  biu_out (n & 65535, v & 255);
}

static t16
alu_neg1 (t16 op1)
{
  t16 r;

  INSTDBG_PREPARE_OPC (op1 & 255, 0, 0);
  r = alu_sub1 (0, op1 & 255);
  INSTDBG_TEST_OP ("neg %%al", r, 0, 0);
  return r;
}

static t16
alu_neg2 (t16 op1)
{
  t16 r;

  INSTDBG_PREPARE_OPC (op1, 0, 0);
  r = alu_sub2 (0, op1 & 65535);
  INSTDBG_TEST_OP ("neg %%ax", r, 0, 0);
  return r;
}

static t16
get1 (void)
{
  t16 tmp;

  tmp = biu_fetch ();
  ip++;
  ip &= 65535;
  return tmp;
}

static t16
get2 (void)
{
  t16 tmp;

  tmp = get1 ();
  return tmp + get1 () * 256;
}

static t16
get12 (void)
{
  t16 tmp;

  tmp = get1 ();
  if (tmp >= 128)
    tmp |= 0xff00;
  return tmp;
}

static t16
getacc1 (void)
{
  return reg1 (0);
}

static t16
getaccc1 (void)
{
  return ca (1), getacc1 ();
}

static t16
getacc2 (void)
{
  return reg2 (0);
}

static t16
getaccc2 (void)
{
  return ca (1), getacc2 ();
}

#if 0				/* Unused */
static t16
getacc12 (void)
{
  return reg12 (0);
}

static t16
getaccc12 (void)
{
  return ca (1), getacc12 ();
}
#endif

static void
letacc1 (t16 v)
{
  letreg1 (0, v);
}

static void
letaccc1 (t16 v)
{
  ca (1), letacc1 (v);
}

static void
letacc2 (t16 v)
{
  letreg2 (0, v);
}

static void
letaccc2 (t16 v)
{
  ca (1), letacc2 (v);
}

static t16
getimm1 (void)
{
  return get1 ();
}

static t16
getimmc1 (void)
{
  return ca (2), getimm1 ();
}

static t16
getimm2 (void)
{
  return get2 ();
}

static t16
getimmc2 (void)
{
  return ca (2), getimm2 ();
}

static t16
getimm12 (void)
{
  return get12 ();
}

static t16
getimmc12 (void)
{
  return ca (2), getimm12 ();
}

static void
e_daa (void)
{
  t16 tmp, tmp2;

  INSTDBG_PREPARE_REG ();
  tmp = f_get ();
  if (((reg1 (0) & 15) > 9) || (tmp & 16))
    {
      tmp2 = reg1 (0) + 6;
      if (tmp2 >= 256)
	tmp |= 1;
      letreg1 (0, tmp2);
      tmp |= 16;
    }
  else
    tmp &= 65535 ^ 16;
  if (((reg1 (0) & 0xf0) > 0x90) || (tmp & 1))
    {
      letreg1 (0, reg1 (0) + 0x60);
      tmp |= 1;
    }
  else
    tmp &= 65535 ^ 1;
  f_or (tmp & 17);
  tmp = reg1 (0);
  if (tmp >= 128)
    f_or (128);
  if (tmp == 0)
    f_or (64);
  f_or (ptable[tmp]);
  f_mask (128+64+16+1);
  INSTDBG_TEST_REG ("daa");
}

static void
e_das (void)
{
  t16 tmp, tmp2;

  INSTDBG_PREPARE_REG ();
  tmp = f_get ();
  if (((reg1 (0) & 15) > 9) || (tmp & 16))
    {
      tmp2 = reg1 (0) + 256 - 6;
      if (tmp2 < 256)
	tmp |= 1;
      letreg1 (0, tmp2);
      tmp |= 16;
    }
  else
    tmp &= 65535 ^ 16;
  if ((reg1 (0) > 0x9f) || (tmp & 1))
    {
      letreg1 (0, reg1 (0) + 256 - 0x60);
      tmp |= 1;
    }
  else
    tmp &= 65535 ^ 1;
  f_or (tmp & 17);
  tmp = reg1 (0);
  if (tmp >= 128)
    f_or (128);
  if (tmp == 0)
    f_or (64);
  f_or (ptable[tmp]);
  f_mask (128+64+16+1);
  INSTDBG_TEST_REG ("das");
}

static void
e_aaa (void)
{
  t16 tmp, tmp2;

  INSTDBG_PREPARE_REG ();
  tmp = reg2 (0);
  tmp2 = tmp & 0xff0f;
  tmp &= 15;
  if ((f_get () & 16) || (tmp >= 10 && tmp <= 15))
    {
      tmp2 += 0x106;
      tmp2 &= 0xff0f;
      f_or (17);
    }
  f_mask (17);
  letreg2 (0, tmp2);
  INSTDBG_TEST_REG ("aaa");
}

static void
e_aas (void)
{
  t16 tmp;

  INSTDBG_PREPARE_REG ();
  tmp = reg1 (0);
  if (((tmp & 15) > 9) || (f_get () & 16))
    {
      tmp += 256 - 6;
      letreg1 (4, reg1 (4) + 255);
      f_or (17);
    }
  f_mask (17);
  letreg1 (0, tmp & 15);
  INSTDBG_TEST_REG ("aas");
}

#if 0
static void
e_repe (void)
{
  t16 tmp, tmp2;
  int i;

  tmp = 1;
  for (;;)
    {
      i = 0;
      tmp++;
      switch (tmp2 = get1 ())
	{
	case 0xa4: if (reg2 (1)) E_MOVSB (); br;
	case 0xa5: if (reg2 (1)) E_MOVSW (); br;
	case 0xa6: if (reg2 (1)) E_CMPSB (); i = 2; br;
	case 0xa7: if (reg2 (1)) E_CMPSW (); i = 2; br;
	case 0xaa: if (reg2 (1)) E_STOSB (); br;
	case 0xab: if (reg2 (1)) E_STOSW (); br;
	case 0xac: if (reg2 (1)) E_LODSB (); br;
	case 0xad: if (reg2 (1)) E_LODSW (); br;
	case 0xae: if (reg2 (1)) E_SCASB (); i = 2; br;
	case 0xaf: if (reg2 (1)) E_SCASW (); i = 2; br;
	default: i = 1;
	}
      if (i == 1)
	{
	  if (tmp2 == 0x26)
	    {
	      segover = 0;
	      //clocks += 2;
	      continue;
	    }
	  if (tmp2 == 0x2e)
	    {
	      segover = 1;
	      //clocks += 2;
	      continue;
	    }
	  if (tmp2 == 0x36)
	    {
	      segover = 2;
	      //clocks += 2;
	      continue;
	    }
	  if (tmp2 == 0x3e)
	    {
	      segover = 3;
	      //clocks += 2;
	      continue;
	    }
	  jmpn (65537-tmp);
	  br;
	}
      br;
    }
  if (i == 1)
    return;
  if (!reg2 (1))
    return;
  letreg2 (1, reg2 (1) + 65535);
  if (i == 0)
    {
      if (reg2 (1))
	jmpn (65536-tmp);
    }
  else
    {
      if (reg2 (1) && (f_get () & 64) == 0)
	jmpn (65536-tmp);
    }
}
#endif

static void
e_repec (void)
{
  t16 tmp, tmp2;
  int i;

  tmp = 1;
  for (;;)
    {
      i = 0;
      tmp++;
      switch (tmp2 = get1 ())
	{
	case 0xa4: if (reg2 (1)) E_MOVSB (), ca (17); else ca (9); br;
	case 0xa5: if (reg2 (1)) E_MOVSW (), ca (17); else ca (9); br;
	case 0xa6: if (reg2 (1)) E_CMPSB (), ca (22); else ca (9); i = 2; br;
	case 0xa7: if (reg2 (1)) E_CMPSW (), ca (22); else ca (9); i = 2; br;
	case 0xaa: if (reg2 (1)) E_STOSB (), ca (10); else ca (9); br;
	case 0xab: if (reg2 (1)) E_STOSW (), ca (10); else ca (9); br;
	case 0xac: if (reg2 (1)) E_LODSB (), ca (13); else ca (9); br;
	case 0xad: if (reg2 (1)) E_LODSW (), ca (13); else ca (9); br;
	case 0xae: if (reg2 (1)) E_SCASB (), ca (15); else ca (9); i = 2; br;
	case 0xaf: if (reg2 (1)) E_SCASW (), ca (15); else ca (9); i = 2; br;
	default: i = 1;
	}
      if (i == 1)
	{
	  if (tmp2 == 0x26)
	    {
	      segover = 0;
	      ca (2);
	      continue;
	    }
	  if (tmp2 == 0x2e)
	    {
	      segover = 1;
	      ca (2);
	      continue;
	    }
	  if (tmp2 == 0x36)
	    {
	      segover = 2;
	      ca (2);
	      continue;
	    }
	  if (tmp2 == 0x3e)
	    {
	      segover = 3;
	      ca (2);
	      continue;
	    }
	  jmpn (65537-tmp);
	  br;
	}
      br;
    }
  if (i == 1)
    return;
  if (!reg2 (1))
    return;
  letreg2 (1, reg2 (1) + 65535);
  if (i == 0)
    {
      if (reg2 (1))
	jmpn (65536-tmp);
    }
  else
    {
      if (reg2 (1) && (f_get () & 64) == 0)
	jmpn (65536-tmp);
    }
}

#if 0
static void
e_repne (void)
{
  t16 tmp, tmp2;
  int i;

  tmp = 1;
  for (;;)
    {
      i = 0;
      tmp++;
      switch (tmp2 = get1 ())
	{
	case 0xa4: if (reg2 (1)) E_MOVSB (); br;
	case 0xa5: if (reg2 (1)) E_MOVSW (); br;
	case 0xa6: if (reg2 (1)) E_CMPSB (); i = 2; br;
	case 0xa7: if (reg2 (1)) E_CMPSW (); i = 2; br;
	case 0xaa: if (reg2 (1)) E_STOSB (); br;
	case 0xab: if (reg2 (1)) E_STOSW (); br;
	case 0xac: if (reg2 (1)) E_LODSB (); br;
	case 0xad: if (reg2 (1)) E_LODSW (); br;
	case 0xae: if (reg2 (1)) E_SCASB (); i = 2; br;
	case 0xaf: if (reg2 (1)) E_SCASW (); i = 2; br;
	default: i = 1;
	}
      if (i == 1)
	{
	  if (tmp2 == 0x26)
	    {
	      segover = 0;
	      //clocks += 2;
	      continue;
	    }
	  if (tmp2 == 0x2e)
	    {
	      segover = 1;
	      //clocks += 2;
	      continue;
	    }
	  if (tmp2 == 0x36)
	    {
	      segover = 2;
	      //clocks += 2;
	      continue;
	    }
	  if (tmp2 == 0x3e)
	    {
	      segover = 3;
	      //clocks += 2;
	      continue;
	    }
	  jmpn (65537-tmp);
	  br;
	}
      br;
    }
  if (i == 1)
    return;
  if (!reg2 (1))
    return;
  letreg2 (1, reg2 (1) + 65535);
  if (i == 0)
    {
      if (reg2 (1))
	jmpn (65536-tmp);
    }
  else
    {
      if (reg2 (1) && (f_get () & 64))
	jmpn (65536-tmp);
    }
}
#endif

static void
e_repnec (void)
{
  t16 tmp, tmp2;
  int i;

  tmp = 1;
  for (;;)
    {
      i = 0;
      tmp++;
      switch (tmp2 = get1 ())
	{
	case 0xa4: if (reg2 (1)) E_MOVSB (), ca (17); else ca (9); br;
	case 0xa5: if (reg2 (1)) E_MOVSW (), ca (17); else ca (9); br;
	case 0xa6: if (reg2 (1)) E_CMPSB (), ca (22); else ca (9); i = 2; br;
	case 0xa7: if (reg2 (1)) E_CMPSW (), ca (22); else ca (9); i = 2; br;
	case 0xaa: if (reg2 (1)) E_STOSB (), ca (10); else ca (9); br;
	case 0xab: if (reg2 (1)) E_STOSW (), ca (10); else ca (9); br;
	case 0xac: if (reg2 (1)) E_LODSB (), ca (13); else ca (9); br;
	case 0xad: if (reg2 (1)) E_LODSW (), ca (13); else ca (9); br;
	case 0xae: if (reg2 (1)) E_SCASB (), ca (15); else ca (9); i = 2; br;
	case 0xaf: if (reg2 (1)) E_SCASW (), ca (15); else ca (9); i = 2; br;
	default: i = 1;
	}
      if (i == 1)
	{
	  if (tmp2 == 0x26)
	    {
	      segover = 0;
	      ca (2);
	      continue;
	    }
	  if (tmp2 == 0x2e)
	    {
	      segover = 1;
	      ca (2);
	      continue;
	    }
	  if (tmp2 == 0x36)
	    {
	      segover = 2;
	      ca (2);
	      continue;
	    }
	  if (tmp2 == 0x3e)
	    {
	      segover = 3;
	      ca (2);
	      continue;
	    }
	  jmpn (65537-tmp);
	  br;
	}
      br;
    }
  if (i == 1)
    return;
  if (!reg2 (1))
    return;
  letreg2 (1, reg2 (1) + 65535);
  if (i == 0)
    {
      if (reg2 (1))
	jmpn (65536-tmp);
    }
  else
    {
      if (reg2 (1) && (f_get () & 64))
	jmpn (65536-tmp);
    }
}

static void
e_mul1 (void)
{
#if 0
  unsigned int b, c, d;

  b = a & 255;
  c = reg1 (0);
  d = (b * c) & 65535;
  letreg2 (0, d);
  if (d & 0xff00)
    f_or (2049);
  f_mask (2049);
#else
  t16 ax, bx, cx, dx;

  INSTDBG_PREPARE_OPC (reg1 (0), getrm1 (), 0);
  bx = reg1 (0);
  cx = getrm1 ();
  ax = 0;
  for (dx = 8; dx > 0; dx--)
    {
      bx = alu_shr2 (bx, 1);
      if (f_get () & 1)
	ax = alu_add2 (ax, cx);
      cx = alu_shl2 (cx, 1);
      f_clear ();
    }
  letreg2 (0, ax);
  if (ax & 0xff00)
    f_or (2049);
  f_mask (2049);
  INSTDBG_TEST_OPF ("mul %%cl", reg2 (0), instdbg_before.regs[1], 0, 2049);
#endif
}

static void
e_mul2 (void)
{
#if 0
  unsigned long b, c, d;

  b = a & 65535;
  c = reg2 (0);
  d = (b * c) & 4294967295UL;
  letreg2 (0, d & 65535);
  d /= 65536UL;
  letreg2 (3, d);
  if (d)
    f_or (2049);
  f_mask (2049);
#else
  t16 ax, bx, cx, dx, si, di;

  INSTDBG_PREPARE_OPC (reg2 (0), getrm2 (), 0);
  bx = getrm2 ();
  cx = 0;
  ax = 0;
  dx = 0;
  si = reg2 (0);
  for (di = 16; di > 0; di--)
    {
      si = alu_shr2 (si, 1);
      f_clear ();
      if (f_get () & 1)
	{
	  ax = alu_add2 (ax, bx);
	  dx = alu_adc2 (dx, cx);
	  f_clear ();
	}
      bx = alu_shl2 (bx, 1);
      cx = alu_rcl2 (cx, 1);
      f_clear ();
    }
  letreg2 (0, ax);
  letreg2 (2, dx);
  if (dx)
    f_or (2049);
  f_mask (2049);
  INSTDBG_TEST_OPF ("mul %%cx", reg2 (0), instdbg_before.regs[1], reg2 (2),
		    2049);
#endif
}

static void
e_imul1 (void)
{
#if 0
  unsigned int b, c, d;
  int f = 0;

  b = a & 255;
  c = reg1 (0);
  if (b >= 128)
    b = ((b ^ 255) + 1) & 255, f ^= 1;
  if (c >= 128)
    c = ((c ^ 255) + 1) & 255, f ^= 1;
  d = (b * c) & 32767;
  if (f)
    d = ((d ^ 65535) + 1) & 65535;
  letreg2 (0, d);
  if ((d & 0xff80) != 0xff80 && (ax & 0xff80) != 0)
    f_or (2049);
  f_mask (2049);
#else
  t16 ax, bx, cx, dx;

  INSTDBG_PREPARE_OPC (reg1 (0), getrm1 (), 0);
  bx = reg12 (0);
  cx = getrm12 ();
  ax = 0;
  for (dx = 16; dx > 0; dx--)
    {
      bx = alu_sar2 (bx, 1);
      if (f_get () & 1)
	ax = alu_add2 (ax, cx);
      cx = alu_shl2 (cx, 1);
      f_clear ();
    }
  letreg2 (0, ax);
  if ((ax & 0xff80) != 0xff80 && (ax & 0xff80) != 0)
    f_or (2049);
  f_mask (2049);
  INSTDBG_TEST_OPF ("imul %%cl", reg2 (0), instdbg_before.regs[1], 0, 2049);
#endif
}

static void
e_imul2 (void)
{
#if 0
  unsigned long b, c, d;

  b = a & 65535;
  c = reg2 (0);
  if (b >= 32768)
    b = ((b ^ 65535) + 1) & 65535, f ^= 1;
  if (c >= 32768)
    c = ((c ^ 65535) + 1) & 65535, f ^= 1;
  d = (b * c) & 2147483647UL;
  if (f)
    d = ((d ^ 4294967295UL) + 1) & 4294967295UL;
  b = d % 65536UL;
  d /= 65536UL;
  letreg2 (0, b);
  letreg2 (3, d);
  if (!(((b & 32768) && d == 65535) || ((b & 32768) == 0 && d == 0)))
    f_or (2049);
  f_mask (2049);
#else
  t16 ax, bx, cx, dx, si, di;

  INSTDBG_PREPARE_OPC (reg2 (0), getrm2 (), 0);
  ax = getrm2 ();
  if (ax & 0x8000)
    dx = 65535;
  else
    dx = 0;
  bx = ax;
  cx = dx;
  ax = 0;
  dx = 0;
  si = reg2 (0);
  for (di = 32; di > 0; di--)
    {
      si = alu_sar2 (si, 1);
      f_clear ();
      if (f_get () & 1)
	{
	  ax = alu_add2 (ax, bx);
	  dx = alu_adc2 (dx, cx);
	  f_clear ();
	}
      bx = alu_shl2 (bx, 1);
      cx = alu_rcl2 (cx, 1);
      f_clear ();
    }
  letreg2 (0, ax);
  letreg2 (2, dx);
  if (!(((ax & 32768) && dx == 65535) ||
	((ax & 32768) == 0 && dx == 0)))
    f_or (2049);
  f_mask (2049);
  INSTDBG_TEST_OPF ("imul %%cx", reg2 (0), instdbg_before.regs[1], reg2 (2),
		    2049);
#endif
}

static void
e_int (t16 a)
{
  E_PUSH (flags);
  E_PUSH (cs);
  E_PUSH (ip);
  jmpf (getmem2 (0, a * 4 + 2), getmem2 (0, a * 4));
  f_clear ();
  f_mask (512 + 256);
}

static void
e_div1 (void)
{
#if 0
  unsigned int b, c, d, e;

  b = reg2 (0);
  c = a & 255;
  if (!c)
    {
      E_INT (0);
      return;
    }
  d = b / c;
  e = b % c;
  if (d >= 256)
    {
      E_INT (0);
      return;
    }
  letreg1 (0, d & 255);
  letreg1 (4, e & 255);
#else
  t16 ax, bx, cx, dx;
  int er = 0;

  INSTDBG_PREPARE_OPC (reg2 (0), getrm1 (), 0);
  dx = 0;
  cx = 8;
  ax = getrm1 () * 256;
  bx = reg2 (0);
  alu_sub2 (bx, ax);
  f_clear ();
  if ((f_get () & 1) == 0)
    er = 1;
  else
    {
      for (; cx > 0; cx--)
	{
	  ax = alu_shr2 (ax, 1);
	  dx = alu_shl2 (dx, 1);
	  f_clear ();
	  alu_sub2 (bx, ax);
	  if (!(f_get () & 1))
	    {
	      bx = alu_sub2 (bx, ax);
	      dx = alu_or2 (dx, 1);
	    }
	}
      if (bx & 0xff00)
	er = 1;
      else
	letreg2 (0, bx * 256 + dx);
    }
  if (er)
    {
      E_INT (0);
    }
  INSTDBG_TEST_OPF ("div %%cl", reg2 (0), instdbg_before.regs[1], 0, 0);
#endif
}

static void
e_div2 (void)
{
#if 0
  unsigned long b, c, d, e;

  b = reg2 (3);
  b *= 65536UL;
  b += reg2 (0);
  c = a & 65535;
  if (!c)
    {
      E_INT (0);
      return;
    }
  d = b / c;
  e = b % c;
  if (d >= 65536)
    {
      E_INT (0);
      return;
    }
  letreg2 (0, d & 65535);
  letreg2 (2, e & 65535);
#else
  t16 ax, bx, cx, dx, si;
  int er = 0;

  INSTDBG_PREPARE_OPC (reg2 (0), getrm2 (), reg2 (2));
  do
    {
      ax = reg2 (0);
      bx = reg2 (2);
      cx = getrm2 ();
      alu_sub2 (bx, cx);
      f_clear ();
      if (!(f_get () & 1))
	{
	  er = 1;
	  br;
	}
      si = 0;
      for (dx = 16; dx > 0; dx--)
	{
	  si = alu_shl2 (si, 1);
	  f_clear ();
	  ax = alu_shl2 (ax, 1);
	  f_clear ();
	  bx = alu_rcl2 (bx, 1);
	  f_clear ();
	  if (f_get () & 1)
	    {
	      bx = alu_sub2 (bx, cx);
	      f_clear ();
	      if (!(f_get () & 1))
		{
		  er = 1;
		  br;
		}
	      si |= 1;
	    }
	  else
	    {
	      alu_sub2 (bx, cx);
	      f_clear ();
	      if (!(f_get () & 1))
		{
		  bx = alu_sub2 (bx, cx);
		  f_clear ();
		  si |= 1;
		}
	    }
	}
      if (er)
	br;
      letreg2 (0, si);
      letreg2 (2, bx);
    }
  while (0);
  if (er)
    {
      E_INT (0);
    }
  INSTDBG_TEST_OPF ("div %%cx", reg2 (0), instdbg_before.regs[1], reg2 (2), 0);
#endif
}

static void
e_idiv1 (void)
{
  t16 ax, bx, cx, dx, di;
  int er = 0;

  INSTDBG_PREPARE_OPC (reg2 (0), getrm1 (), 0);
  dx = 0;
  cx = 8;
  ax = getrm1 () * 256;
  bx = reg2 (0);
  di = 0;
  if (bx & 0x8000)
    {
      di ^= 3;
      bx = alu_neg2 (bx);
      f_clear ();
    }
  if (ax & 0x8000)
    {
      di ^= 1;
      ax ^= 0xff00;
      ax = alu_add2 (ax, 256);
      f_clear ();
    }
  alu_sub2 (bx, ax);
  f_clear ();
  if ((f_get () & 1) == 0)
    er = 1;
  else
    {
      for (; cx > 0; cx--)
	{
	  ax = alu_shr2 (ax, 1);
	  dx = alu_shl2 (dx, 1);
	  f_clear ();
	  alu_sub2 (bx, ax);
	  if (!(f_get () & 1))
	    {
	      bx = alu_sub2 (bx, ax);
	      dx = alu_or2 (dx, 1);
	    }
	}
      if (bx & 0xff00)
	er = 1;
      else
	{
	  bx &= 255;
	  if (bx & 128)
	    bx |= 0xff00;
	  f_clear ();
	  bx = alu_neg2 (bx);
	  if ((!(f_get () & 64)) && (!(f_get () & 128)))
	    er = 1;
	  else
	    {
	      dx &= 255;
	      if (dx & 128)
		dx |= 0xff00;
	      f_clear ();
	      dx = alu_neg2 (dx);
	      if ((!(f_get () & 64)) &&
		  (!(f_get () & 128)))
		er = 1;
	      else
		{
		  if (!(di & 1))
		    {
		      f_clear ();
		      dx = alu_neg2 (dx);
		      if (f_get () & 128)
			er = 1;
		    }
		  if (!(di & 2))
		    {
		      f_clear ();
		      bx = alu_neg2 (bx);
		      if (f_get () & 128)
			er = 1;
		    }
		}
	    }
	}
    }
  if (er)
    {
      E_INT (0);
    }
  else
    {
      bx &= 255;
      dx &= 255;
      letreg2 (0, bx * 256 + dx);
    }
  INSTDBG_TEST_OPF ("idiv %%cl", reg2 (0), instdbg_before.regs[1], 0, 0);
}

static void
e_idiv2 (void)
{
  t16 ax, bx, cx, dx, si, di;
  int er = 0;

  INSTDBG_PREPARE_OPC (reg2 (0), getrm2 (), reg2 (2));
  ax = reg2 (0);
  bx = reg2 (2);
  cx = getrm2 ();
  do
    {
      di = 0;
      if (bx & 0x8000)
	{
	  di ^= 3;
	  bx ^= 65535;
	  ax ^= 65535;
	  ax = alu_add2 (ax, 1);
	  f_clear ();
	  bx = alu_adc2 (bx, 0);
	  f_clear ();
	}
      if (cx & 0x8000)
	{
	  di ^= 1;
	  cx = alu_neg2 (cx);
	  f_clear ();
	}
      alu_sub2 (bx, cx);
      f_clear ();
      if (!(f_get () & 1))
	{
	  er = 1;
	  br;
	}
      si = 0;
      for (dx = 16; dx > 0; dx--)
	{
	  si = alu_shl2 (si, 1);
	  f_clear ();
	  ax = alu_shl2 (ax, 1);
	  f_clear ();
	  bx = alu_rcl2 (bx, 1);
	  f_clear ();
	  if (f_get () & 1)
	    {
	      bx = alu_sub2 (bx, cx);
	      f_clear ();
	      if (!(f_get () & 1))
		{
		  er = 1;
		  br;
		}
	      si |= 1;
	    }
	  else
	    {
	      alu_sub2 (bx, cx);
	      f_clear ();
	      if (!(f_get () & 1))
		{
		  bx = alu_sub2 (bx, cx);
		  f_clear ();
		  si |= 1;
		}
	    }
	}
      if (er)
	br;
      si = alu_neg2 (si);
      f_clear ();
      if ((!(f_get () & 64)) && (!(f_get () & 128)))
	{
	  er = 1;
	  br;
	}
      bx = alu_neg2 (bx);
      f_clear ();
      if ((!(f_get () & 64)) && (!(f_get () & 128)))
	{
	  er = 1;
	  br;
	}
      if (!(di & 1))
	{
	  si = alu_neg2 (si);
	  f_clear ();
	  if (f_get () & 128)
	    {
	      er = 1;
	      br;
	    }
	}
      if (!(di & 2))
	{
	  bx = alu_neg2 (bx);
	  f_clear ();
	  if (f_get () & 128)
	    {
	      er = 1;
	      br;
	    }
	}
      letreg2 (0, si);
      letreg2 (2, bx);
    }
  while (0);
  if (er)
    {
      E_INT (0);
    }
  INSTDBG_TEST_OPF ("idiv %%cx", reg2 (0), instdbg_before.regs[1], reg2 (2),
		    0);
}

int
run8088 (void)
{
  t16 opcode, tmp, tmp2;
  int llll = 0;
  int tf;

  cycles = 0;
  if (f_nmi)
    interrupt_nmi (), E_INT (2);
  if (f_intr && (f_get () & 512))
    E_INT (interrupt_iac ());
  tf = (f_get () & 256) ? 1 : 0;
  for (;;)
    {
      f_clear ();
      opcode = get1 ();
      switch (opcode)
	{
	case 0x00: getmodrm (); E_ADD (rmc, rc, 1); br;
	case 0x01: getmodrm (); E_ADD (rmc, rc, 2); br;
	case 0x02: getmodrm (); E_ADD (rc, rmc, 1); br;
	case 0x03: getmodrm (); E_ADD (rc, rmc, 2); br;
	case 0x04: E_ADD (accc, immc, 1); br;
	case 0x05: E_ADD (accc, immc, 2); br;
	case 0x06: E_PUSH (es); ca (10); br;
	case 0x07: E_POP (es); ca (8); llll += 2; br;
	case 0x08: getmodrm (); E_OR (rmc, rc, 1); br;
	case 0x09: getmodrm (); E_OR (rmc, rc, 2); br;
	case 0x0a: getmodrm (); E_OR (rc, rmc, 1); br;
	case 0x0b: getmodrm (); E_OR (rc, rmc, 2); br;
	case 0x0c: E_OR (accc, immc, 1); br;
	case 0x0d: E_OR (accc, immc, 2); br;
	case 0x0e: E_PUSH (cs); ca (10); br;
	case 0x0f: E_POP (cs); ca (8); llll += 2; E_WARN ("POP CS"); br;
	case 0x10: getmodrm (); E_ADC (rmc, rc, 1); br;
	case 0x11: getmodrm (); E_ADC (rmc, rc, 2); br;
	case 0x12: getmodrm (); E_ADC (rc, rmc, 1); br;
	case 0x13: getmodrm (); E_ADC (rc, rmc, 2); br;
	case 0x14: E_ADC (accc, immc, 1); br;
	case 0x15: E_ADC (accc, immc, 2); br;
	case 0x16: E_PUSH (ss); ca (10); br;
	case 0x17: E_POP (ss); ca (8); llll += 2; br;
	case 0x18: getmodrm (); E_SBB (rmc, rc, 1); br;
	case 0x19: getmodrm (); E_SBB (rmc, rc, 2); br;
	case 0x1a: getmodrm (); E_SBB (rc, rmc, 1); br;
	case 0x1b: getmodrm (); E_SBB (rc, rmc, 2); br;
	case 0x1c: E_SBB (accc, immc, 1); br;
	case 0x1d: E_SBB (accc, immc, 2); br;
	case 0x1e: E_PUSH (ds); ca (10); br;
	case 0x1f: E_POP (ds); ca (8); llll += 2; br;
	case 0x20: getmodrm (); E_AND (rmc, rc, 1); br;
	case 0x21: getmodrm (); E_AND (rmc, rc, 2); br;
	case 0x22: getmodrm (); E_AND (rc, rmc, 1); br;
	case 0x23: getmodrm (); E_AND (rc, rmc, 2); br;
	case 0x24: E_AND (accc, immc, 1); br;
	case 0x25: E_AND (accc, immc, 2); br;
	case 0x26: br;		/* ES: */
	case 0x27: E_DAA (); ca (4); br;
	case 0x28: getmodrm (); E_SUB (rmc, rc, 1); br;
	case 0x29: getmodrm (); E_SUB (rmc, rc, 2); br;
	case 0x2a: getmodrm (); E_SUB (rc, rmc, 1); br;
	case 0x2b: getmodrm (); E_SUB (rc, rmc, 2); br;
	case 0x2c: E_SUB (accc, immc, 1); br;
	case 0x2d: E_SUB (accc, immc, 2); br;
	case 0x2e: br;		/* CS: */
	case 0x2f: E_DAS (); ca (4); br;
	case 0x30: getmodrm (); E_XOR (rmc, rc, 1); br;
	case 0x31: getmodrm (); E_XOR (rmc, rc, 2); br;
	case 0x32: getmodrm (); E_XOR (rc, rmc, 1); br;
	case 0x33: getmodrm (); E_XOR (rc, rmc, 2); br;
	case 0x34: E_XOR (accc, immc, 1); br;
	case 0x35: E_XOR (accc, immc, 2); br;
	case 0x36: br;		/* SS: */
	case 0x37: E_AAA (); ca (4); br;
	case 0x38: getmodrm (); E_CMP (rmc, rc, 1); br;
	case 0x39: getmodrm (); E_CMP (rmc, rc, 2); br;
	case 0x3a: getmodrm (); E_CMP (rc, rmc, 1); br;
	case 0x3b: getmodrm (); E_CMP (rc, rmc, 2); br;
	case 0x3c: E_CMP (accc, immc, 1); br;
	case 0x3d: E_CMP (accc, immc, 2); br;
	case 0x3e: br;		/* DS: */
	case 0x3f: E_AAS (); ca (4); br;
	case 0x40: E_INC (ax, 2); ca (2); br;
	case 0x41: E_INC (cx, 2); ca (2); br;
	case 0x42: E_INC (dx, 2); ca (2); br;
	case 0x43: E_INC (bx, 2); ca (2); br;
	case 0x44: E_INC (sp, 2); ca (2); br;
	case 0x45: E_INC (bp, 2); ca (2); br;
	case 0x46: E_INC (si, 2); ca (2); br;
	case 0x47: E_INC (di, 2); ca (2); br;
	case 0x48: E_DEC (ax, 2); ca (2); br;
	case 0x49: E_DEC (cx, 2); ca (2); br;
	case 0x4a: E_DEC (dx, 2); ca (2); br;
	case 0x4b: E_DEC (bx, 2); ca (2); br;
	case 0x4c: E_DEC (sp, 2); ca (2); br;
	case 0x4d: E_DEC (bp, 2); ca (2); br;
	case 0x4e: E_DEC (si, 2); ca (2); br;
	case 0x4f: E_DEC (di, 2); ca (2); br;
	case 0x50: E_PUSH (ax); ca (10); br;
	case 0x51: E_PUSH (cx); ca (10); br;
	case 0x52: E_PUSH (dx); ca (10); br;
	case 0x53: E_PUSH (bx); ca (10); br;
	case 0x54: E_PUSH (sp); ca (10); br;
	case 0x55: E_PUSH (bp); ca (10); br;
	case 0x56: E_PUSH (si); ca (10); br;
	case 0x57: E_PUSH (di); ca (10); br;
	case 0x58: E_POP (ax); ca (8); br;
	case 0x59: E_POP (cx); ca (8); br;
	case 0x5a: E_POP (dx); ca (8); br;
	case 0x5b: E_POP (bx); ca (8); br;
	case 0x5c: E_POP (sp); ca (8); br;
	case 0x5d: E_POP (bp); ca (8); br;
	case 0x5e: E_POP (si); ca (8); br;
	case 0x5f: E_POP (di); ca (8); br;
	case 0x70: E_JMPIFc (f_get () & 2048); br;
	case 0x71: E_JMPIFc (!(f_get () & 2048)); br;
	case 0x72: E_JMPIFc (f_get () & 1); br;
	case 0x73: E_JMPIFc (!(f_get () & 1)); br;
	case 0x74: E_JMPIFc (f_get () & 64); br;
	case 0x75: E_JMPIFc (!(f_get () & 64)); br;
	case 0x76: E_JMPIFc (f_get () & 65); br;
	case 0x77: E_JMPIFc (!(f_get () & 65)); br;
	case 0x78: E_JMPIFc (f_get () & 128); br;
	case 0x79: E_JMPIFc (!(f_get () & 128)); br;
	case 0x7a: E_JMPIFc (f_get () & 4); br;
	case 0x7b: E_JMPIFc (!(f_get () & 4)); br;
	case 0x7c: E_JMPIFc ((tmp2=f_get()&2176,(tmp2==2048||tmp2==128)));br;
	case 0x7d: E_JMPIFc ((tmp2=f_get()&2176,(tmp2==0||tmp2==2176)));br;
	case 0x7e: E_JMPIFc ((tmp2=f_get()&2240,(((tmp2&2176)==2048||(tmp2&2176)==128)||(tmp2&64)==64)));br;
	case 0x7f: E_JMPIFc ((tmp2=f_get()&2240,(((tmp2&2176)==0||(tmp2&2176)==2176)&&(tmp2&64)==0)));br;
	case 0x80: getmodrm (); E_TABLE1 (rmc, immc, 1); br;
	case 0x81: getmodrm (); E_TABLE1 (rmc, immc, 2); br;
	case 0x82: getmodrm (); E_TABLE1 (rmc, immc, 1); br;
	case 0x83: getmodrm (); E_TABLE1 (rmc, immc1, 2); br;
	case 0x84: getmodrm (); E_TEST (rc, rmc, 1); ca (1); br;
	case 0x85: getmodrm (); E_TEST (rc, rmc, 2); ca (1); br;
	case 0x86: getmodrm (); E_XCHG (rc, rmc, 1); br;
	case 0x87: getmodrm (); E_XCHG (rc, rmc, 2); br;
	case 0x88: getmodrm (); E_MOV (rmc, rc, 1); br;
	case 0x89: getmodrm (); E_MOV (rmc, rc, 2); br;
	case 0x8a: getmodrm (); E_MOV (rc, rmc, 1); br;
	case 0x8b: getmodrm (); E_MOV (rc, rmc, 2); br;
	case 0x8c: getmodrm (); E_MOV (rmc, sregfrommodrmc, 2); br;
	case 0x8d: getmodrm (); E_LEA (r, 2); ca (2); br;
	case 0x8e: getmodrm (); E_MOV (sregfrommodrmc, rmc, 2); llll += 2; br;
	case 0x8f: getmodrm (); E_POP (rmc); ca (eamem ? 9 : 7); br;
	case 0x90: E_XCHG (ax, ax, 2); ca (3); br;
	case 0x91: E_XCHG (ax, cx, 2); ca (3); br;
	case 0x92: E_XCHG (ax, dx, 2); ca (3); br;
	case 0x93: E_XCHG (ax, bx, 2); ca (3); br;
	case 0x94: E_XCHG (ax, sp, 2); ca (3); br;
	case 0x95: E_XCHG (ax, bp, 2); ca (3); br;
	case 0x96: E_XCHG (ax, si, 2); ca (3); br;
	case 0x97: E_XCHG (ax, di, 2); ca (3); br;
	case 0x98: E_CBW (); ca (2); br;
	case 0x99: E_CWD (); ca (5); br;
	case 0x9a: tmp=getimm2();tmp2=getimm2();E_PUSH(cs);E_PUSH(ip);jmpf(tmp2,tmp);ca(28);br; /* CALL FAR */
	case 0x9b: E_WAIT (); br;
	case 0x9c: E_PUSH (flags); ca (10); br;
	case 0x9d: E_POP (flags); ca (8); br;
	case 0x9e: E_SAHF (); ca (4); br;
	case 0x9f: E_LAHF (); ca (4); br;
	case 0xa0: easeg=sregs[segover==4?3:segover];eaofs=getimm2();eamem=1;E_MOV(acc,rm,1);ca(10);br;
	case 0xa1: easeg=sregs[segover==4?3:segover];eaofs=getimm2();eamem=1;E_MOV(acc,rm,2);ca(10);br;
	case 0xa2: easeg=sregs[segover==4?3:segover];eaofs=getimm2();eamem=1;E_MOV(rm,acc,1);ca(10);br;
	case 0xa3: easeg=sregs[segover==4?3:segover];eaofs=getimm2();eamem=1;E_MOV(rm,acc,2);ca(10);br;
	case 0xa4: E_MOVSB (); ca (18); br;
	case 0xa5: E_MOVSW (); ca (18); br;
	case 0xa6: E_CMPSB (); ca (22); br;
	case 0xa7: E_CMPSW (); ca (22); br;
	case 0xa8: E_TEST (acc, imm, 1); ca (4); br;
	case 0xa9: E_TEST (acc, imm, 2); ca (4); br;
	case 0xaa: E_STOSB (); ca (11); br;
	case 0xab: E_STOSW (); ca (11); br;
	case 0xac: E_LODSB (); ca (12); br;
	case 0xad: E_LODSW (); ca (12); br;
	case 0xae: E_SCASB (); ca (15); br;
	case 0xaf: E_SCASW (); ca (15); br;
	case 0xb0: E_MOV (al, imm, 1); ca (4); br;
	case 0xb1: E_MOV (cl, imm, 1); ca (4); br;
	case 0xb2: E_MOV (dl, imm, 1); ca (4); br;
	case 0xb3: E_MOV (bl, imm, 1); ca (4); br;
	case 0xb4: E_MOV (ah, imm, 1); ca (4); br;
	case 0xb5: E_MOV (ch, imm, 1); ca (4); br;
	case 0xb6: E_MOV (dh, imm, 1); ca (4); br;
	case 0xb7: E_MOV (bh, imm, 1); ca (4); br;
	case 0xb8: E_MOV (ax, imm, 2); ca (4); br;
	case 0xb9: E_MOV (cx, imm, 2); ca (4); br;
	case 0xba: E_MOV (dx, imm, 2); ca (4); br;
	case 0xbb: E_MOV (bx, imm, 2); ca (4); br;
	case 0xbc: E_MOV (sp, imm, 2); ca (4); br;
	case 0xbd: E_MOV (bp, imm, 2); ca (4); br;
	case 0xbe: E_MOV (si, imm, 2); ca (4); br;
	case 0xbf: E_MOV (di, imm, 2); ca (4); br;
	case 0xc2: tmp=E_POP2();letreg2(4,reg2(4)+getimm2());jmpn(tmp);ca(12);br; /* RETN pvalue */
	case 0xc3: tmp = E_POP2 (); jmpna (tmp); ca (8); br; /* RETN */
	case 0xc4: getmodrm (); E_LES (); ca (16); br;
	case 0xc5: getmodrm (); E_LDS (); ca (16); br;
	case 0xc6: getmodrm (); E_MOV (rm, imm, 1); ca (eamem ? 10 : 4); br;
	case 0xc7: getmodrm (); E_MOV (rm, imm, 2); ca (eamem ? 10 : 4); br;
	case 0xca: tmp=E_POP2();tmp2=E_POP2();letreg2(4,reg2(4)+getimm2());jmpf(tmp2,tmp);ca (17);br; /* RETF pvalue */
	case 0xcb: tmp = E_POP2 (); tmp2 = E_POP2 (); jmpf (tmp2, tmp); ca (18); br; /* RETF */
	case 0xcc: E_INT (3); ca (52); br;
	case 0xcd: tmp = getimm1 (); E_INT (tmp); ca (51); br;
	case 0xce: if (f_get () & 2048) E_INT (4), ca (53); else ca (4); br;
	case 0xcf: tmp=E_POP2();tmp2=E_POP2();E_POP(flags);jmpf(tmp2,tmp);ca(24);br; /* IRET */
	case 0xd0: getmodrm (); E_TABLE2 (rmc, 1, 1); br;
	case 0xd1: getmodrm (); E_TABLE2 (rmc, 1, 2); br;
	case 0xd2: getmodrm (); tmp = reg1 (1); E_TABLE2 (rm, tmp, 1); ca(4*tmp+(eamem?20:8)); br;
	case 0xd3: getmodrm (); tmp = reg1 (1); E_TABLE2 (rm, tmp, 2); ca(4*tmp+(eamem?20:8)); br;
	case 0xd4: tmp = getimm1 (); E_AAM (tmp); ca (83); br;
	case 0xd5: tmp = getimm1 (); E_AAD (tmp); ca (60); br;
	case 0xd6: E_XLAT (); ca (11); br;
	case 0xd7: E_XLAT (); ca (11); br;
	case 0xd8: E_ESC (); br;
	case 0xd9: E_ESC (); br;
	case 0xda: E_ESC (); br;
	case 0xdb: E_ESC (); br;
	case 0xdc: E_ESC (); br;
	case 0xdd: E_ESC (); br;
	case 0xde: E_ESC (); br;
	case 0xdf: E_ESC (); br;
	case 0xe0: tmp = getimm12 (); E_LOOPNEc (tmp); br;
	case 0xe1: tmp = getimm12 (); E_LOOPEc (tmp); br;
	case 0xe2: tmp = getimm12 (); E_LOOPc (tmp); br;
	case 0xe3: tmp = getimm12 (); E_JCXZc (tmp); br;
	case 0xe4: letreg1 (0, in (getimm1 ())); ca (10); br; /* IN AL,imm8 */
	case 0xe5: tmp=getimm1();letreg1(4,in(tmp+1));letreg1(0,in(tmp));ca(10);br; /* IN AX,imm8 */
	case 0xe6: out (getimm1 (), reg1 (0)); ca (10); br; /* OUT imm8,AL */
	case 0xe7: tmp=getimm1();out(tmp+1,reg1(4));out(tmp,reg1(0));ca(10);br; /* OUT imm8,AX */
	case 0xe8: tmp = getimm2 (); E_PUSH (ip); jmpn (tmp); ca (19); br; /* CALLN */
	case 0xe9: tmp = getimm2 (); jmpn (tmp); ca (15); br; /* JMPN */
	case 0xea: tmp = getimm2 (); tmp2 = getimm2 (); jmpf (tmp2, tmp); ca (15); br; /* JMPF */
	case 0xeb: tmp = getimm12 (); jmpn (tmp); ca (15); br; /* JMPS */
	case 0xec: letreg1 (0, in (reg2 (2))); ca (8); br; /* IN AL,DX */
	case 0xed: tmp=reg2(2);letreg1(4,in(tmp+1));letreg1(0,in(tmp));ca(8);br; /* IN AX,DX */
	case 0xee: out (reg2 (2), reg1 (0)); ca (8); br; /* OUT DX,AL */
	case 0xef: tmp=reg2(2);out(tmp+1,reg1(4));out(tmp,reg1(0));ca(8);br; /* OUT DX,AX */
	case 0xf0: E_LOCK (); ca(2); br;
	case 0xf2: E_REPEc (); br;
	case 0xf3: E_REPNEc (); br;
	case 0xf4: E_HLT (); ca (2); br;
	case 0xf5: E_CMC (); ca (2); br;
	case 0xf6: getmodrm (); E_TABLE3c (1); br;
	case 0xf7: getmodrm (); E_TABLE3c (2); br;
	case 0xf8: E_CLC (); ca (2); br;
	case 0xf9: E_STC (); ca (2); br;
	case 0xfa: E_CLI (); ca (2); br;
	case 0xfb: E_STI (); ca (2); br;
	case 0xfc: E_CLD (); ca (2); br;
	case 0xfd: E_STD (); ca (2); br;
	case 0xfe: getmodrm (); E_TABLE4c (); br;
	case 0xff: getmodrm (); E_TABLE5c (); br;
	default: E_WARN ("???");
	}
      if (opcode == 0x26)
	{
	  segover = 0;
	  ca (2);
	  continue;
	}
      if (opcode == 0x2e)
	{
	  segover = 1;
	  ca (2);
	  continue;
	}
      if (opcode == 0x36)
	{
	  segover = 2;
	  ca (2);
	  continue;
	}
      if (opcode == 0x3e)
	{
	  segover = 3;
	  ca (2);
	  continue;
	}
      segover = 4;
      if (llll-- != 2)
	break;
    }
  if (tf)
    E_WARN ("TF IS ON"), E_INT (1);
  biu_ct ();
  return cycles;
}

void
intr8088 (int f)
{
  f_intr = f;
}

void
nmi8088 (int f)
{
  f_nmi = f;
}

#if 1
void
printip8088(void)
{
#if 0
  static int a = 0;
  static t16 d = 0;

  if (sregs[1] == 0x580 && ip == 0x3e9)
    a = 1;
  if (a)
    {
      if (d != ip)
#endif
	fprintf(stderr,"cs:%x,ip:%x,flags:%x,ss:%x,sp:%x\n",sregs[1],ip,f_get(),sregs[2],regs[4]);
#if 0
      d = ip;
    }
#endif
}
#endif


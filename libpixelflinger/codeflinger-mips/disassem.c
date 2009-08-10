
/* Just a place holder until I finish it if necessary.
 * Right now, I just dump the binary code generated to a file and use
 * the binutils tools to display the instructions.  That way, I ensure I
 * use a different code baseline to display instructions.
 */
#include <sys/cdefs.h>
#include <sys/param.h>
#include <stdio.h>

#include "disassem.h"

#define INSN_SIZE	4

/* Local prototypes */
static u_int disassemble_readword(u_int address);
static void disassemble_printaddr(u_int address);

static char *regnames[] = {
        "zero", "at", "v0", "v1", "a0", "a1", "a2", "a3",
	"t0", "t1", "t2", "t3", "t4", "t5", "t6", "t7",
        "s0", "s1", "s2", "s3", "s4", "s5", "s6", "s7",
	"t8", "t9", "kt0", "kt1", "gp", "sp", "s8", "ra"
};

u_int
disasm(const disasm_interface_t *di, u_int loc, int altfmt)
{
	unsigned int insn;

	insn = di->di_readword(loc);
	di->di_printf("0x%02x, %s, %s, %s, %2d, 0x%02x\n",
		(insn >> 26) &0x3f, 
		regnames[(insn >> 21) &0x1f], 
		regnames[(insn >> 16) &0x1f], 
		regnames[(insn >> 11) &0x1f], 
		(insn >> 6) &0x1f, 
		insn & 0x3f);
	return(loc + INSN_SIZE);
}

static u_int
disassemble_readword(u_int address)
{
	return(*((u_int *)address));
}

static void
disassemble_printaddr(u_int address)
{
	printf("0x%08x", address);
}

static const disasm_interface_t disassemble_di = {
	disassemble_readword, disassemble_printaddr, printf
};

void
disassemble(u_int address)
{
	(void)disasm(&disassemble_di, address, 0);
}

/* End of disassem.c */

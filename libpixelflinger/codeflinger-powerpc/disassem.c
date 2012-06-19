/* Just a place holder until I finish it if necessary.
 * Right now, I just dump the binary code generated to a file and use
 * the binutils tools to display the instructions.  That way, I ensure I
 * use a different code baseline to display instructions.
 */
#include <sys/cdefs.h>
#include <sys/param.h>
#include <stdio.h>

#include "disassem.h"

/* Local prototypes */
static u_int disassemble_readword(u_int address);
static void disassemble_printaddr(u_int address);

u_int
disasm(const disasm_interface_t *di, u_int loc, int altfmt)
{
	return 0;
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

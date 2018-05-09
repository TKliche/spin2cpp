//
// binary data output for spin2cpp
//
// Copyright 2012-2018 Total Spectrum Software Inc.
// see the file COPYING for conditions of redistribution
//
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <errno.h>
#include "spinc.h"

int gl_compressed = 0;

static void putbyte(Flexbuf *f, unsigned int x)
{
    flexbuf_putc(x & 0xff, f);
}
static void putword(Flexbuf *f, unsigned int x)
{
    putbyte(f, x & 0xff);
    putbyte(f,  (x>>8) & 0xff);
}
static void putlong(Flexbuf *f, unsigned int x)
{
    putbyte(f, x & 0xff);
    putbyte(f,  (x>>8) & 0xff);
    putbyte(f, (x>>16) & 0xff);
    putbyte(f, (x>>24) & 0xff);
}

static void
OutputSpinHeader(Flexbuf *f, Module *P)
{
    unsigned int clkfreq;
    unsigned int clkmodeval;

    if (!GetClkFreq(P, &clkfreq, &clkmodeval)) {
        // use defaults
        clkfreq = 80000000;
        clkmodeval = 0x6f;
    }
    
    putlong(f, clkfreq);
    putbyte(f, clkmodeval);
    putbyte(f, 0);      // checksum
    putword(f, 0x0010); // PBASE
    putword(f, 0x7fe8); // VBASE
    putword(f, 0x7ff0); // DBASE
    putword(f, 0x0018); // PCURR
    putword(f, 0x7ff8); // DCURR
    putword(f, 0x0008); // object length?
    putbyte(f, 0x02);
    putbyte(f, 0x00);
    putword(f, 0x0008);
    putword(f, 0x0000); // initial stack: 0 == first run of program

    // simple spin program
    putbyte(f, 0x3f);
    putbyte(f, 0x89);
    putbyte(f, 0xc7);
    putbyte(f, 0x10);
    putbyte(f, 0xa4);
    putbyte(f, 0x6);
    putbyte(f, 0x2c);
    putbyte(f, 0x32);
}

void
OutputDatFile(const char *fname, Module *P, int prefixBin)
{
    FILE *f = NULL;
    Module *save;
    Flexbuf fb;
    size_t curlen;
    size_t desiredlen;
    
    save = current;
    current = P;

    f = fopen(fname, "wb");
    if (!f) {
        fprintf(stderr, "Unable to open output file: ");
        perror(fname);
        exit(1);
    }

    flexbuf_init(&fb, BUFSIZ);
    if (prefixBin && !gl_p2) {
        /* output a binary header */
        OutputSpinHeader(&fb, P);
    }
    PrintDataBlock(&fb, P, NULL, NULL);
    curlen = flexbuf_curlen(&fb);
    fwrite(flexbuf_peek(&fb), curlen, 1, f);
    if (gl_p2) {
        desiredlen = (curlen + 31) & ~31;
    } else {
        desiredlen = (curlen + 3) & ~3;
    }
    while (curlen < desiredlen) {
        fputc(0, f);
        curlen++;
    }
    fclose(f);
    flexbuf_delete(&fb);
    
    current = save;
}

/*
 * functions for output of DAT sections
 */
/*
 * data block printing functions
 */
#define BYTES_PER_LINE 16  /* must be at least 4 */
static int datacount = 0;

static DataBlockOutFunc outc;

static void
outputByteBinary(Flexbuf *f, int c)
{
    flexbuf_putc(c, f);
}

static void
outputByte(Flexbuf *f, int c)
{
    (*outc)(f, c);
    datacount++;
}

static void
outputLong(Flexbuf *f, uint32_t c)
{
    int i;
    for (i = 0; i < 4; i++) {
        outputByte(f, (c & 0xff));
        c = c>>8;
    }
}

static void
outputInstrLong(Flexbuf *f, uint32_t c)
{
    outputLong(f, c);
}

static void
initDataOutput(DataBlockOutFunc func)
{
    datacount = 0;
    if (func) {
        outc = func;
    } else {
        outc = outputByteBinary;
    }
}

static int
GetAddrOffset(AST *ast)
{
    Symbol *sym;
    Label *label;
    
    if (ast->kind != AST_IDENTIFIER) {
        ERROR(ast, "@@@ supported only on identifiers");
        return 0;
    }
    sym = LookupSymbol(ast->d.string);
    if (!sym) {
        ERROR(ast, "Unknown symbol %s", ast->d.string);
        return 0;
    }
    if (sym->type != SYM_LABEL) {
        ERROR(ast, "@@@ supported only on labels");
        return 0;
    }
    label = (Label *)sym->val;
    return label->hubval;
}
        
void
outputDataList(Flexbuf *f, int size, AST *ast, Flexbuf *relocs)
{
    unsigned val, origval;
    int i, reps;
    AST *sub;
    Reloc r;
    
    origval = 0;
    while (ast) {
        sub = ast->left;
        if (sub->kind == AST_ARRAYDECL || sub->kind == AST_ARRAYREF) {
            origval = EvalPasmExpr(ast->left->left);
            reps = EvalPasmExpr(ast->left->right);
        } else if (sub->kind == AST_STRING) {
            const char *ptr = sub->d.string;
            while (*ptr) {
                val = (*ptr++) & 0xff;
                outputByte(f, val);
                for (i = 1; i < size; i++) {
                    outputByte(f, 0);
                }
            }
            reps = 0;
        } else if (sub->kind == AST_RANGE) {
            int start = EvalPasmExpr(sub->left);
            int end = EvalPasmExpr(sub->right);
            while (start <= end) {
                val = start;
                for (i = 0; i < size; i++) {
                    outputByte(f, val & 0xff);
                    val = val >> 8;
                }
                start++;
            }
            reps = 0;
        } else if (sub->kind == AST_ABSADDROF) {
            if (relocs) {
                int addr = flexbuf_curlen(f);
                if (size != LONG_SIZE) {
                    ERROR(ast, "@@@ supported only on long values");
                }
                if ( (addr & 3) != 0 ) {
                    ERROR(ast, "@@@ supported only on long boundary");
                }
                r.addr = addr;
                r.value = origval = GetAddrOffset(sub->left);
                flexbuf_addmem(relocs, (const char *)&r, sizeof(r));
            } else {
                origval = EvalPasmExpr(sub);
            }
            reps = 1;
        } else {
            origval = EvalPasmExpr(sub);
            reps = 1;
        }
        while (reps > 0) {
            val = origval;
            for (i = 0; i < size; i++) {
                outputByte(f, val & 0xff);
                val = val >> 8;
            }
            --reps;
        }
        ast = ast->right;
    }
}

#define BIG_IMM_SRC 0x01
#define BIG_IMM_DST 0x02
#define ANY_BIG_IMM (BIG_IMM_SRC|BIG_IMM_DST)
#define DUMMY_MASK  0x80

static unsigned
ImmMask(Instruction *instr, int opnum, bool bigImm, AST *ast)
{
    unsigned mask;
    
    if (gl_p2) {
        mask = P2_IMM_SRC;
        if (bigImm) {
            mask |= BIG_IMM_SRC;
        }
    } else {
        mask = IMMEDIATE_INSTR;
    }
    switch(instr->ops) {
    case P2_JUMP:
    case P2_LOC:
    case P2_AUG:
    case P2_TJZ_OPERANDS:
    case P2_JINT_OPERANDS:
    case SRC_OPERAND_ONLY:
    case CALL_OPERAND:
        return mask;
    case TWO_OPERANDS:
    case TWO_OPERANDS_OPTIONAL:
    case TWO_OPERANDS_NO_FLAGS:
    case JMPRET_OPERANDS:
    case P2_TWO_OPERANDS:
    case P2_RDWR_OPERANDS:
    case THREE_OPERANDS_BYTE:
    case THREE_OPERANDS_NIBBLE:
    case THREE_OPERANDS_WORD:
        if (gl_p2) {
            if (opnum == 0) {
                mask = P2_IMM_DST;
                if (bigImm) {
                    mask |= BIG_IMM_DST;
                }
            } else if (opnum == 2) {
                mask = DUMMY_MASK; // no change to opcode
            }
        } else {
            if (opnum == 0) {
                ERROR(ast, "bad immediate operand to %s", instr->name);
                return 0;
            }
        }
        return mask;
    case P2_DST_CONST_OK:
        /* uses the I bit instead of L?? */
        mask = P2_IMM_SRC;
        if (bigImm) {
            mask |= BIG_IMM_DST;
        }
        return mask;
    default:
        ERROR(ast, "immediate value not supported for %s instruction", instr->name);
        return 0;
    }
}

/*
 * assemble a special read operand
 */
unsigned
SpecialRdOperand(AST *ast)
{
    HwReg *hwreg;
    uint32_t val = 0;
    int subval = 0;
    int negsubval = 0;
    
    if (ast->kind == AST_OPERATOR && (ast->d.ival == K_INCREMENT
                                      || ast->d.ival == K_DECREMENT))
    {
        if (ast->d.ival == K_INCREMENT) {
            if (ast->left) {
                ast = ast->left;
                // a++: x110 0001
                val = 0x60;
                subval = 1;
            } else {
                // ++a: x100 0001
                ast = ast->right;
                val = 0x40;
                subval = 1;
            }
        } else {
            if (ast->left) {
                ast = ast->left;
                // a--
                val = 0x60;
                subval = -1;
            } else {
                // --a: x101 1111
                ast = ast->right;
                val = 0x40;
                subval = -1;
            }
        }
    }

    negsubval = subval < 0 ? -1 : 1;
    if (ast->kind == AST_RANGEREF) {
        AST *idx = ast->right;
        if (idx && idx->kind == AST_RANGE) {
            if (idx->right) {
                ERROR(ast, "illegal ptr dereference");
                return 0;
            }
            subval = EvalPasmExpr(idx->left) * negsubval;
            if (subval < -16 || subval > 15) {
                ERROR(ast, "ptr index out of range");
                subval = 0;
            }
            ast = ast->left;
        } else {
            ERROR(ast, "bad ptr expression");
            return 0;
        }
    }
    if (ast->kind == AST_HWREG) {
        hwreg = (HwReg *)ast->d.ptr;
        if (hwreg->addr == 0x1f8) {
            // ptra
            val |= 0x100;
        } else if (hwreg->addr == 0x1f9) {
            // ptrb
            val |= 0x180;
        } else {
            if (val) {
                ERROR(ast, "bad hardware reference");
            }
            return 0;
        }
    } else if (val) {
        ERROR(ast, "bad rdlong/wrlong pointer reference");
        return 0;
    }
    return val | (subval & 0x1f);
}

/* utility routine to fix up an opcode based on the #N at the end */
uint32_t
FixupThreeOperands(uint32_t val, AST *op, uint32_t immflags, uint32_t maxN, AST *line, Instruction *instr)
{
    uint32_t NN;
    if (!op || immflags == 0) {
        ERROR(line, "Third operand to %s must be an immediate\n", instr->name);
        return val;
    }
    NN = EvalPasmExpr(op);
    if (NN >= maxN) {
        ERROR(line, "Third operand to %s must be less than %u\n", instr->name, maxN);
        return val;
    }
    val |= NN << 19;
    return val;
}

    
/*
 * assemble an instruction, along with its modifiers
 */
#define MAX_OPERANDS 3

static void
assembleInstruction(Flexbuf *f, AST *ast)
{
    uint32_t val, mask, src, dst;
    uint32_t immmask;
    uint32_t curpc;
    int inHub;
    int32_t isrc;
    Instruction *instr;
    int numoperands, expectops;
    AST *operand[MAX_OPERANDS];
    uint32_t opimm[MAX_OPERANDS];
    AST *line = ast;
    char *callname;
    AST *retast;
    AST *origast;
    int isRelJmp = 0;
    int opidx;
    bool needIndirect = false;
    
    extern Instruction instr_p2[];
    curpc = ast->d.ival & 0x00ffffff;
    inHub = (0 == (ast->d.ival & (1<<30)));
    ast = ast->left;
    
    /* make sure it is aligned */
    if (!gl_p2 && !gl_compressed) {
        while ((datacount % 4) != 0) {
            outputByte(f, 0);
        }
    }

    instr = (Instruction *)ast->d.ptr;
decode_instr:
    origast = ast;
    val = instr->binary;
    immmask = 0;
    if (instr->opc != OPC_NOP) {
        /* for anything except NOP set the condition to "always" */
        if (gl_p2) {
            val |= 0xf << 28;
        } else {
            val |= 0xf << 18;
        }
    }
    /* check for modifiers and operands */
    numoperands = 0;
    opidx = 0;
    ast = ast->right;
    while (ast != NULL) {
        if (ast->kind == AST_EXPRLIST) {
            uint32_t imask;
            AST *op;
            if (numoperands >= MAX_OPERANDS) {
                ERROR(line, "Too many operands to instruction");
                return;
            }
            if (ast->left && ast->left->kind == AST_IMMHOLDER) {
                imask = ImmMask(instr, numoperands, false, ast);
                op = ast->left->left;
            } else if (ast->left && ast->left->kind == AST_BIGIMMHOLDER) {
                imask = ImmMask(instr, numoperands, true, ast);
                op = ast->left->left;
            } else {
                imask = 0;
                op = ast->left;
            }
            operand[numoperands] = op;
            opimm[numoperands] = imask;
            immmask |= imask;
            numoperands++;
        } else if (ast->kind == AST_INSTRMODIFIER) {
            InstrModifier *mod = (InstrModifier *)ast->d.ptr;
            mask = mod->modifier;
            if (mask & 0x0003ffff) {
                val = val & mask;
            } else {
                val = val | mask;
            }
        } else {
            ERROR(line, "Internal error: expected instruction modifier found %d", ast->kind);
            return;
        }
        ast = ast->right;
    }

    /* parse operands and put them in place */
    switch (instr->ops) {
    case NO_OPERANDS:
        expectops = 0;
        break;
    case TWO_OPERANDS:
    case TWO_OPERANDS_OPTIONAL:
    case TWO_OPERANDS_NO_FLAGS:
    case JMPRET_OPERANDS:
    case P2_TJZ_OPERANDS:
    case P2_TWO_OPERANDS:
    case P2_RDWR_OPERANDS:
    case P2_LOC:
        expectops = 2;
        break;
    case THREE_OPERANDS_BYTE:
    case THREE_OPERANDS_WORD:
    case THREE_OPERANDS_NIBBLE:
        expectops = 3;
        break;
    default:
        expectops = 1;
        break;
    }
    if (instr->ops == TWO_OPERANDS_OPTIONAL && numoperands == 1) {
        // duplicate the operand
        // so neg r0 -> neg r0,r0
        operand[numoperands++] = operand[0];
    }
    if (expectops != numoperands) {
        ERROR(line, "Expected %d operands for %s, found %d", expectops, instr->name, numoperands);
        return;
    }
    src = dst = 0;
    switch (instr->ops) {
    case NO_OPERANDS:
        break;
    case P2_TWO_OPERANDS:
        if (!strcmp(instr->name, "rep")) {
            // special case: rep @x, N says to count the instructions up to x
            // and repeat them N times; fixup the operand here
            if (operand[0]->kind == AST_ADDROF && !opimm[0]) {
                operand[0] = AstOperator('/', AstOperator('-', operand[0], AstInteger(curpc+4)), AstInteger(4));
                opimm[0] = P2_IMM_DST;
                immmask |= P2_IMM_DST;
            }
        }
        // fall through
    case TWO_OPERANDS:
    case JMPRET_OPERANDS:
    case TWO_OPERANDS_OPTIONAL:
    case TWO_OPERANDS_NO_FLAGS:
    handle_two_operands:
        dst = EvalPasmExpr(operand[0]);
        src = EvalPasmExpr(operand[1]);
        break;
    case P2_RDWR_OPERANDS:
        dst = EvalPasmExpr(operand[0]);
        src = SpecialRdOperand(operand[1]);
        if (src == 0) {
            src = EvalPasmExpr(operand[1]);
        } else {
            immmask |= P2_IMM_SRC;
        }
        break;
    case P2_TJZ_OPERANDS:
        dst = EvalPasmExpr(operand[0]);
        if (!strcmp(instr->name, "calld") && opimm[1] && (dst >= 0x1f6 && dst <= 0x1f9)) {
            int k = 0;
            // use the .loc version of this instruction
            // if we cannot reach the src address
            // or if we are requested to by the user
            if (operand[1]->kind == AST_CATCH) {
                goto force_loc;
            }
            isrc = EvalPasmExpr(operand[1]);
            if (isrc < 0x400) {
                if (inHub) goto force_loc;
                isrc *= 4;
            } else if (!inHub) {
                goto force_loc;
            }
            isrc = (isrc - (curpc+4));
            if (0 != (isrc & 0x3)) {
                // "loc" required
                goto force_loc;
            }
            isrc /= 4;
            if ((isrc >= -256) && (isrc < 255)) {
                goto skip_loc;
            }
        force_loc:
            while (instr_p2[k].name && strcmp(instr_p2[k].name, "calld.loc") != 0) {
                k++;
            }
            if (!instr_p2[k].name) {
                ERROR(line, "Internal error in calld parsing");
                return;
            }
            instr = &instr_p2[k];
            ast = origast;
            goto decode_instr;
        }
    skip_loc:
        /* fall through */
    case P2_JINT_OPERANDS:
        opidx = (instr->ops == P2_TJZ_OPERANDS) ? 1 : 0;
        if (operand[opidx]->kind == AST_CATCH) {
            // asking for absolute address
            ERROR(line, "Absolute address not valid for %s", instr->name);
            isrc = 0;
        } else if (opimm[opidx]) {
            isrc = EvalPasmExpr(operand[opidx]);
            if (isrc < 0x400) {
                isrc *= 4;
            }
            isrc = isrc - (curpc+4);
            isrc /= 4;
            if ( (isrc < -256) || (isrc > 255) ) {
                ERROR(line, "Source out of range for relative branch %s", instr->name);
                isrc = 0;
            }
            src = isrc & 0x1ff;
        } else {
            src = EvalPasmExpr(operand[opidx]);
        }
        break;
    case SRC_OPERAND_ONLY:
    case P2_AUG:
        dst = 0;
        src = EvalPasmExpr(operand[0]);
        break;
    case P2_LOC:
        dst = EvalPasmExpr(operand[0]);
        if (dst >= 0x1f6 && dst <= 0x1f9) {
            val |= ( (dst-0x1f6) & 0x3) << 21;
        } else {
            if (instr->opc == OPC_GENERIC_BRANCH) {
                // try the .ind version
                needIndirect = true;
            } else {
                ERROR(line, "bad first operand to %s instruction", instr->name);
            }
        }
        opidx = 1;
        // fall through
    case P2_JUMP:
        // indirect jump needs to be handled
        if (needIndirect || !opimm[opidx]) {
            char tempName[80];
            int k;
            if (instr->ops == P2_LOC && instr->opc != OPC_GENERIC_BRANCH) {
                ERROR(line, "loc requires immediate operand");
                return;
            }
            strcpy(tempName, instr->name);
            strcat(tempName, ".ind"); // convert to indirect
            k = 0;
            while (instr_p2[k].name && strcmp(tempName, instr_p2[k].name) != 0) {
                k++;
            }
            if (!instr_p2[k].name) {
                ERROR(line, "Internal error, could not find %s\n", tempName);
                return;
            }
            instr = &instr_p2[k];
            ast = origast;
            goto decode_instr;
        }
        dst = 0;
        immmask = 0; // handle immediates specially for jumps
        // figure out if absolute or relative
        // if  crossing from COG to HUB or vice-versa,
        // make it absolute
        // if it's of the form \xxx, which we parse as CATCH(xxx),
        // then make it absolute
        if (operand[opidx]->kind == AST_CATCH) {
            AST *realval = operand[opidx]->left;
            if (realval->kind == AST_FUNCCALL) {
                realval = realval->left;  // correct for parser misreading
            }
            isrc = EvalPasmExpr(realval);
            isRelJmp = 0;
        } else {
            isrc = EvalPasmExpr(operand[opidx]);
            if ( (inHub && isrc < 0x400)
                 || (!inHub && isrc >= 0x400)
                )
            {
                isRelJmp = 0;
            } else {
                isRelJmp = 1;
            }
        }
        if (isRelJmp) {
            if (!inHub) {
                isrc *= 4;
            }
            isrc = isrc - (curpc+4);
            src = isrc & 0xfffff;
            val = val | (1<<20);
        } else {
            src = isrc & 0xfffff;
        }
        goto instr_ok;
    case DST_OPERAND_ONLY:
    case P2_DST_CONST_OK:
        dst = EvalPasmExpr(operand[0]);
        src = 0;
        break;
    case CALL_OPERAND:
        if (operand[0]->kind != AST_IDENTIFIER) {
            ERROR(operand[0], "call operand must be an identifier");
            return;
        }
        src = EvalPasmExpr(operand[0]);
        callname = (char *)malloc(strlen(operand[0]->d.string) + 8);
        strcpy(callname, operand[0]->d.string);
        strcat(callname, "_ret");
        retast = NewAST(AST_IDENTIFIER, NULL, NULL);
        retast->d.string = callname;
        dst = EvalPasmExpr(retast);
        break;
    case THREE_OPERANDS_NIBBLE:
        val = FixupThreeOperands(val, operand[2], opimm[2], 8, line, instr);
        goto handle_two_operands;
    case THREE_OPERANDS_BYTE:
        val = FixupThreeOperands(val, operand[2], opimm[2], 4, line, instr);
        goto handle_two_operands;
    case THREE_OPERANDS_WORD:
        val = FixupThreeOperands(val, operand[2], opimm[2], 2, line, instr);
        goto handle_two_operands;
    default:
        ERROR(line, "Unsupported instruction `%s'", instr->name);
        return;
    }

    if (instr->ops == P2_AUG) {
        if (immmask == 0) {
            ERROR(line, "%s requires immediate operand", instr->name);
        }
        immmask = 0;
        src = src >> 9;
    } else {
        if (immmask & BIG_IMM_DST) {
            uint32_t augval = val & 0xf0000000; // preserve condition
            augval |= (dst >> 9) & 0x007fffff;
            augval |= 0x0f800000; // AUGD
            dst &= 0x1ff;
            outputInstrLong(f, augval);
            immmask &= ~BIG_IMM_DST;
        }
        if (immmask & BIG_IMM_SRC) {
            uint32_t augval = val & 0xf0000000; // preserve condition
            augval |= (src >> 9) & 0x007fffff;
            augval |= 0x0f000000; // AUGS
            src &= 0x1ff;
            outputInstrLong(f, augval);
            immmask &= ~BIG_IMM_SRC;
        }
        if (src > 511) {
            ERROR(line, "Source operand too big for %s", instr->name);
            return;
        }
    }
    if (dst > 511) {
        ERROR(line, "Destination operand too big for %s", instr->name);
        return;
    }
instr_ok:
    val = val | (dst << 9) | src | (immmask & ~0xff);
    /* output the instruction */
    outputInstrLong(f, val);
}

static void
AlignPc(Flexbuf *f, int size)
{
        while ((datacount % size) != 0) {
            outputByte(f, 0);
        }
}

void
outputAlignedDataList(Flexbuf *f, int size, AST *ast, Flexbuf *relocs)
{
    if (size > 1 && !gl_p2) {
        AlignPc(f, size);
    }
    outputDataList(f, size, ast, relocs);
}

/*
 * output bytes for a file
 */
static void
assembleFile(Flexbuf *f, AST *ast)
{
    FILE *inf;
    const char *name = ast->d.string;
    int c;

    inf = fopen(name, "rb");
    if (!inf) {
        ERROR(ast, "file %s: %s", name, strerror(errno));
        return;
    }
    while ((c = fgetc(inf)) >= 0) {
        outputByte(f, c);
    }
    fclose(inf);
}

/*
 * output padding bytes
 */
static void
padTo(Flexbuf *f, AST *ast)
{
    uint32_t dest = EvalPasmExpr(ast->left);
    uint32_t cur = ast->d.ival;

    while (cur < dest) {
        outputByte(f, 0);
        cur++;
    }
}

/*
 * print out a data block
 */
void
PrintDataBlock(Flexbuf *f, Module *P, DataBlockOutFunc func, Flexbuf *relocs)
{
    AST *ast;
    AST *top;
    
    initDataOutput(func);
    if (gl_errors != 0)
        return;
    for (top = P->datblock; top; top = top->right) {
        ast = top;
        while (ast && ast->kind == AST_COMMENTEDNODE) {
            ast = ast->left;
        }
        if (!ast) continue;
        switch (ast->kind) {
        case AST_BYTELIST:
            outputAlignedDataList(f, 1, ast->left, relocs);
            break;
        case AST_WORDLIST:
            outputAlignedDataList(f, 2, ast->left, relocs);
            break;
        case AST_LONGLIST:
            outputAlignedDataList(f, 4, ast->left, relocs);
            break;
        case AST_ALIGN:
        {
            int size = EvalPasmExpr(ast->left);
            AlignPc(f, size);
            break;
        }
        case AST_INSTRHOLDER:
            assembleInstruction(f, ast);
            break;
        case AST_IDENTIFIER:
            /* just skip labels */
            break;
        case AST_FILE:
            assembleFile(f, ast->left);
            break;
        case AST_ORGH:
            /* need to skip ahead to PC */
            padTo(f, ast);
            break;
        case AST_ORG:
        case AST_RES:
        case AST_FIT:
        case AST_LINEBREAK:
            break;
        case AST_COMMENT:
            break;
        default:
            ERROR(ast, "unknown element in data block");
            break;
        }
    }
}

// output _clkmode and _clkfreq settings
int
GetClkFreq(Module *P, unsigned int *clkfreqptr, unsigned int *clkregptr)
{
    // look up in P->objsyms
    Symbol *clkmodesym = FindSymbol(&P->objsyms, "_clkmode");
    Symbol *sym;
    AST *ast;
    int32_t clkmode, clkfreq, xinfreq;
    int32_t multiplier = 1;
    uint8_t clkreg;
    
    if (!clkmodesym) {
        return 0;  // nothing to do
    }
    ast = (AST *)clkmodesym->val;
    if (clkmodesym->type != SYM_CONSTANT) {
        WARNING(ast, "_clkmode is not a constant");
        return 0;
    }
    clkmode = EvalConstExpr(ast);
    // now we need to figure out the frequency
    clkfreq = 0;
    sym = FindSymbol(&P->objsyms, "_clkfreq");
    if (sym) {
        if (sym->type == SYM_CONSTANT) {
            clkfreq = EvalConstExpr((AST*)sym->val);
        } else {
            WARNING((AST*)sym->val, "_clkfreq is not a constant");
        }
    }
    xinfreq = 0;
    sym = FindSymbol(&P->objsyms, "_xinfreq");
    if (sym) {
        if (sym->type == SYM_CONSTANT) {
            xinfreq = EvalConstExpr((AST*)sym->val);
        } else {
            WARNING((AST*)sym->val, "_xinfreq is not a constant");
        }
    }
    // calculate the multiplier
    clkreg = 0;
    if (clkmode & RCFAST) {
        // nothing to do here
    } else if (clkmode & RCSLOW) {
        clkreg |= 0x01;   // CLKSELx
    } else if (clkmode & (XINPUT)) {
        clkreg |= (1<<5); // OSCENA
        clkreg |= 0x02;   // CLKSELx
    } else {
        clkreg |= (1<<5); // OSCENA
        clkreg |= (1<<6); // PLLENA
        if (clkmode & XTAL1) {
            clkreg |= (1<<3);
        } else if (clkmode & XTAL2) {
            clkreg |= (2<<3);
        } else {
            clkreg |= (3<<3);
        }
        if (clkmode & PLL1X) {
            multiplier = 1;
            clkreg |= 0x3;  // CLKSELx
        } else if (clkmode & PLL2X) {
            multiplier = 2;
            clkreg |= 0x4;  // CLKSELx
        } else if (clkmode & PLL4X) {
            multiplier = 4;
            clkreg |= 0x5;  // CLKSELx
        } else if (clkmode & PLL8X) {
            multiplier = 8;
            clkreg |= 0x6;  // CLKSELx
        } else if (clkmode & PLL16X) {
            multiplier = 16;
            clkreg |= 0x7;  // CLKSELx
        }
    }
    
    // validate xinfreq and clkfreq
    if (xinfreq == 0) {
        if (clkfreq == 0) {
            ERROR(NULL, "Must set at least one of _XINFREQ or _CLKFREQ");
            return 0;
        }
    } else {
        int32_t calcfreq = xinfreq * multiplier;
        if (clkfreq != 0) {
            if (calcfreq != clkfreq) {
                ERROR(NULL, "Inconsistent values for _XINFREQ and _CLKFREQ");
                return 0;
            }
        }
        clkfreq = calcfreq;
    }

    *clkfreqptr = clkfreq;
    *clkregptr = clkreg;
    return 1;
}

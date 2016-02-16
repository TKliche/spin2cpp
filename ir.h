/*
 * Spin to Pasm converter
 * Copyright 2016 Total Spectrum Software Inc.
 * Intermediate representation definitions
 */

#ifndef SPIN_IR_H
#define SPIN_IR_H

typedef struct IR IR;
typedef struct Operand Operand;
typedef struct parserstate ParserState;

enum IROpcode {
    OPC_COMMENT,
    OPC_LABEL,
    OPC_BYTE,
    OPC_WORD,
    OPC_LONG,

    /* various instructions */
    OPC_MOVE,
    OPC_ADD,
    OPC_SUB,
    OPC_AND,
    OPC_ANDN,
    OPC_OR,
    OPC_XOR,
    OPC_SHL,
    OPC_SHR,
    OPC_SAR,
    OPC_ROL,
    OPC_ROR,
    OPC_CMP,
    OPC_NOT,
    OPC_NEG,
    OPC_ABS,
    OPC_CALL,
    OPC_RET,
    OPC_DJNZ,
    OPC_JUMP,

    /* special flag to indicate a dead register */
    OPC_DEAD,

    OPC_UNKNOWN,
};

/* condition for conditional execution */
/* NOTE: opposite conditions must go together
 * in pairs so that InvertCond can easily
 * find the opposite of any condition
 */
typedef enum IRCond {
    COND_TRUE,
    COND_FALSE,
    COND_LT,
    COND_GE,
    COND_EQ,
    COND_NE,
    COND_LE,
    COND_GT
} IRCond;

struct IR {
    enum IROpcode opc;
    enum IRCond cond;
    Operand *dst;
    Operand *src;
        
    IR *prev;
    IR *next;
};

typedef struct IRList {
    IR *head;
    IR *tail;
} IRList;

enum Operandkind {
    REG_IMM,  // for an immediate value
    REG_HW,   // for a hardware register
    REG_REG,  // for a regular register
    REG_LOCAL, // for a "local" register (only live inside function)
    REG_ARG,   // for an argument to a function
    REG_LABEL, // for a code label
    REG_STRING,
};

typedef enum Operandkind Operandkind;

struct Operand {
    enum Operandkind kind;
    const char *name;
    int val;
    int used;
};

char *IRAssemble(IRList *list);
bool CompileToIR(IRList *list, ParserState *P);

#endif
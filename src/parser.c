/*
This file is part of libmtexp, a library providing an intuitive interface
to OpenGL multitexturing.

Copyright (C) 2005 John Tsiombikas <nuclear@siggraph.org>

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

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include "parser.h"

/* operator and operand (argument) stacks */

#define STACK_SIZE	100

static struct symb_stack {
	struct symbol stack[STACK_SIZE];
	int top;
} op_stack;

static struct tree_stack {
	struct ptree *stack[STACK_SIZE];
	int top;
} arg_stack;

#define PUSH(s, x) ((s).stack[s.top++] = x)
#define POP(s) ((s).stack[--s.top])
#define TOP(s) ((s).stack[(s).top - 1])
#define SSIZE(s) ((s).top)


/* symbol table, defines valid symbols, their type, and precedence */

#define SYMB_COUNT	11
static struct symbol symb_table[SYMB_COUNT] = {
	{"+",	SYMB_PLUS,	SYMB_TYPE_OP, {10}},
	{"-",	SYMB_MINUS,	SYMB_TYPE_OP, {10}},
	{"*",	SYMB_MUL,	SYMB_TYPE_OP, {20}},
	{"c",	SYMB_COL,	SYMB_TYPE_ARG, {0}},
	{"t0",	SYMB_T0,	SYMB_TYPE_ARG, {0}},
	{"t1",	SYMB_T1,	SYMB_TYPE_ARG, {0}},
	{"t2",	SYMB_T2,	SYMB_TYPE_ARG, {0}},
	{"t3",	SYMB_T3, 	SYMB_TYPE_ARG, {0}},
	{"(",	SYMB_OPEN, 	SYMB_TYPE_PAREN, {0}},
	{")",	SYMB_CLOSE,	SYMB_TYPE_PAREN, {0}},
	{"#",	SYMB_NUM,	SYMB_TYPE_ARG, {0}}
};

/* forward declarations of various local functions, defined below */
static void shift(struct symbol *s);
static int reduce(void);
static void clean_stacks(void);
static struct symbol *match_symbol(const char *str);
static const char *consume(int symb, const char *eptr);
static struct ptree *make_ptree(struct symbol *s, struct ptree *left, struct ptree *right);


/* --- mtexp_parse() ---
 * parses the specified expression based on the symbol table above
 * and returns the corresponding expression tree
 */
struct ptree *mtexp_parse(const char *expr) {
	const char *eptr = expr - 1;

	while(*++eptr) {
		struct symbol *symb;
		
		if(isspace(*eptr)) continue;	/* eat up any whitespace */
		
		/* get the next symbol (accepts only symbols in the symb_table[]) */
		if(!(symb = match_symbol(eptr))) {
			fprintf(stderr, "unexpected token: %s\n", eptr);
			clean_stacks();
			return 0;
		}

		/* symbol accepted, consume it from the input */
		eptr = consume(symb->symb, eptr);
		
		/* parse the thing */
		switch(symb->type) {
		case SYMB_TYPE_ARG:
			/* if it is an operand, shift */
			shift(symb);
			break;

		case SYMB_TYPE_OP:
			/* if it is an operator, then if the operator stack is 
			 * not empty and the operator on the top of it has higher
			 * precedence than the new one, then reduce first then shift
			 * the new operator. Otherwise just shift.
			 *
			 * note: the >= comparison implies left-associativity for all operators
			 * of equal precedence.
			 */
			if(SSIZE(op_stack) > 0 && TOP(op_stack).val.precedence >= symb->val.precedence) {
				/* The parser tries to be smart here, if the two arguments that
				 * are going to be used during reduce() are both textures, then
				 * we will have a problem during the texture unit setup, as we can't
				 * have two textures as source arguments on the same unit, so we reduce
				 * only if the two arguments on the stack are not both textures
				 */
				int a1, a2;
				a1 = arg_stack.stack[arg_stack.top - 1]->symb.symb;
				a2 = arg_stack.stack[arg_stack.top - 2]->symb.symb;

				if(!(a1 >= SYMB_T0 && a1 <= SYMB_T3) || !(a2 >= SYMB_T0 && a2 <= SYMB_T3)) {
					if(reduce() == -1) {
						fprintf(stderr, "reduce failed, argument stack underflow\n");
						clean_stacks();
						return 0;
					}
				}
			}
			shift(symb);
			break;

		case SYMB_TYPE_PAREN:
			if(symb->symb == SYMB_OPEN) {
				/* if it is an opening parenthesis, shift */
				shift(symb);
			} else {
				/* keep reducing until we reach the openning parenthesis */
				while(TOP(op_stack).symb != SYMB_OPEN) {
					if(reduce() == -1) {
						fprintf(stderr, "reduce failed, argument stack underflow\n");
						clean_stacks();
						return 0;
					}

					if(SSIZE(op_stack) < 1) {
						fprintf(stderr, "parenthesis mismatch (more close than open)\n");
						clean_stacks();
						return 0;
					}
				}

				/* discard the matching openning parenthesis */
				POP(op_stack);
			}
			break;

		default:
			fprintf(stderr, "warning, unexpected symbol while parsing\n");
			clean_stacks();
			return 0;
		}
	}

	/* reduce like there's no tomorrow */
	while(SSIZE(op_stack)) {
		if(reduce() == -1) {
			fprintf(stderr, "reduce failed, argument stack underflow\n");
			clean_stacks();
			return 0;
		}
	}

	if(SSIZE(op_stack) != 0 || SSIZE(arg_stack) != 1) {
		fprintf(stderr, "parse tree creation failed, inconsistent stack state\n");
		fprintf(stderr, "op stack: %d\targ stack: %d\n", SSIZE(op_stack), SSIZE(arg_stack));
		clean_stacks();
		return 0;
	}

	return POP(arg_stack);
}

/* --- mtexp_free_ptree() ---
 * destroyes an expression tree
 */
void mtexp_free_ptree(struct ptree *t) {
	if(!t) return;

	mtexp_free_ptree(t->left);
	mtexp_free_ptree(t->right);
	free(t);
}

/* --- mtexp_show_ptree() ---
 * outputs a crude visualization of the expression tree to stdout.
 * Useful mainly for debugging purposes.
 */
void mtexp_show_ptree(struct ptree *t) {
	static int lvl = -1;
	int i;

	if(t) {
		lvl++;

		for(i=0; i<lvl; i++) fputs("   ", stdout);
		if(lvl) fputs("|- ", stdout);
		puts(t->symb.str);

		mtexp_show_ptree(t->left);
		mtexp_show_ptree(t->right);

		lvl--;
	}
}

/* --- shift() ---
 * pushes the symbol into the appropriate stack
 */
static void shift(struct symbol *s) {
	if(s->type == SYMB_TYPE_ARG) {
		PUSH(arg_stack, make_ptree(s, 0, 0));
	} else {
		PUSH(op_stack, *s);
	}
}

/* --- reduce() ---
 * pops an operator from the operator stack and the appropriate
 * number of operands from the argument stack, makes a tree out of
 * them and pushes it back in the argument stack.
 * 
 * note: at this point all operators are binary, so it always gets
 * two arguments from the stack.
 */
static int reduce(void) {
	struct symbol op;
	struct ptree *a1, *a2;
				
	if(SSIZE(arg_stack) < 2) return -1;

	op = POP(op_stack);
	a2 = POP(arg_stack);
	a1 = POP(arg_stack);

	PUSH(arg_stack, make_ptree(&op, a1, a2));
	return 0;
}


static void clean_stacks(void) {
	while(SSIZE(arg_stack)) {
		struct ptree *t = POP(arg_stack);
		mtexp_free_ptree(t);
	}

	op_stack.top = 0;
}


/* --- match_symbol() ---
 * tries to match the beginning of the passed string to any symbol of
 * the symbol table, and returns the correspondence.
 */
static struct symbol *match_symbol(const char *str) {
	static struct symbol s;
	int i;

	if(isdigit(*str)) {
		s = symb_table[SYMB_NUM];
		s.val.value[0] = s.val.value[1] = s.val.value[2] = s.val.value[3] = atof(str);
		return &s;
	}

	if(*str == '<') {
		s = symb_table[SYMB_NUM];

		if(!isdigit(*++str)) return 0;
		s.val.value[0] = atof(str);
		while(isdigit(*str) || *str == '.') str++;
		while(isspace(*str) || *str == ',') str++;

		for(i=1; i<4; i++) {
			if(!isdigit(*str)) {
				s.val.value[i] = i < 3 ? s.val.value[i - 1] : 1.0f;
			} else {
				s.val.value[i] = atof(str);
			}
			while(isdigit(*str) || *str == '.') str++;
			while(isspace(*str) || *str == ',') str++;
		}
		
		if(*str != '>') return 0;
		return &s;
	}
		

	for(i=0; i<SYMB_COUNT; i++) {
		const char *symb, *strptr;

		if(i == SYMB_NUM) continue;
		
		symb = symb_table[i].str;
		strptr = str;
		while(*symb && *strptr && *symb == *strptr) {
			symb++;
			strptr++;
		}

		if(!*symb) {
			s = symb_table[i];
			return &s;
		}
	}

	return 0;
}

/* --- consume() ---
 * consumes the appropriate ammount of characters for the specified symbol
 * from the expression string and returns the position of the new pointer.
 */
static const char *consume(int symb, const char *eptr) {
	if(symb == SYMB_NUM) {
		if(*eptr == '<') {
			while(*eptr && *eptr++ != '>');
			eptr--;
		} else {
			int dots_eaten = 0;
			while(isdigit(*eptr) || (*eptr == '.' && !dots_eaten++)) eptr++;
			eptr--;
		}
	} else {
		const char *str = symb_table[symb].str + 1;
		while(*str++) eptr++;
	}
	return eptr;
}


static struct ptree *make_ptree(struct symbol *s, struct ptree *left, struct ptree *right) {
	struct ptree *t = malloc(sizeof(struct ptree));
	if(t) {
		t->symb = *s;
		t->left = left;
		t->right = right;
	}
	return t;
}

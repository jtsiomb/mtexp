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

#ifndef _PARSER_H_
#define _PARSER_H_

#define MAX_TEXTURES	4

/* possible symbols in the expression */
enum {
	SYMB_PLUS,		/* + */
	SYMB_MINUS,		/* - */
	SYMB_MUL,		/* * */
	SYMB_COL,		/* c */
	SYMB_T0,		/* t0 */
	SYMB_T1,		/* t1 */
	SYMB_T2,		/* t2 */
	SYMB_T3,		/* t3 */
	SYMB_OPEN,		/* ( */
	SYMB_CLOSE,		/* ) */
	SYMB_NUM		/* a constant */
};

/* symbol types (operator, argument, parenthesis) */
enum {SYMB_TYPE_OP, SYMB_TYPE_ARG, SYMB_TYPE_PAREN};

/* symbol data structure */
struct symbol {
	char *str;	/* textual representation in the expression */
	int symb;
	int type;
	
	union {
		int precedence;	/* for operators */
		float value[4];	/* for constants */
	} val;
};

/* node of the expression tree */
struct ptree {
	struct symbol symb;
	struct ptree *left, *right;
};

#ifdef __cplusplus
extern "C" {
#endif	/* __cplusplus */

/* parses the expression and returns the expression tree */
struct ptree *mtexp_parse(const char *expr);

/* destroyes an expression tree */
void mtexp_free_ptree(struct ptree *t);

/* outputs the expression tree to stdout (for debugging mainly) */
void mtexp_show_ptree(struct ptree *t);

#ifdef __cplusplus
}
#endif	/* __cplusplus */

#endif	/* _PARSER_H_ */

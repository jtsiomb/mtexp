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
#include <stdarg.h>
#include <GL/gl.h>
#include "mtexp.h"
#include "parser.h"

#ifndef GL_VERSION_1_3

#define OLD_OPENGL
#include "glext.h"

PFNGLACTIVETEXTUREARBPROC glActiveTexture;
PFNGLCLIENTACTIVETEXTUREARBPROC glClientActiveTexture;

#endif	/* GL_VERSION_1_3 */


static const GLenum tex_type[] = {
	GL_TEXTURE_1D,
	GL_TEXTURE_2D,
	GL_TEXTURE_3D,
	GL_TEXTURE_CUBE_MAP,
	0
};

struct mtexp {
	struct ptree *tree;
	int passes;
	unsigned int tex[MAX_TEXTURES];
	int tex_count;
	int active_tree;

	int first_call;	/* for debugging purposes */
};

/* OpenGL related functions */
static void init(void);
static void active_unit(int unit);
static int symbol_to_glcombine(int symb);
static int symbol_to_glsource(struct symbol *s);
static int handle_operand(struct symbol *symb, const unsigned int *tex);
static int set_tex_state_rec(const struct ptree *t, const unsigned int *tex, int height);

/* additional parse tree operations */
static int height(struct ptree *t);
static int count_ops(struct ptree *t);
static int is_symb_texture(int s);
static int count_tex_usage(struct ptree *t);


static int first_call = -1;	/* not only for debugging purposes */


/* creates an mtexp state from the specified expression and texture ids */
struct mtexp *mtexp_create(const char *expr, ...) {
	va_list arg_list;
	int i;
	struct mtexp *ts;

	if(first_call == -1) {
		first_call = 0;
		init();
	}
	
	ts = malloc(sizeof(struct mtexp));
	ts->first_call = 1;

	ts->tree = mtexp_parse(expr);
	mtexp_show_ptree(ts->tree);
	
	if(height(ts->tree) != count_ops(ts->tree) + 1) {
		fprintf(stderr, "invalid texture state tree (ops: %d, height: %d)\n", count_ops(ts->tree), height(ts->tree));
		mtexp_free(ts);
		return 0;
	}
	
	ts->tex_count = count_tex_usage(ts->tree);
	printf("textures in tree: %d\n", ts->tex_count);

	va_start(arg_list, expr);

	for(i=0; i<ts->tex_count; i++) {
		ts->tex[i] = va_arg(arg_list, unsigned int);
	}
	
	va_end(arg_list);

	return ts;
}

void mtexp_free(struct mtexp *state) {
	mtexp_free_ptree(state->tree);
	free(state);
}

int mtexp_enable(const struct mtexp *state) {
#ifdef DEBUG
	first_call = state->first_call;
	if(state->first_call) ((struct mtexp*)state)->first_call = 0;
#endif	/* DEBUG */
	
	return set_tex_state_rec(state->tree, state->tex, height(state->tree) - 2);
}

void mtexp_disable(const struct mtexp *state) {
	int i;
	for(i=state->tex_count-1; i>=0; i--) {
		const GLenum *tptr = tex_type;
		active_unit(i);
		while(*tptr) glDisable(*tptr++);
	}
}

/* ---------- local functions ----------- */

static void init(void) {
#ifdef OLD_OPENGL
	glActiveTexture = get_proc_address("glActiveTextureARB");
	glClientActiveTexture = get_proc_address("glClientActiveTextureARB");
#endif	/* OLD_OPENGL */
}

/* sets the active texture unit */
static void active_unit(int unit) {
	glActiveTexture((GLenum)((int)GL_TEXTURE0 + unit));
	glClientActiveTexture((GLenum)((int)GL_TEXTURE0 + unit));
}

static int symbol_to_glcombine(int symb) {
	static int map[] = {GL_ADD, GL_SUBTRACT, GL_MODULATE};
	return map[symb];
}

static int symbol_to_glsource(struct symbol *s) {
	int res;
	
	if(s->type == SYMB_TYPE_ARG) {
		switch(s->symb) {
		case SYMB_COL:
			res = GL_PRIMARY_COLOR;
			break;

		case SYMB_NUM:
			res = GL_CONSTANT;
			break;

		default:
			res = GL_TEXTURE;
		}
	} else {
		res = GL_PREVIOUS;
	}

	return res;
}

static void bind_texture(unsigned int tex) {
	const GLenum *tptr = tex_type;
	
	do {
		glGetError();	/* clear errors */
		glBindTexture(*tptr, tex);
	} while(glGetError() != GL_NO_ERROR && *++tptr);

	if(*tptr) glEnable(*tptr);
}


static int handle_operand(struct symbol *symb, const unsigned int *tex) {
	int operand = symbol_to_glsource(symb);
	
	if(operand == GL_TEXTURE) {
		bind_texture(tex[symb->symb - SYMB_T0]);
	} else if(operand == GL_CONSTANT) {
		glTexEnvfv(GL_TEXTURE_ENV, GL_TEXTURE_ENV_COLOR, symb->val.value);
	}

	return operand;
}

static int set_tex_state_rec(const struct ptree *t, const unsigned int *tex, int height) {
	if(!t) return 0;

	if(set_tex_state_rec(t->left, tex, height - 1) == -1) return -1;
	if(set_tex_state_rec(t->right, tex, height - 1) == -1) return -1;
	
	if(t->symb.type == SYMB_TYPE_OP) {
		int s0, s1, op;
		
		if(!t->left || !t->right) {
			fprintf(stderr, "came upon a binary operator with less than two operands!?\n");
			return -1;
		}

		active_unit(height);

		op = symbol_to_glcombine(t->symb.symb);
		s0 = handle_operand(&t->left->symb, tex);
		s1 = handle_operand(&t->right->symb, tex);
		
		glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_COMBINE);
		glTexEnvi(GL_TEXTURE_ENV, GL_COMBINE_RGB, op);
		glTexEnvi(GL_TEXTURE_ENV, GL_SOURCE0_RGB, s0);
		glTexEnvi(GL_TEXTURE_ENV, GL_SOURCE1_RGB, s1);

#ifdef DEBUG
		if(first_call) {
			printf("\nunit(%d)\n", height);
			printf("op(%s)\n", t->symb.str);
			printf("src0(%s)\n", s0 == GL_PREVIOUS ? "prev" : (s0 == GL_TEXTURE ? "tex" : (s0 == GL_CONSTANT ? "con" : "col")));
			printf("src1(%s)\n", s1 == GL_PREVIOUS ? "prev" : (s1 == GL_TEXTURE ? "tex" : (s1 == GL_CONSTANT ? "con" : "col")));
		}
#endif	/* DEBUG */
	}

	return 0;
}

static int height(struct ptree *t) {
	int hleft, hright;
	if(!t) return 0;

	hleft = height(t->left);
	hright = height(t->right);
	return 1 + (hleft > hright ? hleft : hright);
}

static int count_ops(struct ptree *t) {
	if(!t) return 0;
	return count_ops(t->left) + count_ops(t->right) + (t->symb.type == SYMB_TYPE_OP ? 1 : 0);
}

static int is_symb_texture(int s) {
	return (s >= SYMB_T0 && s <= SYMB_T3) ? 1 : 0;
}

static int count_tex_usage(struct ptree *t) {
	if(!t) return 0;
	return is_symb_texture(t->symb.symb) + count_tex_usage(t->left) + count_tex_usage(t->right);
}


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

#ifndef _MTEXP_H_
#define _MTEXP_H_

struct mtexp;

#ifdef __cplusplus
extern "C" {
#endif	/* __cplusplus */

/* creates an mtexp state from the specified expression and texture ids */
struct mtexp *mtexp_create(const char *expr, ...);

/* frees the memory of an mtexp state */
void mtexp_free(struct mtexp *state);

/* sets up the multitexturing environment according to the
 * mtexp state passed.
 */
int mtexp_enable(const struct mtexp *state);

/* cleans up the OpenGL state by disabling all previously enabled
 * texture units and leaving texture unit 0 active.
 */
void mtexp_disable(const struct mtexp *state);

#ifdef __cplusplus
}
#endif	/* __cplusplus */

#endif	/* _MTEXP_H_ */

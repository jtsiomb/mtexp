/*
This file is an example program for using libmtexp.

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
#include <GL/glut.h>
#include "image.h"
#include "mtexp.h"

void update_display(void);
void key_handler(unsigned char k, int x, int y);
int load_texture(const char *fname);
void cleanup(void);

unsigned int t0, t1;
struct mtexp *ts;

int main(int argc, char **argv) {
	glutInit(&argc, argv);
	glutInitDisplayMode(GLUT_RGBA | GLUT_DOUBLE | GLUT_DEPTH);
	glutInitWindowSize(800, 600);
	glutCreateWindow("test");
	
	glutDisplayFunc(update_display);
	glutIdleFunc(update_display);
	glutKeyboardFunc(key_handler);

	atexit(cleanup);

	glEnable(GL_DEPTH_TEST);
	glEnable(GL_CULL_FACE);
	glFrontFace(GL_CW);
	
	glEnable(GL_LIGHTING);
	glEnable(GL_LIGHT0);

	glMatrixMode(GL_PROJECTION);
	gluPerspective(45.0, 1.33333, 1.0, 1000.0);

	glClearColor(0.0, 0.0, 0.0, 1.0);
	glClearDepth(1.0);

	t0 = load_texture("earth.tga");
	t1 = load_texture("bolt.tga");
	
	ts = mtexp_create(argc > 1 ? argv[1] : "t1 * t0 * c", t0, t1);
	if(!ts) return -1;

	glutMainLoop();
	return 0;
}

void update_display(void) {
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	glTranslatef(0, 0, -5);

	mtexp_enable(ts);

	glBegin(GL_QUADS);
	glNormal3f(0.0, 0.0, 1.0);
	glMultiTexCoord2f(0, 0.0, 0.0);
	glMultiTexCoord2f(1, 0.0, 0.0);
	glVertex3f(-2.0, 1.0, 0.0);
	glMultiTexCoord2f(0, 1.0, 0.0);
	glMultiTexCoord2f(1, 1.0, 0.0);
	glVertex3f(2.0, 1.0, 0.0);
	glMultiTexCoord2f(0, 1.0, 1.0);
	glMultiTexCoord2f(1, 1.0, 1.0);
	glVertex3f(2.0, -1.0, 0.0);
	glMultiTexCoord2f(0, 0.0, 1.0);
	glMultiTexCoord2f(1, 0.0, 1.0);
	glVertex3f(-2.0, -1.0, 0.0);
	glEnd();

	mtexp_disable(ts);

	glutSwapBuffers();
}

void key_handler(unsigned char k, int x, int y) {
	if(k == 27 || k == 'q' || k == 'Q') {
		exit(0);
	}
}

int load_texture(const char *fname) {
	unsigned int tex_id;
	unsigned long *img, x, y;

	if(!(img = load_image(fname, &x, &y))) {
		fprintf(stderr, "failed loading texture \"%s\"\n", fname);
		return 0;
	}

	glGenTextures(1, &tex_id);
	glBindTexture(GL_TEXTURE_2D, tex_id);
	glTexImage2D(GL_TEXTURE_2D, 0, 4, x, y, 0, GL_BGRA, GL_UNSIGNED_BYTE, img);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

	return tex_id;
}

void cleanup(void) {
	mtexp_free(ts);
}

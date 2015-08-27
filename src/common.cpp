/*
 * This file is part of rvg.
 * Copyright (c) 2014, David Gohel All rights reserved.
 *
 * rvg is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * rvg is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with rvg.  If not, see <http://www.gnu.org/licenses/>.
**/

#include "datastruct.h"
#include <stdio.h>
#include <Rinternals.h>
#include <R.h>
#include <string.h>
#include <cairo.h>
#include <R_ext/GraphicsEngine.h>
#include <R_ext/GraphicsDevice.h>


double getFontSize(double cex, double fontsize) {
	double size = cex * fontsize;
	if( size < 1.0 ) size = 0.0;
	return size;
}

double getFontHeight(const char *str, const pGEcontext gc, pDevDesc dev) {
	DOCDesc *pd = (DOCDesc *) dev->deviceSpecific;

	cairo_t *cc;
	cc = pd->cr;

	cairo_text_extents_t te;
	cairo_text_extents (cc, str, &te);

	return te.height;
}

static void set_cf_antialias(cairo_t *cc) {
	cairo_font_options_t *fo = cairo_font_options_create();
	cairo_font_options_set_antialias(fo, CAIRO_ANTIALIAS_SUBPIXEL);
	cairo_set_font_options(cc, fo);
	cairo_font_options_destroy(fo);
}

void updateFontInfo(pDevDesc dev, R_GE_gcontext *gc) {
	DOCDesc *pd = (DOCDesc *) dev->deviceSpecific;
	char *fontname;
	if( gc->fontface == 5 ) {
		fontname = strdup("Symbol");
	} else if( strlen( gc->fontfamily ) > 0 ) {
		fontname = strdup(gc->fontfamily);
	} else {
		fontname = strdup(pd->fontname);
	}
	pd->fi->fontname = fontname;

	cairo_t *cc;
	cc = pd->cr;
	const char *Cfontface= pd->fi->fontname;
	cairo_font_slant_t slant = CAIRO_FONT_SLANT_NORMAL;
	cairo_font_weight_t wght  = CAIRO_FONT_WEIGHT_NORMAL;
	if (gc->fontface==5) Cfontface=strdup("Symbol");

	if (gc->fontface==3 || gc->fontface==4) slant=CAIRO_FONT_SLANT_ITALIC;
	if (gc->fontface==2 || gc->fontface==4) wght=CAIRO_FONT_WEIGHT_BOLD;

	  cairo_select_font_face (cc, Cfontface, slant, wght);
	  set_cf_antialias(cc);

	cairo_set_font_size (cc, gc->cex * gc->ps );
}



int get_and_increment_idx(pDevDesc dev) {
	DOCDesc *pd = (DOCDesc *) dev->deviceSpecific;
	int id = pd->id;
	pd->id++;
	return id;
}


void register_element(pDevDesc dev) {
	DOCDesc *pd = (DOCDesc *) dev->deviceSpecific;
	int id = pd->id - 1;

	if( pd->elt_tracer->on ){
		if( pd->elt_tracer->isinit < 1 ){
			pd->elt_tracer->first_elt = id;
			pd->elt_tracer->last_elt = id;

			pd->elt_tracer->isinit = 1;
		} else {
			pd->elt_tracer->last_elt = id;
		}
	}
}


void closeFile( FILE *file){
	BEGIN_SUSPEND_INTERRUPTS;
	fflush(file);
	fclose(file);
	END_SUSPEND_INTERRUPTS;
}


/*
 * stolen from PicTeX code
 *
 * */
void DOC_ClipLine(double x0, double y0, double x1, double y1,
		pDevDesc dev)
{
	DOCDesc *ptd = (DOCDesc *) dev->deviceSpecific;
	ptd->clippedx0 = x0;
	ptd->clippedx1 = x1;
	ptd->clippedy0 = y0;
	ptd->clippedy1 = y1;

	if (( ptd->clippedx0 < dev->clipLeft && ptd->clippedx1 < dev->clipLeft ) ||
			(ptd->clippedx0 > dev->clipRight && ptd->clippedx1 > dev->clipRight) ||
			(ptd->clippedy0 < dev->clipBottom && ptd->clippedy1 < dev->clipBottom) ||
			(ptd->clippedy0 > dev->clipTop && ptd->clippedy1 > dev->clipTop)) {

		ptd->clippedx0 = ptd->clippedx1;
		ptd->clippedy0 = ptd->clippedy1;
		return;
	}

	/*Clipping Left */
	if (ptd->clippedx1 >= dev->clipLeft && ptd->clippedx0 < dev->clipLeft) {

		ptd->clippedy0 = ((ptd->clippedy1-ptd->clippedy0) /
				(ptd->clippedx1-ptd->clippedx0) *
				(dev->clipLeft-ptd->clippedx0)) +
						ptd->clippedy0;
		ptd->clippedx0 = dev->clipLeft;
	}
	if (ptd->clippedx1 <= dev->clipLeft && ptd->clippedx0 > dev->clipLeft) {
		ptd->clippedy1 = ((ptd->clippedy1-ptd->clippedy0) /
				(ptd->clippedx1-ptd->clippedx0) *
				(dev->clipLeft-ptd->clippedx0)) +
						ptd->clippedy0;
		ptd->clippedx1 = dev->clipLeft;
	}
	/* Clipping Right */
	if (ptd->clippedx1 >= dev->clipRight &&
			ptd->clippedx0 < dev->clipRight) {

		ptd->clippedy1 = ((ptd->clippedy1-ptd->clippedy0) /
				(ptd->clippedx1-ptd->clippedx0) *
				(dev->clipRight-ptd->clippedx0)) +
						ptd->clippedy0;
		ptd->clippedx1 = dev->clipRight;
	}
	if (ptd->clippedx1 <= dev->clipRight &&
			ptd->clippedx0 > dev->clipRight) {

		ptd->clippedy0 = ((ptd->clippedy1-ptd->clippedy0) /
				(ptd->clippedx1-ptd->clippedx0) *
				(dev->clipRight-ptd->clippedx0)) +
						ptd->clippedy0;
		ptd->clippedx0 = dev->clipRight;
	}
	/*Clipping Bottom */
	if (ptd->clippedy1 >= dev->clipBottom  &&
			ptd->clippedy0 < dev->clipBottom ) {
		ptd->clippedx0 = ((ptd->clippedx1-ptd->clippedx0) /
				(ptd->clippedy1-ptd->clippedy0) *
				(dev->clipBottom -ptd->clippedy0)) +
						ptd->clippedx0;
		ptd->clippedy0 = dev->clipBottom ;
	}
	if (ptd->clippedy1 <= dev->clipBottom &&
			ptd->clippedy0 > dev->clipBottom ) {
		ptd->clippedx1 = ((ptd->clippedx1-ptd->clippedx0) /
				(ptd->clippedy1-ptd->clippedy0) *
				(dev->clipBottom -ptd->clippedy0)) +
						ptd->clippedx0;
		ptd->clippedy1 = dev->clipBottom ;
	}
	/*Clipping Top */
	if (ptd->clippedy1 >= dev->clipTop  && ptd->clippedy0 < dev->clipTop ) {
		ptd->clippedx1 = ((ptd->clippedx1-ptd->clippedx0) /
				(ptd->clippedy1-ptd->clippedy0) *
				(dev->clipTop -ptd->clippedy0)) +
						ptd->clippedx0;
		ptd->clippedy1 = dev->clipTop ;
	}
	if (ptd->clippedy1 <= dev->clipTop && ptd->clippedy0 > dev->clipTop ) {
		ptd->clippedx0 = ((ptd->clippedx1-ptd->clippedx0) /
				(ptd->clippedy1-ptd->clippedy0) *
				(dev->clipTop -ptd->clippedy0)) +
						ptd->clippedx0;
		ptd->clippedy0 = dev->clipTop ;
	}
}

/*
 * stolen from PicTeX code
 *
 * */
void DOC_ClipRect(double x0, double y0, double x1, double y1,
		pDevDesc dev)
{
	DOCDesc *ptd = (DOCDesc *) dev->deviceSpecific;
	ptd->clippedx0 = x0;
	ptd->clippedx1 = x1;
	ptd->clippedy0 = y0;
	ptd->clippedy1 = y1;

	if (( ptd->clippedx0 < dev->clipLeft && ptd->clippedx1 < dev->clipLeft ) ||
			(ptd->clippedx0 > dev->clipRight && ptd->clippedx1 > dev->clipRight) ||
			(ptd->clippedy0 < dev->clipBottom && ptd->clippedy1 < dev->clipBottom) ||
			(ptd->clippedy0 > dev->clipTop && ptd->clippedy1 > dev->clipTop)) {
		ptd->clippedx0 = ptd->clippedx1;
		ptd->clippedy0 = ptd->clippedy1;
		return;
	}

	/*Clipping Left */
	if ( ptd->clippedx0 < dev->clipLeft && ptd->clippedx1 >= dev->clipLeft ) {
		ptd->clippedx0 = dev->clipLeft;
	}
	if ( ptd->clippedx0 > dev->clipLeft && ptd->clippedx1 <= dev->clipLeft ) {
		ptd->clippedx1 = dev->clipLeft;
	}
	/* Clipping Right */
	if ( ptd->clippedx0 < dev->clipRight && ptd->clippedx1 >= dev->clipRight ) {
		ptd->clippedx1 = dev->clipRight;
	}
	if ( ptd->clippedx0 > dev->clipRight && ptd->clippedx1 <= dev->clipRight ) {
		ptd->clippedx0 = dev->clipRight;
	}
	/*Clipping Bottom */
	if ( ptd->clippedy0 < dev->clipBottom && ptd->clippedy1 >= dev->clipBottom ) {
		ptd->clippedy0 = dev->clipBottom ;
	}
	if ( ptd->clippedy0 > dev->clipBottom && ptd->clippedy1 <= dev->clipBottom ) {
		ptd->clippedy1 = dev->clipBottom ;
	}
	/*Clipping Top */
	if ( ptd->clippedy0 < dev->clipTop && ptd->clippedy1 >= dev->clipTop ) {
		ptd->clippedy1 = dev->clipTop ;
	}
	if ( ptd->clippedy0 > dev->clipTop && ptd->clippedy1 <= dev->clipTop ) {
		ptd->clippedy0 = dev->clipTop ;
	}

}

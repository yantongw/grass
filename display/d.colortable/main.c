
/****************************************************************************
 *
 * MODULE:       d.colortable
 * AUTHOR(S):    James Westervelt, CERL (original contributor)
 *               Markus Neteler <neteler itc.it>,
 *               Bernhard Reiter <bernhard intevation.de>, 
 *               Eric G. Miller <egm2 jps.net>, 
 *               Glynn Clements <glynn gclements.plus.com>, 
 *               Hamish Bowman <hamish_nospam yahoo.com>, 
 *               Jan-Oliver Wagner <jan intevation.de>
 * PURPOSE:      display the color table associated with a raster map layer in
 *               the active frame on the graphics monitor
 * COPYRIGHT:    (C) 1999-2007 by the GRASS Development Team
 *
 *               This program is free software under the GNU General Public
 *               License (>=v2). Read the file COPYING that comes with GRASS
 *               for details.
 *
 *****************************************************************************/


#include <stdlib.h>
#include <math.h>
#include <grass/display.h>
#include <grass/gis.h>
#include <grass/raster.h>
#include <grass/glocale.h>

int main(int argc, char **argv)
{
    char *map_name;
    int color;
    int lines;
    int cols;

    struct FPRange fp_range;
    struct Colors colors;
    double ratio;
    DCELL dmin, dmax, dval;
    int cats_num;
    int cur_dot_row;
    int cur_dot_col;
    int dots_per_line;
    int dots_per_col;
    int atcat;
    int white;
    int black;
    int atcol;
    int atline;
    int count;
    double t, b, l, r;
    int fp, new_colr;
    double x_box[5];
    double y_box[5];
    struct GModule *module;
    struct Option *opt1, *opt2, *opt3, *opt4;

    /* Initialize the GIS calls */
    G_gisinit(argv[0]);

    module = G_define_module();
    module->keywords = _("display, setup");
    module->description =
	_("Displays the color table associated with a raster map layer.");

    opt1 = G_define_option();
    opt1->key = "map";
    opt1->type = TYPE_STRING;
    opt1->required = YES;
    opt1->gisprompt = "old,cell,raster";
    opt1->description = "Name of existing raster map";

    opt2 = G_define_option();
    opt2->key = "color";
    opt2->type = TYPE_STRING;
    opt2->answer = DEFAULT_FG_COLOR;
    opt2->gisprompt = GISPROMPT_COLOR;
    opt2->description =
	"Color of lines separating the colors of the color table";

    opt3 = G_define_option();
    opt3->key = "lines";
    opt3->type = TYPE_INTEGER;
    opt3->options = "1-1000";
    opt3->description = "Number of lines";

    opt4 = G_define_option();
    opt4->key = "cols";
    opt4->type = TYPE_INTEGER;
    opt4->options = "1-1000";
    opt4->description = "Number of columns";

    /* Check command line */
    if (G_parser(argc, argv))
	exit(EXIT_FAILURE);

    map_name = opt1->answer;
    fp = G_raster_map_is_fp(map_name, "");

    if (opt2->answer != NULL) {
	new_colr = D_translate_color(opt2->answer);
	color = new_colr;
    }

    if (fp)
	lines = 1;
    else
	lines = 0;
    if (opt3->answer != NULL) {
	if (fp)
	    G_warning("<%s> is floating-point; ignoring lines and drawing continuous color ramp",
		      map_name);
	else
	    sscanf(opt3->answer, "%d", &lines);
    }

    if (fp)
	cols = 1;
    else
	cols = 0;
    if (opt4->answer) {
	if (fp)
	    G_warning("<%s> is floating-point; ignoring cols and drawing continuous color ramp",
		      map_name);
	else
	    sscanf(opt4->answer, "%d", &cols);
    }

    /* Make sure map is available */
    if (G_read_colors(map_name, "", &colors) == -1)
	G_fatal_error(_("Color file for <%s> not available"), map_name);
    if (G_read_fp_range(map_name, "", &fp_range) == -1)
	G_fatal_error(_("Range file for <%s> not available"), map_name);
    if (R_open_driver() != 0)
	G_fatal_error(_("No graphics device selected"));

    D_setup_unity(0);
    D_get_src(&t, &b, &l, &r);

    G_get_fp_range_min_max(&fp_range, &dmin, &dmax);
    if (G_is_d_null_value(&dmin) || G_is_d_null_value(&dmax))
	G_fatal_error("Data range is empty");
    cats_num = (int)dmax - (int)dmin + 1;
    if (lines <= 0 && cols <= 0) {
	double dx, dy;

	dy = (double)(b - t);
	dx = (double)(r - l);
	ratio = dy / dx;
	cols = 1 + sqrt((dmax - dmin + 1.) / ratio);
	lines = 1 + cats_num / cols;
    }
    else if (lines > 0 && cols <= 0) {
	cols = 1 + cats_num / lines;
    }
    else if (cols > 0 && lines <= 0) {
	lines = 1 + cats_num / cols;
    }
    /* otherwise, accept without complaint what the user requests
     * It is possible that the number of lines and cols is not
     * sufficient for the number of categories.
     */

    dots_per_line = (b - t) / lines;
    dots_per_col = (r - l) / cols;

    x_box[0] = 0;			y_box[0] = 0;
    x_box[1] = 0;			y_box[1] = (6 - dots_per_line);
    x_box[2] = (dots_per_col - 6);	y_box[2] = 0;
    x_box[3] = 0;			y_box[3] = (dots_per_line - 6);
    x_box[4] = (6 - dots_per_col);	y_box[4] = 0;

    white = D_translate_color("white");
    black = D_translate_color("black");
    G_set_c_null_value(&atcat, 1);
    if (!fp) {
	for (atcol = 0; atcol < cols; atcol++) {
	    cur_dot_row = t;
	    cur_dot_col = l + atcol * dots_per_col;
	    count = 0;
	    for (atline = 0; atline < lines; atline++) {
		cur_dot_row += dots_per_line;
		/* Draw white box */
		D_use_color(color);
		D_begin();
		D_move_abs(cur_dot_col + 2, (cur_dot_row - 1));
		D_cont_rel(0, (2 - dots_per_line));
		D_cont_rel((dots_per_col - 2), 0);
		D_cont_rel(0, (dots_per_line - 2));
		D_cont_rel((2 - dots_per_col), 0);
		D_end();
		D_stroke();
		/* Draw black box */
		D_use_color(black);
		D_begin();
		D_move_abs(cur_dot_col + 3, (cur_dot_row - 2));
		D_cont_rel(0, (4 - dots_per_line));
		D_cont_rel((dots_per_col - 4), 0);
		D_cont_rel(0, (dots_per_line - 4));
		D_cont_rel((4 - dots_per_col), 0);
		D_end();
		D_stroke();
		/* Color box */
		D_color((CELL) atcat, &colors);
		D_pos_abs(cur_dot_col + 4, (cur_dot_row - 3));
		D_polygon_rel(x_box, y_box, 5);

		count++;
		/* first cat number is null value */
		if (count == 1)
		    atcat = (int)dmin;
		else if (++atcat > (int)dmax)
		    break;
	    }
	    if (atcat > (int)dmax)
		break;
	}			/* col loop */
    }				/* int map */
    else {			/* draw continuous color ramp for fp map */

	cur_dot_row = t + dots_per_line;
	cur_dot_col = l;
	/* Draw white box */
	D_use_color(color);
	D_begin();
	D_move_abs(cur_dot_col + 2, (cur_dot_row - 1));
	D_cont_rel(0, (2 - dots_per_line));
	D_cont_rel((dots_per_col - 2), 0);
	D_cont_rel(0, (dots_per_line - 2));
	D_cont_rel((2 - dots_per_col), 0);
	D_end();
	D_stroke();
	/* Draw black box */
	D_use_color(black);
	D_begin();
	D_move_abs(cur_dot_col + 3, (cur_dot_row - 2));
	D_cont_rel(0, (4 - dots_per_line));
	D_cont_rel((dots_per_col - 4), 0);
	D_cont_rel(0, (dots_per_line - 4));
	D_cont_rel((4 - dots_per_col), 0);
	D_end();
	D_stroke();
	/* Color ramp box */

	/* get separate color for each pixel */
	/* fisrt 5 pixels draw null color */
	y_box[1] = -1;
	y_box[3] = 1;
	fprintf(stdout, "dots_per_line: %d\n", dots_per_line);
	for (r = 0; r < dots_per_line - 6; r++) {
	    if (r <= 4)
		G_set_d_null_value(&dval, 1);
	    else
		dval =
		    dmin + (r - 1) * (dmax - dmin) / (dots_per_line - 6 - 5);
	    D_d_color(dval, &colors);
	    D_pos_abs(cur_dot_col + 4, (cur_dot_row - 3) - r);
	    D_polygon_rel(x_box, y_box, 5);
	}
    }

    R_close_driver();

    return 0;
}

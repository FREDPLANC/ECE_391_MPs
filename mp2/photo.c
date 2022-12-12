/*									tab:8
 *
 * photo.c - photo display functions
 *
 * "Copyright (c) 2011 by Steven S. Lumetta."
 *
 * Permission to use, copy, modify, and distribute this software and its
 * documentation for any purpose, without fee, and without written agreement is
 * hereby granted, provided that the above copyright notice and the following
 * two paragraphs appear in all copies of this software.
 * 
 * IN NO EVENT SHALL THE AUTHOR OR THE UNIVERSITY OF ILLINOIS BE LIABLE TO 
 * ANY PARTY FOR DIRECT, INDIRECT, SPECIAL, INCIDENTAL, OR CONSEQUENTIAL 
 * DAMAGES ARISING OUT  OF THE USE OF THIS SOFTWARE AND ITS DOCUMENTATION, 
 * EVEN IF THE AUTHOR AND/OR THE UNIVERSITY OF ILLINOIS HAS BEEN ADVISED 
 * OF THE POSSIBILITY OF SUCH DAMAGE.
 * 
 * THE AUTHOR AND THE UNIVERSITY OF ILLINOIS SPECIFICALLY DISCLAIM ANY 
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF 
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.  THE SOFTWARE 
 * PROVIDED HEREUNDER IS ON AN "AS IS" BASIS, AND NEITHER THE AUTHOR NOR
 * THE UNIVERSITY OF ILLINOIS HAS ANY OBLIGATION TO PROVIDE MAINTENANCE, 
 * SUPPORT, UPDATES, ENHANCEMENTS, OR MODIFICATIONS."
 *
 * Author:	    Steve Lumetta
 * Version:	    3
 * Creation Date:   Fri Sep  9 21:44:10 2011
 * Filename:	    photo.c
 * History:
 *	SL	1	Fri Sep  9 21:44:10 2011
 *		First written (based on mazegame code).
 *	SL	2	Sun Sep 11 14:57:59 2011
 *		Completed initial implementation of functions.
 *	SL	3	Wed Sep 14 21:49:44 2011
 *		Cleaned up code for distribution.
 */


#include <string.h>

#include "assert.h"
#include "modex.h"
#include "photo.h"
#include "photo_headers.h"
#include "world.h"


/* types local to this file (declared in types.h) */

/* 
 * A room photo.  Note that you must write the code that selects the
 * optimized palette colors and fills in the pixel data using them as 
 * well as the code that sets up the VGA to make use of these colors.
 * Pixel data are stored as one-byte values starting from the upper
 * left and traversing the top row before returning to the left of
 * the second row, and so forth.  No padding should be used.
 */
struct photo_t {
    photo_header_t hdr;			/* defines height and width */
    uint8_t        palette[192][3];     /* optimized palette colors */
    uint8_t*       img;                 /* pixel data               */
};

/* 
 * An object image.  The code for managing these images has been given
 * to you.  The data are simply loaded from a file, where they have 
 * been stored as 2:2:2-bit RGB values (one byte each), including 
 * transparent pixels (value OBJ_CLR_TRANSP).  As with the room photos, 
 * pixel data are stored as one-byte values starting from the upper 
 * left and traversing the top row before returning to the left of the 
 * second row, and so forth.  No padding is used.
 */
struct image_t {
    photo_header_t hdr;			/* defines height and width */
    uint8_t*       img;                 /* pixel data               */
};


/* this is the struct defined for the node, the element green stores the total RGB of the color, and the counter record the how many color are there in a node.*/

struct oct_node_t {
	int all_red;
	int all_green;
	int all_blue;
	int counter;  
	unsigned int index;
	unsigned int color;
};

/* file-scope variables */

/* 
 * The room currently shown on the screen.  This value is not known to 
 * the mode X code, but is needed when filling buffers in callbacks from 
 * that code (fill_horiz_buffer/fill_vert_buffer).  The value is set 
 * by calling prep_room.
 */
static const room_t* cur_room = NULL; 


/* 
 * fill_horiz_buffer
 *   DESCRIPTION: Given the (x,y) map pixel coordinate of the leftmost 
 *                pixel of a line to be drawn on the screen, this routine 
 *                produces an image of the line.  Each pixel on the line
 *                is represented as a single byte in the image.
 *
 *                Note that this routine draws both the room photo and
 *                the objects in the room.
 *
 *   INPUTS: (x,y) -- leftmost pixel of line to be drawn 
 *   OUTPUTS: buf -- buffer holding image data for the line
 *   RETURN VALUE: none
 *   SIDE EFFECTS: none
 */
void
fill_horiz_buffer (int x, int y, unsigned char buf[SCROLL_X_DIM])
{
    int            idx;   /* loop index over pixels in the line          */ 
    object_t*      obj;   /* loop index over objects in the current room */
    int            imgx;  /* loop index over pixels in object image      */ 
    int            yoff;  /* y offset into object image                  */ 
    uint8_t        pixel; /* pixel from object image                     */
    const photo_t* view;  /* room photo                                  */
    int32_t        obj_x; /* object x position                           */
    int32_t        obj_y; /* object y position                           */
    const image_t* img;   /* object image                                */

    /* Get pointer to current photo of current room. */
    view = room_photo (cur_room);

    /* Loop over pixels in line. */
    for (idx = 0; idx < SCROLL_X_DIM; idx++) {
        buf[idx] = (0 <= x + idx && view->hdr.width > x + idx ?
		    view->img[view->hdr.width * y + x + idx] : 0);
    }

    /* Loop over objects in the current room. */
    for (obj = room_contents_iterate (cur_room); NULL != obj;
    	 obj = obj_next (obj)) {
	obj_x = obj_get_x (obj);
	obj_y = obj_get_y (obj);
	img = obj_image (obj);

        /* Is object outside of the line we're drawing? */
	if (y < obj_y || y >= obj_y + img->hdr.height ||
	    x + SCROLL_X_DIM <= obj_x || x >= obj_x + img->hdr.width) {
	    continue;
	}

	/* The y offset of drawing is fixed. */
	yoff = (y - obj_y) * img->hdr.width;

	/* 
	 * The x offsets depend on whether the object starts to the left
	 * or to the right of the starting point for the line being drawn.
	 */
	if (x <= obj_x) {
	    idx = obj_x - x;
	    imgx = 0;
	} else {
	    idx = 0;
	    imgx = x - obj_x;
	}

	/* Copy the object's pixel data. */
	for (; SCROLL_X_DIM > idx && img->hdr.width > imgx; idx++, imgx++) {
	    pixel = img->img[yoff + imgx];

	    /* Don't copy transparent pixels. */
	    if (OBJ_CLR_TRANSP != pixel) {
		buf[idx] = pixel;
	    }
	}
    }
}


/* 
 * fill_vert_buffer
 *   DESCRIPTION: Given the (x,y) map pixel coordinate of the top pixel of 
 *                a vertical line to be drawn on the screen, this routine 
 *                produces an image of the line.  Each pixel on the line
 *                is represented as a single byte in the image.
 *
 *                Note that this routine draws both the room photo and
 *                the objects in the room.
 *
 *   INPUTS: (x,y) -- top pixel of line to be drawn 
 *   OUTPUTS: buf -- buffer holding image data for the line
 *   RETURN VALUE: none
 *   SIDE EFFECTS: none
 */
void
fill_vert_buffer (int x, int y, unsigned char buf[SCROLL_Y_DIM])
{
    int            idx;   /* loop index over pixels in the line          */ 
    object_t*      obj;   /* loop index over objects in the current room */
    int            imgy;  /* loop index over pixels in object image      */ 
    int            xoff;  /* x offset into object image                  */ 
    uint8_t        pixel; /* pixel from object image                     */
    const photo_t* view;  /* room photo                                  */
    int32_t        obj_x; /* object x position                           */
    int32_t        obj_y; /* object y position                           */
    const image_t* img;   /* object image                                */

    /* Get pointer to current photo of current room. */
    view = room_photo (cur_room);

    /* Loop over pixels in line. */
    for (idx = 0; idx < SCROLL_Y_DIM; idx++) {
        buf[idx] = (0 <= y + idx && view->hdr.height > y + idx ?
		    view->img[view->hdr.width * (y + idx) + x] : 0);
    }

    /* Loop over objects in the current room. */
    for (obj = room_contents_iterate (cur_room); NULL != obj;
    	 obj = obj_next (obj)) {
	obj_x = obj_get_x (obj);
	obj_y = obj_get_y (obj);
	img = obj_image (obj);

        /* Is object outside of the line we're drawing? */
	if (x < obj_x || x >= obj_x + img->hdr.width ||
	    y + SCROLL_Y_DIM <= obj_y || y >= obj_y + img->hdr.height) {
	    continue;
	}

	/* The x offset of drawing is fixed. */
	xoff = x - obj_x;

	/* 
	 * The y offsets depend on whether the object starts below or 
	 * above the starting point for the line being drawn.
	 */
	if (y <= obj_y) {
	    idx = obj_y - y;
	    imgy = 0;
	} else {
	    idx = 0;
	    imgy = y - obj_y;
	}

	/* Copy the object's pixel data. */
	for (; SCROLL_Y_DIM > idx && img->hdr.height > imgy; idx++, imgy++) {
	    pixel = img->img[xoff + img->hdr.width * imgy];

	    /* Don't copy transparent pixels. */
	    if (OBJ_CLR_TRANSP != pixel) {
		buf[idx] = pixel;
	    }
	}
    }
}


/* 
 * image_height
 *   DESCRIPTION: Get height of object image in pixels.
 *   INPUTS: im -- object image pointer
 *   OUTPUTS: none
 *   RETURN VALUE: height of object image im in pixels
 *   SIDE EFFECTS: none
 */
uint32_t 
image_height (const image_t* im)
{
    return im->hdr.height;
}


/* 
 * image_width
 *   DESCRIPTION: Get width of object image in pixels.
 *   INPUTS: im -- object image pointer
 *   OUTPUTS: none
 *   RETURN VALUE: width of object image im in pixels
 *   SIDE EFFECTS: none
 */
uint32_t 
image_width (const image_t* im)
{
    return im->hdr.width;
}

/* 
 * photo_height
 *   DESCRIPTION: Get height of room photo in pixels.
 *   INPUTS: p -- room photo pointer
 *   OUTPUTS: none
 *   RETURN VALUE: height of room photo p in pixels
 *   SIDE EFFECTS: none
 */
uint32_t 
photo_height (const photo_t* p)
{
    return p->hdr.height;
}


/* 
 * photo_width
 *   DESCRIPTION: Get width of room photo in pixels.
 *   INPUTS: p -- room photo pointer
 *   OUTPUTS: none
 *   RETURN VALUE: width of room photo p in pixels
 *   SIDE EFFECTS: none
 */
uint32_t 
photo_width (const photo_t* p)
{
    return p->hdr.width;
}


/* 
 * prep_room
 *   DESCRIPTION: Prepare a new room for display.  You might want to set
 *                up the VGA palette registers according to the color
 *                palette that you chose for this room.
 *   INPUTS: r -- pointer to the new room
 *   OUTPUTS: none
 *   RETURN VALUE: none
 *   SIDE EFFECTS: changes recorded cur_room for this file
 */
void
prep_room (const room_t* r) //////////////////////////////////////////////// This is not sure for this
{
    /* Record the current room. */
    cur_room = r;

	pre_palette((room_photo(cur_room))->palette);  // to move the palette into the VGA
	 
	return;
}


/* 
 * read_obj_image
 *   DESCRIPTION: Read size and pixel data in 2:2:2 RGB format from a
 *                photo file and create an image structure from it.
 *   INPUTS: fname -- file name for input
 *   OUTPUTS: none
 *   RETURN VALUE: pointer to newly allocated photo on success, or NULL
 *                 on failure
 *   SIDE EFFECTS: dynamically allocates memory for the image
 */
image_t*
read_obj_image (const char* fname)
{
    FILE*    in;		/* input file               */
    image_t* img = NULL;	/* image structure          */
    uint16_t x;			/* index over image columns */
    uint16_t y;			/* index over image rows    */
    uint8_t  pixel;		/* one pixel from the file  */

    /* 
     * Open the file, allocate the structure, read the header, do some
     * sanity checks on it, and allocate space to hold the image pixels.
     * If anything fails, clean up as necessary and return NULL.
     */
    if (NULL == (in = fopen (fname, "r+b")) ||
	NULL == (img = malloc (sizeof (*img))) ||
	NULL != (img->img = NULL) || /* false clause for initialization */
	1 != fread (&img->hdr, sizeof (img->hdr), 1, in) ||
	MAX_OBJECT_WIDTH < img->hdr.width ||
	MAX_OBJECT_HEIGHT < img->hdr.height ||
	NULL == (img->img = malloc 
		 (img->hdr.width * img->hdr.height * sizeof (img->img[0])))) {
	if (NULL != img) {
	    if (NULL != img->img) {
	        free (img->img);
	    }
	    free (img);
	}
	if (NULL != in) {
	    (void)fclose (in);
	}

	return NULL;
    }

    /* 
     * Loop over rows from bottom to top.  Note that the file is stored
     * in this order, whereas in memory we store the data in the reverse
     * order (top to bottom).
     */
    for (y = img->hdr.height; y-- > 0; ) {

	/* Loop over columns from left to right. */
	for (x = 0; img->hdr.width > x; x++) {

	    /* 
	     * Try to read one 8-bit pixel.  On failure, clean up and 
	     * return NULL.
	     */
	    if (1 != fread (&pixel, sizeof (pixel), 1, in)) {
		free (img->img);
		free (img);
	        (void)fclose (in);
		return NULL;
	    }

	    /* Store the pixel in the image data. */
	    img->img[img->hdr.width * y + x] = pixel;
	}
    }

    /* All done.  Return success. */
    (void)fclose (in);
    return img;
}


/* 
 * read_photo
 *   DESCRIPTION: Read size and pixel data in 5:6:5 RGB format from a
 *                photo file and create a photo structure from it.
 *                Code provided simply maps to 2:2:2 RGB.  You must
 *                replace this code with palette color selection, and
 *                must map the image pixels into the palette colors that
 *                you have defined.
 *   INPUTS: fname -- file name for input
 *   OUTPUTS: none
 *   RETURN VALUE: pointer to newly allocated photo on success, or NULL
 *                 on failure
 *   SIDE EFFECTS: dynamically allocates memory for the photo
 */
photo_t*
read_photo (const char* fname)
{
    FILE*    in;	/* input file               */
    photo_t* p = NULL;	/* photo structure          */
    uint16_t x;		/* index over image columns */
    uint16_t y;		/* index over image rows    */
    uint16_t pixel;	/* one pixel from the file  */

    /* 
     * Open the file, allocate the structure, read the header, do some
     * sanity checks on it, and allocate space to hold the photo pixels.
     * If anything fails, clean up as necessary and return NULL.
     */
    if (NULL == (in = fopen (fname, "r+b")) ||
	NULL == (p = malloc (sizeof (*p))) ||
	NULL != (p->img = NULL) || /* false clause for initialization */
	1 != fread (&p->hdr, sizeof (p->hdr), 1, in) ||
	MAX_PHOTO_WIDTH < p->hdr.width ||
	MAX_PHOTO_HEIGHT < p->hdr.height ||
	NULL == (p->img = malloc 
		 (p->hdr.width * p->hdr.height * sizeof (p->img[0])))) {
	if (NULL != p) {
	    if (NULL != p->img) {
	        free (p->img);
	    }
	    free (p);
	}
	if (NULL != in) {
	    (void)fclose (in);
	}
	return NULL;
    }

    /* 
     * Loop over rows from bottom to top.  Note that the file is stored
     * in this order, whereas in memory we store the data in the reverse
     * order (top to bottom).
     */

	init_arrays();   // this is used for initializing arrays

    for (y = p->hdr.height; y-- > 0; ) {

	/* Loop over columns from left to right. */
	for (x = 0; p->hdr.width > x; x++) {

	    /* 
	     * Try to read one 16-bit pixel.  On failure, clean up and 
	     * return NULL.
	     */
	    if (1 != fread (&pixel, sizeof (pixel), 1, in)) {
		free (p->img);
		free (p);
	        (void)fclose (in);
		return NULL;

	    }
	    /* 
	     * 16-bit pixel is coded as 5:6:5 RGB (5 bits red, 6 bits green,
	     * and 6 bits blue).  We change to 2:2:2, which we've set for the
	     * game objects.  You need to use the other 192 palette colors
	     * to specialize the appearance of each photo.
	     *
	     * In this code, you need to calculate the p->palette values,
	     * which encode 6-bit RGB as arrays of three uint8_t's.  When
	     * the game puts up a photo, you should then change the palette 
	     * to match the colors needed for that photo.
	     */
		
		fill_level_array(pixel);
		/*
	    p->img[p->hdr.width * y + x] = (((pixel >> 14) << 4) |
					    (((pixel >> 9) & 0x3) << 2) |
					    ((pixel >> 3) & 0x3));
		*/
	}
    }
	// averaging_colors(); // now the fill job is done, then averaging the colors in it
	averaging_colors();
	assign_palette(p->palette); // then averaging the color and set the palette
	fseek(in, 0, SEEK_SET);
	// the below are the second scan to assign VGA to pixels
	for (y = p->hdr.height; y-- > 0; ) {

		/* Loop over columns from left to right. */
		for (x = 0; p->hdr.width > x; x++) {

			/* 
			* Try to read one 16-bit pixel.  On failure, clean up and 
			* return NULL.
			*/
			if (1 != fread (&pixel, sizeof (pixel), 1, in)) {
			free (p->img);
			free (p);
				(void)fclose (in);
			return NULL;

			}
			/* 
			* 16-bit pixel is coded as 5:6:5 RGB (5 bits red, 6 bits green,
			* and 6 bits blue).  We change to 2:2:2, which we've set for the
			* game objects.  You need to use the other 192 palette colors
			* to specialize the appearance of each photo.
			*
			* In this code, you need to calculate the p->palette values,
			* which encode 6-bit RGB as arrays of three uint8_t's.  When
			* the game puts up a photo, you should then change the palette 
			* to match the colors needed for that photo.
			*/
			
			/*
			p->img[p->hdr.width * y + x] = (((pixel >> 14) << 4) |
							(((pixel >> 9) & 0x3) << 2) |
							((pixel >> 3) & 0x3));
			*/
			p->img[p->hdr.width * y + x -2] = assign_VGA_index(pixel); // assign the pixels bits by bits
		}
	}
    /* All done.  Return success. */
    (void)fclose (in);
    return p;
}

oct_node_t secondLevel[64];
oct_node_t fourthLevel[64*64]; 
oct_node_t selected_fourthLevel[128]; // the 128 colors

/* 
 * init_arrays
 *   DESCRIPTION: init the 4-level and 2-level arrays
 *   INPUTS: none
 *   OUTPUTS: none
 *   RETURN VALUE: none
 *   SIDE EFFECTS: 
 */
void init_arrays(void){
	int i,j,k;  // set all the below to zeros
	for (i = 0; i < 64; i++){
		secondLevel[i].all_blue = 0;
		secondLevel[i].all_green = 0;
		secondLevel[i].all_red = 0;
		secondLevel[i].counter = 0;
		secondLevel[i].color = 0;
		secondLevel[i].index = 0;
	}
	for (j = 0; j < 64*64; j++){  // this is for the 4096 level2 array
		fourthLevel[j].all_blue = 0;
		fourthLevel[j].all_green = 0;
		fourthLevel[j].all_red = 0;
		fourthLevel[j].counter = 0;
		fourthLevel[j].color = 0;
		fourthLevel[j].index = 0;

	}
	for (k = 0; k < 128; k++){ // this is for the front 128 array
		selected_fourthLevel[k].all_blue = 0;
		selected_fourthLevel[k].all_green = 0;
		selected_fourthLevel[k].all_red = 0;
		selected_fourthLevel[k].counter = 0;
		selected_fourthLevel[k].color = 0;
		selected_fourthLevel[k].index = 0;
	}
	return;
}
/* 
 * fill_level_array
 *   DESCRIPTION: When a pixel comes in, get its color and fill the 2level and 4level array
 *   INPUTS: pixel
 *   OUTPUTS: none
 *   RETURN VALUE: none
 *   SIDE EFFECTS: fill the array secondLevel and fourthLevel
 */


void fill_level_array(uint16_t pixel){
	unsigned int red = (pixel >> 11) & (0x1F); // since the R:G:B = 5:6:5, JUST TAKE THEM OUT 
	unsigned int green = (pixel >> 5) & (0x3F); 
	unsigned int blue = (pixel) & (0x1F);
	unsigned int index_second = ((red >> 3) << 4) | ((green >> 4) << 2) | (blue >> 3);
	unsigned int index_fourth = ((red >> 1) << 8) | ((green >> 2) << 4) | (blue >> 1);

	// first fill the second-level array
	secondLevel[index_second].all_red   += red;
	secondLevel[index_second].all_green += green;
	secondLevel[index_second].all_blue  +=  blue;
	secondLevel[index_second].counter   += 1;
	secondLevel[index_second].color = index_second;
	secondLevel[index_second].index = index_second;
	

	// secondly fill the fourth-level array
	fourthLevel[index_fourth].all_red   += red;
	fourthLevel[index_fourth].all_green += green;
	fourthLevel[index_fourth].all_blue  += blue;
	fourthLevel[index_fourth].counter   += 1;
	fourthLevel[index_fourth].color = index_fourth;
	fourthLevel[index_fourth].index = index_fourth;
	
	return;
}
/* 
 * compare_standard
 *   DESCRIPTION: used for qsort, this is standard function for comapre
 *   INPUTS: *a,*b, which should be the struct pointer
 *   
 *   RETURN VALUE: 0,1; 0 if a>b
 *   SIDE EFFECTS: compare the counter of two pointer's
 */
int compare_standard(const void *a, const void *b){  // define the standard for the qsort
	if( (*(oct_node_t*)a).counter > (*(oct_node_t*)b).counter){
		return 0;
	}
	else{
		return 1;
	}
}

/* 
 * averaging
 *   DESCRIPTION: average the colors in the arrays
 *   INPUTS: none
 *   OUTPUTS: none
 *   RETURN VALUE: none
 *   SIDE EFFECTS: remove the affected 2level color, remove the contribution as required
 */
void averaging_colors(void){
	int i,j;
	qsort(fourthLevel,64*64,sizeof(oct_node_t),compare_standard);
	// pick the first 128 colors
	// averaging the color of the fourth_level
	// firstly take the 128 top color chosen
	for(j = 0;j < 128; j++){
		if(fourthLevel[j].counter == 0){
			continue;
		}
		selected_fourthLevel[j].all_red = ((fourthLevel[j].all_red) / fourthLevel[j].counter);
		selected_fourthLevel[j].all_green = ((fourthLevel[j].all_green) / fourthLevel[j].counter);
		selected_fourthLevel[j].all_blue = ((fourthLevel[j].all_blue) / fourthLevel[j].counter);
		selected_fourthLevel[j].color = fourthLevel[j].color;
		//selected_fourthLevel[j].color = ((selected_fourthLevel[j].all_red >> 1) << 8) | ((selected_fourthLevel[j].all_green >> 2) << 4) | (selected_fourthLevel[j].all_blue >> 1);

		// remove the contribution of level4
		unsigned int red = selected_fourthLevel[j].all_red >> 3;
		unsigned int green = selected_fourthLevel[j].all_green >> 4;
		unsigned int blue = selected_fourthLevel[j].all_blue >> 3;
		int index_second;
		index_second  = (red << 4) | (green << 2) | blue;   // find the index of the level 2

		secondLevel[index_second].all_red = secondLevel[index_second].all_red - fourthLevel[j].all_red;  // remove them from averaging
		secondLevel[index_second].all_green = secondLevel[index_second].all_green - fourthLevel[j].all_green;
		secondLevel[index_second].all_blue = secondLevel[index_second].all_blue - fourthLevel[j].all_blue;
		secondLevel[index_second].counter = secondLevel[index_second].counter - fourthLevel[j].counter;
	}
	// then averaging the color of second_level
	for(i = 0;i < 64; i++){
		if(secondLevel[i].counter == 0){
			continue;
		}
		secondLevel[i].all_red = ((secondLevel[i].all_red) / (secondLevel[i].counter));
		secondLevel[i].all_green = ((secondLevel[i].all_green) / (secondLevel[i].counter));
		secondLevel[i].all_blue = ((secondLevel[i].all_blue) / (secondLevel[i].counter));

	}
	return;
}

/* 
 * assign_palette
 *   DESCRIPTION: put optimized color into the palette array
 *   INPUTS: palette
 *   OUTPUTS: none
 *   RETURN VALUE: none
 *   SIDE EFFECTS: 128 first, then 64
 */
void assign_palette(uint8_t palette[192][3]){
	//averaging_colors(); // first average all the colors in the array and sort the fourth-level
	int i,j;
	// to assgin colors into the palette, the first 64 are the assigned values, in the this palette, 128 colors come first, then 64 colors for fourth-level.
	for(i = 0; i < 128; i++){
		palette[i][0] = (uint8_t)(selected_fourthLevel[i].all_red & FiveBitsMask) << 1;
		palette[i][1] = (uint8_t)(selected_fourthLevel[i].all_green & SixBitsMask);
		palette[i][2] = (uint8_t)(selected_fourthLevel[i].all_blue & FiveBitsMask) << 1;
	}
	for(j = 0; j < 64; j++){
		palette[j+128][0] = (uint8_t)(secondLevel[j].all_red & FiveBitsMask) << 1;
		palette[j+128][1] = (uint8_t)(secondLevel[j].all_green & SixBitsMask);
		palette[j+128][2] = (uint8_t)(secondLevel[j].all_blue & FiveBitsMask) << 1;
	}
	return;

}

/* 
 * assign_VGA_index
 *   DESCRIPTION: compute optimized VGA index
 *   INPUTS: pixel
 *   OUTPUTS: none
 *   RETURN VALUE: none
 *   SIDE EFFECTS: none
 */
uint8_t assign_VGA_index(uint16_t pixel){
	uint8_t i;
	unsigned int red = (pixel >> 11) & (FiveBitsMask); // since the R:G:B = 5:6:5, JUST TAKE THEM OUT 
	unsigned int green = (pixel >> 5) & (SixBitsMask); 
	unsigned int blue = (pixel) & (FiveBitsMask);
	int index_second = ((red >> 3) << 4) | ((green >> 4) << 2) | (blue >> 3); // get the color value
	int index_fourth = ((red >> 1) << 8) | ((green >> 2) << 4) | (blue >> 1);
	int VGA;
	for(i = 0; i < 128; i++){
		if((index_fourth) == selected_fourthLevel[i].color){
			VGA = i + 64;
			return VGA;
		}
	}
	/*
	for(j = 0; j < 64; j++){
		if((index_second) == secondLevel[j].color){
			VGA = 128 + 64 + j;
			return VGA;
		}
	}
	*/
	VGA = index_second + 128 + 64;   // for the rest , we assume that the rest are belonging to the second level
	return VGA;
	// return 0;

}

#include "pngfile.h"

int pngSteamTool::readPNGFile(string filepath, pic_data* out)
/* Used to decode PNG images */
{
	FILE* pic_fp;
	pic_fp = fopen(filepath.c_str(), "rb");
	if (!pic_fp) return -1;/* Failed to open file */
		

	/* Initialize various structures */
	png_structp png_ptr;
	png_infop   info_ptr;
	char        buf[PNG_BYTES_TO_CHECK];
	int         temp;

	png_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING, 0, 0, 0);
	info_ptr = png_create_info_struct(png_ptr);

	setjmp(png_jmpbuf(png_ptr)); // This sentence is very important.

	temp = fread(buf, 1, PNG_BYTES_TO_CHECK, pic_fp);
	temp = png_sig_cmp((png_const_bytep)buf, (png_size_t)0, PNG_BYTES_TO_CHECK);

	/* Check if it is a png file */
	if (temp != 0) return 1;

	rewind(pic_fp);
	/* start reading the file */
	png_init_io(png_ptr, pic_fp);
	// read file
	png_read_png(png_ptr, info_ptr, PNG_TRANSFORM_EXPAND, 0);

	int color_type, channels;

	/*Get width, height, bit depth, color type */
	channels = png_get_channels(png_ptr, info_ptr); /* Get number of channels */

	out->bit_depth = png_get_bit_depth(png_ptr, info_ptr); /* Get bit depth */
	color_type = png_get_color_type(png_ptr, info_ptr); /*color type*/

	int i, j;
	int size;
	/* row_pointers inside are rgba data */
	png_bytep* row_pointers;
	row_pointers = png_get_rows(png_ptr, info_ptr);
	out->width = png_get_image_width(png_ptr, info_ptr);
	out->height = png_get_image_height(png_ptr, info_ptr);

	size = out->width * out->height; /* Calculate the total number of pixels in the image */

	if (channels == 4 || color_type == PNG_COLOR_TYPE_RGB_ALPHA)
	{/*if RGB+alpha channel or RGB+other bytes*/
		size *= (4 * sizeof(unsigned char)); /* Each pixel occupies 4 bytes of memory */
		out->flag = HAVE_ALPHA;    /* flags */
		out->rgba = (unsigned char*)malloc(size);
		if (out->rgba == NULL)
		{/* If memory allocation fails */
			fclose(pic_fp);
			printf("\r[*] PNG Fail :  Failed to allocate memory to store data.\n");
			return 1;
		}

		temp = (4 * out->width);/* each line is 4 * out->width bytes */
		for (i = 0; i < out->height; i++)
		{
			for (j = 0; j < temp; j += 4)
			{/* Allocate bytes */
				out->rgba[i * temp + j] = row_pointers[i][j + 2];       // red
				out->rgba[i * temp + j + 1] = row_pointers[i][j + 1];   // green
				out->rgba[i * temp + j + 2] = row_pointers[i][j];   // blue
				out->rgba[i * temp + j + 3] = row_pointers[i][j + 3];   // alpha
			}
		}
	}
	else if (channels == 3 || color_type == PNG_COLOR_TYPE_RGB)
	{/* For RGB channels */
		size *= (3 * sizeof(unsigned char)); /* Each pixel occupies 3 bytes of memory. */
		out->flag = NO_ALPHA;    /* flags */
		out->rgba = (unsigned char*)malloc(size);
		memset(out->rgba, 0, size);
		if (out->rgba == NULL)
		{/* If memory allocation fails */
			fclose(pic_fp);
			printf("\r[*] PNG Fail :  Failed to allocate memory to store data.\n");
			return 1;
		}

		temp = (3 * out->width); /* Each line is 3 * out->width bytes */
		for (i = 0; i < out->height; i++)
		{
			for (j = 0; j < temp; j += 3)
			{/* Allocate bytes */
				out->rgba[i * temp + j] = row_pointers[i][j];       // red
				out->rgba[i * temp + j + 1] = row_pointers[i][j + 1];   // green
				out->rgba[i * temp + j + 2] = row_pointers[i][j + 2];   // blue
			}
		}
	}
	else return 1;

	/* Free the memory occupied by data */
	png_destroy_read_struct(&png_ptr, &info_ptr, 0);
	return 0;
}

int pngSteamTool::writePNGFile(string file_name, pic_data* graph)
/* Function: Write data of LCUI_Graph structure to png file */
{
	int j, i, temp;
	png_byte color_type;

	png_structp png_ptr;
	png_infop info_ptr;
	png_bytep* row_pointers;
	/* create file */
	FILE* fp = fopen(file_name.c_str(), "wb");
	if (!fp)
	{
		printf("\r[*] PNG Fail :  File %s could not be opened for writing\n", file_name.c_str());
		return -1;
	}


	/* initialize stuff */
	png_ptr = png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);

	if (!png_ptr)
	{
		printf("\r[*] PNG Fail :  png_create_write_struct failed\n");
		return -1;
	}
	info_ptr = png_create_info_struct(png_ptr);
	if (!info_ptr)
	{
		printf("\r[*] PNG Fail :  png_create_info_struct failed\n");
		return -1;
	}
	if (setjmp(png_jmpbuf(png_ptr)))
	{
		printf("\r[*] PNG Fail :  Error during init_io\n");
		return -1;
	}
	png_init_io(png_ptr, fp);


	
	if (setjmp(png_jmpbuf(png_ptr)))
	{
		printf("\r[*] PNG Fail :  Error during writing header\n");
		return -1;
	}
	/* Select the color type by judging whether the image data to be written to the file is transparent */
	if (graph->flag == HAVE_ALPHA) color_type = PNG_COLOR_TYPE_RGB_ALPHA;
	else color_type = PNG_COLOR_TYPE_RGB;

	png_set_IHDR(png_ptr, info_ptr, graph->width, graph->height,
		graph->bit_depth, color_type, PNG_INTERLACE_NONE,
		PNG_COMPRESSION_TYPE_BASE, PNG_FILTER_TYPE_BASE);

	png_write_info(png_ptr, info_ptr);


	/* write bytes */
	if (setjmp(png_jmpbuf(png_ptr)))
	{
		printf("\r[*] PNG Fail :  Error during writing bytes\n");
		return -1;
	}
	if (graph->flag == HAVE_ALPHA) temp = (4 * graph->width);
	else temp = (3 * graph->width);

	row_pointers = (png_bytep*)malloc(graph->height * sizeof(png_bytep));

	for (i = 0; i < graph->height; i++)
	{
		row_pointers[i] = (png_bytep)malloc(sizeof(unsigned char) * temp);
		if (graph->flag == HAVE_ALPHA)
		{
			for (j = 0; j < temp; j += 4)
			{
				row_pointers[i][j] = graph->rgba[i * temp + j + 2];
				row_pointers[i][j + 1] = graph->rgba[i * temp + j + 1];
				row_pointers[i][j + 2] = graph->rgba[i * temp + j];
				row_pointers[i][j + 3] = graph->rgba[i * temp + j + 3]; //alpha
			}
		}
		else
		{
			for (j = 0; j < temp; j += 3)
			{
				row_pointers[i][j] = graph->rgba[i * temp + j];
				row_pointers[i][j + 1] = graph->rgba[i * temp + j + 1];
				row_pointers[i][j + 2] = graph->rgba[i * temp + j + 2];
			}
		}

	}

	png_write_image(png_ptr, row_pointers);

	/* end write */
	if (setjmp(png_jmpbuf(png_ptr)))
	{
		printf("\r[*] PNG Fail :  Error during end of write\n");
		return -1;
	}
	png_write_end(png_ptr, NULL);

	/* cleanup heap allocation */
	for (j = 0; j < graph->height; j++)
		free(row_pointers[j]);
	free(row_pointers);
	//free(graph->rgba);
	fclose(fp);
	return 0;
}
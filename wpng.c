#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <setjmp.h>     /* for jmpbuf declaration in writepng.h */
#include <time.h>
#include <zlib.h>
#include <png.h>        /* libpng header; includes zlib.h and setjmp.h */

#ifndef TRUE
#  define TRUE 1
#  define FALSE 0
#endif

#ifndef MAX
#  define MAX(a,b)  ((a) > (b)? (a) : (b))
#  define MIN(a,b)  ((a) < (b)? (a) : (b))
#endif

#ifdef DEBUG
#  define Trace(x)  {fprintf x ; fflush(stderr); fflush(stdout);}
#else
#  define Trace(x)  ;
#endif

typedef unsigned char   uch;
typedef unsigned short  ush;
typedef unsigned long   ulg;

typedef struct _write_png_info {
    double gamma;
    long width;
    long height;
    time_t modtime;
    FILE *infile;
    FILE *outfile;
    void *png_ptr;
    void *info_ptr;
    uch *image_data;
    uch **row_pointers;
    int filter;    /* command-line-filter flag, not PNG row filter! */
    int pnmtype;
    int sample_depth;
    int interlaced;
    int have_bg;
    int have_time;
    int have_text;
    jmp_buf jmpbuf;
    uch bg_red;
    uch bg_green;
    uch bg_blue;
} write_png_info;


/* prototypes for public functions in writepng.c */

int writepng_init(write_png_info *write_png_ptr);

int writepng_encode_image(write_png_info *write_png_ptr);

int writepng_encode_row(write_png_info *write_png_ptr);

int writepng_encode_finish(write_png_info *write_png_ptr);

void writepng_cleanup(write_png_info *write_png_ptr);

void write_ppm2png(FILE *infile, FILE *outfile);

unsigned char ** get_buffer(FILE *infile);

void write_buffer(unsigned char *buffer, char *filename);

void convert2png(unsigned char **buffer, FILE *outfile);

/* local prototypes */
static void wpng_cleanup(void);

/* local prototype */

static void writepng_error_handler(png_structp png_ptr, png_const_charp msg);

static write_png_info wpng_info;   /* lone global */

int main(int argc, char **argv) {  
  write_ppm2png(fopen(argv[1], "rb"), fopen(argv[2], "wb"));
  
  /*unsigned char ** buffer;  
  buffer = get_buffer(fopen(argv[1], "rb"));
  convert2png(buffer, fopen(argv[2], "wb"));
  write_buffer(buffer, "a3.ppm"); */
  
  return 0;
}

unsigned char ** get_buffer(FILE *infile) {
  int nx = 800;
  int ny = 600;
  long i, size;
  size = nx * 3;
  unsigned char** buffer;
  buffer = (unsigned char **)malloc(nx * ny);
  for (i = 0; i < ny; i++) {
    buffer[i] = (unsigned char *)malloc(size);
    fread(buffer[i], 1, size, infile);
  }
  fclose(infile);  
  return buffer;
}

void write_buffer(unsigned char *buffer, char *filename) {
  int nx = 800;
  int ny = 600;  
  char* metadata = "VAR P RANGE 0.000000 0.000000";
  
  FILE * fp = fopen(filename,"wb");
  fprintf(fp,"P6\n#%s\n%d\n%d\n255\n",metadata,nx,ny);
  fwrite(buffer, sizeof(unsigned char),nx*ny*3,fp);
  fclose(fp);
}

void convert2png(unsigned char ** buffer, FILE *outfile) {
  ulg rowbytes;
  int rc;
  
  int nx = 800;
  int ny = 600;
  
  wpng_info.infile = NULL;
  wpng_info.outfile = NULL;
  wpng_info.image_data = NULL;
  wpng_info.row_pointers = NULL;
  wpng_info.filter = FALSE;
  wpng_info.interlaced = FALSE;
  wpng_info.have_bg = FALSE;
  wpng_info.have_time = FALSE;
  wpng_info.have_text = 0;
  wpng_info.gamma = 0.0;

  // input
  wpng_info.outfile = outfile;
  wpng_info.filter = TRUE;
  wpng_info.width = nx;
  wpng_info.height = ny;
  wpng_info.pnmtype = 6;
  wpng_info.have_bg = FALSE;
  wpng_info.sample_depth = 8; 
  wpng_info.have_text = FALSE;
  
  /* prepare the text buffers for libpng's use; note that even though
   * PNG's png_text struct includes a length field, we don't have to fill
   * it out */

  rc = writepng_init(&wpng_info);
  
  // wpng_info.pnmtype == 6
  rowbytes = wpng_info.width * 3;

  /* read and write the image, either in its entirety (if writing interlaced
   * PNG) or row by row (if non-interlaced) */

  fprintf(stderr, "Encoding image data...\n");
  fflush(stderr);
  
  long j;
  wpng_info.infile = fopen("aU.ppm","rb");
  wpng_info.image_data = (unsigned char *)malloc(rowbytes);
    
  for (j = 0;  j < wpng_info.height;  j++) {
    memcpy(wpng_info.image_data, buffer[j], rowbytes);    
    writepng_encode_row(&wpng_info);
  }
  
  writepng_encode_finish(&wpng_info);
  fprintf(stderr, "Done.\n");
  fflush(stderr);
  writepng_cleanup(&wpng_info);
  wpng_cleanup();
}

void write_ppm2png(FILE *infile, FILE *outfile) {
    char pnmline[256];
    ulg rowbytes;
    int rc;

    wpng_info.infile = NULL;
    wpng_info.outfile = NULL;
    wpng_info.image_data = NULL;
    wpng_info.row_pointers = NULL;
    wpng_info.filter = FALSE;
    wpng_info.interlaced = FALSE;
    wpng_info.have_bg = FALSE;
    wpng_info.have_time = FALSE;
    wpng_info.have_text = 0;
    wpng_info.gamma = 0.0;

    // input
    wpng_info.infile = infile;
    wpng_info.outfile = outfile;
    wpng_info.filter = TRUE;
    wpng_info.width = 800;
    wpng_info.height = 600;
    wpng_info.pnmtype = 6;
    wpng_info.have_bg = FALSE;
    wpng_info.sample_depth = 8; 
    wpng_info.have_text = FALSE;
    
    fgets(pnmline, 256, wpng_info.infile);
    do {
        fgets(pnmline, 256, wpng_info.infile);  /* lose any comments */
    } while (pnmline[0] == '#');
    do {
        fgets(pnmline, 256, wpng_info.infile);  /* more comment lines */
    } while (pnmline[0] == '#');    

    /* prepare the text buffers for libpng's use; note that even though
     * PNG's png_text struct includes a length field, we don't have to fill
     * it out */
  
    rc = writepng_init(&wpng_info);
    
    // wpng_info.pnmtype == 6
    rowbytes = wpng_info.width * 3;

    /* read and write the image, either in its entirety (if writing interlaced
     * PNG) or row by row (if non-interlaced) */

    fprintf(stderr, "Encoding image data...\n");
    fflush(stderr);
    
    long j;

    wpng_info.image_data = (uch *)malloc(rowbytes);
    for (j = wpng_info.height;  j > 0L;  --j) {
        fread(wpng_info.image_data, 1, rowbytes, wpng_info.infile);        
        writepng_encode_row(&wpng_info);
    }
    writepng_encode_finish(&wpng_info);
    fprintf(stderr, "Done.\n");
    fflush(stderr);
    writepng_cleanup(&wpng_info);
    wpng_cleanup();
}

static void wpng_cleanup(void)
{
    if (wpng_info.outfile) {
        fclose(wpng_info.outfile);
        wpng_info.outfile = NULL;
    }

    if (wpng_info.infile) {
        fclose(wpng_info.infile);
        wpng_info.infile = NULL;
    }

    if (wpng_info.image_data) {
        free(wpng_info.image_data);
        wpng_info.image_data = NULL;
    }

    if (wpng_info.row_pointers) {
        free(wpng_info.row_pointers);
        wpng_info.row_pointers = NULL;
    }
}


/* returns 0 for success, 2 for libpng problem, 4 for out of memory, 11 for
 *  unexpected pnmtype; note that outfile might be stdout */

int writepng_init(write_png_info *write_png_ptr)
{
    png_structp  png_ptr;       /* note:  temporary variables! */
    png_infop  info_ptr;
    int color_type, interlace_type;


    /* could also replace libpng warning-handler (final NULL), but no need: */

    png_ptr = png_create_write_struct(PNG_LIBPNG_VER_STRING, write_png_ptr,
      writepng_error_handler, NULL);
    if (!png_ptr)
        return 4;   /* out of memory */

    info_ptr = png_create_info_struct(png_ptr);
    if (!info_ptr) {
        png_destroy_write_struct(&png_ptr, NULL);
        return 4;   /* out of memory */
    }


    /* setjmp() must be called in every function that calls a PNG-writing
     * libpng function, unless an alternate error handler was installed--
     * but compatible error handlers must either use longjmp() themselves
     * (as in this program) or exit immediately, so here we go: */

    if (setjmp(write_png_ptr->jmpbuf)) {
        png_destroy_write_struct(&png_ptr, &info_ptr);
        return 2;
    }


    /* make sure outfile is (re)opened in BINARY mode */

    png_init_io(png_ptr, write_png_ptr->outfile);


    /* set the compression levels--in general, always want to leave filtering
     * turned on (except for palette images) and allow all of the filters,
     * which is the default; want 32K zlib window, unless entire image buffer
     * is 16K or smaller (unknown here)--also the default; usually want max
     * compression (NOT the default); and remaining compression flags should
     * be left alone */

    png_set_compression_level(png_ptr, Z_BEST_COMPRESSION);
/*
    >> this is default for no filtering; Z_FILTERED is default otherwise:
    png_set_compression_strategy(png_ptr, Z_DEFAULT_STRATEGY);
    >> these are all defaults:
    png_set_compression_mem_level(png_ptr, 8);
    png_set_compression_window_bits(png_ptr, 15);
    png_set_compression_method(png_ptr, 8);
 */


    /* set the image parameters appropriately */

    if (write_png_ptr->pnmtype == 5)
        color_type = PNG_COLOR_TYPE_GRAY;
    else if (write_png_ptr->pnmtype == 6)
        color_type = PNG_COLOR_TYPE_RGB;
    else if (write_png_ptr->pnmtype == 8)
        color_type = PNG_COLOR_TYPE_RGB_ALPHA;
    else {
        png_destroy_write_struct(&png_ptr, &info_ptr);
        return 11;
    }

    interlace_type = write_png_ptr->interlaced? PNG_INTERLACE_ADAM7 :
                                               PNG_INTERLACE_NONE;

    png_set_IHDR(png_ptr, info_ptr, write_png_ptr->width, write_png_ptr->height,
      write_png_ptr->sample_depth, color_type, interlace_type,
      PNG_COMPRESSION_TYPE_DEFAULT, PNG_FILTER_TYPE_DEFAULT);

    if (write_png_ptr->gamma > 0.0)
        png_set_gAMA(png_ptr, info_ptr, write_png_ptr->gamma);

    if (write_png_ptr->have_bg) {   /* we know it's RGBA, not gray+alpha */
        png_color_16  background;

        background.red = write_png_ptr->bg_red;
        background.green = write_png_ptr->bg_green;
        background.blue = write_png_ptr->bg_blue;
        png_set_bKGD(png_ptr, info_ptr, &background);
    }

    if (write_png_ptr->have_time) {
        png_time  modtime;

        png_convert_from_time_t(&modtime, write_png_ptr->modtime);
        png_set_tIME(png_ptr, info_ptr, &modtime);
    }
    
    /* write all chunks up to (but not including) first IDAT */

    png_write_info(png_ptr, info_ptr);


    /* if we wanted to write any more text info *after* the image data, we
     * would set up text struct(s) here and call png_set_text() again, with
     * just the new data; png_set_tIME() could also go here, but it would
     * have no effect since we already called it above (only one tIME chunk
     * allowed) */


    /* set up the transformations:  for now, just pack low-bit-depth pixels
     * into bytes (one, two or four pixels per byte) */

    png_set_packing(png_ptr);
/*  png_set_shift(png_ptr, &sig_bit);  to scale low-bit-depth values */


    /* make sure we save our pointers for use in writepng_encode_image() */

    write_png_ptr->png_ptr = png_ptr;
    write_png_ptr->info_ptr = info_ptr;


    /* OK, that's all we need to do for now; return happy */

    return 0;
}





/* returns 0 for success, 2 for libpng (longjmp) problem */

int writepng_encode_image(write_png_info *write_png_ptr)
{
    png_structp png_ptr = (png_structp)write_png_ptr->png_ptr;
    png_infop info_ptr = (png_infop)write_png_ptr->info_ptr;


    /* as always, setjmp() must be called in every function that calls a
     * PNG-writing libpng function */

    if (setjmp(write_png_ptr->jmpbuf)) {
        png_destroy_write_struct(&png_ptr, &info_ptr);
        write_png_ptr->png_ptr = NULL;
        write_png_ptr->info_ptr = NULL;
        return 2;
    }


    /* and now we just write the whole image; libpng takes care of interlacing
     * for us */

    png_write_image(png_ptr, write_png_ptr->row_pointers);


    /* since that's it, we also close out the end of the PNG file now--if we
     * had any text or time info to write after the IDATs, second argument
     * would be info_ptr, but we optimize slightly by sending NULL pointer: */

    png_write_end(png_ptr, NULL);

    return 0;
}





/* returns 0 if succeeds, 2 if libpng problem */

int writepng_encode_row(write_png_info *write_png_ptr)  /* NON-interlaced only! */
{
    png_structp png_ptr = (png_structp)write_png_ptr->png_ptr;
    png_infop info_ptr = (png_infop)write_png_ptr->info_ptr;


    /* as always, setjmp() must be called in every function that calls a
     * PNG-writing libpng function */

    if (setjmp(write_png_ptr->jmpbuf)) {
        png_destroy_write_struct(&png_ptr, &info_ptr);
        write_png_ptr->png_ptr = NULL;
        write_png_ptr->info_ptr = NULL;
        return 2;
    }


    /* image_data points at our one row of image data */

    png_write_row(png_ptr, write_png_ptr->image_data);

    return 0;
}





/* returns 0 if succeeds, 2 if libpng problem */

int writepng_encode_finish(write_png_info *write_png_ptr)   /* NON-interlaced! */
{
    png_structp png_ptr = (png_structp)write_png_ptr->png_ptr;
    png_infop info_ptr = (png_infop)write_png_ptr->info_ptr;


    /* as always, setjmp() must be called in every function that calls a
     * PNG-writing libpng function */

    if (setjmp(write_png_ptr->jmpbuf)) {
        png_destroy_write_struct(&png_ptr, &info_ptr);
        write_png_ptr->png_ptr = NULL;
        write_png_ptr->info_ptr = NULL;
        return 2;
    }


    /* close out PNG file; if we had any text or time info to write after
     * the IDATs, second argument would be info_ptr: */

    png_write_end(png_ptr, NULL);

    return 0;
}





void writepng_cleanup(write_png_info *write_png_ptr)
{
    png_structp png_ptr = (png_structp)write_png_ptr->png_ptr;
    png_infop info_ptr = (png_infop)write_png_ptr->info_ptr;

    if (png_ptr && info_ptr)
        png_destroy_write_struct(&png_ptr, &info_ptr);
}





static void writepng_error_handler(png_structp png_ptr, png_const_charp msg)
{
    write_png_info  *write_png_ptr;

    /* This function, aside from the extra step of retrieving the "error
     * pointer" (below) and the fact that it exists within the application
     * rather than within libpng, is essentially identical to libpng's
     * default error handler.  The second point is critical:  since both
     * setjmp() and longjmp() are called from the same code, they are
     * guaranteed to have compatible notions of how big a jmp_buf is,
     * regardless of whether _BSD_SOURCE or anything else has (or has not)
     * been defined. */

    fprintf(stderr, "writepng libpng error: %s\n", msg);
    fflush(stderr);

    write_png_ptr = png_get_error_ptr(png_ptr);
    if (write_png_ptr == NULL) {         /* we are completely hosed now */
        fprintf(stderr,
          "writepng severe error:  jmpbuf not recoverable; terminating.\n");
        fflush(stderr);
        exit(99);
    }

    longjmp(write_png_ptr->jmpbuf, 1);
}

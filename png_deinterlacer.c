#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include <png.h>

struct Image {
  int width;
  int height;
  png_byte color_type;
  png_byte bit_depth;
  png_bytep *data;
};

/*
 * Prints an error message on the stderr and then aborts the application
 */
static void abort_application(const char *msg, ...)
{
  va_list args;
  va_start(args, msg);
  vfprintf(stderr, msg, args);
  fprintf(stderr, "\n");
  va_end(args);
  abort();
}

/*
 * Reads the PNG image and header from file
 */
static void read_png_image(const char *png_file, struct Image *image)
{
  if (png_file == NULL || image == NULL)
    abort_application("read_png_image has invalid arguments!");

  /* opens the PNG file and tests if it contains a valid PNG header */
  FILE *fp = fopen(png_file, "rb");
  if (!fp)
    abort_application("File %s could not be opened!", png_file);

  png_byte header[8]; // 8 is the maximum size that can be checked
  fread(header, 1, 8, fp);
  if (png_sig_cmp(header, 0, 8)) {
    abort_application("File %s does not contain a valid PNG image!", png_file);
  }

  png_structp png_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
  if (!png_ptr)
    abort_application("png_create_read_struct failed!");

  png_infop info_ptr = png_create_info_struct(png_ptr);
  if (!info_ptr)
    abort_application("png_create_read_struct failed!");

  if (setjmp(png_jmpbuf(png_ptr)))
    abort_application("Error during init_io!");

  png_init_io(png_ptr, fp);
  png_set_sig_bytes(png_ptr, 8);

  png_read_info(png_ptr, info_ptr);

  image->width = png_get_image_width(png_ptr, info_ptr);
  image->height = png_get_image_height(png_ptr, info_ptr);
  image->color_type = png_get_color_type(png_ptr, info_ptr);
  image->bit_depth = png_get_bit_depth(png_ptr, info_ptr);

  png_set_interlace_handling(png_ptr);
  png_read_update_info(png_ptr, info_ptr);

  if (setjmp(png_jmpbuf(png_ptr))) {
    abort_application("Error during reading the image!");
  }

  image->data = (png_bytep*)malloc(sizeof(png_bytep) * image->height);
  for (int y = 0; y < image->height; y++) {
    image->data[y] = (png_byte*)malloc(png_get_rowbytes(png_ptr, info_ptr));
  }

  png_read_image(png_ptr, image->data);

  fclose(fp);
}

/*
 * Writes the PNG image and header into the file
 */
static void write_png_image(const char *png_file, const struct Image *image)
{
  if (png_file == NULL || image == NULL)
    abort_application("write_png_image has invalid arguments!");

  FILE *fp = fopen(png_file, "wb");
  if (!fp)
    abort_application("File %s could not be opened!", png_file);

  png_structp png_ptr = png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);

  if (!png_ptr)
    abort_application("png_create_write_struct failed!");

  png_infop info_ptr = png_create_info_struct(png_ptr);
  if (!info_ptr)
    abort_application("png_create_info_struct failed!");

  if (setjmp(png_jmpbuf(png_ptr)))
    abort_application("Error during init_io!");

  png_init_io(png_ptr, fp);

  if (setjmp(png_jmpbuf(png_ptr)))
    abort_application("Error during writing the header!");

  png_set_IHDR(png_ptr, info_ptr, image->width, image->height, image->bit_depth,
      image->color_type, PNG_INTERLACE_NONE, PNG_COMPRESSION_TYPE_BASE, PNG_FILTER_TYPE_BASE);

  /* write the PNG header */
  png_write_info(png_ptr, info_ptr);

  /* write the PNG image */
  if (setjmp(png_jmpbuf(png_ptr)))
    abort_application("Error during writing the image!");

  png_write_image(png_ptr, image->data);

  /* finish the writing */
  if (setjmp(png_jmpbuf(png_ptr)))
    abort_application("Error during finishing the image!");

  png_write_end(png_ptr, NULL);

  fclose(fp);
}

/*
 * Deinterlace the image in place
 */
static void deinterlace_image(struct Image *image)
{
  if (image == NULL)
    abort_application("deinterlace_image has invalid arguments!");
  
  png_byte prev, curr;
  for (int x = 0; x < image->width; x++) {
    prev = image->data[0][x];
    for (int y = 1; y < image->height; y++) {
      curr = image->data[y][x];
      image->data[y][x] = (curr + prev) / 2;
      prev = curr;
    }
  }
}

int main(int argc, char **argv)
{
  if (argc != 3)
    abort_application("Usage: png_deinterlacer <input_png_file> <output_png_file>");

  struct Image *image = (struct Image*)malloc(sizeof(struct Image));
  read_png_image(argv[1], image);
  deinterlace_image(image);
  write_png_image(argv[2], image);

  /* release the allocated memory */
  for (int y = 0; y < image->height; y++)
    free(image->data[y]);
  free(image->data);
  free(image);

  return 0;
}

#include <stdio.h>
#include <vips/vips.h>
#include <math.h>
#include "Sobel.h"

int
main (int argc, char **argv)
{
  VipsImage *in;
  VipsImage *out;

	vips_sobel_get_type();

  if (VIPS_INIT (argv[0]))
    vips_error_exit ("unable to start VIPS");

  if (argc != 3)
    vips_error_exit ("usage: %s <filename>", g_get_prgname ());

  if (!(in = vips_image_new_from_file (argv[1], NULL)))
    vips_error_exit ("unable to open");

	if (vips_sobel(in, &out,NULL))
		vips_error_exit("unable to perform sobel");
		
	if( vips_image_write_to_file( out, argv[2], NULL ))
    vips_error_exit( NULL );

	g_object_unref (in);
	g_object_unref (out);
	return 0;
}

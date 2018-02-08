/* compile with
 *
 * gcc -Wall vipsavg.c `pkg-config vips --cflags --libs`
 */

#include <stdio.h>
#include <vips/vips.h>
#include <math.h>
#include "mag.h"

int
main (int argc, char **argv)
{
  VipsImage *in;
  VipsImage *mask;
	VipsImage *outx;
	VipsImage *outy;
	VipsImage *squarex;
	VipsImage *squarey;
	VipsImage *square;
	VipsImage *final;
	const double xkernel[] = {-1, 0, 1, -2, 0, 2, -1, 0, 1}; 
 	const double ykernel[] = {-1, -2, -1, 0, 0, 0, 1, 2, 1};


  if (VIPS_INIT (argv[0]))
    vips_error_exit ("unable to start VIPS");

mag_get_type();
  if (argc != 3)
    vips_error_exit ("usage: %s <filename>", g_get_prgname ());

  if (!(in = vips_image_new_from_file (argv[1], NULL)))
    vips_error_exit ("unable to open");
    
//Assigns the x-kernel to the mask
	if (!(mask=vips_image_matrix_from_array(3, 3,xkernel,9)))
		vips_error_exit("unable to create X-mask");	
	
	//if(vips_cast_uchar(mask, &tempmask,NULL))
	//	vips_error_exit("cannot cast to Uchar");
//Convolve the input image with the mask and store in outx		
	if (vips_conva (in, &outx, mask, NULL))
		vips_error_exit("Unable to conv"); 

	if (vips_multiply (outx,outx,&squarex, NULL))
		vips_error_exit("unable to multiply");

//create image squarex
//	if (vips_image_write_to_file(squarex, argv[2], NULL ) )
//    vips_error_exit( NULL );	
    
//Assigns the y-kernel to the mask
	if (!(mask=vips_image_matrix_from_array(3, 3,ykernel,9)))
		vips_error_exit("unable to create Y-mask");

	//if(vips_cast_uchar(mask, &tempmask,NULL))
	//	vips_error_exit("cannot cast to Ykernel to Uchar");
//Convolve the input image with the mask and store in outy		
	if (vips_conva (in, &outy, mask, NULL))
		vips_error_exit("Unable to conv"); 

	if (vips_multiply (outy, outy, &squarey, NULL))
		vips_error_exit("Unable to multiply Y");


//create image squarey				
//	if (vips_image_write_to_file(squarey, argv[3], NULL ) )
//    vips_error_exit( NULL );

	if(vips_add(squarex, squarey, &square, NULL))
		vips_error_exit("unable to add");


	switch( vips_image_get_format( square ) ) {
	case VIPS_FORMAT_UCHAR: 	
		printf("UCHAR"); break; 
	case VIPS_FORMAT_CHAR: 	
		printf("CHAR"); break; 
	case VIPS_FORMAT_USHORT: 
		printf("USHORT"); break;
	case VIPS_FORMAT_SHORT: 	
		printf("SHORT"); break;
	case VIPS_FORMAT_UINT: 	
		printf("UINT"); break; 
	case VIPS_FORMAT_INT: 	
		printf("INT"); break;
	case VIPS_FORMAT_FLOAT: 		
		printf("FLOAT"); break; 
	case VIPS_FORMAT_DOUBLE:	
		printf("DOUBLE"); break;
	case VIPS_FORMAT_COMPLEX: 
		printf("COMPLEX"); break;
	case VIPS_FORMAT_DPCOMPLEX: 
		printf("DCOMPLEX"); break;

	default:
		g_assert_not_reached();
	}
	if (mag(square, &final, NULL))
		vips_error_exit("Unable to sqrt");

	if (vips_image_write_to_file(square, argv[2], NULL ) )
    		vips_error_exit( NULL );
	
	g_object_unref (in);
	g_object_unref (mask);
	g_object_unref (outy);
	g_object_unref (squarey);
 	g_object_unref (outx);
	g_object_unref (squarex);
	g_object_unref (square);
	g_object_unref (final);

  return (0);
}

//compile with gcc -Wall progname.c `pkg-config vips --cflags --libs`
//run with ./a.out ~/pics/input.jpg ~/pics/outputx.jpg ~/pics/outputy.jpg


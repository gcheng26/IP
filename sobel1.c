/* compile with
 *
 * gcc -Wall vipsavg.c `pkg-config vips --cflags --libs`
 */

#include <stdio.h>
#include <vips/vips.h>
//#include <mag.h>

int
main (int argc, char **argv)
{
  VipsImage *im;
  VipsImage *mask_a;
  VipsImage *mask_b;
  VipsImage *temp;
	VipsImage *out; 	
	const double mask1[] = {-1, 0, 1}; 
 	const double mask2[] = { 1, 2, 1};
	

  if (VIPS_INIT (argv[0]))
    vips_error_exit ("unable to start VIPS");

  if (argc != 4)
    vips_error_exit ("usage: %s <filename>", g_get_prgname ());

  if (!(im = vips_image_new_from_file (argv[1], NULL)))
    vips_error_exit ("unable to open");
    
  //sets-up the first mask for xconv
  if (!(mask_a=vips_image_matrix_from_array(3, 1,mask1,3)))
		vips_error_exit("unable to create X-mask");	
	//sets up the second mask for xconv
  if (!(mask_b=vips_image_matrix_from_array(1, 3,mask2,3)))
		vips_error_exit("unable to create X-mask");	
  //conv with mask_a
  if (vips_conv (im, &temp, mask_a, NULL))
		vips_error_exit("Unable to conv"); 
	//conv with mask_b	
	if (vips_conv (temp, &out, mask_b, NULL))
		vips_error_exit("Unable to conv");
	//output
	if (vips_image_write_to_file(out, argv[2], NULL ) )
    vips_error_exit( NULL );
	//sets-up the first mask for yconv
	if (!(mask_a=vips_image_matrix_from_array(1, 3,mask1,3)))
		vips_error_exit("unable to create X-mask");	
	//sets-up the second mask for yconv
  if (!(mask_b=vips_image_matrix_from_array(3, 1,mask2,3)))
		vips_error_exit("unable to create X-mask");	
	//conv with mask_a	
	if (vips_conv (im, &temp, mask_a, NULL))
		vips_error_exit("Unable to conv"); 
	//conv with mask_b	
	if (vips_conv (temp, &out, mask_b, NULL))
		vips_error_exit("Unable to conv");
	//output
	if (vips_image_write_to_file(out, argv[3], NULL ) )
    vips_error_exit( NULL );	
	//if (vips_gaussblur(im, &out, 3, NULL))
	//	vips_error_exit ("unable to blur");
		
	
/*    
//Assigns the x-kernel to the mask


//Convolve the input image with the mask and store in outx		
	if (vips_conv (im, &outx, mask, NULL))
		vips_error_exit("Unable to conv"); 

	if (vips_multiply (outx,outx,&squarex, NULL))
		vips_error_exit("unable to multiply");

//create image squarex
//	if (vips_image_write_to_file(squarex, argv[2], NULL ) )
//    vips_error_exit( NULL );	
    
//Assigns the y-kernel to the mask
	if (!(mask=vips_image_matrix_from_array(3, 3,ykernel,9)))
		vips_error_exit("unable to create Y-mask");

//Convolve the input image with the mask and store in outy		
	if (vips_conv (im, &outy, mask, NULL))
		vips_error_exit("Unable to conv"); 

	if (vips_multiply (outy, outy, &squarey, NULL))
		vips_error_exit("Unable to multiply Y");


//create image squarey				
//	if (vips_image_write_to_file(squarey, argv[3], NULL ) )
//    vips_error_exit( NULL );

	if(vips_add(squarex, squarey, &square, NULL))
		vips_error_exit("unable to add");

	if (mag(square, &final, NULL))
		vips_error_exit("Unable to sqrt");
	
	
	g_object_unref (mask);
	g_object_unref (outy);
	g_object_unref (squarey);
 	g_object_unref (outx);
	g_object_unref (squarex);
	g_object_unref (square);
	g_object_unref (final);
*/
	g_object_unref (im);	
	g_object_unref (temp);
	g_object_unref (out);
  return (0);
}

//compile with gcc -Wall progname.c `pkg-config vips --cflags --libs`
//run with ./a.out ~/pics/input.jpg ~/pics/outputx.jpg ~/pics/outputy.jpg


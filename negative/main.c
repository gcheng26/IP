/* Run the "negative" operation from the command-line.
 */

#include <vips/vips.h>

#include "negative.h"

int
main( int argc, char **argv )
{
  VipsImage *in;
  VipsImage *out;

  if( VIPS_INIT( argv[0]) ) 
    vips_error_exit( NULL );

  /* Initialise our new operator.
   */
  negative_get_type();

  if( !(in = vips_image_new_from_file( argv[1], NULL )) )
    vips_error_exit( NULL );
  if( negative( in, &out, "image_max", 128, NULL ) ) {
    g_object_unref( in );
    vips_error_exit( NULL );
  }
  if( vips_image_write_to_file( out, argv[2], NULL ) ) {
    g_object_unref( in );
    vips_error_exit( NULL );
  }

  g_object_unref( in );
  g_object_unref( out );

  return( 0 );
}

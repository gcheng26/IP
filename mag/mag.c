
#ifdef HAVE_CONFIG_H
#include <config.h>
#endif /*HAVE_CONFIG_H*/
#include <vips/intl.h>
#include <stdio.h>
#include <math.h>
#include <vips/vips.h>


typedef struct _Mag {
  VipsOperation parent_instance;

  VipsImage *in;
  VipsImage *out;


} Mag;

typedef struct _MagClass {
  VipsOperationClass parent_class;

  /* No new class members needed for this op.
   */

} MagClass;

G_DEFINE_TYPE( Mag, mag , VIPS_TYPE_OPERATION );


static int
mag_generate( VipsRegion *or, 
  void *vseq, void *a, void *b, gboolean *stop )
{
  /* The area of the output region we have been asked to make.
   */
  VipsRect *r = &or->valid;

  /* The sequence value ... the thing returned by vips_start_one().
   */
  VipsRegion *ir = (VipsRegion *) vseq;

  VipsImage *in = (VipsImage *) a;
  Mag * mag = (Mag *) b;
  int line_size = r->width * mag->in->Bands; 

  int x, y, c;
  c=0;

  /* Request matching part of input region.
   */
  if( vips_region_prepare( ir, r ) )
    return( -1 );

  for( y = 0; y < r->height; y++ ) {
    unsigned char * p = (unsigned char *)
      VIPS_REGION_ADDR( ir, r->left, r->top + y ); 
    unsigned char * q = (unsigned char *)
      VIPS_REGION_ADDR( or, r->left, r->top + y ); 
//takes the input, calculates its square root and assigns it to the output
  	for( x = 0; x < line_size; x++ ) {
  		while ((p[x]>>1)>(c*c)) {
    		c++;
    		if (c>255)
    		printf("error, limit exceeded");
   			}
    
    	q[x]= ((c*c)-(p[x]>>1)>(p[x]>>1)-((c-1)*(c-1)))? c:c-1;
	c=0;	
		}
  }
  return 0;
}

static int
mag_build( VipsObject *object )
{
  VipsObjectClass *class = VIPS_OBJECT_GET_CLASS( object );
  Mag *mag = (Mag *) object;

  if( VIPS_OBJECT_CLASS( mag_parent_class )->build( object ) )
    return( -1 );

  if( vips_check_uncoded( class->nickname, mag->in ) ||
    vips_check_format( class->nickname, mag->in, VIPS_FORMAT_UINT ) )
    return( -1 );

  g_object_set( object, "out", vips_image_new(), NULL ); 

  if( vips_image_pipelinev( mag->out, 
    VIPS_DEMAND_STYLE_THINSTRIP, mag->in, NULL ) )  // using thinstrip should be fine since we are doing an arithmetic type of operation
    return( -1 );

  if( vips_image_generate( mag->out, 
    vips_start_one, 
    mag_generate, 
    vips_stop_one, 
    mag->in, mag ) )
    return( -1 );

  return( 0 );
}


static void
mag_init( Mag * mag )
{	
}


static void
mag_class_init( MagClass *class )
{
  GObjectClass *gobject_class = G_OBJECT_CLASS( class );
  VipsObjectClass *object_class = VIPS_OBJECT_CLASS( class );

  gobject_class->set_property = vips_object_set_property;
  gobject_class->get_property = vips_object_get_property;

  object_class->nickname = "mag";
  object_class->description = "calculated magnitude";
  object_class->build = mag_build;

  VIPS_ARG_IMAGE( class, "in", 1, 
    "Input", 
    "Input image",
    VIPS_ARGUMENT_REQUIRED_INPUT,
    G_STRUCT_OFFSET( Mag, in ) );

  VIPS_ARG_IMAGE( class, "out", 2, 
    "Output", 
    "Output image",
    VIPS_ARGUMENT_REQUIRED_OUTPUT, 
    G_STRUCT_OFFSET( Mag, out ) );
}


int 
mag( VipsImage *in, VipsImage **out, ... )
{
  va_list ap;
  int result;

  va_start( ap, out );
  result = vips_call_split( "mag", ap, in, out );
  va_end( ap );

  return( result );
}



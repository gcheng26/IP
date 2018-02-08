/* Find image nagative.
 *
 * This is an example vips operqtion and not very useful.
 */

#include <vips/vips.h>

#include "negative.h"

typedef struct _Negative {
  VipsOperation parent_instance;

  VipsImage *in;
	VipsImage *tmp;
  VipsImage *out;

  int image_max;

} Negative;

typedef struct _NegativeClass {
  VipsOperationClass parent_class;

  /* No new class members needed for this op.
   */

} NegativeClass;

G_DEFINE_TYPE( Negative, negative, VIPS_TYPE_OPERATION );

static int
negative_generate( VipsRegion *or, 
  void *vseq, void *a, void *b, gboolean *stop )
{
	
//	VipsImage *in2= negative->in;
//	VipsImage *tmp2;
//	if(vips_flip(in2, &tmp2, VIPS_DIRECTION_VERTICAL,NULL))
//		vips_error_exit ("unable to flip");
//	negative->tmp= tmp2
  /* The area of the output region we have been asked to make.
   */

  VipsRect *r = &or->valid;

  /* The sequence value ... the thing returned by vips_start_one().
   */
  VipsRegion *ir = (VipsRegion *) vseq;

  VipsImage *in = (VipsImage *) a;
  Negative *negative = (Negative *) b;
  int line_size = r->width * negative->tmp->Bands; 

  int x, y;

  /* Request matching part of input region.
   */
  if( vips_region_prepare( ir, r ) )
    return( -1 );

  for( y = 0; y < r->height; y++ ) {
    unsigned char * restrict p = (unsigned char *)
      VIPS_REGION_ADDR( ir, r->left, r->top + y ); 
    unsigned char * restrict q = (unsigned char *)
      VIPS_REGION_ADDR( or, r->left, r->top + y ); 

    for( x = 0; x < line_size; x++ ) 
      q[x] = negative->image_max - p[x];
  }

  return( 0 );
}

static int
negative_build( VipsObject *object )
{
  VipsObjectClass *class = VIPS_OBJECT_GET_CLASS( object );
  Negative *negative = (Negative *) object;

  if( VIPS_OBJECT_CLASS( negative_parent_class )->build( object ) )
    return( -1 );

  if( vips_check_uncoded( class->nickname, negative->in ) ||
    vips_check_format( class->nickname, negative->in, VIPS_FORMAT_UCHAR ) )
    return( -1 );


	if(vips_flip(negative->in, &negative->tmp, VIPS_DIRECTION_VERTICAL,NULL))
		vips_error_exit ("unable to flip");

  g_object_set( object, "out", vips_image_new(), NULL ); 

  if( vips_image_pipelinev( negative->out, 
    VIPS_DEMAND_STYLE_THINSTRIP, negative->tmp, NULL ) )
    return( -1 );

  if( vips_image_generate( negative->out, 
    vips_start_one, 
    negative_generate, 
    vips_stop_one, 
    negative->tmp, negative ) )
    return( -1 );

  return( 0 );
}

static void
negative_init( Negative *negative )
{
  negative->image_max = 255;
}

static void
negative_class_init( NegativeClass *class )
{
  GObjectClass *gobject_class = G_OBJECT_CLASS( class );
  VipsObjectClass *object_class = VIPS_OBJECT_CLASS( class );

  gobject_class->set_property = vips_object_set_property;
  gobject_class->get_property = vips_object_get_property;

  object_class->nickname = "negative";
  object_class->description = "photographic negative";
  object_class->build = negative_build;

  VIPS_ARG_IMAGE( class, "in", 1, 
    "Input", 
    "Input image",
    VIPS_ARGUMENT_REQUIRED_INPUT,
    G_STRUCT_OFFSET( Negative, in ) );

 	VIPS_ARG_IMAGE( class, "tmp", 2, 
    "Temp", 
    "Tmp image",
    VIPS_ARGUMENT_REQUIRED_OUTPUT, 
    G_STRUCT_OFFSET( Negative, tmp ) );

  VIPS_ARG_IMAGE( class, "out", 3, 
    "Output", 
    "Output image",
    VIPS_ARGUMENT_REQUIRED_OUTPUT, 
    G_STRUCT_OFFSET( Negative, out ) );

  VIPS_ARG_INT( class, "image_max", 4, 
    "Image maximum", 
    "Maximum value in image: pivot about this",
    VIPS_ARGUMENT_OPTIONAL_INPUT,
    G_STRUCT_OFFSET( Negative, image_max ),
    0, 255, 255 );
}

int 
negative( VipsImage *in, VipsImage **out, ... )
{
  va_list ap;
  int result;

  va_start( ap, out );
  result = vips_call_split( "negative", ap, in, out );
  va_end( ap );

  return( result );
}


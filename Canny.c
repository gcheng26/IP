#ifdef HAVE_CONFIG_H
#include <config.h>
#endif /*HAVE_CONFIG_H*/
#include <vips/intl.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <vips/vips.h>

typedef struct _Canny{
VipsOperation parent_instance;

VipsImage *in;
VipsImage *out;
double sigma;
VipsImage *args[20];
} VipsCanny;

typedef VipsOperationClass VipsCannyClass;

G_DEFINE_TYPE(VipsCanny, vips_canny, VIPS_TYPE_OPERATION);



static VipsPel vips_canny_polar_atan2[256];

static void *
vips_atan2_init( void *null )
{
	int i;

	for( i = 0; i < 256; i++ ) {
		/* Use the bottom 4 bits for x, the top 4 for y. The double
		 * shift does sign extension, assuming 2s complement.
		 */
		int bits = sizeof( int ) * 8 - 4;
		int x = ((i & 0xf) << bits) >> bits;
		int y = ((i >> 4) & 0x0f) << bits >> bits;
		double theta = VIPS_DEG( atan2( x, y ) ) + 360;

		vips_canny_polar_atan2[i] = 256 * theta / 360;
	}

	return( NULL );
}

/*
// step 1: Blurring
static int vips_canny_blur(VipsImage *in, VipsImage *out){

	if (vips_gaussblur(in, &out, canny->sigma, NULL))
		vips_error_exit ("unable to blur");

	return 0;
}
*/


// Step 2: Sobel Operator
//function to output the gradient image after blurring and convolving the input image with X and Y sobel masks
static int vips_canny_gradient( VipsImage *in, VipsImage **Gx, VipsImage **Gy)
{
	VipsImage *temp;
	VipsImage **t;
	VipsPrecision precision;
	temp= vips_image_new();
	t=(VipsImage**) vips_object_local_array((VipsObject*) temp, 10);

	t[1]= vips_image_new_matrixv(1, 3, 1.0, 2.0, 1.0);
	t[2]= vips_image_new_matrixv(3, 1, 1.0, 0.0, -1.0);


	if( in->BandFmt == VIPS_FORMAT_UCHAR ) {
			precision = VIPS_PRECISION_INTEGER;
			vips_image_set_double( t[0], "offset", 128.0 );
		}
		else
			precision = VIPS_PRECISION_FLOAT;


	vips_image_set_double (t[2], "offset", 128.0);
	if(vips_conv( in, &t[3],t[1], "precision", precision, NULL) ||vips_conv(t[3],&Gx, t[2], "precision", precision, NULL)){
		g_object_unref(temp);
		return (-1);

	}

	t[5] = vips_image_new_matrixv( 3, 1, 1.0, 2.0, 1.0 );
	t[6] = vips_image_new_matrixv( 1, 3, 1.0, 0.0, -1.0 );
	vips_image_set_double( t[6], "offset", 128.0 );

	if(vips_conv(in, &t[7], t[5], "precision", precision, NULL) || vips_conv(t[7], &Gy, t[6], "precision", precision, NULL));
	{
		g_object_unref(temp);
		return (-1);
	}

	g_object_unref(temp);
	return (0);
}

//step 2.5: Extracting gradient direction

#define POLAR_UCHAR { \
	for( x = 0; x < r->width; x++ ) { \
		for( band = 0; band < Gx->Bands; band++ ) { \
			int gx = p1[band] - 128; \
			int gy = p2[band] - 128; \
			\
			int i = ((gx >> 4) & 0xf) | (gy & 0xf0); \
			\
			q[0] = (gx * gx + gy * gy + 256) >> 9; \
			q[1] = vips_canny_polar_atan2[i]; \
			\
			q += 2; \
		} \
		\
		p1 += Gx->Bands; \
		p2 += Gx->Bands; \
	} \
}

/* Float/double path. We keep the same ranges as the uchar path to reduce
 * confusion.
 */
#define POLAR( TYPE ) { \
	TYPE *tp1 = (TYPE *) p1; \
	TYPE *tp2 = (TYPE *) p2; \
	TYPE *tq = (TYPE *) q; \
	\
	for( x = 0; x < r->width; x++ ) { \
		for( band = 0; band < Gx->Bands; band++ ) { \
			double gx = tp1[band]; \
			double gy = tp2[band]; \
			double theta = VIPS_DEG( atan2( gx, gy ) ); \
			\
			tq[0] = (gx * gx + gy * gy) / 512.0; \
			tq[1] = 256.0 * fmod( theta + 360.0, 360.0 ) / 360.0; \
			\
			tq += 2; \
		} \
		\
		tp1 += Gx->Bands; \
		tp2 += Gx->Bands; \
	} \
}


// step 2.5: Generating the gradient direction image.
static int
vips_canny_polar_generate( VipsRegion *or,
	void *vseq, void *a, void *b, gboolean *stop )
{
	VipsRegion **in = (VipsRegion **) vseq;
	VipsRect *r = &or->valid;
	VipsImage *Gx = in[0]->im;

	int x, y, band;

	if( vips_reorder_prepare_many( or->im, in, r ) )
		return( -1 );

	for( y = 0; y < r->height; y++ ) {
		VipsPel *p1 = (VipsPel * restrict)
			VIPS_REGION_ADDR( in[0], r->left, r->top + y );
		VipsPel *p2 = (VipsPel * restrict)
			VIPS_REGION_ADDR( in[1], r->left, r->top + y );
		VipsPel *q = (VipsPel * restrict)
			VIPS_REGION_ADDR( or, r->left, r->top + y );

		switch( Gx->BandFmt ) {
		case VIPS_FORMAT_UCHAR:
			POLAR_UCHAR;
			break;

		case VIPS_FORMAT_FLOAT:
			POLAR( float );
			break;

		case VIPS_FORMAT_DOUBLE:
			POLAR( double );
			break;

		default:
			g_assert( FALSE );
		}
	}

	return( 0 );
}

static int
vips_canny_polar( VipsImage **args, VipsImage **out )
{
	static GOnce once = G_ONCE_INIT;

	g_once( &once, (GThreadFunc) vips_atan2_init, NULL );

	*out = vips_image_new();
	if( vips_image_pipeline_array( *out,
		VIPS_DEMAND_STYLE_THINSTRIP, args ) )
		return( -1 );
	(*out)->Bands *= 2;

	if( vips_image_generate( *out,
		vips_start_many, vips_canny_polar_generate, vips_stop_many,
		args, NULL ) )
		return( -1 );

	return( 0 );
}




static int vips_canny_build ( VipsObject *object)
{
	VipsCanny *canny= (VipsCanny *) object;
	VipsImage **dummy= (VipsImage**) vips_object_local_array(object,20);

	VipsImage *in;

	VipsImage *Gx;
	VipsImage *Gy;

	in = canny->in;

	if(VIPS_OBJECT_CLASS(vips_canny_parent_class) -> build(object))
	return (-1);


	if (vips_gaussblur(in, &dummy[0], canny->sigma, NULL))
		vips_error_exit ("unable to blur");


	in =dummy[0];

	if (vips_canny_gradient(in, &Gx, &Gy))
	return (-1);

	in= dummy[1];

	canny->args[0] = Gx;
	canny->args[1] = Gy;
	canny->args[2] = NULL;

	if( vips_canny_polar( canny->args, &dummy[1] ) )
	return( -1 );

	in = dummy[2];

	g_object_set( object, "out", vips_image_new(), NULL );
	if(vips_image_write(in,canny->out))
	return (-1);

}


static void vips_canny_class_init (VipsCannyClass *class)
{
	GObjectClass *gobject_class = G_OBJECT_CLASS (class);
	VipsObjectClass *object_class = (VipsObjectClass *) class;
	VipsOperationClass *operation_class = VIPS_OPERATION_CLASS (class);

	gobject_class->set_property= vips_object_set_property;
	gobject_class->get_property= vips_object_get_property;

	object_class->nickname= "canny";
	object_class->description= _("gaussian blur");
	object_class->build = vips_canny_build;

	operation_class->flags= VIPS_OPERATION_SEQUENTIAL;

	VIPS_ARG_IMAGE(class, "in", 1,
		_( "Input" ),
		_( "Input image" ),
		VIPS_ARGUMENT_REQUIRED_INPUT,
		G_STRUCT_OFFSET( VipsCanny, in ) );

	VIPS_ARG_IMAGE( class, "out", 2,
		_( "Output" ),
		_( "Output image" ),
		VIPS_ARGUMENT_REQUIRED_OUTPUT,
		G_STRUCT_OFFSET( VipsCanny, out ) );

	VIPS_ARG_DOUBLE( class, "sigma", 10,
		_( "Sigma" ),
		_( "Sigma of Gaussian" ),
		VIPS_ARGUMENT_OPTIONAL_INPUT,
		G_STRUCT_OFFSET( VipsCanny, sigma ),
		0.01, 1000, 1.4 );

}

static void vips_canny_init( VipsCanny *canny)
{
canny->sigma=1.4;
//nothing to initialise yet
}



int vips_canny (VipsImage *in, VipsImage **out,...)
{
va_list ap;

int result;

va_start (ap,out);

result= vips_call_split("canny", ap, in, out);

va_end(ap);

return (result);
}

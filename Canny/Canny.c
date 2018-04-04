

/*
#define DEBUG
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif /*HAVE_CONFIG_H*/
#include <vips/intl.h>

#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#include <vips/vips.h>

typedef struct _VipsCanny {
	VipsOperation parent_instance;

	VipsImage *in;
	VipsImage *out;

	double sigma;

	/* Need an image vector for start_many.
	 */
	VipsImage *args[3];
} VipsCanny;

typedef VipsOperationClass VipsCannyClass;

G_DEFINE_TYPE( VipsCanny, vips_canny, VIPS_TYPE_OPERATION );

/* Simple 2x2 -1/+1 difference. For uchar, we try to hit the vector path and
 * use an offset rather than -ves. Otherwise it's float or double output.
 */
static int
vips_canny_gradient( VipsImage *in, VipsImage **Gx, VipsImage **Gy )
{
	VipsImage *scope;
	VipsImage **t;
	VipsPrecision precision;

	scope = vips_image_new();
	t = (VipsImage **) vips_object_local_array( (VipsObject *) scope, 2 );

	t[0] = vips_image_new_matrixv( 2, 2,
		-1.0, 1.0,
		-1.0, 1.0 );

	if( in->BandFmt == VIPS_FORMAT_UCHAR ) {
		precision = VIPS_PRECISION_INTEGER;
		vips_image_set_double( t[0], "offset", 128.0 );
	}
	else
		precision = VIPS_PRECISION_FLOAT;

	if( vips_conv( in, Gx, t[0], "precision", precision, NULL ) ||
		vips_rot90( t[0], &t[1], NULL ) ||
		vips_conv( in, Gy, t[1], "precision", precision, NULL ) ) {
		g_object_unref( scope );
		return( -1 );
	}

	g_object_unref( scope );

	return( 0 );
}

/* LUT for calculating atan2() with +/- 4 bits of precision in each axis.
 */
static VipsPel vips_canny_polar_atan2[256];

/* For the uchar path, gx/gy are -128 to +127, and we need -8 to +7 for the
 * atan2 LUT. Try "256 * (VIPS_DEG( atan2( gx, gy ) ) + 360) / 360;" to test
 * the LUT.
 *
 * For G, we should calculate sqrt( gx * gx + gy * gy ), however we are only
 * interested in relative magnitude (max of sqrt), so we can skip the sqrt
 * itself. We need a result that will fit in 0 - 255, so shift down.
 */
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

/* Calculate G/theta from Gx/Gy. We code theta as 0-256 for 0-360
 * and skip the sqrt on G.
 *
 * For a white disc on a black background, theta is 0 at the top, 64 on the
 * left, 128 on the right and 192 on the right edge.
 */
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

/* Set G to 0 if it's not the local maxima in the direction of the gradient.
 */
#define THIN_UCHAR { \
	for( x = 0; x < r->width; x++ ) { \
		for( band = 0; band < out_bands; band++ ) {  \
			int G = p[lsk + psk]; \
			int theta = p[lsk + psk + 1]; \
			int low_theta = (theta / 32) & 0x7; \
			int high_theta = (low_theta + 1) & 0x7; \
			int residual = theta - low_theta * 32; \
			int lowa = p[offset[low_theta]]; \
			int lowb = p[offset[high_theta]]; \
			int low = (lowa * (32 - residual) +  \
				lowb * residual) / 32; \
			int higha = p[offset[(low_theta + 4) & 0x7]]; \
			int highb = p[offset[(high_theta + 4) & 0x7]]; \
			int high = (higha * (32 - residual) + \
				highb * residual) / 32; \
			\
			if( G <= low || \
				G < high ) \
				G = 0; \
			\
			q[band] = G; \
			\
			p += 2; \
		} \
		\
		q += out_bands; \
	} \
}

#define THIN( TYPE ) { \
	TYPE *tp = (TYPE *) p; \
	TYPE *tq = (TYPE *) q; \
	\
	for( x = 0; x < r->width; x++ ) { \
		for( band = 0; band < out_bands; band++ ) {  \
			TYPE G = tp[lsk + psk]; \
			TYPE theta = tp[lsk + psk + 1]; \
			int low_theta = ((int) (theta / 32)) & 0x7; \
			int high_theta = (low_theta + 1) & 0x7; \
			TYPE residual = theta - low_theta * 32; \
			TYPE lowa = tp[offset[low_theta]]; \
			TYPE lowb = tp[offset[high_theta]]; \
			TYPE low = (lowa * (32 - residual) + \
				lowb * residual) / 32; \
			TYPE higha = tp[offset[(low_theta + 4) & 0x7]]; \
			TYPE highb = tp[offset[(high_theta + 4) & 0x7]]; \
			TYPE high = (higha * (32 - residual) + \
				highb * residual) / 32; \
			\
			if( G <= low || \
				G < high ) \
				G = 0; \
			\
			tq[band] = G; \
			\
			tp += 2; \
		} \
		\
		tq += out_bands; \
	} \
}

static int
vips_canny_thin_generate( VipsRegion *or,
	void *vseq, void *a, void *b, gboolean *stop )
{
	VipsRegion *in = (VipsRegion *) vseq;
	VipsRect *r = &or->valid;
	VipsImage *im = in->im;
	int out_bands = or->im->Bands;

	VipsRect rect;
	int x, y, band;
	int lsk;
	int psk;

	int offset[8];

	rect = *r;
	rect.width += 2;
	rect.height += 2;
	if( vips_region_prepare( in, &rect ) )
		return( -1 );

	/* These are in typed units.
	 */
	lsk = VIPS_REGION_LSKIP( in ) / VIPS_IMAGE_SIZEOF_ELEMENT( im );
	psk = VIPS_IMAGE_SIZEOF_PEL( im ) / VIPS_IMAGE_SIZEOF_ELEMENT( im );

	/* For each of the 8 directions, the offset to get to that pixel from
	 * the top-left of the 3x3.
	 *
	 *   1 | 0 | 7
	 *   --+---+--
	 *   2 | X | 6
	 *   --+---+--
	 *   3 | 4 | 5
	 */
	offset[0] = psk;
	offset[1] = 0;
	offset[2] = lsk;
	offset[3] = 2 * lsk;
	offset[4] = 2 * lsk + psk;
	offset[5] = 2 * lsk + 2 * psk;
	offset[6] = lsk + 2 * psk;
	offset[7] = 2 * psk;

	for( y = 0; y < r->height; y++ ) {
		VipsPel *p = (VipsPel * restrict)
			VIPS_REGION_ADDR( in, r->left, r->top + y );
		VipsPel *q = (VipsPel * restrict)
			VIPS_REGION_ADDR( or, r->left, r->top + y );

		switch( im->BandFmt ) {
		case VIPS_FORMAT_UCHAR:
			//THIN_UCHAR;
			THIN( unsigned char );
			break;

		case VIPS_FORMAT_FLOAT:
			THIN( float );
			break;

		case VIPS_FORMAT_DOUBLE:
			THIN( double );
			break;

		default:
			g_assert( FALSE );
		}
	}

	return( 0 );
}

/* Remove non-maximal edges. At each point, compare the G to the G in either
 * direction and 0 it if it's not the largest.
 */
static int
vips_canny_thin( VipsImage *in, VipsImage **out )
{
	*out = vips_image_new();
	if( vips_image_pipelinev( *out,
		VIPS_DEMAND_STYLE_THINSTRIP, in, NULL ) )
		return( -1 );
	(*out)->Bands /= 2;
	(*out)->Xsize -= 2;
	(*out)->Ysize -= 2;

	if( vips_image_generate( *out,
		vips_start_one, vips_canny_thin_generate, vips_stop_one,
		in, NULL ) )
		return( -1 );

	return( 0 );
}



static int vips_canny_threshold(VipsRegion *or,
	void *vseq, void *a, void *b, gboolean *stop ){

		VipsRegion *in = (VipsRegion *) vseq;
		VipsRect *r = &or->valid;
		VipsImage *im = in->im;
		int total_pixels= (r->width)*(r->height)
		int histogram[total_pixels]={0};

		for (int j=0; j<r->height; j++){
			for (int i=0;i< r->width; i++){
				int h=0xFF & (int * restrict)
					VIPS_REGION_ADDR( in, r->left + i, r->top + j );
				histogram[h]++;
			}
		}
		float sum = 0;

		for (int t=0 ; t<256 ; t++){
			sum += t * histogram[t];
		}

		float sumB = 0;
		int wB = 0;
		int wF = 0;
		int threshold = 0;
		float varMax = 0;					//stores the max value of varaince

		for (int t=0 ; t<256 ; t++) {
   		wB += histogram[t];               // Weight Background
   		if (wB == 0) continue;

   		wF = total_pixels - wB;           // Weight Foreground
   		if (wF == 0) break;

   		sumB += (float) (t * histData[t]);

   		float mB = sumB / wB;            // Mean Background
   		float mF = (sum - sumB) / wF;    // Mean Foreground

   // Calculate Between Class Variance
   		float varBetween = (float)wB * (float)wF * (mB - mF) * (mB - mF);

   // Check if new maximum found
   		if (varBetween > varMax) {
      		varMax = varBetween;
      		threshold = t;
   		}
		}
		printf (" threshold is: %d",t);
	return t;
	}




static int
vips_canny_build( VipsObject *object )
{
	VipsCanny *canny = (VipsCanny *) object;
	VipsImage **t = (VipsImage **) vips_object_local_array( object, 5 );

	VipsImage *in;
	VipsImage *Gx;
	VipsImage *Gy;

	if( VIPS_OBJECT_CLASS( vips_canny_parent_class )->build( object ) )
		return( -1 );

	in = canny->in;

	if( vips_gaussblur( in, &t[0], canny->sigma, NULL ) )
		return( -1 );
	in = t[0];

	if( vips_canny_gradient( in, &Gx, &Gy ) )
		return( -1 );

	/* Form (G, theta).
	 */
	canny->args[0] = Gx;
	canny->args[1] = Gy;
	canny->args[2] = NULL;
	if( vips_canny_polar( canny->args, &t[1] ) )
		return( -1 );
	in = t[1];

	/* Expand by two pixels all around, then thin in the direction of the
	 * gradient.
	 */
	if( vips_embed( in, &t[2], 1, 1, in->Xsize + 2, in->Ysize + 2,
		"extend", VIPS_EXTEND_COPY,
		NULL ) )
		return( -1 );

	if( vips_canny_thin( t[2], &t[3] ) )
		return( -1 );
	in = t[3];

	g_object_set( object, "out", vips_image_new(), NULL );

	if( vips_image_write( in, canny->out ) )
		return( -1 );

	return( 0 );
}


static void
vips_canny_class_init( VipsCannyClass *class )
{
	GObjectClass *gobject_class = G_OBJECT_CLASS( class );
	VipsObjectClass *object_class = (VipsObjectClass *) class;
	VipsOperationClass *operation_class = VIPS_OPERATION_CLASS( class );

	gobject_class->set_property = vips_object_set_property;
	gobject_class->get_property = vips_object_get_property;

	object_class->nickname = "canny";
	object_class->description = _( "Canny edge detector" );
	object_class->build = vips_canny_build;

	operation_class->flags = VIPS_OPERATION_SEQUENTIAL;

	VIPS_ARG_IMAGE( class, "in", 1,
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

static void
vips_canny_init( VipsCanny *canny )
{
	canny->sigma = 1.4;
}

/**
 * vips_canny: (method)
 * @in: input image
 * @out: (out): output image
 * @sigma: how large a mask to use
 * @...: %NULL-terminated list of optional named arguments
 *
 * Optional arguments:
 *
 * * @sigma: %gdouble, sigma for gaussian blur
 *
 * Find edges by Canny's method: The maximum of the derivative of the gradient
 * in the direction of the gradient. This operation will only work well on
 * 8-bit luminance images.
 *
 * Use @sigma to control the scale over which gradient is measured. 1.4 is
 * usually a good value.
 *
 * You will probably need to process the output further to eliminate weak
 * edges.
 *
 * See also: vips_sobel().
 *
 * Returns: 0 on success, -1 on error.
 */
int
vips_canny( VipsImage *in, VipsImage **out, ... )
{
	va_list ap;
	int result;

	va_start( ap, out );
	result = vips_call_split( "canny", ap, in, out );
	va_end( ap );

	return( result );
}

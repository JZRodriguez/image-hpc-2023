#include <stdio.h>
#include <stdlib.h>
#include <mpi.h>	
#include <math.h>
#include "spng.h"
#include "inttypes.h"

const char* color_type_str(enum spng_color_type color_type)
{
	switch (color_type)
	{
		case SPNG_COLOR_TYPE_GRAYSCALE: return "grayscale";
		case SPNG_COLOR_TYPE_TRUECOLOR: return "truecolor";
		case SPNG_COLOR_TYPE_INDEXED: return "indexed color";
		case SPNG_COLOR_TYPE_GRAYSCALE_ALPHA: return "grayscale with alpha";
		case SPNG_COLOR_TYPE_TRUECOLOR_ALPHA: return "truecolor with alpha";
		default: return "(invalid)";
	}
}

int main(int argc, char** argv)
{
	int numproc;
	int miproc;

	MPI_Status status;

	MPI_Init(&argc, &argv); /* Inicializar MPI */
	MPI_Comm_rank(MPI_COMM_WORLD, &miproc); /* Determinar el rango del proceso invocado*/
	MPI_Comm_size(MPI_COMM_WORLD, &numproc); /* Determinar el numero de procesos */

	char* IMG_PATH = "./photo.png";
	char* OUT_PATH = "./out.png";
	int ret = 0;

	FILE* png;
	spng_ctx* ctx = NULL;
	unsigned char* image = NULL;
	png = fopen(IMG_PATH, "rb");

	ctx = spng_ctx_new(0);

	/* Ignore and don't calculate chunk CRC's */
	spng_set_crc_action(ctx, SPNG_CRC_USE, SPNG_CRC_USE);

	/* Set memory usage limits for storing standard and unknown chunks,
	   this is important when reading untrusted files! */
	size_t limit = 1024 * 1024 * 64;
	spng_set_chunk_limits(ctx, limit, limit);

	/* Set source PNG */
	spng_set_png_file(ctx, png); /* or _buffer(), _stream() */

	struct spng_ihdr ihdr;
	spng_get_ihdr(ctx, &ihdr);

	const char* color_name = color_type_str(ihdr.color_type);

	if(miproc==0){
		printf("width: %u\n"
			"height: %u\n"
			"bit depth: %u\n"
			"color type: %u - %s\n",
			ihdr.width, ihdr.height, ihdr.bit_depth, ihdr.color_type, color_name);

		printf("compression method: %u\n"
			"filter method: %u\n"
			"interlace method: %u\n",
			ihdr.compression_method, ihdr.filter_method, ihdr.interlace_method);
	}

	struct spng_plte plte = { 0 };
	spng_get_plte(ctx, &plte);

	if(miproc==0){
		printf("palette entries: %u\n", plte.n_entries);
	}

	size_t image_size, image_width;

	int fmt = SPNG_FMT_PNG;

	spng_decoded_image_size(ctx, fmt, &image_size);

	image = malloc(image_size);

	ret = spng_decode_image(ctx, image, image_size, SPNG_FMT_RGBA8, 0);

	image_width = image_size / ihdr.height;

	/*START HERE*/

	unsigned int pi = (image_size / numproc) * miproc;
	unsigned int pf = (image_size / numproc) * (miproc + 1);
	pi=pi-pi%4;
	pf=pf-pf%4;

	
	for (int i = pi; i < pf; i+=4) {
		image[i] = 0;
	}


	if (miproc == 0) {
		unsigned char* el_bufeandum = malloc(image_size / numproc + 1);
		for (int i = 1; i < numproc; i++) {
			MPI_Recv(el_bufeandum, image_size / numproc, MPI_CHAR, i, 0, MPI_COMM_WORLD, &status);
				
			pi = (image_size / numproc) * i;
			pf = (image_size / numproc) *(i + 1);
			pi=pi-pi%4;
			pf=pf-pf%4;

			for (int j=pi;j<pf;j++){
				image[j]=el_bufeandum[j-pi];
			}
		}
		free(el_bufeandum);

	} else {
		MPI_Send(image+pi, image_size / numproc, MPI_CHAR, 0, 0, MPI_COMM_WORLD);
	}

	/*END HERE*/

	if (miproc == 0) {
		FILE* outpng;
		spng_ctx* outctx = NULL;
		outpng = fopen(OUT_PATH, "wb");
		outctx = spng_ctx_new(SPNG_CTX_ENCODER);
		ret = spng_set_png_file(outctx, outpng);
		ret = spng_set_ihdr(outctx, &ihdr);
		ret = spng_encode_image(outctx, image, image_size, SPNG_FMT_PNG, 0);
		ret = spng_encode_chunks(outctx);
	}
	
	MPI_Barrier(MPI_COMM_WORLD);
	MPI_Finalize();

	return 0;
}


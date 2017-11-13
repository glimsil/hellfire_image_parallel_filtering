#include <hellfire.h>
#include <noc.h>
#include "image.h"

int32_t CPU_N = 8;
int32_t X_CHUNKS = 4;
int32_t Y_CHUNKS = 2;


uint8_t gaussian(uint8_t buffer[5][5]){
	int32_t sum = 0, mpixel;
	uint8_t i, j;

	int16_t kernel[5][5] =	{	{2, 4, 5, 4, 2},
					{4, 9, 12, 9, 4},
					{5, 12, 15, 12, 5},
					{4, 9, 12, 9, 4},
					{2, 4, 5, 4, 2}
				};
	for (i = 0; i < 5; i++)
		for (j = 0; j < 5; j++)
			sum += ((int32_t)buffer[i][j] * (int32_t)kernel[i][j]);
	mpixel = (int32_t)(sum / 159);

	return (uint8_t)mpixel;
}

uint32_t isqrt(uint32_t a){
	uint32_t i, rem = 0, root = 0, divisor = 0;

	for (i = 0; i < 16; i++){
		root <<= 1;
		rem = ((rem << 2) + (a >> 30));
		a <<= 2;
		divisor = (root << 1) + 1;
		if (divisor <= rem){
			rem -= divisor;
			root++;
		}
	}
	return root;
}

uint8_t sobel(uint8_t buffer[3][3]){
	int32_t sum = 0, gx = 0, gy = 0;
	uint8_t i, j;

	int16_t kernelx[3][3] =	{	{-1, 0, 1},
					{-2, 0, 2},
					{-1, 0, 1},
				};
	int16_t kernely[3][3] =	{	{-1, -2, -1},
					{0, 0, 0},
					{1, 2, 1},
				};
	for (i = 0; i < 3; i++){
		for (j = 0; j < 3; j++){
			gx += ((int32_t)buffer[i][j] * (int32_t)kernelx[i][j]);
			gy += ((int32_t)buffer[i][j] * (int32_t)kernely[i][j]);
		}
	}

	sum = isqrt(gy * gy + gx * gx);

	if (sum > 255) sum = 255;
	if (sum < 0) sum = 0;

	return (uint8_t)sum;
}

void do_gaussian(uint8_t *input, uint8_t *output, int32_t width, int32_t height){
	int32_t i = 0, j = 0, k, l;
	uint8_t image_buf[5][5];

	for(i = 0; i < height; i++){
		if (i > 1 && i < height-2){
			for(j = 0; j < width; j++){
				if (j > 1 && j < width-2){
					for (k = 0; k < 5; k++)
						for(l = 0; l < 5; l++)
							image_buf[k][l] = input[(((i + l-2) * width) + (j + k-2))];

					output[((i * width) + j)] = gaussian(image_buf);
				}else{
					output[((i * width) + j)] = 0;
				}
			}
		}else{
			output[((i * width) + j)] = 0;
		}
	}
}

void do_sobel(uint8_t *input, uint8_t *output, int32_t width, int32_t height){
	int32_t i = 0, j = 0, k, l;
	uint8_t image_buf[3][3];

	for(i = 0; i < height; i++){
		if (i > 2 && i < height-3){
			for(j = 0; j < width-1; j++){
				if (j > 2 && j < width-3){
					for (k = 0; k < 3; k++)
						for(l = 0; l < 3; l++)
							image_buf[k][l] = input[(((i + l-1) * width) + (j + k-1))];

					output[((i * width) + j)] = sobel(image_buf);
				}else{
					output[((i * width) + j)] = input[((i * width) + j)];
					// output[((i * width) + j)] = 0;
				}
			}
		}else{
			// output[((i * width) + j)] = 0;
			output[((i * width) + j)] = input[((i * width) + j)];
		}
	}
}
// input, output, height, width, block line, block column, block height, block width
void sub_matrix(uint8_t *input, uint8_t *output, int32_t h, int32_t w, int32_t k, int32_t l, int32_t bh, int32_t bw) {
	int32_t i = 0;
	int32_t _i = 0;

	while (i < bh){
		int32_t j = 0;
		while (j < bw) {
			output[_i] = input[(i + k) * w + (j + l)];
			_i += 1;
			//print ("%d\t" % a[(i + k) * w + (j + l)]),
			j += 1;
		}
		i += 1;
	}
}

void print_m(uint8_t *m, uint8_t w, uint8_t h) {
	int32_t x, y, k=0;
	for(y=0; y<h; y++){
		for(x=0; x<w; x++){
			printf("%d, ", m[y * w + x]);
		}
		printf("\n");
	}
}

void escravo(void) {
	int32_t i;
	int16_t val;
	int32_t index = 0;
	uint16_t cpu, port, size;



	if (hf_comm_create(hf_selfid(), 5000, 0))
		panic(0xff);

	int32_t OFFSETX_SIZE=width/X_CHUNKS;
	int32_t OFFSETY_SIZE=height/Y_CHUNKS;

	int32_t padding = 4;
	int32_t off_padding = 2;


	int32_t _OFFSETX_SIZE=OFFSETX_SIZE + padding;
	int32_t _OFFSETY_SIZE=OFFSETY_SIZE + padding;

	int32_t array_size = OFFSETX_SIZE * OFFSETY_SIZE;
	int32_t _array_size = (_OFFSETX_SIZE) * (_OFFSETY_SIZE);
	// uint8_t img[array_size];
	uint8_t *img, *img2, *img3, *img4;
	// uint8_t *img;
	img = (uint8_t *) malloc(array_size);
	img2= (uint8_t *) malloc(_array_size);
	img3= (uint8_t *) malloc(_array_size);
	img4= (uint8_t *) malloc(_array_size);
	//

	while(1) {

		i = hf_recvprobe();
		if (i >= 0) {
			// recebe a mensagem do mestre
			val = hf_recvack(&cpu, &port, img, &size, i);
			if (val)
				printf("hf_recvack(): error %d\n", val);
			else{

				// print_m(img, OFFSETX_SIZE, OFFSETY_SIZE);

				// printf("-------------\n");
				// printf("sizex: %d, sizey: %d\n", _OFFSETX_SIZE, _OFFSETY_SIZE);
				//add_padding
				int32_t x, y, k=0;
				for(y=0; y<_OFFSETY_SIZE; y++){
					for(x=0; x<_OFFSETX_SIZE; x++){
						// printf("x: %d, y: %d\n", x, y);

						// if(x < 2 || y < 2){
						if(x < off_padding || y < off_padding || (x+off_padding) >= _OFFSETX_SIZE || (y+off_padding) >= _OFFSETY_SIZE){
							img2[y * _OFFSETX_SIZE + x] = 0xff;
						}else {
							// printf("VTNC%d,",img[k]);
							img2[y * _OFFSETX_SIZE + x] = img[k];
							k++;
						}
					}
				}

				// print_m(img2, _OFFSETX_SIZE, _OFFSETY_SIZE);

				do_gaussian(img2, img3, _OFFSETX_SIZE, _OFFSETY_SIZE);
				do_sobel(img3, img4, _OFFSETX_SIZE, _OFFSETY_SIZE);


				// input, output, height, width, block line, block column, block height, block width
				sub_matrix(img4, img, _OFFSETY_SIZE, _OFFSETX_SIZE, off_padding, off_padding, OFFSETY_SIZE, OFFSETX_SIZE);


				// print_m(img, OFFSETX_SIZE, OFFSETY_SIZE);


				// descobri as 2 da manha que se nao botar isso aqui
				// ele responde pro mestre antes de o mestre enviar tudo pros escravos
				// e ai a merda acontece
				delay_ms(50);
				// envia os dados alterados pro mestre
				// val = hf_sendack(0, 1000, img3, size, 0, 3000);
				val = hf_sendack(0, 1000, img, size, 0, 3000);
				// free(img);
				free(img2);
				free(img3);
				free(img4);
			}
		}
	}
}



void mestre(void) {

	//uint32_t time;
	uint16_t cpu, port, size, i;
	int16_t val;
	int32_t time;

	if (hf_comm_create(hf_selfid(), 1000, 0)){
		panic(0xff);
	}

	int32_t OFFSETX_SIZE=width/X_CHUNKS;
	int32_t OFFSETY_SIZE=height/Y_CHUNKS;

	// final image
	int8_t img[height * width];
	uint8_t *output = (uint8_t *) malloc(OFFSETX_SIZE * OFFSETY_SIZE);



	// recebe a resposta
	while (1){

		time = _readcounter();
		// comeca no 1 pq 0 eh o mestre
		int32_t x, y = 0;
		int32_t cpu_counter=1;
		for(y=0; y < Y_CHUNKS; y++) {
			for(x=0; x < X_CHUNKS; x++) {
				// matrix, outputm, height, width, block line, block column, block height, block width
				// printf("x= %d, block line = %d, block width = %d\n", x, x*OFFSETX_SIZE, OFFSETX_SIZE);
				// printf("y= %d, block height = %d, block height = %d\n", y, y*OFFSETY_SIZE, OFFSETY_SIZE);
				sub_matrix(image, output, height, width, y*OFFSETY_SIZE, x*OFFSETX_SIZE, OFFSETY_SIZE, OFFSETX_SIZE);
				//
				// int32_t l = 0;
				// printf("--------cpu:%d\n", cpu_counter);
				// while(l < OFFSETX_SIZE * OFFSETY_SIZE) {
				// 	printf("%d ,", output[l]);
				// 	l++;
				// }
				// printf("\n");
				val = hf_sendack(cpu_counter, 5000, output, OFFSETX_SIZE * OFFSETY_SIZE, hf_cpuid(), 500);
				if (val){
					printf("hf_sendack(): error %d\n", val);
				}

				cpu_counter++;
			}
			//	matrix_print_sub(m, 9, 8, 1, 2, 3, 2)
			// envia um chunk pra cada escravo
			//val = hf_sendack(cpu_counter, 5000, &buf[(cpu_counter-1) * offset], sizeof(buf)/CPU_N, hf_cpuid(), 500);
		}

		cpu_counter = 1;
		while(1) {
			// img = (uint8_t *) malloc(height * width)

			// agora recebe
			i = hf_recvprobe();
			if (i >= 0) {

				val = hf_recvack(&cpu, &port, output, &size, 0);
				if (val){
					printf("hf_recvack(): error %d\n", val);
				} else {


					// faz um calculo pra saber onde pelo ID da cpu
					// poe o resultado

					int32_t offx, a = 0;
					int32_t offy, b = 0;

					// if(cpu <= X_CHUNKS) {
					offy = (cpu-1)/X_CHUNKS;
					offx = (cpu-1) - (offy * X_CHUNKS);
					// } else {
					// 	offx = 1;
					// 	offy = cpu-1 - X_CHUNKS;
					// }
					// for(x=1; x < cpu-1; x += X_CHUNKS) {
					// 	offy ++;
					// }
					// offx = cpu - y;
					//
					// printf("cpu: %d, OFFSETX: %d, OFFSETY: %d, offsetxsize: %d, offsetysize: %d, sizeof output:%d\n", cpu, offx, offy, OFFSETX_SIZE, OFFSETY_SIZE, size);
					// printf("offx = %d, offy = %d, sum = %d\n", offx * OFFSETX_SIZE, offy * OFFSETY_SIZE, (offx * OFFSETX_SIZE) + (offy * OFFSETY_SIZE));


					// monta a matriz de volta
					int32_t currx = offx * OFFSETX_SIZE;
					int32_t curry = offy * OFFSETY_SIZE;
					// printf("CURRX = %d\n", currx);
					// printf("CURRY = %d\n", curry);
					int32_t output_i = 0;
					for(y=0; y< OFFSETY_SIZE; y++){
						for(x=0; x< OFFSETX_SIZE; x++){
							a = currx + x;
							b = curry + y;
							// printf("pos = %d, ", a + (width * y) + (width * curry));
							img[a + (width * y) + (width * curry)] = output[output_i];
							// img[a * OFFSETX_SIZE + b] = output[output_i];
							output_i++;
						}
					}
					// printf("cpu %d size of img %d\n", cpu, sizeof(output));
					// int32_t x = 0;
					// while(x< size){
					// 	printf("%d ,", output[x]);
					// 	// img[x + (offset * (cpu_counter-1))] = output[x];
					// 	x++;
					// }
					// printf("\n---------------------------------\n");
					cpu_counter++;
					// printf("cpu_counter = %d\n", cpu_counter);
				}
			}

			if(cpu_counter == CPU_N+1) {
				// se entrou aqui eh pq recebeu todas as mensages
				cpu_counter = 0;
				// printa o resultado
				// printf("size of img %d", sizeof(img));
				// while(index < sizeof(img)) {int32_t X_CHUNKS = 2;
				// 	printf("index: %d, msg: %d\n", index, img[index]);
				// 	index++;
				// }
				time = _readcounter() - time;
				printf("done in %d clock cycles.\n\n", time);

				int32_t i,j, k = 0;
				printf("\n\nint32_t width = %d, height = %d;\n", width, height);
				printf("uint8_t image[] = {\n");
				// for(i=0; i < width * height; i++) {
				// 	printf("%d, ", img[i]);
				// }
				for (i = 0; i < height; i++){
					for (j = 0; j < width; j++){
						printf("0x%x", img[i * width + j]);
						// printf("%d", img[i * width + j]);
						if ((i < height-1) || (j < width-1)) printf(", ");
						if ((++k % 16) == 0) printf("\n");
					}
				}
				printf("};\n");

				free(output);
				free(img);
				// entra em loop pq nao tem mais nada pra fazer
				while(1){}
			}
		}
	}
}

void app_main(void){
	if (hf_cpuid() == 0){
		//hf_spawn(mestre, 0, 0, 0, "mestre", 4096);
		hf_spawn(mestre, 0, 0, 0, "mestre", 200000);
	}else{
		//hf_spawn(escravo, 0, 0, 0, "escravo", 4096);
		hf_spawn(escravo, 0, 0, 0, "escravo", 200000);
	}
}

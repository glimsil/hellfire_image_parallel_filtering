#include <hellfire.h>
#include <noc.h>
#include "image.h"

int32_t CPU_N = 8;

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
					output[((i * width) + j)] = input[((i * width) + j)];
				}
			}
		}else{
			output[((i * width) + j)] = input[((i * width) + j)];
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
					output[((i * width) + j)] = 0;
				}
			}
		}else{
			output[((i * width) + j)] = 0;
		}
	}
}



void escravo(void) {
	int32_t i;
	int16_t val;
	// int32_t index = 0;
	int16_t chunks = 4;

	if (hf_comm_create(hf_selfid(), 5000, 0))
	panic(0xff);

	int32_t offset = (height * width)/(CPU_N * chunks);
	uint8_t img[offset];
	int32_t c=0;
	while(1) {
		int32_t index = 0;
		uint16_t cpu, port, size;

		i = hf_recvprobe();
		if (i >= 0) {
			c++;
			// recebe a mensagem do mestre
			val = hf_recvack(&cpu, &port, img, &size, i);
			if (val)
			printf("hf_recvack(): error %d\n", val);
			else{

				// processa os dados recebidos
				while(index < size){
					printf("I am cpu: %d, index: %d, msg: %d, size: %d\n", hf_cpuid(), index, img[index], size);
					img[index] = 1;
					index++;
				}

				printf("Img size %d, c=%d\n", size, c);

				// descobri as 2 da manha que se nao botar isso aqui
				// ele responde pro mestre antes de o mestre enviar tudo pros escravos
				// e ai a merda acontece
				delay_ms(50);
				// envia os dados alterados pro mestre
				val = hf_sendack(0, 1000, img, size, i, 3000);
				free(img);
			}
		}
	}
}


void mestre(void) {

	int32_t i, index, i_chunk;
	//uint32_t time;
	uint16_t cpu, port, size;
	int16_t val;
	int16_t chunks = 4;

	uint8_t *buf;

	if (hf_comm_create(hf_selfid(), 1000, 0)){
		panic(0xff);
	}

  // tamanho de cada array pra ser mandando a cada escravo
  int32_t offset = (height * width)/(CPU_N * chunks);

	// final image
	int8_t img[height * width];

	printf("OFFSET SIZE %d", offset);


  // recebe a resposta
	while (1){
		for(i_chunk=0; i_chunk<chunks;i_chunk++){

			// comeca no 1 pq 0 eh o mestre
			for(index=1; index <= CPU_N; index++) {
				// envia um chunk pra cada escravo
				//val = hf_sendack(index, 5000, &buf[(index-1) * offset], sizeof(buf)/CPU_N, hf_cpuid(), 500);
				val = hf_sendack(index, 5000, &image[(i_chunk * offset) + (index-1) * offset], offset, hf_cpuid(), 500);

				if (val){
					printf("hf_sendack(): error %d\n", val);
				}
			}

			index = 1;
			buf = (uint8_t *) malloc(offset);
			// img = (uint8_t *) malloc(height * width)

				// agora recebe
			i = hf_recvprobe();
			if (i >= 0) {

				val = hf_recvack(&cpu, &port, buf, &size, 0);
				if (val){
					printf("hf_recvack(): error %d\n", val);
				} else {

					// Do jeito que ta ignora a order
					// Aqui vamos ter que controlar quem enviou o que e
					// por na ordem
					//
					printf("cpu %d size of img %d\n", cpu, sizeof(buf));
					int32_t x = 0;
					while(x< size){
						img[x + (offset * i_chunk) + (offset * (index-1))] = buf[x];
						x++;
					}
					index++;
				}
			}
		}

		printf("CHUNK = %d", i_chunk);
		//                if(index == CPU_N+1) {
		//                    // se entrou aqui eh pq recebeu todas as mensages
		//                    index = 0;
		//                    // printa o resultado
		//                    printf("size of img %d", sizeof(img));
		//                    while(index < sizeof(img)) {
		//                        printf("index: %d, msg: %d\n", index, img[index]);
		//                        index++;
		//                    }
		//
		//                    free(buf);
		//                    free(img);
		//                    // entra em loop pq nao tem mais nada pra fazer
		//                    while(1){}
		//                }
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

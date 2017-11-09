#include <hellfire.h>
#include <noc.h>

void escravo(void)
{
	int8_t buf[20];
	int32_t i;
	uint16_t cpu, port, size;
	int16_t val;
	int32_t index = 0;
	
	if (hf_comm_create(hf_selfid(), 5000, 0))
		panic(0xff);
		
        while(1) {
            i = hf_recvprobe();
            if (i >= 0) {
                    // recebe a mensagem do mestre
                    val = hf_recvack(&cpu, &port, buf, &size, i);
                    if (val)
                            printf("hf_recvack(): error %d\n", val);
                    else{

                        // processa os dados recebidos
                        while(index < sizeof(buf)){
                            buf[index] = 1;
                            printf("I am cpu: %d, index: %d, msg: %d, size: %d\n", hf_cpuid(), index, buf[index], size);
                            index++;
                        }

                        // descobri as 2 da manha que se nao botar isso aqui
                        // ele responde pro mestre antes de o mestre enviar tudo pros escravos
                        // e ai a merda acontece
                        delay_ms(50);
                        // envia os dados alterados pro mestre
                        val = hf_sendack(0, 1000, buf, sizeof(buf), i, 3000);
                        while(1) {}
                    }
            }
        }

	
}


void mestre(void)
{
	int32_t BUF_SIZE = 100;
        int32_t CPU_N = 5;
	int8_t buf[BUF_SIZE];
	int8_t buf_response[BUF_SIZE];

	int32_t i;
	uint16_t cpu, port, size;
	int16_t val;
	int32_t index = 0;


        // inicia o array com alguma coisa 
        while(index < sizeof(buf)){
            buf[index] = index;
            index++;
        }
        index = 1;
	
	if (hf_comm_create(hf_selfid(), 1000, 0))
		panic(0xff);

        // envia um pedaco do array pra cada escravo
        int32_t offset = BUF_SIZE/CPU_N;
        while(index <= CPU_N) {
            // aqui tem que usar aquele calculo pra enviar a matris como array
            val = hf_sendack(index, 5000, &buf[(index-1) * offset], sizeof(buf)/CPU_N, hf_cpuid(), 500);

            if (val)
                    printf("hf_sendack(): error %d\n", val);

            index++;
        }

        index = 1;
	
        // recebe a resposta
	while (1){

		i = hf_recvprobe();
		if (i >= 0) {
			val = hf_recvack(&cpu, &port, buf, &size, 0);
			if (val)
				printf("hf_recvack(): error %d\n", val);
			else {
                            // Do jeito que ta ignora a order
                            // Aqui vamos ter que controlar quem enviou o que e 
                            // por na ordem
                            //
                            int32_t x = 0;
                            while(x< size){
                                buf_response[x + (offset * (index-1))] = buf[x];
                                x++;
                            }
                            index++;
                        }
		} 
                if(index == CPU_N+1) {
                    // se entrou aqui eh pq recebeu todas as mensages
                    index = 0;
                    // printa o resultado
                    while(index < sizeof(buf_response)) {
                        printf("index: %d, msg: %d\n", index, buf_response[index]);
                        index++;
                    }
                    // entra em loop pq nao tem mais nada pra fazer
                    while(1){}
                }
	}
}

void app_main(void)
{
	if (hf_cpuid() == 0){
		hf_spawn(mestre, 0, 0, 0, "mestre", 4096);
	}else{
		hf_spawn(escravo, 0, 0, 0, "escravo", 4096);
	}
}

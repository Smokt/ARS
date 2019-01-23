// Practica tema 5, Martinez Andres Alejandro


#include <sys/types.h>
#include <sys/socket.h>
#include <errno.h>  /* Para poder usar errno y perror() */
#include <stdio.h>  /* Para usar perror() */
#include <stdlib.h> /* Para user EXIT_FAILURE */
#include <netinet/ip.h> /* Para poder definir sockaddr_in */
#include <netinet/in.h> /* Para poder usar in_addr */
#include <string.h>	/* Para usar strcmp() */
#include <netdb.h>	/* Para poder usar getservbyname() */

#define BUFFERSIZE 512

int main(int argc, char **argv)
{
	int err, socketfd;
	int  puerto;
	struct servent *qotd_name;
	char buffQuote[BUFFERSIZE];


	/* Control de entrada */
	if(argc !=  1 && argc != 3)
	{
		printf("USO: %s [-p puerto] \n", argv[0]);
		exit(1);
	}
	if(argc == 3)
	{
		err = strcmp("-p", argv[1]);
		if (err != 0)
		{
			printf("USO: %s [-p puerto] \n", argv[0]);
			exit(1);
		}
	
		/*Convertimos el puerto de ascii a int
		 * y de host byte order a network byte
		 * order
		 */
		puerto = htons(atoi(argv[2]));

	} else {
		/* Si no nos pasan un puerto por argumento
		 * se lo tenemos que pedir al sistema.
		 */	
		qotd_name = getservbyname("qotd","udp");
		if (qotd_name == NULL)
		{
			/* No se ha encontrado el servicio para el puerto dado */
			perror("getservbyname()");
			exit(EXIT_FAILURE);
		} else {
			puerto = qotd_name->s_port;
		}
	}

	
	/* Generamos un descriptor socket */
	socketfd = socket(AF_INET, SOCK_DGRAM, 0);
	if(socketfd < -1)
       	{
		/* Ha ocurrido un problema al generar el descriptor socket
		 * Imprimimos mensaje de error y paramos
		 */
		perror("socket()");
		exit(EXIT_FAILURE);
	}	

	/* Inicializamos la estructura sockaddr_in del servidor */
	struct sockaddr_in servidor;
	servidor.sin_family = AF_INET;
	servidor.sin_port = puerto;
	servidor.sin_addr.s_addr = INADDR_ANY;

	/* Realizamos el bind del socket */
	err = bind(socketfd, (struct sockaddr *)&servidor, sizeof(servidor));
	if (err < 0)
	{
		/* Ha habido un problema enlazando el socket
		 * Imprimimos el error y paramos
		 */
		perror("bind()");
		exit(EXIT_FAILURE);
	}

	/* Bucle con la logica del servidor */
	while(1)
	{

		/* Recivimos la llamada de un cliente */
		struct sockaddr_in cliente;
		socklen_t length = sizeof(struct sockaddr_in);
		err = recvfrom(socketfd, buffQuote, BUFFERSIZE, 0,
				(struct sockaddr *)&cliente, &length);
		if (err < 0)
		{
			/* Ha habido un problema recibiendo la peticion
			 * Imprimimos el error y paramos
			 */
			perror("recvfrom()");
			exit(EXIT_FAILURE);
		}

		/* Cabecera del quote */
		strcpy(buffQuote, "Quote Of The Day from vm2525:\n");
		

		/* Ejecutamos fortune y creamos un fichero con el aforismo que se envia */
		system("/usr/games/fortune -s > /tmp/tt.txt");
		FILE *fich = fopen("/tmp/tt.txt","r");
		int nc = strlen(buffQuote);
		do{
			buffQuote[nc] = getc(fich);
			nc++;
	
		} while( nc < BUFFERSIZE && (feof(fich)==0) );
		fclose(fich);	
		
		/* Introducimos un final de cadena al final */
		buffQuote[nc-1] = '\0';	

		/* Enviamos el quote al cliente */
		err = sendto(socketfd, buffQuote, BUFFERSIZE, 0,
				(struct sockaddr *)&cliente, sizeof(cliente));
		if (err < 0)
		{
			/* Ha habido un problema enviand el quote
			 * Imprimimos el error y paramos
			 */
			perror("sendto()");
			exit(EXIT_FAILURE);
		}

	}

	return 0;
}


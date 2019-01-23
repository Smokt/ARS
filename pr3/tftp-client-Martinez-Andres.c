// Practica tema 7, Martinez Andres Alejandro


#include <sys/types.h>
#include <sys/socket.h>
#include <string.h>
#include <errno.h>  /* Para poder usar errno y perror() */
#include <stdio.h>  /* Para usar perror() */
#include <stdlib.h> /* Para user EXIT_FAILURE */
#include <netinet/ip.h> /* Para poder definir sockaddr_in */
#include <netinet/in.h> /* Para poder usar in_addr */
#include <arpa/inet.h>	/* Para poder usar inet_aton() */
#include <string.h>	/* Para usar strcmp() */
#include <netdb.h>	/* Para poder usar getservbyname() */
#include <unistd.h>	/* Para poder usar close() */

#define BUFFERSIZE 516

int main(int argc, char **argv)
{
	int err, socketfd;
	int  puerto = 0;
	struct sockaddr_in servidor;
	struct in_addr in_addr_nbo;
	struct servent *tftp_name;
	char buf[BUFFERSIZE];
	int verbose = 0;
	char ack[4];
	/* OP code del ack */
	ack[0]=0;
	ack[1]=4;
	


	/* Control de entrada */
	if(argc !=  4 && argc != 5)
	{
		printf("USO: %s <IP Servidor> {-r|-w} archivo [-v] \n", argv[0]);
		exit(1);
	}
	if(argc == 5)
	{
		if(strcmp(argv[4],"-v")== 0)
		{
			/*Verbose activado*/
			verbose = 1;
		}
	}

	/* Si no nos pasan un puerto por argumento
	 * se lo tenemos que pedir al sistema.
	 */	
	tftp_name = getservbyname("tftp","udp");
	if (tftp_name == NULL)
	{
		/* No se ha encontrado el servicio para el puerto dado */
		perror("getservbyname()");
		exit(EXIT_FAILURE);
	} else {
		puerto = tftp_name->s_port;
	}


	/* Parsing de IP y comporbacion */
	err = inet_aton(argv[1],&in_addr_nbo);
	if (err == 0)
	{
		/* Ha habido un error al combertir la IP
		 * a network byte order
		 * Imprimimos el error y paramos
		 */
		perror("inet_aton()");
		exit(EXIT_FAILURE);
	}

	/* Inicializamos los valores de la estructura scoketaddr_in */
	servidor.sin_family = AF_INET;	/* Dominio de Internet */
	servidor.sin_port = puerto; 		/* Puerto del servidor */
	servidor.sin_addr.s_addr = in_addr_nbo.s_addr; /* Direccion del Servidor */


	/* Creacion de la cabecera socket */
	socketfd = socket(AF_INET,SOCK_DGRAM, 0);
	if (socketfd < 0)
	{
		/* Ha habido un error al generar el descriptor
		 * Imprimimos el codigo del error y paramos
		 */
		perror("socket()");
		exit(EXIT_FAILURE);
	}


	/* Lectura de un fichero */

	if (strcmp(argv[2], "-r") == 0)
	{

		/* Preparamos la Read Request */
		char rrq[109] = "RQ"; 	/* Usamos potencias de 2 */
		strcat(rrq,argv[3]); 	/* Concatenamos el nombre del fichero */
		rrq[2+strlen(argv[3])]=100;
		strcat(rrq,"octet");	/* Concatenamos el campo modo, usaremos octet */
		rrq[2+strlen(argv[3])]=0;
		rrq[0]=0;
		rrq[1]=1;
		

		/* Enviamos la peticion de lectura */	
		err = sendto(socketfd, rrq, 109, 0,(struct sockaddr *)&servidor, sizeof(servidor));
		if (err < 0)
		{
			/* Ha habido un error al enviar los datos.
			* Imprimios el error y paramos
			*/
			perror("sendto()");
			exit(EXIT_FAILURE);
		}
		/* Comprobamos la opcion verbose */
		else if (verbose)
		{
			printf("Read Request enviada al servidor %s para el fichero: %s \n", argv[1], argv[3]);
		}

		/* Creamos el fichero donde vamos a escribir los datos que nos devuleva el servidor */
		FILE *myfile = fopen(argv[3],"w");

		/* Hacemos la recepcion de los bloques de datos que nos devuelve el servidor */
		int block_received;
		int block_expected = 1; /* El primer bloque enviado por el servidor es numerado como 1 */

		/* Recibimos la respuesta del servidor */
		int unsigned length = sizeof(servidor);
		int size;
		do {
			size = recvfrom(socketfd, buf, BUFFERSIZE, 0, (struct sockaddr *)&servidor, &length);

			if (size < 0)
			{
				/* Ha habid un error al recibir los datos.
				* Imprimimos el error y paramos
				*/
				perror("recvfrom()");
				exit(EXIT_FAILURE);
			}

			/* Comprobamos el bloque recibido */
			block_received = (unsigned char)buf[2]*256 + (unsigned char) buf[3];
			if (block_received != block_expected)
			{
				/* Se ha recibido un bloque fuera de orden */
				printf("Se ha recibido el bloque \033[0;31m%d\033[0m fuera de orden.\n", block_received);
				exit(EXIT_FAILURE);
			} else {
				/* El bloque recibido sigue el orden, esperamos el siguiente */
				block_expected++;

				/* Comprobamos si se ha recibido un error */
				if (buf[1] == 5)
				{
					printf("Error recibido \n >> errcode: \033[0;31m%i\033[0m \n >> errstring: %i \n", buf[2], buf[3]);
					/* Uso %i porque al contrario que %d, %i contempla octal y hexadecimal */
					exit(EXIT_FAILURE);
				}

				/* Comprobamos la opcino verbose */
				if (verbose)
				{
					printf("Se ha recibido el bloque: \033[0;32m%d\033[0m \n", block_received);
				}

			}

			/* Volcamos los datos recibidos al fichero */
			fwrite(buf+4, 1, size-4, myfile); /* En err tenemos la cantidad de bytes recibidos */

			/* Preparamos y enviamos el ack */
			ack[2]=buf[2];
			ack[3]=buf[3];

			err = sendto(socketfd, ack, 4, 0, (struct sockaddr *)&servidor, sizeof(servidor));
			if (err < 0)
			{
				/* Ha habid un error al enviar el ack.
				* Imprimimos el error y paramos
				*/
				perror("sendto()");
				exit(EXIT_FAILURE);
			}
			else if (verbose)
			{
				printf("ACK enviado para el bloque: \033[0;32m%d\033[0m \n",block_received);
			}

		} while (size == 516);

		if (verbose)
		{
			printf("El bloque: \033[0;32m%d\033[0m es el ultimo\n", block_received);
		}

		/* Cerramos el fichero */
		fclose(myfile);
	}

	/* Escritura de un fichero */

	if(strcmp(argv[2], "-w") == 0)
	{
		/* Preparamos la Write Request */
		char wrq[109] = "WQ";
		strcat(wrq, argv[3]);	/* Concatenamos el nombre del ficheros */
		wrq[2+strlen(argv[3])] = 100;
		strcat(wrq,"octet");	/* Concatenamos el campo modo, usaremos octet */
		wrq[2+strlen(argv[3])] = 0;
		wrq[0] = 0;
		wrq[1] = 2;

		/* Enviamos la peticion de escritura */	
		err = sendto(socketfd, wrq, 109, 0, (struct sockaddr *)&servidor, sizeof(struct sockaddr));
		if (err < 0)
		{
			/* Ha habido un error al enviar los datos.
			* Imprimios el error y paramos
			*/
			perror("sendto()");
			exit(EXIT_FAILURE);
		}
		else if (verbose)
		{
			printf("Se ha enviado la peticion de escritura al servidor: %s\n", argv[1]);
		}

		FILE *myfile = fopen(argv[3], "r");
		int block_received;
		int unsigned length = sizeof(servidor);
		short index = 0;

		while(!feof(myfile))
		{
			/* Los datos tiene que tener un desplazamiento de 4 para
			 * respetar como se construyen los paquetes de datos.
			 */
			buf[index%512+4] = fgetc(myfile);
			index++;

			/* Se ha completado un paquete de 512, o se ha terminado el fichero */
			if ((index%512 == 0) | feof(myfile))
			{
				err = recvfrom(socketfd, ack, 4, 0, (struct sockaddr *)&servidor, &length);
				if (err < 0)
				{
					/* Ha habido un error al recibir los datos.
					*  Imprimimos el error y paramos
					*/
					perror("recvfrom()");
					exit(EXIT_FAILURE);
				}
				block_received = (unsigned char)ack[2]*256+(unsigned char)ack[3] + 1;
				if (ack[1] == 5)
				{
					/* Se ha recivido un paquete de error.
					 * Imprimimos el error y paramos.
					 */
					 printf("Error recibido \n >> errcode: \033[0;31m%i\033[0m \n >> errstring: \033[0;31m%i\033[0m \n", ack[2], ack[3]);
					/* Uso %i porque al contrario que %d, %i contempla octal y hexadecimal */
					exit(EXIT_FAILURE);
				}
				else if (verbose)
				{
					printf("Recibido el ACK de bloque: \033[0;32m%d\033[0m\n", block_received - 1);
				}
				
				/* Calculamos el tamaño de los bloques ha enviar */
				int size;
				/* Tengo en cuenta que el campo de datos es como maximo de tamaño 512 */
				if (index % 512 == 0)
				{
					size = BUFFERSIZE;
				} else {
					size = index%512 + 3;
				}

				/* Preparamos cabeceras del paquete y hacemos el envio del bloque de datos */
				buf[0] = 0;
				buf[1] = 3;
				buf[2] = block_received / 256;
				buf[3] = block_received % 256;

				err = sendto(socketfd, buf, size, 0, (struct sockaddr *)&servidor, sizeof(servidor));
				if (err < 0)
				{
					/* Ha habido un error al enviar los datos.
					* Imprimios el error y paramos
					*/
					perror("sendto()");
					exit(EXIT_FAILURE);
				}
				else if (verbose)
				{
					printf("Se ha enviado el bloque: \033[0;32m%d\033[0m\n", block_received);	
				}

			}			

		}

		if (verbose)
		{
			printf("El bloque: \033[0;32m%d\033[0m es el ultimo\n", block_received);
		}

		/* Cerramos el fichero */
		fclose(myfile);
		
	}

	/* Cerramos el puerto */
	close(socketfd);
	exit(0);

}

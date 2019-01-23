// Practica tema 5, Martinez Andres Alejandro


#include <sys/types.h>
#include <sys/socket.h>
#include <errno.h>  /* Para poder usar errno y perror() */
#include <stdio.h>  /* Para usar perror() */
#include <stdlib.h> /* Para user EXIT_FAILURE */
#include <netinet/ip.h> /* Para poder definir sockaddr_in */
#include <netinet/in.h> /* Para poder usar in_addr */
#include <arpa/inet.h>	/* Para poder usar inet_aton() */
#include <string.h>	/* Para usar strcmp() */
#include <netdb.h>	/* Para poder usar getservbyname() */
#include <unistd.h>	/* Para poder usar close() */

#define BUFFERSIZE 512

int main(int argc, char **argv)
{
	int err, socketfd;
	int  puerto = 0;
	struct sockaddr_in servidor, cliente;
	struct in_addr in_addr_nbo;
	struct servent *qotd_name;
	char buf[BUFFERSIZE];


	/* Control de entrada */
	if(argc !=  2 && argc != 4)
	{
		printf("USO: %s <IP Servidor> [-p puerto] \n", argv[0]);
		exit(1);
	}
	if(argc == 4)
	{
		err = strcmp("-p", argv[2]);
		if (err != 0)
		{
			printf("USO: %s <IP Servidor> [-p puerto] \n", argv[0]);
			exit(1);
		}
	
		/*Convertimos el puerto de ascii a int
		 * y de host byte order a network byte
		 * order
		 */
		puerto = htons(atoi(argv[3]));

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
			//printf("Se ha pedido el puerto del servicio QOTD al sistema, todo correcto. \n");
		}

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

	cliente.sin_family = AF_INET;	/* Dominio de Internet */
	cliente.sin_port = 0;		/* Al ser un cliente, puerto = 0 */
	cliente.sin_addr.s_addr = INADDR_ANY; /* Dirección cualquiera */

	socketfd = socket(AF_INET,SOCK_DGRAM, 0);
	if (socketfd < 0)
	{
		/* Ha habido un error al generar el descriptor
		 * Imprimimos el codigo del error y paramos
		 */
		perror("socket()");
		exit(EXIT_FAILURE);
	}

	/* Binding del puerto */
	err = bind(socketfd, (struct sockaddr *) &cliente, sizeof(cliente));
	if (err < 0)
	{
		/* Ha habido un error al hacer el binding
		 * Imprimimos el codigo y paramos
		 */
		perror("bind()");
		exit(EXIT_FAILURE);
	}


	/* Enviamos los datos con sendto() */
	
	/* Llenamos el buffer con los datos
	 * que queremos mandar.
	 */
	strcpy(buf,"Hello");

	err = sendto(socketfd, buf,(strlen(buf)+1), 0,
			(struct sockaddr *)&servidor, sizeof(struct sockaddr));
	if (err < 0)
	{
		/* Ha habido un error al enviar los datos.
		 * Imprimios el error y paramos
		 */
		perror("sendto()");
		exit(EXIT_FAILURE);
	}

	/* Recibimos la respuesta del servidor */
	int unsigned length = sizeof(servidor);
	err = recvfrom(socketfd, buf, BUFFERSIZE, 0,
			(struct sockaddr *)&servidor, &length);
	if (err < 0)
	{
		/* Ha habid un error al recibir los datos.
		 * Imprimimos el error y paramos
		 */
		perror("recvfrom()");
		exit(EXIT_FAILURE);
	}

	/* Imprimimos lo que nos devuelve el servidor */
	
	printf("\n%s\n", buf);

	/* Cerramos el puerto */
	close(socketfd);
	exit(0);
}	

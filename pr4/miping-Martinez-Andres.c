// Practica tema 8, Martinez Andres Alejandro

#include "ip-icmp-ping.h"	/* Fichero con estructuras de datos y bibliotecas necesarias */
#include <stdlib.h>		/* Inclusion de libreria estandar */
#include <stdio.h>		/* Inlcusion de libreria de esntrada salida */
#include <string.h>		/* Inclusion de la libreria string para poder usar strcpy() */

#define GREENCOLOR "\033[0;32m"
#define RESETCOLOR "\033[0m"
#define REDCOLOR "\033[1;31m"


int main(int argc, char **argv)
{
	int err, socketfd, numShorts, verbose = 0;
	unsigned short int *puntero;
	unsigned int acumulador = 0;
	struct sockaddr_in target;
	struct in_addr in_addr_nbo;

	/* Control de entrada */
	if(argc !=  2 && argc != 3)
	{
		printf("USO: %s <IP Servidor> [-v] \n", argv[0]);
		exit(1);
	}
	if(argc == 3)
	{
		err = strcmp("-v", argv[2]);
		if (err != 0)
		{
			printf("USO: %s <IP Servidor> [-v] \n", argv[0]);
			exit(1);
		}

		verbose = 1;
	}
		
	/* Parsing de IP y comporbacion */
	err = inet_aton(argv[1], &in_addr_nbo);
	if (err < 0)
	{
		/* Ha habido un error al combertir la IP
		 * a network byte order
		 * Imprimimos el error y paramos
		 */
		perror("inet_aton()");
		exit(EXIT_FAILURE);
	}

	/* Inicializamos los valores de la estructura scoketaddr_in */
	target.sin_family = AF_INET;			/* Dominio de Internet */
	target.sin_port = 0; 				/* Puerto del target */
	target.sin_addr.s_addr = in_addr_nbo.s_addr; 	/* Direccion del target */
	
	/* Inicializamos un descriptor socket */
	socketfd = socket(AF_INET, SOCK_RAW, IPPROTO_ICMP);
	if (socketfd < 0)
	{
		/* Ha habido un error al generar el descriptor
		 * Imprimimos el codigo del error y paramos
		 */
		perror("socket()");
		exit(EXIT_FAILURE);
	}

	/* Cocinamos el paquete ICMP */
	ECHORequest request;				/* Instanciamos la estructura */
	request.icmpHeader.Type = 8;			/* Tipo para ping = 8 */
	request.icmpHeader.Code = 0;			/* Codigo para ping = 0 */
	request.icmpHeader.Checksum = 0;		/* Tenemos que inicializar el cksum a 0 */
	request.ID = getpid();				/* PID del proceso */
	request.SeqNumber = 0;				/* Inicializamos la secuancia a 0 */
	strcpy(request.payload, "cadena arbitraria");	/* Llenamos el payload con datos arbitrarios*/	

	/* Una vez preparado el paquete, calculamos su checksum */
	numShorts = sizeof(request) / 2;	/* Cantidad de bytes en half words del datagrama*/	
	puntero = (unsigned short int *)&request;
	/* Vamos recorriendo y acumulando el contenido del datagrama */
	int iter;
	for(iter = 0; iter < numShorts; iter++)
	{
		acumulador = acumulador + (unsigned int) *puntero;
		puntero++;
	}

	/* Calculamos el complemento a 2 del resultado,
	 * dos veces el complemento a 1
	 */
	for(iter = 0; iter < 2; iter++)
	{	
		/* Calcula el complemento a 1 */
		acumulador = (acumulador >> 16) + (acumulador & 0x0000ffff);
	}
	/* Hacemos un not para obtener el complemento a 1 */	
	acumulador = ~acumulador;

	request.icmpHeader.Checksum = (unsigned short int) acumulador;

	/* Enviamos los datos con sendto() */
	err = sendto(socketfd, &request, 72, 0,
			(struct sockaddr *)&target, sizeof(target));
	if (err < 0)
	{
		/* Ha habido un error al enviar los datos.
		 * Imprimios el error y paramos
		 */
		perror("sendto()");
		exit(EXIT_FAILURE);
	}

	/* Imprimimos la informacion del datagrama */
	if (verbose)
	{
		printf("-> Generando cabecera ICMP.\n");
		printf("-> Type: %u\n", request.icmpHeader.Type);
		printf("-> Code: %u\n", request.icmpHeader.Code);
		printf("-> Identifier (pid): %u\n", request.ID);
		printf("-> Seq. number: %u\n", request.SeqNumber);
		printf("-> Cadena a enviar: %s\n", request.payload);
		printf("-> Checksum: 0x%x\n", request.icmpHeader.Checksum);
		printf("-> Bytes totales del paquete ICMP: %d\n", err);

	}
	
	printf("Paquete ICMP enviado a %s \n", argv[1]);


	/* Recibimos la respuesta del target */
	ECHOResponse response;
	int unsigned length = sizeof(target);
	err = recvfrom(socketfd, &response, 516, 0, (struct sockaddr *)&target, &length);
	if (err < 0)
	{
		/* Ha habid un error al recibir los datos.
		 * Imprimimos el error y paramos
		 */
		perror("recvfrom()");
		exit(EXIT_FAILURE);
	}

	/* Imprimimos la respuesta */
	printf("Respuesta recibida desde %s \n", argv[1]);

	if (verbose)
	{
		printf("-> Bytes totales del paquete de respuesta ICMP: %d\n", err);
		printf("-> Cadena recibida: %s\n", response.payload);
		printf("-> Identifier (pid): %u\n", response.ID);
		printf("-> TTL: %u\n", response.ipHeader.TTL);
		
	}

	/* Tratamos el descriptor de la respuesta */
	printf("Descripcion de la respuesta: ");
	switch (response.icmpHeader.Type)
	{
		case 0:	
			printf(GREENCOLOR "respuesta correcta " RESETCOLOR "(Type: %d, Code: %d) \n", response.icmpHeader.Type, response.icmpHeader.Code);
			break;
		case 3:
			switch (response.icmpHeader.Code)
			{
				case 0:
					printf(REDCOLOR "Destination network unreachable." RESETCOLOR"(Type: %d, Code: %d) \n", response.icmpHeader.Type, response.icmpHeader.Code);
					break;
				case 1:
				        printf(REDCOLOR"Destination host unreachable."RESETCOLOR"(Type: %d, Code: %d )\n", response.icmpHeader.Type, response.icmpHeader.Code);
					break;
				case 2:
					printf(REDCOLOR"Destination protocol unreachable."RESETCOLOR"(Type: %d, Code: %d )\n", response.icmpHeader.Type, response.icmpHeader.Code);
					break;
				case 3:
					printf(REDCOLOR"Destination port unreachable."RESETCOLOR"(Type: %d, Code: %d )\n", response.icmpHeader.Type, response.icmpHeader.Code);
					break;
				case 4:
					printf(REDCOLOR"Fragmentation required, and DF flag set."RESETCOLOR"(Type: %d, Code: %d )\n", response.icmpHeader.Type, response.icmpHeader.Code);
					break;
				case 5:
					printf(REDCOLOR"Source route failed."RESETCOLOR"(Type: %d, Code: %d )\n", response.icmpHeader.Type, response.icmpHeader.Code);
					break;
				case 6:
					printf(REDCOLOR"Destination network unknown."RESETCOLOR"(Type: %d, Code: %d \n)", response.icmpHeader.Type, response.icmpHeader.Code);
					break;
				case 7:
					printf(REDCOLOR"Destination host unknown."RESETCOLOR"(Type: %d, Code: %d \n)", response.icmpHeader.Type, response.icmpHeader.Code);
					break;
				case 8:
					printf(REDCOLOR"Source host isolated."RESETCOLOR"(Type: %d, Code: %d )\n", response.icmpHeader.Type, response.icmpHeader.Code);
					break;
				case 9:
					printf(REDCOLOR"Network administratively prohibited."RESETCOLOR"(Type: %d, Code: %d )\n", response.icmpHeader.Type, response.icmpHeader.Code);
					break;
				case 10:
					printf(REDCOLOR"Host administratively prohibited."RESETCOLOR"(Type: %d, Code: %d )\n", response.icmpHeader.Type, response.icmpHeader.Code);
					break;
				case 11:
					printf(REDCOLOR"Network unreachable for ToS."RESETCOLOR"(Type: %d, Code: %d )\n", response.icmpHeader.Type, response.icmpHeader.Code);
					break;
				case 12:
					printf(REDCOLOR"Host unreachable for ToS."RESETCOLOR"(Type: %d, Code: %d )\n", response.icmpHeader.Type, response.icmpHeader.Code);
					break;
				case 13:
					printf(REDCOLOR"Host unreachable for ToS."RESETCOLOR"(Type: %d, Code: %d )\n", response.icmpHeader.Type, response.icmpHeader.Code);
					break;
				case 14:
					printf(REDCOLOR"Host precedence violation."RESETCOLOR"(Type: %d, Code: %d )\n", response.icmpHeader.Type, response.icmpHeader.Code);
					break;
				case 15:
					printf(REDCOLOR"Precedence cutoff in effect."RESETCOLOR"(Type: %d, Code: %d )\n", response.icmpHeader.Type, response.icmpHeader.Code);
					break;		    
			}
			break;

		case 5:
			switch (response.icmpHeader.Code)
			{
				case 0:
					printf(REDCOLOR"Redirect Datagram for the Network."RESETCOLOR"(Type: %d, Code: %d )\n", response.icmpHeader.Type, response.icmpHeader.Code);
					break;
				case 1:
					printf(REDCOLOR"Redirect Datagram for the Host."RESETCOLOR"(Type: %d, Code: %d )\n", response.icmpHeader.Type, response.icmpHeader.Code);
					break;
				case 2:
					printf(REDCOLOR"Redirect Datagram for TOS & Network."RESETCOLOR"(Type: %d, Code: %d )\n", response.icmpHeader.Type, response.icmpHeader.Code);
					break;
				case 3:
					printf(REDCOLOR"Redirect Datagram for TOS & Host."RESETCOLOR"(Type: %d, Code: %d )\n", response.icmpHeader.Type, response.icmpHeader.Code);
					break;
			}
			break;

		case 11:
			switch (response.icmpHeader.Code)
			{
				case 0:
					printf(REDCOLOR"TTL expired in transit."RESETCOLOR"(Type: %d, Code: %d )\n", response.icmpHeader.Type, response.icmpHeader.Code);
					break;
				case 1:
					printf(REDCOLOR"Fragment reassembly time exceeded."RESETCOLOR"(Type: %d, Code: %d )\n", response.icmpHeader.Type, response.icmpHeader.Code);
					break;
			}
			break;

		case 12:
			switch (response.icmpHeader.Code)
			{
				case 0:
					printf(REDCOLOR"Pointer indicates de error."RESETCOLOR"(Type: %d, Code: %d )\n", response.icmpHeader.Type, response.icmpHeader.Code);
					break;
				case 1:
					printf(REDCOLOR"Missing a required option."RESETCOLOR"(Type: %d, Code: %d )\n", response.icmpHeader.Type, response.icmpHeader.Code);
					break;
				case 2:
					printf(REDCOLOR"Bad length."RESETCOLOR"(Type: %d, Code: %d )\n", response.icmpHeader.Type, response.icmpHeader.Code);
					break;
			}
			break;

	}

	/* Cerramos el puerto */
	close(socketfd);
	exit(0);
}	

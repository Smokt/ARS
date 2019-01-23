// Practica tema 6, Martinez Andres Alejandro


#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>  /* Para poder usar fork() */
#include <errno.h>  /* Para poder usar errno y perror() */
#include <stdio.h>  /* Para usar perror() */
#include <stdlib.h> /* Para user EXIT_FAILURE */
#include <netinet/ip.h> /* Para poder definir sockaddr_in */
#include <netinet/in.h> /* Para poder usar in_addr */
#include <string.h>	/* Para usar strcmp() */
#include <netdb.h>	/* Para poder usar getservbyname() */
#include <arpa/inet.h>	/* Para poder usar inet_ntoa() */
#include <signal.h>	/* Para poder usar signal() */

#define BUFFERSIZE 512
#define ANSI_COLOR_ROJO "\x1b[31m"
#define ANSI_COLOR_VERDE "\x1b[32m"
#define ANSI_COLOR_RESET "\x1b[0m"

/* Variables globales */
int socketfd, accsocket;

//void signal_handler(int *signal);

void signal_handler(int signal)
{
	printf("\n" ANSI_COLOR_ROJO ">>Parada ordenada, cerrando sockets..." ANSI_COLOR_RESET "\n");
	
	int err;	
	/* Realizamos un shutdown para el socket socketfd*/
	err = shutdown(socketfd, SHUT_RDWR);
	if (err < 0)
	{
		/* Ha habido un problema con shutdown
		 * Imprimimos el error y paramos
		 */
		perror("shutdown()");
		exit(EXIT_FAILURE);
	}
	/* Lo suyo seria comprobar si el cliente aun
	 * tiene que mandar datos, mirando si la longitud
	 * en un recv es > 0. Pero en este caso no es
	 * relevante
	 */

	/* Cerramos el socket socketfd */
	err = close(socketfd);
	if (err < 0)
	{
		/* Ha habido un problema con close
		 * Imprimimos el error y paramos
		 */
		perror("close()");
		exit(EXIT_FAILURE);
	}

	exit(0);
}

int main(int argc, char **argv)
{
	int err;
	pid_t child;
	int  puerto;
	struct servent *qotd_name;

	/* Asociamos SIGINT con la funcion signal_handler() */
	if (signal(SIGINT, signal_handler) == SIG_ERR)
	{
		/* Ha habido un problema con signal
		 * Imprimimos el error y paramos
		 */
		perror("signal()");
		exit(EXIT_FAILURE);
	}

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
		qotd_name = getservbyname("qotd","tcp");
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
	socketfd = socket(AF_INET, SOCK_STREAM, 0);
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

	/* Apertura pasiva del socket, preparado para recibir conexiones */
	err = listen(socketfd, 10);
	if (err < 0)
	{
		/* Ha habido un problema al hacer listen()
		 * Imprimimos el error y paramos
		 */
		perror("listen()");
		exit(EXIT_FAILURE);
	}

	printf(ANSI_COLOR_VERDE">>Servidor arrancado correctamente, esperando preticiones..."ANSI_COLOR_RESET"\n");

	/* Bucle con la logica del servidor */
	while(1)
	{	

		/* Esperamos la conexion de un cliente */
		struct sockaddr_in cliente;
		socklen_t length = sizeof(struct sockaddr_in);
		accsocket = accept(socketfd, (struct sockaddr *) &cliente, &length);
		if (accsocket < 0)
		{	
			/* Ha habido un problema con accept
			 * Imprimomos el error y paramos
		 	*/
			perror("accept()");
			exit(EXIT_FAILURE);
		}
		printf(">>Cliente conectado: %s\n", inet_ntoa(cliente.sin_addr));

		/* Creacion de un hijo que atienda la peticion */
		child = fork();
		if (child == 0) /* Proceso hijo */
		{
		
			char buffQuote[BUFFERSIZE];
	
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
			err = send(accsocket, buffQuote, BUFFERSIZE, 0);

			if (err < 0)
			{
				/* Ha habido un problema enviand el quote
				 * Imprimimos el error y paramos
				 */
				perror("sendto()");
				exit(EXIT_FAILURE);
			}
				
			/* Realizamos un shutdown para el socket accsocket
			 * Indicamos que no tenemos mas datos que mandar.
			 */
			//err = shutdown(accsocket, SHUT_RDWR);
			//if (err < 0)
			//{
				/* Ha habido un problema con shutdown
				 * Imprimimos el error y paramos
				 */
			//	perror("shutdown()accsocket");
			//	exit(EXIT_FAILURE);
			//}
			/* Lo suyo seria comprobar si el cliente aun
			 * tiene que mandar datos, mirando si la longitud
			 * en un recv es > 0. Pero en este caso no es
			 * relevante
			 */
		
			/* Cerramos el socket accsocket */
			err = close(accsocket);
			if (err < 0)
			{
				/* Ha habido un problema con close
				 * Imprimimos el error y paramos
				 */
				perror("close()");
				exit(EXIT_FAILURE);
			}

			exit(0);

		}
		else /* Proceso padre */
		{
			/* Imprimo el PID del hijo para tener un poco de control */
			printf("└--Proceso hijo con PID: %d atendiendo la llamada...\n", child);
			
		}
	}

	return 0;
}


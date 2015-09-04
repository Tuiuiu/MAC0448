/* Por Prof. Daniel Batista <batista@ime.usp.br>
 * Em 12/08/2013
 * 
 * Um c�digo simples (n�o � o c�digo ideal, mas � o suficiente para o
 * EP) de um servidor de eco a ser usado como base para o EP1. Ele
 * recebe uma linha de um cliente e devolve a mesma linha. Teste ele
 * assim depois de compilar:
 * 
 * ./servidor 8000
 * 
 * Com este comando o servidor ficar� escutando por conex�es na porta
 * 8000 TCP (Se voc� quiser fazer o servidor escutar em uma porta
 * menor que 1024 voc� precisa ser root).
 *
 * Depois conecte no servidor via telnet. Rode em outro terminal:
 * 
 * telnet 127.0.0.1 8000
 * 
 * Escreva sequ�ncias de caracteres seguidas de ENTER. Voc� ver� que
 * o telnet exibe a mesma linha em seguida. Esta repeti��o da linha �
 * enviada pelo servidor. O servidor tamb�m exibe no terminal onde ele
 * estiver rodando as linhas enviadas pelos clientes.
 * 
 * Obs.: Voc� pode conectar no servidor remotamente tamb�m. Basta saber o
 * endere�o IP remoto da m�quina onde o servidor est� rodando e n�o
 * pode haver nenhum firewall no meio do caminho bloqueando conex�es na
 * porta escolhida.
 */

#define _GNU_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <netdb.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <time.h>
#include <unistd.h>
#include <pthread.h>

#include "users_list.h"
#include "utils.h"

#define LISTENQ 1

#define MAXDATASIZE 100
#define MAXLINE 4096
#define MAX_CONNECTIONS 100
#define MAX_MSG_SIZE 500
#define MAX_TIME_STRING_SIZE 20
#define MAX_CHANNEL_NAME_SIZE 20


char* channel_names[NUMBER_OF_CHANNELS] = {"Canal1", "Canal2"};

User *users_list;

int number_of_connections = 0;


void* client_connection(void* arg);
void write_to_all (char* message);


int main (int argc, char **argv) {
	/* Os sockets. Um que ser� o socket que vai escutar pelas conex�es
	 * e o outro que vai ser o socket espec�fico de cada conex�o */
	int listenfd, connfd;
	/* Informa��es sobre o socket (endere�o e porta) ficam nesta struct */
	struct sockaddr_in servaddr;
	/* Retorno da fun��o fork para saber quem � o processo filho e quem
	 * � o processo pai */
	/*pid_t childpid;*/	

	pthread_t threads[MAX_CONNECTIONS];

	char string[100];
	char string_aux[MAX_NICK_SIZE];

	users_list = list_init();
	
	if (argc != 2) {
		fprintf(stderr,"Uso: %s <Porta>\n",argv[0]);
		fprintf(stderr,"Vai rodar um servidor de echo na porta <Porta> TCP\n");
		exit(1);
	}

	/* Cria��o de um socket. Eh como se fosse um descritor de arquivo. Eh
	 * possivel fazer operacoes como read, write e close. Neste
	 * caso o socket criado eh um socket IPv4 (por causa do AF_INET),
	 * que vai usar TCP (por causa do SOCK_STREAM), j� que o IRC
	 * funciona sobre TCP, e ser� usado para uma aplica��o convencional sobre
	 * a Internet (por causa do n�mero 0) */
	if ((listenfd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
		perror("socket :(\n");
		exit(2);
	}

	/* Agora � necess�rio informar os endere�os associados a este
	 * socket. � necess�rio informar o endere�o / interface e a porta,
	 * pois mais adiante o socket ficar� esperando conex�es nesta porta
	 * e neste(s) endere�os. Para isso � necess�rio preencher a struct
	 * servaddr. � necess�rio colocar l� o tipo de socket (No nosso
	 * caso AF_INET porque � IPv4), em qual endere�o / interface ser�o
	 * esperadas conex�es (Neste caso em qualquer uma -- INADDR_ANY) e
	 * qual a porta. Neste caso ser� a porta que foi passada como
	 * argumento no shell (atoi(argv[1]))
	 */
	bzero(&servaddr, sizeof(servaddr));
	servaddr.sin_family		= AF_INET;
	servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
	servaddr.sin_port		  = htons(atoi(argv[1]));
	if (bind(listenfd, (struct sockaddr *)&servaddr, sizeof(servaddr)) == -1) {
		perror("bind :(\n");
		exit(3);
	}

	/* Como este c�digo � o c�digo de um servidor, o socket ser� um
	 * socket passivo. Para isto � necess�rio chamar a fun��o listen
	 * que define que este � um socket de servidor que ficar� esperando
	 * por conex�es nos endere�os definidos na fun��o bind. */
	if (listen(listenfd, LISTENQ) == -1) {
		perror("listen :(\n");
		exit(4);
	}

	printf("[Servidor no ar. Aguardando conexoes na porta %s]\n",argv[1]);
	printf("[Para finalizar, pressione CTRL+c ou rode um kill ou killall]\n");
	
	/* O servidor no final das contas � um loop infinito de espera por
	 * conex�es e processamento de cada uma individualmente */
	for (;;) {
		/* O socket inicial que foi criado � o socket que vai aguardar
		 * pela conex�o na porta especificada. Mas pode ser que existam
		 * diversos clientes conectando no servidor. Por isso deve-se
		 * utilizar a fun��o accept. Esta fun��o vai retirar uma conex�o
		 * da fila de conex�es que foram aceitas no socket listenfd e
		 * vai criar um socket espec�fico para esta conex�o. O descritor
		 * deste novo socket � o retorno da fun��o accept. */
		if ((connfd = accept(listenfd, (struct sockaddr *) NULL, NULL)) == -1 ) {
			perror("accept :(\n");
			exit(5);
		}


		sprintf (string_aux, "An�nimo %d", number_of_connections);

		
		/* Agora o servidor precisa tratar este cliente de forma
		 * separada. Para isto � criado um processo filho usando a
		 * fun��o fork. O processo vai ser uma c�pia deste. Depois da
		 * fun��o fork, os dois processos (pai e filho) estar�o no mesmo
		 * ponto do c�digo, mas cada um ter� um PID diferente. Assim �
		 * poss�vel diferenciar o que cada processo ter� que fazer. O
		 * filho tem que processar a requisi��o do cliente. O pai tem
		 * que voltar no loop para continuar aceitando novas conex�es */
		/* Se o retorno da fun��o fork for zero, � porque est� no
		 * processo filho. */

		if (pthread_create(&threads[number_of_connections], NULL, client_connection,
		    (void*) create(users_list, connfd, 0, number_of_connections, string_aux)))
		{
			printf ("Erro na cria��o da thread %d.\n", number_of_connections);
			exit (EXIT_FAILURE);
		}
		
		sprintf(string, "Conex�o estabelecida\n");
		write(connfd, string, strlen(string)); 	
		number_of_connections++;

		/**** PROCESSO PAI ****/
		/* Se for o pai, a �nica coisa a ser feita � fechar o socket
		 * connfd (ele � o socket do cliente espec�fico que ser� tratado
		 * pelo processo filho) */
		 	/*write(connfd, "teste", strlen("teste"));
		close(connfd);*/
	}
	exit(0);
}


void* client_connection(void* threadarg)
{
	/* user->id � o �ndice da thread */

	/* Armazena linhas recebidas do cliente */
	char	recvline[MAXLINE + 1];
	/* Armazena o tamanho da string lida do cliente */
	ssize_t  n;

	User *user;

	char string[MAX_MSG_SIZE];
	char command[15];

	time_t rawtime;
	struct tm * timeinfo;
	char time_string[MAX_TIME_STRING_SIZE];

	bool wants_to_quit = false;

	user = (User*) threadarg;

	printf("[Uma conexao aberta]\n");
	/* J� que est� no processo filho, n�o precisa mais do socket
	 * listenfd. S� o processo pai precisa deste socket. */
	/*listenfd;*/
	
	/* Agora pode ler do socket e escrever no socket. Isto tem
	 * que ser feito em sincronia com o cliente. N�o faz sentido
	 * ler sem ter o que ler. Ou seja, neste caso est� sendo
	 * considerado que o cliente vai enviar algo para o servidor.
	 * O servidor vai processar o que tiver sido enviado e vai
	 * enviar uma resposta para o cliente (Que precisar� estar
	 * esperando por esta resposta) 
	 */

	
	/* ========================================================= */
	/* TODO: � esta parte do c�digo que ter� que ser modificada
	 * para que este servidor consiga interpretar comandos IRC	*/
	while (!wants_to_quit && (n=read(user->connfd, recvline, MAXLINE)) > 0) {
		recvline[n]=0;
		printf("[Cliente conectado na thread %d enviou] ", user->id);
		if ((fputs(recvline,stdout)) == EOF) {
			perror("fputs :( \n");
			exit(6);
		}

		sprintf(command, " ");
		sscanf(recvline, "%15s", command);

		if (strcmp(command, "NICK") == 0)
			sscanf(recvline, "%*s %s", user->nickname);
		else if (strcmp (command, "MACDATA") == 0)
		{
			time(&rawtime);
			timeinfo = localtime(&rawtime);

			strftime(time_string, MAX_TIME_STRING_SIZE, "%d/%m/%Y\n", timeinfo);
			write(user->connfd, time_string, strlen(time_string));
		}
		else if (strcmp(command, "MACHORA") == 0)
		{
			time(&rawtime);
			timeinfo = localtime(&rawtime);

			strftime(time_string, MAX_TIME_STRING_SIZE, "%X-%Z\n", timeinfo);
			write(user->connfd, time_string, strlen(time_string));
		}
		else if (strcmp (command, "MACTEMPERATURA") == 0)
		{
			if (get_weather(user->connfd) != 0)
				perror("get weather :( \n");
		}
		else if (strcmp(command, "JOIN") == 0)
		{
			/*char channel1[MAX_CHANNEL_NAME_SIZE] = "";
			char channel2[MAX_CHANNEL_NAME_SIZE] = "";*/
			char confirmation_string[MAX_CHANNEL_NAME_SIZE + 20];
			int i;
			char* token;
			char* delimiters = " ,\n\r";

			token = strtok(recvline, delimiters);
			token = strtok(NULL, delimiters); /* ignora o primeiro token pois � JOIN */

			while (token != NULL)
			{
				for (i = 0; i < NUMBER_OF_CHANNELS; i++)
				{
					printf ("strcmp = %d\n", strcmp(token, channel_names[i]));
					if (strcmp(token, channel_names[i]) == 0)
					{
						user->is_in_channel[i] = true;
						sprintf (confirmation_string, "Conectado ao canal %s\n", channel_names[i]);
						write(user->connfd, confirmation_string, strlen(confirmation_string));
					}
				}
				token = strtok(NULL, delimiters);
			}
		}
		else if (strcmp(command, "QUIT") == 0)
		{
			char quit_message[MAX_MSG_SIZE];
			int has_quit_message;
			char message_to_send[MAX_MSG_SIZE + MAX_NICK_SIZE + 30];

			has_quit_message = sscanf(recvline, "%*s %500s", quit_message);

			if (has_quit_message == 1)
			{
				sprintf(message_to_send, "%s saiu e deixou a mensagem %s\n", user->nickname, quit_message);
				write_to_all(message_to_send);
			}
			else
			{
				sprintf(message_to_send, "%s saiu\n", user->nickname);
				write_to_all(message_to_send);
			}

			wants_to_quit = true;
		}
		else
		{
			sprintf(string, "%s enviou: %s", user->nickname, recvline);
			write_to_all(string);
		}
	}

	/* Ap�s ter feito toda a troca de informa��o com o cliente,
	 * pode finalizar o processo filho */
	printf("[Uma conexao fechada]\n");
	close(user->connfd);

 	/*************************************************************/
	/****************  TIRAR DA LISTA DE USU�RIOS  ***************/
	/*************************************************************/
	
	pthread_exit(NULL);
}

void write_to_all (char* message) /* envia message para todos os usu�rios */
{
	User *aux;

	for (aux = users_list->next; aux != NULL; aux=aux->next)
	{	
		/*char test[1000];
		sprintf (test, "Voc� � o usu�rio %d (%s), e: ", aux->id, aux->nickname);
		write(aux->connfd, test, strlen(test));*/
		write(aux->connfd, message, strlen(message));
	}
}
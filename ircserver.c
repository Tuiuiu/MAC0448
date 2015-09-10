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

/* Standard headers */
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>

/* Linux headers */
#include <time.h>
#include <unistd.h>
#include <sys/types.h>

 /* Network headers */
#include <netdb.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>

/* POSIX Thread headers */
#include <pthread.h>

/* Internal headers */
#include "Weather.h"
#include "User_list.h"
#include "Channel_list.h"

/* Macros */
#define LISTENQ 1

#define MAXDATASIZE 100
#define MAXLINE 4096
#define MAX_CONNECTIONS 100
#define MAX_MSG_SIZE 500
#define MAX_TIME_STRING_SIZE 20

/* Shared variables */
User_list user_list;
Channel_list channel_list;

int number_of_connections = 0;

/* Prototypes */
void* client_connection(void* arg);
void write_to_all (char* message);
void write_to_all_in_channel (Channel channel, char* message);
void start_channels (Channel_list allchannels);

/* Main */
int main (int argc, char **argv) {
	/* Os sockets. Um que ser� o socket que vai escutar pelas conex�es
	 * e o outro que vai ser o socket espec�fico de cada conex�o */
	int listenfd, connfd;
	/* Informa��es sobre o socket (endere�o e porta) ficam nesta struct */
	struct sockaddr_in servaddr;
	/* Retorno da fun��o fork para saber quem � o processo filho e quem
	 * � o processo pai */
	/*pid_t childpid;*/ 
	User aux_user;

	pthread_t threads[MAX_CONNECTIONS];

	char string[100];
	char string_aux[1 + MAX_NICK_SIZE];

	user_list = list_init();
	channel_list = chn_list_init();

	start_channels(channel_list);
	 
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
	servaddr.sin_family     = AF_INET;
	servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
	servaddr.sin_port         = htons(atoi(argv[1]));
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


		sprintf (string_aux, "guest%d", number_of_connections);

		  
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


		aux_user = new_user(connfd, number_of_connections, string_aux);
		insert_user(user_list, aux_user);
		if (pthread_create(&threads[number_of_connections], NULL, client_connection,
			(void*) aux_user))
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
	char    recvline[MAXLINE + 1];
	/* Armazena o tamanho da string lida do cliente */
	ssize_t  n;

	User user;

	char string[1 + MAX_MSG_SIZE];
	char command[15];

	time_t rawtime;
	struct tm * timeinfo;
	char time_string[1 + MAX_TIME_STRING_SIZE];

	bool wants_to_quit = false;

	user = (User) threadarg;

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
	 * para que este servidor consiga interpretar comandos IRC  */
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
		{
			char confirmation_string[1 + MAX_NICK_SIZE + MAX_CHANNEL_NAME_SIZE + 30] = " ";
            char old_nick[1 + MAX_NICK_SIZE];

            strcpy (old_nick, user->nickname);
			sscanf(recvline, "%*s %s", user->nickname);
			sprintf (confirmation_string, ":%s NICK :%s\n", old_nick, user->nickname);
			write (user->connfd, confirmation_string, strlen(confirmation_string));


		}

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
			/*char channel1[1 + MAX_CHANNEL_NAME_SIZE] = "";
			char channel2[1 + MAX_CHANNEL_NAME_SIZE] = "";*/
			char* confirmation_string;
			Channel_list aux;
			char* token;
			char* delimiters = " ,\n\r";
			bool channel_exists;

			token = strtok(recvline, delimiters);
			token = strtok(NULL, delimiters); /* ignora o primeiro token pois � JOIN */
			while (token != NULL)
			{
				channel_exists = false;
				if (token[0] == '#' || token[0] == '&') 
				{
					for (aux = channel_list->next; aux != NULL; aux = aux->next)
					{
						/*printf ("token: %s| canal: aux->channel->name: %s|\n", token, aux->channel->name);
						printf ("strcmp = %d\n", strcmp(token, aux->channel->name));*/
						if (strcmp(token, aux->channel->name) == 0)
						{
							channel_exists = true;
							if (!exists_channel(user->channels, aux->channel->name))
							{
                                char* users_string;
                                int users_string_size = (MAX_NICK_SIZE + 2) * number_of_users (aux->channel->users); /* + 2 pelo @ e pelo espa�o */

                                /* inser��o nas listas */
								insert_channel (user->channels, aux->channel);
								insert_user (aux->channel->users, user);

                                confirmation_string = malloc ((1 + MAX_NICK_SIZE + MAX_CHANNEL_NAME_SIZE + 30 + users_string_size) * sizeof (char));

                                /* resposta dizendo que o JOIN deu certo */
                                sprintf (confirmation_string, ":%s JOIN %s\n", user->nickname, aux->channel->name);
                                write_to_all_in_channel (aux->channel, confirmation_string);

                                /* resposta dizendo o t�pico do canal*/
								sprintf (confirmation_string, ":irc.ircserver.net 331 %s %s :No topic is set\n", user->nickname, aux->channel->name);
								write (user->connfd, confirmation_string, strlen(confirmation_string));

                                /* resposta dizendo os usu�rios conectados no canal */
                                users_string = malloc ((1 + users_string_size) * sizeof (char));
                                users_string[0] = '\0';
                                users_list_to_string (aux->channel->users, users_string);

								sprintf (confirmation_string, ":irc.ircserver.net 353 %s @ %s :%s\n", user->nickname, aux->channel->name, users_string);
								write (user->connfd, confirmation_string, strlen(confirmation_string));
                                free (users_string);

                                /* resposta dizendo que acabou a lista de usu�rios conectados no canal */
								sprintf (confirmation_string, ":irc.ircserver.net 366 %s %s :End of /NAMES list.\n", user->nickname, aux->channel->name);
								write (user->connfd, confirmation_string, strlen(confirmation_string));
                                free (confirmation_string);
							}
						}
					}
					if (!channel_exists)
					{
						insert_new_channel(channel_list, token, user);
					}
				}
				else
				{
                    confirmation_string = malloc (1 + 50 + MAX_NICK_SIZE + MAX_CHANNEL_NAME_SIZE);
					sprintf(confirmation_string, ":irc.ircserver.net 403 %s %s :No such channel\n", user->nickname, token);
					write(user->connfd, confirmation_string, strlen(confirmation_string));
                    free (confirmation_string);
				}
				token = strtok(NULL, delimiters);
			}
		}
		else if (strcmp(command, "QUIT") == 0)
		{
			char quit_message[1 + MAX_MSG_SIZE];
			int has_quit_message;
			char message_to_send[1 + MAX_MSG_SIZE + MAX_NICK_SIZE + 30];

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
		else if (strcmp(command, "PRIVMSG") == 0)
		{
			char receiver[1 + MAX_NICK_SIZE];
			char message[1 + MAX_MSG_SIZE] = " ";
			Channel_list aux;
			User_list usraux;
			sscanf(recvline, "%*s %s :%[^\n\r]", receiver, message);
			printf("message: %s\n", message);

			if (receiver[0] == '#' || receiver[0] == '&')
			{
				for (aux = channel_list->next; aux != NULL; aux = aux->next)
				{
					if (strcmp(aux->channel->name, receiver) == 0)
					{
						sprintf(message, "%s\n", message);
						write_to_all_in_channel(aux->channel, message);
						break;
					}
				}
			}
			else 
			{
				for (usraux = user_list->next; usraux != NULL; usraux = usraux->next)
				{
					if (strcmp(usraux->user->nickname, receiver) == 0)
					{
						write(usraux->user->connfd, message, strlen(message));
						break;
					}
				}
			}
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
	User_list aux;

	for (aux = user_list->next; aux != NULL; aux=aux->next)
	{   
		/*char test[1000];
		sprintf (test, "Voc� � o usu�rio %d (%s), e: ", aux->id, aux->nickname);
		write(aux->connfd, test, strlen(test));*/
		write(aux->user->connfd, message, strlen(message));
	}
}

void write_to_all_in_channel (Channel channel, char* message)
{
	User_list aux;

	for (aux = channel->users->next; aux != NULL; aux=aux->next)
	{   
		printf("\n%s\n", aux->user->nickname);
		write(aux->user->connfd, message, strlen(message));
	}	
}

void start_channels (Channel_list allchannels) 
{
	 User_list channel_users1, channel_users2;
	 Channel new_channel1, new_channel2;

	 new_channel1 = malloc(sizeof(*new_channel1));
	 new_channel2 = malloc(sizeof(*new_channel2));

	 strcpy(new_channel1->name, "#Canal1");
	 strcpy(new_channel2->name, "#Canal2");

	 channel_users1 = list_init();
	 channel_users2 = list_init();

	 new_channel1->users = channel_users1;
	 new_channel2->users = channel_users2;

	 insert_channel(allchannels, new_channel1);
	 insert_channel(allchannels, new_channel2);
}

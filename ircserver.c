/* Por Prof. Daniel Batista <batista@ime.usp.br>
 * Em 12/08/2013
 * 
 * Um código simples (não é o código ideal, mas é o suficiente para o
 * EP) de um servidor de eco a ser usado como base para o EP1. Ele
 * recebe uma linha de um cliente e devolve a mesma linha. Teste ele
 * assim depois de compilar:
 * 
 * ./servidor 8000
 * 
 * Com este comando o servidor ficará escutando por conexões na porta
 * 8000 TCP (Se você quiser fazer o servidor escutar em uma porta
 * menor que 1024 você precisa ser root).
 *
 * Depois conecte no servidor via telnet. Rode em outro terminal:
 * 
 * telnet 127.0.0.1 8000
 * 
 * Escreva sequências de caracteres seguidas de ENTER. Você verá que
 * o telnet exibe a mesma linha em seguida. Esta repetição da linha é
 * enviada pelo servidor. O servidor também exibe no terminal onde ele
 * estiver rodando as linhas enviadas pelos clientes.
 * 
 * Obs.: Você pode conectar no servidor remotamente também. Basta saber o
 * endereço IP remoto da máquina onde o servidor está rodando e não
 * pode haver nenhum firewall no meio do caminho bloqueando conexões na
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
	/* Os sockets. Um que será o socket que vai escutar pelas conexões
	 * e o outro que vai ser o socket específico de cada conexão */
	int listenfd, connfd;
	/* Informações sobre o socket (endereço e porta) ficam nesta struct */
	struct sockaddr_in servaddr;
	/* Retorno da função fork para saber quem é o processo filho e quem
	 * é o processo pai */
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

	 /* Criação de um socket. Eh como se fosse um descritor de arquivo. Eh
	  * possivel fazer operacoes como read, write e close. Neste
	  * caso o socket criado eh um socket IPv4 (por causa do AF_INET),
	  * que vai usar TCP (por causa do SOCK_STREAM), já que o IRC
	  * funciona sobre TCP, e será usado para uma aplicação convencional sobre
	  * a Internet (por causa do número 0) */
	if ((listenfd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
		perror("socket :(\n");
		exit(2);
	}

	 /* Agora é necessário informar os endereços associados a este
	  * socket. É necessário informar o endereço / interface e a porta,
	  * pois mais adiante o socket ficará esperando conexões nesta porta
	  * e neste(s) endereços. Para isso é necessário preencher a struct
	  * servaddr. É necessário colocar lá o tipo de socket (No nosso
	  * caso AF_INET porque é IPv4), em qual endereço / interface serão
	  * esperadas conexões (Neste caso em qualquer uma -- INADDR_ANY) e
	  * qual a porta. Neste caso será a porta que foi passada como
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

	 /* Como este código é o código de um servidor, o socket será um
	  * socket passivo. Para isto é necessário chamar a função listen
	  * que define que este é um socket de servidor que ficará esperando
	  * por conexões nos endereços definidos na função bind. */
	if (listen(listenfd, LISTENQ) == -1) {
		perror("listen :(\n");
		exit(4);
	}

	printf("[Servidor no ar. Aguardando conexoes na porta %s]\n",argv[1]);
	printf("[Para finalizar, pressione CTRL+c ou rode um kill ou killall]\n");
	 
	 /* O servidor no final das contas é um loop infinito de espera por
	  * conexões e processamento de cada uma individualmente */
	for (;;) {
		  /* O socket inicial que foi criado é o socket que vai aguardar
			* pela conexão na porta especificada. Mas pode ser que existam
			* diversos clientes conectando no servidor. Por isso deve-se
			* utilizar a função accept. Esta função vai retirar uma conexão
			* da fila de conexões que foram aceitas no socket listenfd e
			* vai criar um socket específico para esta conexão. O descritor
			* deste novo socket é o retorno da função accept. */
		if ((connfd = accept(listenfd, (struct sockaddr *) NULL, NULL)) == -1 ) {
			perror("accept :(\n");
			exit(5);
		}


		sprintf (string_aux, "guest%d", number_of_connections);

		  
		/* Agora o servidor precisa tratar este cliente de forma
		 * separada. Para isto é criado um processo filho usando a
		 * função fork. O processo vai ser uma cópia deste. Depois da
		 * função fork, os dois processos (pai e filho) estarão no mesmo
		 * ponto do código, mas cada um terá um PID diferente. Assim é
		 * possível diferenciar o que cada processo terá que fazer. O
		 * filho tem que processar a requisição do cliente. O pai tem
		 * que voltar no loop para continuar aceitando novas conexões */
		/* Se o retorno da função fork for zero, é porque está no
		 * processo filho. */


		aux_user = new_user(connfd, number_of_connections, string_aux);
		insert_user(user_list, aux_user);
		if (pthread_create(&threads[number_of_connections], NULL, client_connection,
			(void*) aux_user))
		{
			printf ("Erro na criação da thread %d.\n", number_of_connections);
			exit (EXIT_FAILURE);
		}
		  
		sprintf(string, "Conexão estabelecida\n");
		write(connfd, string, strlen(string));  
		number_of_connections++;

		/**** PROCESSO PAI ****/
		/* Se for o pai, a única coisa a ser feita é fechar o socket
		 * connfd (ele é o socket do cliente específico que será tratado
		 * pelo processo filho) */
		/*write(connfd, "teste", strlen("teste"));
		close(connfd);*/
	}
	exit(0);
}

void* client_connection(void* threadarg)
{
	/* user->id é o índice da thread */

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
	/* Já que está no processo filho, não precisa mais do socket
	 * listenfd. Só o processo pai precisa deste socket. */
	/*listenfd;*/
	 
	/* Agora pode ler do socket e escrever no socket. Isto tem
	 * que ser feito em sincronia com o cliente. Não faz sentido
	 * ler sem ter o que ler. Ou seja, neste caso está sendo
	 * considerado que o cliente vai enviar algo para o servidor.
	 * O servidor vai processar o que tiver sido enviado e vai
	 * enviar uma resposta para o cliente (Que precisará estar
	 * esperando por esta resposta) 
	 */

	 
	/* ========================================================= */
	/* TODO: É esta parte do código que terá que ser modificada
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
			token = strtok(NULL, delimiters); /* ignora o primeiro token pois é JOIN */
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
                                int users_string_size = (MAX_NICK_SIZE + 2) * number_of_users (aux->channel->users); /* + 2 pelo @ e pelo espaço */

                                /* inserção nas listas */
								insert_channel (user->channels, aux->channel);
								insert_user (aux->channel->users, user);

                                confirmation_string = malloc ((1 + MAX_NICK_SIZE + MAX_CHANNEL_NAME_SIZE + 30 + users_string_size) * sizeof (char));

                                /* resposta dizendo que o JOIN deu certo */
                                sprintf (confirmation_string, ":%s JOIN %s\n", user->nickname, aux->channel->name);
                                write_to_all_in_channel (aux->channel, confirmation_string);

                                /* resposta dizendo o tópico do canal*/
								sprintf (confirmation_string, ":irc.ircserver.net 331 %s %s :No topic is set\n", user->nickname, aux->channel->name);
								write (user->connfd, confirmation_string, strlen(confirmation_string));

                                /* resposta dizendo os usuários conectados no canal */
                                users_string = malloc ((1 + users_string_size) * sizeof (char));
                                users_string[0] = '\0';
                                users_list_to_string (aux->channel->users, users_string);

								sprintf (confirmation_string, ":irc.ircserver.net 353 %s @ %s :%s\n", user->nickname, aux->channel->name, users_string);
								write (user->connfd, confirmation_string, strlen(confirmation_string));
                                free (users_string);

                                /* resposta dizendo que acabou a lista de usuários conectados no canal */
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

	/* Após ter feito toda a troca de informação com o cliente,
	 * pode finalizar o processo filho */
	printf("[Uma conexao fechada]\n");
	close(user->connfd);

	/*************************************************************/
	/****************  TIRAR DA LISTA DE USUÁRIOS  ***************/
	/*************************************************************/
	 
	pthread_exit(NULL);
}

void write_to_all (char* message) /* envia message para todos os usuários */
{
	User_list aux;

	for (aux = user_list->next; aux != NULL; aux=aux->next)
	{   
		/*char test[1000];
		sprintf (test, "Você é o usuário %d (%s), e: ", aux->id, aux->nickname);
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

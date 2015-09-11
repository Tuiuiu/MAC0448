/* Standard headers */
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

/* Internal headers */
#include "User_list.h"

User_list list_init()
{
	User_list ini;
	ini = malloc(sizeof (*ini));
	ini->user = NULL;
	ini->next = NULL;
	return ini;
}

void print_list (User_list list)
{
	User_list aux;
	for (aux = list->next; aux != NULL; aux = aux->next)
		printf ("%s ", aux->user->nickname);
}

User_list insert_user(User_list list, User user)
{
	User_list new_user;
	/*printf("Antes de inserir:");
	print_list (list);
	printf ("\n");*/
	new_user = malloc(sizeof (*new_user));
	new_user->user = user;
	new_user->next = list->next;
	list->next = new_user;

	/*printf("Depois de inserir:");
	print_list (list);
	printf ("\n");*/

	return new_user;
}

int number_of_users (User_list list)
{
	User_list aux;
	int number = 0;
	
	for (aux = list->next; aux != NULL; aux = aux->next)
		number++;
	
	return number;
}

bool exists_nickname(User_list list, char *nickname)
{
    User_list aux;
    for (aux = list->next; aux != NULL; aux = aux->next)
    {
        if(strcmp(aux->user->nickname, nickname) == 0)
            return true;
    }
    return false;
}

void users_list_to_string (User_list list, char* string) /* string (que precisa já estar alocada) terá os nicks dos usuários com um @ antes e separados por espaços */
{
	User_list aux;
	char nickname_with_space_and_at[1 + MAX_NICK_SIZE + 1];

	for (aux = list->next; aux != NULL; aux = aux->next)
	{
		sprintf (nickname_with_space_and_at, "@%s ", aux->user->nickname);
		strcat (string, nickname_with_space_and_at);
	}
}

void remove_user(User_list list, char user_nick[MAX_NICK_SIZE])
{
	User_list aux1, aux2;
	aux1 = list; aux2 = list->next;
	while (aux2 != NULL && aux2->user->nickname != user_nick) 
	{
		aux1 = aux2;
		aux2 = aux1->next;
	}
	if (aux2 != NULL) 
	{
		aux1->next = aux2->next;
		free(aux2);
	}
}

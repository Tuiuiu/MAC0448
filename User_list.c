/* Standard headers */
#include <stdlib.h>
#include <string.h>

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

User_list insert_user(User_list list, User user)
{
	User_list new_user;
	new_user = malloc(sizeof (*new_user));
	new_user->user = user;
	new_user->next = list->next;
	list->next = new_user;
	return new_user;
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

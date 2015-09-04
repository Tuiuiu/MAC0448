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

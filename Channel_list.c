/* Standard headers */
#include <stdlib.h>
#include <string.h>

/* Internal headers */
#include "Channel_list.h"

Channel_list chn_list_init()
{
    Channel_list ini;
    ini = malloc(sizeof (*ini));
    ini->channel = NULL;
    ini->next = NULL;
    return ini;
}

Channel_list insert_channel(Channel_list list, Channel channel)
{
    Channel_list new_channel;
    new_channel = malloc(sizeof (*new_channel));
    new_channel->channel = channel;
    new_channel->next = list->next;
    list->next = new_channel;
    return new_channel;
}

Channel_list insert_new_channel(Channel_list all_channels,
                                char *channel_name,
                                User creator)
{
    User_list channel_users;
    Channel new_channel;

    new_channel = malloc(sizeof(*new_channel));
    strcpy(new_channel->name, channel_name);
    channel_users = list_init();
    insert_user(channel_users, creator);
    new_channel->users = channel_users;

    return (insert_channel(all_channels, new_channel));
}

bool exists_channel(Channel_list list, char* channel_name)
{
    Channel_list aux;
    for (aux = list->next; aux != NULL; aux = aux->next)
    {
        if(strcmp(aux->channel->name, channel_name) == 0)
            return true;
    }
    return false;
}

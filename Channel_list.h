#ifndef _CHANNEL_LIST_H
#define _CHANNEL_LIST_H

/* Standard headers */
#include <stdbool.h>

/* Internal headers */
#include "Channel.h"

/* Typedefs */
typedef struct channel_list *Channel_list;

/* Structs */
struct channel_list {
    Channel      channel;
    Channel_list next;
}; 

/* Prototypes */
Channel_list chn_list_init();

Channel_list insert_channel(Channel_list list, Channel channel);

Channel_list insert_new_channel(Channel_list all_channels,
                                char *channel_name,
                                User creator);

bool exists_channel(Channel_list list, char *channel_name);

Channel remove_channel(Channel_list list, char* channel_name);

#endif /* _CHANNEL_LIST_H */
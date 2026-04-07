/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   aplimodes.cpp                                      :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: alexander <alexander@student.42.fr>        +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/04/06 12:41:55 by alexander         #+#    #+#             */
/*   Updated: 2026/04/06 12:44:48 by alexander        ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "Replies.hpp "


void CommandHandler::_applyModes(Client &client, Channel &channel,
                                const std::string &modes,
                                std::vector<std::string> &params,
                                size_t paramIndex)
{
    bool adding = true; // + o -

    for (size_t i = 0; i < modes.size(); i++)
    {
        char c = modes[i];

        if (c == '+')
        {
            adding = true;
            continue;
        }
        else if (c == '-')
        {
            adding = false;
            continue;
        }

        // las flags
        if (c == 'i') // invite-only
        {
            channel.setInviteOnly(adding);
        }
        else if (c == 't') // topic restricted
        {
            channel.setTopicRestricted(adding);
        }
        else if (c == 'o') // operator
        {
            if (paramIndex >= params.size())
                return;

            std::string nick = params[paramIndex++];
            Client *target = server.getClientByNick(nick);

            if (!target || !channel.isMember(*target))
                continue;

            if (adding)
                channel.addOperator(*target);
            else
                channel.removeOperator(*target);
        }
        else if (c == 'k') // key (password)
        {
            if (adding)
            {
                if (paramIndex >= params.size())
                    return;

                channel.setKey(params[paramIndex++]);
            }
            else
            {
                channel.removeKey();
            }
        }
    }

    // broadcast
    std::string msg = ":" + client.getPrefix() +
                      " MODE " + channel.getName() +
                      " " + modes + "\r\n";

    channel.broadcast(msg);
}
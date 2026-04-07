/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   handleInvite.cpp                                   :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: alexander <alexander@student.42.fr>        +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/04/02 11:30:39 by alexander         #+#    #+#             */
/*   Updated: 2026/04/06 12:42:59 by alexander        ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "Replies.hpp "

void CommandHandler::_handleInvite(Client &client, std::vector<std::string> &params)
{
    // parámetros minimos
    if (params.size() < 2)
    {
        sendReply(client, ERR_NEEDMOREPARAMS, "INVITE");
        return;
    }

    std::string targetNick = params[0];
    std::string channelName = params[1];

    // comprobar si el   canal existe
    if (!server.hasChannel(channelName))
    {
        sendReply(client, ERR_NOSUCHCHANNEL, channelName);
        return;
    }

    Channel &channel = server.getChannel(channelName);

    // el emisor debe estar en el canal si no return!!
    if (!channel.isMember(client))
    {
        sendReply(client, ERR_NOTONCHANNEL, channelName);
        return;
    }

    // buscarmos el  target usuario en el servidor
    Client *target = server.getClientByNick(targetNick);
    if (!target)
    {
        sendReply(client, ERR_NOSUCHNICK, targetNick);
        return;
    }

    // canal es +i →  es operador
    if (channel.isInviteOnly() && !channel.isOperator(client))
    {
        sendReply(client, ERR_CHANOPRIVSNEEDED, channelName);
        return;
    }

    // si ya está en el canal nos salimos
    if (channel.isMember(*target))
    {
        sendReply(client, ERR_USERONCHANNEL, targetNick, channelName);
        return;
    }

    // añadimos a lista de invitados
    channel.addInvited(*target);

    // enviamos RPL_INVITING al emisor
    sendReply(client, RPL_INVITING, targetNick, channelName);

    // notificamos al target
    std::string msg = ":" + client.getPrefix() +
                      " INVITE " + targetNick +
                      " :" + channelName + "\r\n";

    target->sendMessage(msg);
}
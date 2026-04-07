/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   CommandHandler.cpp                                 :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: alexander <alexander@student.42.fr>        +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/03/30 11:19:52 by alexander         #+#    #+#             */
/*   Updated: 2026/04/06 12:42:47 by alexander        ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "Replies.hpp "

void CommandHandler::_handleKick(Client &client, std::vector<std::string> &params)
{
    // parámetros mínimos
    if (params.size() < 2)
    {
        sendReply(client, ERR_NEEDMOREPARAMS, "KICK");
        return;
    }

    std::string channelName = params[0];
    std::string targetNick = params[1];
    std::string reason = (params.size() > 2) ? params[2] : client.getNick();

    // comprobar canal existe
    if (!server.hasChannel(channelName))
    {
        sendReply(client, ERR_NOSUCHCHANNEL, channelName);
        return;
    }

    Channel &channel = server.getChannel(channelName);

    // comprobar que el emisor es operador
    if (!channel.isOperator(client))
    {
        sendReply(client, ERR_CHANOPRIVSNEEDED, channelName);
        return;
    }

    // buscar target
    Client *target = server.getClientByNick(targetNick);
    if (!target)
    {
        sendReply(client, ERR_USERNOTINCHANNEL, targetNick, channelName);
        return;
    }

    // comprobar que está en el canal
    if (!channel.isMember(*target))
    {
        sendReply(client, ERR_USERNOTINCHANNEL, targetNick, channelName);
        return;
    }

    // construir mensaje KICK 
    std::string msg = ":" + client.getPrefix() +
                      " KICK " + channelName +
                      " " + targetNick +
                      " :" + reason + "\r\n";

    //  el broadcast ANTES de eliminar
    channel.broadcast(msg);

    // eliminar usuario del canal!!
    channel.removeMember(*target);
}
/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   handleTopic.cpp                                    :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: alexander <alexander@student.42.fr>        +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/04/02 11:42:58 by alexander         #+#    #+#             */
/*   Updated: 2026/04/06 12:43:32 by alexander        ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "Replies.hpp"

void CommandHandler::_handleTopic(Client &client, std::vector<std::string> &params)
{
    // parámetros mínimos
    if (params.size() < 1)
    {
        sendReply(client, ERR_NEEDMOREPARAMS, "TOPIC");
        return;
    }

    std::string channelName = params[0];

    // comprobar canal existe
    if (!server.hasChannel(channelName))
    {
        sendReply(client, ERR_NOSUCHCHANNEL, channelName);
        return;
    }

    Channel &channel = server.getChannel(channelName);

    // comprobamos que el cliente está en el canal
    if (!channel.isMember(client))
    {
        sendReply(client, ERR_NOTONCHANNEL, channelName);
        return;
    }

    // consultamos (sin topic nuevo)
    if (params.size() == 1)
    {
        std::string topic = channel.getTopic();

        if (topic.empty())
            sendReply(client, RPL_NOTOPIC, channelName);
        else
            sendReply(client, RPL_TOPIC, channelName, topic);

        return;
    }

    // si el canal es +t → solo operadores pueden cambiar
    if (channel.isTopicRestricted() && !channel.isOperator(client))
    {
        sendReply(client, ERR_CHANOPRIVSNEEDED, channelName);
        return;
    }

    // construir nuevo topic (puede tener espacios)
    std::string newTopic = params[1];
    for (size_t i = 2; i < params.size(); i++)
        newTopic += " " + params[i];

    //  guardar topic
    channel.setTopic(newTopic);

    // broadcast del cambio
    std::string msg = ":" + client.getPrefix() +
                      " TOPIC " + channelName +
                      " :" + newTopic + "\r\n";

    channel.broadcast(msg);
}
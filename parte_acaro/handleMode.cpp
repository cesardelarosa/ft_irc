/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   handleMode.cpp                                     :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: alexander <alexander@student.42.fr>        +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/04/05 12:07:51 by alexander         #+#    #+#             */
/*   Updated: 2026/04/06 12:43:14 by alexander        ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "Replies.hpp "

void CommandHandler::_handleMode(Client &client, std::vector<std::string> &params)
{
    //parametros minimos
    if (params.size() < 1)
    {
        sendReply(client, ERR_NEEDMOREPARAMS, "MODE");
        return;
    }

    std::string channelName = params[0];

    // si el canal existe
    if (!server.hasChannel(channelName))
    {
        sendReply(client, ERR_NOSUCHCHANNEL, channelName);
        return;
    }

    Channel &channel = server.getChannel(channelName);

    // consultamos
    if (params.size() == 1)
    {
        sendReply(client, RPL_CHANNELMODEIS, channelName, channel.getModes());
        return;
    }

    // si es operador
    if (!channel.isOperator(client))
    {
        sendReply(client, ERR_CHANOPRIVSNEEDED, channelName);
        return;
    }

    std::string modeString = params[1];

    // 🟢 aplicar modos
    _applyModes(client, channel, modeString, params, 2);
}
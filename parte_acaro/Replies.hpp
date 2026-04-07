/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Replies.hpp                                        :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: alexander <alexander@student.42.fr>        +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/04/05 12:11:44 by alexander         #+#    #+#             */
/*   Updated: 2026/04/06 12:44:15 by alexander        ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef REPLIES_HPP
#define REPLIES_HPP

// ERRORES (4xx)

#define ERR_NOSUCHNICK        "401"
#define ERR_NOSUCHCHANNEL     "403"
#define ERR_CANNOTSENDTOCHAN  "404"
#define ERR_UNKNOWNCOMMAND    "421"
#define ERR_NEEDMOREPARAMS    "461"
#define ERR_ALREADYREGISTRED  "462"
#define ERR_PASSWDMISMATCH    "464"

#define ERR_KEYSET            "467" // ya existe
#define ERR_CHANNELISFULL     "471"
#define ERR_UNKNOWNMODE       "472" //modo desconocido
#define ERR_INVITEONLYCHAN    "473"
#define ERR_BADCHANNELKEY     "475"

#define ERR_CHANOPRIVSNEEDED  "482"
#define ERR_NOTONCHANNEL      "442"
#define ERR_USERONCHANNEL     "443"
#define ERR_USERNOTINCHANNEL  "441"

// respuestillas (3xx)

#define RPL_INVITING          "341"
#define RPL_TOPIC             "332"
#define RPL_NOTOPIC           "331"

#define RPL_CHANNELMODEIS     "324" // importante MODE
#define RPL_ENDOFINVITELIST   "337"

#endif
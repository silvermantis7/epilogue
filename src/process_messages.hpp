#pragma once

#include <unordered_map>

namespace epilogue
{
    enum Command_ID : unsigned int
    {
        UNKNOWN,
        PING,
        PRIVMSG,
        JOIN,
        WELCOME,
        NICK_TAKEN,
        ERRONEOUS_NICK,
    };

    std::string nick;

    struct Command
    {
        Command_ID cmd_id = epilogue::Command_ID::UNKNOWN;
        std::string body = "";
        std::string context = "*global*";
        std::string user = "*.*";
    };

    Command process_message(std::string message);
}

epilogue::Command epilogue::process_message(std::string message)
{
    // split message into words
    std::vector<std::string> words;

    {
        std::string word;
        std::stringstream ss(message);

        while (std::getline(ss, word, ' '))
        {
            words.push_back(word);
        }
    }

    // command object to be returned
    epilogue::Command command;

    if (words.size() < 2)
    {
        command.context = "*none*";
        return command;
    }

    /* determine type of command */
    int cmd_num = std::atoi(words.at(1).c_str());

    if (cmd_num)
    {
        switch (cmd_num)
        {
        case 1:
            command.cmd_id = epilogue::Command_ID::WELCOME;
            epilogue::nick = words.at(2);
            break;

        case 433:
            command.cmd_id = epilogue::Command_ID::NICK_TAKEN;
            command.body = message.substr(message.find(':', 1) + 1);
            break;

        case 432:
            command.cmd_id = epilogue::Command_ID::ERRONEOUS_NICK;
            command.body = message.substr(message.find(':', 1) + 1);
            break;

        default:
            break;
        }
    }

    // server ping
    else if (words.at(0) == "PING")
    {
        command.cmd_id = epilogue::Command_ID::PING;
        command.body = words.at(1);
    }

    // sent message
    else if (words.at(1) == "PRIVMSG")
    {
        command.cmd_id = epilogue::Command_ID::PRIVMSG;
        command.body = message.substr(message.find(' ') + 1);
        command.body = command.body.substr(command.body.find(':') + 1);
        command.context = words.at(2);
        command.user = message.substr(1, message.find('!', 1) - 1);

        if (command.user == nick) { command.context = "*none*"; }
        else if (command.context == nick) { command.context = command.user; }

        command.user = "<" + command.user + ">";
    }

    // channel join
    else if (words.at(1) == "JOIN")
    {
        command.context = words.at(2);

        if (command.context.front() == ':')
        {
            command.context = command.context.substr(1);
        }

        command.user = message.substr(1, message.find('!') - 1);

        command.cmd_id = epilogue::Command_ID::JOIN;
        command.body = command.context + " <- " + command.user;
    }

    std::cout << "$ { "
        << command.cmd_id << ", \""
        << command.body << "\", \""
        << command.context << "\", \""
        << command.user << "\" }\n";

    return command;
}

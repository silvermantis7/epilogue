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
        PART,
        QUIT
    };

    std::string nick;

    struct Command
    {
        Command_ID cmd_id = epilogue::Command_ID::UNKNOWN;
        std::string body = "";
        std::string context = "*global*";
        std::string user = "*.*";
    };

    std::unordered_map<std::string, epilogue::Command_ID> literal_commands = {
        { "PRIVMSG", epilogue::Command_ID::PRIVMSG },
        { "JOIN", epilogue::Command_ID::JOIN },
        { "PART", epilogue::Command_ID::PART },
        { "QUIT", epilogue::Command_ID::QUIT }
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
            command.body = message.substr(message.find(':', 1) + 1);
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

        return command;
    }

    // server ping
    if (words.at(0) == "PING")
    {
        command.cmd_id = epilogue::Command_ID::PING;
        command.body = words.at(1);

        return command;
    }

    // discard command if not found in literal_commands
    if (literal_commands.find(words.at(1)) == literal_commands.end())
    {
        command.context = "*none*";
        return command;
    }

    command.cmd_id = literal_commands[words.at(1)];

    switch (command.cmd_id)
    {
    case epilogue::Command_ID::PRIVMSG:
        command.body = message.substr(message.find(" :") + 2);
        command.context = words.at(2);
        command.user = message.substr(1, message.find('!', 1) - 1);

        if (command.user == nick)
        {
            command.context = "*none*";
        }

        else if (command.context == "*nick*")
        {
            command.context = command.user;
        }

        break;

    case epilogue::Command_ID::JOIN:
        command.context = words.at(2);

        if (command.context.front() == ':')
        {
            command.context = command.context.substr(1);
        }

        command.user = message.substr(1, message.find('!') - 1);

        command.cmd_id = epilogue::Command_ID::JOIN;
        command.body = command.context + " <- " + command.user;

        break;

    case epilogue::Command_ID::PART:
        command.context = words.at(2);
        command.user = message.substr(1, message.find('!') - 1);
        command.body = message.substr(message.find(" :") + 2);

        break;

    case epilogue::Command_ID::QUIT:
        command.user = message.substr(1, message.find('!') - 1);
        command.body = message.substr(message.find(" :") + 2);

        break;

    default:
        break;
    }

    return command;
}

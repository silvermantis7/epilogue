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
    };

    std::string nick;

    using Command = std::tuple<Command_ID, std::string, std::string>;
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

    epilogue::Command_ID command_id = epilogue::Command_ID::UNKNOWN;
    std::string command_body = "";
    std::string channel_context = "*global*";

    /* determine type of command */
    int cmd_num = std::atoi(words.at(1).c_str());
    if (cmd_num)
    {
        switch (cmd_num)
        {
        case 1:
            command_id = epilogue::Command_ID::WELCOME;
            epilogue::nick = words.at(2);
            break;
        default:
            break;
        }
    }
    // server ping
    else if (words.at(0) == "PING")
    {
        command_id = epilogue::Command_ID::PING;
        command_body = words.at(1);
    }
    // sent message
    else if (words.at(1) == "PRIVMSG")
    {
        command_id = epilogue::Command_ID::PRIVMSG;
        command_body = "[" + words.at(2) + "] <"
            + message.substr(1, message.find('!') - 1) + "> "
            + message.substr(message.find(':', 1) + 1);
        channel_context = words.at(2);
    }
    // channel join
    else if (words.at(1) == "JOIN")
    {
        std::string channel = words.at(2);
        std::string nick_joined = message.substr(1, message.find('!') - 1);

        command_id = epilogue::Command_ID::JOIN;
        command_body = channel + " <- " + nick_joined;
        if (nick_joined != nick)
            channel_context = channel;
        else
            gui::main_frame->join(channel);
    }

    epilogue::Command command = { command_id, command_body, channel_context };
    std::cout << "$ { " << std::get<0>(command) << ", \""
        << std::get<1>(command) << "\" }\n";
    return command;
}

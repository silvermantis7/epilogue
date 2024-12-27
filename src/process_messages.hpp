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

    struct Command
    {
        Command_ID cmd_id;
        std::string body;
        std::string context;
        std::string sender;
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

    epilogue::Command_ID command_id = epilogue::Command_ID::UNKNOWN;
    std::string command_body = "";
    std::string channel_context = "*global*";
    std::string sender = "*.*";

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
        command_body = message.substr(message.find(':', 1) + 1);
        channel_context = words.at(2);
        sender = message.substr(1, message.find('!', 1) - 1);

        if (sender == nick) channel_context = "*none*";
        else if (channel_context == nick) channel_context = sender;

        sender = "<" + sender + ">";
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

    epilogue::Command command = {
        command_id,
        command_body,
        channel_context,
        sender.c_str()
    };

    std::cout << "$ { "
        << command.cmd_id << ", \""
        << command.body << "\", \""
        << command.context << "\", \""
        << command.sender << "\" }\n";
    return command;
}

#include <unordered_map>

namespace epilogue
{
    enum Command_ID : unsigned int
    {
        UNKNOWN,
        PING,
        PRIVMSG,
        JOIN,
    };

    using Command = std::pair<Command_ID, std::string>;
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

    /* determine type of command */
    // server ping
    if (words.at(0) == "PING")
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
    }
    // channel join
    else if (words.at(1) == "JOIN")
    {
        std::string channel = words.at(2);

        command_id = epilogue::Command_ID::JOIN;
        command_body = channel + " <- "
            + message.substr(1, message.find('!') - 1);
        
        gui::main_frame->join(channel);
    }

    epilogue::Command command = { command_id, command_body };
    std::cout << "$ { " << std::get<0>(command) << ", \""
        << std::get<1>(command) << "\" }\n";
    return command;
}

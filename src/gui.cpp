#include <thread>

#include "gui.hpp"

wxIMPLEMENT_APP(gui::Epilogue);

// pointers to each message display, key is channel name/context
std::unordered_map<std::string, gui::Panel*> channel_logs = { };

bool gui::Epilogue::OnInit()
{
    gui::main_frame = new gui::Main_Frame(nullptr);
    gui::main_frame->Show(true);
    return true;
}

gui::Main_Frame::Main_Frame(wxWindow* parent, wxWindowID id,
    const wxString& title, const wxPoint& pos, const wxSize& size, long style)
    : wxFrame(parent, id, title, pos, size, style)
{
    gui::Connect_Dialog* connect_dialog = new gui::Connect_Dialog(this);

    if (connect_dialog->ShowModal())
    {
        Destroy();
        return;
    }

    this->SetSizeHints(wxDefaultSize, wxDefaultSize);

    window_sizer = new wxBoxSizer(wxVERTICAL);

    main_notebook = new wxAuiNotebook(this, wxID_ANY, wxDefaultPosition,
        wxDefaultSize, wxAUI_NB_DEFAULT_STYLE);
    window_sizer->Add(main_notebook, 1, wxALL | wxEXPAND, 5);

    gui::Panel* global_panel = new gui::Panel("*global*", main_notebook);

    this->SetSizer(window_sizer);
    this->Layout();

    statusbar = this->CreateStatusBar(2, 0, wxID_ANY);

    this->Centre(wxBOTH);

    std::thread read_thread(gui::receive_messages);
    std::thread statusbar_thread(gui::update_statusbar, statusbar,
        &channel_context);
    read_thread.detach();
    statusbar_thread.detach();
}

gui::Main_Frame::~Main_Frame()
{
    if (connection)
    {
        // send QUIT message
        connection->send_message("QUIT :goodbye!\r\n");

        // close connection
        connection->close();
    }
}

gui::Connect_Dialog::Connect_Dialog(wxWindow* parent, wxWindowID id,
    const wxString& title, const wxPoint& pos, const wxSize& size, long style)
    : wxDialog(parent, id, title, pos, size, style)
{
    this->SetSizeHints(wxDefaultSize, wxDefaultSize);
    wxBoxSizer* window_sizer = new wxBoxSizer(wxVERTICAL);

    // server <address:port>
    wxBoxSizer* server_sizer = new wxBoxSizer(wxHORIZONTAL);
    wxStaticText* server_label = new wxStaticText(this, wxID_ANY,
        wxT("server <host:port>"), wxDefaultPosition, wxSize(180, -1),
        wxALIGN_CENTER_VERTICAL);
    server_input = new wxTextCtrl(this, wxID_ANY, wxEmptyString,
        wxDefaultPosition, wxSize(300, -1), 0);
    server_input->SetFont(wxFont(-1, wxFONTFAMILY_TELETYPE, wxFONTSTYLE_NORMAL,
        wxFONTWEIGHT_NORMAL, false, wxT("Monospace")));
    server_sizer->Add(server_label, 0, wxALL, 0);
    server_sizer->Add(server_input, 0, wxALL, 0);

    // nick/user
    wxBoxSizer* nick_sizer = new wxBoxSizer(wxHORIZONTAL);
    wxStaticText* nick_label = new wxStaticText(this, wxID_ANY,
        wxT("nick (same as user)"), wxDefaultPosition, wxSize(180, -1),
        wxALIGN_CENTER_VERTICAL);
    nick_input = new wxTextCtrl(this, wxID_ANY, wxEmptyString,
        wxDefaultPosition, wxSize(300, -1), 0);
    nick_input->SetFont(wxFont(-1, wxFONTFAMILY_TELETYPE, wxFONTSTYLE_NORMAL,
        wxFONTWEIGHT_NORMAL, false, wxT("Monospace")));
    nick_sizer->Add(nick_label, 0, wxALL, 0);
    nick_sizer->Add(nick_input, 0, wxALL, 0);

    // real name
    wxBoxSizer* realname_sizer = new wxBoxSizer(wxHORIZONTAL);
    wxStaticText* realname_label = new wxStaticText(this, wxID_ANY,
        wxT("realname"), wxDefaultPosition, wxSize(180, -1),
        wxALIGN_CENTER_VERTICAL);
    realname_input = new wxTextCtrl(this, wxID_ANY, wxEmptyString,
        wxDefaultPosition, wxSize(300, -1), 0);
    realname_input->SetFont(wxFont(-1, wxFONTFAMILY_TELETYPE,
        wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL, false, wxT("Monospace")));
    realname_sizer->Add(realname_label, 0, wxALL, 0);
    realname_sizer->Add(realname_input, 0, wxALL, 0);

    connect_button = new wxButton(this, wxID_ANY, _("connect"),
        wxDefaultPosition, wxSize(-1, -1), 0);
    connect_button->SetFont(wxFont(-1, wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL,
        wxFONTWEIGHT_NORMAL, false, wxEmptyString));

    window_sizer->Add(server_sizer, 0, wxALL, 5);
    window_sizer->Add(nick_sizer, 0, wxALL, 5);
    window_sizer->Add(realname_sizer, 0, wxALL, 5);
    window_sizer->Add(connect_button, 0, wxALL | wxEXPAND, 5);

    this->SetSizer(window_sizer);
    this->Layout();
    window_sizer->Fit(this);

    connect_button->Bind(wxEVT_BUTTON, &Connect_Dialog::connect, this);
    this->Bind(wxEVT_CLOSE_WINDOW, &Connect_Dialog::on_close, this);

    this->Centre(wxBOTH);
}

void gui::Connect_Dialog::on_close(wxCloseEvent& event)
{
    EndModal(-1);
}

gui::Connect_Dialog::~Connect_Dialog()
{
}

void gui::Panel::send_message(wxCommandEvent& event)
{
    if (message_box->IsEmpty())
    {
        return;
    }

    gui::main_frame->CallAfter([this]
    {
        try
        {
            std::string message = message_box->GetValue().ToStdString();
            message_box->Clear();

            // if the message should be the body of a privmsg
            bool is_privmsg = (context != "*global*");
            if (message.at(0) == '/')
            {
                message.erase(0, 1);
                is_privmsg &= (message.at(0) == '/');
            }

            log_message("-->", message);

            if (is_privmsg)
            {
                message = "PRIVMSG " + context + " :" + message;
            }

            gui::main_frame->get_connection()->send_message(message + "\r\n");
        }
        catch (std::exception& e)
        {
            std::cerr << e.what() << "\n";
            wxMessageBox(e.what(), "client disconnected",
                wxOK | wxICON_INFORMATION);
        }
    });
}

static void gui::receive_messages()
{
    try
    {
        for (;;)
        {
            if (connection->closed())
            {
                break;
            }

            for (epilogue::Command command : connection->read_messages())
            {
                // types of commands to be looged in the message display
                static const std::vector<epilogue::Command_ID> loggable = {
                    epilogue::Command_ID::PRIVMSG,
                    epilogue::Command_ID::JOIN
                };

                switch (command.cmd_id)
                {
                case epilogue::Command_ID::JOIN:
                    if (command.user == epilogue::nick)
                    {
                        main_frame->join(command.context);
                        command.body = "joined [" + command.context + "]";
                        command.context = "*global*";
                    }

                    command.user = "*.*";

                    break;

                default:
                    break;
                }

                if (std::find(loggable.begin(), loggable.end(), command.cmd_id)
                    != loggable.end())
                {
                    if (command.context == "*none*")
                    {
                        break;
                    }

                    wxWindow* notebook
                        = gui::main_frame->get_notebook()->GetCurrentPage();

                    using Panel = gui::Panel;

                    main_frame->CallAfter([&, command]
                    {
                        Panel* panel;

                        // check if command context has a panel
                        if (channel_logs.find(command.context)
                            != channel_logs.end())
                        {
                            panel = channel_logs[command.context];
                        }
                        else
                        {
                            panel = new Panel(command.context,
                                main_frame->get_notebook());
                        }

                        panel->log_message(command.user, command.body);
                    });
                }
            }
        }
    }
    catch (std::exception& e)
    {
        wxMessageBox(e.what(), "client disconnected",
            wxOK | wxICON_INFORMATION);
    }
}

void gui::Connect_Dialog::connect(wxCommandEvent& event)
{
    // check input isn't empty
    if (server_input->IsEmpty())
    {
        wxMessageBox("Please specify server address: e.g. localhost:1234.",
            "server is empty", wxOK | wxICON_INFORMATION);
        return;
    }

    std::string connection_string = server_input->GetValue().ToStdString();

    // check connection string contains one ':' charactor
    int n_colon = 0;

    for (char c : connection_string)
    {
        if (c == ':')
        {
            n_colon++;
        }
    }

    // check server input contains a colon
    if (n_colon != 1)
    {
        wxMessageBox("Server needs a port (missing colon).", "invalid server",
            wxOK | wxICON_INFORMATION);
        return;
    }

    int colon_pos = connection_string.find(':'); // find position of colon
    std::string host = connection_string.substr(0, colon_pos);
    std::string port = connection_string.substr(++colon_pos);

    // check connection string specifies a valid port
    if (!std::atoi(port.c_str()))
    {
        wxMessageBox("Port must be a number.", "invalid port",
            wxOK | wxICON_INFORMATION);
        return;
    }

    // check nick is given
    if (nick_input->IsEmpty())
    {
        wxMessageBox("Please specify a nick.", "nick is empty",
            wxOK | wxICON_INFORMATION);
        return;
    }

    // check realname is given
    if (realname_input->IsEmpty())
    {
        wxMessageBox("Please specify realname.", "realname is empty",
            wxOK | wxICON_INFORMATION);
        return;
    }

    std::cout << "<*> connecting to " << connection_string << "\n";
    std::cout << "\t-> " << host << "\n";
    std::cout << "\t-> " << port << "\n";

    try
    {
        if (!connected)
        {
            connection = ::epilogue::Connection::create(io_context);
            connection->connect(host, port);
            connected = true;

            std::cout << "<*> connection successful\n";

            // lock server input
            server_input->Disable();
        }

        // attempt to authenticate with server
        std::string nick = nick_input->GetValue().ToStdString();
        std::string realname = realname_input->GetValue().ToStdString();
        connection->send_message("NICK :" + nick + "\r\n");
        connection->send_message("USER " + nick + " 0 * :" + realname + "\r\n");

        std::vector<epilogue::Command> received_messages;

        for(;;)
        {
            for (epilogue::Command command : connection->read_messages())
            {
                switch (command.cmd_id)
                {
                case epilogue::Command_ID::WELCOME:
                    EndModal(0);
                    return;

                // case epilogue::Command_ID::1234:
                //     wxMessageBox(command.body, "", wxOK | wxICON_INFORMAITON);
                //     break;

                default: break;
                }
            }

            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
    }
    catch (std::exception& e)
    {
        std::cerr << e.what() << "\n";
        wxMessageBox(e.what(), "failed to establish connection",
            wxOK | wxICON_INFORMATION);
        return;
    }
}

static void gui::update_statusbar(wxStatusBar* statusbar,
    std::string* channel_context)
{
    for (;;)
    {
        time_t time_ptr = time(NULL);
        tm* time_now = std::localtime(&time_ptr);

        char time_str[50];
        std::strftime(time_str, 50, "%A %H:%M", time_now);

        wxTheApp->CallAfter([statusbar, channel_context, time_str]
        {
            statusbar->SetStatusText(time_str, 0);
            statusbar->SetStatusText(*channel_context, 1);
        });

        sleep(1);
    }
}

void gui::Main_Frame::join(std::string channel)
{
    std::cout << "joining [" + channel + "]\n";

    gui::main_frame->CallAfter([channel, this]
    {
        gui::Panel* new_panel = new gui::Panel(channel, main_notebook);
    });
}

gui::Panel::Panel(std::string context, wxAuiNotebook* notebook)
    : wxPanel(notebook, wxID_ANY, wxDefaultPosition, wxDefaultSize,
        wxTAB_TRAVERSAL)
{
    panel_sizer = new wxBoxSizer(wxVERTICAL);
    this->context = context;
    this->SetSizer(panel_sizer);

    // create message window
    message_display = new wxScrolledWindow(this, wxID_ANY, wxDefaultPosition,
        wxDefaultSize, wxVSCROLL | wxEXPAND);
    message_display->SetScrollRate(0, 1);
    message_display->SetBackgroundColour(wxColour(0x00, 0x00, 0x00));
    panel_sizer->Add(message_display, 1, wxALL | wxEXPAND, 5);

    // create message sizer
    message_sizer = new wxFlexGridSizer(0, 2, 2, 5);
    message_sizer->SetFlexibleDirection(wxBOTH);
    message_display->SetSizer(message_sizer);
    message_sizer->FitInside(message_display);

    // create message box
    message_box = new wxTextCtrl(this, wxID_ANY, wxEmptyString,
        wxDefaultPosition, wxSize(-1, -1), 0 | wxTE_PROCESS_ENTER);
    message_box->SetFont(wxFont(-1, wxFONTFAMILY_TELETYPE, wxFONTSTYLE_NORMAL,
        wxFONTWEIGHT_NORMAL, false, wxT("Monospace")));
    panel_sizer->Add(message_box, 0, wxALL | wxEXPAND, 5);

    // create page
    this->Layout();
    panel_sizer->Fit(this);
    notebook->AddPage(this, _(context), false, wxNullBitmap);

    // bind message input to send_message
    message_box->Bind(wxEVT_COMMAND_TEXT_ENTER, &gui::Panel::send_message,
        this);
    message_box->SetFocus();

    // add message display to hashmap of message logs
    channel_logs[context] = this;
}

void gui::Panel::log_message(const std::string& user,
    const std::string& message)
{
    // check if scrollbar is at the bottom
    int scroll_pos = message_display->GetScrollPos(wxVERTICAL);
    int scroll_range = message_display->GetScrollRange(wxVERTICAL);
    int thumb_size = message_display->GetScrollThumb(wxVERTICAL);
    bool autoscroll = (scroll_pos + thumb_size == scroll_range);

    wxStaticText* user_label = new wxStaticText(message_display, wxID_ANY,
        user);
    wxStaticText* message_label = new Message_Label(message_display, message);
    user_label->SetForegroundColour(wxColour(0xFF, 0xFF, 0xFF));
    message_label->SetForegroundColour(wxColour(0xFF, 0xFF, 0xFF));

    message_sizer->Add(user_label, 0, wxALL, 2);
    message_sizer->Add(message_label, 0, wxALL, 2);

    message_sizer->FitInside(message_display);

    if (autoscroll)
    {
        // scroll down to new message
        scroll_range = message_display->GetScrollRange(wxVERTICAL);
        thumb_size = message_display->GetScrollThumb(wxVERTICAL);
        message_display->Scroll(0, scroll_range - thumb_size);
    }
}

gui::Panel::~Panel()
{
    // remove self from channel_logs hashmap
    channel_logs.erase(context);

    if (context != "*global*")
    {
        // send PART message
        main_frame->get_connection()->send_message("PART " + context
            + " :goodbye!\r\n");
    }
    else
    {
        // close application
        main_frame->Close();
    }
}

gui::Message_Label::Message_Label(wxScrolledWindow* message_display,
    const std::string& message)
    : wxStaticText(message_display, wxID_ANY, message)
    , message_display{message_display}
    , message{message}
{
    wrap();

    Bind(wxEVT_SIZE, &gui::Message_Label::wrap, this);
}

void gui::Message_Label::wrap()
{
    int width, height, x_pos, y_pos;
    message_display->GetSize(&width, &height);
    message_display->GetPosition(&x_pos, &y_pos);
    Wrap(width - x_pos - 40);
}

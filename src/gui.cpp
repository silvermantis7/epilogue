#include "gui.hpp"
#include "process_messages.hpp"

wxIMPLEMENT_APP(gui::Epilogue);

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
    gui::Connect_Dialog connect_dialog(this);
    connect_dialog.ShowModal();

    try
    {
        asio::io_context io_context;

        connection = ::epilogue::Connection::create(io_context);
        connection->connect(connect_dialog.host(), connect_dialog.port());
    }
    catch (std::exception& e)
    {
        std::cerr << e.what() << "\n";
        wxMessageBox(e.what(), "failed to establish connection",
            wxOK | wxICON_INFORMATION);
        exit(-1);
    }

    std::cout << "<*> connection successful\n";

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

    std::thread receive_thread(gui::receive_messages, connection);
    receive_thread.detach();

    std::thread statusbar_thread(gui::update_statusbar, statusbar,
        &channel_context);
    statusbar_thread.detach();
}

gui::Main_Frame::~Main_Frame()
{
    delete window_sizer;
    window_sizer = nullptr;
    delete main_notebook;
    main_notebook = nullptr;
}

gui::Connect_Dialog::Connect_Dialog(wxWindow* parent, wxWindowID id,
    const wxString& title, const wxPoint& pos, const wxSize& size, long style)
    : wxDialog(parent, id, title, pos, size, style)
{
    this->SetSizeHints(wxDefaultSize, wxDefaultSize);

    wxBoxSizer* window_sizer;
    window_sizer = new wxBoxSizer(wxVERTICAL);

    server_input = new wxTextCtrl(this, wxID_ANY, wxEmptyString,
        wxDefaultPosition, wxSize(300, 25), 0 | wxTE_PROCESS_ENTER);
    server_input->SetFont(wxFont(-1, wxFONTFAMILY_TELETYPE, wxFONTSTYLE_NORMAL,
        wxFONTWEIGHT_NORMAL, false, wxT("Monospace")));

    window_sizer->Add(server_input, 0, wxALL, 5);

    connect_button = new wxButton(this, wxID_ANY, _("connect"),
        wxDefaultPosition, wxSize(-1, 25), 0);
    connect_button->SetFont(wxFont(-1, wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL,
        wxFONTWEIGHT_NORMAL, false, wxEmptyString));

    window_sizer->Add(connect_button, 0, wxALL | wxEXPAND, 5);

    this->SetSizer(window_sizer);
    this->Layout();
    window_sizer->Fit(this);

    connect_button->Bind(wxEVT_BUTTON, &Connect_Dialog::connect, this);
    server_input->Bind(wxEVT_COMMAND_TEXT_ENTER, &Connect_Dialog::connect,
        this);

    this->Centre(wxBOTH);
}

gui::Connect_Dialog::~Connect_Dialog()
{
}

void gui::Panel::send_message(wxCommandEvent& event)
{
    if (message_box->IsEmpty())
        return;

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

            int n_rows = message_display->GetItemCount();
            message_display->InsertItem(n_rows, "-->");
            message_display->SetItem(n_rows, 1, message);
            message_display->SetColumnWidth(0, wxLIST_AUTOSIZE);
            message_display->SetColumnWidth(1, wxLIST_AUTOSIZE);

            if (is_privmsg)
                message = "PRIVMSG " + context + " :" + message;

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

static void gui::receive_messages(::epilogue::Connection::pointer connection)
{
    try
    {
        for (;;)
        {
            for (std::string message : connection->read_messages())
            {
                message.pop_back(); // remove '\r' character
                epilogue::Command command = epilogue::process_message(message);

                // types of commands to be looged in the message display
                static const std::vector<epilogue::Command_ID> loggable = {
                    epilogue::Command_ID::PRIVMSG,
                    epilogue::Command_ID::JOIN
                };

                if (command.cmd_id == epilogue::Command_ID::PING)
                    connection->send_message("PONG " + command.body + "\r\n");
                if (std::find(loggable.begin(), loggable.end(), command.cmd_id)
                    != loggable.end())
                {
                    if (command.context == "*none*") break;
                    wxWindow* notebook
                        = gui::main_frame->get_notebook()->GetCurrentPage();

                    using Panel = gui::Panel;

                    main_frame->CallAfter([&, command]
                    {
                        // check if command context has a panel
                        if (auto panel = Panel::channel_logs.find(
                            command.context);
                            panel == Panel::channel_logs.end())
                        {
                            new Panel(command.context,
                                main_frame->get_notebook());
                        }

                        wxListCtrl* message_display =
                        Panel::channel_logs[command.context];

                        int n_rows = message_display->GetItemCount();

                        message_display->InsertItem(n_rows, command.sender);
                        message_display->SetItem(n_rows, 1, command.body);
                        message_display->SetColumnWidth(0, wxLIST_AUTOSIZE);
                        message_display->SetColumnWidth(1, wxLIST_AUTOSIZE);
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
        return;

    std::string connection_string = server_input->GetValue().ToStdString();

    // check connection string contains one ':' charactor
    int n_colon = 0;

    for (char c : connection_string)
        if (c == ':')
            n_colon++;

    if (n_colon != 1)
        return;

    // find position of colon
    int colon_pos = connection_string.find(':');
    // get host
    host_ = connection_string.substr(0, colon_pos);
    // get port
    port_ = connection_string.substr(++colon_pos);

    // check connection string specifies a valid port
    if (!std::atoi(port_.c_str()))
        return;

    std::cout << "<*> connecting to " << connection_string << "\n";
    std::cout << "\t-> " << host_ << "\n";
    std::cout << "\t-> " << port_ << "\n";

    EndModal(0);
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
    // create message display
    message_display = new wxListCtrl(this, wxID_ANY, wxDefaultPosition,
        wxDefaultSize, wxLC_REPORT | wxLC_NO_HEADER);
    message_display->SetFont(wxFont(-1, wxFONTFAMILY_TELETYPE,
        wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL, false, wxT("Monospace")));
    message_display->InsertColumn(0, "user", wxLIST_FORMAT_RIGHT);
    message_display->InsertColumn(1, "message", wxLIST_FORMAT_LEFT);
    panel_sizer->Add(message_display, 1, wxEXPAND | wxALL, 5);
    // create message box
    message_box = new wxTextCtrl(this, wxID_ANY, wxEmptyString,
        wxDefaultPosition, wxSize(-1, 25), 0 | wxTE_PROCESS_ENTER);
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
    channel_logs[context] = message_display;
}

std::unordered_map<std::string, wxListCtrl*> gui::Panel::channel_logs = { };

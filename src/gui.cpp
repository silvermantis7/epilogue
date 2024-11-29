#include "gui.hpp"
#include "process_messages.hpp"

wxIMPLEMENT_APP(gui::Epilogue);

bool gui::Epilogue::OnInit()
{
    gui::Frame* frame = new gui::Frame(nullptr);
    frame->Show(true);
    return true;
}

gui::Frame::Frame(wxWindow* parent, wxWindowID id, const wxString& title,
    const wxPoint& pos, const wxSize& size, long style)
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
    this->SetForegroundColour(wxColour(0, 255, 255));
    this->SetBackgroundColour(wxColour(34, 34, 34));

    wxBoxSizer* box_sizer;
    box_sizer = new wxBoxSizer(wxVERTICAL);

    message_display = new wxListCtrl(this, wxID_ANY, wxDefaultPosition,
        wxDefaultSize, wxLC_ICON | wxLC_REPORT | wxLC_NO_HEADER);
    message_display->SetFont(wxFont(9, wxFONTFAMILY_TELETYPE,
        wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL, false, wxT("Monospace")));
    message_display->SetForegroundColour(wxColour(255, 255, 255));
    message_display->SetBackgroundColour(wxColour(24, 24, 24));
    message_display->InsertColumn(0, "message", wxLIST_FORMAT_LEFT);

    box_sizer->Add(message_display, 1, wxEXPAND | wxALL, 5);

    message_box = new wxTextCtrl(this, wxID_ANY, wxEmptyString,
        wxDefaultPosition, wxSize(-1,25), 0 | wxTE_PROCESS_ENTER);
    message_box->SetFont(wxFont(9, wxFONTFAMILY_TELETYPE, wxFONTSTYLE_NORMAL,
        wxFONTWEIGHT_NORMAL, false, wxT("Monospace")));
    message_box->SetForegroundColour(wxColour(255, 255, 255));
    message_box->SetBackgroundColour(wxColour(48, 48, 48));

    box_sizer->Add(message_box, 0, wxALL | wxEXPAND, 5);

    this->SetSizer(box_sizer);
    this->Layout();

    statusbar = this->CreateStatusBar(2, 0, wxID_ANY);
    statusbar->SetForegroundColour(wxColour(255, 255, 255));
    statusbar->SetBackgroundColour(wxColour(16, 16, 16));

    this->Centre(wxBOTH);

    message_box->Connect(wxEVT_COMMAND_TEXT_ENTER,
        wxCommandEventHandler(gui::Frame::send_message), NULL, this);
    message_box->SetFocus();

    std::thread receive_thread(gui::receive_messages, message_display,
        connection);
    receive_thread.detach();

    std::thread statusbar_thread(gui::update_statusbar, statusbar,
        &channel_context);
    statusbar_thread.detach();
}

gui::Frame::~Frame()
{
}

gui::Connect_Dialog::Connect_Dialog(wxWindow* parent, wxWindowID id,
    const wxString& title, const wxPoint& pos, const wxSize& size, long style)
    : wxDialog(parent, id, title, pos, size, style)
{
	this->SetSizeHints(wxDefaultSize, wxDefaultSize);
	this->SetForegroundColour(wxColour(255, 255, 255));
	this->SetBackgroundColour(wxColour(34, 34, 34));

	wxBoxSizer* bSizer2;
	bSizer2 = new wxBoxSizer(wxVERTICAL);

	server_input = new wxTextCtrl(this, wxID_ANY, wxEmptyString,
        wxDefaultPosition, wxSize(200, 25), 0 | wxTE_PROCESS_ENTER);
	server_input->SetFont(wxFont(8, wxFONTFAMILY_TELETYPE, wxFONTSTYLE_NORMAL,
        wxFONTWEIGHT_NORMAL, false, wxT("Monospace")));
	server_input->SetForegroundColour(wxColour(255, 255, 255));
	server_input->SetBackgroundColour(wxColour(48, 48, 48));

	bSizer2->Add(server_input, 0, wxALL, 5);

	connect_button = new wxButton(this, wxID_ANY, _("connect"),
        wxDefaultPosition, wxSize(-1, 25), 0);
    connect_button->SetFont(wxFont(8, wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL,
        wxFONTWEIGHT_NORMAL, false, wxEmptyString));
	connect_button->SetForegroundColour(wxColour(255, 255, 255));
	connect_button->SetBackgroundColour(wxColour(40, 40, 40));

	bSizer2->Add(connect_button, 0, wxALL | wxEXPAND, 5);

	this->SetSizer(bSizer2);
	this->Layout();
	bSizer2->Fit(this);

    connect_button->Bind(wxEVT_BUTTON, &Connect_Dialog::connect, this);
    server_input->Bind(wxEVT_COMMAND_TEXT_ENTER, &Connect_Dialog::connect,
        this);

	this->Centre(wxBOTH);
}

gui::Connect_Dialog::~Connect_Dialog()
{
}

void gui::Frame::send_message(wxCommandEvent& event)
{
    if (message_box->IsEmpty())
        return;

    try
    {
        std::string message = message_box->GetValue().ToStdString();

        // if the message should be the body of a privmsg
        if (message.at(0) != '/' || message.at(1) == '/')
        {
            if (message.at(0) == '/')
                message.erase(0, 1);

            message = "PRIVMSG " + channel_context + " :" + message;
        }

        if (message.at(0) == '/')
            message.erase(0, 1);

        connection->send_message(message + "\r\n");

        message_display->InsertItem(message_display->GetItemCount(), message);
        message_display->SetColumnWidth(0, wxLIST_AUTOSIZE);
    }
    catch (std::exception& e)
    {
        std::cerr << e.what() << "\n";
        wxMessageBox(e.what(), "client disconnected",
            wxOK | wxICON_INFORMATION);
    }

    message_box->Clear();
}

static void gui::receive_messages(wxListCtrl* message_display,
    ::epilogue::Connection::pointer connection)
{
    try
    {
        for (;;)
        {
            for (std::string message : connection->read_messages())
            {
                message.pop_back(); // remove '\r' character

                epilogue::Command command = epilogue::process_message(message);
                epilogue::Command_ID command_id = std::get<0>(command);
                std::string command_body = std::get<1>(command);

                if (command_id == epilogue::Command_ID::PING)
                    connection->send_message("PONG " + command_body + "\r\n");
                else if (command_id == epilogue::Command_ID::PRIVMSG
                    || command_id == epilogue::Command_ID::JOIN)
                {
                    wxTheApp->CallAfter([command_body, message_display]
                    {
                        message_display->InsertItem(
                            message_display->GetItemCount(), command_body);
                        message_display->SetColumnWidth(0, wxLIST_AUTOSIZE);
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

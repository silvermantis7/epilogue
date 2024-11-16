#include "gui.hpp"

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
    try
    {
        asio::io_context io_context;

        connection = ::epilogue::Connection::create(io_context);
        connection->connect("127.0.0.1", "6667");
    }
    catch (std::exception& e)
    {
        std::cerr << e.what() << "\n";
        wxMessageBox(e.what(), "failed to establish connection",
            wxOK | wxICON_INFORMATION);
        exit(-1);
    }

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

	this->Centre(wxBOTH);

    message_box->Connect(wxEVT_COMMAND_TEXT_ENTER,
        wxCommandEventHandler(gui::Frame::send_message), NULL, this);
    message_box->SetFocus();

    std::thread receive_thread(gui::receive_messages, message_display,
        connection);
    receive_thread.detach();
}

gui::Frame::~Frame()
{
}

void gui::Frame::send_message(wxCommandEvent& event)
{
    if (message_box->IsEmpty())
        return;

    try
    {
        connection->send_message(message_box->GetValue().ToStdString() + "\n");
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

                wxTheApp->CallAfter([message, message_display]
                {
                    message_display->InsertItem(message_display->GetItemCount(),
                        message);
                    message_display->SetColumnWidth(0, wxLIST_AUTOSIZE);
                });
            }
        }
    }
    catch (std::exception& e)
    {
        wxMessageBox(e.what(), "client disconnected",
            wxOK | wxICON_INFORMATION);
    }
}

#include <UI/GridPreferencesDialog.h>

gridPreferences::gridPreferences() : wxDialog(NULL, wxID_ANY, "Grid")
{
    wxFont *font = new wxFont(8.5, wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL);
    
    wxBoxSizer *line1Sizer = new wxBoxSizer(wxHORIZONTAL);
    wxBoxSizer *line2Sizer = new wxBoxSizer(wxHORIZONTAL);
    wxBoxSizer *line3Sizer = new wxBoxSizer(wxHORIZONTAL);
    wxBoxSizer *line4Sizer = new wxBoxSizer(wxHORIZONTAL);
    wxBoxSizer *line5Sizer = new wxBoxSizer(wxHORIZONTAL);
    wxBoxSizer *footerSizer = new wxBoxSizer(wxHORIZONTAL);
    wxBoxSizer *topSizer = new wxBoxSizer(wxVERTICAL);
    
    wxStaticText *ppuText = new wxStaticText(this, wxID_ANY, "Pixels/Unit:");
    ppuText->SetFont(*font);
    _ppuTextCtrl->Create(this, wxID_ANY, wxEmptyString, wxPoint(79, 12), wxSize(100, 20));
    _ppuTextCtrl->SetFont(*font);
    
    line1Sizer->Add(ppuText, 0, wxCENTER | wxALL, 6);
    line1Sizer->Add(_ppuTextCtrl, 0, wxCENTER | wxTOP | wxBOTTOM | wxRIGHT, 6);
    
    wxStaticText *gridText = new wxStaticText(this, wxID_ANY, "Grid Size:");
    gridText->SetFont(*font);
    _gridSizeTextCtrl->Create(this, wxID_ANY, wxEmptyString, wxPoint(79, 38), wxSize(100, 20));
    _gridSizeTextCtrl->SetFont(*font);
    
    line2Sizer->Add(gridText, 0, wxCENTER | wxBOTTOM | wxLEFT | wxRIGHT, 6);
    line2Sizer->Add(_gridSizeTextCtrl, 0, wxCENTER | wxTOP | wxBOTTOM | wxRIGHT, 6);
    
    _showGridCheckBox->Create(this, wxID_ANY, "Show Grid", wxPoint(15, 64), wxSize(75, 17));
    _showGridCheckBox->SetFont(*font);
    _showOriginCheckBox->Create(this, wxID_ANY, "Show Origin", wxPoint(102, 64), wxSize(83, 17));
    _showOriginCheckBox->SetFont(*font);
    
    line3Sizer->Add(_showGridCheckBox, 0, wxCENTER | wxBOTTOM | wxRIGHT | wxLEFT, 6);
    line3Sizer->Add(_showOriginCheckBox, 0, wxCENTER | wxTOP | wxBOTTOM | wxRIGHT, 6)
    
    _snapGridCheckBox->Create(this, wxID_ANY, "Snap Grid", wxPoint(15, 87), wxSize(73, 17));
    _snapGridCheckBox->SetFont(*font);
    
    line4Sizer->Add(_snapGridCheckBox, 0, wxCENTER | wxBOTTOM | wxRIGHT | wxLEFT, 6);
    
    _showBlockNameCheckBox->Create(this, wxID_ANY, "Show Block Names", wxPoint(15, 110), wxSize(119, 17));
    
    line5Sizer->Add(_showBlockNameCheckBox, 0, wxCENTER | wxBOTTOM | wxRIGHT | wxLEFT, 6);
    
    wxButton *okButton = new wxButton(this, wxID_OK, "Ok", wxPoint(23, 133), wxSize(75, 23));
    okButton->SetFont(*font);
    
    wxButton *cancelButton = new wxButton(this, wxID_ANY, "Cancel", wxPoint(104, 133), wxSize(75, 23));
    cancelButton->SetFont(*font);
    
    footerSizer->Add(okButton, 0, wxCENTER | wxBOTTOM | wxRIGHT | wxLEFT, 6);
    footerSizer->Add(cancelButton, 0, wxCENTER | wxBOTTOM | wxRIGHT, 6);
    
    topSizer->Add(line1Sizer);
    topSizer->Add(line2Sizer);
    topSizer->Add(line3Sizer);
    topSizer->Add(line4Sizer);
    topSizer->Add(footerSizer, 0, wxALIGN_RIGHT);
    
    SetSizerAndFit(topSizer);
    
    updateInterface();
}



void gridPreferences::getParameters()
{
    
}



void gridPreferences::setParameters()
{
    
}



void gridPreferences::updateInterface()
{
    
}



gridPreferences::~gridPreferences()
{
    
}
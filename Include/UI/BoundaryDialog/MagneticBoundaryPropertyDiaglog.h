#ifndef MAGNETIC_BOUNDARY_PROPERTY_DIALOG_H_
#define MAGNETIC_BOUNDARY_PROPERTY_DIALOG_H_

#include <string>

#include <wx/wx.h>
#include <wx/stattext.h>
#include <wx/combobox.h>
#include <wx/arrstr.h>
#include <wx/statbox.h>

#include <common/enums.h>
#include <common/BoundaryConditions.h>
#include <common/MagneticBoundary.h>

/*! /class magneticBoundaryDialog
 *  /brief
 *  This class handles all of the interaction in order to add/modify a boundary condition for
 *  a mangetic boundary
 */
class magneticBoundaryDialog : public wxDialog
{
private:
    //! This is a local copy of the new or modified magnetic boundary
    magneticBoundary _magneticBoundary;
    
    wxTextCtrl *nameTextCtrl = new wxTextCtrl();
    
    wxArrayString *BCNameArray = new wxArrayString();
    
    wxComboBox *BCComboBox = new wxComboBox();
    
    wxStaticBox *smallSkinDepthGroupBox = new wxStaticBox();
    
    wxStaticBox *mixedBCGroupBox = new wxStaticBox();
    
    wxStaticBox *prescribedAGroupBox = new wxStaticBox();
    
    wxTextCtrl *uRelativeTextCtrl = new wxTextCtrl();
    
    wxTextCtrl *sigTextCtrl = new wxTextCtrl();
    
    wxTextCtrl *c1TextCtrl = new wxTextCtrl();
    
    wxTextCtrl *c0TextCtrl = new wxTextCtrl();
    
    wxTextCtrl *A0TextCtrl = new wxTextCtrl();
    
    wxTextCtrl *A1TextCtrl = new wxTextCtrl();
    
    wxTextCtrl *A2TextCtrl = new wxTextCtrl();
    
    wxTextCtrl *phiTextCtrl = new wxTextCtrl();
    
    void onBCComboChange(wxCommandEvent &event);
    
public:
    //! The constructor for the class
    magneticBoundaryDialog();
    
    //! The desctrutor for the class
    ~magneticBoundaryDialog();
    
    //! This function will return the new/modified boundary condition. However, this function must be called after the form return a wxID_OK
    magneticBoundary getNewBoundary();
    
    //! This function will set the boundary condition of the local boundary condition. Useful for when modifying a BC
    void setBoundaryCondition();
    
private:
   wxDECLARE_EVENT_TABLE();
};

#endif
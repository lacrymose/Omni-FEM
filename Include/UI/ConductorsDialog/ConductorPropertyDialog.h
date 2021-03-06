#ifndef CONDUCTORPROPERTYDIALOG_H_
#define CONDUCTORPROPERTYDIALOG_H_

#include <string>
#include <iostream>
#include <iomanip>

#include <wx/wx.h>
#include <wx/stattext.h>
#include <wx/combobox.h>
#include <wx/arrstr.h>
#include <wx/statbox.h>
#include <wx/sizer.h>
#include <wx/radiobut.h>

#include <common/enums.h>
#include <common/ConductorProperty.h>

//! Class that handles editing of the electrical conductor
/*!
    This class creates a dialog which is then able to 
    expose the conductor property class to the user for editing.
    This class is able to accept a conductor as an input. If this
    is the case, then the class will display the properties of the 
    conductor to the user for editing. This particular class is 
    used in electrical simualtions.
    For additional documentation of the wxdialog class, refer to the
    following link:
    http://docs.wxwidgets.org/3.1.0/classwx_dialog.html
*/ 
class conductorPropertyDialog : public wxDialog
{
private:

    //! The conductor that is being edited 
    /*!
        For new conductors, this variable will contain the
        default parameters. For conductors that are being edited,
        this variable will contain the parameters of the conductor
        that is being edited
    */
    conductorProperty _conductorProperty;
    
    //! Text box that is used to display the name of the conductor
    /*!
        This text box is drawn so that the user is able to edit
        the name of the conductor.
        For additional documentation on the wxTextCtrl class refer
        to the following link:
        http://docs.wxwidgets.org/3.1.0/classwx_text_ctrl.html
    */ 
    wxTextCtrl *nameTextCtrl = new wxTextCtrl();
    
    //! Text box that is used to display the value of the total charge of the conductor
    /*!
        This text box is drawn so that the user is able to edit
        the value of the total charge of the conductor.
        This text box is enabled if the user selects Total Charge.
        For additional documentation on the wxTextCtrl class refer
        to the following link:
        http://docs.wxwidgets.org/3.1.0/classwx_text_ctrl.html
    */ 
    wxTextCtrl *chargeTextCtrl = new wxTextCtrl();
    
    //! Text box that is used to display the value of the prescribed voltage of the conductor
    /*!
        This text box is drawn so that the user is able to edit
        the value of the prescribed voltage of the conductor.
        This text box is enabled if the user selects Prescribed Voltage.
        For additional documentation on the wxTextCtrl class refer
        to the following link:
        http://docs.wxwidgets.org/3.1.0/classwx_text_ctrl.html
    */
    wxTextCtrl *voltageTextCtrl = new wxTextCtrl();
    
    //! Radio button used to enable selection of Total Charge
    /*!
        This radio button is used to allow the user to select the
        conductor as a total charge
        For additional documentation for the wxRadioButton class
        refer to the following link:
        http://docs.wxwidgets.org/trunk/classwx_radio_button.html
    */ 
    wxRadioButton *radioButton1 = new wxRadioButton();
    
    //! Radio button used to enable selection of Total Charge
    /*!
        This radio button is used to allow the user to select the
        conductor as a total charge
        For additional documentation for the wxRadioButton class
        refer to the following link:
        http://docs.wxwidgets.org/trunk/classwx_radio_button.html
    */ 
    wxRadioButton *radioButton2 = new wxRadioButton();
    
    //! Event procedure for the first radio button
    /*!
        Event procedure for the Total Charge button. This function will
        enable the chargeTextCtrl and disable the voltageTextCtrl.
        \param event Required parameter for proper event procedure functionality
    */ 
    void onRadioButton1Click(wxCommandEvent &event);
    
    //! Event procedure for the second radio button
    /*!
        Event procedure for the Prescribed Voltage button. This function will
        disable the chargeTextCtrl and enable the voltageTextCtrl.
        \param event Required parameter for proper event procedure functionality
    */
    void onRadioButton2Click(wxCommandEvent &event);
    
    //! Function that is called in order to change the values in the text boxes
    /*!
        This function is called after the creation of the dialog. If the user 
        sets a conductor, then the program will need to update the 
        interface in order to display the apprioate values for the conductor.
        This also applies if the user is creating a new conductor. The program calls
        the clear conductor function in order to reset the values stored in the boundary
        condition. This function will also update the states of the radio buttons.
    */
    void updateInterface();
    
public:

    //! Constructor of the class
    /*!
        This function will create the dialog and
        place all of the relevent widgets onto the 
        dialog and position all widgets where
        they need to go.
        \param par Pointer to the parent window
    */ 
    conductorPropertyDialog(wxWindow *par);
    
    //! Function that is used to retrieve the conductor from the dialog
    /*!
        This function is used in order to retrieve the parameters for the conductor 
        that the user editted.
        \param newConductor The conductor that the user wants to save the parameters to
    */ 
    void getNewConductor(conductorProperty &newConductor);
    
    //! Function that is used in order to set the conductor of the dialog
    /*!
        This function is primarly used when the program needs to edit
        a selected conductor. This function will take the 
        user selection and set the conductor variable within this class. The
        function will then update the interface in order to expose all of the 
        values of the conductor
        \param conductor The conductor condition that will be edited
    */ 
    void setConductor(conductorProperty &conductor);
    
    //! Resets the conductor back to default value
    /*!
        This is useful when adding a new conductor
    */ 
    void clearConductor();
    
    //! The destructor of the class
    ~conductorPropertyDialog();
    
private:
    //! Required for event procedures to work
    wxDECLARE_EVENT_TABLE(); 
};



#endif
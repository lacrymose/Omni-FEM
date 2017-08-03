/*
	This file will contain all of the class implementation for the file menu in the menu bar of the main fram
*/

//#define BOOST_NO_EXCEPTIONS

#include "UI/OmniFEMFrame.h"
#include <fstream>
#include <istream>
#include "UI/geometryShapes.h"

#include <common/ElectricalBoundary.h>
#include <common/ElectroStaticMaterial.h>
#include <common/ElectrostaticPreference.h>
#include <common/ConductorProperty.h>

#include <common/CircuitProperty.h>
#include <common/MagneticBoundary.h>
#include <common/MagneticMaterial.h>
#include <common/MagneticPreference.h>

#include <common/NodalProperty.h>

#include <common/ExteriorRegion.h>

#include <boost/archive/text_oarchive.hpp>
#include <boost/archive/text_iarchive.hpp>
#include "boost/serialization/vector.hpp"
#include <boost/archive/archive_exception.hpp>




void OmniFEMMainFrame::onNewFile(wxCommandEvent &event)
{
    if(_UIState == systemState::MODEL_DEFINING || _UIState == systemState::PHYSICS_CHOOSING || _UIState == systemState::SIMULATING || _UIState == systemState::VIEWING_RESULTS)
    {
        // Create a message box confirming that the user would like to create a new file 
        if(wxMessageBox("Create New File?", "New File", wxOK | wxCANCEL | wxICON_QUESTION) == wxCANCEL)
            return;
    }
    enableToolMenuBar(false);
    createProblemChoosingClient();
}


void OmniFEMMainFrame::onOpenFile(wxCommandEvent &event)
{	
	wxFileDialog openFileDialog(this, "Open File", "", "", "", wxFD_OPEN | wxFD_FILE_MUST_EXIST | wxFD_CHANGE_DIR);
	
	if(openFileDialog.ShowModal() != wxID_CANCEL)
	{
		wxMessageBox("File has been opened", "Open File", wxOK | wxICON_INFORMATION);
	}
}


void OmniFEMMainFrame::OnSave(wxCommandEvent &event)
{
	if(_saveFilePath != "")
	{
		save(_saveFilePath);
	}
	else
	{
		wxFileDialog saveFileDialog(this, "Save File", "", "", "", wxFD_SAVE | wxFD_OVERWRITE_PROMPT | wxFD_CHANGE_DIR);
		if(saveFileDialog.ShowModal() != wxID_CANCEL)
		{
			wxString appendedTitle = "Omni-FEM - ";
            wxString fileName;
            if(saveFileDialog.GetFilename().Contains(wxString(".omniFEM")))
            {
                std::string tempFileName = saveFileDialog.GetFilename().ToStdString();
                for(int i = 0; i < (tempFileName.length() - 8); i++)
                    fileName += wxString(tempFileName[i]);
            }
            else
                fileName = saveFileDialog.GetFilename();
			_problemDefinition.setName(fileName);
			appendedTitle.append(_problemDefinition.getName());
			this->SetTitle(appendedTitle);
			_saveFilePath = saveFileDialog.GetPath();
			save(_saveFilePath);
		}
	}
}



void OmniFEMMainFrame::onSaveAs(wxCommandEvent &event)
{
	wxFileDialog saveFileDialog(this, "Save File As", "", "", "", wxFD_SAVE | wxFD_OVERWRITE_PROMPT | wxFD_CHANGE_DIR);
	
	if(saveFileDialog.ShowModal() != wxID_CANCEL)
	{
        wxString appendedTitle = "Omni-FEM - ";
		wxString fileName;
		if(saveFileDialog.GetFilename().Contains(wxString(".omniFEM")))
		{
			std::string tempFileName = saveFileDialog.GetFilename().ToStdString();
			for(int i = 0; i < (tempFileName.length() - 8); i++)
				fileName += wxString(tempFileName[i]);
		}
		else
			fileName = saveFileDialog.GetFilename();
			
		_problemDefinition.setName(fileName);
        appendedTitle.append(_problemDefinition.getName());
        this->SetTitle(appendedTitle);
		_saveFilePath = saveFileDialog.GetPath();
		save(_saveFilePath);
	}
}



void OmniFEMMainFrame::save(string filePath)
{
	std::ofstream saveFile;
	wxString pathName(filePath);
	
	if(!pathName.Contains(wxString(".omniFEM")))
		pathName += wxString(".omniFEM");
	
//	saveFile.open(pathName.ToStdString(), std::ofstream::out);
	std::ofstream ofs(pathName.ToStdString());
	boost::archive::text_oarchive oa(ofs);
	oa << _problemDefinition;
	oa << _model;
	
/*	if(saveFile.is_open())
	{	
		saveFile << "v1.0" << std::endl;
		saveFile << std::to_string((int)_problemDefinition.getPhysicsProblem()) << std::endl;
		saveFile << std::to_string((int)_model->getGridPreferences()->getCoordinateSystem()) << std::endl;
		if(_problemDefinition.getPhysicsProblem() == physicProblems::PROB_ELECTROSTATIC)
		{
			saveFile << _problemDefinition.getElectricalPreferences().getComments().ToStdString() << std::endl;
			saveFile << "\\-\\" << endl;
			saveFile << _problemDefinition.getElectricalPreferences().getDepth() << std::endl;
			saveFile << _problemDefinition.getElectricalPreferences().getMinAngle() << std::endl;
			saveFile << _problemDefinition.getElectricalPreferences().getPrecision() << std::endl;
			saveFile << (int)_problemDefinition.getElectricalPreferences().getProblemType() << std::endl;
			saveFile << (int)_problemDefinition.getElectricalPreferences().getUnitLength() << std::endl;
			
			saveFile << "MATERIALS" << std::endl;
			for(std::vector<electrostaticMaterial>::iterator materialIterator = _problemDefinition.getElectricalMaterialList()->begin(); materialIterator != _problemDefinition.getElectricalMaterialList()->end(); materialIterator++)
			{
				wxString name = wxString(materialIterator->getName());
				wxString relativePermittivityX = wxString(std::to_string(materialIterator->getEpsilonX()));
				wxString relativePermittivityY = wxString(std::to_string(materialIterator->getEpsilonY()));
				wxString chargeDensity = wxString(std::to_string(materialIterator->getChargeDensity()));
				wxString combinedForm = name + wxString(",") + relativePermittivityX + wxString(",") + relativePermittivityY + wxString(",") + chargeDensity;
				saveFile << combinedForm.ToStdString() << std::endl;
			}
			
			saveFile << "BOUNDARYCONDITIONS" << std::endl;
			for(std::vector<electricalBoundary>::iterator boundaryIterator = _problemDefinition.getElectricalBoundaryList()->begin(); boundaryIterator != _problemDefinition.getElectricalBoundaryList()->end(); boundaryIterator++)
			{
				wxString name = wxString(boundaryIterator->getBoundaryName());
				wxString boundaryType = wxString(std::to_string((int)boundaryIterator->getBC()));
				wxString c0 = wxString(std::to_string(boundaryIterator->getC0Value()));
				wxString c1 = wxString(std::to_string(boundaryIterator->getC1Value()));
				wxString sigma = wxString(std::to_string(boundaryIterator->getSigma()));
				wxString voltage = wxString(std::to_string(boundaryIterator->getVoltage()));
				wxString combinedForm = name + wxString(",") + boundaryType + wxString(",") + c0 + wxString(",") + c1 + wxString(",") + sigma + wxString(",") + voltage;
				saveFile << combinedForm.ToStdString() << endl;
			}
			
			saveFile << "NODEPROPERTIES" << std::endl;
			for(std::vector<nodalProperty>::iterator nodalIterator = _problemDefinition.getNodalPropertyList()->begin(); nodalIterator != _problemDefinition.getNodalPropertyList()->end(); nodalIterator++)
			{
				wxString name = wxString(nodalIterator->getName());
				wxString isSpecificpotentialProperty = wxString(std::to_string((int)nodalIterator->getState()));
				wxString valueNumber = wxString(std::to_string(nodalIterator->getValue()));
				wxString combinedForm = name + wxString(",") + isSpecificpotentialProperty + wxString(",") + valueNumber;
				saveFile << combinedForm.ToStdString() << endl;
			}
			
			saveFile << "CONDUCTORPROPERTIES" << std::endl;
			for(std::vector<conductorProperty>::iterator conductorIterator = _problemDefinition.getConductorList()->begin(); conductorIterator != _problemDefinition.getConductorList()->end(); conductorIterator++)
			{
				wxString name = conductorIterator->getName();
				wxString aBoolean = wxString(std::to_string((int)conductorIterator->getIsTotalChargeState()));
				wxString valueNumber = wxString(std::to_string(conductorIterator->getValue()));
				wxString combinedForm = name + wxString(",") + aBoolean + wxString(",") + valueNumber;
				saveFile << combinedForm.ToStdString() << std::endl;	
			}
			
			saveFile << "EXTERNALREGION" << std::endl;
			wxString exteriorRegionData = wxString(std::to_string(_problemDefinition.getExteriorRegion()->getCenter())) + wxString(",") + wxString(std::to_string(_problemDefinition.getExteriorRegion()->getRadiusExterior())) + wxString(",") + wxString(std::to_string(_problemDefinition.getExteriorRegion()->getRadiusInterior()));
			saveFile << exteriorRegionData.ToStdString() << endl;
		}
		else if(_problemDefinition.getPhysicsProblem() == physicProblems::PROB_MAGNETICS)
		{
			saveFile << _problemDefinition.getMagneticPreference().getComments().ToStdString() << std::endl;
			saveFile << "\\-\\" << std::endl;
			saveFile << _problemDefinition.getMagneticPreference().getDepth() << std::endl;
			saveFile << (int)_problemDefinition.getMagneticPreference().getACSolver() << std::endl;
			saveFile << _problemDefinition.getMagneticPreference().getFrequency() << std::endl;
			saveFile << _problemDefinition.getMagneticPreference().getMinAngle() << std::endl;
			saveFile << _problemDefinition.getMagneticPreference().getPrecision() << std::endl;
			saveFile << (int)_problemDefinition.getMagneticPreference().getProblemType() << std::endl;
			saveFile << (int)_problemDefinition.getMagneticPreference().getUnitLength() << std::endl;
			
			saveFile << "MATERIALS" << std::endl;
			for(std::vector<magneticMaterial>::iterator materialIterator = _problemDefinition.getMagnetMaterialList()->begin(); materialIterator != _problemDefinition.getMagnetMaterialList()->end(); materialIterator++)
			{
				wxString materialName = wxString(materialIterator->getName());
				wxString linearState = wxString(std::to_string((int)materialIterator->getBHState()));
				wxString relativeX = wxString(std::to_string(materialIterator->getMUrX()));
				wxString relativeY = wxString(std::to_string(materialIterator->getMUrY()));
				wxString valuePhiX = wxString(std::to_string(materialIterator->getPhiX()));
				wxString valuePhiY = wxString(std::to_string(materialIterator->getPhiY()));
				wxString valueCoercivity = wxString(std::to_string(materialIterator->getCoercivity()));
				wxString valueConduct = wxString(std::to_string(materialIterator->getSigma()));
				wxString valueCurrent = wxString(std::to_string(materialIterator->getCurrentDensity()));
				wxString specialValue = wxString(std::to_string((int)materialIterator->getSpecialAttribute()));
				wxString thicknessValue = wxString(std::to_string(materialIterator->getLaminationThickness()));
				wxString strandValue = wxString(std::to_string(materialIterator->getNumberStrands()));
				wxString valueFF = wxString(std::to_string(materialIterator->getLaminationFillFactor()));
				wxString valueDiameter = wxString(std::to_string(materialIterator->getStrandDiameter()));
				/* This next section is for the nonlinear properties 
				wxString anisotropyState = wxString(std::to_string((int)materialIterator->getJilesAtherton().getIsAnisotropyMaterial()));
				wxString valueAlpha = wxString(std::to_string(materialIterator->getJilesAtherton().getAlpha()));
				wxString valueAParam = wxString(std::to_string(materialIterator->getJilesAtherton().getAParam()));
				wxString valueMsParam = wxString(std::to_string(materialIterator->getJilesAtherton().getSaturationMagnetization()));
				wxString valueKParam = wxString(std::to_string(materialIterator->getJilesAtherton().getKParam()));
				wxString valueCParam = wxString(std::to_string(materialIterator->getJilesAtherton().getMagnetizationReversibility()));
				wxString valuePsi = wxString(std::to_string(materialIterator->getJilesAtherton().getPsi()));
				wxString valueTParam = wxString(std::to_string(materialIterator->getJilesAtherton().getTParameter()));
				wxString combinedForm = materialName + wxString(",") + linearState + wxString(",") + relativeX + wxString(",") + relativeY + wxString(",") + valuePhiX + wxString(",") 
										+ valuePhiY + wxString(",") + valueCoercivity + wxString(",") + valueConduct + wxString(",") + valueCurrent + wxString(",") + specialValue +
										wxString(",") + thicknessValue + wxString(",") + strandValue + wxString(",") + valueFF + wxString(",") + valueDiameter + wxString(",") + 
										anisotropyState + wxString(",") + valueAlpha + wxString(",") + valueAParam + wxString(",") + valueMsParam + wxString(",") + valueKParam + 
										wxString(",") + valueCParam + wxString(",") + valuePsi + wxString(",") + valueTParam;
				saveFile << combinedForm.ToStdString() << std::endl;
			}
			
			saveFile << "BOUNDARYCONDITION" << std::endl;
			for(std::vector<magneticBoundary>::iterator boundaryIterator = _problemDefinition.getMagneticBoundaryList()->begin(); boundaryIterator != _problemDefinition.getMagneticBoundaryList()->end(); boundaryIterator++)
			{
				wxString boundaryName = wxString(boundaryIterator->getBoundaryName());
				wxString valueA0 = wxString(std::to_string(boundaryIterator->getA0()));
				wxString valueA1 = wxString(std::to_string(boundaryIterator->getA1()));
				wxString valueA2 = wxString(std::to_string(boundaryIterator->getA2()));
				wxString boundaryType = wxString(std::to_string((int)boundaryIterator->getBC()));
				wxString valueC0 = wxString(std::to_string(boundaryIterator->getC0Value()));
				wxString valueC1 = wxString(std::to_string(boundaryIterator->getC1Value()));
				wxString valueMu = wxString(std::to_string(boundaryIterator->getMu()));
				wxString valuePhi = wxString(std::to_string(boundaryIterator->getPhi()));
				wxString valueSig = wxString(std::to_string(boundaryIterator->getSigma()));
				wxString combinedForm = boundaryName + wxString(",") + valueA0 + wxString(",") + valueA1 + wxString(",") + valueA2 + wxString(",") + boundaryType
										+ wxString(",") + valueC0 + wxString(",") + valueC1 + wxString(",") + valueMu + wxString(",") + valuePhi + wxString(",") 
										+ valueSig;
				saveFile << combinedForm.ToStdString() << std::endl;
			}
			
			saveFile << "NODEPROPERTIES" << std::endl;
			for(std::vector<nodalProperty>::iterator nodalIterator = _problemDefinition.getNodalPropertyList()->begin(); nodalIterator != _problemDefinition.getNodalPropertyList()->end(); nodalIterator++)
			{
				wxString name = wxString(nodalIterator->getName());
				wxString isSpecificpotentialProperty = wxString(std::to_string((int)nodalIterator->getState()));
				wxString valueNumber = wxString(std::to_string(nodalIterator->getValue()));
				wxString combinedForm = name + wxString(",") + isSpecificpotentialProperty + wxString(",") + valueNumber;
				saveFile << combinedForm.ToStdString() << endl;
			}
			
			saveFile << "CIRCUITPROPERTIES" << std::endl;
			for(std::vector<circuitProperty>::iterator circuitIterator = _problemDefinition.getCircuitList()->begin(); circuitIterator != _problemDefinition.getCircuitList()->end(); circuitIterator++)
			{
				wxString circuitName = wxString(circuitIterator->getName());
				wxString seriesState = wxString(std::to_string((int)circuitIterator->getCircuitSeriesState()));
				wxString currentValue = wxString(std::to_string(circuitIterator->getCurrent()));
				wxString combinedForm = circuitName + wxString(",") + seriesState + wxString(",") + currentValue;
				saveFile << combinedForm.ToStdString() << std::endl;
			}
		}
		
		saveFile << "NODES" << std::endl;
		for(plf::colony<node>::iterator nodeIterator = _model->getModelNodeList()->begin(); nodeIterator != _model->getModelNodeList()->end(); nodeIterator++)
		{
			wxString centerX = wxString(std::to_string(nodeIterator->getCenterXCoordinate()));
			wxString centerY = wxString(std::to_string(nodeIterator->getCenterYCoordinate()));
			wxString nodeNumber = wxString(std::to_string(nodeIterator->getNodeID()));
			wxString nodalSetting = wxString(nodeIterator->getNodeSetting()->getNodalPropertyName());
			wxString conductorName = wxString(nodeIterator->getNodeSetting()->getConductorPropertyName());
			wxString groupNumber = wxString(std::to_string(nodeIterator->getNodeSetting()->getGroupNumber()));
			wxString combinedForm = centerX + wxString(",") + centerY + wxString(",") + nodeNumber + wxString(",") + nodalSetting + wxString(",") + conductorName + wxString(",") + groupNumber;
			saveFile << combinedForm.ToStdString() << std::endl;
		}
		
		saveFile << "LINES" << std::endl;
		for(plf::colony<edgeLineShape>::iterator lineIterator = _model->getModelLineList()->begin(); lineIterator != _model->getModelLineList()->end(); lineIterator++)
		{
			wxString firstNode = wxString(std::to_string(lineIterator->getFirstNode()->getNodeID()));
			wxString secondNode = wxString(std::to_string(lineIterator->getSecondNode()->getNodeID()));
			wxString lineProperty = wxString(lineIterator->getSegmentProperty()->getBoundaryName());
			wxString autoMeshSpacing = wxString(std::to_string((int)lineIterator->getSegmentProperty()->getMeshAutoState()));
			wxString elementSize = wxString(std::to_string(lineIterator->getSegmentProperty()->getElementSizeAlongLine()));
			wxString conductorName = wxString(lineIterator->getSegmentProperty()->getConductorName());
			wxString segmentHidden = wxString(std::to_string((int)lineIterator->getSegmentProperty()->getHiddenState()));
			wxString groupNumber = wxString(std::to_string(lineIterator->getSegmentProperty()->getGroupNumber()));
			wxString combinedForm = firstNode + wxString(",") + secondNode + wxString(",") + lineProperty + wxString(",") + autoMeshSpacing
									+ wxString(",") + elementSize + wxString(",") + conductorName + wxString(",") + segmentHidden + wxString(",")
									+ groupNumber;;
			saveFile << combinedForm.ToStdString() << endl;
		}
		
		saveFile << "ARCS" << std::endl;
		for(plf::colony<arcShape>::iterator arcIterator = _model->getModelArcList()->begin(); arcIterator != _model->getModelArcList()->end(); arcIterator++)
		{
			wxString lineProperty = wxString(arcIterator->getSegmentProperty()->getBoundaryName());
			wxString arcAngle = wxString(std::to_string(arcIterator->getArcAngle()));
			wxString segmentNumber = wxString(std::to_string(arcIterator->getnumSegments()));
			wxString firstNode = wxString(std::to_string(arcIterator->getFirstNode()->getNodeID()));
			wxString secondNode = wxString(std::to_string(arcIterator->getSecondNode()->getNodeID()));
			wxString radius = wxString(std::to_string(arcIterator->getRadius()));
			wxString centerX = wxString(std::to_string(arcIterator->getCenterXCoordinate()));
			wxString centerY = wxString(std::to_string(arcIterator->getCenterYCoordinate()));
			wxString autoMeshSpacing = wxString(std::to_string((int)arcIterator->getSegmentProperty()->getMeshAutoState()));
			wxString elementSize = wxString(std::to_string(arcIterator->getSegmentProperty()->getElementSizeAlongLine()));
			wxString conductorName = wxString(arcIterator->getSegmentProperty()->getConductorName());
			wxString segmentHidden = wxString(std::to_string((int)arcIterator->getSegmentProperty()->getHiddenState()));
			wxString groupNumber = wxString(std::to_string(arcIterator->getSegmentProperty()->getGroupNumber()));			
			wxString combinedForm = lineProperty + wxString(",") + arcAngle + wxString(",") + segmentNumber + wxString(",") + firstNode + wxString(",") + secondNode + wxString(",") 
									+ radius + wxString(",") + centerY + wxString(",") + centerX + wxString(",") + autoMeshSpacing + wxString(",") + elementSize + wxString(",") + 
									conductorName + wxString(",") + segmentHidden + wxString(",") + groupNumber;
			saveFile << combinedForm.ToStdString() << std::endl;
		}
		
		saveFile << "BLOCKLABELS" << std::endl;
		for(plf::colony<blockLabel>::iterator labelIterator = _model->getModelBlockList()->begin(); labelIterator != _model->getModelBlockList()->end(); labelIterator++)
		{
			wxString blockPropertyMaterial = wxString(labelIterator->getProperty()->getMaterialName());
			wxString blockPropertyCircuit = wxString(labelIterator->getProperty()->getCircuitName());
			wxString meshSizeType = wxString(std::to_string((int)labelIterator->getProperty()->getMeshsizeType()));
			wxString autoMesh = wxString(std::to_string((int)labelIterator->getProperty()->getAutoMeshState()));
			wxString theMeshSize = wxString(std::to_string(labelIterator->getProperty()->getMeshSize()));
			wxString turnCount = wxString(std::to_string(labelIterator->getProperty()->getNumberOfTurns()));
			wxString magnetization = labelIterator->getProperty()->getMagnetization();// This is mainly becuase everything else is doing it
			wxString groupNumber = wxString(std::to_string(labelIterator->getProperty()->getGroupNumber()));
			wxString externalState = wxString(std::to_string((int)labelIterator->getProperty()->getIsExternalState()));
			wxString defaultState = wxString(std::to_string((int)labelIterator->getProperty()->getDefaultState()));
			wxString centerX = wxString(std::to_string(labelIterator->getCenterXCoordinate()));
			wxString centerY = wxString(std::to_string(labelIterator->getCenterYCoordinate()));
			wxString combinedForm = blockPropertyMaterial + wxString(",") + blockPropertyCircuit + wxString(",") + meshSizeType + wxString(",") + autoMesh + wxString(",") + theMeshSize
									+ wxString(",") + turnCount + wxString(",") + magnetization + wxString(",") + groupNumber + wxString(",") + externalState + wxString(",") 
									+ defaultState + wxString(",") + centerX + wxString(",") + centerY;
			saveFile << combinedForm.ToStdString() << std::endl;
		}
	}	
	else
	{
		wxMessageBox("Please close all instances of the file before saving");
	}*/
	saveFile.close();
}



void OmniFEMMainFrame::load(string filePath)
{
	std::ifstream loadFile;
	wxString pathName(filePath);

	loadFile.open(pathName.ToStdString(), std::ofstream::in);
	
	if(loadFile.is_open())
	{
		while(!loadFile.eof())
		{
			
		}
	}
}
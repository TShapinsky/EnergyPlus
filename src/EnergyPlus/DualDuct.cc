// EnergyPlus, Copyright (c) 1996-2020, The Board of Trustees of the University of Illinois,
// The Regents of the University of California, through Lawrence Berkeley National Laboratory
// (subject to receipt of any required approvals from the U.S. Dept. of Energy), Oak Ridge
// National Laboratory, managed by UT-Battelle, Alliance for Sustainable Energy, LLC, and other
// contributors. All rights reserved.
//
// NOTICE: This Software was developed under funding from the U.S. Department of Energy and the
// U.S. Government consequently retains certain rights. As such, the U.S. Government has been
// granted for itself and others acting on its behalf a paid-up, nonexclusive, irrevocable,
// worldwide license in the Software to reproduce, distribute copies to the public, prepare
// derivative works, and perform publicly and display publicly, and to permit others to do so.
//
// Redistribution and use in source and binary forms, with or without modification, are permitted
// provided that the following conditions are met:
//
// (1) Redistributions of source code must retain the above copyright notice, this list of
//     conditions and the following disclaimer.
//
// (2) Redistributions in binary form must reproduce the above copyright notice, this list of
//     conditions and the following disclaimer in the documentation and/or other materials
//     provided with the distribution.
//
// (3) Neither the name of the University of California, Lawrence Berkeley National Laboratory,
//     the University of Illinois, U.S. Dept. of Energy nor the names of its contributors may be
//     used to endorse or promote products derived from this software without specific prior
//     written permission.
//
// (4) Use of EnergyPlus(TM) Name. If Licensee (i) distributes the software in stand-alone form
//     without changes from the version obtained under this License, or (ii) Licensee makes a
//     reference solely to the software portion of its product, Licensee must refer to the
//     software as "EnergyPlus version X" software, where "X" is the version number Licensee
//     obtained under this License and may not use a different name for the software. Except as
//     specifically required in this Section (4), Licensee shall not use in a company name, a
//     product name, in advertising, publicity, or other promotional activities any name, trade
//     name, trademark, logo, or other designation of "EnergyPlus", "E+", "e+" or confusingly
//     similar designation, without the U.S. Department of Energy's prior written consent.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR
// IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY
// AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR
// CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
// CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
// SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
// THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
// OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
// POSSIBILITY OF SUCH DAMAGE.

// C++ Headers
#include <cmath>
#include <string>

// ObjexxFCL Headers
#include <ObjexxFCL/Array.functions.hh>
#include <ObjexxFCL/Fmath.hh>
#include <ObjexxFCL/gio.hh>
#include <ObjexxFCL/string.functions.hh>

// EnergyPlus Headers
#include <EnergyPlus/BranchNodeConnections.hh>
#include <EnergyPlus/DataAirLoop.hh>
#include <EnergyPlus/DataContaminantBalance.hh>
#include <EnergyPlus/DataConvergParams.hh>
#include <EnergyPlus/DataDefineEquip.hh>
#include <EnergyPlus/DataEnvironment.hh>
#include <EnergyPlus/DataHVACGlobals.hh>
#include <EnergyPlus/DataHeatBalFanSys.hh>
#include <EnergyPlus/DataHeatBalance.hh>
#include <EnergyPlus/DataIPShortCuts.hh>
#include <EnergyPlus/DataLoopNode.hh>
#include <EnergyPlus/DataPrecisionGlobals.hh>
#include <EnergyPlus/DataSizing.hh>
#include <EnergyPlus/DataZoneEnergyDemands.hh>
#include <EnergyPlus/DataZoneEquipment.hh>
#include <EnergyPlus/DualDuct.hh>
#include <EnergyPlus/General.hh>
#include <EnergyPlus/GeneralRoutines.hh>
#include <EnergyPlus/GlobalNames.hh>
#include <EnergyPlus/InputProcessing/InputProcessor.hh>
#include <EnergyPlus/NodeInputManager.hh>
#include <EnergyPlus/OutputProcessor.hh>
#include <EnergyPlus/Psychrometrics.hh>
#include <EnergyPlus/ReportSizingManager.hh>
#include <EnergyPlus/ScheduleManager.hh>
#include <EnergyPlus/UtilityRoutines.hh>

namespace EnergyPlus {

namespace DualDuct {
    // Module containing the DualDuct simulation routines

    // MODULE INFORMATION:
    //       AUTHOR         Richard J. Liesen
    //       DATE WRITTEN   February 2000
    //       MODIFIED       Clayton Miller, Brent Griffith Aug. 2010 - Added DualDuctOA Terminal Unit to Simulate Decoupled OA/RA
    //       RE-ENGINEERED  na

    // PURPOSE OF THIS MODULE:
    // To encapsulate the data and algorithms required to
    // manage the DualDuct Systems Simulation

    // METHODOLOGY EMPLOYED:
    // Needs description, as appropriate.

    // Using/Aliasing
    using namespace DataPrecisionGlobals;
    using namespace DataLoopNode;
    using DataEnvironment::StdRhoAir;
    using DataGlobals::BeginEnvrnFlag;
    using DataGlobals::NumOfZones;
    using DataGlobals::ScheduleAlwaysOn;
    using DataGlobals::SysSizingCalc;
    using DataHVACGlobals::SmallAirVolFlow;
    using DataHVACGlobals::SmallMassFlow;
    using namespace DataSizing;
    using namespace ScheduleManager;

    // MODULE PARAMETER DEFINITIONS
    int const DualDuct_ConstantVolume(1);
    int const DualDuct_VariableVolume(2);
    int const DualDuct_OutdoorAir(3);
    std::string const cCMO_DDConstantVolume("AirTerminal:DualDuct:ConstantVolume");
    std::string const cCMO_DDVariableVolume("AirTerminal:DualDuct:VAV");
    std::string const cCMO_DDVarVolOA("AirTerminal:DualDuct:VAV:OutdoorAir");

    int const DD_OA_ConstantOAMode(11);
    int const DD_OA_ScheduleOAMode(12);
    int const DD_OA_DynamicOAMode(13);

    int const PerPersonModeNotSet(20);
    int const PerPersonDCVByCurrentLevel(21);
    int const PerPersonByDesignLevel(22);

    static std::string const BlankString;

    // MODULE VARIABLE DECLARATIONS:
    Array1D_bool CheckEquipName;

    int NumDampers(0); // The Number of Dampers found in the Input //Autodesk Poss used uninitialized in ReportDualDuctConnections
    int NumDualDuctConstVolDampers;
    int NumDualDuctVarVolDampers;
    int NumDualDuctVarVolOA;
    Real64 MassFlowSetToler;
    bool GetDualDuctInputFlag(true); // Flag set to make sure you get input once

    // Object Data
    Array1D<DualDuctAirTerminal> dd_airterminal;
    std::unordered_map<std::string, std::string> UniqueDualDuctAirTerminalNames;
    Array1D<DualDuctAirTerminalFlowConditions> dd_airterminalInlet;
    Array1D<DualDuctAirTerminalFlowConditions> dd_airterminalHotAirInlet;
    Array1D<DualDuctAirTerminalFlowConditions> dd_airterminalColdAirInlet;
    Array1D<DualDuctAirTerminalFlowConditions> dd_airterminalOutlet;
    Array1D<DualDuctAirTerminalFlowConditions> dd_airterminalOAInlet;        // VAV:OutdoorAir Outdoor Air Inlet
    Array1D<DualDuctAirTerminalFlowConditions> dd_airterminalRecircAirInlet; // VAV:OutdoorAir Recirculated Air Inlet

    void SimulateDualDuct(std::string const &CompName, bool const FirstHVACIteration, int const ZoneNum, int const ZoneNodeNum, int &CompIndex)
    {

        // SUBROUTINE INFORMATION:
        //       AUTHOR         Richard Liesen
        //       DATE WRITTEN   February 2000
        //       MODIFIED       na
        //       RE-ENGINEERED  na

        // PURPOSE OF THIS SUBROUTINE:
        // This subroutine manages Damper component simulation.
        // It is called from the SimAirLoopComponent
        // at the system time step.

        // Using/Aliasing
        using General::TrimSigDigits;

        // SUBROUTINE LOCAL VARIABLE DECLARATIONS:
        int DamperNum; // The Damper that you are currently loading input into

        // FLOW:

        // Obtains and Allocates Damper related parameters from input file
        if (GetDualDuctInputFlag) { // First time subroutine has been entered
            GetDualDuctInput();
            GetDualDuctInputFlag = false;
        }

        // Find the correct DamperNumber with the AirLoop & CompNum from AirLoop Derived Type
        if (CompIndex == 0) {
            DamperNum = UtilityRoutines::FindItemInList(CompName, dd_airterminal, &DualDuctAirTerminal::Name);
            if (DamperNum == 0) {
                ShowFatalError("SimulateDualDuct: Damper not found=" + CompName);
            }
            CompIndex = DamperNum;
        } else {
            DamperNum = CompIndex;
            if (DamperNum > NumDampers || DamperNum < 1) {
                ShowFatalError("SimulateDualDuct: Invalid CompIndex passed=" + TrimSigDigits(CompIndex) +
                               ", Number of Dampers=" + TrimSigDigits(NumDampers) + ", Damper name=" + CompName);
            }
            if (CheckEquipName(DamperNum)) {
                if (CompName != dd_airterminal(DamperNum).Name) {
                    ShowFatalError("SimulateDualDuct: Invalid CompIndex passed=" + TrimSigDigits(CompIndex) + ", Damper name=" + CompName +
                                   ", stored Damper Name for that index=" + dd_airterminal(DamperNum).Name);
                }
                CheckEquipName(DamperNum) = false;
            }
        }

        auto &thisDualDuct( dd_airterminal(DamperNum));

        if (CompIndex > 0) {
            DataSizing::CurTermUnitSizingNum = DataDefineEquip::AirDistUnit(thisDualDuct.ADUNum).TermUnitSizingNum;
            // With the correct DamperNum Initialize
            thisDualDuct.InitDualDuct(DamperNum, FirstHVACIteration); // Initialize all Damper related parameters

            // Calculate the Correct Damper Model with the current DamperNum
            {
                auto const SELECT_CASE_var(thisDualDuct.DamperType);

                if (SELECT_CASE_var == DualDuct_ConstantVolume) { // 'AirTerminal:DualDuct:ConstantVolume'

                    thisDualDuct.SimDualDuctConstVol(DamperNum, ZoneNum, ZoneNodeNum);

                } else if (SELECT_CASE_var == DualDuct_VariableVolume) { // 'AirTerminal:DualDuct:VAV'

                    thisDualDuct.SimDualDuctVarVol(DamperNum, ZoneNum, ZoneNodeNum);

                } else if (SELECT_CASE_var == DualDuct_OutdoorAir) {

                    thisDualDuct.SimDualDuctVAVOutdoorAir(DamperNum, ZoneNum, ZoneNodeNum); // 'AirTerminal:DualDuct:VAV:OutdoorAir'
                }
            }

            // Update the current Damper to the outlet nodes
            thisDualDuct.UpdateDualDuct(DamperNum);

            // Report the current Damper
            thisDualDuct.ReportDualDuct(DamperNum);
        } else {
            ShowFatalError("SimulateDualDuct: Damper not found=" + CompName);
        }
    }

    // Get Input Section of the Module
    //******************************************************************************

    void GetDualDuctInput()
    {

        // SUBROUTINE INFORMATION:
        //       AUTHOR         Richard Liesen
        //       DATE WRITTEN   April 1998
        //       MODIFIED       Julien Marrec of EffiBEM, 2017-12-18
        //       RE-ENGINEERED  na

        // PURPOSE OF THIS SUBROUTINE:
        // This subroutine is the main routine to call other input routines and Get routines

        // METHODOLOGY EMPLOYED:
        // Uses the status flags to trigger events.

        // Using/Aliasing
        using BranchNodeConnections::TestCompSet;
        using DataDefineEquip::AirDistUnit;
        using DataDefineEquip::NumAirDistUnits;
        using DataZoneEquipment::ZoneEquipConfig;
        using NodeInputManager::GetOnlySingleNode;
        using namespace DataIPShortCuts;
        using namespace DataHeatBalance;
        using General::RoundSigDigits;
        using ReportSizingManager::ReportSizingOutput;

        // SUBROUTINE PARAMETER DEFINITIONS:
        static std::string const RoutineName("GetDualDuctInput: "); // include trailing bla

        // SUBROUTINE LOCAL VARIABLE DECLARATIONS:

        int DamperNum;   // The Damper that you are currently loading input into
        int DamperIndex; // Loop index to Damper that you are currently loading input into
        int NumAlphas;
        int NumNums;
        int IOStat;
        static Array1D<Real64> NumArray(2, 0.0);
        static Array1D_string AlphArray(7);
        static Array1D_string cAlphaFields(7);       // Alpha field names
        static Array1D_string cNumericFields(2);     // Numeric field names
        static Array1D_bool lAlphaBlanks(7, true);   // Logical array, alpha field input BLANK = .TRUE.
        static Array1D_bool lNumericBlanks(2, true); // Logical array, numeric field input BLANK = .TRUE.
        std::string CurrentModuleObject;             // for ease in getting objects
        static bool ErrorsFound(false);              // If errors detected in input
        int CtrlZone;                                // controlled zone do loop index
        int SupAirIn;                                // controlled zone supply air inlet index
        int ADUNum;                                  // loop control to search Air Distribution Units
        static Real64 DummyOAFlow(0.0);

        // Flow
        NumDualDuctConstVolDampers = inputProcessor->getNumObjectsFound(cCMO_DDConstantVolume);
        NumDualDuctVarVolDampers = inputProcessor->getNumObjectsFound(cCMO_DDVariableVolume);
        NumDualDuctVarVolOA = inputProcessor->getNumObjectsFound(cCMO_DDVarVolOA);
        NumDampers = NumDualDuctConstVolDampers + NumDualDuctVarVolDampers + NumDualDuctVarVolOA;
        dd_airterminal.allocate(NumDampers);
        UniqueDualDuctAirTerminalNames.reserve(NumDampers);
        CheckEquipName.dimension(NumDampers, true);

        dd_airterminalInlet.allocate(NumDampers);
        dd_airterminalHotAirInlet.allocate(NumDampers);
        dd_airterminalColdAirInlet.allocate(NumDampers);
        dd_airterminalOutlet.allocate(NumDampers);

        dd_airterminalOAInlet.allocate(NumDampers);
        dd_airterminalRecircAirInlet.allocate(NumDampers);

        if (NumDualDuctConstVolDampers > 0) {
            for (DamperIndex = 1; DamperIndex <= NumDualDuctConstVolDampers; ++DamperIndex) {

                // Load the info from the damper
                CurrentModuleObject = cCMO_DDConstantVolume;

                inputProcessor->getObjectItem(CurrentModuleObject,
                                              DamperIndex,
                                              AlphArray,
                                              NumAlphas,
                                              NumArray,
                                              NumNums,
                                              IOStat,
                                              lNumericBlanks,
                                              lAlphaBlanks,
                                              cAlphaFields,
                                              cNumericFields);

                // Anything below this line in this control block should use DamperNum
                DamperNum = DamperIndex;
                GlobalNames::VerifyUniqueInterObjectName(UniqueDualDuctAirTerminalNames, AlphArray(1), CurrentModuleObject, cAlphaFields(1), ErrorsFound);
                 dd_airterminal(DamperNum).Name = AlphArray(1);
                 dd_airterminal(DamperNum).DamperType = DualDuct_ConstantVolume;
                 dd_airterminal(DamperNum).Schedule = AlphArray(2);
                if (lAlphaBlanks(2)) {
                    dd_airterminal(DamperNum).SchedPtr = ScheduleAlwaysOn;
                } else {
                    dd_airterminal(DamperNum).SchedPtr = GetScheduleIndex(AlphArray(2));
                    if ( dd_airterminal(DamperNum).SchedPtr == 0) {
                        ShowSevereError(CurrentModuleObject + ", \"" +  dd_airterminal(DamperNum).Name + "\" " + cAlphaFields(2) + " = " + AlphArray(2) +
                                        " not found.");
                        ErrorsFound = true;
                    }
                }
                 dd_airterminal(DamperNum).OutletNodeNum = GetOnlySingleNode(AlphArray(3),
                                                                    ErrorsFound,
                                                                    CurrentModuleObject,
                                                                    AlphArray(1),
                                                                    NodeType_Air,
                                                                    NodeConnectionType_Outlet,
                                                                    1,
                                                                    ObjectIsNotParent,
                                                                    cAlphaFields(3));
                 dd_airterminal(DamperNum).HotAirInletNodeNum = GetOnlySingleNode(AlphArray(4),
                                                                         ErrorsFound,
                                                                         CurrentModuleObject,
                                                                         AlphArray(1),
                                                                         NodeType_Air,
                                                                         NodeConnectionType_Inlet,
                                                                         1,
                                                                         ObjectIsNotParent,
                                                                         cAlphaFields(4));
                 dd_airterminal(DamperNum).ColdAirInletNodeNum = GetOnlySingleNode(AlphArray(5),
                                                                          ErrorsFound,
                                                                          CurrentModuleObject,
                                                                          AlphArray(1),
                                                                          NodeType_Air,
                                                                          NodeConnectionType_Inlet,
                                                                          1,
                                                                          ObjectIsNotParent,
                                                                          cAlphaFields(5));

                 dd_airterminal(DamperNum).MaxAirVolFlowRate = NumArray(1);
                 dd_airterminal(DamperNum).ZoneMinAirFrac = 0.0;

                // Register component set data - one for heat and one for cool
                TestCompSet(CurrentModuleObject + ":HEAT",  dd_airterminal(DamperNum).Name, AlphArray(4), AlphArray(3), "Air Nodes");
                TestCompSet(CurrentModuleObject + ":COOL",  dd_airterminal(DamperNum).Name, AlphArray(5), AlphArray(3), "Air Nodes");

                for (ADUNum = 1; ADUNum <= NumAirDistUnits; ++ADUNum) {
                    if ( dd_airterminal(DamperNum).OutletNodeNum == AirDistUnit(ADUNum).OutletNodeNum) {
                        AirDistUnit(ADUNum).InletNodeNum =  dd_airterminal(DamperNum).ColdAirInletNodeNum;
                        AirDistUnit(ADUNum).InletNodeNum2 =  dd_airterminal(DamperNum).HotAirInletNodeNum;
                        dd_airterminal(DamperNum).ADUNum = ADUNum;
                    }
                }
                // one assumes if there isn't one assigned, it's an error?
                if ( dd_airterminal(DamperNum).ADUNum == 0) {
                    // convenient String
                    if ( dd_airterminal(DamperNum).DamperType == DualDuct_ConstantVolume) {
                        CurrentModuleObject = "ConstantVolume";
                    } else if ( dd_airterminal(DamperNum).DamperType == DualDuct_VariableVolume) {
                        CurrentModuleObject = "VAV";
                    } else if ( dd_airterminal(DamperNum).DamperType == DualDuct_OutdoorAir) {
                        CurrentModuleObject = "VAV:OutdoorAir";
                    } else {
                        CurrentModuleObject = "*invalid*";
                    }
                    ShowSevereError(RoutineName + "No matching List:Zone:AirTerminal for AirTerminal:DualDuct = [" + CurrentModuleObject + ',' +
                                     dd_airterminal(DamperNum).Name + "].");
                    ShowContinueError("...should have outlet node=" + NodeID( dd_airterminal(DamperNum).OutletNodeNum));
                    ErrorsFound = true;
                } else {

                    // Fill the Zone Equipment data with the inlet node numbers of this unit.
                    for (CtrlZone = 1; CtrlZone <= NumOfZones; ++CtrlZone) {
                        if (!ZoneEquipConfig(CtrlZone).IsControlled) continue;
                        for (SupAirIn = 1; SupAirIn <= ZoneEquipConfig(CtrlZone).NumInletNodes; ++SupAirIn) {
                            if ( dd_airterminal(DamperNum).OutletNodeNum == ZoneEquipConfig(CtrlZone).InletNode(SupAirIn)) {
                                if (ZoneEquipConfig(CtrlZone).AirDistUnitCool(SupAirIn).OutNode > 0) {
                                    ShowSevereError("Error in connecting a terminal unit to a zone");
                                    ShowContinueError(NodeID( dd_airterminal(DamperNum).OutletNodeNum) + " already connects to another zone");
                                    ShowContinueError("Occurs for terminal unit " + CurrentModuleObject + " = " +  dd_airterminal(DamperNum).Name);
                                    ShowContinueError("Check terminal unit node names for errors");
                                    ErrorsFound = true;
                                } else {
                                    ZoneEquipConfig(CtrlZone).AirDistUnitCool(SupAirIn).InNode =  dd_airterminal(DamperNum).ColdAirInletNodeNum;
                                    ZoneEquipConfig(CtrlZone).AirDistUnitHeat(SupAirIn).InNode =  dd_airterminal(DamperNum).HotAirInletNodeNum;
                                    ZoneEquipConfig(CtrlZone).AirDistUnitCool(SupAirIn).OutNode =  dd_airterminal(DamperNum).OutletNodeNum;
                                    ZoneEquipConfig(CtrlZone).AirDistUnitHeat(SupAirIn).OutNode =  dd_airterminal(DamperNum).OutletNodeNum;
                                    AirDistUnit( dd_airterminal(DamperNum).ADUNum).TermUnitSizingNum =
                                        ZoneEquipConfig(CtrlZone).AirDistUnitCool(SupAirIn).TermUnitSizingIndex;
                                    AirDistUnit( dd_airterminal(DamperNum).ADUNum).ZoneEqNum = CtrlZone;
                                }
                                 dd_airterminal(DamperNum).CtrlZoneNum = CtrlZone;
                                 dd_airterminal(DamperNum).ActualZoneNum = ZoneEquipConfig(CtrlZone).ActualZoneNum;
                                 dd_airterminal(DamperNum).CtrlZoneInNodeIndex = SupAirIn;
                            }
                        }
                    }
                }
                // Setup the Average damper Position output variable
                // CurrentModuleObject='AirTerminal:DualDuct:ConstantVolume'
                SetupOutputVariable("Zone Air Terminal Cold Supply Duct Damper Position",
                                    OutputProcessor::Unit::None,
                                     dd_airterminal(DamperNum).ColdAirDamperPosition,
                                    "System",
                                    "Average",
                                     dd_airterminal(DamperNum).Name);
                SetupOutputVariable("Zone Air Terminal Hot Supply Duct Damper Position",
                                    OutputProcessor::Unit::None,
                                     dd_airterminal(DamperNum).HotAirDamperPosition,
                                    "System",
                                    "Average",
                                     dd_airterminal(DamperNum).Name);

            } // end Number of Damper Loop
        }

        if (NumDualDuctVarVolDampers > 0) {
            for (DamperIndex = 1; DamperIndex <= NumDualDuctVarVolDampers; ++DamperIndex) {

                // Load the info from the damper
                CurrentModuleObject = cCMO_DDVariableVolume;

                inputProcessor->getObjectItem(CurrentModuleObject,
                                              DamperIndex,
                                              AlphArray,
                                              NumAlphas,
                                              NumArray,
                                              NumNums,
                                              IOStat,
                                              lNumericBlanks,
                                              lAlphaBlanks,
                                              cAlphaFields,
                                              cNumericFields);

                // Anything below this line in this control block should use DamperNum
                DamperNum = DamperIndex + NumDualDuctConstVolDampers;
                GlobalNames::VerifyUniqueInterObjectName(UniqueDualDuctAirTerminalNames, AlphArray(1), CurrentModuleObject, cAlphaFields(1), ErrorsFound);
                 dd_airterminal(DamperNum).Name = AlphArray(1);
                 dd_airterminal(DamperNum).DamperType = DualDuct_VariableVolume;
                 dd_airterminal(DamperNum).Schedule = AlphArray(2);
                if (lAlphaBlanks(2)) {
                     dd_airterminal(DamperNum).SchedPtr = ScheduleAlwaysOn;
                } else {
                     dd_airterminal(DamperNum).SchedPtr = GetScheduleIndex(AlphArray(2));
                    if ( dd_airterminal(DamperNum).SchedPtr == 0) {
                        ShowSevereError(CurrentModuleObject + ", \"" +  dd_airterminal(DamperNum).Name + "\" " + cAlphaFields(2) + " = " + AlphArray(2) +
                                        " not found.");
                        ErrorsFound = true;
                    }
                }
                 dd_airterminal(DamperNum).OutletNodeNum = GetOnlySingleNode(AlphArray(3),
                                                                    ErrorsFound,
                                                                    CurrentModuleObject,
                                                                    AlphArray(1),
                                                                    NodeType_Air,
                                                                    NodeConnectionType_Outlet,
                                                                    1,
                                                                    ObjectIsNotParent,
                                                                    cAlphaFields(3));
                 dd_airterminal(DamperNum).HotAirInletNodeNum = GetOnlySingleNode(AlphArray(4),
                                                                         ErrorsFound,
                                                                         CurrentModuleObject,
                                                                         AlphArray(1),
                                                                         NodeType_Air,
                                                                         NodeConnectionType_Inlet,
                                                                         1,
                                                                         ObjectIsNotParent,
                                                                         cAlphaFields(4));
                 dd_airterminal(DamperNum).ColdAirInletNodeNum = GetOnlySingleNode(AlphArray(5),
                                                                          ErrorsFound,
                                                                          CurrentModuleObject,
                                                                          AlphArray(1),
                                                                          NodeType_Air,
                                                                          NodeConnectionType_Inlet,
                                                                          1,
                                                                          ObjectIsNotParent,
                                                                          cAlphaFields(5));

                 dd_airterminal(DamperNum).MaxAirVolFlowRate = NumArray(1);
                 dd_airterminal(DamperNum).ZoneMinAirFrac = NumArray(2);

                // Register component set data - one for heat and one for cool
                TestCompSet(CurrentModuleObject + ":HEAT",  dd_airterminal(DamperNum).Name, AlphArray(4), AlphArray(3), "Air Nodes");
                TestCompSet(CurrentModuleObject + ":COOL",  dd_airterminal(DamperNum).Name, AlphArray(5), AlphArray(3), "Air Nodes");

                for (ADUNum = 1; ADUNum <= NumAirDistUnits; ++ADUNum) {
                    if ( dd_airterminal(DamperNum).OutletNodeNum == AirDistUnit(ADUNum).OutletNodeNum) {
                        AirDistUnit(ADUNum).InletNodeNum =  dd_airterminal(DamperNum).ColdAirInletNodeNum;
                        AirDistUnit(ADUNum).InletNodeNum2 =  dd_airterminal(DamperNum).HotAirInletNodeNum;
                         dd_airterminal(DamperNum).ADUNum = ADUNum;
                    }
                }
                // one assumes if there isn't one assigned, it's an error?
                if ( dd_airterminal(DamperNum).ADUNum == 0) {
                    // convenient String
                    if ( dd_airterminal(DamperNum).DamperType == DualDuct_ConstantVolume) {
                        CurrentModuleObject = "ConstantVolume";
                    } else if ( dd_airterminal(DamperNum).DamperType == DualDuct_VariableVolume) {
                        CurrentModuleObject = "VAV";
                    } else if ( dd_airterminal(DamperNum).DamperType == DualDuct_OutdoorAir) {
                        CurrentModuleObject = "VAV:OutdoorAir";
                    } else {
                        CurrentModuleObject = "*invalid*";
                    }
                    ShowSevereError(RoutineName + "No matching List:Zone:AirTerminal for AirTerminal:DualDuct = [" + CurrentModuleObject + ',' +
                                     dd_airterminal(DamperNum).Name + "].");
                    ShowContinueError("...should have outlet node=" + NodeID( dd_airterminal(DamperNum).OutletNodeNum));
                    ErrorsFound = true;
                } else {

                    // Fill the Zone Equipment data with the inlet node numbers of this unit.
                    for (CtrlZone = 1; CtrlZone <= NumOfZones; ++CtrlZone) {
                        if (!ZoneEquipConfig(CtrlZone).IsControlled) continue;
                        for (SupAirIn = 1; SupAirIn <= ZoneEquipConfig(CtrlZone).NumInletNodes; ++SupAirIn) {
                            if ( dd_airterminal(DamperNum).OutletNodeNum == ZoneEquipConfig(CtrlZone).InletNode(SupAirIn)) {
                                ZoneEquipConfig(CtrlZone).AirDistUnitCool(SupAirIn).InNode =  dd_airterminal(DamperNum).ColdAirInletNodeNum;
                                ZoneEquipConfig(CtrlZone).AirDistUnitHeat(SupAirIn).InNode =  dd_airterminal(DamperNum).HotAirInletNodeNum;
                                ZoneEquipConfig(CtrlZone).AirDistUnitCool(SupAirIn).OutNode =  dd_airterminal(DamperNum).OutletNodeNum;
                                ZoneEquipConfig(CtrlZone).AirDistUnitHeat(SupAirIn).OutNode =  dd_airterminal(DamperNum).OutletNodeNum;
                                AirDistUnit( dd_airterminal(DamperNum).ADUNum).TermUnitSizingNum =
                                    ZoneEquipConfig(CtrlZone).AirDistUnitCool(SupAirIn).TermUnitSizingIndex;
                                AirDistUnit( dd_airterminal(DamperNum).ADUNum).ZoneEqNum = CtrlZone;

                                dd_airterminal(DamperNum).CtrlZoneNum = CtrlZone;
                                dd_airterminal(DamperNum).ActualZoneNum = ZoneEquipConfig(CtrlZone).ActualZoneNum;
                                dd_airterminal(DamperNum).CtrlZoneInNodeIndex = SupAirIn;
                            }
                        }
                    }
                }
                if (!lAlphaBlanks(6)) {
                     dd_airterminal(DamperNum).OARequirementsPtr = UtilityRoutines::FindItemInList(AlphArray(6), OARequirements);
                    if ( dd_airterminal(DamperNum).OARequirementsPtr == 0) {
                        ShowSevereError(cAlphaFields(6) + " = " + AlphArray(6) + " not found.");
                        ShowContinueError("Occurs in " + cCMO_DDVariableVolume + " = " + dd_airterminal(DamperNum).Name);
                        ErrorsFound = true;
                    } else {
                         dd_airterminal(DamperNum).NoOAFlowInputFromUser = false;
                    }
                }

                // Setup the Average damper Position output variable
                // CurrentModuleObject='AirTerminal:DualDuct:VAV'
                SetupOutputVariable("Zone Air Terminal Cold Supply Duct Damper Position",
                                    OutputProcessor::Unit::None,
                                     dd_airterminal(DamperNum).ColdAirDamperPosition,
                                    "System",
                                    "Average",
                                     dd_airterminal(DamperNum).Name);
                SetupOutputVariable("Zone Air Terminal Hot Supply Duct Damper Position",
                                    OutputProcessor::Unit::None,
                                     dd_airterminal(DamperNum).HotAirDamperPosition,
                                    "System",
                                    "Average",
                                     dd_airterminal(DamperNum).Name);
                SetupOutputVariable("Zone Air Terminal Outdoor Air Volume Flow Rate",
                                    OutputProcessor::Unit::m3_s,
                                    dd_airterminal(DamperNum).OutdoorAirFlowRate,
                                    "System",
                                    "Average",
                                    dd_airterminal(DamperNum).Name);
            } // end Number of Damper Loop
        }

        if (NumDualDuctVarVolOA > 0) {
            for (DamperIndex = 1; DamperIndex <= NumDualDuctVarVolOA; ++DamperIndex) {

                // Load the info from the damper
                CurrentModuleObject = cCMO_DDVarVolOA;

                inputProcessor->getObjectItem(CurrentModuleObject,
                                              DamperIndex,
                                              AlphArray,
                                              NumAlphas,
                                              NumArray,
                                              NumNums,
                                              IOStat,
                                              lNumericBlanks,
                                              lAlphaBlanks,
                                              cAlphaFields,
                                              cNumericFields);

                // Anything below this line in this control block should use DamperNum
                DamperNum = DamperIndex + NumDualDuctConstVolDampers + NumDualDuctVarVolDampers;
                GlobalNames::VerifyUniqueInterObjectName(UniqueDualDuctAirTerminalNames, AlphArray(1), CurrentModuleObject, cAlphaFields(1), ErrorsFound);
                 dd_airterminal(DamperNum).Name = AlphArray(1);
                 dd_airterminal(DamperNum).DamperType = DualDuct_OutdoorAir;
                 dd_airterminal(DamperNum).Schedule = AlphArray(2);
                if (lAlphaBlanks(2)) {
                     dd_airterminal(DamperNum).SchedPtr = ScheduleAlwaysOn;
                } else {
                     dd_airterminal(DamperNum).SchedPtr = GetScheduleIndex(AlphArray(2));
                    if ( dd_airterminal(DamperNum).SchedPtr == 0) {
                        ShowSevereError(CurrentModuleObject + ", \"" +  dd_airterminal(DamperNum).Name + "\" " + cAlphaFields(2) + " = " + AlphArray(2) +
                                        " not found.");
                        ErrorsFound = true;
                    }
                }
                 dd_airterminal(DamperNum).OutletNodeNum = GetOnlySingleNode(AlphArray(3),
                                                                    ErrorsFound,
                                                                    CurrentModuleObject,
                                                                    AlphArray(1),
                                                                    NodeType_Air,
                                                                    NodeConnectionType_Outlet,
                                                                    1,
                                                                    ObjectIsNotParent,
                                                                    cAlphaFields(3));
                 dd_airterminal(DamperNum).OAInletNodeNum = GetOnlySingleNode(AlphArray(4),
                                                                     ErrorsFound,
                                                                     CurrentModuleObject,
                                                                     AlphArray(1),
                                                                     NodeType_Air,
                                                                     NodeConnectionType_Inlet,
                                                                     1,
                                                                     ObjectIsNotParent,
                                                                     cAlphaFields(4));

                if (!lAlphaBlanks(5)) {
                     dd_airterminal(DamperNum).RecircAirInletNodeNum = GetOnlySingleNode(AlphArray(5),
                                                                                ErrorsFound,
                                                                                CurrentModuleObject,
                                                                                AlphArray(1),
                                                                                NodeType_Air,
                                                                                NodeConnectionType_Inlet,
                                                                                1,
                                                                                ObjectIsNotParent,
                                                                                cAlphaFields(5));
                } else {
                    // for this model, we intentionally allow not using the recirc side
                     dd_airterminal(DamperNum).RecircIsUsed = false;
                }

                 dd_airterminal(DamperNum).MaxAirVolFlowRate = NumArray(1);
                 dd_airterminal(DamperNum).MaxAirMassFlowRate =  dd_airterminal(DamperNum).MaxAirVolFlowRate * StdRhoAir;

                // Register component set data - one for OA and one for RA
                TestCompSet(CurrentModuleObject + ":OutdoorAir",  dd_airterminal(DamperNum).Name, AlphArray(4), AlphArray(3), "Air Nodes");
                if (  dd_airterminal(DamperNum).RecircIsUsed) {
                    TestCompSet(CurrentModuleObject + ":RecirculatedAir",  dd_airterminal(DamperNum).Name, AlphArray(5), AlphArray(3), "Air Nodes");
                }

                {
                    auto const SELECT_CASE_var(AlphArray(7));
                    if (SELECT_CASE_var == "CURRENTOCCUPANCY") {
                         dd_airterminal(DamperNum).OAPerPersonMode = PerPersonDCVByCurrentLevel;

                    } else if (SELECT_CASE_var == "DESIGNOCCUPANCY") {
                         dd_airterminal(DamperNum).OAPerPersonMode = PerPersonByDesignLevel;
                    }
                }
                // checks on this are done later

                for (ADUNum = 1; ADUNum <= NumAirDistUnits; ++ADUNum) {
                    if (  dd_airterminal(DamperNum).OutletNodeNum == AirDistUnit(ADUNum).OutletNodeNum) {
                        AirDistUnit(ADUNum).InletNodeNum =  dd_airterminal(DamperNum).OAInletNodeNum;
                        AirDistUnit(ADUNum).InletNodeNum2 =  dd_airterminal(DamperNum).RecircAirInletNodeNum;
                         dd_airterminal(DamperNum).ADUNum = ADUNum;
                    }
                }
                // one assumes if there isn't one assigned, it's an error?
                if (  dd_airterminal(DamperNum).ADUNum == 0) {
                    // convenient String
                    if (  dd_airterminal(DamperNum).DamperType == DualDuct_ConstantVolume) {
                        CurrentModuleObject = "ConstantVolume";
                    } else if (  dd_airterminal(DamperNum).DamperType == DualDuct_VariableVolume) {
                        CurrentModuleObject = "VAV";
                    } else if (  dd_airterminal(DamperNum).DamperType == DualDuct_OutdoorAir) {
                        CurrentModuleObject = "VAV:OutdoorAir";
                    } else {
                        CurrentModuleObject = "*invalid*";
                    }
                    ShowSevereError(RoutineName + "No matching List:Zone:AirTerminal for AirTerminal:DualDuct = [" + CurrentModuleObject + ',' +
                                     dd_airterminal(DamperNum).Name + "].");
                    ShowContinueError("...should have outlet node=" + NodeID(  dd_airterminal(DamperNum).OutletNodeNum));
                    ErrorsFound = true;
                } else {

                    // Fill the Zone Equipment data with the inlet node numbers of this unit.
                    for (CtrlZone = 1; CtrlZone <= NumOfZones; ++CtrlZone) {
                        if (!ZoneEquipConfig(CtrlZone).IsControlled) continue;
                        for (SupAirIn = 1; SupAirIn <= ZoneEquipConfig(CtrlZone).NumInletNodes; ++SupAirIn) {
                            if (  dd_airterminal(DamperNum).OutletNodeNum == ZoneEquipConfig(CtrlZone).InletNode(SupAirIn)) {
                                if (  dd_airterminal(DamperNum).RecircIsUsed) {
                                    ZoneEquipConfig(CtrlZone).AirDistUnitCool(SupAirIn).InNode =  dd_airterminal(DamperNum).RecircAirInletNodeNum;
                                } else {
                                    ZoneEquipConfig(CtrlZone).AirDistUnitCool(SupAirIn).InNode =  dd_airterminal(DamperNum).OAInletNodeNum;
                                }
                                ZoneEquipConfig(CtrlZone).AirDistUnitHeat(SupAirIn).InNode =  dd_airterminal(DamperNum).OAInletNodeNum;
                                ZoneEquipConfig(CtrlZone).AirDistUnitCool(SupAirIn).OutNode =  dd_airterminal(DamperNum).OutletNodeNum;
                                ZoneEquipConfig(CtrlZone).AirDistUnitHeat(SupAirIn).OutNode =  dd_airterminal(DamperNum).OutletNodeNum;
                                AirDistUnit(  dd_airterminal(DamperNum).ADUNum).TermUnitSizingNum =
                                    ZoneEquipConfig(CtrlZone).AirDistUnitCool(SupAirIn).TermUnitSizingIndex;
                                AirDistUnit(  dd_airterminal(DamperNum).ADUNum).ZoneEqNum = CtrlZone;

                                 dd_airterminal(DamperNum).CtrlZoneNum = CtrlZone;
                                 dd_airterminal(DamperNum).ActualZoneNum = ZoneEquipConfig(CtrlZone).ActualZoneNum;
                                 dd_airterminal(DamperNum).CtrlZoneInNodeIndex = SupAirIn;
                            }
                        }
                    }
                }
                 dd_airterminal(DamperNum).OARequirementsPtr = UtilityRoutines::FindItemInList(AlphArray(6), OARequirements);
                if (  dd_airterminal(DamperNum).OARequirementsPtr == 0) {
                    ShowSevereError(cAlphaFields(6) + " = " + AlphArray(6) + " not found.");
                    ShowContinueError("Occurs in " + cCMO_DDVarVolOA + " = " +  dd_airterminal(DamperNum).Name);
                    ErrorsFound = true;
                } else {
                     dd_airterminal(DamperNum).NoOAFlowInputFromUser = false;

                    // now fill design OA rate
                     dd_airterminal(DamperNum).CalcOAOnlyMassFlow(DamperNum, DummyOAFlow,  dd_airterminal(DamperNum).DesignOAFlowRate);

                    if (  dd_airterminal(DamperNum).MaxAirVolFlowRate != AutoSize) {
                        ReportSizingOutput(CurrentModuleObject,
                                            dd_airterminal(DamperNum).Name,
                                           "Maximum Outdoor Air Flow Rate [m3/s]",
                                            dd_airterminal(DamperNum).DesignOAFlowRate);

                        if (  dd_airterminal(DamperNum).RecircIsUsed) {
                             dd_airterminal(DamperNum).DesignRecircFlowRate =  dd_airterminal(DamperNum).MaxAirVolFlowRate -  dd_airterminal(DamperNum).DesignOAFlowRate;
                             dd_airterminal(DamperNum).DesignRecircFlowRate = max(0.0,  dd_airterminal(DamperNum).DesignRecircFlowRate);
                            ReportSizingOutput(CurrentModuleObject,
                                                dd_airterminal(DamperNum).Name,
                                               "Maximum Recirculated Air Flow Rate [m3/s]",
                                                dd_airterminal(DamperNum).DesignRecircFlowRate);
                        } else {
                            if (  dd_airterminal(DamperNum).MaxAirVolFlowRate <  dd_airterminal(DamperNum).DesignOAFlowRate) {
                                ShowSevereError("The value " + RoundSigDigits(  dd_airterminal(DamperNum).MaxAirVolFlowRate, 5) + " in " + cNumericFields(1) +
                                                "is lower than the outdoor air requirement.");
                                ShowContinueError("Occurs in " + cCMO_DDVarVolOA + " = " +  dd_airterminal(DamperNum).Name);
                                ShowContinueError("The design outdoor air requirement is " + RoundSigDigits(  dd_airterminal(DamperNum).DesignOAFlowRate, 5));
                                ErrorsFound = true;
                            }
                        }
                    }
                }

                if (  dd_airterminal(DamperNum).OAPerPersonMode == PerPersonModeNotSet) {
                    DummyOAFlow = OARequirements(  dd_airterminal(DamperNum).OARequirementsPtr).OAFlowPerPerson;
                    if ((DummyOAFlow == 0.0) && (lAlphaBlanks(7))) {       // no worries
                                                                           // do nothing, okay since no per person requirement involved
                    } else if ((DummyOAFlow > 0.0) && (lAlphaBlanks(7))) { // missing input
                        ShowSevereError(cAlphaFields(7) + " was blank.");
                        ShowContinueError("Occurs in " + cCMO_DDVarVolOA + " = " +  dd_airterminal(DamperNum).Name);
                        ShowContinueError("Valid choices are \"CurrentOccupancy\" or \"DesignOccupancy\"");
                        ErrorsFound = true;
                    } else if ((DummyOAFlow > 0.0) && !(lAlphaBlanks(7))) { // incorrect input
                        ShowSevereError(cAlphaFields(7) + " = " + AlphArray(7) + " not a valid key choice.");
                        ShowContinueError("Occurs in " + cCMO_DDVarVolOA + " = " +  dd_airterminal(DamperNum).Name);
                        ShowContinueError("Valid choices are \"CurrentOccupancy\" or \"DesignOccupancy\"");
                        ErrorsFound = true;
                    }
                }

                // Setup the Average damper Position output variable
                SetupOutputVariable("Zone Air Terminal Outdoor Air Duct Damper Position",
                                    OutputProcessor::Unit::None,
                                     dd_airterminal(DamperNum).OADamperPosition,
                                    "System",
                                    "Average",
                                     dd_airterminal(DamperNum).Name);
                SetupOutputVariable("Zone Air Terminal Recirculated Air Duct Damper Position",
                                    OutputProcessor::Unit::None,
                                     dd_airterminal(DamperNum).RecircAirDamperPosition,
                                    "System",
                                    "Average",
                                     dd_airterminal(DamperNum).Name);
                SetupOutputVariable("Zone Air Terminal Outdoor Air Fraction",
                                    OutputProcessor::Unit::None,
                                     dd_airterminal(DamperNum).OAFraction,
                                    "System",
                                    "Average",
                                     dd_airterminal(DamperNum).Name);

            } // end Number of Damper Loop
        }

        if (ErrorsFound) {
            ShowFatalError(RoutineName + "Errors found in input.  Preceding condition(s) cause termination.");
        }
    }

    // End of Get Input subroutines for the Module
    //******************************************************************************

    // Beginning Initialization Section of the Module
    //******************************************************************************

    void DualDuctAirTerminal::InitDualDuct(int const DamperNum, bool const FirstHVACIteration)
    {

        // SUBROUTINE INFORMATION:
        //       AUTHOR         Richard J. Liesen
        //       DATE WRITTEN   February 1998
        //       MODIFIED       na
        //       RE-ENGINEERED  na

        // PURPOSE OF THIS SUBROUTINE:
        // This subroutine is for  initializations of the Damper Components.

        // METHODOLOGY EMPLOYED:
        // Uses the status flags to trigger events.

        // Using/Aliasing
        using DataConvergParams::HVACFlowRateToler;
        using DataDefineEquip::AirDistUnit;
        using DataHeatBalance::People;
        using DataHeatBalance::TotPeople;
        using DataZoneEquipment::CheckZoneEquipmentList;
        using DataZoneEquipment::ZoneEquipConfig;
        using DataZoneEquipment::ZoneEquipInputsFilled;
        using Psychrometrics::PsyRhoAirFnPbTdbW;

        // SUBROUTINE LOCAL VARIABLE DECLARATIONS:
        int HotInNode;
        int ColdInNode;
        int OAInNode; // Outdoor Air Inlet Node for VAV:OutdoorAir units
        int RAInNode; // Reciruclated Air Inlet Node for VAV:OutdoorAir units
        int OutNode;
        static bool MyOneTimeFlag(true);
        static Array1D_bool MyEnvrnFlag;
        static Array1D_bool MySizeFlag;
        static Array1D_bool MyAirLoopFlag;
        static bool ZoneEquipmentListChecked(false); // True after the Zone Equipment List has been checked for items
        int Loop;                                    // Loop checking control variable
        Real64 PeopleFlow;                           // local sum variable, m3/s
        // FLOW:

        // Do the Begin Simulation initializations
        if (MyOneTimeFlag) {

            MyEnvrnFlag.allocate(NumDampers);
            MySizeFlag.allocate(NumDampers);
            MyAirLoopFlag.dimension(NumDampers, true);
            MyEnvrnFlag = true;
            MySizeFlag = true;
            MassFlowSetToler = HVACFlowRateToler * 0.00001;

            MyOneTimeFlag = false;
        }

        if (!ZoneEquipmentListChecked && ZoneEquipInputsFilled) {
            ZoneEquipmentListChecked = true;
            // Check to see if there is a Air Distribution Unit on the Zone Equipment List
            for (Loop = 1; Loop <= NumDampers; ++Loop) {
                if (  dd_airterminal(Loop).ADUNum == 0) continue;
                if (CheckZoneEquipmentList("ZONEHVAC:AIRDISTRIBUTIONUNIT", AirDistUnit(  dd_airterminal(Loop).ADUNum).Name)) continue;
                ShowSevereError("InitDualDuct: ADU=[Air Distribution Unit," + AirDistUnit(  dd_airterminal(Loop).ADUNum).Name +
                                "] is not on any ZoneHVAC:EquipmentList.");
                if (  dd_airterminal(Loop).DamperType == DualDuct_ConstantVolume) {
                    ShowContinueError("...Dual Duct Damper=[" + cCMO_DDConstantVolume + ',' +  dd_airterminal(Loop).Name + "] will not be simulated.");
                } else if (  dd_airterminal(Loop).DamperType == DualDuct_VariableVolume) {
                    ShowContinueError("...Dual Duct Damper=[" + cCMO_DDVariableVolume + ',' +  dd_airterminal(Loop).Name + "] will not be simulated.");
                } else if (  dd_airterminal(Loop).DamperType == DualDuct_OutdoorAir) {
                    ShowContinueError("...Dual Duct Damper=[" + cCMO_DDVarVolOA + ',' +  dd_airterminal(Loop).Name + "] will not be simulated.");
                } else {
                    ShowContinueError("...Dual Duct Damper=[unknown/invalid," +  dd_airterminal(Loop).Name + "] will not be simulated.");
                }
            }
        }

        if (!SysSizingCalc && MySizeFlag(DamperNum)) {

            SizeDualDuct(DamperNum);

            MySizeFlag(DamperNum) = false;
        }

        // Do the Begin Environment initializations
        if (BeginEnvrnFlag && MyEnvrnFlag(DamperNum)) {

            if (  dd_airterminal(DamperNum).DamperType == DualDuct_ConstantVolume ||  dd_airterminal(DamperNum).DamperType == DualDuct_VariableVolume) {
                OutNode =  dd_airterminal(DamperNum).OutletNodeNum;
                HotInNode =  dd_airterminal(DamperNum).HotAirInletNodeNum;
                ColdInNode =  dd_airterminal(DamperNum).ColdAirInletNodeNum;
                Node(OutNode).MassFlowRateMax =  dd_airterminal(DamperNum).MaxAirVolFlowRate * StdRhoAir;
                if (  dd_airterminal(DamperNum).DamperType == DualDuct_ConstantVolume) {
                    Node(OutNode).MassFlowRateMin = 0.0;
                } else if (  dd_airterminal(DamperNum).DamperType == DualDuct_VariableVolume) {
                    Node(OutNode).MassFlowRateMin = Node(OutNode).MassFlowRateMax *  dd_airterminal(DamperNum).ZoneMinAirFrac;
                } else {
                    Node(OutNode).MassFlowRateMin = 0.0;
                }
                dd_airterminalHotAirInlet(DamperNum).AirMassFlowRateMax = Node(OutNode).MassFlowRateMax;
                dd_airterminalColdAirInlet(DamperNum).AirMassFlowRateMax = Node(OutNode).MassFlowRateMax;
                Node(HotInNode).MassFlowRateMax = Node(OutNode).MassFlowRateMax;
                Node(ColdInNode).MassFlowRateMax = Node(OutNode).MassFlowRateMax;
                Node(HotInNode).MassFlowRateMin = 0.0;
                Node(ColdInNode).MassFlowRateMin = 0.0;
                MyEnvrnFlag(DamperNum) = false;

            } else if (  dd_airterminal(DamperNum).DamperType == DualDuct_OutdoorAir) {
                // Initialize for DualDuct:VAV:OutdoorAir
                OutNode =  dd_airterminal(DamperNum).OutletNodeNum;
                OAInNode =  dd_airterminal(DamperNum).OAInletNodeNum;
                if (  dd_airterminal(DamperNum).RecircIsUsed) RAInNode =  dd_airterminal(DamperNum).RecircAirInletNodeNum;
                Node(OutNode).MassFlowRateMax =  dd_airterminal(DamperNum).MaxAirMassFlowRate;
                Node(OutNode).MassFlowRateMin = 0.0;
                dd_airterminalOAInlet(DamperNum).AirMassFlowRateMax =  dd_airterminal(DamperNum).DesignOAFlowRate * StdRhoAir;
                if (  dd_airterminal(DamperNum).RecircIsUsed) {
                    dd_airterminalRecircAirInlet(DamperNum).AirMassFlowRateMax =
                         dd_airterminal(DamperNum).MaxAirMassFlowRate - dd_airterminalOAInlet(DamperNum).AirMassFlowRateMax;
                    Node(RAInNode).MassFlowRateMax = dd_airterminalRecircAirInlet(DamperNum).AirMassFlowRateMax;
                    Node(RAInNode).MassFlowRateMin = 0.0;
                    dd_airterminalRecircAirInlet(DamperNum).AirMassFlowDiffMag = 1.0e-10 * dd_airterminalRecircAirInlet(DamperNum).AirMassFlowRateMax;
                }
                Node(OAInNode).MassFlowRateMax = dd_airterminalOAInlet(DamperNum).AirMassFlowRateMax;
                Node(OAInNode).MassFlowRateMin = 0.0;
                // figure per person by design level for the OA duct.
                PeopleFlow = 0.0;
                for (Loop = 1; Loop <= TotPeople; ++Loop) {
                    if (People(Loop).ZonePtr !=  dd_airterminal(DamperNum).ActualZoneNum) continue;
                    int damperOAFlowMethod = OARequirements(  dd_airterminal(DamperNum).OARequirementsPtr).OAFlowMethod;
                    if (damperOAFlowMethod == OAFlowPPer || damperOAFlowMethod == OAFlowSum || damperOAFlowMethod == OAFlowMax) {
                        PeopleFlow += People(Loop).NumberOfPeople * OARequirements(  dd_airterminal(DamperNum).OARequirementsPtr).OAFlowPerPerson;
                    }
                }
                 dd_airterminal(DamperNum).OAPerPersonByDesignLevel = PeopleFlow;

                MyEnvrnFlag(DamperNum) = false;
            }
        }

        if (!BeginEnvrnFlag) {
            MyEnvrnFlag(DamperNum) = true;
        }

        // Find air loop associated with this terminal unit
        if (MyAirLoopFlag(DamperNum)) {
            if (  dd_airterminal(DamperNum).AirLoopNum == 0) {
                if ((  dd_airterminal(DamperNum).CtrlZoneNum > 0) && (  dd_airterminal(DamperNum).CtrlZoneInNodeIndex > 0)) {
                     dd_airterminal(DamperNum).AirLoopNum =
                        ZoneEquipConfig(  dd_airterminal(DamperNum).CtrlZoneNum).InletNodeAirLoopNum(  dd_airterminal(DamperNum).CtrlZoneInNodeIndex);
                    AirDistUnit(  dd_airterminal(DamperNum).ADUNum).AirLoopNum =  dd_airterminal(DamperNum).AirLoopNum;
                    // Don't set MyAirLoopFlag to false yet because airloopnums might not be populated yet
                }
            } else {
                MyAirLoopFlag(DamperNum) = false;
            }
        }

        // Initialize the Inlet Nodes of the Sys
        if (  dd_airterminal(DamperNum).DamperType == DualDuct_ConstantVolume ||  dd_airterminal(DamperNum).DamperType == DualDuct_VariableVolume) {
            HotInNode =  dd_airterminal(DamperNum).HotAirInletNodeNum;
            ColdInNode =  dd_airterminal(DamperNum).ColdAirInletNodeNum;
            OutNode =  dd_airterminal(DamperNum).OutletNodeNum;
        } else if (  dd_airterminal(DamperNum).DamperType == DualDuct_OutdoorAir) {
            OAInNode =  dd_airterminal(DamperNum).OAInletNodeNum;
            if (  dd_airterminal(DamperNum).RecircIsUsed) RAInNode =  dd_airterminal(DamperNum).RecircAirInletNodeNum;
            OutNode =  dd_airterminal(DamperNum).OutletNodeNum;
        }

        if (FirstHVACIteration) {
            //     CALL DisplayString('Init First HVAC Iteration {'//TRIM(  dd_airterminal(DamperNum)%DamperName)//'}') !-For debugging - REMOVE
            // The first time through set the mass flow rate to the Max
            // Take care of the flow rates first. For Const Vol and VAV.
            if (  dd_airterminal(DamperNum).DamperType == DualDuct_ConstantVolume ||  dd_airterminal(DamperNum).DamperType == DualDuct_VariableVolume) {
                if ((Node(HotInNode).MassFlowRate > 0.0) && (GetCurrentScheduleValue(  dd_airterminal(DamperNum).SchedPtr) > 0.0)) {
                    Node(HotInNode).MassFlowRate = dd_airterminalHotAirInlet(DamperNum).AirMassFlowRateMax;
                } else {
                    Node(HotInNode).MassFlowRate = 0.0;
                }
                if ((Node(ColdInNode).MassFlowRate > 0.0) && (GetCurrentScheduleValue(  dd_airterminal(DamperNum).SchedPtr) > 0.0)) {
                    Node(ColdInNode).MassFlowRate = dd_airterminalColdAirInlet(DamperNum).AirMassFlowRateMax;
                } else {
                    Node(ColdInNode).MassFlowRate = 0.0;
                }
                // Next take care of the Max Avail Flow Rates
                if ((Node(HotInNode).MassFlowRateMaxAvail > 0.0) && (GetCurrentScheduleValue( dd_airterminal(DamperNum).SchedPtr) > 0.0)) {
                    Node(HotInNode).MassFlowRateMaxAvail = dd_airterminalHotAirInlet(DamperNum).AirMassFlowRateMax;
                } else {
                    Node(HotInNode).MassFlowRateMaxAvail = 0.0;
                }
                if ((Node(ColdInNode).MassFlowRateMaxAvail > 0.0) && (GetCurrentScheduleValue( dd_airterminal(DamperNum).SchedPtr) > 0.0)) {
                    Node(ColdInNode).MassFlowRateMaxAvail = dd_airterminalColdAirInlet(DamperNum).AirMassFlowRateMax;
                } else {
                    Node(ColdInNode).MassFlowRateMaxAvail = 0.0;
                }
                // The last item is to take care of the Min Avail Flow Rates
                if ((Node(HotInNode).MassFlowRate > 0.0) && (GetCurrentScheduleValue( dd_airterminal(DamperNum).SchedPtr) > 0.0)) {
                    Node(HotInNode).MassFlowRateMinAvail = dd_airterminalHotAirInlet(DamperNum).AirMassFlowRateMax *  dd_airterminal(DamperNum).ZoneMinAirFrac;
                } else {
                    Node(HotInNode).MassFlowRateMinAvail = 0.0;
                }
                if ((Node(ColdInNode).MassFlowRate > 0.0) && (GetCurrentScheduleValue(  dd_airterminal(DamperNum).SchedPtr) > 0.0)) {
                    Node(ColdInNode).MassFlowRateMinAvail = dd_airterminalColdAirInlet(DamperNum).AirMassFlowRateMax *  dd_airterminal(DamperNum).ZoneMinAirFrac;
                } else {
                    Node(ColdInNode).MassFlowRateMinAvail = 0.0;
                }

            } else if (  dd_airterminal(DamperNum).DamperType == DualDuct_OutdoorAir) {
                // The first time through set the mass flow rate to the Max for VAV:OutdoorAir
                if ((Node(OAInNode).MassFlowRate > 0.0) && (GetCurrentScheduleValue(  dd_airterminal(DamperNum).SchedPtr) > 0.0)) {
                    Node(OAInNode).MassFlowRate = dd_airterminalOAInlet(DamperNum).AirMassFlowRateMax;
                } else {
                    Node(OAInNode).MassFlowRate = 0.0;
                }
                if (  dd_airterminal(DamperNum).RecircIsUsed) {
                    if ((Node(RAInNode).MassFlowRate > 0.0) && (GetCurrentScheduleValue(  dd_airterminal(DamperNum).SchedPtr) > 0.0)) {
                        Node(RAInNode).MassFlowRate = dd_airterminalRecircAirInlet(DamperNum).AirMassFlowRateMax;
                    } else {
                        Node(RAInNode).MassFlowRate = 0.0;
                    }
                    // clear flow history
                    dd_airterminalRecircAirInlet(DamperNum).AirMassFlowRateHist1 = 0.0;
                    dd_airterminalRecircAirInlet(DamperNum).AirMassFlowRateHist2 = 0.0;
                    dd_airterminalRecircAirInlet(DamperNum).AirMassFlowRateHist3 = 0.0;
                }
                // Next take care of the Max Avail Flow Rates
                if ((Node(OAInNode).MassFlowRateMaxAvail > 0.0) && (GetCurrentScheduleValue(  dd_airterminal(DamperNum).SchedPtr) > 0.0)) {
                    Node(OAInNode).MassFlowRateMaxAvail = dd_airterminalOAInlet(DamperNum).AirMassFlowRateMax;
                } else {
                    Node(OAInNode).MassFlowRateMaxAvail = 0.0;
                }
                if (  dd_airterminal(DamperNum).RecircIsUsed) {
                    if ((Node(RAInNode).MassFlowRateMaxAvail > 0.0) && (GetCurrentScheduleValue(  dd_airterminal(DamperNum).SchedPtr) > 0.0)) {
                        Node(RAInNode).MassFlowRateMaxAvail = dd_airterminalRecircAirInlet(DamperNum).AirMassFlowRateMax;
                    } else {
                        Node(RAInNode).MassFlowRateMaxAvail = 0.0;
                    }
                }
                // The last item is to take care of the Min Avail Flow Rates. VAV:OutdoorAir
                Node(OAInNode).MassFlowRateMinAvail = 0.0;
                if (  dd_airterminal(DamperNum).RecircIsUsed) Node(RAInNode).MassFlowRateMinAvail = 0.0;
            }
        }

        // Initialize the Inlet Nodes of the Dampers for Const. Vol and VAV
        if (  dd_airterminal(DamperNum).DamperType == DualDuct_ConstantVolume ||  dd_airterminal(DamperNum).DamperType == DualDuct_VariableVolume) {

            dd_airterminalHotAirInlet(DamperNum).AirMassFlowRateMaxAvail = min(Node(OutNode).MassFlowRateMax, Node(HotInNode).MassFlowRateMaxAvail);
            dd_airterminalHotAirInlet(DamperNum).AirMassFlowRateMinAvail =
                min(max(Node(OutNode).MassFlowRateMin, Node(HotInNode).MassFlowRateMinAvail), Node(HotInNode).MassFlowRateMaxAvail);

            dd_airterminalColdAirInlet(DamperNum).AirMassFlowRateMaxAvail = min(Node(OutNode).MassFlowRateMax, Node(ColdInNode).MassFlowRateMaxAvail);
            dd_airterminalColdAirInlet(DamperNum).AirMassFlowRateMinAvail =
                min(max(Node(OutNode).MassFlowRateMin, Node(ColdInNode).MassFlowRateMinAvail), Node(ColdInNode).MassFlowRateMaxAvail);

            // Do the following initializations (every time step): This should be the info from
            // the previous components outlets or the node data in this section.
            // Load the node data in this section for the component simulation
            dd_airterminalHotAirInlet(DamperNum).AirMassFlowRate = Node(HotInNode).MassFlowRate;
            dd_airterminalHotAirInlet(DamperNum).AirTemp = Node(HotInNode).Temp;
            dd_airterminalHotAirInlet(DamperNum).AirHumRat = Node(HotInNode).HumRat;
            dd_airterminalHotAirInlet(DamperNum).AirEnthalpy = Node(HotInNode).Enthalpy;
            dd_airterminalColdAirInlet(DamperNum).AirMassFlowRate = Node(ColdInNode).MassFlowRate;
            dd_airterminalColdAirInlet(DamperNum).AirTemp = Node(ColdInNode).Temp;
            dd_airterminalColdAirInlet(DamperNum).AirHumRat = Node(ColdInNode).HumRat;
            dd_airterminalColdAirInlet(DamperNum).AirEnthalpy = Node(ColdInNode).Enthalpy;

            // Initialize the Inlet Nodes of the Dampers for VAV:OutdoorAir
        } else if (  dd_airterminal(DamperNum).DamperType == DualDuct_OutdoorAir) {
            dd_airterminalOAInlet(DamperNum).AirMassFlowRateMaxAvail = Node(OAInNode).MassFlowRateMaxAvail;
            dd_airterminalOAInlet(DamperNum).AirMassFlowRateMinAvail = Node(OAInNode).MassFlowRateMinAvail;

            // Do the following initializations (every time step): This should be the info from
            // the previous components outlets or the node data in this section.
            // Load the node data in this section for the component simulation
            dd_airterminalOAInlet(DamperNum).AirMassFlowRate = Node(OAInNode).MassFlowRate;
            dd_airterminalOAInlet(DamperNum).AirTemp = Node(OAInNode).Temp;
            dd_airterminalOAInlet(DamperNum).AirHumRat = Node(OAInNode).HumRat;
            dd_airterminalOAInlet(DamperNum).AirEnthalpy = Node(OAInNode).Enthalpy;
            if (  dd_airterminal(DamperNum).RecircIsUsed) {
                dd_airterminalRecircAirInlet(DamperNum).AirMassFlowRateMaxAvail = Node(RAInNode).MassFlowRateMaxAvail;
                dd_airterminalRecircAirInlet(DamperNum).AirMassFlowRateMinAvail = Node(RAInNode).MassFlowRateMinAvail;
                dd_airterminalRecircAirInlet(DamperNum).AirMassFlowRate = Node(RAInNode).MassFlowRate;
                dd_airterminalRecircAirInlet(DamperNum).AirTemp = Node(RAInNode).Temp;
                dd_airterminalRecircAirInlet(DamperNum).AirHumRat = Node(RAInNode).HumRat;
                dd_airterminalRecircAirInlet(DamperNum).AirEnthalpy = Node(RAInNode).Enthalpy;
            }
        }
    }

    void DualDuctAirTerminal::SizeDualDuct(int const DamperNum)
    {

        // SUBROUTINE INFORMATION:
        //       AUTHOR         Fred Buhl
        //       DATE WRITTEN   January 2002
        //       MODIFIED       na
        //       RE-ENGINEERED  na

        // PURPOSE OF THIS SUBROUTINE:
        // This subroutine is for sizing Dual Duct air terminal units for which flow rates have not been
        // specified in the input.

        // METHODOLOGY EMPLOYED:
        // Obtains flow rates from the zone or system sizing arrays.

        // Using/Aliasing
        using ReportSizingManager::ReportSizingOutput;

        // SUBROUTINE LOCAL VARIABLE DECLARATIONS:
        std::string DamperType;

        if (  dd_airterminal(DamperNum).MaxAirVolFlowRate == AutoSize) {

            if ((CurZoneEqNum > 0) && (CurTermUnitSizingNum > 0)) {
                if ( dd_airterminal(DamperNum).DamperType == DualDuct_ConstantVolume) {
                    DamperType = cCMO_DDConstantVolume;
                } else if ( dd_airterminal(DamperNum).DamperType == DualDuct_VariableVolume) {
                    DamperType = cCMO_DDVariableVolume;
                } else if ( dd_airterminal(DamperNum).DamperType == DualDuct_OutdoorAir) {
                    DamperType = cCMO_DDVarVolOA;
                } else {
                    DamperType = "Invalid/Unknown";
                }
                CheckZoneSizing(DamperType,  dd_airterminal(DamperNum).Name);
                 dd_airterminal(DamperNum).MaxAirVolFlowRate =
                    max(TermUnitFinalZoneSizing(CurTermUnitSizingNum).DesCoolVolFlow, TermUnitFinalZoneSizing(CurTermUnitSizingNum).DesHeatVolFlow);
                if (  dd_airterminal(DamperNum).DamperType == DualDuct_OutdoorAir) {
                    if ( dd_airterminal(DamperNum).RecircIsUsed) {
                         dd_airterminal(DamperNum).DesignRecircFlowRate = max(TermUnitFinalZoneSizing(CurTermUnitSizingNum).DesCoolVolFlow,
                                                                     TermUnitFinalZoneSizing(CurTermUnitSizingNum).DesHeatVolFlow);
                         dd_airterminal(DamperNum).MaxAirVolFlowRate = dd_airterminal(DamperNum).DesignRecircFlowRate + dd_airterminal(DamperNum).DesignOAFlowRate;
                    } else {
                         dd_airterminal(DamperNum).MaxAirVolFlowRate = dd_airterminal(DamperNum).DesignOAFlowRate;
                         dd_airterminal(DamperNum).DesignRecircFlowRate = 0.0;
                    }
                     dd_airterminal(DamperNum).MaxAirMassFlowRate =  dd_airterminal(DamperNum).MaxAirVolFlowRate * StdRhoAir;
                }

                if (  dd_airterminal(DamperNum).MaxAirVolFlowRate < SmallAirVolFlow) {
                     dd_airterminal(DamperNum).MaxAirVolFlowRate = 0.0;
                     dd_airterminal(DamperNum).MaxAirMassFlowRate = 0.0;
                     dd_airterminal(DamperNum).DesignOAFlowRate = 0.0;
                     dd_airterminal(DamperNum).DesignRecircFlowRate = 0.0;
                }
                ReportSizingOutput(DamperType,  dd_airterminal(DamperNum).Name, "Maximum Air Flow Rate [m3/s]", dd_airterminal(DamperNum).MaxAirVolFlowRate);
                if ( dd_airterminal(DamperNum).DamperType == DualDuct_OutdoorAir) {
                    ReportSizingOutput(
                        DamperType, dd_airterminal(DamperNum).Name, "Maximum Outdoor Air Flow Rate [m3/s]", dd_airterminal(DamperNum).DesignOAFlowRate);
                    if ( dd_airterminal(DamperNum).RecircIsUsed) {
                        ReportSizingOutput(DamperType,
                                           dd_airterminal(DamperNum).Name,
                                           "Maximum Recirculated Air Flow Rate [m3/s]",
                                           dd_airterminal(DamperNum).DesignRecircFlowRate);
                    }
                }
            }
        }
    }

    // End Initialization Section of the Module
    //******************************************************************************

    // Begin Algorithm Section of the Module
    //******************************************************************************

    void DualDuctAirTerminal::SimDualDuctConstVol(int const DamperNum, int const ZoneNum, int const ZoneNodeNum)
    {

        // SUBROUTINE INFORMATION:
        //       AUTHOR         Richard J. Liesen
        //       DATE WRITTEN   Jan 2000
        //       MODIFIED       na
        //       RE-ENGINEERED  na

        // PURPOSE OF THIS SUBROUTINE:
        // This subroutine simulates the simple mixing damper.

        // METHODOLOGY EMPLOYED:
        // There is method to this madness.

        // REFERENCES:
        // na

        // Using/Aliasing
        using namespace DataZoneEnergyDemands;
        // unused0909   USE DataHeatBalFanSys, ONLY: Mat
        using DataHVACGlobals::SmallTempDiff;
        using Psychrometrics::PsyCpAirFnWTdb;
        using Psychrometrics::PsyTdbFnHW;

        // Locals
        // SUBROUTINE ARGUMENT DEFINITIONS:

        // SUBROUTINE PARAMETER DEFINITIONS:
        // na

        // INTERFACE BLOCK SPECIFICATIONS
        // na

        // DERIVED TYPE DEFINITIONS
        // na

        // SUBROUTINE LOCAL VARIABLE DECLARATIONS:
        Real64 MassFlow;    // [kg/sec]   Total Mass Flow Rate from Hot & Cold Inlets
        Real64 HumRat;      // [Kg Moisture / Kg dry air]
        Real64 Enthalpy;    // [Watts]
        Real64 Temperature; // [C]
        Real64 QTotLoad;    // [W]
        Real64 QZnReq;      // [W]
        Real64 CpAirZn;
        Real64 CpAirSysHot;
        Real64 CpAirSysCold;

        // Get the calculated load from the Heat Balance from ZoneSysEnergyDemand
        QTotLoad = ZoneSysEnergyDemand(ZoneNum).RemainingOutputRequired;
        // Need the design MassFlowRate for calculations
        if (GetCurrentScheduleValue(dd_airterminal(DamperNum).SchedPtr) > 0.0) {
            MassFlow = dd_airterminalHotAirInlet(DamperNum).AirMassFlowRateMaxAvail / 2.0 + dd_airterminalColdAirInlet(DamperNum).AirMassFlowRateMaxAvail / 2.0;
        } else {
            MassFlow = 0.0;
        }
        // If there is massflow then need to provide the correct amount of total
        //  required zone energy
        if (MassFlow > SmallMassFlow) {
            CpAirZn = PsyCpAirFnWTdb(Node(ZoneNodeNum).HumRat, Node(ZoneNodeNum).Temp);
            QZnReq = QTotLoad + MassFlow * CpAirZn * Node(ZoneNodeNum).Temp;
            // If the enthalpy is the same for the hot and cold duct then there would be a
            //  divide by zero so for heating or cooling set the damper to one max flow
            //  or the other.
            if (std::abs(dd_airterminalColdAirInlet(DamperNum).AirTemp - dd_airterminalHotAirInlet(DamperNum).AirTemp) > SmallTempDiff) {
                // CpAirSysHot = PsyCpAirFnWTdb(dd_airterminalHotAirInlet(DamperNum)%AirHumRat,dd_airterminalHotAirInlet(DamperNum)%AirTemp)
                // CpAirSysCold= PsyCpAirFnWTdb(dd_airterminalColdAirInlet(DamperNum)%AirHumRat,dd_airterminalColdAirInlet(DamperNum)%AirTemp)
                CpAirSysHot = CpAirZn;
                CpAirSysCold = CpAirZn;
                // Determine the Cold Air Mass Flow Rate
                dd_airterminalColdAirInlet(DamperNum).AirMassFlowRate =
                    (QZnReq - MassFlow * CpAirSysHot * dd_airterminalHotAirInlet(DamperNum).AirTemp) /
                    (CpAirSysCold * dd_airterminalColdAirInlet(DamperNum).AirTemp - CpAirSysHot * dd_airterminalHotAirInlet(DamperNum).AirTemp);
            } else if ((QTotLoad > 0.0) && (dd_airterminalHotAirInlet(DamperNum).AirMassFlowRate > 0.0)) {
                dd_airterminalColdAirInlet(DamperNum).AirMassFlowRate = 0.0;
            } else {
                dd_airterminalColdAirInlet(DamperNum).AirMassFlowRate = MassFlow;
            }
            // Check to make sure that the calculated flow is not greater than the available flows
            if (dd_airterminalColdAirInlet(DamperNum).AirMassFlowRate > dd_airterminalColdAirInlet(DamperNum).AirMassFlowRateMaxAvail) {
                dd_airterminalColdAirInlet(DamperNum).AirMassFlowRate = dd_airterminalColdAirInlet(DamperNum).AirMassFlowRateMaxAvail;
            } else if (dd_airterminalColdAirInlet(DamperNum).AirMassFlowRate < dd_airterminalColdAirInlet(DamperNum).AirMassFlowRateMinAvail) {
                dd_airterminalColdAirInlet(DamperNum).AirMassFlowRate = dd_airterminalColdAirInlet(DamperNum).AirMassFlowRateMinAvail;
            }
            // Using Mass Continuity to determine the other duct flow quantity
            dd_airterminalHotAirInlet(DamperNum).AirMassFlowRate = MassFlow - dd_airterminalColdAirInlet(DamperNum).AirMassFlowRate;
            if (dd_airterminalHotAirInlet(DamperNum).AirMassFlowRate > dd_airterminalHotAirInlet(DamperNum).AirMassFlowRateMaxAvail) {
                dd_airterminalHotAirInlet(DamperNum).AirMassFlowRate = dd_airterminalHotAirInlet(DamperNum).AirMassFlowRateMaxAvail;
            } else if (dd_airterminalHotAirInlet(DamperNum).AirMassFlowRate < dd_airterminalHotAirInlet(DamperNum).AirMassFlowRateMinAvail) {
                dd_airterminalHotAirInlet(DamperNum).AirMassFlowRate = dd_airterminalHotAirInlet(DamperNum).AirMassFlowRateMinAvail;
            }
            MassFlow = dd_airterminalColdAirInlet(DamperNum).AirMassFlowRate + dd_airterminalHotAirInlet(DamperNum).AirMassFlowRate;
        } else {
            // System is Off set massflow to 0.0
            MassFlow = 0.0;
        }
        if (MassFlow > SmallMassFlow) {
            // After flows are calculated then calculate the mixed air flow properties.
            HumRat = (dd_airterminalHotAirInlet(DamperNum).AirHumRat * dd_airterminalHotAirInlet(DamperNum).AirMassFlowRate +
                      dd_airterminalColdAirInlet(DamperNum).AirHumRat * dd_airterminalColdAirInlet(DamperNum).AirMassFlowRate) /
                     MassFlow;
            Enthalpy = (dd_airterminalHotAirInlet(DamperNum).AirEnthalpy * dd_airterminalHotAirInlet(DamperNum).AirMassFlowRate +
                        dd_airterminalColdAirInlet(DamperNum).AirEnthalpy * dd_airterminalColdAirInlet(DamperNum).AirMassFlowRate) /
                       MassFlow;

            // If there is no air flow than calculate the No Flow conditions
        } else {
            dd_airterminalColdAirInlet(DamperNum).AirMassFlowRate = 0.0;
            dd_airterminalHotAirInlet(DamperNum).AirMassFlowRate = 0.0;
            HumRat = (dd_airterminalHotAirInlet(DamperNum).AirHumRat + dd_airterminalColdAirInlet(DamperNum).AirHumRat) / 2.0;
            Enthalpy = (dd_airterminalHotAirInlet(DamperNum).AirEnthalpy + dd_airterminalColdAirInlet(DamperNum).AirEnthalpy) / 2.0;
        }
        Temperature = PsyTdbFnHW(Enthalpy, HumRat);

        // Load all properties in the damper outlet
        dd_airterminalOutlet(DamperNum).AirTemp = Temperature;
        dd_airterminalOutlet(DamperNum).AirHumRat = HumRat;
        dd_airterminalOutlet(DamperNum).AirMassFlowRate = MassFlow;
        dd_airterminalOutlet(DamperNum).AirMassFlowRateMaxAvail = MassFlow;
        dd_airterminalOutlet(DamperNum).AirMassFlowRateMinAvail =
            min(dd_airterminalHotAirInlet(DamperNum).AirMassFlowRateMinAvail, dd_airterminalColdAirInlet(DamperNum).AirMassFlowRateMinAvail);
        dd_airterminalOutlet(DamperNum).AirEnthalpy = Enthalpy;

        // Calculate the hot and cold damper position in %
        if ((dd_airterminalHotAirInlet(DamperNum).AirMassFlowRateMax == 0.0) || (dd_airterminalColdAirInlet(DamperNum).AirMassFlowRateMax == 0.0)) {
            dd_airterminal(DamperNum).ColdAirDamperPosition = 0.0;
            dd_airterminal(DamperNum).HotAirDamperPosition = 0.0;
        } else {
            dd_airterminal(DamperNum).ColdAirDamperPosition =
                dd_airterminalColdAirInlet(DamperNum).AirMassFlowRate / dd_airterminalColdAirInlet(DamperNum).AirMassFlowRateMax;
            dd_airterminal(DamperNum).HotAirDamperPosition = dd_airterminalHotAirInlet(DamperNum).AirMassFlowRate / dd_airterminalHotAirInlet(DamperNum).AirMassFlowRateMax;
        }
    }

    void DualDuctAirTerminal::SimDualDuctVarVol(int const DamperNum, int const ZoneNum, int const ZoneNodeNum)
    {

        // SUBROUTINE INFORMATION:
        //       AUTHOR         Richard J. Liesen
        //       DATE WRITTEN   Jan 2000
        //       MODIFIED       na
        //                      TH 3/2012: added supply air flow adjustment based on zone maximum outdoor
        //                                 air fraction - a TRACE feature
        //       RE-ENGINEERED  na

        // PURPOSE OF THIS SUBROUTINE:
        // This subroutine simulates the simple mixing damper.

        // METHODOLOGY EMPLOYED:
        // There is method to this madness.

        // REFERENCES:
        // na

        // Using/Aliasing
        using namespace DataZoneEnergyDemands;
        // unused0909   USE DataHeatBalFanSys, ONLY: Mat
        using DataHVACGlobals::SmallTempDiff;
        using Psychrometrics::PsyCpAirFnWTdb;
        using Psychrometrics::PsyTdbFnHW;

        // Locals
        // SUBROUTINE ARGUMENT DEFINITIONS:

        // SUBROUTINE PARAMETER DEFINITIONS:
        // na

        // INTERFACE BLOCK SPECIFICATIONS
        // na

        // DERIVED TYPE DEFINITIONS
        // na

        // SUBROUTINE LOCAL VARIABLE DECLARATIONS:
        Real64 MassFlow;    // [kg/sec]   Total Mass Flow Rate from Hot & Cold Inlets
        Real64 HumRat;      // [Kg Moisture / Kg dry air]
        Real64 Enthalpy;    // [Watts]
        Real64 Temperature; // [C]
        Real64 QTotLoad;    // [W]
        Real64 QZnReq;      // [W]
        Real64 CpAirZn;     // specific heat of zone air
        Real64 CpAirSysHot;
        Real64 CpAirSysCold;
        Real64 MassFlowBasedOnOA; // Supply air flow rate based on minimum OA requirement
        Real64 AirLoopOAFrac;     // fraction of outdoor air entering air loop outside air system

        // The calculated load from the Heat Balance
        QTotLoad = ZoneSysEnergyDemand(ZoneNum).RemainingOutputRequired;
        // Calculate all of the required Cp's
        CpAirZn = PsyCpAirFnWTdb(Node(ZoneNodeNum).HumRat, Node(ZoneNodeNum).Temp);
        // CpAirSysHot = PsyCpAirFnWTdb(dd_airterminalHotAirInlet(DamperNum)%AirHumRat,dd_airterminalHotAirInlet(DamperNum)%AirTemp)
        // CpAirSysCold= PsyCpAirFnWTdb(dd_airterminalColdAirInlet(DamperNum)%AirHumRat,dd_airterminalColdAirInlet(DamperNum)%AirTemp)
        CpAirSysHot = CpAirZn;
        CpAirSysCold = CpAirZn;

        // calculate supply air flow rate based on user specified OA requirement
        CalcOAMassFlow(DamperNum, MassFlowBasedOnOA, AirLoopOAFrac);

        // Then depending on if the Load is for heating or cooling it is handled differently.  First
        // the massflow rate of either heating or cooling is determined to meet the entire load.  Then
        // if the massflow is below the minimum or greater than the Max it is set to either the Min
        // or the Max as specified for the VAV model.
        if (GetCurrentScheduleValue( dd_airterminal(DamperNum).SchedPtr) == 0.0) {
            // System is Off set massflow to 0.0
            MassFlow = 0.0;

        } else if ((QTotLoad > 0.0) && (dd_airterminalHotAirInlet(DamperNum).AirMassFlowRateMaxAvail > 0.0)) {
            // Then heating is needed
            // Next check for the denominator equal to zero
            if (std::abs((CpAirSysHot * dd_airterminalHotAirInlet(DamperNum).AirTemp) - (CpAirZn * Node(ZoneNodeNum).Temp)) / CpAirZn > SmallTempDiff) {
                MassFlow = QTotLoad / (CpAirSysHot * dd_airterminalHotAirInlet(DamperNum).AirTemp - CpAirZn * Node(ZoneNodeNum).Temp);
            } else {
                // If denominator tends to zero then mass flow would go to infinity thus set to the max for this iteration
                MassFlow = dd_airterminalHotAirInlet(DamperNum).AirMassFlowRateMaxAvail;
            }
            // Check to see if the flow is < the Min or > the Max air Fraction to the zone; then set to min or max
            if (MassFlow <= (dd_airterminalHotAirInlet(DamperNum).AirMassFlowRateMax * dd_airterminal(DamperNum).ZoneMinAirFrac)) {
                MassFlow = dd_airterminalHotAirInlet(DamperNum).AirMassFlowRateMax * dd_airterminal(DamperNum).ZoneMinAirFrac;
                MassFlow = max(MassFlow, dd_airterminalHotAirInlet(DamperNum).AirMassFlowRateMinAvail);
            } else if (MassFlow >= dd_airterminalHotAirInlet(DamperNum).AirMassFlowRateMaxAvail) {
                MassFlow = dd_airterminalHotAirInlet(DamperNum).AirMassFlowRateMaxAvail;
            }

            // Apply the zone maximum outdoor air fraction for VAV boxes - a TRACE feature
            if (ZoneSysEnergyDemand(ZoneNum).SupplyAirAdjustFactor > 1.0) {
                MassFlow *= ZoneSysEnergyDemand(ZoneNum).SupplyAirAdjustFactor;
            }

            MassFlow = max(MassFlow, MassFlowBasedOnOA);
            MassFlow = min(MassFlow, dd_airterminalHotAirInlet(DamperNum).AirMassFlowRateMaxAvail);

        } else if ((QTotLoad < 0.0) && (dd_airterminalColdAirInlet(DamperNum).AirMassFlowRateMaxAvail > 0.0)) {
            // Then cooling is required
            // Next check for the denominator equal to zero
            if (std::abs((CpAirSysCold * dd_airterminalColdAirInlet(DamperNum).AirTemp) - (CpAirZn * Node(ZoneNodeNum).Temp)) / CpAirZn > SmallTempDiff) {
                MassFlow = QTotLoad / (CpAirSysCold * dd_airterminalColdAirInlet(DamperNum).AirTemp - CpAirZn * Node(ZoneNodeNum).Temp);
            } else {
                // If denominator tends to zero then mass flow would go to infinity thus set to the max for this iteration
                MassFlow = dd_airterminalColdAirInlet(DamperNum).AirMassFlowRateMaxAvail;
            }

            // Check to see if the flow is < the Min or > the Max air Fraction to the zone; then set to min or max
            if ((MassFlow <= (dd_airterminalColdAirInlet(DamperNum).AirMassFlowRateMax * dd_airterminal(DamperNum).ZoneMinAirFrac)) && (MassFlow >= 0.0)) {
                MassFlow = dd_airterminalColdAirInlet(DamperNum).AirMassFlowRateMax * dd_airterminal(DamperNum).ZoneMinAirFrac;
                MassFlow = max(MassFlow, dd_airterminalColdAirInlet(DamperNum).AirMassFlowRateMinAvail);
            } else if (MassFlow < 0.0) {
                MassFlow = dd_airterminalColdAirInlet(DamperNum).AirMassFlowRateMaxAvail;
            } else if (MassFlow >= dd_airterminalColdAirInlet(DamperNum).AirMassFlowRateMaxAvail) {
                MassFlow = dd_airterminalColdAirInlet(DamperNum).AirMassFlowRateMaxAvail;
            }

            // Apply the zone maximum outdoor air fraction for VAV boxes - a TRACE feature
            if (ZoneSysEnergyDemand(ZoneNum).SupplyAirAdjustFactor > 1.0) {
                MassFlow *= ZoneSysEnergyDemand(ZoneNum).SupplyAirAdjustFactor;
            }

            MassFlow = max(MassFlow, MassFlowBasedOnOA);
            MassFlow = min(MassFlow, dd_airterminalColdAirInlet(DamperNum).AirMassFlowRateMaxAvail);

        } else if ((dd_airterminalHotAirInlet(DamperNum).AirMassFlowRateMaxAvail > 0.0) || (dd_airterminalColdAirInlet(DamperNum).AirMassFlowRateMaxAvail > 0.0)) {
            // No Load on Zone set to mixed condition
            MassFlow = (dd_airterminalHotAirInlet(DamperNum).AirMassFlowRateMax / 2.0) * dd_airterminal(DamperNum).ZoneMinAirFrac +
                       dd_airterminalColdAirInlet(DamperNum).AirMassFlowRateMax / 2.0 * dd_airterminal(DamperNum).ZoneMinAirFrac;

            // Apply the zone maximum outdoor air fraction for VAV boxes - a TRACE feature
            if (ZoneSysEnergyDemand(ZoneNum).SupplyAirAdjustFactor > 1.0) {
                MassFlow *= ZoneSysEnergyDemand(ZoneNum).SupplyAirAdjustFactor;
            }

            MassFlow = max(MassFlow, MassFlowBasedOnOA);
            MassFlow = min(MassFlow, (dd_airterminalHotAirInlet(DamperNum).AirMassFlowRateMaxAvail + dd_airterminalColdAirInlet(DamperNum).AirMassFlowRateMaxAvail));

        } else {
            // System is Off set massflow to 0.0
            MassFlow = 0.0;
        }

        // Now the massflow for heating or cooling has been determined and if the massflow was reset to the
        // Min or Max we will need to mix the hot and cold deck to meet the zone load.  Knowing the enthalpy
        // of the zone and the hot and cold air flows we can determine exactly by using the Energy and Continuity
        // Eqns.  Of course we have to make sure that we are within the Min and Max flow conditions.
        if (MassFlow > SmallMassFlow) {
            // Determine the enthalpy required from Zone enthalpy and the zone load.
            QZnReq = QTotLoad + MassFlow * CpAirZn * Node(ZoneNodeNum).Temp;
            // Using the known enthalpies the cold air inlet mass flow is determined.  If the enthalpy of the hot and cold
            // air streams are equal the IF-Then block handles that condition.
            if (std::abs(dd_airterminalColdAirInlet(DamperNum).AirTemp - dd_airterminalHotAirInlet(DamperNum).AirTemp) > SmallTempDiff) {
                // Calculate the Cold air mass flow rate
                dd_airterminalColdAirInlet(DamperNum).AirMassFlowRate =
                    (QZnReq - MassFlow * CpAirSysHot * dd_airterminalHotAirInlet(DamperNum).AirTemp) /
                    (CpAirSysCold * dd_airterminalColdAirInlet(DamperNum).AirTemp - CpAirSysHot * dd_airterminalHotAirInlet(DamperNum).AirTemp);
            } else if ((QTotLoad > 0.0) && (dd_airterminalHotAirInlet(DamperNum).AirMassFlowRate > 0.0)) {
                dd_airterminalColdAirInlet(DamperNum).AirMassFlowRate = 0.0;
            } else {
                dd_airterminalColdAirInlet(DamperNum).AirMassFlowRate = MassFlow;
            }

            // Need to make sure that the flows are within limits
            if (dd_airterminalColdAirInlet(DamperNum).AirMassFlowRate > dd_airterminalColdAirInlet(DamperNum).AirMassFlowRateMaxAvail) {
                dd_airterminalColdAirInlet(DamperNum).AirMassFlowRate = dd_airterminalColdAirInlet(DamperNum).AirMassFlowRateMaxAvail;

                // These are shutoff boxes for either the hot or the cold, therfore one side or other can = 0.0
            } else if (dd_airterminalColdAirInlet(DamperNum).AirMassFlowRate < 0.0) {
                dd_airterminalColdAirInlet(DamperNum).AirMassFlowRate = 0.0;
            } else if (dd_airterminalColdAirInlet(DamperNum).AirMassFlowRate > MassFlow) {
                dd_airterminalColdAirInlet(DamperNum).AirMassFlowRate = MassFlow;
            }
            // Using Mass Continuity to determine the other duct flow quantity
            dd_airterminalHotAirInlet(DamperNum).AirMassFlowRate = MassFlow - dd_airterminalColdAirInlet(DamperNum).AirMassFlowRate;

            if (dd_airterminalHotAirInlet(DamperNum).AirMassFlowRate < MassFlowSetToler) {
                dd_airterminalHotAirInlet(DamperNum).AirMassFlowRate = 0.0;
                dd_airterminalColdAirInlet(DamperNum).AirMassFlowRate = MassFlow;
            } else if (dd_airterminalColdAirInlet(DamperNum).AirMassFlowRate < MassFlowSetToler) {
                dd_airterminalColdAirInlet(DamperNum).AirMassFlowRate = 0.0;
                dd_airterminalHotAirInlet(DamperNum).AirMassFlowRate = MassFlow;
            }

            // After the flow rates are determined the properties are calculated.
            HumRat = (dd_airterminalHotAirInlet(DamperNum).AirHumRat * dd_airterminalHotAirInlet(DamperNum).AirMassFlowRate +
                      dd_airterminalColdAirInlet(DamperNum).AirHumRat * dd_airterminalColdAirInlet(DamperNum).AirMassFlowRate) /
                     MassFlow;
            Enthalpy = (dd_airterminalHotAirInlet(DamperNum).AirEnthalpy * dd_airterminalHotAirInlet(DamperNum).AirMassFlowRate +
                        dd_airterminalColdAirInlet(DamperNum).AirEnthalpy * dd_airterminalColdAirInlet(DamperNum).AirMassFlowRate) /
                       MassFlow;

            // IF the system is OFF the properties are calculated for this special case.
        } else {
            dd_airterminalColdAirInlet(DamperNum).AirMassFlowRate = 0.0;
            dd_airterminalHotAirInlet(DamperNum).AirMassFlowRate = 0.0;
            HumRat = (dd_airterminalHotAirInlet(DamperNum).AirHumRat + dd_airterminalColdAirInlet(DamperNum).AirHumRat) / 2.0;
            Enthalpy = (dd_airterminalHotAirInlet(DamperNum).AirEnthalpy + dd_airterminalColdAirInlet(DamperNum).AirEnthalpy) / 2.0;
        }
        Temperature = PsyTdbFnHW(Enthalpy, HumRat);

        dd_airterminalOutlet(DamperNum).AirTemp = Temperature;
        dd_airterminalOutlet(DamperNum).AirHumRat = HumRat;
        dd_airterminalOutlet(DamperNum).AirMassFlowRate = MassFlow;
        dd_airterminalOutlet(DamperNum).AirMassFlowRateMaxAvail = MassFlow;
        dd_airterminalOutlet(DamperNum).AirMassFlowRateMinAvail = dd_airterminal(DamperNum).ZoneMinAirFrac * dd_airterminalHotAirInlet(DamperNum).AirMassFlowRateMax;
        dd_airterminalOutlet(DamperNum).AirEnthalpy = Enthalpy;
        dd_airterminal(DamperNum).OutdoorAirFlowRate = MassFlow * AirLoopOAFrac;

        // Calculate the hot and cold damper position in %
        if ((dd_airterminalHotAirInlet(DamperNum).AirMassFlowRateMax == 0.0) || (dd_airterminalColdAirInlet(DamperNum).AirMassFlowRateMax == 0.0)) {
            dd_airterminal(DamperNum).ColdAirDamperPosition = 0.0;
            dd_airterminal(DamperNum).HotAirDamperPosition = 0.0;
        } else {
            dd_airterminal(DamperNum).ColdAirDamperPosition =
                dd_airterminalColdAirInlet(DamperNum).AirMassFlowRate / dd_airterminalColdAirInlet(DamperNum).AirMassFlowRateMax;
            dd_airterminal(DamperNum).HotAirDamperPosition = dd_airterminalHotAirInlet(DamperNum).AirMassFlowRate / dd_airterminalHotAirInlet(DamperNum).AirMassFlowRateMax;
        }
    }

    void DualDuctAirTerminal::SimDualDuctVAVOutdoorAir(int const DamperNum, int const ZoneNum, int const ZoneNodeNum)
    {

        // SUBROUTINE INFORMATION:
        //       AUTHOR         Clayton Miller
        //       DATE WRITTEN   Aug 2010
        //       MODIFIED       B. Griffith, Dec 2010, major rework
        //       RE-ENGINEERED  na

        // PURPOSE OF THIS SUBROUTINE:
        // Designed to accommodate for systems with outdoor air (OA) and recirculated air (RA)
        // as two separate air streams to controlled at the zone level in a dual duct system.

        // METHODOLOGY EMPLOYED:
        // The terminal unit is be designed to set the airflow of the of the OA stream at the zone
        // level based on the zonal ventilation requirements and the RA stream flowrate of recirculated
        // cooling air stream in order to meet the remaining thermal load.
        // If the zone calls for cooling but the inlet air temperature is too warm, recirc side set to zero
        // if the zone calls for heating and the inlet air is warm enough, modulate damper to meet load
        // if the zone calls for heating and the inlet air is too cold, zero flow (will not control sans reheat)

        // REFERENCES:
        // na

        // Using/Aliasing
        using namespace DataZoneEnergyDemands;
        using Psychrometrics::PsyCpAirFnWTdb;
        using Psychrometrics::PsyTdbFnHW;
        using namespace DataGlobals;
        using DataHeatBalFanSys::ZoneThermostatSetPointHi;
        using DataHeatBalFanSys::ZoneThermostatSetPointLo;
        using DataHVACGlobals::SmallTempDiff;
        using General::TrimSigDigits;

        // Locals
        // SUBROUTINE ARGUMENT DEFINITIONS:

        // SUBROUTINE PARAMETER DEFINITIONS:
        // na

        // INTERFACE BLOCK SPECIFICATIONS
        // na

        // DERIVED TYPE DEFINITIONS
        // na

        // SUBROUTINE LOCAL VARIABLE DECLARATIONS:
        Real64 MassFlowMax;     // [kg/sec]   Maximum Mass Flow Rate from OA and Recirc Inlets
        Real64 HumRat;          // [Kg Moisture / Kg dry air]
        Real64 Enthalpy;        // [Watts]
        Real64 Temperature;     // [C]
        Real64 QTotLoadRemain;  // [W]
        Real64 QtoHeatSPRemain; // [W]
        Real64 QtoCoolSPRemain; // [W]
        //  REAL(r64) :: QTotRemainAdjust  ! [W]
        Real64 QtoHeatSPRemainAdjust; // [W]
        Real64 QtoCoolSPRemainAdjust; // [W]
        Real64 QOALoadToHeatSP;       // [W]
        Real64 QOALoadToCoolSP;       // [W]
        Real64 QOALoad;               // Amount of cooling load accounted for by OA Stream [W]
        Real64 QRALoad;               // Amount of cooling load accounted for by Recirc Stream [W]
        Real64 CpAirZn;               // specific heat of zone air
        Real64 CpAirSysOA;            // specific heat of outdoor air
        Real64 CpAirSysRA;            // specific heat of recirculated air
        Real64 OAMassFlow;            // Supply air flow rate based on minimum OA requirement - for printing
        Real64 TotMassFlow;           // [kg/sec]   Total Mass Flow Rate from OA and Recirc Inlets
        int OAInletNodeNum;
        int RecircInletNodeNum;

        OAInletNodeNum = dd_airterminal(DamperNum).OAInletNodeNum;
        if ( dd_airterminal(DamperNum).RecircIsUsed) {
            RecircInletNodeNum = dd_airterminal(DamperNum).RecircAirInletNodeNum;
        }
        // Calculate required ventilation air flow rate based on user specified OA requirement
        CalcOAOnlyMassFlow(DamperNum, OAMassFlow);

        // The calculated load from the Heat Balance, adjusted for any equipment sequenced before terminal
        QTotLoadRemain = ZoneSysEnergyDemand(ZoneNum).RemainingOutputRequired;
        QtoHeatSPRemain = ZoneSysEnergyDemand(ZoneNum).RemainingOutputReqToHeatSP;
        QtoCoolSPRemain = ZoneSysEnergyDemand(ZoneNum).RemainingOutputReqToCoolSP;

        // Calculate all of the required Cp's
        CpAirZn = PsyCpAirFnWTdb(Node(ZoneNodeNum).HumRat, Node(ZoneNodeNum).Temp);
        CpAirSysOA = PsyCpAirFnWTdb(Node(OAInletNodeNum).HumRat, Node(OAInletNodeNum).Temp);
        if ( dd_airterminal(DamperNum).RecircIsUsed) CpAirSysRA = PsyCpAirFnWTdb(Node(RecircInletNodeNum).HumRat, Node(RecircInletNodeNum).Temp);

        // Set the OA Damper to the calculated ventilation flow rate
        dd_airterminalOAInlet(DamperNum).AirMassFlowRate = OAMassFlow;
        // Need to make sure that the OA flows are within limits
        if (dd_airterminalOAInlet(DamperNum).AirMassFlowRate > dd_airterminalOAInlet(DamperNum).AirMassFlowRateMaxAvail) {
            dd_airterminalOAInlet(DamperNum).AirMassFlowRate = dd_airterminalOAInlet(DamperNum).AirMassFlowRateMaxAvail;
        } else if (dd_airterminalOAInlet(DamperNum).AirMassFlowRate < 0.0) {
            dd_airterminalOAInlet(DamperNum).AirMassFlowRate = 0.0;
        }

        //..Find the amount of load that the OAMassFlow accounted for
        if (std::abs((CpAirSysOA * dd_airterminalOAInlet(DamperNum).AirTemp) - (CpAirZn * Node(ZoneNodeNum).Temp)) / CpAirZn > SmallTempDiff) {
            QOALoad = dd_airterminalOAInlet(DamperNum).AirMassFlowRate * (CpAirSysOA * dd_airterminalOAInlet(DamperNum).AirTemp - CpAirZn * Node(ZoneNodeNum).Temp);

            QOALoadToHeatSP = dd_airterminalOAInlet(DamperNum).AirMassFlowRate *
                              (CpAirSysOA * dd_airterminalOAInlet(DamperNum).AirTemp - CpAirZn * ZoneThermostatSetPointLo(ZoneNum));
            QOALoadToCoolSP = dd_airterminalOAInlet(DamperNum).AirMassFlowRate *
                              (CpAirSysOA * dd_airterminalOAInlet(DamperNum).AirTemp - CpAirZn * ZoneThermostatSetPointHi(ZoneNum));

        } else {
            QOALoad = 0.0;
            QOALoadToHeatSP = 0.0;
            QOALoadToCoolSP = 0.0;
        }

        if ( dd_airterminal(DamperNum).RecircIsUsed) {

            // correct load for recirc side to account for impact of OA side
            // QTotRemainAdjust      = QTotLoadRemain  - QOALoad
            QtoHeatSPRemainAdjust = QtoHeatSPRemain - QOALoadToHeatSP;
            QtoCoolSPRemainAdjust = QtoCoolSPRemain - QOALoadToCoolSP;

            if (QtoCoolSPRemainAdjust < 0.0) {
                QRALoad = QtoCoolSPRemainAdjust;
            } else if (QtoHeatSPRemainAdjust > 0.0) {
                QRALoad = QtoHeatSPRemainAdjust;
            } else {
                QRALoad = 0.0;
            }

            //  IF (QTotLoadRemain == 0.0d0) THEN  ! floating in deadband
            //    IF ((QTotRemainAdjust < 0.0d0) .AND. (QtoCoolSPRemainAdjust < 0.0d0)) THEN !really need cooling
            //      QRALoad = QtoCoolSPRemainAdjust
            //    ELSEIF ((QTotRemainAdjust > 0.0d0) .AND. (QtoHeatSPRemainAdjust > 0.0d0)) THEN ! really need heating
            //      QRALoad = QtoHeatSPRemainAdjust
            //    ELSE
            //      QRALoad = 0.0 ! still floating in deadband even with impact of OA side
            //    ENDIF
            //  ELSE
            //    QRALoad = QTotRemainAdjust
            //  ENDIF

            if (QRALoad < 0.0) {                                                                 // cooling
                if ((dd_airterminalRecircAirInlet(DamperNum).AirTemp - Node(ZoneNodeNum).Temp) < -0.5) { // can cool
                    //  Find the Mass Flow Rate of the RA Stream needed to meet the zone cooling load
                    if (std::abs((CpAirSysRA * dd_airterminalRecircAirInlet(DamperNum).AirTemp) - (CpAirZn * Node(ZoneNodeNum).Temp)) / CpAirZn >
                        SmallTempDiff) {
                        dd_airterminalRecircAirInlet(DamperNum).AirMassFlowRate =
                            QRALoad / (CpAirSysRA * dd_airterminalRecircAirInlet(DamperNum).AirTemp - CpAirZn * Node(ZoneNodeNum).Temp);
                    }
                } else {
                    dd_airterminalRecircAirInlet(DamperNum).AirMassFlowRate = 0.0;
                }

            } else if (QRALoad > 0.0) { // heating
                //    IF ((dd_airterminalRecircAirInlet(DamperNum)%AirTemp - Node(ZoneNodeNum)%Temp) > 2.0d0)  THEN ! can heat
                //      dd_airterminalRecircAirInlet(DamperNum)%AirMassFlowRate = QRALoad / &
                //                         (CpAirSysRA*dd_airterminalRecircAirInlet(DamperNum)%AirTemp - CpAirZn*Node(ZoneNodeNum)%Temp)
                //    ELSE
                dd_airterminalRecircAirInlet(DamperNum).AirMassFlowRate = 0.0;
                //    ENDIF

            } else { // none needed.
                dd_airterminalRecircAirInlet(DamperNum).AirMassFlowRate = 0.0;
            }

            // Need to make sure that the RA flows are within limits
            if (dd_airterminalRecircAirInlet(DamperNum).AirMassFlowRate > dd_airterminalRecircAirInlet(DamperNum).AirMassFlowRateMaxAvail) {
                dd_airterminalRecircAirInlet(DamperNum).AirMassFlowRate = dd_airterminalRecircAirInlet(DamperNum).AirMassFlowRateMaxAvail;
                // These are shutoff boxes for either the hot or the cold, therfore one side or other can = 0.0
            } else if (dd_airterminalRecircAirInlet(DamperNum).AirMassFlowRate < 0.0) {
                dd_airterminalRecircAirInlet(DamperNum).AirMassFlowRate = 0.0;
            }

        } else {
            dd_airterminalRecircAirInlet(DamperNum).AirMassFlowRate = 0.0;
            dd_airterminalRecircAirInlet(DamperNum).AirMassFlowRateMaxAvail = 0.0;
        } // recirc used

        // look for bang-bang condition: flow rate oscillating between 2 values during the air loop / zone
        // equipment iteration. If detected, set flow rate to previous value.
        if (((std::abs(dd_airterminalRecircAirInlet(DamperNum).AirMassFlowRate - dd_airterminalRecircAirInlet(DamperNum).AirMassFlowRateHist2) <
              dd_airterminalRecircAirInlet(DamperNum).AirMassFlowDiffMag) ||
             (std::abs(dd_airterminalRecircAirInlet(DamperNum).AirMassFlowRate - dd_airterminalRecircAirInlet(DamperNum).AirMassFlowRateHist3) <
              dd_airterminalRecircAirInlet(DamperNum).AirMassFlowDiffMag)) &&
            (std::abs(dd_airterminalRecircAirInlet(DamperNum).AirMassFlowRate - dd_airterminalRecircAirInlet(DamperNum).AirMassFlowRateHist1) >=
             dd_airterminalRecircAirInlet(DamperNum).AirMassFlowDiffMag)) {
            if (dd_airterminalRecircAirInlet(DamperNum).AirMassFlowRate > 0.0) {
                dd_airterminalRecircAirInlet(DamperNum).AirMassFlowRate = dd_airterminalRecircAirInlet(DamperNum).AirMassFlowRateHist1;
            }
        }

        // Find the Max Box Flow Rate.
        MassFlowMax = dd_airterminalOAInlet(DamperNum).AirMassFlowRateMaxAvail + dd_airterminalRecircAirInlet(DamperNum).AirMassFlowRateMaxAvail;
        if (GetCurrentScheduleValue( dd_airterminal(DamperNum).SchedPtr) > 0.0) {
            TotMassFlow = dd_airterminalOAInlet(DamperNum).AirMassFlowRate + dd_airterminalRecircAirInlet(DamperNum).AirMassFlowRate;
        } else {
            TotMassFlow = 0.0;
        }

        if (TotMassFlow > SmallMassFlow) {

            // If the sum of the two air streams' flow is greater than the Max Box Flow Rate then reset the RA Stream
            if (TotMassFlow > MassFlowMax) {
                dd_airterminalRecircAirInlet(DamperNum).AirMassFlowRate = MassFlowMax - dd_airterminalOAInlet(DamperNum).AirMassFlowRate;
            }
            // After the flow rates are determined the properties are calculated.
            TotMassFlow = dd_airterminalOAInlet(DamperNum).AirMassFlowRate + dd_airterminalRecircAirInlet(DamperNum).AirMassFlowRate;
            if (TotMassFlow > SmallMassFlow) {
                HumRat = (dd_airterminalOAInlet(DamperNum).AirHumRat * dd_airterminalOAInlet(DamperNum).AirMassFlowRate +
                          dd_airterminalRecircAirInlet(DamperNum).AirHumRat * dd_airterminalRecircAirInlet(DamperNum).AirMassFlowRate) /
                         TotMassFlow;
                Enthalpy = (dd_airterminalOAInlet(DamperNum).AirEnthalpy * dd_airterminalOAInlet(DamperNum).AirMassFlowRate +
                            dd_airterminalRecircAirInlet(DamperNum).AirEnthalpy * dd_airterminalRecircAirInlet(DamperNum).AirMassFlowRate) /
                           TotMassFlow;
            } else {
                HumRat = (dd_airterminalRecircAirInlet(DamperNum).AirHumRat + dd_airterminalOAInlet(DamperNum).AirHumRat) / 2.0;
                Enthalpy = (dd_airterminalRecircAirInlet(DamperNum).AirEnthalpy + dd_airterminalOAInlet(DamperNum).AirEnthalpy) / 2.0;
            }
        } else {

            // The Max Box Flow Rate is zero and the box is off.
            dd_airterminalRecircAirInlet(DamperNum).AirMassFlowRate = 0.0;
            dd_airterminalOAInlet(DamperNum).AirMassFlowRate = 0.0;
            HumRat = (dd_airterminalRecircAirInlet(DamperNum).AirHumRat + dd_airterminalOAInlet(DamperNum).AirHumRat) / 2.0;
            Enthalpy = (dd_airterminalRecircAirInlet(DamperNum).AirEnthalpy + dd_airterminalOAInlet(DamperNum).AirEnthalpy) / 2.0;
        }

        Temperature = PsyTdbFnHW(Enthalpy, HumRat);

        dd_airterminalOutlet(DamperNum).AirTemp = Temperature;
        dd_airterminalOutlet(DamperNum).AirHumRat = HumRat;
        dd_airterminalOutlet(DamperNum).AirMassFlowRate = TotMassFlow;
        dd_airterminalOutlet(DamperNum).AirMassFlowRateMaxAvail = MassFlowMax;
        dd_airterminalOutlet(DamperNum).AirEnthalpy = Enthalpy;

        // Calculate the OA and RA damper position in %
        if ( dd_airterminal(DamperNum).RecircIsUsed) {
            if (dd_airterminalRecircAirInlet(DamperNum).AirMassFlowRateMax == 0.0) { // protect div by zero
                dd_airterminal(DamperNum).RecircAirDamperPosition = 0.0;
            } else {
                dd_airterminal(DamperNum).RecircAirDamperPosition =
                    dd_airterminalRecircAirInlet(DamperNum).AirMassFlowRate / dd_airterminalRecircAirInlet(DamperNum).AirMassFlowRateMax;
            }
        }

        if (dd_airterminalOAInlet(DamperNum).AirMassFlowRateMax == 0.0) { // protect div by zero
            dd_airterminal(DamperNum).OADamperPosition = 0.0;
        } else {
            dd_airterminal(DamperNum).OADamperPosition = dd_airterminalOAInlet(DamperNum).AirMassFlowRate / dd_airterminalOAInlet(DamperNum).AirMassFlowRateMax;
        }

        // Calculate OAFraction of mixed air after the box
        if (TotMassFlow > 0) {
            if ( dd_airterminal(DamperNum).RecircIsUsed) {
                if (dd_airterminalOAInlet(DamperNum).AirMassFlowRate == 0.0) {
                    dd_airterminal(DamperNum).OAFraction = 0.0;
                } else if (dd_airterminalRecircAirInlet(DamperNum).AirMassFlowRate == 0.0) {
                    dd_airterminal(DamperNum).OAFraction = 1.0;
                } else {
                    dd_airterminal(DamperNum).OAFraction = dd_airterminalOAInlet(DamperNum).AirMassFlowRate / TotMassFlow;
                }
            } else {
                dd_airterminal(DamperNum).OAFraction = 1.0;
            }
        } else {
            dd_airterminal(DamperNum).OAFraction = 0.0;
        }

        dd_airterminalRecircAirInlet(DamperNum).AirMassFlowRateHist3 = dd_airterminalRecircAirInlet(DamperNum).AirMassFlowRateHist2;
        dd_airterminalRecircAirInlet(DamperNum).AirMassFlowRateHist2 = dd_airterminalRecircAirInlet(DamperNum).AirMassFlowRateHist1;
        dd_airterminalRecircAirInlet(DamperNum).AirMassFlowRateHist1 = dd_airterminalRecircAirInlet(DamperNum).AirMassFlowRate;
    }

    void DualDuctAirTerminal::CalcOAMassFlow(int const DamperNum,  // index to terminal unit
                        Real64 &SAMassFlow,   // outside air based on optional user input
                        Real64 &AirLoopOAFrac // outside air based on optional user input
    )
    {

        // FUNCTION INFORMATION:
        //       AUTHOR         R. Raustad (FSEC)
        //       DATE WRITTEN   Mar 2010
        //       MODIFIED       Mangesh Basarkar, 06/2011: Modifying outside air based on airloop DCV flag
        //       RE-ENGINEERED  na

        // PURPOSE OF THIS FUNCTION:
        // Calculates the amount of outside air required based on optional user input.
        // Zone multipliers are included and are applied in GetInput.

        // METHODOLOGY EMPLOYED:
        // User input defines method used to calculate OA.

        using DataAirLoop::AirLoopControlInfo;
        using DataAirLoop::AirLoopFlow;
        using DataZoneEquipment::CalcDesignSpecificationOutdoorAir;
        using DataZoneEquipment::ZoneEquipConfig;
        using Psychrometrics::PsyRhoAirFnPbTdbW;

        bool const UseMinOASchFlag(true); // Always use min OA schedule in calculations.

        Real64 OAVolumeFlowRate; // outside air volume flow rate (m3/s)
        Real64 OAMassFlow;       // outside air mass flow rate (kg/s)

        // initialize OA flow rate and OA report variable
        SAMassFlow = 0.0;
        AirLoopOAFrac = 0.0;
        int AirLoopNum = dd_airterminal(DamperNum).AirLoopNum;

        // Calculate the amount of OA based on optional user inputs
        if (AirLoopNum > 0) {
            AirLoopOAFrac = AirLoopFlow(AirLoopNum).OAFrac;
            // If no additional input from user, RETURN from subroutine
            if ( dd_airterminal(DamperNum).NoOAFlowInputFromUser) return;
            // Calculate outdoor air flow rate, zone multipliers are applied in GetInput
            if (AirLoopOAFrac > 0.0) {
                OAVolumeFlowRate = CalcDesignSpecificationOutdoorAir( dd_airterminal(DamperNum).OARequirementsPtr,
                                                                     dd_airterminal(DamperNum).ActualZoneNum,
                                                                     AirLoopControlInfo(AirLoopNum).AirLoopDCVFlag,
                                                                     UseMinOASchFlag);
                OAMassFlow = OAVolumeFlowRate * StdRhoAir;

                // convert OA mass flow rate to supply air flow rate based on air loop OA fraction
                SAMassFlow = OAMassFlow / AirLoopOAFrac;
            }
        }
    }

    void DualDuctAirTerminal::CalcOAOnlyMassFlow(int const DamperNum,          // index to terminal unit
                            Real64 &OAMassFlow,           // outside air flow from user input kg/s
                            Optional<Real64> MaxOAVolFlow // design level for outside air m3/s
    )
    {

        // FUNCTION INFORMATION:
        //       AUTHOR         C. Miller (Mod of CaclOAMassFlow by R. Raustad (FSEC))
        //       DATE WRITTEN   Aug 2010
        //       MODIFIED       B. Griffith, Dec 2010 clean up, sizing optional, scheduled OA
        //       RE-ENGINEERED  na

        // PURPOSE OF THIS FUNCTION:
        // Calculates the amount of outside air required based on optional user input. Returns
        // ONLY calculated OAMassFlow without consideration of AirLoopOAFrac. Used for
        // the DualDuct:VAV:OutdoorAir object which does not mix OA with RA

        // METHODOLOGY EMPLOYED:
        // User input defines method used to calculate OA.

        // REFERENCES:

        // Using/Aliasing
        using DataZoneEquipment::CalcDesignSpecificationOutdoorAir;
        using Psychrometrics::PsyRhoAirFnPbTdbW;

        // Locals
        // SUBROUTINE ARGUMENT DEFINITIONS:

        // FUNCTION PARAMETER DEFINITIONS:
        bool const UseMinOASchFlag(true); // Always use min OA schedule in calculations.
        static std::string const RoutineName("HVACDualDuctSystem:CalcOAOnlyMassFlow");

        // INTERFACE BLOCK SPECIFICATIONS
        // na

        // DERIVED TYPE DEFINITIONS
        // na

        // FUNCTION LOCAL VARIABLE DECLARATIONS:

        Real64 OAVolumeFlowRate; // outside air volume flow rate (m3/s)
        bool UseOccSchFlag;      // TRUE = use actual occupancy, FALSE = use total zone people
        bool PerPersonNotSet;

        // Calculate the amount of OA based on optional user inputs
        OAMassFlow = 0.0;

        // If no additional input from user, RETURN from subroutine
        if ( dd_airterminal(DamperNum).NoOAFlowInputFromUser) {
            ShowSevereError("CalcOAOnlyMassFlow: Problem in AirTerminal:DualDuct:VAV:OutdoorAir = " + dd_airterminal(DamperNum).Name +
                            ", check outdoor air specification");
            if (present(MaxOAVolFlow)) MaxOAVolFlow = 0.0;
            return;
        }

        if ( dd_airterminal(DamperNum).OAPerPersonMode == PerPersonDCVByCurrentLevel) {
            UseOccSchFlag = true;
            PerPersonNotSet = false;
        } else {
            UseOccSchFlag = false;
            PerPersonNotSet = false;
            if ( dd_airterminal(DamperNum).OAPerPersonMode == PerPersonModeNotSet) PerPersonNotSet = true;
        }

        OAVolumeFlowRate = CalcDesignSpecificationOutdoorAir(
            dd_airterminal(DamperNum).OARequirementsPtr, dd_airterminal(DamperNum).ActualZoneNum, UseOccSchFlag, UseMinOASchFlag, PerPersonNotSet);

        OAMassFlow = OAVolumeFlowRate * StdRhoAir;

        if (present(MaxOAVolFlow)) {
            OAVolumeFlowRate = CalcDesignSpecificationOutdoorAir(
                dd_airterminal(DamperNum).OARequirementsPtr, dd_airterminal(DamperNum).ActualZoneNum, UseOccSchFlag, UseMinOASchFlag, _, true);
            MaxOAVolFlow = OAVolumeFlowRate;
        }
    }

    // End Algorithm Section of the Module
    // *****************************************************************************

    // Beginning of Update subroutines for the Damper Module
    // *****************************************************************************

    void DualDuctAirTerminal::UpdateDualDuct(int const DamperNum)
    {

        // SUBROUTINE INFORMATION:
        //       AUTHOR         Richard J. Liesen
        //       DATE WRITTEN   February 2000
        //       MODIFIED       Aug 2010 Clayton Miller - Added DualDuctVAVOutdoorAir
        //       RE-ENGINEERED  na

        // PURPOSE OF THIS SUBROUTINE:
        // This subroutine updates the dampers.

        // METHODOLOGY EMPLOYED:
        // There is method to this madness.

        // REFERENCES:
        // na

        // Using/Aliasing
        using DataContaminantBalance::Contaminant;

        // Locals
        // SUBROUTINE ARGUMENT DEFINITIONS:

        // SUBROUTINE PARAMETER DEFINITIONS:
        // na

        // INTERFACE BLOCK SPECIFICATIONS
        // na

        // DERIVED TYPE DEFINITIONS
        // na

        // SUBROUTINE LOCAL VARIABLE DECLARATIONS:
        int OutletNode;
        int HotInletNode;
        int ColdInletNode;
        int OAInletNode; // Outdoor Air Duct Inlet Node - for DualDuctOutdoorAir
        int RAInletNode; // Recirculated Air Duct Inlet Node - for DualDuctOutdoorAir

        if ( dd_airterminal(DamperNum).DamperType == DualDuct_ConstantVolume || dd_airterminal(DamperNum).DamperType == DualDuct_VariableVolume) {

            OutletNode = dd_airterminal(DamperNum).OutletNodeNum;
            HotInletNode = dd_airterminal(DamperNum).HotAirInletNodeNum;
            ColdInletNode = dd_airterminal(DamperNum).ColdAirInletNodeNum;

            // Set the outlet air nodes of the Damper
            Node(HotInletNode).MassFlowRate = dd_airterminalHotAirInlet(DamperNum).AirMassFlowRate;
            Node(ColdInletNode).MassFlowRate = dd_airterminalColdAirInlet(DamperNum).AirMassFlowRate;
            Node(OutletNode).MassFlowRate = dd_airterminalOutlet(DamperNum).AirMassFlowRate;
            Node(OutletNode).MassFlowRateMaxAvail = dd_airterminalOutlet(DamperNum).AirMassFlowRate;
            Node(OutletNode).MassFlowRateMinAvail = dd_airterminalOutlet(DamperNum).AirMassFlowRateMinAvail;
            Node(OutletNode).Temp = dd_airterminalOutlet(DamperNum).AirTemp;
            Node(OutletNode).HumRat = dd_airterminalOutlet(DamperNum).AirHumRat;
            Node(OutletNode).Enthalpy = dd_airterminalOutlet(DamperNum).AirEnthalpy;
            // Set the outlet nodes for properties that just pass through & not used
            // FIX THIS LATER!!!!
            Node(OutletNode).Quality = Node(HotInletNode).Quality;
            Node(OutletNode).Press = Node(HotInletNode).Press;

            if (Contaminant.CO2Simulation) {
                if (Node(OutletNode).MassFlowRate > 0.0) {
                    Node(OutletNode).CO2 =
                        (Node(HotInletNode).CO2 * Node(HotInletNode).MassFlowRate + Node(ColdInletNode).CO2 * Node(ColdInletNode).MassFlowRate) /
                        Node(OutletNode).MassFlowRate;
                } else {
                    Node(OutletNode).CO2 = max(Node(HotInletNode).CO2, Node(ColdInletNode).CO2);
                }
            }
            if (Contaminant.GenericContamSimulation) {
                if (Node(OutletNode).MassFlowRate > 0.0) {
                    Node(OutletNode).GenContam = (Node(HotInletNode).GenContam * Node(HotInletNode).MassFlowRate +
                                                  Node(ColdInletNode).GenContam * Node(ColdInletNode).MassFlowRate) /
                                                 Node(OutletNode).MassFlowRate;
                } else {
                    Node(OutletNode).GenContam = max(Node(HotInletNode).GenContam, Node(ColdInletNode).GenContam);
                }
            }
        } else if ( dd_airterminal(DamperNum).DamperType == DualDuct_OutdoorAir) {

            OutletNode = dd_airterminal(DamperNum).OutletNodeNum;
            OAInletNode = dd_airterminal(DamperNum).OAInletNodeNum;
            if ( dd_airterminal(DamperNum).RecircIsUsed) {
                RAInletNode = dd_airterminal(DamperNum).RecircAirInletNodeNum;
                Node(RAInletNode).MassFlowRate = dd_airterminalRecircAirInlet(DamperNum).AirMassFlowRate;
            }
            // Set the outlet air nodes of the Damper
            Node(OAInletNode).MassFlowRate = dd_airterminalOAInlet(DamperNum).AirMassFlowRate;
            Node(OutletNode).MassFlowRate = dd_airterminalOutlet(DamperNum).AirMassFlowRate;
            Node(OutletNode).MassFlowRateMaxAvail = dd_airterminalOutlet(DamperNum).AirMassFlowRate;
            Node(OutletNode).MassFlowRateMinAvail = dd_airterminalOutlet(DamperNum).AirMassFlowRateMinAvail;
            Node(OutletNode).Temp = dd_airterminalOutlet(DamperNum).AirTemp;
            Node(OutletNode).HumRat = dd_airterminalOutlet(DamperNum).AirHumRat;
            Node(OutletNode).Enthalpy = dd_airterminalOutlet(DamperNum).AirEnthalpy;
            // Set the outlet nodes for properties that just pass through & not used
            // FIX THIS LATER!!!!
            Node(OutletNode).Quality = Node(OAInletNode).Quality;
            Node(OutletNode).Press = Node(OAInletNode).Press;

            if ( dd_airterminal(DamperNum).RecircIsUsed) {
                if (Node(OutletNode).MassFlowRate > 0.0) {
                    if (Contaminant.CO2Simulation) {
                        Node(OutletNode).CO2 =
                            (Node(OAInletNode).CO2 * Node(OAInletNode).MassFlowRate + Node(RAInletNode).CO2 * Node(RAInletNode).MassFlowRate) /
                            Node(OutletNode).MassFlowRate;
                    }
                    if (Contaminant.GenericContamSimulation) {
                        Node(OutletNode).GenContam = (Node(OAInletNode).GenContam * Node(OAInletNode).MassFlowRate +
                                                      Node(RAInletNode).GenContam * Node(RAInletNode).MassFlowRate) /
                                                     Node(OutletNode).MassFlowRate;
                    }
                } else {
                    if (Contaminant.CO2Simulation) {
                        Node(OutletNode).CO2 = max(Node(OAInletNode).CO2, Node(RAInletNode).CO2);
                    }
                    if (Contaminant.GenericContamSimulation) {
                        Node(OutletNode).GenContam = max(Node(OAInletNode).GenContam, Node(RAInletNode).GenContam);
                    }
                }

            } else {
                if (Contaminant.CO2Simulation) {
                    Node(OutletNode).CO2 = Node(OAInletNode).CO2;
                }
                if (Contaminant.GenericContamSimulation) {
                    Node(OutletNode).GenContam = Node(OAInletNode).GenContam;
                }
            }
        }
    }

    //        End of Update subroutines for the Damper Module
    // *****************************************************************************

    // Beginning of Reporting subroutines for the Damper Module
    // *****************************************************************************

    void DualDuctAirTerminal::ReportDualDuct(int const EP_UNUSED(DamperNum)) // unused1208
    {

        // SUBROUTINE INFORMATION:
        //       AUTHOR         Unknown
        //       DATE WRITTEN   Unknown
        //       MODIFIED       na
        //       RE-ENGINEERED  na

        // PURPOSE OF THIS SUBROUTINE:
        // This subroutine updates the damper report variables.

        // METHODOLOGY EMPLOYED:
        // There is method to this madness.

        // REFERENCES:
        // na

        // USE STATEMENTS:
        // na

        // Locals
        // SUBROUTINE ARGUMENT DEFINITIONS:

        // SUBROUTINE PARAMETER DEFINITIONS:
        // na

        // INTERFACE BLOCK SPECIFICATIONS
        // na

        // DERIVED TYPE DEFINITIONS
        // na

        // SUBROUTINE LOCAL VARIABLE DECLARATIONS:

        // Still needs to report the Damper power from this component
    }

    void ReportDualDuctConnections()
    {

        // SUBROUTINE INFORMATION:
        //       AUTHOR         Michael J. Witte
        //       DATE WRITTEN   February 2004
        //       MODIFIED       B. Griffith, DOAS VAV dual duct
        //       RE-ENGINEERED  na

        // PURPOSE OF THIS SUBROUTINE:
        // Report dual duct damper connections to the BND file.

        // METHODOLOGY EMPLOYED:
        // Needs description, as appropriate.

        // REFERENCES:
        // na

        // Using/Aliasing
        using DataAirLoop::AirToZoneNodeInfo;
        using DataGlobals::OutputFileBNDetails;
        using DataHVACGlobals::NumPrimaryAirSys;
        using DataZoneEquipment::NumSupplyAirPaths;
        using DataZoneEquipment::SupplyAirPath;

        // Locals
        // SUBROUTINE ARGUMENT DEFINITIONS:
        // na

        // SUBROUTINE PARAMETER DEFINITIONS:
        // na

        // INTERFACE BLOCK SPECIFICATIONS
        // na

        // DERIVED TYPE DEFINITIONS
        // na

        // SUBROUTINE LOCAL VARIABLE DECLARATIONS:
        int Count1;
        int Count2;
        int Count3;
        int Found;
        int SupplyAirPathNum; // Supply air path ID
        std::string ChrOut;
        std::string ChrName;
        std::string DamperType;

        // Formats
        static ObjexxFCL::gio::Fmt Format_100("('! <#Dual Duct Damper Connections>,<Number of Dual Duct Damper Connections>')");
        static ObjexxFCL::gio::Fmt Format_101("(A)");
        static ObjexxFCL::gio::Fmt Format_102("('! <Dual Duct Damper>,<Dual Duct Damper Count>,<Dual Duct Damper Name>,<Inlet Node>,','<Outlet Node>,<Inlet "
                                   "Node Type>,<AirLoopHVAC Name>')");
        static ObjexxFCL::gio::Fmt fmtLD("*");

        if (!allocated(dd_airterminal))
            return; // Autodesk Bug: Can arrive here with Damper unallocated (SimulateDualDuct not yet called) with NumDampers either set >0 or
                    // uninitialized

        // Report Dual Duct Dampers to BND File
        ObjexxFCL::gio::write(OutputFileBNDetails, Format_101) << "! ===============================================================";
        ObjexxFCL::gio::write(OutputFileBNDetails, Format_100);
        ObjexxFCL::gio::write(ChrOut, fmtLD) << NumDampers * 2;
        ObjexxFCL::gio::write(OutputFileBNDetails, Format_101) << " #Dual Duct Damper Connections," + stripped(ChrOut);
        ObjexxFCL::gio::write(OutputFileBNDetails, Format_102);

        for (Count1 = 1; Count1 <= NumDampers; ++Count1) {

            // Determine if this damper is connected to a supply air path
            Found = 0;
            for (Count2 = 1; Count2 <= NumSupplyAirPaths; ++Count2) {
                SupplyAirPathNum = Count2;
                Found = 0;
                for (Count3 = 1; Count3 <= SupplyAirPath(Count2).NumOutletNodes; ++Count3) {
                    if ( dd_airterminal(Count1).HotAirInletNodeNum == SupplyAirPath(Count2).OutletNode(Count3)) Found = Count3;
                    if ( dd_airterminal(Count1).ColdAirInletNodeNum == SupplyAirPath(Count2).OutletNode(Count3)) Found = Count3;
                    if ( dd_airterminal(Count1).OAInletNodeNum == SupplyAirPath(Count2).OutletNode(Count3)) Found = Count3;
                    if ( dd_airterminal(Count1).RecircAirInletNodeNum == SupplyAirPath(Count2).OutletNode(Count3)) Found = Count3;
                }
                if (Found != 0) break;
            }
            if (Found == 0) SupplyAirPathNum = 0;

            // Determine which air loop this dual duct damper is connected to
            Found = 0;
            for (Count2 = 1; Count2 <= NumPrimaryAirSys; ++Count2) {
                ChrName = AirToZoneNodeInfo(Count2).AirLoopName;
                Found = 0;
                for (Count3 = 1; Count3 <= AirToZoneNodeInfo(Count2).NumSupplyNodes; ++Count3) {
                    if (SupplyAirPathNum != 0) {
                        if (SupplyAirPath(SupplyAirPathNum).InletNodeNum == AirToZoneNodeInfo(Count2).ZoneEquipSupplyNodeNum(Count3)) Found = Count3;
                    } else {
                        if ( dd_airterminal(Count1).HotAirInletNodeNum == AirToZoneNodeInfo(Count2).ZoneEquipSupplyNodeNum(Count3)) Found = Count3;
                        if ( dd_airterminal(Count1).ColdAirInletNodeNum == AirToZoneNodeInfo(Count2).ZoneEquipSupplyNodeNum(Count3)) Found = Count3;
                        if ( dd_airterminal(Count1).OAInletNodeNum == AirToZoneNodeInfo(Count2).ZoneEquipSupplyNodeNum(Count3)) Found = Count3;
                        if ( dd_airterminal(Count1).RecircAirInletNodeNum == AirToZoneNodeInfo(Count2).ZoneEquipSupplyNodeNum(Count3)) Found = Count3;
                    }
                }
                if (Found != 0) break;
            }
            if (Found == 0) ChrName = "**Unknown**";

            ObjexxFCL::gio::write(ChrOut, fmtLD) << Count1;
            if ( dd_airterminal(Count1).DamperType == DualDuct_ConstantVolume) {
                DamperType = cCMO_DDConstantVolume;
            } else if ( dd_airterminal(Count1).DamperType == DualDuct_VariableVolume) {
                DamperType = cCMO_DDVariableVolume;
            } else if ( dd_airterminal(Count1).DamperType == DualDuct_OutdoorAir) {
                DamperType = cCMO_DDVarVolOA;
            } else {
                DamperType = "Invalid/Unknown";
            }

            if (( dd_airterminal(Count1).DamperType == DualDuct_ConstantVolume) || ( dd_airterminal(Count1).DamperType == DualDuct_VariableVolume)) {
                ObjexxFCL::gio::write(OutputFileBNDetails, Format_101) << " Dual Duct Damper," + stripped(ChrOut) + ',' + DamperType + ',' +
                                                                    dd_airterminal(Count1).Name + ',' + NodeID( dd_airterminal(Count1).HotAirInletNodeNum) + ',' +
                                                                   NodeID( dd_airterminal(Count1).OutletNodeNum) + ",Hot Air," + ChrName;

                ObjexxFCL::gio::write(OutputFileBNDetails, Format_101) << " Dual Duct Damper," + stripped(ChrOut) + ',' + DamperType + ',' +
                                                                    dd_airterminal(Count1).Name + ',' + NodeID( dd_airterminal(Count1).ColdAirInletNodeNum) +
                                                                   ',' + NodeID( dd_airterminal(Count1).OutletNodeNum) + ",Cold Air," + ChrName;
            } else if ( dd_airterminal(Count1).DamperType == DualDuct_OutdoorAir) {
                ObjexxFCL::gio::write(OutputFileBNDetails, Format_101) << "Dual Duct Damper, " + stripped(ChrOut) + ',' + DamperType + ',' +
                                                                    dd_airterminal(Count1).Name + ',' + NodeID( dd_airterminal(Count1).OAInletNodeNum) + ',' +
                                                                   NodeID( dd_airterminal(Count1).OutletNodeNum) + ",Outdoor Air," + ChrName;
                ObjexxFCL::gio::write(OutputFileBNDetails, Format_101) << "Dual Duct Damper, " + stripped(ChrOut) + ',' + DamperType + ',' +
                                                                    dd_airterminal(Count1).Name + ',' + NodeID( dd_airterminal(Count1).RecircAirInletNodeNum) +
                                                                   ',' + NodeID( dd_airterminal(Count1).OutletNodeNum) + ",Recirculated Air," + ChrName;
            }
        }
    }

    void GetDualDuctOutdoorAirRecircUse(std::string const &EP_UNUSED(CompTypeName), std::string const &CompName, bool &RecircIsUsed)
    {

        // SUBROUTINE INFORMATION:
        //       AUTHOR         B. Griffith
        //       DATE WRITTEN   Aug 2011
        //       MODIFIED       na
        //       RE-ENGINEERED  na

        // PURPOSE OF THIS SUBROUTINE:
        // get routine to learn if a dual duct outdoor air unit is using its recirc deck

        // SUBROUTINE LOCAL VARIABLE DECLARATIONS:
        //  INTEGER :: DamperNum
        static bool FirstTimeOnly(true);
        static Array1D_bool RecircIsUsedARR;
        static Array1D_string DamperNamesARR;
        int DamperIndex;                 // Loop index to Damper that you are currently loading input into
        std::string CurrentModuleObject; // for ease in getting objects
        static Array1D<Real64> NumArray(2, 0.0);
        static Array1D_string AlphArray(7);
        static Array1D_string cAlphaFields(7);       // Alpha field names
        static Array1D_string cNumericFields(2);     // Numeric field names
        static Array1D_bool lAlphaBlanks(7, true);   // Logical array, alpha field input BLANK = .TRUE.
        static Array1D_bool lNumericBlanks(2, true); // Logical array, numeric field input BLANK = .TRUE.
        int NumAlphas;
        int NumNums;
        int IOStat;

        RecircIsUsed = true;

        // this doesn't work because it fires code that depends on things being further along
        //  IF (GetDualDuctInputFlag) THEN  !First time subroutine has been entered
        //    CALL GetDualDuctInput
        //    GetDualDuctInputFlag=.FALSE.
        //  END IF

        if (FirstTimeOnly) {
            NumDualDuctVarVolOA = inputProcessor->getNumObjectsFound(cCMO_DDVarVolOA);
            RecircIsUsedARR.allocate(NumDualDuctVarVolOA);
            DamperNamesARR.allocate(NumDualDuctVarVolOA);
            if (NumDualDuctVarVolOA > 0) {
                for (DamperIndex = 1; DamperIndex <= NumDualDuctVarVolOA; ++DamperIndex) {

                    CurrentModuleObject = cCMO_DDVarVolOA;

                    inputProcessor->getObjectItem(CurrentModuleObject,
                                                  DamperIndex,
                                                  AlphArray,
                                                  NumAlphas,
                                                  NumArray,
                                                  NumNums,
                                                  IOStat,
                                                  lNumericBlanks,
                                                  lAlphaBlanks,
                                                  cAlphaFields,
                                                  cNumericFields);
                    DamperNamesARR(DamperIndex) = AlphArray(1);
                    if (!lAlphaBlanks(5)) {
                        RecircIsUsedARR(DamperIndex) = true;
                    } else {
                        RecircIsUsedARR(DamperIndex) = false;
                    }
                }
            }
            FirstTimeOnly = false;
        }

        DamperIndex = UtilityRoutines::FindItemInList(CompName, DamperNamesARR, NumDualDuctVarVolOA);
        if (DamperIndex > 0) {
            RecircIsUsed = RecircIsUsedARR(DamperIndex);
        }
    }

    //        End of Reporting subroutines for the Damper Module
    // *****************************************************************************

    void clear_state()
    {
        UniqueDualDuctAirTerminalNames.clear();
    }

} // namespace DualDuct

} // namespace EnergyPlus

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

#ifndef WindowManagerExteriorOptical_hh_INCLUDED
#define WindowManagerExteriorOptical_hh_INCLUDED

#include <memory>

namespace EnergyPlus {
namespace DataHeatBalance {
    struct MaterialProperties;
}
} // namespace EnergyPlus

namespace FenestrationCommon {

enum class WavelengthRange;
enum class Property;
enum class Side;

} // namespace FenestrationCommon

namespace SpectralAveraging {

class CSpectralSampleData;
class CSpectralSample;
class CAngularSpectralSample;

} // namespace SpectralAveraging

namespace SingleLayerOptics {

class CBSDFLayer;
class CScatteringLayer;
class ICellDescription;
class CMaterial;
class CMaterialSingleBand;

} // namespace SingleLayerOptics

namespace EnergyPlus {

namespace WindowManager {

    class CWCEIntegrator;

    // Initialize window optical properties with Windows-CalcEngine routines that are BSDF based
    //void InitWCE_BSDFOpticalData();

    void InitWCE_SimplifiedOpticalData();

    std::shared_ptr<SingleLayerOptics::CBSDFLayer> getBSDFLayer( const DataHeatBalance::MaterialProperties & t_Material,
                                                                 const FenestrationCommon::WavelengthRange t_Range );

    std::shared_ptr<SingleLayerOptics::CScatteringLayer> getScatteringLayer( const DataHeatBalance::MaterialProperties & t_Material,
                                                                             const FenestrationCommon::WavelengthRange t_Range );

    ///////////////////////////////////////////////////////////////////////////////
    //   CWCEMaterialFactory
    ///////////////////////////////////////////////////////////////////////////////
    class CWCEMaterialFactory
    {
    public:
        virtual ~CWCEMaterialFactory() = default;
        CWCEMaterialFactory(const DataHeatBalance::MaterialProperties & t_Material,
                            const FenestrationCommon::WavelengthRange t_Range);

        std::shared_ptr<SingleLayerOptics::CMaterial> getMaterial();

    protected:
        virtual void init() = 0;
        std::shared_ptr<SingleLayerOptics::CMaterial> m_Material;
        DataHeatBalance::MaterialProperties m_MaterialProperties;
        FenestrationCommon::WavelengthRange m_Range;
        bool m_Initialized;
    };

    ///////////////////////////////////////////////////////////////////////////////
    //   CWCESpecularMaterialsFactory
    ///////////////////////////////////////////////////////////////////////////////
    class CWCESpecularMaterialsFactory : public CWCEMaterialFactory
    {
    public:
        CWCESpecularMaterialsFactory(const DataHeatBalance::MaterialProperties & t_Material,
                                     const FenestrationCommon::WavelengthRange t_Range);

    private:
        void init() override;
    };

    ///////////////////////////////////////////////////////////////////////////////
    //   CWCEMaterialDualBandFactory
    ///////////////////////////////////////////////////////////////////////////////
    class CWCEMaterialDualBandFactory : public CWCEMaterialFactory
    {
        // Common interface class for devices with materials defined over visible and solar range.
        // It is mainly intended from shading devices.
    public:
        CWCEMaterialDualBandFactory(const DataHeatBalance::MaterialProperties & t_Material,
                                    const FenestrationCommon::WavelengthRange t_Range);

    protected:
        void init() override;
        virtual std::shared_ptr<SingleLayerOptics::CMaterialSingleBand> createVisibleRangeMaterial() = 0;
        virtual std::shared_ptr<SingleLayerOptics::CMaterialSingleBand> createSolarRangeMaterial() = 0;
    };

    ///////////////////////////////////////////////////////////////////////////////
    //   CWCEVenetianBlindMaterialsFactory
    ///////////////////////////////////////////////////////////////////////////////
    class CWCEVenetianBlindMaterialsFactory : public CWCEMaterialDualBandFactory
    {
    public:
        CWCEVenetianBlindMaterialsFactory(const DataHeatBalance::MaterialProperties & t_Material,
                                          const FenestrationCommon::WavelengthRange t_Range);

    private:
        std::shared_ptr<SingleLayerOptics::CMaterialSingleBand> createVisibleRangeMaterial() override;
        std::shared_ptr<SingleLayerOptics::CMaterialSingleBand> createSolarRangeMaterial() override;
    };

    ///////////////////////////////////////////////////////////////////////////////
    //   CWCEScreenMaterialsFactory
    ///////////////////////////////////////////////////////////////////////////////
    class CWCEScreenMaterialsFactory : public CWCEMaterialDualBandFactory
    {
    public:
        CWCEScreenMaterialsFactory(const DataHeatBalance::MaterialProperties & t_Material,
                                   const FenestrationCommon::WavelengthRange t_Range);

    private:
        std::shared_ptr<SingleLayerOptics::CMaterialSingleBand> createVisibleRangeMaterial() override;
        std::shared_ptr<SingleLayerOptics::CMaterialSingleBand> createSolarRangeMaterial() override;
    };

    ///////////////////////////////////////////////////////////////////////////////
    //   CWCEDiffuseShadeMaterialsFactory
    ///////////////////////////////////////////////////////////////////////////////
    class CWCEDiffuseShadeMaterialsFactory : public CWCEMaterialDualBandFactory
    {
    public:
        CWCEDiffuseShadeMaterialsFactory(const DataHeatBalance::MaterialProperties & t_Material,
                                         const FenestrationCommon::WavelengthRange t_Range);

    private:
        std::shared_ptr<SingleLayerOptics::CMaterialSingleBand> createVisibleRangeMaterial() override;
        std::shared_ptr<SingleLayerOptics::CMaterialSingleBand> createSolarRangeMaterial() override;
    };

    ///////////////////////////////////////////////////////////////////////////////
    //   CWCECellFactory
    ///////////////////////////////////////////////////////////////////////////////
    class IWCECellDescriptionFactory
    {
    public:
        virtual ~IWCECellDescriptionFactory() = default;
        IWCECellDescriptionFactory(const DataHeatBalance::MaterialProperties & t_Material);

        virtual std::shared_ptr<SingleLayerOptics::ICellDescription> getCellDescription() = 0;

    protected:
        DataHeatBalance::MaterialProperties m_Material;
    };

    ///////////////////////////////////////////////////////////////////////////////
    //   CWCESpecularCellFactory
    ///////////////////////////////////////////////////////////////////////////////
    class CWCESpecularCellFactory : public IWCECellDescriptionFactory
    {
    public:
        explicit CWCESpecularCellFactory(const DataHeatBalance::MaterialProperties & t_Material);

        std::shared_ptr<SingleLayerOptics::ICellDescription> getCellDescription() override;
    };

    ///////////////////////////////////////////////////////////////////////////////
    //   CWCEVenetianBlindCellFactory
    ///////////////////////////////////////////////////////////////////////////////
    class CWCEVenetianBlindCellFactory : public IWCECellDescriptionFactory
    {
    public:
        CWCEVenetianBlindCellFactory(const DataHeatBalance::MaterialProperties & t_Material);

        std::shared_ptr<SingleLayerOptics::ICellDescription> getCellDescription() override;
    };

    ///////////////////////////////////////////////////////////////////////////////
    //   CWCEScreenCellFactory
    ///////////////////////////////////////////////////////////////////////////////
    class CWCEScreenCellFactory : public IWCECellDescriptionFactory
    {
    public:
        CWCEScreenCellFactory(const DataHeatBalance::MaterialProperties & t_Material);

        std::shared_ptr<SingleLayerOptics::ICellDescription> getCellDescription() override;
    };

    ///////////////////////////////////////////////////////////////////////////////
    //   CWCEDiffuseShadeCellFactory
    ///////////////////////////////////////////////////////////////////////////////
    class CWCEDiffuseShadeCellFactory : public IWCECellDescriptionFactory
    {
    public:
        CWCEDiffuseShadeCellFactory(const DataHeatBalance::MaterialProperties & t_Material);

        std::shared_ptr<SingleLayerOptics::ICellDescription> getCellDescription() override;
    };

    ///////////////////////////////////////////////////////////////////////////////
    //   CWCELayerFactory
    ///////////////////////////////////////////////////////////////////////////////
    class CWCELayerFactory
    {
    public:
        virtual ~CWCELayerFactory() = default;
        CWCELayerFactory(const DataHeatBalance::MaterialProperties & t_Material, 
            const FenestrationCommon::WavelengthRange t_Range);

        std::shared_ptr<SingleLayerOptics::CBSDFLayer> getBSDFLayer();
        std::shared_ptr<SingleLayerOptics::CScatteringLayer> getLayer();

    protected:
        // void init();
        std::pair<std::shared_ptr<SingleLayerOptics::CMaterial>, std::shared_ptr<SingleLayerOptics::ICellDescription>> init();

        virtual void createMaterialFactory() = 0;
        std::shared_ptr<SingleLayerOptics::ICellDescription> getCellDescription() const;

        const DataHeatBalance::MaterialProperties m_Material;
        const FenestrationCommon::WavelengthRange m_Range;
        bool m_BSDFInitialized;
        bool m_SimpleInitialized;

        std::shared_ptr<CWCEMaterialFactory> m_MaterialFactory;
        std::shared_ptr<IWCECellDescriptionFactory> m_CellFactory;
        std::shared_ptr<SingleLayerOptics::CBSDFLayer> m_BSDFLayer;
        std::shared_ptr<SingleLayerOptics::CScatteringLayer> m_ScatteringLayer;
    };

    ///////////////////////////////////////////////////////////////////////////////
    //   CWCESpecularLayerFactory
    ///////////////////////////////////////////////////////////////////////////////
    class CWCESpecularLayerFactory : public CWCELayerFactory
    {
    public:
        CWCESpecularLayerFactory(const DataHeatBalance::MaterialProperties & t_Material,
                                 const FenestrationCommon::WavelengthRange t_Range);

    private:
        void createMaterialFactory() override;
    };

    ///////////////////////////////////////////////////////////////////////////////
    //   CWCEVenetianBlindLayerFactory
    ///////////////////////////////////////////////////////////////////////////////
    class CWCEVenetianBlindLayerFactory : public CWCELayerFactory
    {
    public:
        CWCEVenetianBlindLayerFactory(const DataHeatBalance::MaterialProperties & t_Material,
                                      const FenestrationCommon::WavelengthRange t_Range);

    private:
        void createMaterialFactory() override;
    };

    ///////////////////////////////////////////////////////////////////////////////
    //   CWCEScreenLayerFactory
    ///////////////////////////////////////////////////////////////////////////////
    class CWCEScreenLayerFactory : public CWCELayerFactory
    {
    public:
        CWCEScreenLayerFactory(const DataHeatBalance::MaterialProperties & t_Material,
                               const FenestrationCommon::WavelengthRange t_Range);

    private:
        void createMaterialFactory() override;
    };

    ///////////////////////////////////////////////////////////////////////////////
    //   CWCEDiffuseShadeLayerFactory
    ///////////////////////////////////////////////////////////////////////////////
    class CWCEDiffuseShadeLayerFactory : public CWCELayerFactory
    {
    public:
        CWCEDiffuseShadeLayerFactory(const DataHeatBalance::MaterialProperties & t_Material,
                                     const FenestrationCommon::WavelengthRange t_Range);

    private:
        void createMaterialFactory() override;
    };

} // namespace WindowManager

} // namespace EnergyPlus

#endif

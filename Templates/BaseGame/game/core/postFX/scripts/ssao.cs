///
$SSAOPostFx::intensity = 5.0;
$SSAOPostFx::radius = 1.0;
$SSAOPostFx::scale = 1.0;
$SSAOPostFx::bias = 0.35;

///
$SSAOPostFx::targetScale = "1 1";


function SSAOPostFx::onAdd( %this )
{
    %this.wasVis = "Uninitialized";
}

function SSAOPostFx::preProcess( %this )
{
    %this.targetScale = $SSAOPostFx::targetScale;
}

function SSAOPostFx::setShaderConsts( %this )
{
    %this.setShaderConst( "$cameraFOV",      $Pref::Player::DefaultFOV );

    %ao = %this->ao;
    %ao.setShaderConst( "$intensity", $SSAOPostFx::intensity );
    %ao.setShaderConst( "$radius",    $SSAOPostFx::radius );
    %ao.setShaderConst( "$scale",     $SSAOPostFx::scale );
    %ao.setShaderConst( "$bias",      $SSAOPostFx::bias );
}

function SSAOPostFx::onEnabled( %this )
{
    // This tells the AL shaders to reload and sample
    // from our #ssaoMask texture target.
    $AL::UseSSAOMask = true;

    return true;
}

function SSAOPostFx::onDisabled( %this )
{
    $AL::UseSSAOMask = false;
}

//-----------------------------------------------------------------------------
// GFXStateBlockData / ShaderData
//-----------------------------------------------------------------------------
singleton GFXStateBlockData( SSAOStateBlock : PFX_DefaultStateBlock )
{
    samplersDefined = true;
    samplerStates[0] = SamplerClampPoint;
    samplerStates[1] = SamplerClampPoint;
    samplerStates[2] = SamplerWrapLinear;
};

singleton GFXStateBlockData( SSAOPosStateBlock : PFX_DefaultStateBlock )
{
    samplersDefined = true;
    samplerStates[0] = SamplerClampPoint;
};

singleton ShaderData( SSAOShader )
{
    DXVertexShaderFile 	= $Core::CommonShaderPath @ "/postFX/postFxV.hlsl";
    DXPixelShaderFile 	= $Core::CommonShaderPath @ "/postFX/SSAO/SSAO_P.hlsl";
    pixVersion = 3.0;
};

singleton ShaderData( SSAOPosShader )
{
    DXVertexShaderFile 	= $Core::CommonShaderPath @ "/postFX/SSAO/SSAO_Pos_V.hlsl";
    DXPixelShaderFile 	= $Core::CommonShaderPath @ "/postFX/SSAO/SSAO_Pos_P.hlsl";
    pixVersion = 3.0;
};

//-----------------------------------------------------------------------------
// PostEffects
//-----------------------------------------------------------------------------

singleton PostEffect( SSAOPostFx )
{
    allowReflectPass = false;

    renderTime = "PFXBeforeBin";
    renderBin = "ProbeBin";
    renderPriority = 10;

    shader = SSAOPosShader;
    stateBlock = SSAOPosStateBlock;

    texture[0] = "#deferred";

    target = "$outTex";
    targetFormat = "GFXFormatR16G16B16A16F";
    targetScale = "0.5 0.5";

    singleton PostEffect()
    {
        internalName = "ao";

        shader = SSAOShader;
        stateBlock = SSAOStateBlock;
        
        texture[0] = "$inTex";
        texture[1] = "#deferred";
        texture[2] = "core/postFX/images/randNorm.png";

        target = "#ssaoMask";
    };

};

/// Just here for debug visualization of the
/// SSAO mask texture used during lighting.
singleton PostEffect( SSAOVizPostFx )
{
    allowReflectPass = false;

    shader = PFX_PassthruShader;
    stateBlock = PFX_DefaultStateBlock;

    texture[0] = "#ssaoMask";

    target = "$backbuffer";
};

/*
 * REVIEWED RECONSTRUCTION of W:\SWJediPowerBattles\work\d3d\d3derr.cpp.
 * PDB module: 0022
 * Object: W:\SWJediPowerBattles\winver\obj\x64\Steam_Release\d3derr.obj
 * Primary source: W:\SWJediPowerBattles\work\d3d\d3derr.cpp
 * Compiler language: c++
 * Emitted procedures: 2
 *
 * Use inventory/function_map.tsv with ExportReconstruction.java.
 */

#include "jpb/d3derr.h"

#include <cstdio>
#include <windows.h>

#define DD_ERROR(code, text) {code, const_cast<char *>(text)}

_d3derr alldderrs[199] = {
    DD_ERROR(0x88760005u, "Alreadyinitialized"),
    DD_ERROR(0x8876000au, "Cannotattachsurface"),
    DD_ERROR(0x88760014u, "Cannotdetachsurface"),
    DD_ERROR(0x88760028u, "Currentlynotavail"),
    DD_ERROR(0x88760037u, "Exception"),
    DD_ERROR(0x80004005u, "Generic"),
    DD_ERROR(0x8876005au, "Heightalign"),
    DD_ERROR(0x8876005fu, "Incompatibleprimary"),
    DD_ERROR(0x88760064u, "Invalidcaps"),
    DD_ERROR(0x8876006eu, "Invalidcliplist"),
    DD_ERROR(0x88760078u, "Invalidmode"),
    DD_ERROR(0x88760082u, "Invalidobject"),
    DD_ERROR(0x80070057u, "Invalidparams"),
    DD_ERROR(0x88760091u, "Invalidpixelformat"),
    DD_ERROR(0x88760096u, "Invalidrect"),
    DD_ERROR(0x887600a0u, "Lockedsurfaces"),
    DD_ERROR(0x887600aau, "No3d"),
    DD_ERROR(0x887600b4u, "Noalphahw"),
    DD_ERROR(0x887600b5u, "Nostereohardware"),
    DD_ERROR(0x887600b6u, "Nosurfaceleft"),
    DD_ERROR(0x887600cdu, "Nocliplist"),
    DD_ERROR(0x887600d2u, "Nocolorconvhw"),
    DD_ERROR(0x887600d4u, "Nocooperativelevelset"),
    DD_ERROR(0x887600d7u, "Nocolorkey"),
    DD_ERROR(0x887600dcu, "Nocolorkeyhw"),
    DD_ERROR(0x887600deu, "Nodirectdrawsupport"),
    DD_ERROR(0x887600e1u, "Noexclusivemode"),
    DD_ERROR(0x887600e6u, "Nofliphw"),
    DD_ERROR(0x887600f0u, "Nogdi"),
    DD_ERROR(0x887600fau, "Nomirrorhw"),
    DD_ERROR(0x887600ffu, "Notfound"),
    DD_ERROR(0x88760104u, "Nooverlayhw"),
    DD_ERROR(0x8876010eu, "Overlappingrects"),
    DD_ERROR(0x88760118u, "Norasterophw"),
    DD_ERROR(0x88760122u, "Norotationhw"),
    DD_ERROR(0x88760136u, "Nostretchhw"),
    DD_ERROR(0x8876013cu, "Not4bitcolor"),
    DD_ERROR(0x8876013du, "Not4bitcolorindex"),
    DD_ERROR(0x88760140u, "Not8bitcolor"),
    DD_ERROR(0x8876014au, "Notexturehw"),
    DD_ERROR(0x8876014fu, "Novsynchw"),
    DD_ERROR(0x88760154u, "Nozbufferhw"),
    DD_ERROR(0x8876015eu, "Nozoverlayhw"),
    DD_ERROR(0x88760168u, "Outofcaps"),
    DD_ERROR(0x8007000eu, "Outofmemory"),
    DD_ERROR(0x8876017cu, "Outofvideomemory"),
    DD_ERROR(0x8876017eu, "Overlaycantclip"),
    DD_ERROR(0x88760180u, "Overlaycolorkeyonlyoneactive"),
    DD_ERROR(0x88760183u, "Palettebusy"),
    DD_ERROR(0x88760190u, "Colorkeynotset"),
    DD_ERROR(0x8876019au, "Surfacealreadyattached"),
    DD_ERROR(0x887601a4u, "Surfacealreadydependent"),
    DD_ERROR(0x887601aeu, "Surfacebusy"),
    DD_ERROR(0x887601b3u, "Cantlocksurface"),
    DD_ERROR(0x887601b8u, "Surfaceisobscured"),
    DD_ERROR(0x887601c2u, "Surfacelost"),
    DD_ERROR(0x887601ccu, "Surfacenotattached"),
    DD_ERROR(0x887601d6u, "Toobigheight"),
    DD_ERROR(0x887601e0u, "Toobigsize"),
    DD_ERROR(0x887601eau, "Toobigwidth"),
    DD_ERROR(0x80004001u, "Unsupported"),
    DD_ERROR(0x887601feu, "Unsupportedformat"),
    DD_ERROR(0x88760208u, "Unsupportedmask"),
    DD_ERROR(0x88760209u, "Invalidstream"),
    DD_ERROR(0x88760219u, "Verticalblankinprogress"),
    DD_ERROR(0x8876021cu, "Wasstilldrawing"),
    DD_ERROR(0x8876021eu, "Ddscapscomplexrequired"),
    DD_ERROR(0x88760230u, "Xalign"),
    DD_ERROR(0x88760231u, "Invaliddirectdrawguid"),
    DD_ERROR(0x88760232u, "Directdrawalreadycreated"),
    DD_ERROR(0x88760233u, "Nodirectdrawhw"),
    DD_ERROR(0x88760234u, "Primarysurfacealreadyexists"),
    DD_ERROR(0x88760235u, "Noemulation"),
    DD_ERROR(0x88760236u, "Regiontoosmall"),
    DD_ERROR(0x88760237u, "Clipperisusinghwnd"),
    DD_ERROR(0x88760238u, "Noclipperattached"),
    DD_ERROR(0x88760239u, "Nohwnd"),
    DD_ERROR(0x8876023au, "Hwndsubclassed"),
    DD_ERROR(0x8876023bu, "Hwndalreadyset"),
    DD_ERROR(0x8876023cu, "Nopaletteattached"),
    DD_ERROR(0x8876023du, "Nopalettehw"),
    DD_ERROR(0x8876023eu, "Bltfastcantclip"),
    DD_ERROR(0x8876023fu, "Noblthw"),
    DD_ERROR(0x88760240u, "Noddropshw"),
    DD_ERROR(0x88760241u, "Overlaynotvisible"),
    DD_ERROR(0x88760242u, "Nooverlaydest"),
    DD_ERROR(0x88760243u, "Invalidposition"),
    DD_ERROR(0x88760244u, "Notaoverlaysurface"),
    DD_ERROR(0x88760245u, "Exclusivemodealreadyset"),
    DD_ERROR(0x88760246u, "Notflippable"),
    DD_ERROR(0x88760247u, "Cantduplicate"),
    DD_ERROR(0x88760248u, "Notlocked"),
    DD_ERROR(0x88760249u, "Cantcreatedc"),
    DD_ERROR(0x8876024au, "Nodc"),
    DD_ERROR(0x8876024bu, "Wrongmode"),
    DD_ERROR(0x8876024cu, "Implicitlycreated"),
    DD_ERROR(0x8876024du, "Notpalettized"),
    DD_ERROR(0x8876024eu, "Unsupportedmode"),
    DD_ERROR(0x8876024fu, "Nomipmaphw"),
    DD_ERROR(0x88760250u, "Invalidsurfacetype"),
    DD_ERROR(0x88760258u, "Nooptimizehw"),
    DD_ERROR(0x88760259u, "Notloaded"),
    DD_ERROR(0x8876025au, "Nofocuswindow"),
    DD_ERROR(0x8876025bu, "Notonmipmapsublevel"),
    DD_ERROR(0x8876026cu, "Dcalreadycreated"),
    DD_ERROR(0x88760276u, "Nononlocalvidmem"),
    DD_ERROR(0x88760280u, "Cantpagelock"),
    DD_ERROR(0x88760294u, "Cantpageunlock"),
    DD_ERROR(0x887602a8u, "Notpagelocked"),
    DD_ERROR(0x887602b2u, "Moredata"),
    DD_ERROR(0x887602b3u, "Expired"),
    DD_ERROR(0x887602b4u, "Testfinished"),
    DD_ERROR(0x887602b5u, "Newmode"),
    DD_ERROR(0x887602b6u, "D3dnotinitialized"),
    DD_ERROR(0x887602b7u, "Videonotactive"),
    DD_ERROR(0x887602b8u, "Nomonitorinformation"),
    DD_ERROR(0x887602b9u, "Nodriversupport"),
    DD_ERROR(0x887602bbu, "Devicedoesntownsurface"),
    DD_ERROR(0x800401f0u, "Notinitialized"),
    DD_ERROR(0x887602bcu, "Badmajorversion"),
    DD_ERROR(0x887602bdu, "Badminorversion"),
    DD_ERROR(0x887602c1u, "Invalid_device"),
    DD_ERROR(0x887602c2u, "Initfailed"),
    DD_ERROR(0x887602c3u, "Deviceaggregated"),
    DD_ERROR(0x887602c6u, "Execute_create_failed"),
    DD_ERROR(0x887602c7u, "Execute_destroy_failed"),
    DD_ERROR(0x887602c8u, "Execute_lock_failed"),
    DD_ERROR(0x887602c9u, "Execute_unlock_failed"),
    DD_ERROR(0x887602cau, "Execute_locked"),
    DD_ERROR(0x887602cbu, "Execute_not_locked"),
    DD_ERROR(0x887602ccu, "Execute_failed"),
    DD_ERROR(0x887602cdu, "Execute_clipped_failed"),
    DD_ERROR(0x887602d0u, "Texture_no_support"),
    DD_ERROR(0x887602d1u, "Texture_create_failed"),
    DD_ERROR(0x887602d2u, "Texture_destroy_failed"),
    DD_ERROR(0x887602d3u, "Texture_lock_failed"),
    DD_ERROR(0x887602d4u, "Texture_unlock_failed"),
    DD_ERROR(0x887602d5u, "Texture_load_failed"),
    DD_ERROR(0x887602d6u, "Texture_swap_failed"),
    DD_ERROR(0x887602d7u, "Texture_locked"),
    DD_ERROR(0x887602d8u, "Texture_not_locked"),
    DD_ERROR(0x887602d9u, "Texture_getsurf_failed"),
    DD_ERROR(0x887602dau, "Matrix_create_failed"),
    DD_ERROR(0x887602dbu, "Matrix_destroy_failed"),
    DD_ERROR(0x887602dcu, "Matrix_setdata_failed"),
    DD_ERROR(0x887602ddu, "Matrix_getdata_failed"),
    DD_ERROR(0x887602deu, "Setviewportdata_failed"),
    DD_ERROR(0x887602dfu, "Invalidcurrentviewport"),
    DD_ERROR(0x887602e0u, "Invalidprimitivetype"),
    DD_ERROR(0x887602e1u, "Invalidvertextype"),
    DD_ERROR(0x887602e2u, "Texture_badsize"),
    DD_ERROR(0x887602e3u, "Invalidramptexture"),
    DD_ERROR(0x887602e4u, "Material_create_failed"),
    DD_ERROR(0x887602e5u, "Material_destroy_failed"),
    DD_ERROR(0x887602e6u, "Material_setdata_failed"),
    DD_ERROR(0x887602e7u, "Material_getdata_failed"),
    DD_ERROR(0x887602e8u, "Invalidpalette"),
    DD_ERROR(0x887602e9u, "Zbuff_needs_systemmemory"),
    DD_ERROR(0x887602eau, "Zbuff_needs_videomemory"),
    DD_ERROR(0x887602ebu, "Surfacenotinvidmem"),
    DD_ERROR(0x887602eeu, "Light_set_failed"),
    DD_ERROR(0x887602efu, "Lighthasviewport"),
    DD_ERROR(0x887602f0u, "Lightnotinthisviewport"),
    DD_ERROR(0x887602f8u, "Scene_in_scene"),
    DD_ERROR(0x887602f9u, "Scene_not_in_scene"),
    DD_ERROR(0x887602fau, "Scene_begin_failed"),
    DD_ERROR(0x887602fbu, "Scene_end_failed"),
    DD_ERROR(0x88760302u, "Inbegin"),
    DD_ERROR(0x88760303u, "Notinbegin"),
    DD_ERROR(0x88760304u, "Noviewports"),
    DD_ERROR(0x88760305u, "Viewportdatanotset"),
    DD_ERROR(0x88760306u, "Viewporthasnodevice"),
    DD_ERROR(0x88760307u, "Nocurrentviewport"),
    DD_ERROR(0x88760800u, "Invalidvertexformat"),
    DD_ERROR(0x88760802u, "Colorkeyattached"),
    DD_ERROR(0x8876080cu, "Vertexbufferoptimized"),
    DD_ERROR(0x8876080du, "Vbuf_create_failed"),
    DD_ERROR(0x8876080eu, "Vertexbufferlocked"),
    DD_ERROR(0x8876080fu, "Vertexbufferunlockfailed"),
    DD_ERROR(0x88760816u, "Zbuffer_notpresent"),
    DD_ERROR(0x88760817u, "Stencilbuffer_notpresent"),
    DD_ERROR(0x88760818u, "Wrongtextureformat"),
    DD_ERROR(0x88760819u, "Unsupportedcoloroperation"),
    DD_ERROR(0x8876081au, "Unsupportedcolorarg"),
    DD_ERROR(0x8876081bu, "Unsupportedalphaoperation"),
    DD_ERROR(0x8876081cu, "Unsupportedalphaarg"),
    DD_ERROR(0x8876081du, "Toomanyoperations"),
    DD_ERROR(0x8876081eu, "Conflictingtexturefilter"),
    DD_ERROR(0x8876081fu, "Unsupportedfactorvalue"),
    DD_ERROR(0x88760821u, "Conflictingrenderstate"),
    DD_ERROR(0x88760822u, "Unsupportedtexturefilter"),
    DD_ERROR(0x88760823u, "Toomanyprimitives"),
    DD_ERROR(0x88760824u, "Invalidmatrix"),
    DD_ERROR(0x88760825u, "Toomanyvertices"),
    DD_ERROR(0x88760826u, "Conflictingtexturepalette"),
    DD_ERROR(0x88760834u, "Invalidstateblock"),
    DD_ERROR(0x88760835u, "Inbeginstateblock"),
    DD_ERROR(0x88760836u, "Notinbeginstateblock"),
    {0, nullptr}
};

#undef DD_ERROR

static char unknown_error[] = "?Noerror?";

/* 0x3AE50, 171 bytes, global, 7 named locals
 * d3derr
 * PDB type: void (unsigned, char*, unsigned,...
 * Source: W:\SWJediPowerBattles\work\d3d\d3derr.cpp
 */
void d3derr(unsigned hr, char *msg, unsigned line, char *file)
{
    char buffer[256];
    char *error_message = unknown_error;
    int index = 0;

    (void)msg;
    while (alldderrs[index].err != 0) {
        if (alldderrs[index].err == hr) {
            error_message = alldderrs[index].errmsg;
            break;
        }
        ++index;
    }
    std::sprintf(
        buffer,
        "%s(%d): err %d - %s\n",
        file,
        line,
        hr & 0x7fffu,
        error_message);
    OutputDebugStringA(buffer);
}

/* 0x3AF00, 67 bytes, global, 3 named locals
 * dderrmsg
 * PDB type: char* (unsigned)
 * Source: W:\SWJediPowerBattles\work\d3d\d3derr.cpp
 */
char *dderrmsg(unsigned hr)
{
    int index = 0;

    while (alldderrs[index].err != 0) {
        if (alldderrs[index].err == hr) {
            return alldderrs[index].errmsg;
        }
        ++index;
    }
    return unknown_error;
}

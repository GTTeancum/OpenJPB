/*
 * Sparse Streets JPX-to-FBX ownership map.
 *
 * Generated from the shipped STREETS.jpx and streets.fbx by the optional
 * jpb_fbx_jpx_match_probe using the executable's exact ufbx 0.6.1 version.
 * No game asset or FBX parser is embedded here: records identify only JPX
 * patch offsets/triangle ranges and their cullmesh ownership masks. The map
 * is enabled only after an exact whole-file FNV-1a fingerprint match.
 */

#include "jpb/streets_cull_map.h"

#include "jpb/cube.h"

#include <stddef.h>
#include <string.h>

typedef struct JPBStreetsCullRange {
    uint32_t siteOffset;
    uint16_t firstTriangle;
    uint16_t lastTriangle;
    uint32_t cullMask;
} JPBStreetsCullRange;

static const JPBStreetsCullRange streetsCullRanges[] = {
    {2195324u, 0u, 5u, UINT32_C(0x00000040)},
    {2195496u, 0u, 5u, UINT32_C(0x00000100)},
    {2428480u, 0u, 1u, UINT32_C(0x00100000)},
    {3351232u, 0u, 1u, UINT32_C(0x00000006)},
    {3351232u, 2u, 2u, UINT32_C(0x00000004)},
    {3351232u, 4u, 5u, UINT32_C(0x00000006)},
    {3351232u, 6u, 6u, UINT32_C(0x00000004)},
    {3351232u, 7u, 9u, UINT32_C(0x00000006)},
    {3351232u, 11u, 16u, UINT32_C(0x00000006)},
    {3351580u, 0u, 1u, UINT32_C(0x00000006)},
    {3351580u, 4u, 6u, UINT32_C(0x00000006)},
    {3351580u, 7u, 7u, UINT32_C(0x00000004)},
    {3351580u, 8u, 12u, UINT32_C(0x00000006)},
    {3351864u, 0u, 5u, UINT32_C(0x00000004)},
    {3352036u, 0u, 5u, UINT32_C(0x00000004)},
    {3352208u, 0u, 2u, UINT32_C(0x00000006)},
    {3352208u, 4u, 14u, UINT32_C(0x00000006)},
    {3352524u, 0u, 0u, UINT32_C(0x00000006)},
    {3352616u, 0u, 1u, UINT32_C(0x00000010)},
    {3352724u, 0u, 1u, UINT32_C(0x00000010)},
    {3352724u, 4u, 5u, UINT32_C(0x00000010)},
    {3352896u, 0u, 1u, UINT32_C(0x00000040)},
    {3353004u, 0u, 0u, UINT32_C(0x00000060)},
    {3353004u, 1u, 1u, UINT32_C(0x00000040)},
    {3353112u, 0u, 5u, UINT32_C(0x00000100)},
    {3353284u, 0u, 5u, UINT32_C(0x00000100)},
    {3353456u, 0u, 5u, UINT32_C(0x00000040)},
    {3353628u, 0u, 5u, UINT32_C(0x00000100)},
    {3353800u, 0u, 1u, UINT32_C(0x00000100)},
    {3353908u, 0u, 1u, UINT32_C(0x00000100)},
    {3354016u, 0u, 8u, UINT32_C(0x00001800)},
    {3354016u, 9u, 10u, UINT32_C(0x00001000)},
    {3354016u, 11u, 12u, UINT32_C(0x00001800)},
    {3354300u, 0u, 1u, UINT32_C(0x00001800)},
    {3354300u, 2u, 3u, UINT32_C(0x00001000)},
    {3354300u, 4u, 8u, UINT32_C(0x00001800)},
    {3354520u, 0u, 2u, UINT32_C(0x00000600)},
    {3354520u, 4u, 15u, UINT32_C(0x00000600)},
    {3354852u, 0u, 1u, UINT32_C(0x00000600)},
    {3354852u, 2u, 3u, UINT32_C(0x00000400)},
    {3354852u, 4u, 5u, UINT32_C(0x00000600)},
    {3354852u, 6u, 6u, UINT32_C(0x00000400)},
    {3354852u, 7u, 16u, UINT32_C(0x00000600)},
    {3354852u, 18u, 23u, UINT32_C(0x00000600)},
    {3355312u, 0u, 2u, UINT32_C(0x00000600)},
    {3355312u, 3u, 3u, UINT32_C(0x00000400)},
    {3355312u, 4u, 4u, UINT32_C(0x00000600)},
    {3355312u, 6u, 6u, UINT32_C(0x00000400)},
    {3355312u, 7u, 7u, UINT32_C(0x00000600)},
    {3355516u, 0u, 0u, UINT32_C(0x00000600)},
    {3355516u, 1u, 2u, UINT32_C(0x00000400)},
    {3355516u, 3u, 4u, UINT32_C(0x00000600)},
    {3355516u, 7u, 8u, UINT32_C(0x00000600)},
    {3355736u, 0u, 4u, UINT32_C(0x00001800)},
    {3355736u, 6u, 7u, UINT32_C(0x00001800)},
    {3355940u, 0u, 1u, UINT32_C(0x00001800)},
    {3355940u, 2u, 2u, UINT32_C(0x00001000)},
    {3355940u, 4u, 4u, UINT32_C(0x00001800)},
    {3355940u, 5u, 6u, UINT32_C(0x00001000)},
    {3355940u, 7u, 8u, UINT32_C(0x00001800)},
    {3355940u, 10u, 10u, UINT32_C(0x00001800)},
    {3356192u, 0u, 0u, UINT32_C(0x00006000)},
    {3356192u, 1u, 2u, UINT32_C(0x00002000)},
    {3356192u, 3u, 4u, UINT32_C(0x00006000)},
    {3356192u, 5u, 6u, UINT32_C(0x00002000)},
    {3356192u, 7u, 9u, UINT32_C(0x00006000)},
    {3356192u, 10u, 13u, UINT32_C(0x00002000)},
    {3356192u, 14u, 14u, UINT32_C(0x00006000)},
    {3356192u, 15u, 15u, UINT32_C(0x00002000)},
    {3356192u, 16u, 16u, UINT32_C(0x00006000)},
    {3356192u, 17u, 17u, UINT32_C(0x00002000)},
    {3356192u, 18u, 19u, UINT32_C(0x00006000)},
    {3356192u, 20u, 20u, UINT32_C(0x00002000)},
    {3356192u, 21u, 22u, UINT32_C(0x00006000)},
    {3356192u, 23u, 24u, UINT32_C(0x00002000)},
    {3356192u, 25u, 26u, UINT32_C(0x00006000)},
    {3356192u, 27u, 29u, UINT32_C(0x00002000)},
    {3356192u, 30u, 31u, UINT32_C(0x00006000)},
    {3356780u, 0u, 1u, UINT32_C(0x00006000)},
    {3356888u, 0u, 1u, UINT32_C(0x00180000)},
    {3356888u, 2u, 3u, UINT32_C(0x00100000)},
    {3356888u, 4u, 4u, UINT32_C(0x00180000)},
    {3356888u, 5u, 5u, UINT32_C(0x00100000)},
    {3356888u, 7u, 7u, UINT32_C(0x00180000)},
    {3357092u, 0u, 1u, UINT32_C(0x00180000)},
    {3357092u, 2u, 3u, UINT32_C(0x00100000)},
    {3357092u, 4u, 4u, UINT32_C(0x00180000)},
    {3357248u, 0u, 0u, UINT32_C(0x00018000)},
    {3357248u, 1u, 2u, UINT32_C(0x00008000)},
    {3357248u, 3u, 3u, UINT32_C(0x00010000)},
    {3357248u, 4u, 9u, UINT32_C(0x00018000)},
    {3357248u, 10u, 16u, UINT32_C(0x00008000)},
    {3357248u, 17u, 18u, UINT32_C(0x00018000)},
    {3357248u, 20u, 21u, UINT32_C(0x00018000)},
    {3357676u, 0u, 0u, UINT32_C(0x00006000)},
    {3357768u, 0u, 5u, UINT32_C(0x00600000)},
    {3357768u, 7u, 9u, UINT32_C(0x00600000)},
    {3357768u, 10u, 10u, UINT32_C(0x00400000)},
    {3357768u, 11u, 12u, UINT32_C(0x00600000)},
    {3358052u, 0u, 1u, UINT32_C(0x00600000)},
    {3358052u, 3u, 4u, UINT32_C(0x00600000)},
    {3358052u, 6u, 7u, UINT32_C(0x00600000)},
    {3358256u, 0u, 1u, UINT32_C(0x00008000)},
    {3358256u, 4u, 5u, UINT32_C(0x00018000)},
    {3358428u, 0u, 8u, UINT32_C(0x00600000)},
    {3358648u, 0u, 2u, UINT32_C(0x00018000)},
    {3358648u, 3u, 3u, UINT32_C(0x00010000)},
    {3358648u, 4u, 5u, UINT32_C(0x00018000)},
    {3358648u, 6u, 6u, UINT32_C(0x00010000)},
    {3358648u, 8u, 9u, UINT32_C(0x00018000)},
    {3358648u, 10u, 11u, UINT32_C(0x00010000)},
    {3358648u, 12u, 14u, UINT32_C(0x00008000)},
    {3358648u, 15u, 17u, UINT32_C(0x00018000)},
    {3359012u, 0u, 2u, UINT32_C(0x00600000)},
    {3359012u, 4u, 7u, UINT32_C(0x00600000)},
    {3359012u, 9u, 10u, UINT32_C(0x00600000)},
    {3359264u, 0u, 1u, UINT32_C(0x00060000)},
    {3359264u, 3u, 9u, UINT32_C(0x00060000)},
    {3359264u, 10u, 10u, UINT32_C(0x00040000)},
    {3359264u, 11u, 14u, UINT32_C(0x00060000)},
    {3359264u, 15u, 16u, UINT32_C(0x00020000)},
    {3359264u, 17u, 20u, UINT32_C(0x00060000)},
    {3359264u, 21u, 21u, UINT32_C(0x00040000)},
    {3359264u, 22u, 22u, UINT32_C(0x00060000)},
    {3359264u, 23u, 23u, UINT32_C(0x00020000)},
    {3359264u, 24u, 24u, UINT32_C(0x00060000)},
    {3359264u, 25u, 25u, UINT32_C(0x00040000)},
    {3359264u, 26u, 32u, UINT32_C(0x00060000)},
    {3359264u, 33u, 33u, UINT32_C(0x00020000)},
    {3359264u, 34u, 36u, UINT32_C(0x00060000)},
    {3359932u, 0u, 5u, UINT32_C(0x00040000)},
    {3360104u, 0u, 1u, UINT32_C(0x01800000)},
    {3360104u, 3u, 5u, UINT32_C(0x01800000)},
    {3360104u, 6u, 6u, UINT32_C(0x01000000)},
    {3360104u, 7u, 12u, UINT32_C(0x01800000)},
    {3360104u, 13u, 14u, UINT32_C(0x01000000)},
    {3360104u, 15u, 15u, UINT32_C(0x01800000)},
    {3360104u, 16u, 17u, UINT32_C(0x01000000)},
    {3360104u, 18u, 24u, UINT32_C(0x01800000)},
    {3360580u, 0u, 0u, UINT32_C(0x01800000)},
    {3360580u, 1u, 2u, UINT32_C(0x01000000)},
    {3360580u, 3u, 3u, UINT32_C(0x01800000)},
    {3360580u, 6u, 7u, UINT32_C(0x01800000)},
    {3360784u, 0u, 1u, UINT32_C(0x00000060)},
    {3360784u, 2u, 3u, UINT32_C(0x00000020)},
    {3360784u, 4u, 5u, UINT32_C(0x00000060)},
    {3360784u, 7u, 8u, UINT32_C(0x00000060)},
    {3360784u, 9u, 9u, UINT32_C(0x00000040)},
    {3360784u, 10u, 11u, UINT32_C(0x00000060)},
    {3361052u, 0u, 0u, UINT32_C(0x00000180)},
    {3361052u, 1u, 3u, UINT32_C(0x00000080)},
    {3361052u, 4u, 4u, UINT32_C(0x00000180)},
    {3361052u, 7u, 7u, UINT32_C(0x00000180)},
    {3361256u, 0u, 5u, UINT32_C(0x00000004)},
    {3361428u, 0u, 5u, UINT32_C(0x00000004)},
    {3361600u, 0u, 1u, UINT32_C(0x00000004)},
    {3361708u, 0u, 1u, UINT32_C(0x00000004)},
    {3361816u, 0u, 0u, UINT32_C(0x00000018)},
    {3361816u, 1u, 2u, UINT32_C(0x00000010)},
    {3361816u, 3u, 4u, UINT32_C(0x00000018)},
    {3361816u, 7u, 8u, UINT32_C(0x00000018)},
    {3361816u, 9u, 9u, UINT32_C(0x00000010)},
    {3361816u, 10u, 10u, UINT32_C(0x00000018)},
    {3362068u, 0u, 1u, UINT32_C(0x00000018)},
    {3362068u, 2u, 3u, UINT32_C(0x00000010)},
    {3362068u, 4u, 5u, UINT32_C(0x00000018)},
    {3362068u, 8u, 9u, UINT32_C(0x00000018)},
    {3362068u, 10u, 11u, UINT32_C(0x00000010)},
    {3362068u, 12u, 12u, UINT32_C(0x00000018)},
    {3362068u, 13u, 14u, UINT32_C(0x00000010)},
    {3362068u, 15u, 15u, UINT32_C(0x00000018)},
    {3362068u, 16u, 17u, UINT32_C(0x00000010)},
    {3362068u, 18u, 18u, UINT32_C(0x00000018)},
    {3362068u, 19u, 20u, UINT32_C(0x00000010)},
    {3362068u, 21u, 21u, UINT32_C(0x00000018)},
    {3362496u, 0u, 5u, UINT32_C(0x00000060)},
    {3362496u, 7u, 8u, UINT32_C(0x00000060)},
    {3362716u, 0u, 12u, UINT32_C(0x00000060)},
    {3362716u, 15u, 15u, UINT32_C(0x00000060)},
    {3363048u, 0u, 0u, UINT32_C(0x00000180)},
    {3363140u, 0u, 2u, UINT32_C(0x00000180)},
    {3363140u, 3u, 3u, UINT32_C(0x00000100)},
    {3363140u, 4u, 4u, UINT32_C(0x00000180)},
    {3363140u, 5u, 6u, UINT32_C(0x00000100)},
    {3363140u, 7u, 8u, UINT32_C(0x00000180)},
    {3363360u, 0u, 1u, UINT32_C(0x00000060)},
    {3363360u, 2u, 2u, UINT32_C(0x00000040)},
    {3363360u, 3u, 5u, UINT32_C(0x00000060)},
    {3363360u, 8u, 12u, UINT32_C(0x00000060)},
    {3363360u, 13u, 13u, UINT32_C(0x00000040)},
    {3363360u, 14u, 18u, UINT32_C(0x00000060)},
    {3363740u, 0u, 1u, UINT32_C(0x00000180)},
    {3363740u, 4u, 5u, UINT32_C(0x00000180)},
    {3363740u, 7u, 8u, UINT32_C(0x00000180)},
    {3363960u, 0u, 5u, UINT32_C(0x00000180)},
    {3363960u, 6u, 6u, UINT32_C(0x00000100)},
    {3363960u, 7u, 9u, UINT32_C(0x00000180)},
    {3363960u, 12u, 13u, UINT32_C(0x00000180)},
    {3363960u, 14u, 15u, UINT32_C(0x00000100)},
    {3363960u, 16u, 16u, UINT32_C(0x00000180)},
    {3363960u, 17u, 18u, UINT32_C(0x00000100)},
    {3363960u, 19u, 19u, UINT32_C(0x00000180)},
    {3364356u, 0u, 5u, UINT32_C(0x00000180)},
    {3364356u, 8u, 12u, UINT32_C(0x00000180)},
    {3364640u, 0u, 1u, UINT32_C(0x00001000)},
    {3364640u, 4u, 5u, UINT32_C(0x00001000)},
    {3364812u, 0u, 11u, UINT32_C(0x00001000)},
    {3364812u, 12u, 12u, UINT32_C(0x00001800)},
    {3364812u, 13u, 13u, UINT32_C(0x00001000)},
    {3365112u, 0u, 0u, UINT32_C(0x00000600)},
    {3365112u, 1u, 1u, UINT32_C(0x00000400)},
    {3365220u, 0u, 0u, UINT32_C(0x00000400)},
    {3365220u, 1u, 1u, UINT32_C(0x00000600)},
    {3365220u, 4u, 5u, UINT32_C(0x00000400)},
    {3365392u, 0u, 5u, UINT32_C(0x00000400)},
    {3365564u, 0u, 13u, UINT32_C(0x00000400)},
    {3365864u, 0u, 0u, UINT32_C(0x00001800)},
    {3365864u, 1u, 1u, UINT32_C(0x00001000)},
    {3365972u, 0u, 0u, UINT32_C(0x00001800)},
    {3365972u, 1u, 1u, UINT32_C(0x00001000)},
    {3365972u, 4u, 5u, UINT32_C(0x00001000)},
    {3366144u, 0u, 1u, UINT32_C(0x00004000)},
    {3366144u, 4u, 9u, UINT32_C(0x00004000)},
    {3366380u, 0u, 1u, UINT32_C(0x00004000)},
    {3366488u, 0u, 1u, UINT32_C(0x00100000)},
    {3366596u, 0u, 1u, UINT32_C(0x00100000)},
    {3366704u, 0u, 1u, UINT32_C(0x00010000)},
    {3366704u, 4u, 9u, UINT32_C(0x00010000)},
    {3366940u, 0u, 1u, UINT32_C(0x00010000)},
    {3367048u, 0u, 0u, UINT32_C(0x00180000)},
    {3367048u, 1u, 2u, UINT32_C(0x00100000)},
    {3367048u, 3u, 7u, UINT32_C(0x00180000)},
    {3367252u, 0u, 0u, UINT32_C(0x00180000)},
    {3367252u, 1u, 2u, UINT32_C(0x00100000)},
    {3367252u, 3u, 3u, UINT32_C(0x00180000)},
    {3367392u, 0u, 5u, UINT32_C(0x00010000)},
    {3367564u, 0u, 1u, UINT32_C(0x00400000)},
    {3367564u, 4u, 5u, UINT32_C(0x00400000)},
    {3367736u, 0u, 0u, UINT32_C(0x00018000)},
    {3367736u, 1u, 1u, UINT32_C(0x00010000)},
    {3367736u, 2u, 3u, UINT32_C(0x00018000)},
    {3367736u, 4u, 5u, UINT32_C(0x00010000)},
    {3367736u, 7u, 10u, UINT32_C(0x00010000)},
    {3367736u, 11u, 12u, UINT32_C(0x00018000)},
    {3367736u, 13u, 13u, UINT32_C(0x00010000)},
    {3367736u, 14u, 14u, UINT32_C(0x00018000)},
    {3367736u, 15u, 17u, UINT32_C(0x00010000)},
    {3367736u, 19u, 21u, UINT32_C(0x00010000)},
    {3368164u, 0u, 2u, UINT32_C(0x00400000)},
    {3368164u, 3u, 4u, UINT32_C(0x00600000)},
    {3368164u, 5u, 5u, UINT32_C(0x00400000)},
    {3368336u, 0u, 0u, UINT32_C(0x00060000)},
    {3368336u, 1u, 1u, UINT32_C(0x00040000)},
    {3368336u, 2u, 3u, UINT32_C(0x00060000)},
    {3368336u, 4u, 8u, UINT32_C(0x00040000)},
    {3368336u, 9u, 9u, UINT32_C(0x00060000)},
    {3368336u, 10u, 13u, UINT32_C(0x00040000)},
    {3368336u, 15u, 18u, UINT32_C(0x00040000)},
    {3368336u, 21u, 22u, UINT32_C(0x00040000)},
    {3368336u, 24u, 25u, UINT32_C(0x00040000)},
    {3368828u, 0u, 5u, UINT32_C(0x01000000)},
    {3369000u, 0u, 1u, UINT32_C(0x01000000)},
    {3369000u, 4u, 5u, UINT32_C(0x01000000)},
    {3369724u, 0u, 1u, UINT32_C(0x00006000)},
    {3369724u, 2u, 2u, UINT32_C(0x00004000)},
    {3369724u, 3u, 4u, UINT32_C(0x00002000)},
    {3369724u, 5u, 6u, UINT32_C(0x00006000)},
    {3369724u, 7u, 8u, UINT32_C(0x00002000)},
    {3369724u, 9u, 9u, UINT32_C(0x00006000)},
    {3369724u, 10u, 12u, UINT32_C(0x00002000)},
    {3369724u, 13u, 14u, UINT32_C(0x00006000)},
    {3369724u, 15u, 16u, UINT32_C(0x00002000)},
    {3369724u, 17u, 19u, UINT32_C(0x00006000)},
    {3369724u, 20u, 20u, UINT32_C(0x00002000)},
    {3369724u, 21u, 21u, UINT32_C(0x00006000)},
    {3370152u, 0u, 1u, UINT32_C(0x00004000)},
    {3370260u, 0u, 1u, UINT32_C(0x00400000)},
    {3370368u, 0u, 1u, UINT32_C(0x00400000)},
    {3370476u, 0u, 1u, UINT32_C(0x00004000)},
    {3370476u, 4u, 5u, UINT32_C(0x00004000)},
    {3370476u, 6u, 7u, UINT32_C(0x00006000)},
    {3370476u, 8u, 9u, UINT32_C(0x00004000)},
};

static uint64_t streets_fnv1a64(const uint8_t *data, size_t size)
{
    uint64_t hash = UINT64_C(14695981039346656037);
    size_t index;

    for (index = 0; index < size; ++index) {
        hash ^= data[index];
        hash *= UINT64_C(1099511628211);
    }
    return hash;
}

int jpb_IsMatchedStreetsJpx(const JPBJpxView *view)
{
    const char *materialOne;
    const char *materialSix;

    if (view == NULL || view->data == NULL ||
        view->size != 3370796u || view->numMaterials != 13u ||
        view->firstStripOffset != 28u) {
        return 0;
    }
    materialOne = jpx_GetMaterialName(view, 1);
    materialSix = jpx_GetMaterialName(view, 6);
    return materialOne != NULL && materialSix != NULL &&
           strcmp(materialOne, "O_STREEU.TGA") == 0 &&
           strcmp(materialSix, "P_STREEV.TGA") == 0 &&
           streets_fnv1a64(view->data, view->size) ==
               UINT64_C(0x77f091ac13a9987e);
}

uint32_t jpb_StreetsJpxTriangleCullMask(
    const JPBJpxPatchSite *site, uint16_t triangle_index)
{
    size_t low = 0;
    size_t high =
        sizeof(streetsCullRanges) / sizeof(streetsCullRanges[0]);

    if (site == NULL || site->offset > UINT32_MAX) {
        return 0;
    }
    while (low < high) {
        size_t middle = low + (high - low) / 2;

        if (streetsCullRanges[middle].siteOffset <
            (uint32_t)site->offset) {
            low = middle + 1;
        } else {
            high = middle;
        }
    }
    while (low <
           sizeof(streetsCullRanges) / sizeof(streetsCullRanges[0]) &&
           streetsCullRanges[low].siteOffset ==
               (uint32_t)site->offset) {
        if (triangle_index >= streetsCullRanges[low].firstTriangle &&
            triangle_index <= streetsCullRanges[low].lastTriangle) {
            return streetsCullRanges[low].cullMask;
        }
        ++low;
    }
    return 0;
}

int jpb_StreetsJpxCullMaskVisible(uint32_t mask)
{
    size_t index;

    if (mask == 0) {
        return 1;
    }
    for (index = 0; index < JPB_CULL_MESH_COUNT; ++index) {
        if ((mask & (UINT32_C(1) << index)) != 0 &&
            cullmesh[index] == 1) {
            return 1;
        }
    }
    return 0;
}

